// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#include "SerialPort.h"

#include "stringUtils.h"

#include <ChimeraTK/Exception.h>

#include <fcntl.h>
#include <termios.h> //For termain IO interface
#include <unistd.h>  //POSIX OS API

#include <cerrno>  //for errno
#include <chrono>  //Needed for timeout
#include <cstring> //Used for memset, strerrorname_np, strerrordesc_np
#include <future>  //Needed for timeout
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

namespace ChimeraTK {

  /********************************************************************************************************************/

  speed_t getSpeedFlag(unsigned int baudRate) {
    switch(baudRate) {
      // Standard POSIX Baud Rates
      case 0:
        return B0;
      case 50:
        return B50;
      case 75:
        return B75;
      case 110:
        return B110;
      case 134:
        return B134;
      case 150:
        return B150;
      case 200:
        return B200;
      case 300:
        return B300;
      case 600:
        return B600;
      case 1200:
        return B1200;
      case 1800:
        return B1800;
      case 2400:
        return B2400;
      case 4800:
        return B4800;
      case 9600:
        return B9600;
      case 19200:
        return B19200;
      case 38400:
        return B38400;

      // Common Extensions (Linux/BSD)
      case 57600:
        return B57600;
      case 115200:
        return B115200;
      case 230400:
        return B230400;

#ifdef B460800
      case 460800:
        return B460800;
#endif
#ifdef B500000
      case 500000:
        return B500000;
#endif
#ifdef B576000
      case 576000:
        return B576000;
#endif
#ifdef B921600
      case 921600:
        return B921600;
#endif
#ifdef B1000000
      case 1000000:
        return B1000000;
#endif
#ifdef B1152000
      case 1152000:
        return B1152000;
#endif
#ifdef B1500000
      case 1500000:
        return B1500000;
#endif
#ifdef B2000000
      case 2000000:
        return B2000000;
#endif
#ifdef B2500000
      case 2500000:
        return B2500000;
#endif
#ifdef B3000000
      case 3000000:
        return B3000000;
#endif
#ifdef B3500000
      case 3500000:
        return B3500000;
#endif
#ifdef B4000000
      case 4000000:
        return B4000000;
#endif

      default:
        throw ChimeraTK::logic_error(
            std::string("Baud rate ") + std::to_string(baudRate) + " not supported by CommandBasedBackend.");
    }
  }

  /********************************************************************************************************************/

  SerialPort::SerialPort(const std::string& device, uint32_t baudRate) {
    _fileDescriptor = open(device.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK); // from fcntl
                                                                            // O_RDWR = open for read + write
                                                                            // O_RDONLY = open read only
                                                                            // O_WRONLY = open write only
                                                                            // O_NOCTTY = no controlling termainl
    if(_fileDescriptor == -1) {
      std::string err = "Unable to open device \"" + device + "\"";
      throw ChimeraTK::runtime_error(err);
    }

    // Make an instance tty of the termios structure.
    // termios::tcgetattr reads the current terminal settings into the tty structure.
    // throw if this fails
    termios tty{};
    if(tcgetattr(_fileDescriptor, &tty) != 0) {
      std::string err = "Error from tcgetattr\n";
      throw ChimeraTK::runtime_error(err);
    }

    // Set baud rate using termios.
    auto speedFlag = getSpeedFlag(baudRate);
    int iCfsetospeed = cfsetospeed(&tty, speedFlag);
    int iCfsetispeed = cfsetispeed(&tty, speedFlag);
    if(iCfsetospeed < 0 or iCfsetispeed < 0) {
      std::string err = "Error setting IO speed\n";
      throw ChimeraTK::runtime_error(err);
    }
    // see https://www.man7.org/linux/man-pages/man3/termios.3.html
    // NOLINTBEGIN(hicpp-signed-bitwise)
    tty.c_cflag &= ~PARENB; // disables parity generation and detection.
    tty.c_cflag &= ~CSTOPB; // Use 1 stop bit, not 2
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;            // Use 8 bit chars.
    tty.c_cflag &= ~CRTSCTS;       // no flow control
    tty.c_lflag &= ~ICANON;        // Set non-canonical mode so VMIN and VTIME are effective
    tty.c_cc[VMIN] = 0;            // VMIN defines the minimum number of characters to read
    tty.c_cc[VTIME] = 0;           // 0.5 seconds read timeout //FIXME hacked to be 0 for polling read test
    tty.c_cflag |= CREAD | CLOCAL; // turn on READ & ignore ctrl lines
                                   // NOLINTEND(hicpp-signed-bitwise)

    // Flush Port, then applies attributes.
    tcflush(_fileDescriptor, TCIFLUSH);
    if(tcsetattr(_fileDescriptor, TCSANOW, &tty) != 0) { // from termios
      std::string err = "Error from tcsetattr\n";
      throw ChimeraTK::runtime_error(err);
    }
  } // end SerialPort constructor

  /********************************************************************************************************************/

  SerialPort::~SerialPort() {
    close(_fileDescriptor);
  }

  /********************************************************************************************************************/

  void SerialPort::send(const std::string& str) const {
    ssize_t bytesWritten = write(_fileDescriptor, str.c_str(), str.size()); // unistd::write

    if(bytesWritten != static_cast<ssize_t>(str.size())) {
      std::string err = "Incomplete write";
      throw ChimeraTK::runtime_error(err);
    }
  }

  /********************************************************************************************************************/

  std::optional<std::string> SerialPort::readline(const std::string& delimiter) noexcept {
    size_t delimPos;
    static constexpr int readBufferLen = 256;
    std::vector<char> readBuffer(readBufferLen);

    _terminateRead = false;
    // Search for delimiter in persistentBufferStr. While it's not there, read into persistentBufferStr and try again.
    for(delimPos = _persistentBufferStr.find(delimiter); delimPos == std::string::npos;
        delimPos = _persistentBufferStr.find(delimiter)) {
      ssize_t __attribute__((unused)) bytesRead =
          read(_fileDescriptor, readBuffer.data(), readBuffer.size() - 1); // unistd::read

      if(_terminateRead) {
        return std::nullopt;
      }
      // The -1 makes room for read to insert a '\0' null termination at the end.
      _persistentBufferStr += std::string(readBuffer.data());

      if(bytesRead == 0) {
        usleep(1000);
      }
    }

    // Now the delimiter has been found at position delimPos.
    std::string outputStr = _persistentBufferStr.substr(0, delimPos);
    _persistentBufferStr = _persistentBufferStr.substr(delimPos + delimiter.size());
    return outputStr;
  } // end readline

  /********************************************************************************************************************/

  std::optional<std::string> SerialPort::readBytes(const size_t nBytesToRead) {
    // Read until we've read nBytesToRead, with possible interrupts through _terminateRead
    if(nBytesToRead == 0) {
      return "";
    }

    std::string outputBuffer;
    outputBuffer.reserve(nBytesToRead);
    std::vector<char> readBuffer(nBytesToRead);
    ssize_t bytesRead;

    for(size_t totalBytesRead = 0; totalBytesRead < nBytesToRead; totalBytesRead += bytesRead) {
      bytesRead = read(_fileDescriptor, readBuffer.data(), nBytesToRead - totalBytesRead); // unistd::read

      if(_terminateRead) {
        return std::nullopt;
      }

      if(bytesRead > 0) {
        // Append the read bytes to outputBuffer
        outputBuffer.append(readBuffer.data(), bytesRead);
      }
      else if(bytesRead == 0) { // read EOF, wait for reconnection.
        usleep(1000);
      }
      else {
        std::ostringstream errorMsg;
        errorMsg << "Read error: " << strerrorname_np(errno) << " (" << strerrordesc_np(errno) << ")";
        throw ChimeraTK::runtime_error(errorMsg.str());
      }
    }

    return outputBuffer;
  }

  /********************************************************************************************************************/

  std::string SerialPort::readlineWithTimeout(const std::chrono::milliseconds& timeout, const std::string& delimiter) {
    auto future =
        std::async(std::launch::async, [&]() -> std::optional<std::string> { return this->readline(delimiter); });

    if(future.wait_for(timeout) == std::future_status::timeout) {
      // In case of a timeout we have to tell the read to stop so the
      // async thread can complete.
      terminateRead();
      std::string err = "readline operation timed out.";
      throw ChimeraTK::runtime_error(err);
    }

    // No timeout occured, the readData optional should have data.
    auto readData = future.get();
    if(not readData.has_value()) {
      // read was abandoned or failed
      throw ChimeraTK::runtime_error("readline failed to return a value.");
    }
    return readData.value();
  } // end readlineWithTimeout

  /********************************************************************************************************************/
  std::string SerialPort::readBytesWithTimeout(const size_t nBytesToRead, const std::chrono::milliseconds& timeout) {
    if(nBytesToRead == 0) {
      return "";
    }
    auto future =
        std::async(std::launch::async, [&]() -> std::optional<std::string> { return this->readBytes(nBytesToRead); });

    if(future.wait_for(timeout) == std::future_status::timeout) {
      // If the code has not returned, then there has been a timeout or read has been abandoned.

      // In case of a timeout we have to tell the read to stop so the
      // async thread can complete.
      terminateRead();
      std::string err = "readBytes operation timed out.";
      throw ChimeraTK::runtime_error(err);
    }

    // No timeout occured, the readData optional should have data.
    auto readData = future.get();
    if(not readData.has_value()) {
      // read was abandoned or failed
      throw ChimeraTK::runtime_error("readBytes failed to return a value.");
    }
    return readData.value();
  }

  /********************************************************************************************************************/

  void SerialPort::terminateRead() {
    _terminateRead = true;
  }

} // namespace ChimeraTK

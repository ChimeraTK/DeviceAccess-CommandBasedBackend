// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#include "SerialPort.h"

#include "stringUtils.h"

#include <ChimeraTK/Exception.h>

#include <chrono>  //Needed for timeout
#include <cstring> //Used for memset
#include <errno.h> //for errno
#include <fcntl.h>
#include <future> //Needed for timeout
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <termios.h> //For termain IO interface
#include <unistd.h>  //POSIX OS API

namespace ChimeraTK {
  /********************************************************************************************************************/

  SerialPort::SerialPort(const std::string& device) {
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
    int iCfsetospeed = cfsetospeed(&tty, (speed_t)B9600);
    int iCfsetispeed = cfsetispeed(&tty, (speed_t)B9600);
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
    std::vector<char> readBuffer(readBufferLen, '\0'); // Initialize with zeros

    _terminateRead = false;
    // Search for delimiter in persistentBufferStr. While it's not there, read into persistentBufferStr and try again.
    for(delimPos = _persistentBufferStr.find(delimiter); delimPos == std::string::npos;
        delimPos = _persistentBufferStr.find(delimiter)) {
      std::fill(readBuffer.begin(), readBuffer.end(), '\0');
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
    return std::make_optional(outputStr);
  } // end readline

  /********************************************************************************************************************/

  std::optional<std::string> SerialPort::readBytes(const size_t nBytesToRead) {
    // Read until we've read nBytesToRead, with possible interrupts through _terminateRead
    if(nBytesToRead == 0) {
      return "";
    }

    std::string outputBuffer;
    outputBuffer.reserve(nBytesToRead);
    std::string readBuffer(nBytesToRead, '\0');
    size_t bytesRead;

    for(size_t totalBytesRead = 0; totalBytesRead < nBytesToRead; totalBytesRead += bytesRead) {
      bytesRead = read(_fileDescriptor, &readBuffer[0], nBytesToRead - totalBytesRead); // unistd::read

      if(_terminateRead) {
        return std::nullopt;
      }

      if(bytesRead > 0) {
        // Append the read bytes to outputBuffer
        outputBuffer.append(readBuffer.data(), bytesRead);

        // Reset readBuffer for next iteration
        std::fill(readBuffer.begin(), readBuffer.begin() + bytesRead, '\0');
      }
      else if(bytesRead == 0) { // read EOF, wait for reconnection.
        usleep(1000);
      }
      else {
        char errorMsg[256];
        sprintf(errorMsg, "Read error: %s (%s)", strerrorname_np(errno), strerrordesc_np(errno));
        throw ChimeraTK::runtime_error(std::string(errorMsg));
      }
    }

    return std::make_optional(outputBuffer);
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

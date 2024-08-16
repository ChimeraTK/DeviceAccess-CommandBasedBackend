// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#include "SerialPort.h"

#include <chrono>  //needed for timout
#include <cstring> //used for memset
#include <fcntl.h>
#include <future> //needed for timeout
#include <iostream>
#include <string>
#include <termios.h> //for termain IO interface
#include <unistd.h>  //POSIX OS API

/**********************************************************************************************************************/

SerialPort::SerialPort(const std::string& device, const std::string& delimiter)
: delim(delimiter.size() < 1 ? "\n" : delimiter), delim_size(delim.size()) {
  fileDescriptor = open(device.c_str(), O_RDWR | O_NOCTTY); // from fcntl
                                                            // O_RDWR = open for read + write
                                                            // O_RDONLY = open read only
                                                            // O_WRONLY = open write only
                                                            // O_NOCTTY = no controlling termainl
  if(fileDescriptor == -1) {
    std::string err = "Unable to open device " + device + "\n";
    throw std::runtime_error(err);
  }

  termios tty; // Make an instance of the termios structure
  // tcgetattr reads the current terminal settings into the tty structure.
  // throw if this fails
  if(tcgetattr(fileDescriptor, &tty) != 0) { // from termios
    std::string err = "Error from tcgetattr\n";
    throw std::runtime_error(err);
  }

  // Set baud rate, using termios
  int iCfsetospeed = cfsetospeed(&tty, (speed_t)B9600);
  int iCfsetispeed = cfsetispeed(&tty, (speed_t)B9600);
  if(iCfsetospeed < 0 or iCfsetispeed < 0) {
    std::string err = "Error setting IO speed\n";
    throw std::runtime_error(err);
  }
  // see https://www.man7.org/linux/man-pages/man3/termios.3.htmlL
  tty.c_cflag &= ~PARENB; // disables parity generation and detection.
  tty.c_cflag &= ~CSTOPB; // Use 1 stop bit, not 2
  tty.c_cflag &= ~CSIZE;
  tty.c_cflag |= CS8;            // Use 8 bit chars.
  tty.c_cflag &= ~CRTSCTS;       // no flow control
  tty.c_lflag &= ~ICANON;        // Set non-canonical mode so VMIN and VTIME are effective
  tty.c_cc[VMIN] = 0;            // 0: read blocks,  1: read doesn't block
                                 // VMIN defines the minimum number of characters to read
  tty.c_cc[VTIME] = 5;           // 0.5 seconds read timeout //Really??
  tty.c_cflag |= CREAD | CLOCAL; // turn on READ & ignore ctrl lines

  // Flush Port, then applies attributes
  tcflush(fileDescriptor, TCIFLUSH);
  if(tcsetattr(fileDescriptor, TCSANOW, &tty) != 0) { // from termios
    std::string err = "Error from tcsetattr\n";
    throw std::runtime_error(err);
  }
} // end SerialPort constructor

/**********************************************************************************************************************/

SerialPort::~SerialPort() {
  close(fileDescriptor);
}

/**********************************************************************************************************************/

void SerialPort::send(const std::string& str) const {
  std::string strToWrite = str + delim;
  int bytesWritten = write(fileDescriptor, strToWrite.c_str(), strToWrite.size()); // from unistd

  if(bytesWritten != static_cast<ssize_t>(strToWrite.size())) {
    std::string err = "Incomplete write";
    throw std::runtime_error(err);
  }
}

/**********************************************************************************************************************/

std::optional<std::string> SerialPort::readline() noexcept {
  size_t delimPos;
  static constexpr int readBufferLen = 256;
  char readBuffer[readBufferLen];

  _terminateRead = false;
  // search for delim in persistentBufferStr. While it's not there, read into persistentBufferStr and try again.
  for(delimPos = _persistentBufferStr.find(delim); delimPos == std::string::npos;
      delimPos = _persistentBufferStr.find(delim)) {
    memset(readBuffer, 0, sizeof(readBuffer));
    ssize_t __attribute__((unused)) bytesRead = read(fileDescriptor, readBuffer, sizeof(readBuffer) - 1); // from unistd
    std::cout << "readLine timeout!" << std::endl;
    if(_terminateRead) {
      return std::nullopt;
    }
    // the -1 makes room for read to insert a '\0' null termination at the end
    _persistentBufferStr += std::string(readBuffer);
  }

  // Now the delimiter has been found at position delimPos.
  std::string outputStr = _persistentBufferStr.substr(0, delimPos);
  _persistentBufferStr = _persistentBufferStr.substr(delimPos + delim_size);
  return std::make_optional(outputStr);
} // end readline

/**********************************************************************************************************************/

std::string SerialPort::readlineWithTimeout(const std::chrono::milliseconds& timeout) {
  auto future = std::async(std::launch::async, [&]() -> std::optional<std::string> { return this->readline(); });
  if(future.wait_for(timeout) != std::future_status::timeout) {
    // no timeout. Let's see if the optional has data
    auto readData = future.get();
    if(readData.has_value()) {
      return readData.value();
    }
  }
  // if the code has not returned there has been a timeout or read has been abandone
  std::string err = "readline operation timed out.";
  throw std::runtime_error(err);
} // end readlineWithTimeout

/**********************************************************************************************************************/

void SerialPort::terminateRead() {
  _terminateRead = true;
}

// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include <atomic>
#include <chrono>
#include <optional>
#include <string>

/**
 * The SerialPort class handles, opens, closes, and
 * gives read/write access to a specified serial port.
 *
 * Usage:
 * SerialPort sp("/dev/pts/8");
 * sp.readline()
 */
class SerialPort {
 public:
  /**
   * Sets-up a bidirectional serial port, and flushes the port.
   *
   * Port settings:
   *  Baud rate = B9600
   *  No parity checking (~PARENB)
   *  Singe stop bit (~CSTOPB)
   *  8-bit character size (CS8)
   *  Ignore ctrl lines
   *
   * @param[in] device The serial port address of the device, such as /tmp/virtual-tty
   * @param[in] delmimiter The line delimiter seperating serial port messages.
   * @throws ChimeraTK::runtime_error
   */
  SerialPort(const std::string& device, const std::string& delimiter = "\r\n");

  /**
   * Closes the port.
   */
  ~SerialPort();

  /**
   * Write str into the serial port, with delim delimiter appended.
   * @throws ChimeraTK::runtime_error if the write fails.
   */
  void send(const std::string& str) const;

  /**
   * Read a delim delimited line from the serial port. Result ends in delim.
   * Retruns an empty optional if terminateRead() has been called.
   */
  std::optional<std::string> readline() noexcept;

  /**
   * Read a delim delimited line from the serial port. Result does NOT end in delim
   * @throws ChimeraTK::runtime_error if timeout exceeded.
   */
  std::string readlineWithTimeout(const std::chrono::milliseconds& timeout);

  /**
   * delim is the line delimiter for the serial port communication.
   */
  const std::string delim;

  /**
   * delim_size = delim.size();
   * Must come after delim here due to initaization order.
   */
  const size_t delim_size;

  /**
   * Terminate a blocking read call.
   */
  void terminateRead();

 protected:
  /**
   * The serial port handle.
   */
  int fileDescriptor;

  /**
   * Carries-over the not returned data from readline() call to the next.
   */
  std::string _persistentBufferStr;

  /**
   * Setting this to true using the terminateRead() function interrupts readline().
   */
  std::atomic_bool _terminateRead{false};
}; // end SerialPort

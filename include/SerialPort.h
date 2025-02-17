// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include <atomic>
#include <chrono>
#include <optional>
#include <string>
#include <vector>

namespace ChimeraTK {

  constexpr const char SERIAL_DEFAULT_DELIMITER[] = "\r\n";
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
    explicit SerialPort(const std::string& device);

    /**
     * Closes the port.
     */
    ~SerialPort();

    /**
     * Write str into the serial port, with no delimiter appended.
     * @throws ChimeraTK::runtime_error if the write fails.
     */
    void send(const std::string& str) const;

    void sendBinary(const std::string& hexData) const;

    /**
     * @brief Read a delimited line from the serial port.
     * @param[in] delimiter Line delimiter used to identify the end of the line.
     * @returns an optional string line read from the device, ending with the specified delimiter.
     * An empty optional is retuned if terminateRead() has been called.
     */
    std::optional<std::string> readline(const std::string& delimiter = SERIAL_DEFAULT_DELIMITER) noexcept;

    // NEW
    std::optional<std::vector<unsigned char>> readBytes(const size_t numBytesToRead);

    /**
     * @brief Read a delimiter delimited line from the serial port. Result does NOT end in delimiter
     * @param[in] timeout the timeout in milliseconds
     * @param[in] delimiter The line delimiter
     * @return The responce as a sting
     * @throws ChimeraTK::runtime_error if timeout exceeded.
     */
    std::string readlineWithTimeout(
        const std::chrono::milliseconds& timeout, const std::string& delimiter = SERIAL_DEFAULT_DELIMITER);

    // NEW
    std::vector<unsigned char> readBytesWithTimeout(
        const size_t numBytesToRead, const std::chrono::milliseconds& timeout);

    /**
     */

    /**
     * Terminate a blocking read call.
     */
    void terminateRead();

   protected:
    /**
     * The serial port handle.
     */
    int _fileDescriptor;

    /**
     * Carries-over the not returned data from readline() call to the next.
     */
    std::string _persistentBufferStr;

    /**
     * Setting this to true using the terminateRead() function interrupts readline().
     */
    std::atomic_bool _terminateRead{false};
  }; // end SerialPort

} // namespace ChimeraTK

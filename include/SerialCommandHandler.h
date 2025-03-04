// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once
#include "CommandHandler.h"
#include "SerialPort.h"

#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

/**
 * The SerialCommandHandler class sets up a serial port and provides for read/write,
 * as well as sending commands and reading back response using sendCommand.
 */
class SerialCommandHandler : public CommandHandler {
 public:
  /**
   * @brief Open and setup the serial port, set the readback timeout parameter.
   * @param[in] device The address of the serial device, such as /tmp/virtual-tty
   * @param[in] delimiter Sets the default line delimiter seperating serial communication messages.
   * @param[in] timeoutInMilliseconds The timeout duration in ms
   */
  explicit SerialCommandHandler(const std::string& device,
      const std::string& delimiter = ChimeraTK::SERIAL_DEFAULT_DELIMITER, ulong timeoutInMilliseconds = 1000);

  ~SerialCommandHandler() override = default;

  std::vector<std::string> sendCommandAndReadLines(std::string cmd, size_t nLinesToRead = 1,
      const WritableDelimiter& writeDelimiter = CommandHandlerDefaultDelimiter{},
      const ReadableDelimiter& readDelimiter = CommandHandlerDefaultDelimiter{}) override;

  std::string sendCommandAndReadBytes(
      std::string cmd, size_t nBytesToRead, const WritableDelimiter& writeDelimiter = NoDelimiter{}) override;

  /**
   * @brief Simply sends the command cmd to the serial port with no readback.
   * @param[in] cmd The command to be sent
   * @param[in] writeDelimiter: if not set, the default delimiter set in the constructor is used. Otherwise, this is
   * used. Setting this to a string overrides that delimiter. Set to "" or (preferably) NoDelimiter{} to send cmd as a
   * pure binary sequence.
   */
  inline void write(
      std::string& cmd, const WritableDelimiter& writeDelimiter = CommandHandlerDefaultDelimiter{}) const {
    _serialPort->send(cmd + toString(writeDelimiter));
  }

  /**
   * @brief A simple blocking readline with no timeout. This can wait forever.
   * @param[in] delimiter: if not set, the default delimiter set in the constructor is used. Otherwise, this
   * string setting overrides that delimiter.
   * If set to "", the default delimiter is used.
   * @returns The line read from the serail port.
   * @throws ChimeraTK::logic_error if interface fails to read a value.
   */
  [[nodiscard]] std::string waitAndReadline(
      const ReadableDelimiter& delimiter = CommandHandlerDefaultDelimiter{}) const;

 protected:
  /**
   * The SerialPort handle
   */
  std::unique_ptr<ChimeraTK::SerialPort> _serialPort;
}; // end SerialCommandHandler

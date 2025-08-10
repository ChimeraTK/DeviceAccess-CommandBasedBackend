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

  /**
   * @brief A simple blocking readline with no timeout. This can wait forever.
   * @param[in] delimiter: if not set, the default delimiter set in the constructor is used. Otherwise, this
   * string setting overrides that delimiter.
   * If set to "", the default delimiter is used.
   * @returns The line read from the serail port.
   * @throws ChimeraTK::logic_error if interface fails to read a value.
   */
  [[nodiscard]] std::string waitAndReadline(const Delimiter& delimiter = CommandHandlerDefaultDelimiter{}) const;

 protected:
  std::vector<std::string> sendCommandAndReadLinesImpl(
      std::string cmd, size_t nLinesToRead, const Delimiter& writeDelimiter, const Delimiter& readDelimiter) override;

  std::string sendCommandAndReadBytesImpl(
      std::string cmd, size_t nBytesToRead, const Delimiter& writeDelimiter) override;

  /**
   * The SerialPort handle
   */
  std::unique_ptr<ChimeraTK::SerialPort> _serialPort;
}; // end SerialCommandHandler

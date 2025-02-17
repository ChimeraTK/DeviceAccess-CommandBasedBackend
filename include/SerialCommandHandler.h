// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once
#include "CommandHandler.h"
#include "SerialPort.h"

#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

/**
 * The SerialCommandHandler class sets up a serial port and provides for read/write,
 * as well as sending commands and reading back responce using sendCommand.
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
   * Sends the command cmd to the device and collects the repsonce as a vector of nLinesExpected strings.
   * @param[in] cmd The command to be sent
   * @param[in] nLinesExpected The number of lines expected in reply to the sent command cmd, and the length of hte
   * return vector
   * @returns A vector of strings containing the responce lines.
   * @throws ChimeraTK::runtime_error if those returns do not occur within timeout.
   */
  std::vector<std::string> sendCommand(std::string cmd, size_t nLinesExpected) override; // TODO rename?

  // NEW
  /**
   * Sends the command cmd to the device and collects the repsonce as a vector of bytes.
   * @param[in] cmd The command to be sent
   * @param[in] numBytesToRead The number of bytes expected in reply to the sent command cmd, and the length of the
   * return vector
   * @returns A vector of bytes (uchars) containing the responce lines.
   * @throws ChimeraTK::runtime_error if those returns do not occur within timeout.
   */
  std::vector<unsigned char> sendCommandAndReadBytes(
      std::string cmd, size_t numBytesToRead); // override; TODO restore override

  /**
   * Simply sends the command cmd to the serial port with no readback.
   * @param[in] cmd The command to be sent
   */
  void write(std::string& cmd) const { _serialPort->send(cmd); }

  // NEW
  /**
   * Simply sends the data to the serial port with no readback.
   * @param[in] data The vector of bytes of data to be sent
   */
  void writeBinary(std::string& hexData) const { _serialPort->sendBinary(hexData); }

  /**
   * A simple blocking readline with no timeout. This can wait forever.
   *
   * @returns The line read from the serail port.
   * @throws ChimeraTK::logic_error if interface fails to read a value.
   */
  [[nodiscard]] std::string waitAndReadline() const;

 protected:
  /**
   * The SerialPort handle
   */
  std::unique_ptr<SerialPort> _serialPort;
}; // end SerialCommandHandler

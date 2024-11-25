// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once
#include "CommandHandler.h"
#include "SerialPort.h"

#include <chrono>
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
   * Open and setup the serial port, set the readback timeout parameter.
   *
   * @param device The address of the serial device, such as /tmp/virtual-tty
   * @param delim The line delimiter seperating serial communication messages.
   * @param[in] timeoutInMilliseconds The timeout duration in ms
   */
  SerialCommandHandler(
      const std::string& device, const std::string& delim = "\r\n", ulong timeoutInMilliseconds = 1000);

  ~SerialCommandHandler() override = default;

  /**
   * Send a command to the serial port and read back a single line responce, which is returned.
   * @param[in] cmd The command to be sent
   * @returns The responce text, presumed to be a single line.
   */
  std::string sendCommand(std::string cmd) override;

  /**
   * Sends the command cmd to the device and collects the repsonce as a vector of nLinesExpected strings.
   * @param[in] cmd The command to be sent
   * @param[in] nLinesExpected The number of lines expected in reply to the sent command cmd, and the length of hte
   * return vector
   * @returns A vector of strings containing the responce lines.
   * @throws ChimeraTK::runtime_error if those returns do not occur within timeout.
   */
  std::vector<std::string> sendCommand(std::string cmd, const size_t nLinesExpected) override;

  /**
   * Simply sends the command cmd to the serial port with no readback.
   * @param[in] cmd The command to be sent
   */
  void write(std::string cmd) const { _serialPort->send(cmd); }

  /**
   * A simple blocking readline with no timeout. This can wait forever.
   *
   * @returns The line read from the serail port.
   * @throws ChimeraTK::logic_error if interface fails to read a value.
   */
  std::string waitAndReadline() const;

 protected:
  /**
   * Timeout parameter in milliseconds used to to timeout sendCommand.
   */
  std::chrono::milliseconds _timeout;

  /**
   * The SerialPort handle
   */
  std::unique_ptr<SerialPort> _serialPort;
}; // end SerialCommandHandler

// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once
#include "CommandHandler.h"
#include "TcpSocket.h"

#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace ChimeraTK {

  /**
   * The TcpCommandHandler class sets up a tcp port and provides sendCommand functions.
   */
  class TcpCommandHandler : public CommandHandler {
   public:
    /**
     * Open and setup the tcp port, set the readback timeout parameter.
     * @param[in] host
     * @param[in] port
     * @param[in] timeoutInMilliseconds The timeout duration in ms.
     */
    TcpCommandHandler(const std::string& host, const std::string& port, ulong timeoutInMilliseconds = 1000);

    /**
     * Send a command to the tcp port and read backs a single line responce.
     * @param[in] cmd The command to be sent
     * @returns The responce text, presumed to be a single line.
     */
    std::string sendCommand(std::string cmd) override;

    /**
     * Sends the cmd command to the device and collects the repsonce as a vector of nLinesExpected strings.
     * @param[in] cmd The command to be sent
     * @param[in] nLinesExpected The number of lines expected in reply to the sent command cmd, and the length of hte
     * return vector
     * @returns A vector of strings containing the responce lines.
     */
    std::vector<std::string> sendCommand(std::string cmd, size_t nLinesExpected) override;

    /**
     * Writes a string cmd to the device.
     * Typically cmd is a command.
     * @param[in] cmd The string to be written to the device.
     */
    inline void write(std::string& cmd) const { _tcpDevice->send(cmd); }

    /**
     * Reads from the Tcp network device.
     * @returns One line of text read from the device.
     */
    inline std::string read() { return _tcpDevice->readResponse(); }

   protected:
    std::unique_ptr<TcpSocket> _tcpDevice;
  };

} // namespace ChimeraTK

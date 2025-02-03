// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once
#include "CommandHandler.h"
#include "TcpSocket.h"

#include <memory>
#include <optional>
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
     * @param[in] delimiter Sets the line default line delimiter. This can be overridden on a per-command basis.
     * @param[in] timeoutInMilliseconds The timeout duration in ms.
     */
    TcpCommandHandler(const std::string& host, const std::string& port,
        const std::string& delimiter = ChimeraTK::TCP_DEFAULT_DELIMITER, const ulong timeoutInMilliseconds = 1000);

    std::vector<std::string> sendCommandAndReadLines(std::string cmd, size_t nLinesToRead = 1,
        const WritableDelimiter& writeDelimiter = CommandHandlerDefaultDelimiter{},
        const ReadableDelimiter& readDelimiter = CommandHandlerDefaultDelimiter{}) override;

    std::string sendCommandAndReadBytes(
        std::string cmd, size_t nBytesToRead, const WritableDelimiter& writeDelimiter = NoDelimiter{}) override;

    /**
     * Writes a string cmd to the device.
     * Typically cmd is a command.
     * @param[in] cmd The string to be written to the device.
     * @param[in] overrideDelimiter: if not set, the default delimiter set in the constructor is used. Otherwise, this is used.
     */
    inline void write(
        const std::string& cmd, const WritableDelimiter& writeDelimiter = CommandHandlerDefaultDelimiter{}) const {
      _tcpDevice->send(cmd + toString(writeDelimiter));
    }

   protected:
    std::unique_ptr<TcpSocket> _tcpDevice;
  };

} // namespace ChimeraTK

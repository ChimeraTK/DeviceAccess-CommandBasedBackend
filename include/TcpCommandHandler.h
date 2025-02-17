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

    /**
     * Sends the cmd command to the device and collects the repsonce as a vector of nLinesExpected strings.
     * @param[in] cmd The command to be sent
     * @param[in] nLinesExpected The number of lines expected in reply to the sent command cmd, and the length of hte
     * @param[in] overrideWriteDelimiter if set, overrides the default _delimiter that is set in the constructor for
sending cmd. Can be set to "" to send a raw binary command
   * @param[in] overrideReadDelimiter if set, overrides the default _delimiter that is set in the constructor for
reading back lines.
     * @returns A vector of strings containing the responce lines.
     */
    std::vector<std::string> sendCommandAndReadLines(const std::string cmd, const size_t nLinesExpected = 1,
        const std::optional<std::string>& overrideWriteDelimiter = std::nullopt,
        const std::string& overrideReadDelimiter = "") override;

    /**
     * @param[in] cmd The command to be sent
     */

    /**
     * Writes a string cmd to the device.
     * Typically cmd is a command.
     * @param[in] cmd The string to be written to the device.
     * @param[in] overrideDelimiter: if not set, the default delimiter set in the constructor is used. Otherwise, this is used.
     */
    inline void write(const std::string& cmd, const std::optional<std::string>& overrideDelimiter = std::nullopt) const;

   protected:
    std::unique_ptr<TcpSocket> _tcpDevice;
  };

} // namespace ChimeraTK

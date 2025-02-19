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
     * Sends the cmd command to the device and collects the repsonce as a vector of nLinesToRead strings.
     * @param[in] cmd The command to be sent
     * @param[in] nLinesToRead The number of lines required in reply to the sent command cmd, and the length of hte
     * @param[in] overrideWriteDelimiter if set, overrides the default _delimiter that is set in the constructor for
sending cmd. Can be set to "" to send a raw binary command
   * @param[in] overrideReadDelimiter if set, overrides the default _delimiter that is set in the constructor for
reading back lines.
     * @returns A vector of strings containing the responce lines.
     */
    std::vector<std::string> sendCommandAndReadLines(const std::string cmd, const size_t nLinesToRead = 1,
        const std::optional<std::string>& overrideWriteDelimiter = std::nullopt,
        const std::string& overrideReadDelimiter = "") override;

    /**
     * Sends the command cmd to the device and collects the repsonce as a vector of bytes.
     * @param[in] cmd The command to be sent
     * @param[in] nBytesToRead The number of bytes required in reply to the sent command cmd, and the length of the
     * @param[in] overrideWriteDelimiter if set, overrides the default _delimiter that is set in the constructor for
     * sending cmd. Can be set to "" to send a raw binary command return vector
     * return string
     * @returns A string as a container of bytes containing the responce.
     * @throws ChimeraTK::runtime_error if those returns do not occur within timeout.
     */
    std::string sendCommandAndReadBytes(const std::string cmd, const size_t nBytesToRead,
        const std::optional<std::string>& overrideWriteDelimiter = std::nullopt) override;

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

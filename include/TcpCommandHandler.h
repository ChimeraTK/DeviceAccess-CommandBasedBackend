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
        const std::string& delimiter = ChimeraTK::TCP_DEFAULT_DELIMITER, ulong timeoutInMilliseconds = 1000);

   protected:
    std::vector<std::string> sendCommandAndReadLinesImpl(
        std::string cmd, size_t nLinesToRead, const Delimiter& writeDelimiter, const Delimiter& readDelimiter) override;

    std::string sendCommandAndReadBytesImpl(
        std::string cmd, size_t nBytesToRead, const Delimiter& writeDelimiter) override;

    std::unique_ptr<TcpSocket> _tcpDevice;
  };

} // namespace ChimeraTK

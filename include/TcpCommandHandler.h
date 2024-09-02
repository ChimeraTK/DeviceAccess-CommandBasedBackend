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
     */
    TcpCommandHandler(const std::string& host, const std::string& port, ulong timeoutInMilliseconds = 1000);

    /**
     * Send a command to the tcp port and read backs a single line responce.
     */
    std::string sendCommand(std::string cmd) override;

    /**
     * Sends the cmd command to the device and collects the repsonce as a vector of nLinesExpected strings
     */
    std::vector<std::string> sendCommand(std::string cmd, size_t nLinesExpected) override;

    inline void write(std::string cmd) const { _tcpDevice->send(std::move(cmd)); }

    inline std::string read() { return _tcpDevice->readResponse(); }

   protected:
    std::unique_ptr<TcpSocket> _tcpDevice;
  };

} // namespace ChimeraTK

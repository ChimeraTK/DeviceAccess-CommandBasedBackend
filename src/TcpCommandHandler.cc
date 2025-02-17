// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#include "TcpCommandHandler.h"

#include "stringUtils.h"

#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace ChimeraTK {

  /********************************************************************************************************************/

  TcpCommandHandler::TcpCommandHandler(
      const std::string& host, const std::string& port, const std::string& delimiter, const ulong timeoutInMilliseconds)
  : CommandHandler(delimiter, timeoutInMilliseconds) {
    _tcpDevice = std::make_unique<TcpSocket>(host, port);
    _tcpDevice->connect();
  }

  /********************************************************************************************************************/

  std::vector<std::string> TcpCommandHandler::sendCommandAndReadLines(const std::string cmd,
      const size_t nLinesExpected, const std::optional<std::string>& overrideWriteDelimiter,
      const std::string& overrideReadDelimiter) {
    std::vector<std::string> ret;
    std::string readDelimiter = overrideReadDelimiter == "" ? _delimiter : overrideReadDelimiter;
    write(cmd, overrideWriteDelimiter);
    for(size_t line = 0; line < nLinesExpected; ++line) {
      ret.push_back(_tcpDevice->readlineWithTimeout(_timeout, readDelimiter));
    }

    return ret;
  }

  /********************************************************************************************************************/

  inline void TcpCommandHandler::write(
      const std::string& cmd, const std::optional<std::string>& overrideDelimiter) const {
    if(not overrideDelimiter.has_value()) {
      _tcpDevice->send(cmd + _delimiter);
    }
    else {
      _tcpDevice->send(cmd + overrideDelimiter.value());
    }
  }

  /********************************************************************************************************************/
} // namespace ChimeraTK

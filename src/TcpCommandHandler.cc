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

  TcpCommandHandler::TcpCommandHandler(const std::string& host, const std::string& port, ulong timeoutInMilliseconds) {
    _tcpDevice = std::make_unique<TcpSocket>(host, port, timeoutInMilliseconds);
    _tcpDevice->connect();
  }

  /********************************************************************************************************************/

  std::string TcpCommandHandler::sendCommand(std::string cmd) {
    _tcpDevice->send(cmd);
    return _tcpDevice->readResponse();
  }

  /********************************************************************************************************************/

  std::vector<std::string> TcpCommandHandler::sendCommand(std::string cmd, size_t nLinesExpected) {
    std::vector<std::string> ret;
    _tcpDevice->send(cmd);
    for(size_t line = 0; line < nLinesExpected; ++line) {
      ret.push_back(_tcpDevice->readResponse());
    }

    return ret;
  }

  /********************************************************************************************************************/
} // namespace ChimeraTK

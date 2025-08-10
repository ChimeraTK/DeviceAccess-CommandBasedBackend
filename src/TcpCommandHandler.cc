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

  TcpCommandHandler::TcpCommandHandler(const std::string& host, const std::string& port, const std::string& _delimiter,
      const ulong timeoutInMilliseconds)
  : CommandHandler(_delimiter, timeoutInMilliseconds) {
    _tcpDevice = std::make_unique<TcpSocket>(host, port);
    _tcpDevice->connect();
  }

  /********************************************************************************************************************/

  std::vector<std::string> TcpCommandHandler::sendCommandAndReadLinesImpl(
      std::string cmd, size_t nLinesToRead, const Delimiter& writeDelimiter, const Delimiter& readDelimiter) {
    std::vector<std::string> ret;

    _tcpDevice->send(cmd + toString(writeDelimiter));

    std::string delim = toStringGuarded(readDelimiter);
    for(size_t line = 0; line < nLinesToRead; ++line) {
      ret.push_back(_tcpDevice->readlineWithTimeout(timeout, delim));
    }

    return ret;
  }

  /********************************************************************************************************************/

  std::string TcpCommandHandler::sendCommandAndReadBytesImpl(
      std::string cmd, size_t nBytesToRead, const Delimiter& writeDelimiter) {
    _tcpDevice->send(cmd + toString(writeDelimiter));
    return _tcpDevice->readBytesWithTimeout(nBytesToRead, timeout);
  }

  /********************************************************************************************************************/
} // namespace ChimeraTK

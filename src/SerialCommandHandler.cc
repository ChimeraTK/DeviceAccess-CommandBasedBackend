// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#include "SerialCommandHandler.h"

#include "SerialPort.h"
#include "stringUtils.h"

#include <ChimeraTK/Exception.h>

#include <chrono> //Needed for timeout type.
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

/**********************************************************************************************************************/

SerialCommandHandler::SerialCommandHandler(
    const std::string& device, const std::string& _delimiter, ulong timeoutInMilliseconds)
: CommandHandler(_delimiter, timeoutInMilliseconds) {
  _serialPort = std::make_unique<ChimeraTK::SerialPort>(device);
}

/**********************************************************************************************************************/

std::vector<std::string> SerialCommandHandler::sendCommandAndReadLinesImpl(
    std::string cmd, size_t nLinesToRead, const Delimiter& writeDelimiter, const Delimiter& readDelimiter) {
  std::vector<std::string> outputStrVec;
  outputStrVec.reserve(nLinesToRead);

  _serialPort->send(cmd + toString(writeDelimiter));

  if(nLinesToRead == 0) {
    return outputStrVec;
  }

  std::string delim = toStringGuarded(readDelimiter);
  std::string readStr;
  for(size_t nLinesFound = 0; nLinesFound < nLinesToRead; ++nLinesFound) {
    try {
      readStr = _serialPort->readlineWithTimeout(timeout, delim);
    }
    catch(const ChimeraTK::runtime_error& e) {
      std::string err = std::string(e.what()) + " Retrieved:";
      for(auto& s : outputStrVec) {
        err += "\n" + s;
      }
      throw ChimeraTK::runtime_error(err);
    }
    outputStrVec.push_back(readStr);
  }

  return outputStrVec;
}

/**********************************************************************************************************************/

std::string SerialCommandHandler::sendCommandAndReadBytesImpl(
    std::string cmd, size_t nBytesToRead, const Delimiter& writeDelimiter) {
  _serialPort->send(cmd + toString(writeDelimiter));
  return _serialPort->readBytesWithTimeout(nBytesToRead, timeout);
}

/**********************************************************************************************************************/

std::string SerialCommandHandler::waitAndReadline(const Delimiter& readDelimiter) const {
  std::string delim = toStringGuarded(readDelimiter);
  auto readData = _serialPort->readline(delim);
  if(not readData.has_value()) {
    throw std::logic_error("FIXME: BAD INTERFACE");
    // Needs to actually be std::logic_error
    // This is not a ChimeraTK::logic_error since it occurs at runtime
    // Yet it indicates a programming mistake
    // So it's not a ChimeraTK::runtime_error either.
    // FIXME ticket 13739
  }
  return readData.value();
}

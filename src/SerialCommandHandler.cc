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
    const std::string& device, const std::string& delimiter, ulong timeoutInMilliseconds)
: CommandHandler(delimiter, timeoutInMilliseconds) {
  _serialPort = std::make_unique<ChimeraTK::SerialPort>(device);
}

/**********************************************************************************************************************/

void SerialCommandHandler::write(std::string& cmd, const std::optional<std::string>& overrideDelimiter) const {
  if(not overrideDelimiter.has_value()) {
    _serialPort->send(cmd + _delimiter);
  }
  else {
    _serialPort->send(cmd + overrideDelimiter.value());
  }
} // end write

/**********************************************************************************************************************/

std::vector<std::string> SerialCommandHandler::sendCommand(std::string cmd, const size_t nLinesExpected) {
  std::vector<std::string> outputStrVec;
  outputStrVec.reserve(nLinesExpected);

  _serialPort->send(cmd);

  std::string readStr;
  for(size_t nLinesFound = 0; nLinesFound < nLinesExpected; ++nLinesFound) {
    try {
      readStr = _serialPort->readlineWithTimeout(_timeout);
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

std::string SerialCommandHandler::waitAndReadline() const {
  auto readData = _serialPort->readline();
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

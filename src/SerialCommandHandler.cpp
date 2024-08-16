// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#include "SerialCommandHandler.h"

#include "SerialPort.h"
#include "stringUtils.h"

#include <chrono> //needed for timout type
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

/**********************************************************************************************************************/

SerialCommandHandler::SerialCommandHandler(const char* device, ulong timeoutInMilliseconds)
: _timeout(timeoutInMilliseconds) {
  _serialPort = std::make_unique<SerialPort>(device);
}

/**********************************************************************************************************************/

std::string SerialCommandHandler::sendCommand(std::string cmd) {
  _serialPort->send(cmd);
  return _serialPort->readlineWithTimeout(_timeout);
}

/**********************************************************************************************************************/

std::vector<std::string> SerialCommandHandler::sendCommand(std::string cmd, const size_t nLinesExpected) {
  /**
   * Sends the cmd command to the device and collects the repsonce as a vector of nLinesExpected strings
   * If those returns do not occur within timeout, throws std::runtime_error
   */

  std::vector<std::string> outputStrVec;
  outputStrVec.reserve(nLinesExpected);

  _serialPort->send(cmd);

  std::string readStr;
  std::vector<std::string> readStrParsed;
  size_t nLinesFound = 0;
  for(; nLinesFound < nLinesExpected; nLinesFound += readStrParsed.size()) {
    try {
      readStr = _serialPort->readlineWithTimeout(_timeout);
    }
    catch(const std::runtime_error& e) {
      std::string err = std::string(e.what()) + " Retrieved:";
      for(auto& s : outputStrVec) {
        err += "\n" + s;
      }
      throw std::runtime_error(err);
    }
    readStrParsed = parseStr(readStr, _serialPort->delim);
    outputStrVec.insert(outputStrVec.end(), readStrParsed.begin(), readStrParsed.end());
  } // end for
  if(nLinesFound > nLinesExpected) {
    std::string err = "Error: Found " + std::to_string(nLinesFound - nLinesExpected) + " more lines than expected";
    throw std::runtime_error(err);
  }
  return outputStrVec;
} // end sendCommand

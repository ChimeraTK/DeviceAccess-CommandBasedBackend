// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#include <iostream>
#include <cstring>
#include <string>
#include <vector>
#include <chrono> //needed for timout type
#include <stdexcept>
#include "SerialPort.h"
#include "SerialCommandHandler.h"
#include "stringUtils.h"

/**********************************************************************************************************************/

SerialCommandHandler::SerialCommandHandler(const char* device,ulong timeoutInMilliseconds){
    setTimeout(timeoutInMilliseconds);
    serialPort = new SerialPort(device);
}

/**********************************************************************************************************************/

std::string SerialCommandHandler::sendCommand(std::string cmd) {
    serialPort->send(cmd);
    return serialPort->readlineWithTimeout(timeout); 
}

/**********************************************************************************************************************/

std::vector<std::string> SerialCommandHandler::sendCommand( std::string cmd, const size_t nLinesExpected) {
    /**
     * Sends the cmd command to the device and collects the repsonce as a vector of nLinesExpected strings 
     * If those returns do not occur within timeout, throws std::runtime_error
     */

    std::vector<std::string> outputStrVec;
    outputStrVec.reserve(nLinesExpected);

    serialPort->send(cmd);

    std::string readStr;                                                           
    std::vector<std::string> readStrParsed;
    size_t nLinesFound = 0;
    for(; nLinesFound < nLinesExpected; nLinesFound += readStrParsed.size()){
        try{
            readStr = serialPort->readlineWithTimeout(timeout);
        } 
        catch(const std::runtime_error& e) {
            std::string err = std::string(e.what()) + " Retreived:";
            for(auto s : outputStrVec){
                err += "\n" + s;
            }
            throw std::runtime_error(err);
        }
        readStrParsed = parseStr(readStr, serialPort->delim );
        outputStrVec.insert(outputStrVec.end(), readStrParsed.begin(), readStrParsed.end());
    }//end for
    if(nLinesFound > nLinesExpected){
        std::string err = "Error: Found "+std::to_string(nLinesFound - nLinesExpected)+" more lines than expected";
        throw std::runtime_error(err);
    }
    return outputStrVec;
}//end sendCommand


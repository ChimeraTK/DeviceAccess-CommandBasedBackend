#include <iostream>
#include <cstring>
#include <string>
#include <vector>
#include <future> //needed for timeout
#include <chrono> //needed for timout
#include <stdexcept>
#include <fstream>
#include "SerialPort.h"
#include "SerialCommandHandler.h"

//*********************************************************************************************************************
std::vector<std::string> parse(const std::string& stringToBeParsed, const std::string& delimeter) {
    std::vector<std::string> lines;
    size_t pos = 0;

    while (pos != std::string::npos) {
        size_t nextPos = stringToBeParsed.find(delimeter, pos);
        std::string line = stringToBeParsed.substr(pos, nextPos - pos);
        lines.push_back(line);

        // Move the position to the next delimeter
        if (nextPos != std::string::npos) {
            pos = nextPos + delimeter.length();
        } else {
            break;
        }
    }

    return lines;
} //end parse

//*********************************************************************************************************************
SerialCommandHandler::SerialCommandHandler(const char* device,ulong timeoutInMilliseconds){
    setTimeout(timeoutInMilliseconds);
    serialPort = new SerialPort(device);
}

//*****************************************************************************************************************
SerialCommandHandler::~SerialCommandHandler(){ delete serialPort; }

//*****************************************************************************************************************
inline void SerialCommandHandler::setTimeout(const ulong& timeoutInMilliseconds){
    timeout =  std::chrono::milliseconds(timeoutInMilliseconds); 
}

//*****************************************************************************************************************
/*
   std::string SerialCommandHandler::readlineWithTimeout(){
   auto future = std::async(std::launch::async, 
   [this]()->std::string{ 
   return serialPort->readline();
   }
   );
   if(future.wait_for(timeout) != std::future_status::timeout) {
   return future.get();
   } else{
   std::string err = "readline operation timed out.";
   throw std::runtime_error(err);
   }
   } //end readlineWithTimeout
   */

//*****************************************************************************************************************
std::string SerialCommandHandler::sendCommand(std::string cmd) {
    serialPort->send(cmd);
    return serialPort->readlineWithTimeout(timeout); 
    //return readlineWithTimeout();
    //return replaceNewlines(serialPort->readlineWithTimeout(timeout)); //DEBUG
}

//*****************************************************************************************************************
void SerialCommandHandler::write(std::string cmd){ 
    serialPort->send(cmd);
}

//*****************************************************************************************************************
std::string SerialCommandHandler::waitAndReadline(){ 
    return serialPort->readline();
}

//*****************************************************************************************************************
std::vector<std::string> SerialCommandHandler::sendCommand( std::string cmd, const size_t nLinesExpected) {
    /**
     * Sends the cmd command to the device and collects the repsonce as a vector of nLinesExpected strings 
     * If those returns do not occur within timeout, throws std::runtime_error
     */

    std::vector<std::string> outputStrVec;
    outputStrVec.reserve(nLinesExpected);

    std::cout<<"Start multi-line test: "<<cmd<<std::endl;//DEBUG
    serialPort->send(cmd);

    std::string readStr;                                                           
    std::vector<std::string> readStrParsed;
    size_t nLinesFound = 0;
    for(; nLinesFound < nLinesExpected; nLinesFound += readStrParsed.size()){
        std::cout<<"line="<<nLinesFound<<" of "<< nLinesExpected <<std::endl;//DEBUG

        try{
            readStr = serialPort->readlineWithTimeout(timeout); //readlineWithTimeout();
        } 
        catch(const std::runtime_error& e) {
            std::string err = std::string(e.what()) + " Retreived:";//DEBUG
            for(auto s : outputStrVec){
                err += "\n" + s;
            }
            std::cerr<<err<<std::endl; //DEBUG
            throw std::runtime_error(err);
        }
        readStrParsed = parse(readStr, serialPort->getDelim() );
        outputStrVec.insert(outputStrVec.end(), readStrParsed.begin(), readStrParsed.end());
    }//end loop
    if(nLinesFound > nLinesExpected){
        std::string err = "Error: Found "+std::to_string(nLinesFound - nLinesExpected)+" more lines than expected";
        throw std::runtime_error(err);
    }
    return outputStrVec;
}//end sendCommand



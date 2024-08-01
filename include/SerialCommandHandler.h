#pragma once
#include <string>
#include <vector>
#include "CommandHandler.h"
#include "SerialPort.h"
#include <chrono>

std::vector<std::string> parse(const std::string& stringToBeParsed, const std::string& delimeter = "\r\n");

//*********************************************************************************************************************
class SerialCommandHandler : CommandHandler{
    public:
        SerialCommandHandler(const char* device,ulong timeoutInMilliseconds = 1000);
        ~SerialCommandHandler();



        std::string sendCommand(std::string cmd) override;

        std::vector<std::string> sendCommand( std::string cmd, const size_t nLinesExpected) override;
        //Sends the cmd command to the device and collects the repsonce as a vector of nLinesExpected strings 

        void write(std::string cmd); //simply sends cmd to port with no readback

        std::string waitAndReadline(); //readline with no timeout, can wait forever

        //std::string readlineWithTimeout(); //Moved into SerialPort class

        inline void setTimeout(const ulong& timeoutInMilliseconds);
    private:
        SerialPort* serialPort;
        std::chrono::milliseconds timeout;
}; //end SerialCommandHandler 


// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once
#include <string>
#include <vector>
#include "CommandHandler.h"
#include "SerialPort.h"
#include <chrono>

/**
 *
 */
class SerialCommandHandler : CommandHandler{
    public:

        /**
         * 
         */
        SerialCommandHandler(const char* device,ulong timeoutInMilliseconds = 1000);

        /**
         * Deletes serialPort, closing the port 
         */
        ~SerialCommandHandler();

        /**
         * 
         */
        std::string sendCommand(std::string cmd) override;

        /**
         * Sends the cmd command to the device and collects the repsonce as a vector of nLinesExpected strings 
         */
        std::vector<std::string> sendCommand( std::string cmd, const size_t nLinesExpected) override;

        /**
         * Simply sends cmd to the serial port with no readback.
         */
        void write(std::string cmd) const; 

        /**
         * A simple blocking readline with no timeout. This can wait forever.
         */
        std::string waitAndReadline() const noexcept; 

        /**
         * A convenience function to set the timeout parameter, used to to timeout sendCommand.
         */
        inline void setTimeout(const ulong& timeoutInMilliseconds) noexcept;

        /**
         * Timeout parameter in milliseconds used to to timeout sendCommand.
         */
        std::chrono::milliseconds timeout;

    protected:
        SerialPort* serialPort;
}; //end SerialCommandHandler 

/**********************************************************************************************************************/

/**
 * Parse a string by the delimiter, and return a vector of the resulting segments
 * no delimiters are present in the resulting segments.
 */
[[nodiscard]] std::vector<std::string> parse(const std::string& stringToBeParsed, const std::string& delimiter = "\r\n") noexcept;

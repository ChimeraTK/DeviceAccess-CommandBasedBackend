// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once
#include <string>
#include <vector>
#include "CommandHandler.h"
#include "SerialPort.h"
#include <chrono>

/**
 * The SerialCommandHandler class sets up a serial port
 * and provides for read/write, as well as sending commands are reading back the responce with sendCommand.
 */
class SerialCommandHandler : CommandHandler{
    public:

        /**
         * Open and setup the serial port, set the readback timeout parameter. 
         */
        SerialCommandHandler(const char* device,ulong timeoutInMilliseconds = 1000);

        /**
         * Deletes serialPort, closing the port 
         */
        ~SerialCommandHandler(){ delete serialPort; }

        /**
         * Send a command to the serial port and read back a single line responce, which is returned.
         */
        std::string sendCommand(std::string cmd) override;

        /**
         * Sends the cmd command to the device and collects the repsonce as a vector of nLinesExpected strings 
         */
        std::vector<std::string> sendCommand( std::string cmd, const size_t nLinesExpected) override;

        /**
         * Simply sends cmd to the serial port with no readback.
         */
        void write(std::string cmd) const { serialPort->send(cmd); }

        /**
         * A simple blocking readline with no timeout. This can wait forever.
         */
        std::string waitAndReadline() const noexcept { return serialPort->readline(); }

        /**
         * A convenience function to set the timeout parameter, used to to timeout sendCommand.
         */
        inline void setTimeout(const ulong& milliseconds) noexcept { timeout=std::chrono::milliseconds(milliseconds); }

        /**
         * Timeout parameter in milliseconds used to to timeout sendCommand.
         */
        std::chrono::milliseconds timeout;

    protected:
        SerialPort* serialPort;
}; //end SerialCommandHandler 


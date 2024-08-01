// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once
#include <string>
#include <chrono>

/**
 * The SerialPort class handles, opens, closes, and 
 * gives read/write access to a specified serial port.
 * 
 * Usage: 
 * SerialPort sp("/dev/pts/8");
 * sp.readline()
 */
class SerialPort {
    public:
        /**
         * Sets-up a bidirectional serial port, and flushes the port. 
         *
         * Can throw std::runtime_error
         *
         * Port settings: 
         *  Baud rate = B9600
         *  No parity checking (~PARENB)
         *  Singe stop bit (~CSTOPB)
         *  8-bit character size (CS8)
         *  Ignore ctrl lines
         */
        SerialPort(const std::string& device, const std::string& delim="\r\n");

        /**
         * Closes the port.
         */
        ~SerialPort();

        /**
         * Write str into the serial port, with _delim delimiter appended 
         * throws std::runtime_error if the write fails
         */
        void send(const std::string& str) const; 

        /**
         * Read a _delim delimited line from the serial port. Result ends in _delim
         */
        std::string readline() const noexcept; 

        /**
         * Read a _delim delimited line from the serial port. Result does NOT end in _delim
         * throws std::runtime_error if timeout exceeded.
         */
        std::string readlineWithTimeout(const std::chrono::milliseconds& timeout) const; 

        /**
         * Returns true if and only if the provided string ends in the delimiter _delim.
         * Forseen only for interal use.
         */
        [[nodiscard]] bool strEndsInDelim(const std::string& str) const noexcept; 

        /**
         * Removes the delimiter _delim from str if it's present and returns the result
         * If no delimiter is found, returns the input
         * Use this to ensure that the resulting string doesn't end in the delimiter
         * Forseen only for interal use.
         */
        [[nodiscard]] std::string stripDelim(const std::string& str) const noexcept; 


        /**
         * delim is the line delimiter for the serial port communication 
         */
        const std::string delim; 

        /**
         * delim_size = delim.size(); Must come after delim here due to initaization order.
         */
        const size_t delim_size;    

     private:                                    
        /**
         * The serial port handle.
         */
        int fileDescriptor;
}; //end SerialPort

/**
* This replaces '\n' with 'N' and '\r' with 'R' and returns the modified string. 
* This is only used for visualizing delimiters during debugging.
*/
[[nodiscard]] std::string replaceNewlines(const std::string& input) noexcept; 

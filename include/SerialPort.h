#pragma once
//#include <iostream>
#include <string>
#include <chrono>
//#include <fstream>
//#include <fcntl.h> 
//#include <unistd.h> //POSIX OS API
//#include <termios.h> //for termain IO interface
//#include <cstring>
//#include <future>

class SerialPort {
    public:
        SerialPort(std::string device, std::string delim="\r\n");
        ~SerialPort();
        void send(const std::string& str); //write str into the serial prot
        std::string readline(); //read a _delim delimited line from the serial port. Result ends in _delim
        std::string readlineWithTimeout(std::chrono::milliseconds timeout); //throws if timeout exceeded.

        std::string getDelim();
        bool strEndsInDelim(std::string str); //Returns true if and only if the provided string ends in the delimiter _delim.
        std::string stripDelim(std::string str); //Removes the delimiter _delim from str

    private:
        int fileDescriptor;
        int _delim_size;    //= _delim.size();
        std::string _delim; //Line delimiter for the serial port
        //std::string _serialDeviceName; //Not currently needed.
}; //end SerialPort

std::string replaceNewlines(const std::string& input); //for Debugging

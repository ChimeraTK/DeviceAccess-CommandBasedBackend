#pragma once
#include <string>
#include <vector>

class CommandHandler{ 
    public:
    virtual std::string sendCommand(std::string cmd) = 0; 
    virtual std::vector<std::string> sendCommand(std::string cmd, const size_t nLinesExpected) = 0; 
};

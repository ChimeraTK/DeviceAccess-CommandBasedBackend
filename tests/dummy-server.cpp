// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#include <iostream>
#include <sstream>
#include <vector>
#include <string>
#include <cstdlib> //for atoi quacking test, DEBUG
#include "SerialPort.h"
#include "SerialCommandHandler.h"
//#include <unistd.h>
//First start:
//socat -d -d pty,raw,echo=0,link=/tmp/virtual-tty pty,raw,echo=0,link=/tmp/virtual-tty-back

/**********************************************************************************************************************/
std::vector<std::string> splitString(const std::string& input, char delimiter) {
    // Parse the string based on the delimiter. Return a vector the resulting strings.
    std::vector<std::string> result;
    std::stringstream ss(input);
    std::string item;
    while (std::getline(ss, item, delimiter)) {
        result.push_back(item);
    }
    return result;
}

/**********************************************************************************************************************/
int main(){
    SerialPort serialPort("/tmp/virtual-tty-back");
    std::cout<<"echoing port /tmp/virtual-tty-back"<<std::endl;//DEBUG

    std::string data;
    long long nIter = 0;
    while(true){

        std::cout<<"dummy-server is patiently listening ("<<nIter++<<")..."<<std::endl; //DEBUG
        data = serialPort.readline();

        std::cout<<"rx'ed \""<<replaceNewlines(data)<<"\""<<std::endl;//DEBUG

        if(data.find("send") == 0){ //for the quacking condition //DEBUG section
            int n = std::atoi(data.substr(5).c_str());
            //std::cout<<"repplying "<<n<<" times"<<std::endl;//DEBUG
            for(int i=0;i<n;i++){
                std::string dat = "reply "+std::to_string(i);
                std::cout<<"tx'ing \""<<replaceNewlines(dat)<<"\""<<std::endl;//DEBUG
                serialPort.send(dat);
            }
        } else{
            std::vector<std::string> lines = splitString(data, ';');
            for(const std::string& dat : lines){
                std::cout<<"tx'ing \""<<replaceNewlines(dat)<<"\""<<std::endl;//DEBUG
                serialPort.send(dat);
            }
        }//end else

    }
    return 0;
}


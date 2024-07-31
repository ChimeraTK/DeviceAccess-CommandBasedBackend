//#include <unistd.h>
#include <iostream>
#include <sstream>
#include <vector>
#include <string>
#include <cstdlib> //for atoi quacking test, DEBUG
#include "serialport.h"
//First start:
//socat -d -d pty,raw,echo=0,link=/tmp/virtual-tty pty,raw,echo=0,link=/tmp/virtual-tty-back

//*********************************************************************************************************************
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

//*********************************************************************************************************************
int main(){
    //long sleepTimeMillisec = 1;
    //long sleepTimeMicrosec = 1000L*sleepTimeMillisec;
    SerialPort serialPort("/tmp/virtual-tty-back");
    std::cout<<"echoing port /tmp/virtual-tty-back"<<std::endl;//DEBUG

    std::string data;
    while(true){

        std::cout<<"patiently listening..."<<std::endl; //DEBUG
        data = serialPort.receive();

        std::cout<<"rx'ed "<<replaceNewlines(data)<<std::endl;//DEBUG

        if(data.find("send") == 0){ //for the quacking condition //DEBUG section
            int n = std::atoi(data.substr(5).c_str());
            std::cout<<"quacking "<<n<<" times"<<std::endl;//DEBUG
            for(int i=0;i<n;i++){
                std::string dat = "quack "+std::to_string(i);
                std::cout<<"tx'ing "<<replaceNewlines(dat)<<std::endl;//DEBUG
                serialPort.send(dat);

                //without sleep, fe receives all concatenated onto one line
                //std::cout<<"sleep "<<sleepTimeMillisec<<"ms"<<std::endl;//DEBUG
                //usleep(sleepTimeMicrosec); //sleep for 50ms
            }
        } else{
            std::vector<std::string> lines = splitString(data, ';');
            for(const std::string& dat : lines){
                std::cout<<"tx'ing "<<replaceNewlines(dat)<<std::endl;//DEBUG
                serialPort.send(dat);
                //std::cout<<"sleep "<<sleepTimeMillisec<<"ms"<<std::endl;//DEBUG
                //usleep(sleepTimeMicrosec); //sleep for 50ms
            }
        }//end else

    }
    return 0;
}


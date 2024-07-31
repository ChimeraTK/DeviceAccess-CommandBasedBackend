#include <iostream>
#include <cstring>
#include <string>
#include <vector>
#include <future> //needed for timeout
#include <chrono> //needed for timout
#include <stdexcept>
#include <fstream>
#include "serialport.h"

//Use this with 
//socat -d -d pty,raw,echo=0,link=/tmp/virtual-tty pty,raw,echo=0,link=/tmp/virtual-tty-back
//and be_mylib.cpp

//*********************************************************************************************************************
std::vector<std::string> parse(const std::string& stringToBeParsed, const std::string& delimeter = "\r\n") {
    std::vector<std::string> lines;
    size_t pos = 0;

    while (pos != std::string::npos) {
        // Find the next occurrence of the delimeteriter
        size_t nextPos = stringToBeParsed.find(delimeter, pos);

        // Extract the line between the current position and the next delimeteriter
        std::string line = stringToBeParsed.substr(pos, nextPos - pos);

        // Add the line to the vector
        lines.push_back(line);

        // Move the position to the next line
        if (nextPos != std::string::npos) {
            pos = nextPos + delimeter.length();
        } else {
            // No more delimeteriters found, break out of the loop
            break;
        }
    }

    return lines;
} //end parse

//*********************************************************************************************************************
class CommandHandler{ 
    public:
    virtual std::string sendCommand(std::string cmd) = 0; 
    virtual std::vector<std::string> sendCommand(std::string cmd, const size_t nLinesExpected) = 0; 
};

//*********************************************************************************************************************
class SerialCommandHandler : CommandHandler{

    SerialPort* serialPort;
    std::chrono::milliseconds timeout;

    public:

    //*****************************************************************************************************************
    SerialCommandHandler(const char* device,ulong timeoutInMilliseconds = 1000){
        setTimeout(timeoutInMilliseconds);
        serialPort = new SerialPort(device);
    }

    //*****************************************************************************************************************
    ~SerialCommandHandler(){ delete serialPort; }

    //*****************************************************************************************************************
    inline void setTimeout(const ulong& timeoutInMilliseconds){
        timeout =  std::chrono::milliseconds(timeoutInMilliseconds); 
    }

    //*****************************************************************************************************************
    std::string receiveWithTimeout(){
        auto future = std::async(std::launch::async, 
                [this]()->std::string{ 
                    return serialPort->receive();
                    }
                );
        if(future.wait_for(timeout) != std::future_status::timeout) {
            return future.get();
            /*std::string item = future.get();
            std::cout<<"got "<<item<<std::endl;//DEBUG
            return item;*/
        } else{
            std::string err = "Receive timed out.";
            //std::cerr<<err<<std::endl; //DEBUG
            throw std::runtime_error(err);
        }
    } //end receiveWithTimeout

    //*****************************************************************************************************************
    std::string sendCommand(std::string cmd) override {
        serialPort->send(cmd);
        return receiveWithTimeout();  //Production
        //return replaceNewlines(receiveWithTimeout()); //DEBUG
    }

    //*****************************************************************************************************************
    void tx(std::string cmd){ ///DEBUG
        serialPort->send(cmd);
    }

    //*****************************************************************************************************************
    std::string rx(){ ///DEBUG
        return serialPort->receive();
        //return replaceNewlines(serialPort->receive());
    }

    //*****************************************************************************************************************
    std::vector<std::string> sendCommand( std::string cmd, const size_t nLinesExpected) override {
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
                readStr = receiveWithTimeout() ;
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
}; //end SerialCommandHandler 

//*********************************************************************************************************************
//*********************************************************************************************************************

int main(){
    SerialCommandHandler s("/tmp/virtual-tty");

    std::string res;

    std::cout<<"Test 0: taking turns"<<std::endl;

    for( long l = 0; l<10;l++){
        s.tx("quest" + std::to_string(l) );
        std::cout<<"tx'ed "<<"quest" + std::to_string(l) <<std::endl;
        res = s.rx(); //able to receive just fine after sending.
        std::cout<<"rx'ed "<<replaceNewlines(res)<<std::endl;
    }


    std::cout<<"******************************************************************************************************"<<std::endl;
    std::cout<<"Test 1: testytest"<<std::endl;
    res = s.sendCommand("testytest");
    std::cout<<"rx'ed "<<replaceNewlines(res)<<std::endl;

    std::cout<<"******************************************************************************************************"<<std::endl;
    std::string longstr = "aFirst100aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaabbbb2ndHundredbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbcccc201thru249ccccccccccccccccccccccccccccccccccc012345";
    std::cout<<"Test 255: "<<longstr<<std::endl;
    //res = s.sendCommand(longstr);
    s.tx(longstr);
    res = s.rx();
    std::cout<<"rx'ed "<<replaceNewlines(res)<<std::endl;
    

    std::cout<<"******************************************************************************************************"<<std::endl;
    std::cout<<"Test 1.2: testytest"<<std::endl;
    res = s.sendCommand("testytest");
    std::cout<<"rx'ed "<<replaceNewlines(res)<<std::endl;

    std::cout<<"******************************************************************************************************"<<std::endl;
    longstr = "aFirst100aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaabbbb2ndHundredbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbcccc201thru249ccccccccccccccccccccccccccccccccccc01234";
    std::cout<<"Test 254: "<<longstr<<std::endl;
    res = s.sendCommand(longstr);
    std::cout<<"rx'ed "<<replaceNewlines(res)<<std::endl;

    std::cout<<"******************************************************************************************************"<<std::endl;
    std::cout<<"Test 1.3: testytest"<<std::endl;
    res = s.sendCommand("testytest");
    std::cout<<"rx'ed "<<replaceNewlines(res)<<std::endl;

    std::cout<<"******************************************************************************************************"<<std::endl;
    longstr = "aFirst100aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaabbbb2ndHundredbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbcccc201thru249ccccccccccccccccccccccccccccccccccc0123";
    std::cout<<"Test 253: "<<longstr<<std::endl;
    res = s.sendCommand(longstr);
    std::cout<<"rx'ed "<<replaceNewlines(res)<<std::endl;

    std::cout<<"******************************************************************************************************"<<std::endl;
    std::cout<<"Test 3: questyquest"<<std::endl;
    res = s.sendCommand("questyquest");
    std::cout<<"rx'ed "<<replaceNewlines(res)<<std::endl;
    

    std::cout<<"******************************************************************************************************"<<std::endl;
    std::cout<<"Test 4: send 5"<<std::endl;
    auto ret2 =   s.sendCommand("send 5",5);
    std::cout<<"Rx'ed:"<<std::endl;
    for (const std::string& str : ret2) {
        //std::cout<<str<<std::endl;
        std::cout<<replaceNewlines(str)<<std::endl;
    }

    std::cout<<"******************************************************************************************************"<<std::endl;
    std::cout<<"Test 5: qwer;asdf"<<std::endl;
    auto ret =   s.sendCommand("qwer;asdf",2);
    std::cout<<"Rx'ed:"<<std::endl;
    for (const std::string& str : ret) {
        //std::cout<<str<<std::endl;
        std::cout<<replaceNewlines(str)<<std::endl;
    }
    return 0;
}

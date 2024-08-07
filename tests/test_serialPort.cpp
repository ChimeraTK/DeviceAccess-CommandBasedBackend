// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#include <iostream>
#include <string>
#include <vector>
#include "SerialCommandHandler.h"
#include "SerialPort.h"
#include "stringUtils.h"

//Use this with 
//socat -d -d pty,raw,echo=0,link=/tmp/virtual-tty pty,raw,echo=0,link=/tmp/virtual-tty-back
//and dummy-server

int main(){
    int testCode = 0;
    std::string cmd;
    std::string res;

    SerialCommandHandler s("/tmp/virtual-tty");

    // ****************************************************************************************************************
    testCode = 0; //TEST 0
    for( long l = 0; l<10;l++){
        cmd = "reply" + std::to_string(l);
        s.write( cmd );
        res = s.waitAndReadline(); //able to receive just fine after sending.
        if(res != cmd){
            std::cerr<<"Serail Communication Test 0:\ntx'ed "<<cmd<<"\nrx'ed "<<replaceNewLines(res)<<std::endl;
            return std::max(1,testCode + ((int)l)+1);
        }
    }

    // ****************************************************************************************************************
    testCode = 1000; //TEST 1
    cmd = "test1";
    res = s.sendCommand(cmd);
    if(res != cmd){
        std::cerr<<"Serail Communication Test 1:\ntx'ed "<<cmd<<"\nrx'ed "<<replaceNewLines(res)<<std::endl;
        return std::max(1, (int) (testCode + res.size() - cmd.size()));
    }

    // ****************************************************************************************************************
    testCode = 2000; //TEST 2: cmd string longer than buffer
    cmd = "test2aaafirst100aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaabbbb2ndHundredbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbcccc201thru249ccccccccccccccccccccccccccccccccccc012345";
    //cmd is 255 char long, sending 257 chars
    res = s.sendCommand(cmd);
    if(res != cmd){
        std::cerr<<"Serail Communication Test 2:\ntx'ed "<<cmd<<"\nrx'ed "<<replaceNewLines(res)<<std::endl;
        return std::max(1, (int) (testCode + res.size() - 255));
    }

    // ****************************************************************************************************************
    testCode = 3000; //TEST 3: catch any overflow from test 2
    cmd = "test3";
    res = s.sendCommand(cmd);
    if(res != cmd){
        std::cerr<<"Serail Communication Test 3:\ntx'ed "<<cmd<<"\nrx'ed "<<replaceNewLines(res)<<std::endl;
        return std::max(1, (int) (testCode + res.size() - cmd.size()));
    }
    
    // ****************************************************************************************************************
    testCode = 4000; //TEST 4: 
                    //responce has 2-character delimiter straddle read buffer, so first char of delimiter comes in 
                    //first buffer read, and second delimter char comes in second buffer read
    cmd = "test4aaafirst100aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaabbbb2ndHundredbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbcccc201thru249ccccccccccccccccccccccccccccccccccc01234";
    //cmd is 254 char long, sending 256 chars
    res = s.sendCommand(cmd);
    if(res != cmd){
        std::cerr<<"Serail Communication Test 4:\ntx'ed "<<cmd<<"\nrx'ed "<<replaceNewLines(res)<<std::endl;
        return std::max(1, (int) (testCode + res.size() - 254));
    }

    // ****************************************************************************************************************
    testCode = 5000; //TEST 5: catch any overflow from test 4
    cmd = "test5";
    res = s.sendCommand(cmd);
    if(res != cmd){
        std::cerr<<"Serail Communication Test 5:\ntx'ed "<<cmd<<"\nrx'ed "<<replaceNewLines(res)<<std::endl;
        return std::max(1, (int) (testCode + res.size() - cmd.size()));
    }

    // ****************************************************************************************************************
    testCode = 6000; //TEST 6: command, together with delimiter, is exactly the size of the buffer.
    cmd = "test6aaafirst100aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaabbbb2ndHundredbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbcccc201thru249ccccccccccccccccccccccccccccccccccc0123";
    //cmd is 253 char long, sending 255 chars
    res = s.sendCommand(cmd);
    if(res != cmd){
        std::cerr<<"Serail Communication Test 6:\ntx'ed "<<cmd<<"\nrx'ed "<<replaceNewLines(res)<<std::endl;
        return std::max(1, (int) (testCode + res.size() - 253));
    }

    // ****************************************************************************************************************
    testCode = 7000; //TEST 7: catch any overflow from test 6
    cmd = "test7";
    res = s.sendCommand(cmd);
    if(res != cmd){
        std::cerr<<"Serail Communication Test 7:\ntx'ed "<<cmd<<"\nrx'ed "<<replaceNewLines(res)<<std::endl;
        return std::max(testCode,1);
    }
    
    // ****************************************************************************************************************
    testCode = 8000; //TEST 8: 'send [N]' is a special code in dummy that makes it reply with N lines. 
    int itemCode = 0;
    int n = 5; 
    cmd = "send "+std::to_string(n);
    auto ret2 =   s.sendCommand(cmd,n); //For n=5, expect ret = {"reply 0","reply 1","reply 2","reply 3","reply 4"}
    if(((int) ret2.size()) != n){
        std::cerr<<"Serail Communication Test 8:\nReceived wrong length of responce. Expected "<<n<<" items, received "<<ret2.size()<<std::endl;
        return std::max(1, (int) (testCode + itemCode + n - ret2.size()));//itemCode100
    } else {
        for(int i=0;i<n;i++){
            itemCode = 100*(i+1);
            std::string expectedResponce = "reply "+std::to_string(i);
            if(ret2[i] != expectedResponce ){
                std::cerr<<"Serail Communication Test 8:\nIn reply item "<<i<<" expected "<<expectedResponce<<"\nbut rx'ed "<<ret2[i]<<std::endl;
                return std::max(1, (int) (testCode + itemCode + ret2[i].size() - expectedResponce.size()));
            }

        }//end for
    }

    // ****************************************************************************************************************
    testCode = 9000; //TEST 9: Test semicolon parsing into multiple return lines
    cmd = "qwer;asdf";
    auto ret =   s.sendCommand(cmd,2); //Expect ret = {"qwer","asdf"}
    if(ret.size() != 2){
        std::cerr<<"Serail Communication Test 9:\nReceived wrong length of responce. Expected "<<n<<" items, received "<<ret.size()<<std::endl;
        return std::max(1, (int) (testCode + 0 + 2 - ret.size()));
    } else if(ret[0] != "qwer" or ret[1] != "asdf"){
        std::cerr<<"Serail Communication Test 9:\ntx'ed "<<cmd<<"\nexpected {\"qwer\", \"asdf\"}\n rx'ed: "<<std::endl;
        for (const std::string& str : ret) {
            std::cerr<<replaceNewLines(str)<<std::endl;
        }
        if(ret[0] != "qwer") return std::max(1, (int) (testCode + 100 + ret[0].size() - 4));
        else if(ret[1] != "asdf") return std::max(1, (int) (testCode + 200 + ret[0].size() - 4 ));
    }

    // ****************************************************************************************************************
    return 0; //All tests pass 
} //end main

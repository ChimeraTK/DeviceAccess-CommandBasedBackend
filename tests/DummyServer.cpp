// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#include "DummyServer.h"

#include "SerialPort.h"
#include "stringUtils.h"

#include <cstdlib> //for atoi quacking test, DEBUG
#include <iostream>
#include <string>
#include <vector>

// First start socat:
// socat -d -d pty,raw,echo=0,link=/tmp/virtual-tty pty,raw,echo=0,link=/tmp/virtual-tty-back

void DummyServer::mainLoop() {
  if(_debug) std::cout << "echoing port /tmp/virtual-tty-back" << std::endl; // DEBUG

  std::string data;
  uint64_t nIter = 0;
  while(true) {
    if(_debug) std::cout << "dummy-server is patiently listening (" << nIter++ << ")..." << std::endl; // DEBUG
    data = serialPort.readline();

    if(_debug) std::cout << "rx'ed \"" << replaceNewLines(data) << "\"" << std::endl; // DEBUG

    if(data.find("send") == 0) { // A fake command e.g. "send 4" to test multiline readback //DEBUG section
      if(data.size() < 6) {
        serialPort.send("12345 Syntax error: send command needs an argument");
        continue;
      }
      if(data[4] != ' ') {
        serialPort.send("12345 Syntax error: unknown command: " + data);
        continue;
      }
      int n = std::atoi(data.substr(5).c_str());
      for(int i = 0; i < n; i++) {
        std::string dat = "reply " + std::to_string(i);
        if(_debug) std::cout << "tx'ing \"" << replaceNewLines(dat) << "\"" << std::endl; // DEBUG
        serialPort.send(dat);
      }
    }
    else if(data == "*CLS") {
      if(_debug) {
        std::cout << "Received debug clear command" << std::endl;
      }
    }
    else if(data == "*CLS") {
      if(_debug) {
        std::cout << "Received debug clear command" << std::endl;
      }
    }
    else if(data == "*IDN?") {
      serialPort.send("Dummy server for command based serial backend.");
    }
    else if(data == "SAI?") {
      serialPort.send("AXIS_1");
      serialPort.send("AXIS_2");
    }
    else if(data.find("ACC AXIS_1 ") == 0) {
      setAcc(0, data);
    }
    else if(data.find("ACC AXIS_2 ") == 0) {
      setAcc(1, data);
    }
    else if(data == "ACC?") {
      serialPort.send("AXIS_1=" + std::to_string(acc[0]));
      serialPort.send("AXIS_2=" + std::to_string(acc[1]));
    }
    else if(data == "ACC? AXIS1") {
      serialPort.send("AXIS_1=" + std::to_string(acc[0]));
    }
    else if(data == "ACC? AXIS2") {
      serialPort.send("AXIS_2=" + std::to_string(acc[1]));
    }
    else if(data.find("SOUR:FREQ:CW ") == 0) {
      if(data.size() < 14) {
        serialPort.send("12345 Syntax error: SOUR:FREQ:CW needs an argument");
        continue;
      }
      try {
        cwFrequency = std::stol(data.substr(13));
        if(_debug) {
          std::cout << "Setting cwFrequency to " << cwFrequency << std::endl;
        }
      }
      catch(...) {
        serialPort.send("12345 Syntax error in argument: " + data.substr(13));
      }
    }
    else if(data == "SOUR:FREQ:CW?") {
      serialPort.send(std::to_string(cwFrequency));
    }
    else if(data.find("CALC1:DATA:TRAC? ") == 0) {
      // found the only right trace in this simple dummy
      if(data.find("CALC1:DATA:TRAC? 'myTrace'") == 0) {
        if(data.find("CALC1:DATA:TRAC? 'myTrace' SDAT") == 0) {
          std::string out;
          for(auto& val : trace) {
            out += std::to_string(val) + ",";
          }
          out.pop_back();
          serialPort.send(out);
        }
        else {
          serialPort.send("error: unknow data format");
        }
      }
      else {
        serialPort.send("error: unknown trace");
      }
    }
    else {
      std::vector<std::string> lines = parseStr(data, ';');
      for(const std::string& dat : lines) {
        if(_debug) std::cout << "tx'ing \"" << replaceNewLines(dat) << "\"" << std::endl; // DEBUG
        serialPort.send(dat);
      }
    } // end else
  }
}

void DummyServer::setAcc(size_t i, std::string data) {
  if(data.size() < 12) {
    serialPort.send("12345 Syntax error: ACC AXIS_1 needs an argument");
    return;
  }
  try {
    acc[i] = std::stof(data.substr(11));
    if(_debug) {
      std::cout << "Setting acc[" << i << "] to " << acc[i] << std::endl;
    }
  }
  catch(...) {
    serialPort.send("12345 Syntax error in argument: " + data.substr(11));
  }
}

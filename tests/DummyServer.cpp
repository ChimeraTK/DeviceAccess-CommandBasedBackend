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
  static const bool debug = true;
  if(debug) std::cout << "echoing port /tmp/virtual-tty-back" << std::endl; // DEBUG

  std::string data;
  uint64_t nIter = 0;
  while(true) {
    if(debug) std::cout << "dummy-server is patiently listening (" << nIter++ << ")..." << std::endl; // DEBUG
    data = serialPort.readline();

    if(debug) std::cout << "rx'ed \"" << replaceNewLines(data) << "\"" << std::endl; // DEBUG

    if(data.find("send") == 0) { // A fake command e.g. "send 4" to test multiline readback //DEBUG section
      int n = std::atoi(data.substr(5).c_str());
      for(int i = 0; i < n; i++) {
        std::string dat = "reply " + std::to_string(i);
        if(debug) std::cout << "tx'ing \"" << replaceNewLines(dat) << "\"" << std::endl; // DEBUG
        serialPort.send(dat);
      }
    }
    else {
      std::vector<std::string> lines = parseStr(data, ';');
      for(const std::string& dat : lines) {
        if(debug) std::cout << "tx'ing \"" << replaceNewLines(dat) << "\"" << std::endl; // DEBUG
        serialPort.send(dat);
      }
    } // end else
  }
} // end mainLoop

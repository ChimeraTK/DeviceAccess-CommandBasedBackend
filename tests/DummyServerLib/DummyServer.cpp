// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#include "DummyServer.h"

#include "SerialPort.h"
#include "stringUtils.h"

#include <ChimeraTK/Exception.h>
#include <ChimeraTK/SupportedUserTypes.h>

#include <boost/process.hpp>

#include <cstdlib> //for atoi quacking test, DEBUG
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

DummyServer::DummyServer(bool useRandomDevice) {
  if(useRandomDevice) {
    // generate a random number to attach to the device name to muliple test can run in parallel
    // Initialise with high resolution clock as see so all processes get a different value.
    auto now = std::chrono::high_resolution_clock::now();
    auto epoch = std::chrono::nanoseconds(now.time_since_epoch()).count();
    std::srand(epoch);
    int random_value = std::rand() % 100000; // 5 digits
    deviceNode += std::to_string(random_value);
  }
  _backportNode = deviceNode + "-back";

  activate();
}

DummyServer::~DummyServer() {
  if(_debug) std::cout << "this is ~DummyServer" << std::endl;
  try {
    if(_debug) std::cout << "joining main thread" << std::endl;
    deactivate();
  }
  catch(...) {
    // try/catch make the linter happy. We are beyond recovery here.
    std::terminate();
  }
}

void DummyServer::activate() {
  if(_mainLoopThread.joinable()) {
    // main loop thread is running, already active
    assert(_serialPort);
    assert(_socatRunner.running());

    return;
  }

  // first start socat, which provides the virtual serial ports
  _socatRunner = boost::process::child(boost::process::search_path("socat"),
      boost::process::args({"-d", "-d", "pty,raw,echo=0,link=" + deviceNode, "pty,raw,echo=0,link=" + _backportNode}));

  // try to open the virtual tty with a timeout (1000 tries with 10  ms waiting time)
  static constexpr size_t maxTries = 1000;
  for(size_t i = 0; i < maxTries; ++i) {
    boost::this_thread::sleep_for(boost::chrono::milliseconds(10));
    try {
      _serialPort = std::make_unique<SerialPort>(_backportNode);
      break;
    }
    catch(ChimeraTK::runtime_error&) {
      if(i == maxTries - 1) {
        throw;
      }
    }
  }
  if(_debug) std::cout << "echoing port " << _backportNode << std::endl; // DEBUG

  // finally start the main loop, which accesses the serial port
  _stopMainLoop = false;
  _mainLoopThread = boost::thread([&]() { mainLoop(); });
}

void DummyServer::deactivate() {
  // First stop the main thread which is accessing the SerialPort object
  std::cout << "DEBUG!!! << DummyServer::deactivate() joining main thread." << std::endl;
  if(_mainLoopThread.joinable()) {
    _stopMainLoop = true;
    // Try sending the read terminate several times. The main loop in another thread might just have started reading and
    // cleared it (race condition).
    do {
      _serialPort->terminateRead();
    } while(!_mainLoopThread.try_join_for(boost::chrono::milliseconds(10)));
  }

  // Remove the SerialPort which is accessing the virtual serial ports provided by socat
  _serialPort.reset();

  // Finally, stop the socat runner
  std::cout << "DEBUG!!! << DummyServer::deactivate() stopping socat runner." << std::endl;
  assert(_socatRunner.running());
  _socatRunner.terminate();
  _socatRunner.wait();

  // Wait for "front door" file descriptor to become invalid
  std::cout << "DEBUG!!! << DummyServer::deactivate() waiting for front door to close." << std::endl;
  while(true) {
    auto fd = ::open(deviceNode.c_str(), O_RDONLY | O_NOCTTY);

    if(fd < 0) {
      break;
    }

    close(fd);
    usleep(10000);
  };
}

void DummyServer::waitForStop() {
  if(_mainLoopThread.joinable()) {
    _mainLoopThread.join();
  }
}

void DummyServer::mainLoop() {
  uint64_t nIter = 0;
  while(true) {
    if(_debug) std::cout << "dummy-server is patiently listening (" << nIter++ << ")..." << std::endl; // DEBUG
    auto readValue = _serialPort->readline();
    if(!readValue.has_value() || _stopMainLoop) {
      return;
    }
    auto data = readValue.value();

    if(_debug) std::cout << "rx'ed \"" << replaceNewLines(data) << "\"" << std::endl; // DEBUG

    if(sendNothing) {
      continue;
    }

    if(sendGarbage) {
      _serialPort->send("gnrbBlrpnBrtz");
      continue;
    }

    if(data.find("send") == 0) { // A fake command e.g. "send 4" to test multiline readback //DEBUG section
      if(data.size() < 6) {
        _serialPort->send("12345 Syntax error: send command needs an argument");
        continue;
      }
      if(data[4] != ' ') {
        _serialPort->send("12345 Syntax error: unknown command: " + data);
        continue;
      }
      int n = std::atoi(data.substr(5).c_str());
      for(int i = 0; i < n; i++) {
        std::string dat = "reply " + std::to_string(i);
        if(_debug) std::cout << "tx'ing \"" << replaceNewLines(dat) << "\"" << std::endl; // DEBUG
        _serialPort->send(dat);
      }
    }
    else if(data == "*CLS") {
      if(_debug) {
        std::cout << "Received debug clear command" << std::endl;
      }
    }
    else if(data == u8"\u0018") {
      voidCounter++;
      std::cout << "Received Emergency Stop Movement command." << std::endl;
    }
    else if(data == "*IDN?") {
      _serialPort->send("Dummy server for command based serial backend.");
    }
    else if(data == "SAI?") {
      _serialPort->send(sai[0]);
      if(!sendTooFew) {
        _serialPort->send(sai[1]);
      }
    }
    else if(data.find("ACC ") == 0) {
      auto tokens = tokenise(data);
      if(tokens.size() <= 3) {
        _serialPort->send("12345 Syntax error: ACC needs axis and value");
      }

      setAcc(tokens[1], tokens[2]);
      if(tokens.size() == 5) {
        setAcc(tokens[3], tokens[4]);
      }
      if(tokens.size() != 5 && tokens.size() != 3) {
        _serialPort->send("12345 Syntax error: ACC has wrong number of arguments");
      }
    }
    else if(data == "ACC?") {
      if(responseWithDataAndSyntaxError) {
        _serialPort->send("AXXIS_1=" + std::to_string(acc[0]));
      }
      else {
        _serialPort->send("AXIS_1=" + std::to_string(acc[0]));
      }
      if(!sendTooFew) {
        _serialPort->send("AXIS_2=" + std::to_string(acc[1]));
      }
    }
    else if(data == "ACC? AXIS1") {
      _serialPort->send("AXIS_1=" + std::to_string(acc[0]));
    }
    else if(data == "ACC? AXIS2") {
      _serialPort->send("AXIS_2=" + std::to_string(acc[1]));
    }

    else if(data.find("HEX ") == 0) {
      auto tokens = tokenise(data);
      if(tokens.size() == 4) {
        for(size_t i = 0; i < tokens.size() - 1; i++) {
          setHex(i, tokens[i + 1]);
        }
      }
      else {
        _serialPort->send("12345 Syntax error: HEX has wrong number of arguments");
      }
    }
    else if(data == "HEX?") {
      if(responseWithDataAndSyntaxError) {
        _serialPort->send("_0x" + getHexStr(hex[0]));
      }
      else {
        _serialPort->send("0x" + getHexStr(hex[0]));
      }
      if(!sendTooFew) {
        _serialPort->send("0x" + getHexStr(hex[1]));
        _serialPort->send(getHexStr(hex[2]));
      }
    }
    else if(data.find("SOUR:FREQ:CW ") == 0) {
      if(data.size() < 14) {
        _serialPort->send("12345 Syntax error: SOUR:FREQ:CW needs an argument");
        continue;
      }
      try {
        cwFrequency = std::stol(data.substr(13));
        if(_debug) {
          std::cout << "Setting cwFrequency to " << cwFrequency << std::endl;
        }
      }
      catch(...) {
        _serialPort->send("12345 Syntax error in argument: " + data.substr(13));
      }
    }
    else if(data == "SOUR:FREQ:CW?") {
      if(sendTooFew) {
        continue;
      }
      if(responseWithDataAndSyntaxError) {
        _serialPort->send("BL" + std::to_string(cwFrequency));
        continue;
      }
      _serialPort->send(std::to_string(cwFrequency));
    }
    else if(data.find("CALC1:DATA:TRAC? ") == 0) {
      // found the only right trace in this simple dummy
      if(data.find("CALC1:DATA:TRAC? 'myTrace'") == 0) {
        if(data.find("CALC1:DATA:TRAC? 'myTrace' SDAT") == 0) {
          std::string out;
          for(auto& val : trace) {
            out += std::to_string(val) + ",";
          }
          if(responseWithDataAndSyntaxError) {
            out[10] = 'M';
            if(_debug) {
              std::cout << "sending with syntax error: " << out << std::endl;
            }
          }
          out.pop_back();
          if(sendTooFew) {
            auto lastComma = out.rfind(',');
            out = out.substr(0, lastComma);
            std::cout << "sending with syntax error: " << out << std::endl;
          }
          _serialPort->send(out);
        }
        else {
          _serialPort->send("error: unknow data format");
        }
      }
      else {
        _serialPort->send("error: unknown trace");
      }
    }
    else {
      std::vector<std::string> lines = splitString(data, ";");
      for(const std::string& dat : lines) {
        if(_debug) std::cout << "tx'ing \"" << replaceNewLines(dat) << "\"" << std::endl; // DEBUG
        _serialPort->send(dat);
      }
    } // end else
  }
}

void DummyServer::setAcc(const std::string& axis, const std::string& value) {
  size_t i;
  if(axis == "AXIS_1") {
    i = 0;
  }
  else if(axis == "AXIS_2") {
    i = 1;
  }
  else {
    _serialPort->send("12345 Unknown axis: " + axis);
    return;
  }
  try {
    acc[i] = std::stof(value);
    if(_debug) {
      std::cout << "Setting acc[" << i << "] to " << acc[i] << std::endl;
    }
  }
  catch(...) {
    _serialPort->send("12345 Syntax error in argument: " + value);
  }
}

void DummyServer::setHex(size_t i, const std::string& value) {
  if(i > 2) {
    _serialPort->send("12345 Unknown element: " + std::to_string(i));
    return;
  }
  try {
    if(value.length() >= 2 and (value[0] == '0') and ((value[1] == 'x') or (value[1] == 'X'))) {
      hex[i] = ChimeraTK::userTypeToUserType<uint64_t, std::string>(value);
    }
    else {
      hex[i] = ChimeraTK::userTypeToUserType<uint64_t, std::string>("0x" + value);
    }

    if(_debug) {
      std::cout << "Setting hex[" << i << "] to " << std::hex << hex[i] << std::endl;
    }
  }
  catch(...) {
    _serialPort->send("12345 Syntax error in argument: " + value);
  }
}

std::string getHexStr(uint64_t h) {
  std::ostringstream oss;
  oss << std::hex << h;
  return oss.str();
}

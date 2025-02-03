// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#include "DummyServer.h"

#include "SerialPort.h"
#include "stringUtils.h"

#include <ChimeraTK/Exception.h>
#include <ChimeraTK/SupportedUserTypes.h>

#include <boost/process.hpp>

#include <signal.h>

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

DummyServer::DummyServer(bool useRandomDevice, bool debug) : _debug(debug) {
  if(useRandomDevice) {
    // Generate a random number to attach to the device name so muliple tests can run in parallel.
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
  if(_debug) {
    std::cout << "this is ~DummyServer" << std::endl;
  }
  try {
    if(_debug) {
      std::cout << "joining main thread" << std::endl;
    }
    deactivate();
  }
  catch(...) {
    // Try/catch make the linter happy. We are beyond recovery here.
    std::terminate();
  }
}

void DummyServer::activate() {
  if(_mainLoopThread.joinable()) {
    // Main loop thread is running, already active
    assert(_serialPort);
    assert(_socatRunner.running());

    return;
  }

  // First start socat, which provides the virtual serial ports.
  _socatRunner = boost::process::child(boost::process::search_path("socat"),
      boost::process::args({"-d", "-d", "pty,raw,echo=0,link=" + deviceNode, "pty,raw,echo=0,link=" + _backportNode}));

  // Try to open the virtual tty with a timeout (1000 tries with 10  ms waiting time).
  static constexpr size_t maxTries = 1000;
  for(size_t i = 0; i < maxTries; ++i) {
    boost::this_thread::sleep_for(boost::chrono::milliseconds(10));
    try {
      _serialPort = std::make_unique<ChimeraTK::SerialPort>(_backportNode);
      break;
    }
    catch(ChimeraTK::runtime_error&) {
      if(i == maxTries - 1) {
        throw;
      }
    }
  }
  if(_debug) {
    std::cout << "echoing port " << _backportNode << std::endl;
  }

  // Finally start the main loop, which accesses the serial port.
  _stopMainLoop = false;
  _mainLoopThread = boost::thread([&]() { mainLoop(); });
}

void DummyServer::deactivate() {
  // First stop the main threadi, which is accessing the SerialPort object.
  std::cout << "DEBUG!!! << DummyServer::deactivate() joining main thread."
            << std::endl; // FIXME TODO remove before release
  if(_mainLoopThread.joinable()) {
    // Try sending the read terminate several times. The main loop in another thread might just have started reading and
    // cleared it (race condition).
    do {
      _stopMainLoop = true;
      _serialPort->terminateRead();
    } while(!_mainLoopThread.try_join_for(boost::chrono::milliseconds(10)));
  }

  // Remove the SerialPort which is accessing the virtual serial ports provided by socat.
  _serialPort.reset();

  // Finally, stop the socat runner.
  std::cout << "DEBUG!!! << DummyServer::deactivate() stopping socat runner."
            << std::endl; // FIXME TODO remove before release

  assert(_socatRunner.running());
  // Send SIGTERM to properly terminate socat.
  // Don't use boost child's terminate(), which sends SIGKILL. This can lead to remaining device nodes which are not
  // cleaned up properly.
  kill(_socatRunner.id(), SIGTERM);
  _socatRunner.wait();

  // Wait for the "front door" file descriptor to become invalid.
  std::cout << "DEBUG!!! << DummyServer::deactivate() waiting for front door to close."
            << std::endl; // FIXME TODO remove before release

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
  std::string delim = ChimeraTK::SERIAL_DEFAULT_DELIMITER;
  uint64_t nIter = 0;
  while(true) {
    if(not byteMode) {
      if(_debug) {
         std::cout << "dummy-server is patiently listening in readline mode(" << nIter++ << ")..." << std::endl;
      }
      auto readValue = _serialPort->readline();
      if(!readValue.has_value() || _stopMainLoop) {
        return;
      }
      auto data = readValue.value();

      if(_debug) {
        std::cout << "rx'ed \"" << replaceNewLines(data) << "\"" << std::endl;
      }

      if(sendNothing) {
        continue;
      }

      if(sendGarbage) {
        sendDelimited("gnrbBlrpnBrtz");
        continue;
      }
      if(data == "*CLS") {
        if(_debug) {
          std::cout << "Received debug clear command" << std::endl;
        }
      }
    else if((data.size() == 1) && (data[0] == 0x18)) {
        voidCounter++;
        if(_debug) {
          std::cout << "Received Emergency Stop Movement command." << std::endl;
        }
      }
      else if(data == "*IDN?") {
        sendDelimited("Dummy server for command based serial backend.");
      }
      else if(data == "SAI?") {
        sendDelimited(sai[0]);
        if(!sendTooFew) {
          sendDelimited(sai[1]);
        }
      }
      else if(data.find("ACC ") == 0) {
        auto tokens = tokenise(data);
        if(tokens.size() <= 3) {
          sendDelimited("12345 Syntax error: ACC needs axis and value");
        }

        setAcc(tokens[1], tokens[2]);
        if(tokens.size() == 5) {
          setAcc(tokens[3], tokens[4]);
        }
        if(tokens.size() != 5 && tokens.size() != 3) {
          sendDelimited("12345 Syntax error: ACC has wrong number of arguments");
        }
      }
      else if(data == "ACC?") {
        if(responseWithDataAndSyntaxError) {
          sendDelimited("AXXIS_1=" + std::to_string(acc[0]));
        }
        else {
          sendDelimited("AXIS_1=" + std::to_string(acc[0]));
        }
        if(!sendTooFew) {
          sendDelimited("AXIS_2=" + std::to_string(acc[1]));
        }
      }
      else if(data == "ACC? AXIS1") {
        sendDelimited("AXIS_1=" + std::to_string(acc[0]));
      }
      else if(data == "ACC? AXIS2") {
        sendDelimited("AXIS_2=" + std::to_string(acc[1]));
      }

      else if(data.find("HEX ") == 0) {
        auto tokens = tokenise(data);
        if(tokens.size() == 4) {
          for(size_t i = 0; i < tokens.size() - 1; i++) {
            setHex(i, tokens[i + 1]);
          }
        }
        else {
          sendDelimited("12345 Syntax error: HEX has wrong number of arguments");
        }
      }
      else if(data == "HEX?") {
        if(responseWithDataAndSyntaxError) {
          sendDelimited("_0x" + getHexStr(hex[0]));
        }
        else {
          sendDelimited("0x" + getHexStr(hex[0]));
        }
        if(!sendTooFew) {
          sendDelimited("0x" + getHexStr(hex[1]));
          sendDelimited(getHexStr(hex[2]));
        }
      }
      else if(data.find("SOUR:FREQ:CW ") == 0) {
        if(data.size() < 14) {
          sendDelimited("12345 Syntax error: SOUR:FREQ:CW needs an argument");
          continue;
        }
        try {
          cwFrequency = std::stol(data.substr(13));
          if(_debug) {
            std::cout << "Setting cwFrequency to " << cwFrequency << std::endl;
          }
        }
        catch(...) {
          sendDelimited("12345 Syntax error in argument: " + data.substr(13));
        }
      }
      else if(data == "SOUR:FREQ:CW?") {
        if(sendTooFew) {
          continue;
        }
        if(responseWithDataAndSyntaxError) {
          sendDelimited("BL" + std::to_string(cwFrequency));
          continue;
        }
        sendDelimited(std::to_string(cwFrequency));
      }
      else if(data.find("CALC1:DATA:TRAC? ") == 0) {
        // Seek the only one particular trace for this simple dummy server.
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
              if(_debug) {
                std::cout << "sending with syntax error: " << out << std::endl;
              }
            }
            sendDelimited(out);
          }
          else {
            sendDelimited("error: unknow data format");
          }
        }
        else {
          sendDelimited("error: unknown trace");
        }
      }
      else if(data.find("altDelimLine") == 0) {
        auto replyStr = stripDelim(data, ChimeraTK::SERIAL_DEFAULT_DELIMITER,
            std::size(ChimeraTK::SERIAL_DEFAULT_DELIMITER) - 1); //-1 for null terminator
        _serialPort->send(replyStr);                             // Do not add a delimiter here.
      }
      else if(data.find("setByteMode") == 0) {
        // Turn the system to byte mode so that the next command will read bytesToRead rather than lines.
        // reply with 2 bytes = "ok", exercising sending lines and receiving bytes.
        byteMode = true;
        auto tokens = tokenise(data);
        if(tokens.size() >= 2) {
          bytesToRead = std::stoul(tokens[1]);
          std::cout << "Set BytesToRead to " << bytesToRead << std::endl;
        }
        else {
          bytesToRead = 16;
        }
        _serialPort->send("ok");
      }
      else {
        std::vector<std::string> lines = splitString(data, ";");
        for(const std::string& dat : lines) {
          if(_debug) {
            std::cout << "tx'ing \"" << replaceNewLines(dat) << "\"" << std::endl;
          }
          sendDelimited(dat);
        }
      }
    }      // end if line mode
    else { // byte mode
      if(_debug) {
         std::cout << "dummy-server is patiently listening in byte mode to read "<<bytesToRead<<" bytes (" << nIter++ << ")..." << std::endl;
      }
      auto readValue = _serialPort->readBytes(bytesToRead);
      if(!readValue.has_value() || _stopMainLoop) {
        return;
      }
      std::string data = readValue.value();

      if(data.find("setLineMode") == 0) { // go back to line mode
        // this exercises sending bytes and reading lines
        byteMode = false;
        std::cout << "Now on line mode" << std::endl;
        sendDelimited("OK");
      }
      else {
        // echo back the input
        _serialPort->send(data);
      }
      //
    }
  } // end while
} // end mainLoop

void DummyServer::setAcc(const std::string& axis, const std::string& value) {
  size_t i;
  if(axis == "AXIS_1") {
    i = 0;
  }
  else if(axis == "AXIS_2") {
    i = 1;
  }
  else {
    sendDelimited("12345 Unknown axis: " + axis);
    return;
  }
  try {
    acc[i] = std::stof(value);
    if(_debug) {
      std::cout << "Setting acc[" << i << "] to " << acc[i] << std::endl;
    }
  }
  catch(...) {
    sendDelimited("12345 Syntax error in argument: " + value);
  }
}

void DummyServer::setHex(size_t i, const std::string& value) {
  if(i > 2) {
    sendDelimited("12345 Unknown element: " + std::to_string(i));
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
    sendDelimited("12345 Syntax error in argument: " + value);
  }
}

std::string getHexStr(uint64_t h) {
  std::ostringstream oss;
  oss << std::hex << h;
  return oss.str();
}

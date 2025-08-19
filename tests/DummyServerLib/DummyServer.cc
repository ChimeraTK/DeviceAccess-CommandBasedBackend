// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#include "DummyServer.h"

#include "Checksum.h"
#include "SerialPort.h"
#include "stringUtils.h"

#include <ChimeraTK/Exception.h>
#include <ChimeraTK/SupportedUserTypes.h>

#include <boost/process.hpp>

#include <csignal>
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
      std::cout << "DummyServer joining main thread" << std::endl;
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
    std::cout << "DummyServer: echoing port " << _backportNode << std::endl;
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
        std::cout << "DummyServer is patiently listening in readline mode(" << nIter++ << ")..." << std::endl;
      }
      auto readValue = _serialPort->readline();
      if(!readValue.has_value() || _stopMainLoop) {
        return;
      }
      auto data = readValue.value();

      if(_debug) {
        std::cout << "DummyServer: rx'ed \"" << replaceNewLines(data) << "\"" << std::endl;
        if(data.empty()) {
          std::cout << "DummyServer: WARNING: This may indication that a delimiter was incorrectly appended to the "
                       "previous command."
                    << std::endl;
        }
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
          std::cout << "DummyServer: Received debug clear command" << std::endl;
        }
      }
      else if((data.size() == 1) && (data[0] == 0x18)) {
        voidCounter++;
        if(_debug) {
          std::cout << "DummyServer: Received Emergency Stop Movement command. voidCounter = " << voidCounter
                    << std::endl;
        }
      }
      else if(data == "*IDN?") {
        if(_debug) {
          std::cout << "Send IDN? repsonse " << std::endl;
        }
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
          if(_debug) {
            std::cout << "12339 Syntax error: HEX has wrong number of arguments" << std::endl;
          }
          sendDelimited("12339 Syntax error: HEX has wrong number of arguments");
        }
      }
      else if(data == "HEX?") {
        if(responseWithDataAndSyntaxError) {
          if(_debug) {
            std::cout << "12340 DummyServer: Replying with \"_0x" << getHexStr(hex[0]) << "\"" << std::endl;
          }
          sendDelimited("_0x" + getHexStr(hex[0]));
        }
        else {
          if(_debug) {
            std::cout << "12341 DummyServer: Replying with \"0x" << getHexStr(hex[0]) << "\"" << std::endl;
          }
          sendDelimited("0x" + getHexStr(hex[0]));
        }
        if(!sendTooFew) {
          if(_debug) {
            std::cout << "12342 DummyServer: Replying with \"0x" << getHexStr(hex[1])
                      << R"(\R\N)" + getHexStr(hex[2]) + R"(\R\N")" << std::endl;
          }
          sendDelimited("0x" + getHexStr(hex[1]));
          sendDelimited(getHexStr(hex[2]));
        }
      }
      else if(data.find("SOUR:FREQ:CW ") == 0) {
        if(data.size() < 14) {
          if(_debug) {
            std::cout << "12343 Syntax error: SOUR:FREQ:CW needs an argument" << std::endl;
          }
          sendDelimited("12343 Syntax error: SOUR:FREQ:CW needs an argument");
          continue;
        }
        try {
          cwFrequency = std::stol(data.substr(13));
          if(_debug) {
            std::cout << "12344 DummyServer: NORMAL Setting cwFrequency to " << cwFrequency << std::endl;
          }
        }
        catch(...) {
          if(_debug) {
            std::cout << "12345 Syntax error in argument: " + data.substr(13) << std::endl;
          }
          sendDelimited("12345 Syntax error in argument: " + data.substr(13));
        }
      }
      else if(data == "SOUR:FREQ:CW?") {
        if(sendTooFew) {
          continue;
        }
        if(responseWithDataAndSyntaxError) {
          if(_debug) {
            std::cout << "12346 DummyServer: sending cwFrequency as \"" << "BL" + std::to_string(cwFrequency) << "\""
                      << std::endl;
          }
          sendDelimited("BL" + std::to_string(cwFrequency));
          continue;
        }
        if(_debug) {
          std::cout << "12347 DummyServer: NORMAL Replying with cwFrequency as \"" << std::to_string(cwFrequency)
                    << "\"" << std::endl;
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
                std::cout << "DummyServer: sending with syntax error: " << out << std::endl;
              }
            }
            out.pop_back();
            if(sendTooFew) {
              auto lastComma = out.rfind(',');
              out = out.substr(0, lastComma);
              if(_debug) {
                std::cout << "DummyServer: sending with syntax error: " << out << std::endl;
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
      else if(data.find("FLT?") == 0) {
        if(_debug) {
          std::cout << "DummyServer: register FLT? Replying with \"" << flt << "\"" << std::endl;
        }
        sendDelimited(std::to_string(flt));
      }
      else if(data.find("FLT") == 0) {
        if(_debug) {
          std::cout << "DummyServer: register FLT:" << data << "." << std::endl;
        }
        if(data.size() < 4) {
          sendDelimited("12346 Syntax error: FLT needs an argument");
          continue;
        }
        try {
          flt = std::stof(data.substr(4));
          if(_debug) {
            std::cout << "DummyServer: Setting flt to " << flt << std::endl;
          }
        }
        catch(...) {
          sendDelimited("12347 Syntax error in argument: " + data.substr(4));
        }
      }
      else if(data.find('\u0007') == 0) {
        if(_debug) {
          std::cout << "DummyServer: register 0007" << std::endl;
        }
        sendDelimited(std::string("\xB0", 1));
      }
      else if(data.find("altDelimLine") == 0) {
        if(_debug) {
          std::cout << "DummyServer: altDelimLine" << std::endl;
        }
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
          std::cout << "DummyServer: Set BytesToRead to " << bytesToRead << std::endl;
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
            std::cout << "DummyServer: tx'ing \"" << replaceNewLines(dat) << "\"" << std::endl;
          }
          sendDelimited(dat);
        }
      }
    } // end if line mode
    else { // byte mode
      if(_debug) {
        std::cout << "DummyServer is patiently listening in byte mode to read " << bytesToRead << " bytes (" << nIter++
                  << ")..." << std::endl;
      }
      auto readValue = _serialPort->readBytes(bytesToRead);
      if(!readValue.has_value() || _stopMainLoop) {
        if(_stopMainLoop) {
          std::cout << "DummyServer: Stopping main loop" << std::endl;
        }
        else {
          std::cout << "DummyServer: Byte mode readValue has no value!" << std::endl;
        }
        return;
      }
      std::string data = readValue.value();

      if((data.find('\x10') == 0)) { // go back to line mode
                                     // this exercises sending bytes and reading lines
        if(_debug) {
          std::cout << "DummyServer: Registering x10 setLineMode?" << std::endl;
        }
        byteMode = false;
        if(_debug) {
          std::cout << "DummyServer: Now on line mode" << std::endl;
        }
        sendDelimited("\x06"); // send back Acknowlege, as line delimited binary
      }
      if((data.find("setLineMode") == 0)) { // go back to line mode
                                            // this exercises sending bytes and reading lines
        if(_debug) {
          std::cout << "DummyServer: Registering setLineMode?" << std::endl;
        }
        byteMode = false;
        if(_debug) {
          std::cout << "DummyServer: Now on line mode" << std::endl;
        }
        sendDelimited("OK");
      }
      else if((data.find("BFLT?") == 0)) {
        if(_debug) {
          std::cout << "DummyServer: Registering BFLT?" << std::endl;
        }
        std::string bflt = binaryStrFromFloat(flt);
        if(_debug) {
          std::cout << "DummyServer: sending " << hexStrFromBinaryStr(bflt) << std::endl;
        }
        _serialPort->send(bflt); // sends with no delimiter
        byteMode = false;        // go back to line mode at the end of this command
        if(_debug) {
          std::cout << "DummyServer: Now on line mode" << std::endl;
        }
      }
      else if((data.find("BFLT ") == 0)) {
        if(_debug) {
          std::cout << "DummyServer: Registering BFLT:" << data << "." << std::endl;
        }
        std::string floatData = data.substr(5);
        if(floatData.size() >= 4) {
          auto fltOption = floatFromBinaryStr<float>(floatData);
          if(fltOption) {
            flt = *fltOption;
            if(_debug) {
              std::cout << "DummyServer: set flt to " << flt << std::endl;
            }
          }
          else {
            std::cout << "DummyServer: Unable to convert to float from binary 0x" << hexStrFromBinaryStr(floatData)
                      << std::endl;
          }
        }
        else {
          std::cout << "DummyServer: BFLT needs a valid argument" << std::endl;
        }
        byteMode = false; // go back to line mode at the end of this command
        if(_debug) {
          std::cout << "DummyServer: Now on line mode" << std::endl;
        }
      }
      else if((data.find(std::string("\xF5\x03\xAD\xD5\x00\x00\x00\x00", 8)) == 0)) { // uLog read command
        if(_debug) {
          std::cout << "DummyServer: Registering ulog read cmd" << std::endl; // DEBUG
        }
        char requiredChecksum = '\x7A';
        if(data[8] == requiredChecksum) { // If correct checksum in the read command
          std::string ulogStr("\0\0\0\0", 4);
          auto maybeStr = binaryStrFromInt(ulog, 4);
          if(maybeStr) {
            ulogStr = *maybeStr;
          }
          else {
            std::cout << "DummyServer: Unable to convert to int to binary string" << ulog << std::endl;
          }
          std::string retStr = std::string("\xF5\x04\xAD\xD5", 4) + ulogStr; // F504 prefaces a read response
          retStr += binaryStrFromHexStr(ChimeraTK::getChecksumAlgorithm(checksum::CS8)(retStr)); // Append the checksum
          if(_debug) {
            std::cout << "DummyServer: Writing back " << hexStrFromBinaryStr(retStr) << std::endl; // DEBUG
          }
          _serialPort->send(retStr);
        }
        else {
          std::string requiredChecksumStr = std::string(1, requiredChecksum);
          std::cout << "DummyServer: Bad checksum (" << hexStrFromBinaryStr(data.substr(8))
                    << ") on ulog read command: " << hexStrFromBinaryStr(data) << " expected "
                    << hexStrFromBinaryStr(requiredChecksumStr) << std::endl;
          _serialPort->send(
              std::string("\xBA\xDC\x50", 3) + data[8] + std::string("\x0A\x50\x20\xB0", 4) + requiredChecksumStr);
          // That is to say "BADCheckSum_", what it is, "_hAS_TO_Be_", what it is required to be.
        }
        byteMode = false; // go back to line mode at the end of this command
        if(_debug) {
          std::cout << "DummyServer: Now on line mode" << std::endl; // DEBUG
        }
      }
      else if((data.find(std::string("\xF5\x01\xAD\xD5", 4)) == 0)) { // uLog write command
        if(_debug) {
          std::cout << "DummyServer: Registering ulog write cmd" << std::endl;
        }
        auto intOpt = intFromBinaryStr<uint32_t>(data.substr(4, 4));
        if(intOpt) {
          ulog = *intOpt;
          if(_debug) {
            std::cout << "DummyServer: Retreived ulog as " << ulog << std::endl;
          }
        }
        else {
          std::cout << "DummyServer: Unable to convert to int from binary 0x" << hexStrFromBinaryStr(data.substr(4, 4))
                    << std::endl;
        }
        std::string requiredChecksumStr =
            binaryStrFromHexStr(ChimeraTK::getChecksumAlgorithm(checksum::CS8)(data.substr(0, 8)));
        if(data[8] == requiredChecksumStr[0]) { // If the checksum is correct
          std::string retStr = std::string("\xF5\x02\xAD\xD5", 4) + data.substr(4, 4);
          retStr += binaryStrFromHexStr(ChimeraTK::getChecksumAlgorithm(checksum::CS8)(retStr)); // Append the checksum
          if(_debug) {
            std::cout << "DummyServer: Writing back " << hexStrFromBinaryStr(retStr) << std::endl; // DEBUG
          }
          _serialPort->send(retStr);
        }
        else {
          std::cout << "DummyServer: Bad checksum (" << hexStrFromBinaryStr(data.substr(8))
                    << ") on ulog write command: " << hexStrFromBinaryStr(data) << " expected "
                    << hexStrFromBinaryStr(requiredChecksumStr) << std::endl;
          _serialPort->send(
              std::string("\xBA\xD0\xC5", 3) + data[8] + std::string("\x0A\x50\x20\xB0", 4) + requiredChecksumStr);
          // That is to say "BAD_CheckSum", what it is, "_hAS_TO_Be_", what it is required to be.
        }
      }

      else {
        // echo back the input
        _serialPort->send(data);
        if(_debug) {
          std::cout << "DummyServer: Catch-all for " << data << std::endl; // DEBUG
        }
      }
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
      std::cout << "DummyServer: Setting acc[" << i << "] to " << acc[i] << std::endl;
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
      std::cout << "DummyServer: Setting hex[" << i << "] to " << std::hex << hex[i] << std::endl;
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

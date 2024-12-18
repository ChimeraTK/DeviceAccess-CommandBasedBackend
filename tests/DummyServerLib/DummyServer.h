// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "SerialPort.h"

#include <boost/process.hpp>
#include <boost/thread.hpp>

#include <atomic>
#include <memory>

/**
 * A stand-in server class for the CommandBasedBackend to communicate with
 * over for testing.
 * The CommandBasedBackend is to send serial commands to the DummyServer,
 * relayed by socat. The DummyServer then emulates as a target hardware device.
 * Socat is started automatically in the constructor.
 */
class DummyServer {
 public:
  /**
   * useRandomDevice randomizes the serial communication port (the _backportNode)
   * debug toggles printouts
   */
  DummyServer(bool useRandomDevice = true, bool debug = false);
  ~DummyServer();

  /**
   * Helper class to have a thread safe string storage.
   */
  class LockingString {
   public:
    LockingString(const char* val) : _value(val) {}

    operator std::string() const {
      auto l = std::lock_guard(_mutex);
      return _value;
    }
    LockingString& operator=(std::string const& val) {
      auto l = std::lock_guard(_mutex);
      _value = val;
      return *this;
    }

   private:
    std::string _value;
    mutable std::mutex _mutex;
  };

  // These hold data values interogated by the CommandBasedBackend
  // Their names reflect register names
  std::atomic<float> acc[2]{0.2, 0.3};
  std::atomic<float> mov[2]{1.2, 1.3};
  std::atomic<uint64_t> cwFrequency{1300000000};
  std::atomic<float> trace[10]{0., 1., 4., 9., 16., 25., 36., 49., 64., 81.};
  LockingString sai[2]{"AXIS_1", "AXIS_2"};
  std::atomic<uint64_t> hex[3]{0xbabef00d, 0xFEEDC0DE, 0xBADdCAFE};
  std::atomic<uint64_t> voidCounter{0};

  std::atomic_bool sendNothing{false};
  std::atomic_bool sendTooFew{false};
  std::atomic_bool responseWithDataAndSyntaxError{false};
  std::atomic_bool sendGarbage{false};

  // Pause execution here to wait for the main thread to stop (e.g. because ^C has been pressed).
  void waitForStop();

  // Starts the socat runner and the main thread
  void activate();
  // Stops the main thread and then terminates the socat runner
  void deactivate();

  // The device node for the business logic. It normaly contains a random component, so
  // we need a way to read it back.
  std::string deviceNode{"/tmp/virtual-tty"};

 protected:
  void mainLoop();
  std::unique_ptr<SerialPort> _serialPort;

  void setAcc(const std::string& axis, const std::string& value);
  void setHex(size_t i, const std::string& value);

  bool _debug;

  boost::process::child _socatRunner;
  boost::thread _mainLoopThread;
  std::atomic_bool _stopMainLoop{false};

  std::string _backportNode;
};

/**
 * Returns the hexidecimal representation of the uint64_t with no "0x" prefix.
 */
std::string getHexStr(uint64_t h);

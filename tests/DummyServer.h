// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "SerialPort.h"

#include <boost/process.hpp>
#include <boost/thread.hpp>

#include <atomic>
#include <memory>

class DummyServer {
 public:
  DummyServer();
  ~DummyServer();

  float acc[2]{0.2, 0.3};
  float mov[2]{1.2, 1.3};
  uint64_t cwFrequency{1300000000};
  float trace[10]{0., 1., 4., 9., 16., 25., 36., 49., 64., 81.};

  void waitForStop();

  void stop();

 protected:
  void mainLoop();
  std::unique_ptr<SerialPort> _serialPort;

  void setAcc(size_t i, const std::string& data);

  bool _debug{true};

  boost::process::child _socatRunner;
  boost::thread _mainLoopThread;
  std::atomic_bool _stopMainLoop{false};
};

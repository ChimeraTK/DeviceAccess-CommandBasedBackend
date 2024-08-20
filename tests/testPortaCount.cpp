// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "SerialPort.h"

#include <iostream>

int main() {
  SerialPort s("/dev/ttyUSB0");

  while(true) {
    auto line = s.readline();
    if(!line.has_value()) {
      std::cout << "read terminated. Good bye!" << std::endl;
      return 0;
    }
    std::cout << "Received: " << line.value() << std::endl;
  }
}

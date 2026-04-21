// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#include "DummyServer.h"

#include <iostream>

int main() {
  DummyServer dummy(false, // Don't use random number when stating the dummy as a stand-alone executable
      true,                // do use debug printouts
      115200);             // Use default baud rate for serial port
  dummy.waitForStop();

  return 0;
} // end main

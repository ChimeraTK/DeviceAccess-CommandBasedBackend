// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#include "DummyServer.h"

#include <iostream>

int main() {
  DummyServer dummy(false,
      true); // Don't use random number when stating the dummy as a stand-alone executable; do use debug printouts
  dummy.waitForStop();

  return 0;
} // end main

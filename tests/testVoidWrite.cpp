// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#define BOOST_TEST_DYN_LINK
// Define a name for the test module.
#define BOOST_TEST_MODULE CommandBasedBackendVoidWriteTest
// Only after defining the name include the unit test header.
#include <boost/test/unit_test.hpp>
using namespace boost::unit_test_framework;

#include "DummyServer.h"

#include <ChimeraTK/Device.h>
#include <ChimeraTK/UnifiedBackendTest.h>

static DummyServer dummyServer;

BOOST_AUTO_TEST_CASE(voidWrite) {
  auto device = ChimeraTK::Device("(CommandBasedTTY:" + dummyServer.deviceNode + "?map=test.json)");

  device.open();

  auto accessor = device.getVoidRegisterAccessor("/emergencyStopMovement");

  // check pre-conditon
  BOOST_TEST(dummyServer.voidCounter == 0);
  accessor.write();
  // wait up to 3000 ms to check that the dummy server thread has seen the command
  CHECK_TIMEOUT(dummyServer.voidCounter == 1, 3000);
}

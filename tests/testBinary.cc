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

#include <iostream>

struct TestFixture {
  DummyServer dummyServer{true, true};
  ChimeraTK::Device device;

  TestFixture() : device("(CommandBasedTTY:" + dummyServer.deviceNode + "?map=test.json)") { device.open(); }
};

// Declares and registers a fixture used by all test cases under a test suite.
// Each test case in the subtree of the test suite uses the fixture.
BOOST_FIXTURE_TEST_SUITE(MapFileV2Tests, TestFixture)

BOOST_AUTO_TEST_CASE(testFloat) {
  auto accessor = device.getScalarRegisterAccessor<float>("/floatTest");

  float fIn = 2.5;
  accessor = fIn;
  accessor.write();
  accessor.read();
  float fOut = accessor;
  BOOST_CHECK_EQUAL(fIn, fOut);
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testBinFloat) {
  auto accessor = device.getScalarRegisterAccessor<float>("/binFloatTest");
  auto byteModeAccessor = device.getScalarRegisterAccessor<uint32_t>("/setByteMode");

  // Put the Dummy in binary mode, and have it read 9 bytes
  byteModeAccessor = 9;
  byteModeAccessor.write();

  float fIn = 2.5;
  accessor = fIn;
  accessor.write(); // runs //No accompanying signal from Dummy server showing it ever got the signal. This reg. has no
                    // write response, so we keep going

  // Now the Dummy server puts its self back into line mode, but we're going to read bytes back:
  // Configure it to read the 5 byte read command
  byteModeAccessor = 5;
  byteModeAccessor.write();

  accessor.read();
  // Now the Dummy server puts its self back into line mode
  float fOut = accessor;
  BOOST_CHECK_EQUAL(fIn, fOut);
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testReadStatus) {
  auto accessor = device.getScalarRegisterAccessor<uint32_t>("/controllerReadyStatus");

  accessor.read();
  uint32_t out = accessor;
  BOOST_CHECK_EQUAL(out, 0xB0);
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testULog) {
  auto byteModeAccessor = device.getScalarRegisterAccessor<uint32_t>("/setByteMode");
  auto uLogAccessor = device.getScalarRegisterAccessor<uint32_t>("/uLog");
  uint32_t w = 8;
  byteModeAccessor = w;
  byteModeAccessor.write();

  uint32_t in = 999;
  uLogAccessor = in;
  uLogAccessor.write();
  // The Dummy server stays in binary mode after this write, and will again look to read 8 bytes.
  uLogAccessor.read();
  uint32_t out = uLogAccessor;
  BOOST_CHECK_EQUAL(in, out);
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_SUITE_END()

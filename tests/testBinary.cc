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

/**********************************************************************************************************************/

constexpr bool DEBUG = false;

/**********************************************************************************************************************/

struct TestFixture {
  DummyServer dummyServer{true, DEBUG};
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

  if(DEBUG) {
    std::cout << "testBinary: testBinFloat. write 9" << std::endl;
  }
  // Put the Dummy in binary mode, and have it read 9 bytes
  byteModeAccessor = 9;
  byteModeAccessor.write();

  if(DEBUG) {
    std::cout << "testBinary: testBinFloat. write 2.5" << std::endl;
  }
  float fIn = 2.5;
  accessor = fIn;
  accessor.write(); // runs //No accompanying signal from Dummy server showing it ever got the signal. This reg. has no
                    // write response, so we keep going

  // Now the Dummy server puts its self back into line mode, but we're going to read bytes back:
  // Configure it to read the 5 byte read command
  if(DEBUG) {
    std::cout << "testBinary: testBinFloat. write 5" << std::endl;
  }
  byteModeAccessor = 5;
  byteModeAccessor.write();

  if(DEBUG) {
    std::cout << "testBinary: testBinFloat. read" << std::endl;
  }
  accessor.read();
  // Now the Dummy server puts its self back into line mode

  if(DEBUG) {
    std::cout << "testBinary: testBinFloat. done" << std::endl;
  }
  float fOut = accessor;
  BOOST_CHECK_EQUAL(fIn, fOut);
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testReadStatus) {
  auto accessor = device.getScalarRegisterAccessor<uint32_t>("/controllerReadyStatus");
  if(DEBUG) {
    std::cout << "testBinary: testReadStatus. Send read" << std::endl;
  }

  accessor.read();
  if(DEBUG) {
    std::cout << "testBinary: read returned" << std::endl;
  }
  uint32_t out = accessor;
  BOOST_CHECK_EQUAL(out, 0);
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testULog) {
  if(DEBUG) {
    std::cout << "testBinary: testULog write" << std::endl;
  }
  auto byteModeAccessor = device.getScalarRegisterAccessor<uint32_t>("/setByteMode");
  auto uLogAccessor = device.getScalarRegisterAccessor<uint32_t>("/uLog");
  uint32_t w = 8;
  byteModeAccessor = w;
  if(DEBUG) {
    std::cout << "testBinary: testULog write 8" << std::endl;
  }
  byteModeAccessor.write();
  if(DEBUG) {
    std::cout << "testBinary: testULog write 999" << std::endl;
  }

  uint32_t in = 999;
  uLogAccessor = in;
  uLogAccessor.write();
  // The Dummy server stays in binary mode after this write, and will again look to read 8 bytes.
  if(DEBUG) {
    std::cout << "testBinary: testULog read" << std::endl;
  }
  uLogAccessor.read();
  if(DEBUG) {
    std::cout << "testBinary: testULog done" << std::endl;
  }
  uint32_t out = uLogAccessor;
  BOOST_CHECK_EQUAL(in, out);
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_SUITE_END()

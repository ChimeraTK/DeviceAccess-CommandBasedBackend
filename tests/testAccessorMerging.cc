// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#define BOOST_TEST_DYN_LINK
// Define a name for the test module.
#define BOOST_TEST_MODULE CommandBasedBackendAccessorMergingTest
// Only after defining the name include the unit test header.
#include <boost/test/unit_test.hpp>
using namespace boost::unit_test_framework;

#include "DummyServer.h"

#include <ChimeraTK/Device.h>
#include <ChimeraTK/TransferGroup.h>

static DummyServer dummyServer;

/**
 * Dediacated test that partial accessors are merging correctly.
 */
BOOST_AUTO_TEST_CASE(AccessorMerging) {
  auto device = ChimeraTK::Device("(CommandBasedTTY:" + dummyServer.deviceNode + "?map=test.json)");

  device.open();

  auto fullAccessor = device.getOneDRegisterAccessor<double>("/ACC");
  auto acc1 = device.getScalarRegisterAccessor<double>("/ACC", 1);
  auto acc23 = device.getOneDRegisterAccessor<double>("/ACC", 2, 2);
  // Pre-Tests
  acc1.read(); // initial value, must not be what we are about to write
  BOOST_TEST(acc1 != 20.);
  acc23.read(); // initial value, must not be what we are about to write
  BOOST_TEST(acc23[0] != 30.);
  BOOST_TEST(acc23[1] != 40.);

  fullAccessor = {10., 20., 30., 40., 50.};
  fullAccessor.write();
  // do a read to synchronise the command counter. As the write is "fire and forget", and is processed asynchronously,
  // the counter is probably not increased by the dummy yet when the code continues here.
  // The read guarantees that the dummy has seen and counted the command, as the read is waiting for the corresponding
  // response.
  fullAccessor.read();

  ChimeraTK::TransferGroup tg;
  tg.addAccessor(acc1);
  tg.addAccessor(acc23);

  uint64_t oldCommandCount = dummyServer.commandCounter;

  tg.read();

  // The actual test: exactly one command for the read has been performed
  BOOST_TEST(dummyServer.commandCounter == oldCommandCount + 1);

  // Additional tests: The data has arrived correctly (Feature creep, should be in DeviceAccess for the SubArray decorator).
  BOOST_TEST(acc1 == 20.);
  BOOST_TEST(acc23[0] == 30.);
  BOOST_TEST(acc23[1] == 40.);
}

/**
 * Dediacated test that partial accessors are merging correctly.
 */
BOOST_AUTO_TEST_CASE(AccessorMergingReadModifyWrite) {
  auto device = ChimeraTK::Device("(CommandBasedTTY:" + dummyServer.deviceNode + "?map=test.json)");

  device.open();

  auto fullAccessor = device.getOneDRegisterAccessor<double>("/ACC");
  auto acc1 = device.getScalarRegisterAccessor<double>("/ACC", 1);
  auto acc23 = device.getOneDRegisterAccessor<double>("/ACC", 2, 2);

  fullAccessor = {10., 20., 30., 40., 50.};
  fullAccessor.write();
  // do a read to synchronise the command counter. As the write is "fire and forget", and is processed asynchronously,
  // the counter is probably not increased by the dummy yet when the code continues here.
  // The read guarantees that the dummy has seen and counted the command, as the read is waiting for the corresponding
  // response.
  fullAccessor.read();

  ChimeraTK::TransferGroup tg;
  tg.addAccessor(acc1);
  tg.addAccessor(acc23);

  uint64_t oldCommandCount = dummyServer.commandCounter;
  std::cout << "oldCommandCount " << oldCommandCount << std::endl;

  acc1 = 22;
  acc23[0] = 33;
  acc23[1] = 44;

  tg.write();

  // we have to perform a read again to synchronise the command counter. It will be one more than the write operation.
  fullAccessor.read();

  // exactly one has been performed
  BOOST_TEST(dummyServer.commandCounter ==
      oldCommandCount + 3); // two for the read/modify/write, and one for the full accessor read.

  // Additional test: The read/modify/write worked as intended.
  BOOST_TEST(fullAccessor[0] == 10.);
  BOOST_TEST(fullAccessor[1] == 22.);
  BOOST_TEST(fullAccessor[2] == 33.);
  BOOST_TEST(fullAccessor[3] == 44.);
  BOOST_TEST(fullAccessor[4] == 50.);
}

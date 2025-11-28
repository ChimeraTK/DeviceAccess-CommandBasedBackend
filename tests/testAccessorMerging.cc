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

  fullAccessor = {10., 20., 30., 40., 50.};
  fullAccessor.write();

  ChimeraTK::TransferGroup tg;
  tg.addAccessor(acc1);
  tg.addAccessor(acc23);

  uint64_t oldCommandCount = dummyServer.commandCounter;
  std::cout << "oldCommandCount " << oldCommandCount << std::endl;

  tg.read();

  // exactly one has been performed
  BOOST_TEST(dummyServer.commandCounter == oldCommandCount + 1);

  // Additional test: The data has arrived correctly (Feature creep, should be in DeviceAccess for the SubArray decorator).
}

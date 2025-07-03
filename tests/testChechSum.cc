// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#define BOOST_TEST_DYN_LINK
// Define a name for the test module.
#define BOOST_TEST_MODULE CommandBasedBackendVoidWriteTest
// Only after defining the name include the unit test header.
#include <boost/test/unit_test.hpp>
using namespace boost::unit_test_framework;

#include "CheckSum.h"
#include "DummyServer.h"
#include "stringUtils.h"

#include <ChimeraTK/Device.h>
#include <ChimeraTK/UnifiedBackendTest.h>

BOOST_AUTO_TEST_CASE(testCheckSum8) {
  std::string hexInput = "3132A33B343C5363D738EF39";
  std::string binInput = binaryStrFromHexStr(hexInput);
  checksumFunction checksumer = getChecksumer("CS8");
  std::string CsOutHex = checksumer(binInput);
  std::string CsAnsHex = "9E";
  // see https://www.scadacore.com/tools/programming-calculators/online-checksumer-calculator/ under CS8 mod 256
  BOOST_CHECK_EQUAL(CsOutHex, CsAnsHex);
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testCheckSum32) {
  std::string hexInput = "3132A33B343C5363D738EF39";
  std::string binInput = binaryStrFromHexStr(hexInput);
  checksumFunction checksumer = getChecksumer("CS32");
  std::string CsOutHex = checksumer(binInput);
  std::string CsAnsHex = "0000049E";
  BOOST_CHECK_EQUAL(CsOutHex, CsAnsHex);
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testCheckSumCrcCcit16) {
  std::string hexInput = "313233343536373839";
  std::string binInput = binaryStrFromHexStr(hexInput);
  checksumFunction checksumer = getChecksumer("CrcCcit16");
  std::string CsOutHex = checksumer(binInput);
  std::string CsAnsHex = "29B1";
  // see https://www.boost.org/doc/libs/latest/libs/crc/test/crc_test.cpp under "Global data".
  BOOST_CHECK_EQUAL(CsOutHex, CsAnsHex);
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testSha256) {
  std::string binInput = "Old McDonnald had a farm, E-I-E-I-O. And on that farm he had a hash function, E-I-E-I-O";

  checksumFunction checksumer = getChecksumer("Sha256");
  std::string CsOutHex = checksumer(binInput);
  std::string CsAnsHex = "D4D2AA4F1328BA94477B1FC217E1D25C15268263C3CB11F2327674A979F4F6F4";
  // see https://md5calc.com/hash/sha256/Old+McDonnald+had+a+farm%2C+E-I-E-I-O.+And+on+that+farm+he+had+a+hash+function%2C+E-I-E-I-O
  BOOST_CHECK_EQUAL(CsOutHex, CsAnsHex);
}

/**********************************************************************************************************************/

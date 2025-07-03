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

// static DummyServer dummyServer;

BOOST_AUTO_TEST_CASE(testCheckSum8) {
  std::string hexInput = "3132A33B343C5363D738EF39";
  constexpr bool padLeft = true;
  std::string binInput = binaryStrFromHexStr(hexInput, padLeft);
  checksumFunction checksumer = getChecksumer("CS8");
  std::string CsOutHex = checksumer(binInput);
  std::string CsAnsHex = "9E";
  // see https://www.scadacore.com/tools/programming-calculators/online-checksumer-calculator/ under CS8 mod 256
  BOOST_CHECK_EQUAL(CsOutHex, CsAnsHex);
}

BOOST_AUTO_TEST_CASE(testCheckSum32) {
  std::string hexInput = "3132A33B343C5363D738EF39";
  constexpr bool padLeft = true;
  std::string binInput = binaryStrFromHexStr(hexInput, padLeft);
  checksumFunction checksumer = getChecksumer("CS32");
  std::string CsOutHex = checksumer(binInput);
  std::string CsAnsHex = "0000049E";

  BOOST_CHECK_EQUAL(CsOutHex, CsAnsHex);
}

BOOST_AUTO_TEST_CASE(testCheckSumCrcCcit16) {
  std::string hexInput = "313233343536373839";
  constexpr bool padLeft = true;
  std::string binInput = binaryStrFromHexStr(hexInput, padLeft);
  checksumFunction checksumer = getChecksumer("CrcCcit16");
  std::string CsOutHex = checksumer(binInput);
  std::string CsAnsHex =
      "29B1"; // see https://www.boost.org/doc/libs/latest/libs/crc/test/crc_test.cpp under "Global data".
  BOOST_CHECK_EQUAL(CsOutHex, CsAnsHex);
}

/*
BOOST_AUTO_TEST_CASE(testSha256) {
    // https://codebeautify.org/checksumer-calculator
    std::string hexInput = "3132A33B343C5363D738EF39"; //ascii "12?;4<Sc?8?9"
    constexpr bool padLeft = true;
    std::string binInput = binaryStrFromHexStr(hexInput, padLeft);
    checksumFunction checksumer = getChecksumer("Sha256");
    std::string CsOutHex = checksumer(binInput); // comes out 9E
    std::string CsAnsHex = "CAB66D3FC5C02E110B0A75CB5B36DF8F378A97C8CFEA6E44CEBE501AF734409C";

    BOOST_CHECK_EQUAL(CsOutHex, CsAnsHex);
}*/

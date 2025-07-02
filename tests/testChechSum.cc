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
//
/* std::string binData = binaryStrFromHexStr(hexData, padLeft); //If you are starting from a string of hexidecimal
 * std::string checksumTag = "myChecksum"; // configuration, case insensitive
 * auto checksum = makeChecksumer(checksumTag);
 * std::string hexidecimalChecksumResult = (*checksum)(binData);
 */

BOOST_AUTO_TEST_CASE(testCheckSum8) {
  // https://www.scadacore.com/tools/programming-calculators/online-checksum-calculator/
  std::string hexInput = "3132A33B343C5363D738EF39";
  bool padLeft = true;
  std::string binInput = binaryStrFromHexStr(hexInput, padLeft);
  checksumFunction checksum = getChecksumer("CS8");
    // Our function is summing the bytes and taking the mod. 
  std::string CsOutHex = checksum(binInput); 
  std::string CsAnsHex = "9E";
  // CS8 xor: 9A
  // CS8 mod 256: 9E
  // CS2 2's compliment: 62
  BOOST_CHECK_EQUAL(CsOutHex, CsAnsHex);
}

BOOST_AUTO_TEST_CASE(testCheckSum32) {
    std::string hexInput = "3132A33B343C5363D738EF39";
    bool padLeft = true;
    std::string binInput = binaryStrFromHexStr(hexInput, padLeft);
    checksumFunction checksum = getChecksumer("CS32");
    std::string CsOutHex = checksum(binInput);
    std::string CsAnsHex = "0000049E";

    BOOST_CHECK_EQUAL(CsOutHex, CsAnsHex);
}

BOOST_AUTO_TEST_CASE(testCheckSumCrcCcit16) {
  // https://codebeautify.org/checksum-calculator
  std::string hexInput = "3132A33B343C5363D738EF39";
  bool padLeft = true;
  std::string binInput = binaryStrFromHexStr(hexInput, padLeft);
  checksumFunction checksum = getChecksumer("CrcCcit16");
  std::string CsOutHex = checksum(binInput); // comes out 2436
  std::string CsAnsHex = "14F8";

  BOOST_CHECK_EQUAL(CsOutHex, CsAnsHex);
}

/*
BOOST_AUTO_TEST_CASE(testSha256) {
    // https://codebeautify.org/checksum-calculator
    std::string hexInput = "3132A33B343C5363D738EF39"; //ascii "12?;4<Sc?8?9"
    bool padLeft = true;
    std::string binInput = binaryStrFromHexStr(hexInput, padLeft);
    checksumFunction checksum = getChecksumer("Sha256");
    std::string CsOutHex = checksum(binInput); // comes out 9E
    std::string CsAnsHex = "CAB66D3FC5C02E110B0A75CB5B36DF8F378A97C8CFEA6E44CEBE501AF734409C";

    BOOST_CHECK_EQUAL(CsOutHex, CsAnsHex);
}*/

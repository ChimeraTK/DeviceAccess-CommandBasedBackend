// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#define BOOST_TEST_DYN_LINK
// Define a name for the test module.
#define BOOST_TEST_MODULE CommandBasedBackendVoidWriteTest
// Only after defining the name include the unit test header.
#include <boost/test/unit_test.hpp>
using namespace boost::unit_test_framework;

#include "Checksum.h"
#include "DummyServer.h"
#include "stringUtils.h"

#include <ChimeraTK/Device.h>
#include <ChimeraTK/UnifiedBackendTest.h>

namespace ChimeraTK {

  /********************************************************************************************************************/

  BOOST_AUTO_TEST_CASE(testCheckSum8) {
    std::string hexInput = "3132A33B343C5363D738EF39";
    std::string binInput = binaryStrFromHexStr(hexInput);
    checksumAlgorithm checksumAlgorithmFunc = getChecksumAlgorithm(checksum::CS8);
    std::string CsOutHex = checksumAlgorithmFunc(binInput);
    std::string CsAnsHex = "9E";
    // see https://www.scadacore.com/tools/programming-calculators/online-checksumer-calculator/ under CS8 mod 256
    BOOST_CHECK_EQUAL(CsOutHex, CsAnsHex);
  }

  /********************************************************************************************************************/

  BOOST_AUTO_TEST_CASE(testCheckSum32) {
    std::string hexInput = "3132A33B343C5363D738EF39";
    std::string binInput = binaryStrFromHexStr(hexInput);
    checksumAlgorithm checksumAlgorithmFunc = getChecksumAlgorithm(checksum::CS32);
    std::string CsOutHex = checksumAlgorithmFunc(binInput);
    std::string CsAnsHex = "0000049E";
    BOOST_CHECK_EQUAL(CsOutHex, CsAnsHex);
  }

  /********************************************************************************************************************/

  BOOST_AUTO_TEST_CASE(testCheckSumCrcCcit16) {
    std::string hexInput = "313233343536373839";
    std::string binInput = binaryStrFromHexStr(hexInput);
    checksumAlgorithm checksumAlgorithmFunc = getChecksumAlgorithm(checksum::CRC_CCIT16);
    std::string CsOutHex = checksumAlgorithmFunc(binInput);
    std::string CsAnsHex = "29B1";
    // see https://www.boost.org/doc/libs/latest/libs/crc/test/crc_test.cpp under "Global data".
    BOOST_CHECK_EQUAL(CsOutHex, CsAnsHex);
  }

  /********************************************************************************************************************/

  BOOST_AUTO_TEST_CASE(testSha256) {
    std::string binInput = "Old McDonnald had a farm, E-I-E-I-O. And on that farm he had a hash function, E-I-E-I-O";

    checksumAlgorithm checksumAlgorithmFunc = getChecksumAlgorithm(checksum::SHA256);
    std::string CsOutHex = checksumAlgorithmFunc(binInput);
    std::string CsAnsHex = "D4D2AA4F1328BA94477B1FC217E1D25C15268263C3CB11F2327674A979F4F6F4";
    // see https://md5calc.com/hash/sha256/Old+McDonnald+had+a+farm%2C+E-I-E-I-O.+And+on+that+farm+he+had+a+hash+function%2C+E-I-E-I-O
    BOOST_CHECK_EQUAL(CsOutHex, CsAnsHex);
  }

  /********************************************************************************************************************/

  BOOST_AUTO_TEST_CASE(testChecksumValidation) {
    BOOST_CHECK_NO_THROW(validateChecksumPattern("", ""));
    BOOST_CHECK_NO_THROW(validateChecksumPattern("Pattern with no checksum tags", "error message detail"));
    BOOST_CHECK_NO_THROW(
        validateChecksumPattern("{{ cs.1}}{{csStart.0  }}asdf{{csEnd.0}}{{cs.0}}{{csStart.1}}qwer{{csEnd.1}}", ""));

    BOOST_CHECK_THROW(
        validateChecksumPattern("{{cs.2}}{{csStart.0}}asdf{{csEnd.0}}{{cs.0}}{{csStart.2}}qwer{{csEnd.2}}", ""),
        ChimeraTK::logic_error); // index gap
    BOOST_CHECK_THROW(
        validateChecksumPattern("{{cs.3}}{{csStart.0}}asdf{{csEnd.0}}{{cs.0}}{{csStart.1}}qwer{{csEnd.1}}", ""),
        ChimeraTK::logic_error); // incomplete
    BOOST_CHECK_THROW(
        validateChecksumPattern("{{cs.1}{{csStart.0}}asdf{{csEnd.0}}{{cs.0}}{{csStart.1}}qwer{{csEnd.1}}", ""),
        ChimeraTK::logic_error); // missing parentheses->incomplete
    BOOST_CHECK_THROW(
        validateChecksumPattern("{cs.1}}{{csStart.0}}asdf{{csEnd.0}}{{cs.0}}{{csStart.1}}qwer{{csEnd.1}}", ""),
        ChimeraTK::logic_error); // missing parentheses->incomplete
    BOOST_CHECK_THROW(
        validateChecksumPattern("{{cs.1}{}{csStart.0}}asdf{{csEnd.0}}{{cs.0}}{{csStart.1}}qwer{{csEnd.1}}", ""),
        ChimeraTK::logic_error); // garbled parentheses->incomplete
    BOOST_CHECK_THROW(
        validateChecksumPattern("{{cs.1}}{{csstart.0}}asdf{{csEnd.0}}{{cs.0}}{{csStart.1}}qwer{{csEnd.1}}", ""),
        ChimeraTK::logic_error); // incorrect capitalization->incomplete
    BOOST_CHECK_THROW(validateChecksumPattern("{{cs.1}}{{csStart.0csEnd.0}}{{cs.0}}{{csStart.1}}qwer{{csEnd.1}}", ""),
        ChimeraTK::logic_error); // garbled tags->incomplete
    BOOST_CHECK_THROW(validateChecksumPattern("{{csStart.0}}asdf{{csEnd.0}}{{cs.0}}{{csStart.1}}qwer{{csEnd.1}}", ""),
        ChimeraTK::logic_error); // missing insertion tag
    BOOST_CHECK_THROW(validateChecksumPattern("{{cs.1}}asdf{{csEnd.0}}{{cs.0}}{{csStart.1}}qwer{{csEnd.1}}", ""),
        ChimeraTK::logic_error); // missing start tag
    BOOST_CHECK_THROW(validateChecksumPattern("{{cs.1}}{{csStart.0}}asdf{{csEnd.0}}{{cs.0}}{{csStart.1}}qwer", ""),
        ChimeraTK::logic_error); // missing end tag
    BOOST_CHECK_THROW(
        validateChecksumPattern("{{cs.1}}{{csEnd.0}}asdf{{csStart.0}}{{cs.0}}{{csStart.1}}qwer{{csEnd.1}}", ""),
        ChimeraTK::logic_error); // end<start
    BOOST_CHECK_THROW(
        validateChecksumPattern("{{cs.1}}{{csStart.0}}asdf{{cs.0}}{{csEnd.0}}{{csStart.1}}qwer{{csEnd.1}}", ""),
        ChimeraTK::logic_error); // contains self
    BOOST_CHECK_THROW(
        validateChecksumPattern("{{csStart.0}}{{cs.1}}asdf{{csEnd.0}}{{cs.0}}{{csStart.1}}qwer{{csEnd.1}}", ""),
        ChimeraTK::logic_error); // overlap, insertion tag contained
    BOOST_CHECK_THROW(
        validateChecksumPattern("{{cs.1}}{{csStart.0}}asdf{{csStart.1}}{{csEnd.0}}qwer{{csEnd.1}}{{cs.0}}", ""),
        ChimeraTK::logic_error); // overlap, start tag contained
    BOOST_CHECK_THROW(
        validateChecksumPattern("{{cs.1}}{{csStart.1}}asdf{{csStart.0}}{{csEnd.1}}qwer{{csEnd.0}}{{cs.0}}", ""),
        ChimeraTK::logic_error); // overlap, end tag contained
    BOOST_CHECK_THROW(
        validateChecksumPattern("{{csStart.0}}{{cs.1}}asdf{{csStart.1}}qwer{{csEnd.1}}{{csEnd.0}}{{cs.0}}", ""),
        ChimeraTK::logic_error); // nesting
  }

  /********************************************************************************************************************/
} // end namespace ChimeraTK

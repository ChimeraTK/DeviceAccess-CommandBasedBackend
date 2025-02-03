// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE StringUtils

#include <boost/test/unit_test.hpp>
using namespace boost::unit_test_framework;

#include "stringUtils.h"

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testTokeniseNominal) {
  // Typical use case: some well formed, space separated string tokens
  auto tokens = tokenise("A hello world example!");

  BOOST_TEST(tokens.size() == 4);
  BOOST_REQUIRE(tokens.size() >= 4); // to avoid bad vector access
  BOOST_TEST(tokens[0] == "A");
  BOOST_TEST(tokens[1] == "hello");
  BOOST_TEST(tokens[2] == "world");
  BOOST_TEST(tokens[3] == "example!");
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testTokeniseTrailingEndingWhitespace) {
  // More elaborate example with multiple lines and tab
  auto tokens = tokenise(" A fancy\r\nhello\tworld\rexample! ");

  BOOST_TEST(tokens.size() == 5);
  BOOST_REQUIRE(tokens.size() >= 5); // to avoid bad vector access
  BOOST_TEST(tokens[0] == "A");
  BOOST_TEST(tokens[1] == "fancy");
  BOOST_TEST(tokens[2] == "hello");
  BOOST_TEST(tokens[3] == "world");
  BOOST_TEST(tokens[4] == "example!");
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testTokeniseJustWhitespace) {
  auto tokens = tokenise(" \t ");

  BOOST_TEST(tokens.size() == 0);
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testTokeniseEmptyString) {
  auto tokens = tokenise("");

  BOOST_TEST(tokens.size() == 0);
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testHexConversion) {
  std::string hexInput = "BEEF";

  std::transform(hexInput.begin(), hexInput.end(), hexInput.begin(), ::toupper);

  std::string binaryData = binaryStrFromHexStr(hexInput);
  std::string hexOutput = hexStrFromBinaryStr(binaryData);

  std::transform(hexOutput.begin(), hexOutput.end(), hexOutput.begin(), ::toupper);

  BOOST_CHECK_EQUAL(hexInput, hexOutput);
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testHexConversionOdd) {
  std::string hexInput = "BEEFE";

  std::transform(hexInput.begin(), hexInput.end(), hexInput.begin(), ::toupper);

  std::string binaryData = binaryStrFromHexStr(hexInput);
  std::string hexOutput = hexStrFromBinaryStr(binaryData);

  std::transform(hexOutput.begin(), hexOutput.end(), hexOutput.begin(), ::toupper);
  // Expect hexOutput to be "0BEEFE"

  std::string hexOutputMatchingPart =
      (hexOutput.size() >= hexInput.size()) ? hexOutput.substr(hexOutput.size() - hexInput.size()) : hexOutput;

  BOOST_CHECK_EQUAL(hexOutput[0], '0');
  BOOST_CHECK_EQUAL(hexInput, hexOutputMatchingPart);
}

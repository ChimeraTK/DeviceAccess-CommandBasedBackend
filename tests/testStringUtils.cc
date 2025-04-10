// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE StringUtils

#include <boost/test/unit_test.hpp>
using namespace boost::unit_test_framework;

#include "stringUtils.h"

#include <optional>

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

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(caseInsensitiveStrCompare_tests) {
  BOOST_TEST(caseInsensitiveStrCompare("things", "tHiNgS") == true);
  BOOST_TEST(caseInsensitiveStrCompare("things", "stufff") == false);
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(binaryStrFromInt_tests) {
  std::string res;

  // Test natural width conversion
  res = binaryStrFromInt(static_cast<int32_t>(5)).value_or("");
  BOOST_CHECK_EQUAL(res, "\x05");

  res = binaryStrFromInt(static_cast<int32_t>(-2)).value_or("");
  BOOST_CHECK_EQUAL(res, "\xFE");

  // Test natural width. int32 won't fit in 3 byts, but its ok.
  res = binaryStrFromInt(static_cast<int32_t>(5), 3).value_or("");
  std::string s = {"\x00\x00\x05", 3};
  BOOST_CHECK_EQUAL(res, s);

  res = binaryStrFromInt(static_cast<int32_t>(-2), 3).value_or("");
  BOOST_CHECK_EQUAL(res, "\xFF\xFF\xFE");

  // Test left packing. Put an 8-bit number into a 10 byte string
  res = binaryStrFromInt(static_cast<int8_t>(5), 10).value_or("");
  s = {"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x05", 10};
  BOOST_CHECK_EQUAL(res, s);

  res = binaryStrFromInt(static_cast<int8_t>(-2), 10).value_or("");
  BOOST_CHECK_EQUAL(res, "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFE");

  // Test fixed width with EXPAND behavior, which is default when not overflowing
  res = binaryStrFromInt(static_cast<int32_t>(0xABCDEF), 2, OverflowBehavior::EXPAND).value_or("");
  BOOST_CHECK_EQUAL(res, "\xAB\xCD\xEF");

  // Test fixed width with TRUNCATE behavior. Truncate to 2 bytes
  res = binaryStrFromInt(static_cast<int32_t>(0xABCDEF), 2, OverflowBehavior::TRUNCATE).value_or("");
  BOOST_CHECK_EQUAL(res, "\xCD\xEF");

  // Test fixed width with NULLOPT overflow behavior
  bool hasValue = binaryStrFromInt(static_cast<int32_t>(300), 1, OverflowBehavior::NULLOPT).has_value();
  BOOST_CHECK_EQUAL(hasValue, false);
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(intFromBinaryStr_tests) {
  // Test conversion from binary string, as well as left-packing.
  BOOST_CHECK_EQUAL(intFromBinaryStr<int32_t>({"\0\x05", 2}).value_or(-999), /*==*/5);

  BOOST_CHECK_EQUAL(intFromBinaryStr<int32_t>({"\xFF\xFE"}).value_or(-999), /*==*/-2);
  BOOST_CHECK_EQUAL(intFromBinaryStr<uint32_t>({"\xFF"}).value_or(-999), /*==*/255);

  // Test natural width. Put 10 bytes of string into 8 bits of int, but it's ok.
  std::string s = std::string("\x00\x00\x00\x00\x00\x00\x00\x00\x00\x05", 10);
  int8_t is1 = intFromBinaryStr<int8_t>(s).value_or(-1);
  BOOST_CHECK_EQUAL(is1, 5);

  int8_t is2 = intFromBinaryStr<int8_t>({"\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFE"}).value_or(-1);
  BOOST_CHECK_EQUAL(is2, -2);

  // Test overflow case with truncation
  bool truncateIfOverflow = true;
  s = std::string("\xF0\x00\x00\x05", 4);
  BOOST_CHECK_EQUAL(intFromBinaryStr<int8_t>(s, truncateIfOverflow).value_or(-9), /*==*/5);

  // Test overflow case without truncation
  truncateIfOverflow = false;
  BOOST_CHECK_EQUAL(intFromBinaryStr<int8_t>(s, truncateIfOverflow).has_value(), /*==*/false);
}

/**********************************************************************************************************************/

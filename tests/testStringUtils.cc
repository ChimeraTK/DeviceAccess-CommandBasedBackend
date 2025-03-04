// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE StringUtils

#include <boost/test/unit_test.hpp>
using namespace boost::unit_test_framework;

#include "stringUtils.h"

#include <ChimeraTK/SupportedUserTypes.h>

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
  std::string s;
  BOOST_CHECK_EQUAL(intFromBinaryStr<int32_t>({"\0\x05", 2}).value_or(-999), /*==*/5);

  BOOST_CHECK_EQUAL(intFromBinaryStr<int32_t>({"\xFF\xFE"}).value_or(-999), /*==*/-2);
  BOOST_CHECK_EQUAL(intFromBinaryStr<uint32_t>({"\xFF"}).value_or(-999), /*==*/255);

  // Test natural width. Put 10 bytes of string into 8 bits of int, but it's ok.
  s = std::string("\x00\x00\x00\x00\x00\x00\x00\x00\x00\x05", 10);
  BOOST_CHECK_EQUAL(intFromBinaryStr<int8_t>(s).value_or(-1), /*==*/5); // FAIL [garbage] != 5
  BOOST_CHECK_EQUAL(intFromBinaryStr<int8_t>({"\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFE"}).value_or(-1),
      /*==*/-2); // FAIL [garbage] != -2

  // Test overflow case with truncation
  bool truncateIfOverflow = true;
  s = std::string("\xF0\x00\x00\x05", 4);
  BOOST_CHECK_EQUAL(intFromBinaryStr<int8_t>(s, truncateIfOverflow).value_or(-999),
      /*==*/5); // Fails, appears to runaway.
                //"natural width 0, max bytes1, is negative

  // Test overflow case without truncation
  truncateIfOverflow = false;
  BOOST_CHECK_EQUAL(intFromBinaryStr<int8_t>(s, truncateIfOverflow).has_value(), /*==*/false); // Fails
}

BOOST_AUTO_TEST_CASE(toTransportLayerHexInt_test) {
  // This tests the mechanics of toTransportLayerHexInt, which is also used for binary int.

  // no fixed width, odd number of characters
  int32_t iIn = 0xD0E;
  std::string ans{"D0E", 3};
  std::string hexOut = hexStrFromBinaryStr(binaryStrFromInt<int32_t>(iIn, std::nullopt).value());
  BOOST_CHECK_EQUAL(hexOut, ans);

  // no fixed width, even number of characters
  iIn = 0xAB0C;
  ans = std::string("AB0C", 4);
  hexOut = hexStrFromBinaryStr(binaryStrFromInt<int32_t>(iIn, std::nullopt).value());
  BOOST_CHECK_EQUAL(hexOut, ans);

  // fixed width, unsigned, even number of characters in, even out
  iIn = 0xAB0C;
  ans = std::string("00AB0C", 6);
  hexOut = hexStrFromBinaryStr(binaryStrFromInt<int32_t>(iIn, std::nullopt).value(), 6, false);
  BOOST_CHECK_EQUAL(hexOut, ans);

  // fixed width, unsigned, even number of characters in, odd out
  iIn = 0xAB0C;
  ans = std::string("0AB0C", 5);
  hexOut = hexStrFromBinaryStr(binaryStrFromInt<int32_t>(iIn, std::nullopt).value(), 5, false);
  BOOST_CHECK_EQUAL(hexOut, ans);

  // fixed width, unsigned, odd number of characters in, odd number out
  iIn = 0xD0E;
  ans = std::string("00D0E", 5);
  hexOut = hexStrFromBinaryStr(binaryStrFromInt<int32_t>(iIn, std::nullopt).value(), 5, false);
  BOOST_CHECK_EQUAL(hexOut, ans);

  // fixed width, unsigned, odd number of characters in, even number out
  iIn = 0xD0E;
  ans = std::string("000D0E", 6);
  hexOut = hexStrFromBinaryStr(binaryStrFromInt<int32_t>(iIn, std::nullopt).value(), 6, false);
  BOOST_CHECK_EQUAL(hexOut, ans);

  // fixed width, signed, positive, even number of characters
  iIn = 0xAB0C;
  ans = std::string("00AB0C", 6);
  hexOut = hexStrFromBinaryStr(binaryStrFromInt<int32_t>(iIn, std::nullopt).value(), 6, true);
  BOOST_CHECK_EQUAL(hexOut, ans);

  // fixed width, signed, positive, odd number of characters
  iIn = 0xD0E;
  ans = std::string("00D0E", 5);
  hexOut = hexStrFromBinaryStr(binaryStrFromInt<int32_t>(iIn, std::nullopt).value(), 5, true);
  BOOST_CHECK_EQUAL(hexOut, ans);

  // fixed width, signed, negative, even number of characters
  iIn = -1 * (0xAB0C);
  ans = std::string("FF54f4", 6);
  hexOut = hexStrFromBinaryStr(binaryStrFromInt<int32_t>(iIn, std::nullopt).value(), 6, true);
  BOOST_CHECK_EQUAL(hexOut, ans);

  // fixed width, signed, negative, odd number of characters
  iIn = -1 * (0xD0E);
  ans = std::string("FF2F2", 5);
  hexOut = hexStrFromBinaryStr(binaryStrFromInt<int32_t>(iIn, std::nullopt).value(), 5, true);
  BOOST_CHECK_EQUAL(hexOut, ans);

  // Trivial case
  iIn = 0;
  ans = std::string("000", 3);
  hexOut = hexStrFromBinaryStr(binaryStrFromInt<int32_t>(iIn, std::nullopt).value(), 3, true);
  BOOST_CHECK_EQUAL(hexOut, ans);
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(toUserTypeHexInt_test) {
  // This tests the mechanics of toUserTypeHexInt, which is also used for binary int.

  // Unsigned int cases
  std::string str{"AB0C", 4};
  uint16_t uiAns = 0xAB0C;
  uint16_t uiTest = ChimeraTK::userTypeToUserType<uint16_t, std::string>("0x" + str);
  BOOST_CHECK_EQUAL(uiTest, uiAns);

  str = std::string("A0B", 3);
  uiAns = 0xA0B;
  uiTest = ChimeraTK::userTypeToUserType<uint16_t, std::string>("0x" + str);
  BOOST_CHECK_EQUAL(uiTest, uiAns);

  // Trivial case, unsigned int
  str = std::string("0", 1);
  uiAns = 0x0;
  uiTest = ChimeraTK::userTypeToUserType<uint16_t, std::string>("0x" + str);
  BOOST_CHECK_EQUAL(uiTest, uiAns);

  // Signed int cases:
  // positive even
  str = std::string("2B0C", 4);
  int16_t iAns = 0x2B0C;
  int16_t iTest = intFromBinaryStr<int16_t>(binaryStrFromHexStr(str)).value();
  BOOST_CHECK_EQUAL(iTest, iAns);

  // positive odd
  str = std::string("20B", 3);
  iAns = 0x20B;
  iTest = intFromBinaryStr<int16_t>(binaryStrFromHexStr(str)).value();
  BOOST_CHECK_EQUAL(iTest, iAns);

  // negative even
  iAns = -1 * static_cast<int16_t>(0x2B0C);
  str = std::string("AB0C", 4);
  iTest = intFromBinaryStr<int16_t>(binaryStrFromHexStr(str)).value();
  BOOST_CHECK_EQUAL(iTest, iAns);

  // negative odd
  iAns = -1 * static_cast<int16_t>(0x20B);
  str = std::string("A0B", 3);
  iTest = intFromBinaryStr<int16_t>(binaryStrFromHexStr(str)).value();
  BOOST_CHECK_EQUAL(iTest, iAns);

  // Trivial case, signed int
  iAns = 0;
  str = std::string("0", 1);
  iTest = intFromBinaryStr<int16_t>(binaryStrFromHexStr(str)).value();
  BOOST_CHECK_EQUAL(iTest, iAns);
}

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
  std::string h1 = "BEEF"; // basic test with no nulls.
  std::string b1 = binaryStrFromHexStr(h1);
  strCmp(b1, "\xBE\xEF");
  BOOST_CHECK_EQUAL(b1, "\xBE\xEF");
  std::string h1V1 = hexStrFromBinaryStr(b1);
  std::string h1V2 = hexStrFromBinaryStr(b1, 4);
  BOOST_CHECK_EQUAL(h1, h1V1);
  BOOST_CHECK_EQUAL(h1, h1V2);

  std::string h2 = "00AB00CD"; // basic test with nulls, evens
  std::string b2 = binaryStrFromHexStr(h2);
  // reminder: printable replaces null characters with "\\0" so string comparison doesn't choke on null.
  BOOST_CHECK_EQUAL(printable(b2), "\\0\xAB\\0\xCD");
  std::string h2V1 = hexStrFromBinaryStr(b2);
  std::string h2V2 = hexStrFromBinaryStr(b2, h2.length());
  BOOST_CHECK_EQUAL(h2.size(), h2V1.size());
  BOOST_CHECK_EQUAL(h2.size(), h2V2.size());
  BOOST_CHECK_EQUAL(h2, h2V1);
  BOOST_CHECK_EQUAL(h2, h2V2);

  std::string h3 = "ABCDE";                               // odd, pad left
  std::string b3L = binaryStrFromHexStr(h3, true, false); // pad left with 0 like a uint
  strCmp(b3L, "\x0A\xBC\xDE");
  BOOST_CHECK_EQUAL(b3L, "\x0A\xBC\xDE");

  std::string b3R = binaryStrFromHexStr(h3, false); // pad right
  strCmp(b3R, "\xAB\xCD\xE0");                      // DEBUG
  BOOST_CHECK_EQUAL(b3R, "\xAB\xCD\xE0");
  std::string h3V1 = hexStrFromBinaryStr(b3L, h3.length());
  // hexStrFromBinaryStr doesn't support right-padding, so no test
  strCmp(h3, h3V1); // DEBUG
  BOOST_CHECK_EQUAL(h3, h3V1);

  std::string h4 = "0ABC0";                  // odd, pad left, building nulls
  std::string b4L = binaryStrFromHexStr(h4); // pad left to null
  std::string b4L_cmp{"\0\xAB\xC0", 3};
  strCmp(b4L, b4L_cmp); // DEBUG
  BOOST_CHECK_EQUAL(printable(b4L), "\\0\xAB\xC0");

  std::string b4R = binaryStrFromHexStr(h4, false); // pad right to null
  std::string b4R_cmp{"\x0A\xBC\0", 3};
  strCmp(b4R, b4R_cmp); // DEBUG
  BOOST_CHECK_EQUAL(printable(b4R), "\x0A\xBC\\0");

  std::string h4V1 = hexStrFromBinaryStr(b4L, h4.length());
  // hexStrFromBinaryStr doesn't support right-padding, so no test
  BOOST_CHECK_EQUAL(h4.size(), h4V1.size());
  BOOST_CHECK_EQUAL(h4, h4V1);

  // Evaluate to a hex container that is larger than the input binary, with signed behavior test
  std::string h2ExtUnsigned = hexStrFromBinaryStr(b2, h2.size() + 3);
  std::string h2ExtUnsignedExpected = "000" + h2;
  BOOST_CHECK_EQUAL(h2ExtUnsigned, h2ExtUnsignedExpected);

  // Signed positive
  std::string h2ExtSigned = hexStrFromBinaryStr(b2, h2.size() + 3, true);
  std::string h2ExtSignedExpected = "000" + h2;
  BOOST_CHECK_EQUAL(h2ExtSigned, h2ExtSignedExpected);

  // Signed negative
  std::string h1ExtSigned = hexStrFromBinaryStr(b1, h1.size() + 3, true);
  std::string h1ExtSignedExpected = "FFF" + h1;
  BOOST_CHECK_EQUAL(h1ExtSigned, h1ExtSignedExpected);
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(nullReplacement_test) {
  std::string s = {"rtyuiR67\089oi", 13};
  s[5] = '\x00'; // replace 'R' with null so that the string literal use the single unicode char '\x0067'

  BOOST_CHECK_EQUAL(printable(s), "rtyui\\067\\089oi");

  std::string d = denull(s);
  BOOST_CHECK_EQUAL(d, "rtyuiNULLCHAR_E0xUr3HTw@_67NULLCHAR_E0xUr3HTw@_89oi");

  std::string r = renull(d);
  BOOST_CHECK_EQUAL(printable(r), printable(s));
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
  res = binaryStrFromInt(static_cast<uint32_t>(0xABCDEF), 2, false, OverflowBehavior::EXPAND).value_or("");
  std::string ans{"\xAB\xCD\xEF", 3};
  strCmp(res, ans);
  BOOST_CHECK_EQUAL(res, ans);

  res = binaryStrFromInt(static_cast<int32_t>(0xABCDEF), 2, true, OverflowBehavior::EXPAND).value_or("");
  ans = std::string("\0\xAB\xCD\xEF", 4); // Another byte gets added to show the sign.
  strCmp(res, ans);
  BOOST_CHECK_EQUAL(res, ans);

  // Test fixed width with TRUNCATE behavior. Truncate to 2 bytes
  res = binaryStrFromInt(static_cast<int32_t>(0xABCDEF), 2, true, OverflowBehavior::TRUNCATE).value_or("");
  BOOST_CHECK_EQUAL(res, "\xCD\xEF");

  // Test fixed width with NULLOPT overflow behavior
  bool hasValue = binaryStrFromInt(static_cast<int32_t>(300), 1, true, OverflowBehavior::NULLOPT).has_value();
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

BOOST_AUTO_TEST_CASE(toTransportLayerHexInt_test) {
  // This tests the mechanics of toTransportLayerHexInt, which is also used for binary int.

  // no fixed width, odd number of characters
  int32_t iIn = 0xD0E;
  std::string ans{"0D0E",
      4}; // We get 2 bytes out of the binaryStringFromInt, which is unconstrained to 3, so it becomes 4 hex digits
  std::string hexOut = hexStrFromBinaryStr(binaryStrFromInt<int32_t>(iIn, std::nullopt).value());
  BOOST_CHECK_EQUAL(hexOut, ans);

  // no fixed width, even number of characters
  iIn = 0xAB0C;
  ans = std::string("AB0C", 4);
  hexOut = hexStrFromBinaryStr(binaryStrFromInt<uint32_t>(iIn, std::nullopt).value());
  // If set to binaryStrFromInt<int32_t> rather than uint32, we expect "00AB0C"
  BOOST_CHECK_EQUAL(hexOut, ans);

  // fixed width, unsigned, even number of characters in, even out
  iIn = 0xAB0C;
  size_t width = 6;
  ans = std::string("00AB0C", width);
  hexOut = hexStrFromBinaryStr(binaryStrFromInt<int32_t>(iIn, 2 * width).value(), width, false);
  BOOST_CHECK_EQUAL(hexOut, ans);

  // fixed width, unsigned, even number of characters in, odd out
  iIn = 0xAB0C;
  width = 5;
  ans = std::string("0AB0C", width);
  hexOut = hexStrFromBinaryStr(binaryStrFromInt<int32_t>(iIn, 2 * width).value(), width, false);
  BOOST_CHECK_EQUAL(hexOut, ans);

  // fixed width, unsigned, odd number of characters in, odd number out
  iIn = 0xD0E;
  width = 5;
  ans = std::string("00D0E", width);
  hexOut = hexStrFromBinaryStr(binaryStrFromInt<int32_t>(iIn, 2 * width).value(), width, false);
  BOOST_CHECK_EQUAL(hexOut, ans);

  // fixed width, unsigned, odd number of characters in, even number out
  iIn = 0xD0E;
  width = 6;
  ans = std::string("000D0E", width);
  hexOut = hexStrFromBinaryStr(binaryStrFromInt<int32_t>(iIn, 2 * width).value(), width, false);
  BOOST_CHECK_EQUAL(hexOut, ans);

  // fixed width, signed, positive, even number of characters
  iIn = 0xAB0C;
  width = 6;
  ans = std::string("00AB0C", width);
  hexOut = hexStrFromBinaryStr(binaryStrFromInt<int32_t>(iIn, 2 * width).value(), width, true);
  BOOST_CHECK_EQUAL(hexOut, ans);

  // REDUNDANT just checking
  hexOut = hexStrFromBinaryStr(binaryStrFromInt<int32_t>(iIn).value(), width, true);
  BOOST_CHECK_EQUAL(hexOut, ans); // FAIL [FFAB0C != 00AB0C]
  // But this is all crazy. Just go directly to hex.

  // fixed width, signed, positive, odd number of characters
  iIn = 0xD0E;
  width = 5;
  ans = std::string("00D0E", width);
  hexOut = hexStrFromBinaryStr(binaryStrFromInt<int32_t>(iIn, 2 * width).value(), width, true);
  BOOST_CHECK_EQUAL(hexOut, ans);

  // fixed width, signed, negative, even number of characters
  iIn = -1 * (0xAB0C);
  width = 6;
  ans = std::string("FF54F4", width);
  hexOut = hexStrFromBinaryStr(binaryStrFromInt<int32_t>(iIn, 2 * width).value(), 6, true);
  BOOST_CHECK_EQUAL(hexOut, ans);

  // fixed width, signed, negative, odd number of characters
  iIn = -1 * (0xD0E);
  width = 5;
  ans = std::string("FF2F2", width);
  hexOut = hexStrFromBinaryStr(binaryStrFromInt<int32_t>(iIn, 2 * width).value(), width, true);
  BOOST_CHECK_EQUAL(hexOut, ans);

  // Trivial case
  iIn = 0;
  width = 3;
  ans = std::string("000", 3);
  hexOut = hexStrFromBinaryStr(binaryStrFromInt<int32_t>(iIn, 2 * width).value(), width, true);
  BOOST_CHECK_EQUAL(hexOut, ans);
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(toUserTypeHexInt_test) {
  // This tests the mechanics of toUserTypeHexInt, which is also used for binary int.

  std::string str;

  // Unsigned int cases
  uint16_t uiAns, uiTest;

  str = std::string("AB0C", 4);
  uiAns = 0xAB0C;
  uiTest = ChimeraTK::userTypeToUserType<uint16_t, std::string>("0x" + str);
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
  int16_t iAns, iTest;

  // positive even
  str = std::string("2B0C", 4);
  iAns = 0x2B0C;
  iTest = intFromBinaryStr<int16_t>(binaryStrFromHexStr(str)).value();
  BOOST_CHECK_EQUAL(iTest, iAns);

  // positive odd
  str = std::string("20B", 3);
  iAns = 0x20B;
  iTest = intFromBinaryStr<int16_t>(binaryStrFromHexStr(str)).value();
  BOOST_CHECK_EQUAL(iTest, iAns);

  // negative even
  str = std::string("AB0C", 4);
  iAns = -1 * static_cast<int16_t>(0x54F4);
  iTest = intFromBinaryStr<int16_t>(binaryStrFromHexStr(str, true, true)).value();
  BOOST_CHECK_EQUAL(iTest, iAns);

  // negative odd
  str = std::string("A0B", 3);
  iAns = -1 * static_cast<int16_t>(0x5F5);
  iTest = intFromBinaryStr<int16_t>(binaryStrFromHexStr(str, true, true)).value();
  BOOST_CHECK_EQUAL(iTest, iAns);

  // Trivial case, signed int
  iAns = 0;
  str = std::string("0", 1);
  iTest = intFromBinaryStr<int16_t>(binaryStrFromHexStr(str)).value();
  BOOST_CHECK_EQUAL(iTest, iAns);
}

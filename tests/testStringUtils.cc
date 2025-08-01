// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE StringUtils

#include <boost/test/unit_test.hpp>
using namespace boost::unit_test_framework;

#include "stringUtils.h"

#include <ChimeraTK/SupportedUserTypes.h>

#include <cfloat>
#include <optional>
#include <tuple>

/**********************************************************************************************************************/
#define NICE_CHECK_EQUAL(A, B)                                                                                         \
  do {                                                                                                                 \
    strCmp((A), (B));                                                                                                  \
    BOOST_CHECK_EQUAL((A), (B));                                                                                       \
  } while(false)
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
  NICE_CHECK_EQUAL(b1, "\xBE\xEF");
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

  std::string h3 = "ABCDE"; // odd
  std::string b3L = binaryStrFromHexStr(h3, false);
  NICE_CHECK_EQUAL(b3L, "\x0A\xBC\xDE");

  std::string h4 = "0ABC0"; // odd, building nulls
  std::string b4L = binaryStrFromHexStr(h4);
  std::string b4L_cmp{"\0\xAB\xC0", 3};
  NICE_CHECK_EQUAL(printable(b4L), "\\0\xAB\xC0");

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

constexpr OverflowBehavior TRUNCATE = OverflowBehavior::TRUNCATE;
constexpr OverflowBehavior EXPAND = OverflowBehavior::EXPAND;
constexpr OverflowBehavior NULLOPT = OverflowBehavior::NULLOPT;
constexpr WidthOption COMPACT = WidthOption::COMPACT;
constexpr WidthOption TYPE_WIDTH = WidthOption::TYPE_WIDTH;
std::array<WidthOption, 2> widthOptions = {COMPACT, TYPE_WIDTH};
std::array<OverflowBehavior, 3> overflowBehaviors = {TRUNCATE, EXPAND, NULLOPT};
using Boolean = ChimeraTK::Boolean;
const std::string NO = "nullopt";

BOOST_AUTO_TEST_CASE(binaryStrFromInt_tests) {
  const std::string ZERO_STR("\x00", 1);
  size_t width;

  /*------------------------------------------------------------------------------------------------------------------*/
  { // bool
    std::vector<std::pair<bool, std::string>> testCases = {{false, ZERO_STR}, {true, "\x01"}};
    for(const auto& [input, expected] : testCases) {
      for(auto widthOpt : widthOptions) {
        for(bool isSigned : {true, false}) {
          NICE_CHECK_EQUAL(binaryStrFromInt(input, widthOpt, isSigned).value_or(NO), expected);
        }
        NICE_CHECK_EQUAL(binaryStrFromInt(input, widthOpt).value_or(NO), expected);
      }
      NICE_CHECK_EQUAL(binaryStrFromInt(input).value_or(NO), expected);
    }

    // natural size of input exceeds the fixed width
    width = 0;
    for(const auto& [input, expected] : testCases) {
      for(bool isSigned : {true, false}) {
        NICE_CHECK_EQUAL(binaryStrFromInt(input, width, isSigned, EXPAND).value_or(NO), expected);
        NICE_CHECK_EQUAL(binaryStrFromInt(input, width, isSigned, TRUNCATE).value_or(NO), ZERO_STR);
        NICE_CHECK_EQUAL(binaryStrFromInt(input, width, isSigned, NULLOPT).value_or(NO), NO);
      }
    }

    // natural size is less than or equal to the fixed width, so expand
    for(width = 1; width <= 5; width++) {
      for(const auto& [input, expected] : testCases) {
        for(bool isSigned : {true, false}) {
          for(auto overflowBehavior : overflowBehaviors) {
            NICE_CHECK_EQUAL(binaryStrFromInt(input, width, isSigned, overflowBehavior).value_or(NO), expected);
          }
          NICE_CHECK_EQUAL(binaryStrFromInt(input, width, isSigned).value_or(NO), expected);
        }
        NICE_CHECK_EQUAL(binaryStrFromInt(input, width).value_or(NO), expected);
      }

      for(auto& p : testCases) { // prepend zero bytes to all test cases
        p.second = ZERO_STR + p.second;
      }
    }

  } // end bool

  /*------------------------------------------------------------------------------------------------------------------*/
  { // Boolean
    std::vector<std::pair<Boolean, std::string>> testCases = {{false, ZERO_STR}, {true, "\x01"}};
    for(const auto& [input, expected] : testCases) {
      for(auto widthOpt : widthOptions) {
        for(bool isSigned : {true, false}) {
          NICE_CHECK_EQUAL(binaryStrFromInt(input, widthOpt, isSigned).value_or(NO), expected);
        }
        NICE_CHECK_EQUAL(binaryStrFromInt(input, widthOpt).value_or(NO), expected);
      }
      NICE_CHECK_EQUAL(binaryStrFromInt(input).value_or(NO), expected);
    }

    // natural size of input exceeds the fixed width
    width = 0;
    for(const auto& [input, expected] : testCases) {
      for(bool isSigned : {true, false}) {
        NICE_CHECK_EQUAL(binaryStrFromInt(input, width, isSigned, EXPAND).value_or(NO), expected);
        NICE_CHECK_EQUAL(binaryStrFromInt(input, width, isSigned, TRUNCATE).value_or(NO), ZERO_STR);
        NICE_CHECK_EQUAL(binaryStrFromInt(input, width, isSigned, NULLOPT).value_or(NO), NO);
      }
    }

    // natural size is less than or equal to the fixed width, so expand
    for(width = 1; width <= 5; width++) {
      for(const auto& [input, expected] : testCases) {
        for(bool isSigned : {true, false}) {
          for(auto overflowBehavior : overflowBehaviors) {
            NICE_CHECK_EQUAL(binaryStrFromInt(input, width, isSigned, overflowBehavior).value_or(NO), expected);
          }
          NICE_CHECK_EQUAL(binaryStrFromInt(input, width, isSigned).value_or(NO), expected);
        }
        NICE_CHECK_EQUAL(binaryStrFromInt(input, width).value_or(NO), expected);
      }

      for(auto& p : testCases) { // prepend zero bytes to all test cases
        p.second = ZERO_STR + p.second;
      }
    }
  } // end Boolean

  /*------------------------------------------------------------------------------------------------------------------*/
  { // uint8_t
    std::vector<std::pair<uint8_t, std::string>> testCases = {
        {0, ZERO_STR},
        {5, "\x05"},
        {0xAB, "\xAB"},
        {std::numeric_limits<uint8_t>::max(), "\xFF"},
    };
    for(const auto& [input, expected] : testCases) {
      for(auto widthOpt : widthOptions) {
        for(bool isSigned : {true, false}) {
          NICE_CHECK_EQUAL(binaryStrFromInt(input, widthOpt, isSigned).value_or(NO), expected);
        }
        NICE_CHECK_EQUAL(binaryStrFromInt(input, widthOpt).value_or(NO), expected);
      }
      NICE_CHECK_EQUAL(binaryStrFromInt(input).value_or(NO), expected);
    }

    // natural size of input equals the fixed width
    std::vector<std::tuple<uint8_t, size_t, std::string>> fixedWidthEqualSizeTestCases = {
        {0, 1, ZERO_STR},
        {0xAB, 1, "\xAB"},
        {std::numeric_limits<uint8_t>::max(), 1, "\xFF"},
    };
    for(const auto& [input, width_, expected] : fixedWidthEqualSizeTestCases) {
      for(bool isSigned : {true, false}) {
        for(auto overflowBehavior : overflowBehaviors) {
          NICE_CHECK_EQUAL(binaryStrFromInt(input, width_, isSigned, overflowBehavior).value_or(NO), expected);
        }
        NICE_CHECK_EQUAL(binaryStrFromInt(input, width_, isSigned).value_or(NO), expected);
      }
      NICE_CHECK_EQUAL(binaryStrFromInt(input, width_).value_or(NO), expected);
    }

    // natural size is less than the fixed width parameter, unsigned
    std::vector<std::tuple<uint8_t, size_t, std::string>> fixedWidthUnsignedLesserSizeTestCases = {
        {0, 3, std::string("\0\0\x00", 3)},
        {5, 1, "\x05"},
        {5, 3, std::string("\0\0\x05", 3)},
        {0xAB, 3, std::string("\0\0\xAB", 3)},
        {std::numeric_limits<uint8_t>::max(), 4, std::string("\0\0\0\xFF", 4)},
    };
    for(const auto& [input, width_, expected] : fixedWidthUnsignedLesserSizeTestCases) {
      for(auto overflowBehavior : overflowBehaviors) {
        bool isSigned = false;
        NICE_CHECK_EQUAL(binaryStrFromInt(input, width_, isSigned, overflowBehavior).value_or(NO), expected);
        NICE_CHECK_EQUAL(binaryStrFromInt(input, width_, isSigned).value_or(NO), expected);
      }
      NICE_CHECK_EQUAL(binaryStrFromInt(input, width_).value_or(NO), expected);
    }

    // natural size is less than the fixed width parameter, signed
    std::vector<std::tuple<uint8_t, size_t, std::string>> fixedWidthSignedLesserSizeTestCases = {
        {5, 1, "\x05"},
        {5, 2, std::string("\0\x05", 2)},
        {5, 3, std::string("\0\0\x05", 3)},
        {0, 3, std::string("\0\0\0", 3)},
        {0xAB, 3, std::string("\xFF\xFF\xAB", 3)},
        {std::numeric_limits<uint8_t>::max(), 3, std::string("\xFF\xFF\xFF\xFF", 3)},
    };
    for(const auto& [input, width_, expected] : fixedWidthSignedLesserSizeTestCases) {
      for(auto overflowBehavior : overflowBehaviors) {
        bool isSigned = true;
        NICE_CHECK_EQUAL(binaryStrFromInt(input, width_, isSigned, overflowBehavior).value_or(NO), expected);
        NICE_CHECK_EQUAL(binaryStrFromInt(input, width_, isSigned).value_or(NO), expected);
      }
    }

    // natural size is greater than the fixed width parameter, signed
    width = 0;
    for(const auto& [input, expected] : testCases) {
      for(bool isSigned : {true, false}) {
        NICE_CHECK_EQUAL(binaryStrFromInt(input, width, isSigned, EXPAND).value_or(NO), expected);
        NICE_CHECK_EQUAL(binaryStrFromInt(input, width, isSigned, TRUNCATE).value_or(NO), ZERO_STR);
        NICE_CHECK_EQUAL(binaryStrFromInt(input, width, isSigned, NULLOPT).value_or(NO), NO);
      }
    }
  } // end uint8_t

  /*------------------------------------------------------------------------------------------------------------------*/
  { // uint16_t
    const int nBytes = 16 / 8;

    // COMPACT, unsigned
    std::vector<std::pair<uint16_t, std::string>> compactUnsignedTestCases = {
        {0, ZERO_STR},
        {5, "\x05"},
        {0xAB, "\xAB"},
        {0xABC, "\x0A\xBC"},
        {0xABCD, "\xAB\xCD"},
        {std::numeric_limits<uint16_t>::max(), "\xFF\xFF"},
    };
    for(const auto& [input, expected] : compactUnsignedTestCases) {
      bool isSigned = false;
      NICE_CHECK_EQUAL(binaryStrFromInt(input, COMPACT, isSigned).value_or(NO), expected);
      NICE_CHECK_EQUAL(binaryStrFromInt(input, COMPACT).value_or(NO), expected);
    }

    // COMPACT, signed
    std::vector<std::pair<uint16_t, std::string>> compactSignedtestCases = {
        {0x7FFF, "\x7F\xFF"}, // max positive int
        {0xABCD, "\xAB\xCD"}, {0xABC, "\x0A\xBC"}, {0xAB, std::string("\0\xAB", 2)}, {5, "\x05"}, {0, ZERO_STR},
        {std::numeric_limits<uint16_t>::max(), "\xFF"}, // interpreted as -1, left F's get compacted
        {0xFFFB, "\xFB"},                               //-5
        {0xFF7F, "\xFF\x7F"},                           //-129, need 2nd byte for sign
    };
    for(const auto& [input, expected] : compactSignedtestCases) {
      bool isSigned = true;
      NICE_CHECK_EQUAL(binaryStrFromInt(input, COMPACT, isSigned).value_or(NO), expected);
    }

    // TYPE_WIDTH
    std::vector<std::pair<uint16_t, std::string>> typeWidthTestCases = {
        {0, std::string("\0\0", nBytes)},
        {5, std::string("\0\x05", nBytes)},
        {0xAB, std::string("\0\xAB", nBytes)},
        {0xABC, std::string("\x0A\xBC", nBytes)},
        {0xABCD, std::string("\xAB\xCD", nBytes)},
        {std::numeric_limits<uint16_t>::max(), std::string("\xFF\xFF", nBytes)},
    };
    for(const auto& [input, expected] : typeWidthTestCases) {
      for(bool isSigned : {true, false}) {
        NICE_CHECK_EQUAL(binaryStrFromInt(input, TYPE_WIDTH, isSigned).value_or(NO), expected);
      }
      NICE_CHECK_EQUAL(binaryStrFromInt(input, TYPE_WIDTH).value_or(NO), expected);
    }

    // natural size of input equals the fixed width
    std::vector<std::tuple<uint16_t, size_t, std::string>> fixedWidthEqualSizeTestCases = {
        {0, 1, ZERO_STR},
        {0x11, 1, "\x11"},
        {0x7B, 1, "\x7B"}, // NOLINT(modernize-raw-string-literal) Use leading 7 so the signed case doesn't prepend \x00
        {0x7BCD, 2, "\x7B\xCD"},
        {std::numeric_limits<uint16_t>::max(), 2, "\xFF\xFF"},
    };
    for(const auto& [input, width_, expected] : fixedWidthEqualSizeTestCases) {
      for(bool isSigned : {true, false}) {
        for(auto overflowBehavior : overflowBehaviors) {
          NICE_CHECK_EQUAL(binaryStrFromInt(input, width_, isSigned, overflowBehavior).value_or(NO), expected);
        }
        NICE_CHECK_EQUAL(binaryStrFromInt(input, width_, isSigned).value_or(NO), expected);
      }
      NICE_CHECK_EQUAL(binaryStrFromInt(input, width_).value_or(NO), expected);
    }

    // natural size is less than the fixed width parameter, unsigned
    std::vector<std::tuple<uint16_t, size_t, std::string>> fixedWidthUnsignedLesserSizeTestCases = {
        {0, 3, std::string("\0\0\x00", 3)},
        {5, 1, "\x05"},
        {5, 3, std::string("\0\0\x05", 3)},
        {0xAB, 2, std::string("\0\xAB", 2)},
        {0xABC, 3, std::string("\0\x0A\xBC", 3)},
        {0xABCD, 3, std::string("\0\xAB\xCD", 3)},
        {std::numeric_limits<uint16_t>::max(), 4, std::string("\0\0\xFF\xFF", 4)},
    };
    for(const auto& [input, width_, expected] : fixedWidthUnsignedLesserSizeTestCases) {
      for(auto overflowBehavior : overflowBehaviors) {
        bool isSigned = false;
        NICE_CHECK_EQUAL(binaryStrFromInt(input, width_, isSigned, overflowBehavior).value_or(NO), expected);
        NICE_CHECK_EQUAL(binaryStrFromInt(input, width_, isSigned).value_or(NO), expected);
      }
      NICE_CHECK_EQUAL(binaryStrFromInt(input, width_).value_or(NO), expected);
    }

    // natural size is less than the fixed width parameter, signed
    std::vector<std::tuple<uint16_t, size_t, std::string>> fixedWidthSignedLesserSizeTestCases = {
        {5, 1, "\x05"},
        {5, 2, std::string("\0\x05", 2)},
        {5, 3, std::string("\0\0\x05", 3)},
        {0, 3, std::string("\0\0\0", 3)},
        {0xFFAB, 3, std::string("\xFF\xFF\xAB", 3)},
        {std::numeric_limits<uint16_t>::max(), 3, std::string("\xFF\xFF\xFF\xFF", 3)},
    };
    for(const auto& [input, width_, expected] : fixedWidthSignedLesserSizeTestCases) {
      for(auto overflowBehavior : overflowBehaviors) {
        bool isSigned = true;
        NICE_CHECK_EQUAL(binaryStrFromInt(input, width_, isSigned, overflowBehavior).value_or(NO), expected);
        NICE_CHECK_EQUAL(binaryStrFromInt(input, width_, isSigned).value_or(NO), expected);
      }
    }

    // natural size is greater than the fixed width parameter
    width = 0;
    std::vector<std::pair<uint16_t, std::string>> fixedWidthTestCases = {
        {0, ZERO_STR},
        {5, "\x05"},
        {0x7B, "\x7B"}, // NOLINT(modernize-raw-string-literal)
        {0xABC, "\x0A\xBC"},
        {0x7BCD, "\x7B\xCD"},
    };
    for(const auto& [input, expected] : fixedWidthTestCases) {
      for(bool isSigned : {true, false}) {
        NICE_CHECK_EQUAL(binaryStrFromInt(input, width, isSigned, EXPAND).value_or(NO), expected);
        NICE_CHECK_EQUAL(binaryStrFromInt(input, width, isSigned, TRUNCATE).value_or(NO), ZERO_STR);
        NICE_CHECK_EQUAL(binaryStrFromInt(input, width, isSigned, NULLOPT).value_or(NO), NO);
      }
    }
    width = 1;
    for(bool isSigned : {true, false}) {
      NICE_CHECK_EQUAL(binaryStrFromInt<uint16_t>(0xABCD, width, isSigned, EXPAND).value_or(NO), "\xAB\xCD");
      NICE_CHECK_EQUAL(binaryStrFromInt<uint16_t>(0xABCD, width, isSigned, TRUNCATE).value_or(NO), "\xCD");
      NICE_CHECK_EQUAL(binaryStrFromInt<uint16_t>(0xABCD, width, isSigned, NULLOPT).value_or(NO), NO);

      NICE_CHECK_EQUAL(binaryStrFromInt<uint16_t>(0x0ABC, width, isSigned, EXPAND).value_or(NO), "\x0A\xBC");
      NICE_CHECK_EQUAL(binaryStrFromInt<uint16_t>(0x0ABC, width, isSigned, TRUNCATE).value_or(NO), "\xBC");
      NICE_CHECK_EQUAL(binaryStrFromInt<uint16_t>(0x0ABC, width, isSigned, NULLOPT).value_or(NO), NO);
    }
  } // end uint16_t

  /*------------------------------------------------------------------------------------------------------------------*/
  { // uint32_t
    const int nBytes = 32 / 8;
    // COMPACT, unsigned
    std::vector<std::pair<uint32_t, std::string>> compactUnsignedTestCases = {
        {0, ZERO_STR},
        {5, "\x05"},
        {0xAB, "\xAB"},
        {0xABC, "\x0A\xBC"},
        {0xABCD, "\xAB\xCD"},
        {std::numeric_limits<uint32_t>::max(), "\xFF\xFF\xFF\xFF"},
    };
    for(const auto& [input, expected] : compactUnsignedTestCases) {
      bool isSigned = false;
      NICE_CHECK_EQUAL(binaryStrFromInt(input, COMPACT, isSigned).value_or(NO), expected);
      NICE_CHECK_EQUAL(binaryStrFromInt(input, COMPACT).value_or(NO), expected);
    }

    // COMPACT, signed
    std::vector<std::pair<uint32_t, std::string>> compactSignedtestCases = {
        {0x7FFFFFFF, "\x7F\xFF\xFF\xFF"}, // max positive int
        {0xABCD, std::string("\0\xAB\xCD", 3)}, {0xABC, "\x0A\xBC"}, {0xAB, std::string("\0\xAB", 2)}, {5, "\x05"},
        {0, ZERO_STR}, {std::numeric_limits<uint32_t>::max(), "\xFF"}, // interpreted as -1, left F's get compacted
        {0xFFFFFFFB, "\xFB"},                                          //-5
        {0xFFFFFF7F, "\xFF\x7F"},                                      //-129, need 2nd byte for sign
    };
    for(const auto& [input, expected] : compactSignedtestCases) {
      bool isSigned = true;
      NICE_CHECK_EQUAL(binaryStrFromInt(input, COMPACT, isSigned).value_or(NO), expected);
    }

    //    TYPE_WIDTH)
    std::vector<std::pair<uint32_t, std::string>> typeWidthTestCases = {
        {0, std::string("\0\0\0\0", nBytes)},
        {5, std::string("\0\0\0\x05", nBytes)},
        {0xAB, std::string("\0\0\0\xAB", nBytes)},
        {0xABC, std::string("\0\0\x0A\xBC", nBytes)},
        {0xABCD, std::string("\0\0\xAB\xCD", nBytes)},
        {std::numeric_limits<uint32_t>::max(), std::string("\xFF\xFF\xFF\xFF", nBytes)},
    };
    for(const auto& [input, expected] : typeWidthTestCases) {
      for(bool isSigned : {true, false}) {
        NICE_CHECK_EQUAL(binaryStrFromInt(input, TYPE_WIDTH, isSigned).value_or(NO), expected);
      }
      NICE_CHECK_EQUAL(binaryStrFromInt(input, TYPE_WIDTH).value_or(NO), expected);
    }

    // natural size is greater than the fixed width parameter
    width = 0;
    std::vector<std::pair<uint16_t, std::string>> fixedWidthTestCases = {
        {0, ZERO_STR},
        {5, "\x05"},
        {0x7B, "\x7B"}, // NOLINT(modernize-raw-string-literal)
        {0xABC, "\x0A\xBC"},
        {0x7BCD, "\x7B\xCD"},
    };
    for(const auto& [input, expected] : fixedWidthTestCases) {
      for(bool isSigned : {true, false}) {
        NICE_CHECK_EQUAL(binaryStrFromInt(input, width, isSigned, EXPAND).value_or(NO), expected);
        NICE_CHECK_EQUAL(binaryStrFromInt(input, width, isSigned, TRUNCATE).value_or(NO), ZERO_STR);
        NICE_CHECK_EQUAL(binaryStrFromInt(input, width, isSigned, NULLOPT).value_or(NO), NO);
      }
    }
    width = 1;
    for(bool isSigned : {true, false}) {
      NICE_CHECK_EQUAL(binaryStrFromInt<uint32_t>(0x7BCD, width, isSigned, EXPAND).value_or(NO), "\x7B\xCD");
      NICE_CHECK_EQUAL(binaryStrFromInt<uint32_t>(0xABCD, width, isSigned, TRUNCATE).value_or(NO), "\xCD");
      NICE_CHECK_EQUAL(binaryStrFromInt<uint32_t>(0xABCD, width, isSigned, NULLOPT).value_or(NO), NO);

      NICE_CHECK_EQUAL(binaryStrFromInt<uint32_t>(0x0ABC, width, isSigned, EXPAND).value_or(NO), "\x0A\xBC");
      NICE_CHECK_EQUAL(binaryStrFromInt<uint32_t>(0x0ABC, width, isSigned, TRUNCATE).value_or(NO), "\xBC");
      NICE_CHECK_EQUAL(binaryStrFromInt<uint32_t>(0x0ABC, width, isSigned, NULLOPT).value_or(NO), NO);

      NICE_CHECK_EQUAL(
          binaryStrFromInt<uint32_t>(0x80000000, 2, isSigned, TRUNCATE).value_or(NO), std::string("\x00\x00", 2));
    }
  } // end uint32_t

  /*------------------------------------------------------------------------------------------------------------------*/
  { // uint64_t
    const int nBytes = 64 / 8;

    // COMPACT, unsigned
    std::vector<std::pair<uint64_t, std::string>> compactUnsignedTestCases = {
        {std::numeric_limits<uint64_t>::max(), "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF"},
        {0x8000000000000000, std::string("\x80\x00\x00\x00\x00\x00\x00\x00", nBytes)},
        {0xABCD, "\xAB\xCD"},
        {0xABC, "\x0A\xBC"},
        {0xAB, "\xAB"},
        {5, "\x05"},
        {0, ZERO_STR},
    };
    for(const auto& [input, expected] : compactUnsignedTestCases) {
      bool isSigned = false;
      NICE_CHECK_EQUAL(binaryStrFromInt(input, COMPACT, isSigned).value_or(NO), expected);
      NICE_CHECK_EQUAL(binaryStrFromInt(input, COMPACT).value_or(NO), expected);
    }

    // COMPACT, signed
    std::vector<std::pair<uint64_t, std::string>> compactSignedtestCases = {
        {0x7FFFFFFFFFFFFFFF, "\x7F\xFF\xFF\xFF\xFF\xFF\xFF\xFF"}, // max positive int when treated as signed
        {0xABCD, std::string("\0\xAB\xCD", 3)}, {0xABC, "\x0A\xBC"}, {0xAB, std::string("\0\xAB", 2)}, {5, "\x05"},
        {0, ZERO_STR}, {std::numeric_limits<uint64_t>::max(), "\xFF"}, // interpreted as -1, left F's get compacted
        {0xFFFFFFFFFFFFFFFB, "\xFB"},                                  //-5
        {0xFFFFFFFFFFFFFF7F, "\xFF\x7F"},                              //-129, need 2nd byte for sign
        {0x8000000000000000, std::string("\x80\x00\x00\x00\x00\x00\x00\x00", nBytes)}, // most negative
    };
    for(const auto& [input, expected] : compactSignedtestCases) {
      bool isSigned = true;
      NICE_CHECK_EQUAL(binaryStrFromInt(input, COMPACT, isSigned).value_or(NO), expected);
    }

    // TYPE_WIDTH
    std::vector<std::pair<uint64_t, std::string>> typeWidthTestCases = {
        {0, std::string("\0\0\0\0\0\0\0\0", nBytes)},
        {5, std::string("\0\0\0\0\0\0\0\x05", nBytes)},
        {0xAB, std::string("\0\0\0\0\0\0\0\xAB", nBytes)},
        {0xABC, std::string("\0\0\0\0\0\0\x0A\xBC", nBytes)},
        {0xABCD, std::string("\0\0\0\0\0\0\xAB\xCD", nBytes)},
        {std::numeric_limits<uint64_t>::max(), std::string("\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF", nBytes)},
    };
    for(const auto& [input, expected] : typeWidthTestCases) {
      for(bool isSigned : {true, false}) {
        NICE_CHECK_EQUAL(binaryStrFromInt(input, TYPE_WIDTH, isSigned).value_or(NO), expected);
      }
      NICE_CHECK_EQUAL(binaryStrFromInt(input, TYPE_WIDTH).value_or(NO), expected);
    }
    // natural size is greater than the fixed width parameter
    std::vector<std::pair<uint64_t, std::string>> fixedWidthTestCases = {
        {0x8000000000000000, std::string("\x80\x00\x00\x00\x00\x00\x00\x00", nBytes)},
        {0x7BCD, "\x7B\xCD"},
        {0xABC, "\x0A\xBC"},
        {0x7B, "\x7B"}, // NOLINT(modernize-raw-string-literal)
        {5, "\x05"},
        {0, ZERO_STR},
    };
    width = 0;
    for(const auto& [input, expected] : fixedWidthTestCases) {
      for(bool isSigned : {true, false}) {
        NICE_CHECK_EQUAL(binaryStrFromInt(input, width, isSigned, EXPAND).value_or(NO), expected);
        NICE_CHECK_EQUAL(binaryStrFromInt(input, width, isSigned, TRUNCATE).value_or(NO), ZERO_STR);
        NICE_CHECK_EQUAL(binaryStrFromInt(input, width, isSigned, NULLOPT).value_or(NO), NO);
      }
    }
    width = 1;
    for(bool isSigned : {true, false}) {
      NICE_CHECK_EQUAL(binaryStrFromInt<uint64_t>(0x7BCD, width, isSigned, EXPAND).value_or(NO), "\x7B\xCD");
      NICE_CHECK_EQUAL(binaryStrFromInt<uint64_t>(0xABCD, width, isSigned, TRUNCATE).value_or(NO), "\xCD");
      NICE_CHECK_EQUAL(binaryStrFromInt<uint64_t>(0xABCD, width, isSigned, NULLOPT).value_or(NO), NO);

      NICE_CHECK_EQUAL(binaryStrFromInt<uint64_t>(0x0ABC, width, isSigned, EXPAND).value_or(NO), "\x0A\xBC");
      NICE_CHECK_EQUAL(binaryStrFromInt<uint64_t>(0x0ABC, width, isSigned, TRUNCATE).value_or(NO), "\xBC");
      NICE_CHECK_EQUAL(binaryStrFromInt<uint64_t>(0x0ABC, width, isSigned, NULLOPT).value_or(NO), NO);

      NICE_CHECK_EQUAL(
          binaryStrFromInt<uint64_t>(0x80000000, 2, isSigned, TRUNCATE).value_or(NO), std::string("\x00\x00", 2));
    }
  } // end uint64_t

  /*------------------------------------------------------------------------------------------------------------------*/
  { // int8_t
    std::vector<std::pair<int8_t, std::string>> testCases = {
        {std::numeric_limits<int8_t>::max(), "\x7F"},
        {5, "\x05"},
        {0, ZERO_STR},
        {-1, "\xFF"},
        {0xAB, "\xAB"},
        {std::numeric_limits<int8_t>::min(), "\x80"},
    };
    for(const auto& [input, expected] : testCases) {
      for(auto widthOpt : widthOptions) {
        for(bool isSigned : {true, false}) {
          NICE_CHECK_EQUAL(binaryStrFromInt(input, widthOpt, isSigned).value_or(NO), expected);
        }
        NICE_CHECK_EQUAL(binaryStrFromInt(input, widthOpt).value_or(NO), expected);
      }
      NICE_CHECK_EQUAL(binaryStrFromInt(input).value_or(NO), expected);
    }
    // fixed width all: TODO
    // natural size is greater than the fixed width parameter, signed
    // TODO

    // OLD
    // Put an 8-bit number into a 10 byte string
    width = 10;
    NICE_CHECK_EQUAL(
        binaryStrFromInt<int8_t>(5, width).value_or(NO), std::string("\x00\x00\x00\x00\x00\x00\x00\x00\x00\x05", 10));
    NICE_CHECK_EQUAL(
        binaryStrFromInt<int8_t>(-2, width).value_or(NO), std::string("\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFE", 10));
  } // end int8_t

  /*------------------------------------------------------------------------------------------------------------------*/
  { // int16_t
    const int nBytes = 16 / 8;
    // COMPACT, unsigned
    std::vector<std::pair<int16_t, std::string>> compactUnsignedTestCases = {
        {std::numeric_limits<int16_t>::max(), "\x7F\xFF"},
        {0x8000, std::string("\x80\x00", nBytes)},
        {0xABCD, "\xAB\xCD"},
        {0xABC, "\x0A\xBC"},
        {0xAB, "\xAB"},
        {5, "\x05"},
        {0, ZERO_STR},
    };
    for(const auto& [input, expected] : compactUnsignedTestCases) {
      bool isSigned = false;
      NICE_CHECK_EQUAL(binaryStrFromInt(input, COMPACT, isSigned).value_or(NO), expected);
    }

    // COMPACT, signed
    std::vector<std::pair<int16_t, std::string>> compactSignedtestCases = {
        {std::numeric_limits<int16_t>::max(), "\x7F\xFF"}, // max positive int
        {0xABCD, std::string("\xAB\xCD", 2)},              // 0th bit is the sign bit, so no prepend.
        {0xABC, "\x0A\xBC"}, {0xAB, std::string("\0\xAB", 2)}, {5, "\x05"}, {0, ZERO_STR},
        {0xFFFF, "\xFF"},     // interpreted as -1, left F's get compacted
        {0xFFFB, "\xFB"},     //-5
        {0xFF7F, "\xFF\x7F"}, //-129, need 2nd byte for sign
        {std::numeric_limits<int16_t>::min(), std::string("\x80\x00", nBytes)}, // most negative
    };
    for(const auto& [input, expected] : compactSignedtestCases) {
      bool isSigned = true;
      NICE_CHECK_EQUAL(binaryStrFromInt(input, COMPACT, isSigned).value_or(NO), expected);
      NICE_CHECK_EQUAL(binaryStrFromInt(input, COMPACT).value_or(NO), expected);
    }

    //    TYPE_WIDTH)
    std::vector<std::pair<int16_t, std::string>> typeWidthTestCases = {
        {0, std::string("\0\0", nBytes)},
        {5, std::string("\0\x05", nBytes)},
        {0xAB, std::string("\0\xAB", nBytes)},
        {0xABC, std::string("\x0A\xBC", nBytes)},
        {0xABCD, std::string("\xAB\xCD", nBytes)},
        {std::numeric_limits<int16_t>::max(), std::string("\x7F\xFF", nBytes)},
    };
    for(const auto& [input, expected] : typeWidthTestCases) {
      for(bool isSigned : {true, false}) {
        NICE_CHECK_EQUAL(binaryStrFromInt(input, TYPE_WIDTH, isSigned).value_or(NO), expected);
      }
      NICE_CHECK_EQUAL(binaryStrFromInt(input, TYPE_WIDTH).value_or(NO), expected);
    }
    // fixed width all: TODO
  } // end int16_t
  /*------------------------------------------------------------------------------------------------------------------*/
  { // int32_t
    const int nBytes = 32 / 8;
    // COMPACT, unsigned
    std::vector<std::pair<int32_t, std::string>> compactUnsignedTestCases = {
        {std::numeric_limits<int32_t>::max(), "\x7F\xFF\xFF\xFF"},
        {0x80000000, std::string("\x80\x00\x00\x00", nBytes)},
        {0xABCD, "\xAB\xCD"},
        {0xABC, "\x0A\xBC"},
        {0xAB, "\xAB"},
        {5, "\x05"},
        {0, ZERO_STR},
    };
    for(const auto& [input, expected] : compactUnsignedTestCases) {
      bool isSigned = false;
      NICE_CHECK_EQUAL(binaryStrFromInt(input, COMPACT, isSigned).value_or(NO), expected);
    }

    // COMPACT, signed
    std::vector<std::pair<int32_t, std::string>> compactSignedtestCases = {
        {std::numeric_limits<int32_t>::max(), "\x7F\xFF\xFF\xFF"}, // max positive int
        {0xABCD, std::string("\0\xAB\xCD", 3)}, {0xABC, "\x0A\xBC"}, {0xAB, std::string("\0\xAB", 2)}, {5, "\x05"},
        {0, ZERO_STR}, {0xFFFFFFFF, "\xFF"}, // interpreted as -1, left F's get compacted
        {0xFFFFFFFB, "\xFB"},                //-5
        {0xFFFFFF7F, "\xFF\x7F"},            //-129, need 2nd byte for sign
        {std::numeric_limits<int32_t>::min(), std::string("\x80\x00\x00\x00", nBytes)}, // most negative
    };
    for(const auto& [input, expected] : compactSignedtestCases) {
      bool isSigned = true;
      NICE_CHECK_EQUAL(binaryStrFromInt(input, COMPACT, isSigned).value_or(NO), expected);
      NICE_CHECK_EQUAL(binaryStrFromInt(input, COMPACT).value_or(NO), expected);
    }

    //    TYPE_WIDTH)
    std::vector<std::pair<int32_t, std::string>> typeWidthTestCases = {
        {0, std::string("\0\0\0\0", nBytes)},
        {5, std::string("\0\0\0\x05", nBytes)},
        {0xAB, std::string("\0\0\0\xAB", nBytes)},
        {0xABC, std::string("\0\0\x0A\xBC", nBytes)},
        {0xABCD, std::string("\0\0\xAB\xCD", nBytes)},
        {std::numeric_limits<int32_t>::max(), std::string("\x7F\xFF\xFF\xFF", nBytes)},
    };
    for(const auto& [input, expected] : typeWidthTestCases) {
      for(bool isSigned : {true, false}) {
        NICE_CHECK_EQUAL(binaryStrFromInt(input, TYPE_WIDTH, isSigned).value_or(NO), expected);
      }
      NICE_CHECK_EQUAL(binaryStrFromInt(input, TYPE_WIDTH).value_or(NO), expected);
    }

    // fixed width all: TODO
    // natural size is greater than the fixed width parameter, signed
    // TODO
  } // end int32_t
  /*------------------------------------------------------------------------------------------------------------------*/
  { // int64_t
    const int nBytes = 64 / 8;
    // COMPACT, unsigned
    std::vector<std::pair<int64_t, std::string>> compactUnsignedTestCases = {
        {std::numeric_limits<int64_t>::max(), "\x7F\xFF\xFF\xFF\xFF\xFF\xFF\xFF"},
        {0x8000000000000000, std::string("\x80\x00\x00\x00\x00\x00\x00\x00", nBytes)},
        {0xABCD, "\xAB\xCD"},
        {0xABC, "\x0A\xBC"},
        {0xAB, "\xAB"},
        {5, "\x05"},
        {0, ZERO_STR},
    };
    for(const auto& [input, expected] : compactUnsignedTestCases) {
      bool isSigned = false;
      NICE_CHECK_EQUAL(binaryStrFromInt(input, COMPACT, isSigned).value_or(NO), expected);
    }

    // COMPACT, signed
    std::vector<std::pair<int64_t, std::string>> compactSignedtestCases = {
        {std::numeric_limits<int64_t>::max(), "\x7F\xFF\xFF\xFF\xFF\xFF\xFF\xFF"}, // max positive int
        {0xABCD, std::string("\0\xAB\xCD", 3)}, {0xABC, "\x0A\xBC"}, {0xAB, std::string("\0\xAB", 2)}, {5, "\x05"},
        {0, ZERO_STR}, {0xFFFFFFFFFFFFFFFF, "\xFF"}, // interpreted as -1, left F's get compacted
        {0xFFFFFFFFFFFFFFFB, "\xFB"},                //-5
        {0xFFFFFFFFFFFFFF7F, "\xFF\x7F"},            //-129, need 2nd byte for sign
        {std::numeric_limits<int64_t>::min(), std::string("\x80\x00\x00\x00\x00\x00\x00\x00", nBytes)}, // most negative
    };
    for(const auto& [input, expected] : compactSignedtestCases) {
      bool isSigned = true;
      NICE_CHECK_EQUAL(binaryStrFromInt(input, COMPACT, isSigned).value_or(NO), expected);
      NICE_CHECK_EQUAL(binaryStrFromInt(input, COMPACT).value_or(NO), expected);
    }

    // TYPE_WIDTH)
    std::vector<std::pair<int64_t, std::string>> typeWidthTestCases = {
        {0, std::string("\0\0\0\0\0\0\0\0", nBytes)},
        {5, std::string("\0\0\0\0\0\0\0\x05", nBytes)},
        {0xAB, std::string("\0\0\0\0\0\0\0\xAB", nBytes)},
        {0xABC, std::string("\0\0\0\0\0\0\x0A\xBC", nBytes)},
        {0xABCD, std::string("\0\0\0\0\0\0\xAB\xCD", nBytes)},
        {std::numeric_limits<int64_t>::max(), std::string("\x7F\xFF\xFF\xFF\xFF\xFF\xFF\xFF", nBytes)},
    };
    for(const auto& [input, expected] : typeWidthTestCases) {
      for(bool isSigned : {true, false}) {
        NICE_CHECK_EQUAL(binaryStrFromInt(input, TYPE_WIDTH, isSigned).value_or(NO), expected);
      }
      NICE_CHECK_EQUAL(binaryStrFromInt(input, TYPE_WIDTH).value_or(NO), expected);
    }
  } // end int64_t
} // end binaryStrFromInt_tests

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(intFromBinaryStr_tests) {
  // Test conversion from binary string
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

BOOST_AUTO_TEST_CASE(hexStrFromInt_test) {
  // This tests the mechanics of toTransportLayerHexInt, which is also used for binary int.

  std::string ans, hexOut;

  // Bool
  ChimeraTK::Boolean bIn;

  bIn = false;
  hexOut = hexStrFromInt<ChimeraTK::Boolean>(bIn).value_or(NO);
  BOOST_CHECK_EQUAL(hexOut, "0");

  bIn = true;
  hexOut = hexStrFromInt<ChimeraTK::Boolean>(bIn).value_or(NO);
  BOOST_CHECK_EQUAL(hexOut, "1");

  bIn = true;
  hexOut = hexStrFromInt<ChimeraTK::Boolean>(bIn, 3, false).value_or(NO);
  BOOST_CHECK_EQUAL(hexOut, "001");

  // no fixed width, odd number of characters
  int32_t iIn;

  iIn = 0x10E;
  ans = std::string("10E", 3);
  hexOut = hexStrFromInt<int32_t>(iIn, WidthOption::COMPACT).value_or(NO);
  BOOST_CHECK_EQUAL(hexOut, ans);

  // no fixed width, odd number of characters, keeping sign bit.
  iIn = 0xD0E;
  ans = std::string("0D0E", 4);
  hexOut = hexStrFromInt<int32_t>(iIn, WidthOption::COMPACT).value_or(NO);
  BOOST_CHECK_EQUAL(hexOut, ans);

  // no fixed width, even number of characters
  iIn = 0xAB0C;
  ans = std::string("AB0C", 4);
  hexOut = hexStrFromInt<uint32_t>(iIn).value_or(NO);
  // If set to binaryStrFromInt<int32_t> rather than uint32, we expect "00AB0C"
  BOOST_CHECK_EQUAL(hexOut, ans);

  // fixed width, unsigned, even number of characters in, even out
  {
    std::vector<std::tuple<int32_t, size_t, bool, std::string>> testCases{
        {0xAB0C, 6, false, "00AB0C"},
        {0xAB0C, 5, false, "0AB0C"},        // fixed width, unsigned, even number of characters in, odd out
        {0xD0E, 5, false, "00D0E"},         // fixed width, unsigned, odd number of characters in, odd number out
        {0xD0E, 6, false, "000D0E"},        // fixed width, unsigned, odd number of characters in, even number out
        {0xAB0C, 6, true, "00AB0C"},        // fixed width, signed, positive, even number of characters
        {0xD0E, 5, true, "00D0E"},          // fixed width, signed, positive, odd number of characters
        {-1 * (0xAB0C), 6, true, "FF54F4"}, // fixed width, signed, negative, even number of characters
        {-1 * (0xD0E), 5, true, "FF2F2"},   // fixed width, signed, negative, odd number of characters
        {0, 3, true, "000"},                // Trivial case
    };
    for(const auto& [iInput, width, isSigned, strAns] : testCases) {
      BOOST_CHECK_EQUAL(hexStrFromInt<int32_t>(iInput, width, isSigned).value_or(NO), strAns);
    }
  }
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(toUserTypeHexInt_test) {
  // This tests the mechanics of toUserTypeHexInt, which is also used for binary int.

  std::string str;

  { // Boolean
    ChimeraTK::Boolean T = true, F = false;
    std::vector<std::pair<std::string, ChimeraTK::Boolean>> testCases = {
        {"", F},
        {"0000", F},
        {"0001", T},
        {"AB0C", T},
    };
    for(const auto& [inputStr, ans] : testCases) {
      ChimeraTK::Boolean test = ChimeraTK::userTypeToUserType<uint16_t, std::string>("0x" + inputStr);
      BOOST_CHECK_EQUAL(test, ans);

      BOOST_CHECK_EQUAL(intFromBinaryStr<ChimeraTK::Boolean>(binaryStrFromHexStr(inputStr)).value_or(-1), ans);
    }

    BOOST_CHECK_EQUAL(
        intFromBinaryStr<bool>(binaryStrFromHexStr("0001")).value_or(-1), true); // TODO make a suite for this
  }

  { // Unsigned int cases
    std::vector<std::pair<std::string, uint16_t>> testCases = {
        {"AB0C", 0xAB0C}, // Case: common value, even number of characters
        {"A0B", 0xA0B},   // Case: common value, odd number of characters
        {"0", 0},         // Case: zero
        {"", 0},          // Case: trivial
    };
    for(const auto& [inputStr, ans] : testCases) {
      auto test = ChimeraTK::userTypeToUserType<uint16_t, std::string>("0x" + inputStr);
      BOOST_CHECK_EQUAL(test, ans);

      BOOST_CHECK_EQUAL(intFromBinaryStr<uint16_t>(binaryStrFromHexStr(inputStr)).value_or(-1), ans);
    }
  }

  /*------------------------------------------------------------------------------------------------------------------*/
  // TODO
  // Why do we even have the above?
  // Why do we bother with the combination of intFromBinaryStr<int16_t>(binaryStrFromHexStr when we have the
  // userTypeToUserType for hex? Why not break this off into a different test

  // Signed int cases:
  { // int16
    std::vector<std::tuple<std::string, int16_t, bool>> testCases = {
        // inputStr, answer, isSigned
        {"0", 0, false},                                   // Trivial case, signed int
        {"2B0C", 0x2B0C, false},                           // positive even
        {"20B", 0x20B, false},                             // positive odd
        {"AB0C", -1 * static_cast<int16_t>(0x54F4), true}, // negative even
        {"A0B", -1 * static_cast<int16_t>(0x5F5), true},   // negative odd
    };
    for(const auto& [inputStr, ans, isSigned] : testCases) {
      BOOST_CHECK_EQUAL(intFromBinaryStr<int16_t>(binaryStrFromHexStr(inputStr, isSigned)).value_or(-1), ans);
    }
    BOOST_CHECK_EQUAL(intFromBinaryStr<int16_t>("").value_or(-1), 0);
  }

} // end toUserTypeHexInt_test

BOOST_AUTO_TEST_CASE(floatBinary_test) {
  // For each case, convert the number to binary, and back to float. Then convert to hex and back to float by way of
  // binary. Check that each round trip works.
  std::string b, b2, h;

  std::vector<float> floatTestCases = {0.F, 0.25, -0.25, FLT_MAX, FLT_TRUE_MIN, FLT_EPSILON};
  for(float f : floatTestCases) {
    b = binaryStrFromFloat(f);
    float f2 = floatFromBinaryStr<float>(b).value_or(-1.F);
    h = hexStrFromFloat(f);
    b2 = binaryStrFromHexStr(h);
    float f3 = floatFromBinaryStr<float>(b2).value_or(-1.F);
    BOOST_CHECK_EQUAL(f, f2);
    BOOST_CHECK_EQUAL(f, f3);
  }

  /*------------------------------------------------------------------------------------------------------------------*/
  std::vector<double> doubleTestCases = {0.0, 3.14e9, -3.14e9, DBL_MAX, DBL_TRUE_MIN, DBL_EPSILON};
  for(double d : doubleTestCases) {
    b = binaryStrFromFloat(d);
    double d2 = floatFromBinaryStr<double>(b).value_or(-1.0);
    h = hexStrFromFloat(d);
    b2 = binaryStrFromHexStr(h);
    double d3 = floatFromBinaryStr<double>(b2).value_or(-1.0);
    BOOST_CHECK_EQUAL(d, d2);
    BOOST_CHECK_EQUAL(d, d3);
  }
}

/**********************************************************************************************************************/

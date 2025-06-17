// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE StringUtils

#include <boost/test/unit_test.hpp>
using namespace boost::unit_test_framework;

#include "jsonUtils.h"

#include <ChimeraTK/Exception.h>

#include <nlohmann/json.hpp>

#include <optional>
#include <unordered_map>

/**********************************************************************************************************************/
using json = nlohmann::json;

namespace ChimeraTK {
  BOOST_AUTO_TEST_CASE(caseInsensitiveGetValueOption_test) {
    json j = {{"read", "ACC?"}, {"write", 3}};

    auto noResult = caseInsensitiveGetValueOption(j, "readwrite");
    BOOST_TEST(!noResult.has_value());

    auto result = caseInsensitiveGetValueOption(j, "wRiTe");
    BOOST_REQUIRE(result.has_value());
    BOOST_CHECK_EQUAL(result.value(), 3);

    result = caseInsensitiveGetValueOption(j, "rEaD");
    BOOST_REQUIRE(result.has_value());
    BOOST_CHECK_EQUAL(result.value(), "ACC?");
  }

  /********************************************************************************************************************/

  BOOST_AUTO_TEST_CASE(caseInsensitiveGetValueOr_test) {
    json j = {{"read", "ACC?"}, {"write", 3}};

    BOOST_CHECK_EQUAL(caseInsensitiveGetValueOr(j, "write", -1), 3);
    BOOST_CHECK_EQUAL(caseInsensitiveGetValueOr(j, "wRiTe", -1), 3);
    BOOST_CHECK_EQUAL(caseInsensitiveGetValueOr(j, "rEaD", "fail"), "ACC?");
    BOOST_CHECK_EQUAL(caseInsensitiveGetValueOr(j, "readwrite", "fail"), "fail");
  }

  /********************************************************************************************************************/

  enum class TestKeys { Read, Write };

  BOOST_AUTO_TEST_CASE(throwIfHasInvalidJsonKeyCaseInsensitive_test) {
    json j = {{"read", "ACC?"}, {"write", 3}};

    std::unordered_map<TestKeys, std::string> validKeyMap = {{TestKeys::Read, "ReAd"}, {TestKeys::Write, "wRiTe"}};

    BOOST_CHECK_NO_THROW(throwIfHasInvalidJsonKeyCaseInsensitive(j, validKeyMap, "Invalid key found"));

    json jInvalid = {{"read", "ACC?"}, {"write", 3}, {"extra", 42}};

    BOOST_CHECK_EXCEPTION(
        throwIfHasInvalidJsonKeyCaseInsensitive(jInvalid, validKeyMap, "Invalid key found"), ChimeraTK::logic_error,
        [](const ChimeraTK::logic_error&) { return true; } // No message check, just confirms the type
    );
  }

  /********************************************************************************************************************/
} // end namespace ChimeraTK

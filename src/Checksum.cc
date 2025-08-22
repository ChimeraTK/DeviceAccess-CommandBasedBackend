// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "Checksum.h"

#include "stringUtils.h"

#include <ChimeraTK/Exception.h> //for ChimeraTK::logic_error

#include <openssl/evp.h> // OpenSSL 3.0 EVP API
#include <openssl/sha.h> // For SHA256_DIGEST_LENGTH

#include <boost/crc.hpp>

#include <memory> // For std::unique_ptr
#include <regex>

#define FUNC_NAME (std::string(__func__) + ": ")

// First put the hex through
// std::string binData = binaryStrFromHexStr(hexData);

static const checksumFunction checksum8 = [](const std::string binData) -> std::string {
  uint8_t sum = 0;
  for(unsigned char c : binData) {
    sum += c;
  }
  return hexStrFromInt<uint8_t>(sum, WidthOption::TYPE_WIDTH).value();
};

/**********************************************************************************************************************/

static const checksumFunction checksum32 = [](const std::string binData) -> std::string {
  uint32_t sum = 0;
  for(unsigned char c : binData) {
    sum += c;
  }
  return hexStrFromInt<uint32_t>(sum, WidthOption::TYPE_WIDTH).value();
};

/**********************************************************************************************************************/

static const checksumFunction checksumCrcCcit16 = [](const std::string binData) -> std::string {
  boost::crc_optimal<16, 0x1021, 0xFFFF, 0x0000, false, false> crc;
  /*
   * see https://www.scadacore.com/tools/programming-calculators/online-checksum-calculator/
   * 16: CRC width is 16 bits.
   * 0x1021: Polynomial.
   * 0xFFFF: Initial value.
   * 0x0000: Final XOR value.
   * false: ReflectIn - input data bits are not reflected.
   * false: ReflectOut - output CRC bits are not reflected.
   */
  crc.process_bytes(binData.data(), binData.size());
  uint16_t result = crc.checksum();
  return hexStrFromInt<uint16_t>(result, WidthOption::TYPE_WIDTH).value();
};

/**********************************************************************************************************************/

static const checksumFunction checksumSha256 = [](const std::string binData) -> std::string {
  std::string binResult(SHA256_DIGEST_LENGTH, '\0');
  std::unique_ptr<EVP_MD_CTX, decltype(&EVP_MD_CTX_free)> ctx(EVP_MD_CTX_new(), &EVP_MD_CTX_free);
  if(not ctx) {
    throw ChimeraTK::runtime_error("Failed to create EVP_MD_CTX");
  }

  if(EVP_DigestInit_ex(ctx.get(), EVP_sha256(), nullptr) != 1 ||
      EVP_DigestUpdate(ctx.get(), binData.data(), binData.size()) != 1 ||
      EVP_DigestFinal_ex(ctx.get(), reinterpret_cast<unsigned char*>(&binResult[0]), nullptr) != 1) {
    throw ChimeraTK::runtime_error("SHA256 computation failed");
  }

  return hexStrFromBinaryStr(binResult);
};

/**********************************************************************************************************************/
/**********************************************************************************************************************/

std::string getRegexString(checksum cs) {
  static const std::map<checksum, std::string> checksumToRegexStrMap = {
      // clang-format off
    // String values must be parentheses bound capture groups.
    {checksum::CS8,        "([0-9A-Fa-f]{1})"},
    {checksum::CS32,       "([0-9A-Fa-f]{4})"},
    {checksum::SHA256,     "([0-9A-Fa-f]{32})"},
    {checksum::CRC_CCIT16, "([0-9A-Fa-f]{2})"} // clang-format on
  };
  try {
    return checksumToRegexStrMap.at(cs);
  }
  catch(const std::out_of_range&) {
    throw ChimeraTK::logic_error(FUNC_NAME + "Encountered unmapped checksum " + toStr(cs));
  }
}
/**********************************************************************************************************************/

checksumFunction getChecksumer(checksum cs) {
  static const std::map<checksum, checksumFunction> checksumToFuncMap = {
      // clang-format off
    {checksum::CS8, checksum8},
    {checksum::CS32, checksum32},
    {checksum::SHA256, checksumSha256},
    {checksum::CRC_CCIT16, checksumCrcCcit16}
      // clang-format on
  };
  try {
    return checksumToFuncMap.at(cs);
  }
  catch(const std::out_of_range&) {
    throw ChimeraTK::logic_error(FUNC_NAME + "Encountered unmapped checksum " + toStr(cs));
  }
}

/**********************************************************************************************************************/

struct ChecksumTripple {
  /*
   * ChecksumTripple contains the locations of a tripple of checksum tags.
   * Locations are string indicies, with -1 (or any negative int) indicates that the corresponding tag is not found yet.
   * The first/last '}' location scheme of csStart and csEnd facilitate extracting the checksum block substring.
   * ChecksumTripple's are used in std::map<int, ChecksumTripple> which relates the checksum index to the
   * ChecksumTripple. The checksum index is the 'i' in {{csStart.i}}.
   */
  static constexpr int NOT_FOUND = -1;
  int csStart = NOT_FOUND; // The string index of the last '}' of the CHECKSUM_START tag (example: {{csStart.i}} )
  int csEnd = NOT_FOUND;   // The string index of the first '{' of the CHECKSUM_END tag (example: {{csEnd.i}} )
  int csPoint = NOT_FOUND; // The string index of the first '{' of CHECKSUM_POINT tag (example: {{cs.i}} )
};

/**********************************************************************************************************************/
/*
 * Throws ChimeraTK::logic_error if there are missing indicies
 * which is to say, if the map doesn't map all all int keys from 0 to map.size()-1
 */
void throwIfChecksumMapHasGaps(const std::map<int, ChecksumTripple>& map, const std::string& errorMessageDetail) {
  int iExpected = -1;
  for(auto const& [iKey, _] : map) {
    if(iKey != (++iExpected)) {
      throw ChimeraTK::logic_error(FUNC_NAME + "Checksum indices have gaps, missing checksum " +
          std::to_string(iExpected) + " - " + errorMessageDetail);
    }
  }
}

/**********************************************************************************************************************/
/*
 * Throws ChimeraTK::logic_error if one of the three types of tags was not found for some index.
 * For instance, the pattern "{{csStart.0}}asdf{{cs.0}}" is missing the CHECKSUM_END tag {{csEnd.0}},
 * so this input would make this function throw.
 */
void throwIfChecksumMapHasIncompleteEntries(
    const std::map<int, ChecksumTripple>& map, const std::string& errorMessageDetail) {
  auto makeErrorMessage = [&](int iKey, injaTemplatePatternKeys tag) -> std::string {
    return FUNC_NAME + "Checksum " + std::to_string(iKey) + " is miss tag " + toStr(tag) + " - " + errorMessageDetail;
  };

  for(auto const& [iKey, checksumTripple] : map) {
    if(checksumTripple.csStart == ChecksumTripple::NOT_FOUND) {
      throw ChimeraTK::logic_error(makeErrorMessage(iKey, injaTemplatePatternKeys::CHECKSUM_START));
    }
    if(checksumTripple.csEnd == ChecksumTripple::NOT_FOUND) {
      throw ChimeraTK::logic_error(makeErrorMessage(iKey, injaTemplatePatternKeys::CHECKSUM_END));
    }
    if(checksumTripple.csPoint == ChecksumTripple::NOT_FOUND) {
      throw ChimeraTK::logic_error(makeErrorMessage(iKey, injaTemplatePatternKeys::CHECKSUM_POINT));
    }
  }
}

/**********************************************************************************************************************/
/*
 * Throws ChimeraTK::logic_error if there are inja checksums end tags before their corresponding start tag.
 * Situations of the form: CHECKSUM_END i position < CHECKSUM_START i position.
 */
void throwIfChecksumMapHasEndsBeforeStarts(
    const std::map<int, ChecksumTripple>& map, const std::string& errorMessageDetail) {
  for(auto const& [i, iTripple] : map) {
    if(iTripple.csStart >= iTripple.csEnd) {
      throw ChimeraTK::logic_error(FUNC_NAME + toStr(injaTemplatePatternKeys::CHECKSUM_END) + " tag comes before " +
          toStr(injaTemplatePatternKeys::CHECKSUM_START) + " tag for checksum " + std::to_string(i) + " - " +
          errorMessageDetail);
    }
  }
}

/**********************************************************************************************************************/
/*
 * Throws ChimeraTK::logic_error if there are inja checksums insertion point tags inside their own checksum block
 * which would make the checksum take its own output as input. Situation of the form:
 * CHECKSUM_START i position < CHECKSUM_POINT i position < CHECKSUM_END i position.
 */
void throwIfChecksumMapHasInfiniteLoops(
    const std::map<int, ChecksumTripple>& map, const std::string& errorMessageDetail) {
  for(auto const& [i, iTripple] : map) {
    if((iTripple.csStart <= iTripple.csPoint) and (iTripple.csPoint <= iTripple.csEnd)) {
      throw ChimeraTK::logic_error(FUNC_NAME + toStr(injaTemplatePatternKeys::CHECKSUM_POINT) +
          " tag is illegally between the " + toStr(injaTemplatePatternKeys::CHECKSUM_START) + " and " +
          toStr(injaTemplatePatternKeys::CHECKSUM_END) + " tags for checksum " + std::to_string(i) + " - " +
          errorMessageDetail);
    }
  }
}

/**********************************************************************************************************************/
/*
 * Throws ChimeraTK::logic_error if the checksums inja tags are badly ordered such that the checksums overlap or
 * contain each other. Situations of the form:
 * * CHECKSUM_START i position < Any j checksum tag position < CHECKSUM_END i position.
 */
void throwIfChecksumMapHasNestingOrOverlaps(
    const std::map<int, ChecksumTripple>& map, const std::string& errorMessageDetail) {
  auto makeNestedChecksumErrorMessage = [&](injaTemplatePatternKeys tag, int i, int j) -> std::string {
    return FUNC_NAME + toStr(tag) + " of checksum " + std::to_string(j) + " is nested within " +
        toStr(injaTemplatePatternKeys::CHECKSUM_START) + "-" + toStr(injaTemplatePatternKeys::CHECKSUM_END) +
        " of checksum " + std::to_string(i) + " - " + errorMessageDetail;
  };

  for(auto iIt = map.begin(); iIt != map.end(); ++iIt) {
    int const& i = iIt->first;
    ChecksumTripple const& iTripple = iIt->second;

    for(auto jIt = std::next(iIt); jIt != map.end(); ++jIt) {
      int const& j = jIt->first;
      ChecksumTripple const& jTripple = jIt->second;

      // Guard against nested csPoint
      if((iTripple.csStart <= jTripple.csPoint) and (jTripple.csPoint <= iTripple.csEnd)) {
        throw ChimeraTK::logic_error(makeNestedChecksumErrorMessage(injaTemplatePatternKeys::CHECKSUM_POINT, i, j));
      }

      // Guard against nested csStart
      if((iTripple.csStart <= jTripple.csStart) and (jTripple.csStart <= iTripple.csEnd)) {
        throw ChimeraTK::logic_error(makeNestedChecksumErrorMessage(injaTemplatePatternKeys::CHECKSUM_START, i, j));
      }

      // Guard against nested csEnd
      if((iTripple.csStart <= jTripple.csEnd) and (jTripple.csEnd <= iTripple.csEnd)) {
        throw ChimeraTK::logic_error(makeNestedChecksumErrorMessage(injaTemplatePatternKeys::CHECKSUM_END, i, j));
      }
    } // end for
  } // end for
} // end throwIfChecksumMapHasNestingOrOverlaps

/**********************************************************************************************************************/
/*
 * Performs all validation tasks on the map
 */
void validateChecksumMap(const std::map<int, ChecksumTripple>& map, const std::string& errorMessageDetail) {
  if(map.size() == 0) {
    return;
  }
  throwIfChecksumMapHasGaps(map, errorMessageDetail);
  throwIfChecksumMapHasIncompleteEntries(map, errorMessageDetail);
  throwIfChecksumMapHasEndsBeforeStarts(map, errorMessageDetail);
  throwIfChecksumMapHasInfiniteLoops(map, errorMessageDetail);
  throwIfChecksumMapHasNestingOrOverlaps(map, errorMessageDetail);
}

/**********************************************************************************************************************/
/*
 * parsePattern constructs, verifies, and returns a map from the checksum index to the positions of the checksum tags.
 * The checksum index is the 'i' in {{csStart.i}}. Patterns without checksum tags yield an empty map.
 * Example:
 * String index hints: 0         1         2         3        4         5         6         7
 *                     012345678901234567890123456789012346789012345678901234567890123456789012
 * Input pattern:     "{{cs.1}}{{csStart.0}}asdf{{csEnd.0}}{{cs.0}}{{csStart.1}}asdf{{csEnd.1}}"
 * Posiitons noted:    ^                   ^    ^          ^                   ^    ^
 * Expected output: [0:{20,25,37}, 1: {0, 57, 62}]
 */
std::map<int, ChecksumTripple> parsePattern(const std::string& pattern) {
  std::map<int, ChecksumTripple> resultMap;

  // Lambda to construct regex from enum
  auto makeRegex = [](injaTemplatePatternKeys key) -> std::regex {
    return std::regex("\\{\\{" + toStr(key) + "\\.(\\d+)\\}\\}");
  };

  static const std::regex csStartRegex = makeRegex(injaTemplatePatternKeys::CHECKSUM_START);
  static const std::regex csEndRegex = makeRegex(injaTemplatePatternKeys::CHECKSUM_END);
  static const std::regex csPointRegex = makeRegex(injaTemplatePatternKeys::CHECKSUM_POINT);

  enum positionToUse : bool { useFirstPosition = false, useLastPosition = true }; // Enum for position selection

  // Lambda to parse a single tag type/ChecksumTripple::field, putting the tag locations into resultMap[i].field.
  auto parseTag = [&](const std::regex& tagRegex, positionToUse whichPos, int ChecksumTripple::* field) -> void {
    auto begin = std::sregex_iterator(pattern.begin(), pattern.end(), tagRegex);
    auto end = std::sregex_iterator();
    for(auto it = begin; it != end; ++it) {
      std::smatch match = *it;
      int checksumIndex = std::stoi(match[1].str());
      if(whichPos == useFirstPosition) {
        resultMap[checksumIndex].*field = static_cast<int>(match.position()); // posiiton of the first '{'
      }
      else {
        resultMap[checksumIndex].*field =
            static_cast<int>(match.position() + match.length() - 1); // posiiton of the last '}'
      }
    }
  }; // end lambda

  parseTag(csStartRegex, useLastPosition, &ChecksumTripple::csStart);
  parseTag(csEndRegex, useFirstPosition, &ChecksumTripple::csEnd);
  parseTag(csPointRegex, useFirstPosition, &ChecksumTripple::csPoint);

  return resultMap;
} // end parsePattern

/**********************************************************************************************************************/
void validateChecksumPattern(const std::string& pattern, const std::string& errorMessageDetail) {
  validateChecksumMap(parsePattern(pattern), errorMessageDetail);
}

/**********************************************************************************************************************/
size_t getNChecksums(const std::string& pattern, const std::string& errorMessageDetail) {
  auto map = parsePattern(pattern);
  validateChecksumMap(map, errorMessageDetail);
  return map.size();
}

/**********************************************************************************************************************/
std::vector<std::string_view> getChecksumBlockSnippets(
    const std::string& pattern, const std::string& errorMessageDetail) {
  auto svPattern = std::string_view(pattern);

  std::map<int, ChecksumTripple> map = parsePattern(pattern);
  validateChecksumMap(map, errorMessageDetail);

  std::vector<std::string_view> snippets;
  for(auto const& [_, iTripple] : map) { // fill snippets with substrings of the pattern
    snippets.push_back(svPattern.substr(iTripple.csStart + 1, iTripple.csEnd - iTripple.csStart - 1));
  }
  return snippets;
}

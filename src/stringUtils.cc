// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#include "stringUtils.h"

#include <type_traits> //used for static asserts of binaryStrFromInt and intFromBinaryStr

#include <algorithm>
#include <cctype>
#include <cstring>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

std::vector<std::string> splitString(const std::string& stringToBeParsed, const std::string& delimiter) noexcept {
  std::vector<std::string> subStrings;
  size_t pos = 0;

  while(true) {
    size_t nextPos = stringToBeParsed.find(delimiter, pos);
    std::string subString = stringToBeParsed.substr(pos, nextPos - pos);
    subStrings.push_back(subString);

    if(nextPos == std::string::npos) {
      break;
    }

    pos = nextPos + delimiter.length();
  }

  return subStrings;
} // end splitString

/**********************************************************************************************************************/

std::vector<std::string> tokenise(const std::string& stringToBeParsed) noexcept {
  std::vector<std::string> output;
  std::basic_stringstream inputStream(stringToBeParsed);

  std::string token;
  while(!inputStream.eof()) {
    token = "";
    inputStream >> token;
    if(!token.empty()) {
      output.push_back(token);
    }
  }

  return output;
}

/**********************************************************************************************************************/

bool strEndsInDelim(const std::string& str, const std::string& delimiter, const size_t delimiterSize) noexcept {
  int s = static_cast<int>(str.size()) - 1;
  int d = static_cast<int>(delimiterSize) - 1;
  while(s >= 0 and d >= 0) {
    if(str[s--] != delimiter[d--]) {
      return false;
    }
  }
  return d < 0;
}

/**********************************************************************************************************************/

std::string stripDelim(const std::string& str, const std::string& delimiter, const size_t delimiterSize) noexcept {
  if(strEndsInDelim(str, delimiter, delimiterSize)) {
    return str.substr(0, str.size() - delimiterSize);
  }
  return str;
}

/**********************************************************************************************************************/

std::string replaceNewLines(const std::string& input) noexcept {
  std::string result = input;

  // Replace '\n' with 'N'
  size_t pos = result.find('\n');
  while(pos != std::string::npos) {
    result.replace(pos, 1, "\\N");
    pos = result.find('\n', pos + 1);
  }

  // Replace '\r' with 'R'
  pos = result.find('\r');
  while(pos != std::string::npos) {
    result.replace(pos, 1, "\\R");
    pos = result.find('\r', pos + 1);
  }

  return result;
} // end replaceNewlines

/**********************************************************************************************************************/

void toLowerCase(std::string& str) noexcept {
  std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c) { return std::tolower(c); });
}

/**********************************************************************************************************************/

std::string binaryStrFromHexStr(const std::string& hexData) noexcept {
  size_t hexDataLength = hexData.length();

  std::string binaryData;
  binaryData.reserve((hexDataLength + 1) / 2);

  size_t i = 0;
  if(hexDataLength % 2 == 1) {
    std::string oddFirstChar = hexData.substr(i, 1);
    char oddFirstByteChar = static_cast<char>(std::stoi(oddFirstChar, nullptr, 16));
    binaryData.push_back(oddFirstByteChar);
    i = 1;
  }
  for(; i < hexDataLength; i += 2) {
    std::string byteHexStr = hexData.substr(i, 2);
    char byteChar = static_cast<char>(std::stoi(byteHexStr, nullptr, 16));
    binaryData.push_back(byteChar);
  }
  return binaryData;
}

/**********************************************************************************************************************/

std::string hexStrFromBinaryStr(const std::string& binaryData) noexcept {
  constexpr std::array<char, 16> hexLUT = {
      '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

  std::string hexString(binaryData.size() * 2, '\0');
  auto it = hexString.begin();
  for(unsigned char byte : binaryData) {
    *it++ = hexLUT[(byte >> 4) & 0xF]; // append the high nibble
    *it++ = hexLUT[byte & 0xF];        // append the low nibble
  }
  return hexString;
}

/**********************************************************************************************************************/

bool caseInsensitiveStrCompare(const std::string& a, const std::string& b) noexcept {
  return std::equal(
      a.begin(), a.end(), b.begin(), b.end(), [](char A, char B) { return std::tolower(A) == std::tolower(B); });
}

/**********************************************************************************************************************/

template<typename intType>
static size_t getIntNaturalWidth(const intType payload) noexcept {
  size_t naturalWidth = 1; // how many chars long should payload be with no leading 0's.
  if(payload >= 0) {
    intType temp = payload >> 8;
    while(temp > 0) {
      temp >>= 8;
      naturalWidth++;
    }
  }
  else { // negative payload
    intType temp = payload >> 8;
    while(temp != -1) {
      temp >>= 8; // bit shift left-packs with 0xFF for negatives.
      naturalWidth++;
    }
  }
  return naturalWidth;
}

template<typename intType>
std::optional<std::string> binaryStrFromInt(
    const intType payload, const std::optional<size_t>& fixedWidth, const OverflowBehavior overflowBehavior) noexcept {
  static_assert(std::is_integral<intType>::value, "Type must be integral");

  char leftPackChar = ((payload >= 0) ? '\0' : '\xFF');
  size_t naturalWidth =
      getIntNaturalWidth<intType>(payload); // how many chars long should payload be with no leading 0's.
  size_t strWidth = naturalWidth;

  if(fixedWidth.has_value()) {
    strWidth = fixedWidth.value();

    if(strWidth < naturalWidth) { // overflow situation
      if(overflowBehavior == OverflowBehavior::EXPAND) {
        strWidth = naturalWidth;
      }
      else if(overflowBehavior == OverflowBehavior::NULLOPT) {
        return std::nullopt;
      } // else overflowBehavior = TRUNCATE
    }
  }

  std::string result(strWidth, leftPackChar);

  const size_t nLeftPackBytes = std::max(static_cast<size_t>(0), strWidth - sizeof(intType));
  const size_t bytesToTransfer = strWidth - nLeftPackBytes;
  for(size_t i = 0; i < bytesToTransfer; i++) {
    // Transfer digits from least significant to most, populating the string from back to front
    result[strWidth - 1 - i] = static_cast<char>((payload >> (8 * i)) & 0xFF);
  }
  return result;
}
/**********************************************************************************************************************/

static size_t getStrNaturalWidth(const std::string binaryContainer, const bool interpretAsPositive = true) noexcept {
  const char leftpackChar = (interpretAsPositive ? '\0' : '\xFF');
  size_t naturalWidth = binaryContainer.find_first_not_of(leftpackChar);
  return ((naturalWidth == std::string::npos) ? 1 : naturalWidth);
}

template<typename intType>
std::optional<intType> intFromBinaryStr(const std::string& binaryContainer, const bool truncateIfOverflow) noexcept {
  static_assert(std::is_integral<intType>::value, "Type must be integral");

  if(binaryContainer.empty()) {
    return 0;
  }

  bool isNegative = (std::is_signed<intType>::value) and (static_cast<unsigned char>((binaryContainer[0]) & 0x80) != 0);
  size_t naturalWidth = getStrNaturalWidth(binaryContainer, not isNegative);
  const size_t maxBytes = sizeof(intType);

  // Validate input length
  if((not truncateIfOverflow) and (naturalWidth > maxBytes)) {
    return std::nullopt;
  }

  intType result = 0;

  // Calculate how many leading F's we need to add if the string is shorter than expected
  const size_t nLeftPackBytes = std::max(static_cast<size_t>(0), maxBytes - binaryContainer.size());
  if(isNegative) {
    for(int i = 0; i < nLeftPackBytes; i++) {
      size_t shift = 8 * (maxBytes - 1 - i);
      result |= static_cast<intType>(0xFF) << shift;
    }
  }

  // Process each byte in the input string
  const size_t bytesToTransfer = maxBytes - nLeftPackBytes;
  const size_t truncationBytes = binaryContainer.size() - bytesToTransfer;
  for(size_t i = 0; i < bytesToTransfer; ++i) {
    size_t shift = 8 * (bytesToTransfer - 1 - i);
    result |= static_cast<intType>(static_cast<unsigned char>(binaryContainer[i + truncationBytes])) << shift;
  }

  return result;
}

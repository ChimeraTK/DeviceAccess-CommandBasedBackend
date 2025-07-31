// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#include "stringUtils.h"

#include <algorithm>
#include <cctype>
#include <cstddef> //Added by ninja fix-linter, maybe for size_t. Likely not needed.
#include <cstring>
#include <functional>
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

std::string getLower(const std::string& str) noexcept {
  std::string copy = str;
  toLowerCase(copy);
  return copy;
}

/**********************************************************************************************************************/

std::string binaryStrFromHexStr(const std::string& hexStr, const bool padLeft, const bool isSigned) noexcept {
  /* Use case: writing to device. We fill in the hexidecimal of interest with the regex, then convert to binary for sending.
  * If h is odd, the first byte or last will be special, requiring padding
  * This padding could go on the left or the right (padLeft = false).

  * Problem: if you feed in ABC with padLeft=true, this gets interpreted to 0ABC, loosing the signed bit.
  * So we need isSigned to tell us whether to move the signed bit.
  */

  const std::function<unsigned char(char)> binCharFromHexChar = [](char hex) noexcept -> char {
    constexpr char offsetA = 'A' - 10;
    constexpr char offseta = 'a' - 10;
    return static_cast<char>(hex - (hex <= '9' ? '0' : hex <= 'F' ? offsetA : offseta));
    /* More verbosely, this does:
      if((hex >='0') and (hex <='9')){
        return hex - '0';
    } else if((hex >='A') and (hex <='F')) {
        return hex + 10 - 'A';
    } else if((hex >='a') and (hex <='f')) {
        return hex + 10 - 'a';
    } */
  };

  std::string binOut((hexStr.length() + 1) / 2, '\x00');
  const size_t hexLengthIsOdd = hexStr.length() % 2;

  if(hexLengthIsOdd) {
    if(padLeft) {
      binOut[0] = static_cast<char>(binCharFromHexChar(hexStr[0]));
      if(isSigned and (binOut[0] >= 0x08)) { // signed and negative
        binOut[0] += '\xF0';                 // Left-pack with F.
      }
    }
    else { // pad right
      binOut.back() = static_cast<char>(binCharFromHexChar(hexStr[hexStr.length() - 1]) << 4U);
    }
  }

  const auto bStart = static_cast<size_t>(hexLengthIsOdd and padLeft); // = (hexLengthIsOdd and padLeft) ? 1 : 0;
  const size_t bEnd = binOut.length() -
      static_cast<size_t>(hexLengthIsOdd and
          not padLeft); // = (hexLengthIsOdd and not padLeft) ? (binOut.length() - 1) : binOut.length();

  for(size_t b = bStart; b < bEnd; ++b) {
    const unsigned char hiNibble = (binCharFromHexChar(hexStr[(2 * b) - bStart]) << 4U);
    const unsigned char loNibble = binCharFromHexChar(hexStr[(2 * b) + 1 - bStart]);
    binOut[b] = static_cast<char>(hiNibble | loNibble);
  }
  return binOut;
}

/**********************************************************************************************************************/

/*
 * Returns the pair of hexidecimal chars for {high nibble, low nibble"
 */
inline std::pair<char, char> getHexDigitsFromByte(unsigned char byte) {
  char upper = "0123456789ABCDEF"[(static_cast<unsigned>(byte) >> 4U) & 0xFU];
  char lower = "0123456789ABCDEF"[static_cast<unsigned>(byte) & 0xFU]; // char array isn't store 2x
  return {upper, lower};
}

/**********************************************************************************************************************/

std::string hexStrFromBinaryStr(const std::string& byteStr) noexcept {
  // Requires the byteStr.length() to be accurate, despite the expected presence of null characters.
  // So something needs to ensure it is the correct length, such as with a resize() command.

  std::string hexOut;
  hexOut.resize(2 * byteStr.length());
  auto hexIt = hexOut.begin();
  for(unsigned char byte : byteStr) {
    auto [highNibble, lowNibble] = getHexDigitsFromByte(byte);
    *hexIt++ = highNibble;
    *hexIt++ = lowNibble;
  }
  return hexOut;
}

/**********************************************************************************************************************/

std::string hexStrFromBinaryStr(const std::string& byteStr, size_t nHexChars, bool isSigned) noexcept {
  // Requires the byteStr.length() to be accurate, despite the expected presence of null characters.
  // So something needs to ensure it is the correct length, such as with a resize() command.

  int byteStrLength = static_cast<int>(byteStr.length());
  std::string hexOut(nHexChars, '0'); // Fill hexOut with 00000

  if(byteStrLength == 0) {
    return hexOut;
  }

  int64_t hexStrIndexLeft = static_cast<int64_t>(nHexChars) -
      2L; // The hexOut index of the left of the two hex digits resulting from the byte at byteStrIndex.
  for(int byteStrIndex = byteStrLength - 1; byteStrIndex >= 0; byteStrIndex--) {
    auto byte = static_cast<unsigned char>(byteStr[byteStrIndex]);
    hexStrIndexLeft = static_cast<int64_t>(nHexChars) - (2L * (byteStrLength - byteStrIndex));

    auto [highNibble, lowNibble] = getHexDigitsFromByte(byte);
    if(hexStrIndexLeft >= -1L) { // If the right hex digit doesn't hit index 0
      hexOut[hexStrIndexLeft + 1L] = lowNibble;
    }
    else {
      break; // nHexChars is shorter than 2*byteStringSize and nHexChars is odd
    }
    if(hexStrIndexLeft >= 0L) {
      hexOut[hexStrIndexLeft] = highNibble;
    }
    else {
      break; // nHexChars is shorter than 2*byteStringSize and nHexChars is even
    }
  }

  // nHexChars is longer than byteStr, left pack with F if byteStr
  if(isSigned and hexStrIndexLeft > 0L) {
    bool bit0 = static_cast<bool>(
        (byteStr[0] >> 15U) & 0x01U); // NOLINT(hicpp-signed-bitwise) Behavior relies on signed integer bit shift
    if(bit0) {
      for(int64_t h = 0L; h < hexStrIndexLeft; h++) {
        hexOut[h] = 'F';
      }
    }
  }
  return hexOut;
}

/**********************************************************************************************************************/

bool caseInsensitiveStrCompare(const std::string& a, const std::string& b) noexcept {
  return std::equal(
      a.begin(), a.end(), b.begin(), b.end(), [](char A, char B) { return std::tolower(A) == std::tolower(B); });
}

/**********************************************************************************************************************/

std::string replaceAll(const std::string& s, const char charToBeReplaced, const std::string& replacement) noexcept {
  size_t nOccurences = std::count(s.begin(), s.end(), charToBeReplaced);
  std::string result;
  result.reserve(s.size() + nOccurences * replacement.size());
  for(char i : s) {
    if(i != charToBeReplaced) {
      result += i;
    }
    else {
      result += replacement;
    }
  }
  return result;
}

/**********************************************************************************************************************/

std::string replaceAll(
    const std::string& s, const std::string& strToBeReplaced, const std::string& replacement) noexcept {
  // If either string contains null, make sure to explicitly set the length to the correct length.
  static const size_t defaultLimit = 1000;
  std::string result = s; // copy

  size_t pos = 0;
  size_t limiter = 0;
  const size_t limit = std::max(defaultLimit, s.size());
  const size_t repSize = replacement.size();
  int nOccurences = 0;
  while(((pos = result.find(strToBeReplaced, pos)) != std::string::npos) and (limit > limiter++)) {
    result.replace(pos, strToBeReplaced.size(), replacement);
    pos += repSize;
    ++nOccurences;
  }
  result.resize(s.size() + nOccurences * replacement.size() - nOccurences * strToBeReplaced.size());
  return result;
}

/**********************************************************************************************************************/

std::string denull(const std::string& s) noexcept {
  return replaceAll(s, '\0', NULL_TAG);
}

/**********************************************************************************************************************/

std::string printable(const std::string& s) noexcept {
  return replaceAll(s, '\0', "\\0");
}

/**********************************************************************************************************************/

std::string renull(const std::string& s) noexcept {
  static const std::string nullStr{"\0", 1}; // have to set the length to 1
  return replaceAll(s, NULL_TAG, nullStr);
}

/**********************************************************************************************************************/

bool strCmp(const std::string& a, const std::string& b) noexcept {
  std::string aHex = hexStrFromBinaryStr(a);
  std::string bHex = hexStrFromBinaryStr(b);
  if(a.size() != b.size()) {
    std::cout << "strCmp fail - length mismatch: " << a << " length " << a.size() << " != " << b << " length "
              << b.size() << "   0x" << aHex << " != 0x" << bHex << std::endl;
    return false;
  }
  for(size_t i = 0; i < a.size(); i++) {
    if(a[i] != b[i]) {
      std::cout << "strCmp fail - content mismatch: " << a << " and " << b << " differ at index i=" << i << " ("
                << size_t(a[i]) << " vs " << size_t(b[i]) << ")"
                << "   0x" << aHex << " != 0x" << bHex << std::endl;
      return false;
    }
  }
  return true;
}

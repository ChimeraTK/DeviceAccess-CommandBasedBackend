// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#include "stringUtils.h"

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

std::string replaceAll(const std::string& s, const char charToBeReplaced, const std::string& replacement) noexcept {
  size_t nOccurences = std::count(s.begin(), s.end(), charToBeReplaced);
  std::string result;
  result.reserve(s.size() + nOccurences * replacement.size());
  for(size_t i = 0; i < s.size(); ++i) {
    if(s[i] != charToBeReplaced) {
      result += s[i];
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
  long limiter = 0;
  long limit = std::max(defaultLimit, s.size());
  size_t repSize = replacement.size();
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

bool strCmp(const std::string& A, const std::string& B) noexcept {
  if(A.size() != B.size()) {
    std::cout << "strCmp fail - length mismatch: " << A << " length " << A.size() << " != " << B << " length "
              << B.size() << std::endl;
    return false;
  }
  for(size_t i = 0; i < A.size(); i++) {
    if(A[i] != B[i]) {
      std::cout << "strCmp fail - content mismatch: " << A << " and " << B << " differ at index i=" << i << " ("
                << size_t(A[i]) << " vs " << size_t(B[i]) << ")" << std::endl;
      return false;
    }
  }
  return true;
}

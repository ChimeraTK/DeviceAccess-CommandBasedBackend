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

// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#include "stringUtils.h"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <iostream>
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

bool strEndsInDelim(const std::string& str, const std::string& delim, const size_t delim_size) noexcept {
  int s = str.size() - 1;
  int d = delim_size - 1;
  while(s >= 0 and d >= 0) {
    if(str[s--] != delim[d--]) {
      return false;
    }
  }
  return d < 0;
}

/**********************************************************************************************************************/

std::string stripDelim(const std::string& str, const std::string& delim, const size_t delim_size) noexcept {
  if(strEndsInDelim(str, delim, delim_size)) {
    return str.substr(0, str.size() - delim_size);
  }
  else {
    return str;
  }
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

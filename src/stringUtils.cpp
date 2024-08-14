// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#include "stringUtils.h"

#include <cstring>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

std::vector<std::string> parseStr(const std::string& stringToBeParsed, const std::string& delimiter) noexcept {
  std::vector<std::string> lines;
  size_t pos = 0;

  while(pos != std::string::npos) {
    size_t nextPos = stringToBeParsed.find(delimiter, pos);
    std::string line = stringToBeParsed.substr(pos, nextPos - pos);
    lines.push_back(line);

    // Move the position to the next delimiter
    if(nextPos != std::string::npos) {
      pos = nextPos + delimiter.length();
    }
    else {
      break;
    }
  }

  return lines;
} // end parseStr

/**********************************************************************************************************************/

std::vector<std::string> parseStr(const std::string& stringToBeParsed, const char delimiter) noexcept {
  std::vector<std::string> result;
  std::stringstream ss(stringToBeParsed);
  std::string item;
  while(std::getline(ss, item, delimiter)) {
    result.push_back(item);
  }
  return result;
} // end parseStr

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
    result.replace(pos, 1, "N");
    pos = result.find('\n', pos + 1);
  }

  // Replace '\r' with 'R'
  pos = result.find('\r');
  while(pos != std::string::npos) {
    result.replace(pos, 1, "R");
    pos = result.find('\r', pos + 1);
  }

  return result;
} // end replaceNewlines

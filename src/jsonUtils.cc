// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "jsonUtils.h"

#include <nlohmann/json.hpp>

#include <optional>
#include <string>

/**********************************************************************************************************************/

std::optional<json> caseInsensitiveGetValueOption(const json& j, const std::string& caseInsensitiveKeyString) {
  for(auto& [jsonKey, value] : j.items()) {
    if(caseInsensitiveStrCompare(jsonKey, caseInsensitiveKeyString)) {
      return value;
    }
  }
  return std::nullopt;
}

/**********************************************************************************************************************/

template<typename T>
T caseInsensitiveGetValueOr(const json& j, const std::string& caseInsensitiveKeyString, T defaultValue) {
  if(auto optValue = caseInsensitiveGetValueOption(j, caseInsensitiveKeyString)) {
    return optValue->get<T>();
  }
  return defaultValue;
}

/**********************************************************************************************************************/

template<size_t N>
void throwIfHasInvalidJsonKey(
    const json& j, const std::array<std::string, N>& validKeys, const std::string& errorMessage) {
  for(auto it = j.begin(); it != j.end(); ++it) {
    bool stringIsNotInArray = true;
    for(const std::string& i : validKeys) {
      if(it.key() == i) {
        stringIsNotInArray = false;
        break;
      }
    }

    if(stringIsNotInArray) {
      throw ChimeraTK::logic_error(errorMessage + " " + it.key());
    }
  }
}

/**********************************************************************************************************************/

template<size_t N>
void throwIfHasInvalidJsonKeyCaseInsensitive(
    const json& j, const std::array<std::string, N>& validKeys, const std::string& errorMessage) {
  for(auto it = j.begin(); it != j.end(); ++it) {
    bool stringIsNotInArray = true;
    for(const std::string& i : validKeys) {
      if(caseInsensitiveStrCompare(it.key(), i)) {
        stringIsNotInArray = false;
        break;
      }
    }

    if(stringIsNotInArray) {
      throw ChimeraTK::logic_error(errorMessage + " " + it.key());
    }
  }
}

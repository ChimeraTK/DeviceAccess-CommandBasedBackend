// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "stringUtils.h"

#include <ChimeraTK/Exception.h>

#include <nlohmann/json.hpp>

#include <algorithm>
#include <optional>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <vector>

using json = nlohmann::json;

namespace ChimeraTK {

  /**
   * @brief Gets the json value for the key, if present, otherwise std::nullopt
   * Example Usage:
   * if (auto delimOption = caseInsensitiveGetValueOption(j, "dElImItEr")) {
   *   std::string delim = delimOption->get<std::string>();
   * }
   * @param[in] caseInsensitiveKeyString A case insensitive json key
   * @returns an optional of the json-wrapped value.
   */
  [[nodiscard]] std::optional<json> caseInsensitiveGetValueOption(
      const json& j, const std::string& caseInsensitiveKeyString);

  /********************************************************************************************************************/
  /**
   * @brief Gets the value from the json, if present, corresponding the key, or else the default value
   * The funny business with decay_t ensures that string literals passed to defaultValue get converted to std::string -
   * needed by the get type parameter.
   * @param[in] caseInsensitiveKeyString A case insensitive json key
   * @return value with type matching the type of the defaultValue
   */
  template<typename T>
  [[nodiscard]] inline T caseInsensitiveGetValueOr(
      const json& j, const std::string& caseInsensitiveKeyString, T defaultValue) {
    auto optJsonValue = caseInsensitiveGetValueOption(j, caseInsensitiveKeyString);
    return optJsonValue ? optJsonValue->get<T>() : defaultValue;
  }

  // Handle the case that a string literal is passed, which would make T = const char*, which get can't take.
  [[nodiscard]] inline std::string caseInsensitiveGetValueOr(
      const json& j, const std::string& caseInsensitiveKeyString, const char* defaultValue) {
    return caseInsensitiveGetValueOr(j, caseInsensitiveKeyString, std::string(defaultValue));
  }

  /********************************************************************************************************************/

  /**
   * Throws a ChimeraTK::logic_error if the json has a key is that is not in the right-hand-side of an
   * unordered_map (mapping enums to valid keys), or if there are duplicate keys, or the json is null.
   * while ignoring the case of the keys
   */
  template<typename EnumType>
  void throwIfHasInvalidJsonKeyCaseInsensitive(
      const json& j, const std::unordered_map<EnumType, std::string>& validKeysMap, const std::string& errorMessage) {
    if(j.is_null()) {
      throw ChimeraTK::logic_error(errorMessage + ". JSON is null");
    }
    std::vector<std::string> validKeysSeen;
    std::vector<std::string> duplicateKeyErrors = {};
    for(auto it = j.begin(); it != j.end(); ++it) {
      bool keyIsValid = false;

      // Check if the current JSON key matches any string value in the unordered_map
      for(auto mapIt = validKeysMap.begin(); (mapIt != validKeysMap.end()) and (not keyIsValid); ++mapIt) {
        keyIsValid = caseInsensitiveStrCompare(it.key(), mapIt->second);

        if(keyIsValid) { // If key found, make sure it's not a duplicate
          if(std::find(validKeysSeen.begin(), validKeysSeen.end(), mapIt->second) == validKeysSeen.end()) {
            // Key not seen before, add it to the list
            validKeysSeen.push_back(mapIt->second);
          }
          else { // This key has been seen before; add a complaint to the error stream.
            duplicateKeyErrors.push_back(" Duplicate key:\"" + it.key() + "\".");
          }
        }
      }

      if(not keyIsValid) {
        throw ChimeraTK::logic_error(errorMessage + ". Unknown key:\"" + it.key() + "\".");
      }
      if(not duplicateKeyErrors.empty()) {
        std::string errorStream{};
        for(const auto& error : duplicateKeyErrors) {
          errorStream += error;
        }
        throw ChimeraTK::logic_error(errorMessage + errorStream);
      }
    }
  }

  /********************************************************************************************************************/

} // end namespace ChimeraTK

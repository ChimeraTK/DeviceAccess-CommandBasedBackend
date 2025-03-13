// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once
#include "stringUtils.h"
#include <nlohmann/json.hpp>

#include <optional>
#include <string>

using json = nlohmann::json;

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
    const json& j, const std::string& caseInsensitiveKeyString) noexcept;

/**
 * @brief Gets the value from the json, if present, corresponding the key, or else the default value
 * @param[in] caseInsensitiveKeyString A case insensitive json key
 * @return value with type matching the type of the defaultValue
 */
template<typename T>
[[nodiscard]] T caseInsensitiveGetValueOr(
    const json& j, const std::string& caseInsensitiveKeyString, T defaultValue) noexcept;

/**
 * Throws a ChimeraTK::logic_error if the json has a key is that is not in an std::array of valid keys.
 * TODO remove this since its unused.
 */
template<size_t N>
void throwIfHasInvalidJsonKey(
    const json& j, const std::array<std::string, N>& validKeys, const std::string& errorMessage);

/**
 * Throws a ChimeraTK::logic_error if the json has a key is that is not in an std::array of valid keys,
 * while ignoring the case of the keys
 */
template<size_t N>
void throwIfHasInvalidJsonKeyCaseInsensitive(
    const json& j, const std::array<std::string, N>& validKeys, const std::string& errorMessage);

// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "mapFileKeys.h"

#include <functional>
#include <string>
#include <string_view>

// Type alias for our checksum function signature
using checksumFunction = std::function<std::string(const std::string&)>;

/**
 * @brief Retrieves a regex string to match the checksum
 * @param cs Enum indicating which checksum to get
 * @return A string representation of a regex
 * @throws ChimeraTK::logic_error if cs isn't mapped.
 */
[[nodiscard]] std::string getRegexString(checksum cs);

/**
 * @brief Retrieves a checksum function by its name.
 * @param cs Enum indicating which checksum to get
 * @return The checksumFunction if found.
 * @throws ChimeraTK::logic_error if cs isn't mapped.
 */
checksumFunction getChecksumer(checksum cs);

/**
 * Validates the checksum tags in the pattern string.
 * @throws ChimeraTK::logic_error if its invalid.
 */
void validateChecksumPattern(const std::string& pattern, const std::string& errorMessageDetail);

/**
 * @brief Parse the inja pattern and count the number of CS tags.
 * @param[in] pattern The inja pattern string
 * @return The number of checksums tags
 * @throws ChimeraTK::logic_error if the pattern is invalid
 */
[[nodiscard]] size_t getNChecksums(const std::string& pattern, const std::string& errorMessageDetail);

/**
 * @brief Parses the inja pattern and fetcehs the checksum blocks between the start and end tags.
 * @param[in] pattern The inja pattern string
 * @return A vector of string_views corresponding to the checksum blocks
 * @throws ChimeraTK::logic_error if the pattern is invalid
 */
[[nodiscard]] std::vector<std::string_view> getChecksumBlockSnippets(
    const std::string& pattern, const std::string& errorMessageDetail);

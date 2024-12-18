// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once
#include <string>
#include <vector>

/**
 * Split a string by the delimiter, and return a vector of the resulting segments.
 * No delimiters are present in the resulting segments.
 * If the string starts/ends with a delimiter, then there is an empty string at the beginning/end of the vector.
 */
[[nodiscard]] std::vector<std::string> splitString(
    const std::string& stringToBeParsed, const std::string& delimiter) noexcept;

/**
 * Parse the string into a vector of space-delimited tokens.
 * No whitespace is retuned in the result.
 * The vector may be empty, which occurs if the input string is empty or there is only whitespace.
 */
[[nodiscard]] std::vector<std::string> tokenise(const std::string& stringToBeParsed) noexcept;

/**
 * Returns true if and only if the provided string ends in the delimiter delim.
 * @param[in] delim The delimiter
 # @param[in] delim_size Must equal delim.size()
 */
[[nodiscard]] bool strEndsInDelim(const std::string& str, const std::string& delim, size_t delim_size) noexcept;

/**
 * Removes the delimiter delim from str if it's present and returns the result.
 * If no delimiter is found, then returns the input.
 * Use this to ensure that the resulting string doesn't end in the delimiter.
 * @param[in] delim The delimiter
 # @param[in] delim_size Must equal delim.size()
 */
[[nodiscard]] std::string stripDelim(const std::string& str, const std::string& delim, size_t delim_size) noexcept;

/**
 * This replaces '\n' with 'N' and '\r' with 'R' and returns the modified string.
 * This is only used for visualizing delimiters during debugging.
 */
[[nodiscard]] std::string replaceNewLines(const std::string& input) noexcept;

/**
 * Convert string to lower case in-place. Use this to make inputs case insensitive.
 */
void toLowerCase(std::string& str) noexcept;

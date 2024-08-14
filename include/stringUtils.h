// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once
#include <string>
#include <vector>

/**
 * Parse a string by the delimiter, and return a vector of the resulting segments
 * no delimiters are present in the resulting segments.
 */
[[nodiscard]] std::vector<std::string> parseStr(
    const std::string& stringToBeParsed, const std::string& delimiter = "\r\n") noexcept;

/**
 * Parse the string by the delimiter. Return a vector the resulting strings segments.
 * Strings in the vector will not contain the delimiter.
 */
[[nodiscard]] std::vector<std::string> parseStr(const std::string& stringToBeParsed, const char delimiter) noexcept;

/**
 * Returns true if and only if the provided string ends in the delimiter delim.
 * Requires delim_size = delim.size()
 */
[[nodiscard]] bool strEndsInDelim(const std::string& str, const std::string& delim, const size_t delim_size) noexcept;

/**
 * Removes the delimiter delim from str if it's present and returns the result
 * If no delimiter is found, returns the input
 * Use this to ensure that the resulting string doesn't end in the delimiter
 * Requires delim_size = delim.size()
 */
[[nodiscard]] std::string stripDelim(
    const std::string& str, const std::string& delim, const size_t delim_size) noexcept;

/**
 * This replaces '\n' with 'N' and '\r' with 'R' and returns the modified string.
 * This is only used for visualizing delimiters during debugging.
 */
[[nodiscard]] std::string replaceNewLines(const std::string& input) noexcept;

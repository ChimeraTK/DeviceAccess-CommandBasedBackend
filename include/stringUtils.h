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
 * @param[in] delimiter The line delimiter
 # @param[in] delimiterSize Must equal delimiter.size()
 */
[[nodiscard]] bool strEndsInDelim(const std::string& str, const std::string& delimiter, size_t delimiterSize) noexcept;

/**
 * Removes the line delimiter from str if it's present and returns the result.
 * If no delimiter is found, then returns the input.
 * Use this to ensure that the resulting string doesn't end in the delimiter.
 * @param[in] delimiter The line delimiter
 # @param[in] delimiterSize Must equal delim.size()
 */
[[nodiscard]] std::string stripDelim(
    const std::string& str, const std::string& delimiter, size_t delimiterSize) noexcept;

/**
 * This replaces '\n' with 'N' and '\r' with 'R' and returns the modified string.
 * This is only used for visualizing delimiters during debugging.
 */
[[nodiscard]] std::string replaceNewLines(const std::string& input) noexcept;

/**
 * Convert string to lower case in-place. Use this to make inputs case insensitive.
 */
void toLowerCase(std::string& str) noexcept;

/**
 * @brief Convert the string of hexidecimal into a string containing the corresponding binary data.
 * @param[in] hexData A string of hexidecimal such as "B0B", case insensitive.
 * If hexData has odd length, then a leading 0 is assumed.
 * @returns A string containing the corresponding binary data, with characters like \xFF, of length ceil(input.ceil / 2).
 */
[[nodiscard]] std::string binaryStrFromHexStr(const std::string& hexData) noexcept;

/**
 * @brief Convert a string container of bytes into the string hexidecimal representation of that data.
 * @param[in] binaryData A string container of bytes (like "\xFF").
 * @returns the hexidecimal representation string of the input, like "FF". Capitals are used for A-F. The returned
 * string is guarenteed to be exactly twice as long as the input string.
 */
[[nodiscard]] std::string hexStrFromBinaryStr(const std::string& binaryData) noexcept;

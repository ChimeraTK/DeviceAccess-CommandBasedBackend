// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once
#include <type_traits>

#include <cassert> //DEBUG
#include <optional>
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

/**
 * @brief Case insensitive equality comparison of two strings.
 * @returns true if and only if tolower(a) == tolower(b).
 */
[[nodiscard]] bool caseInsensitiveStrCompare(const std::string& a, const std::string& b) noexcept;

/**********************************************************************************************************************/

template<typename T>
using enableIfIntegral = std::enable_if_t<std::is_integral<T>::value>;

template<typename T>
using enableIfFloat = std::enable_if_t<std::is_floating_point<T>::value>;

template<typename T>
using enableIfNumeric = std::enable_if_t<std::is_arithmetic<T>::value>;

/**********************************************************************************************************************/
enum class OverflowBehavior { NULLOPT, EXPAND, TRUNCATE };

template<typename intType, typename = enableIfIntegral<intType>>
static size_t getIntNaturalWidth(const intType payload) noexcept {
  size_t naturalWidth = 1; // how many chars long should payload be with no leading 0's.
  if(payload >= 0) {
    intType temp = payload >> 8;
    int limit = 130; // DEBUG
    while((temp > 0) and (0 < limit--)) {
      temp >>= 8;
      naturalWidth++;
    }
    assert(limit > 0 && "Error! Runaway while loop on positive branch of enableIfIntegral"); // DEBUG
  }
  else { // negative payload
    intType temp = payload >> 8;
    int limit = 130; // DEBUG
    while((temp != static_cast<intType>(-1)) and (0 < limit--)) {
      temp >>= 8; // bit shift left-packs with 0xFF for negatives.
      naturalWidth++;
    }
    assert(limit > 0 && "Error! Runaway while loop on negative branch of enableIfIntegral"); // DEBUG
  }
  return naturalWidth;
}

/**
 * @brief Converts signed and unsigned integers into a string holding the binary representation of the int.
 * This is only compiled when intType is an integer type
 * @param[in] payload A signed or unsigned integer of type intType.
 * @param[in] fixedWidth Determines the character width of the output, if set. When fixedWidth exceeding the size of
 * intType, the output is left-packed with the sign bit of payload.
 * @param[in] overflowBehavior Determins the behavior if a fixed width is set for the output, but the payload int cannot
 * fit. When overflowBehavior is not set, output size = natural width When overflowBehavior = NULLOPT,  output size =
 * fixedWidth if it fits, otherwise return a std::nullopt. When overflowBehavior = EXPAND,   output size =
 * max(fixedWidth,natural width) When overflowBehavior = TRUNCATE, output size = fixedWidth Overflow behavior comes into
 * play if the natural width of the payload exceeds the value of fixedWidth. (if payload is an int64 = 255, its natural
 * width is 1 char = '\xFF'. If payload is an int = -257, its natural width is 2 = "\xFE\xFF".
 * @returns an optional to a string container holding the binary representation of the int. std::nullopt is returned if
 * the int overflows and overflowBehavior = NULLOPT.
 */
template<typename intType, typename = enableIfIntegral<intType>>
[[nodiscard]] std::optional<std::string> binaryStrFromInt(const intType payload,
    const std::optional<size_t>& fixedWidth = std::nullopt,
    const OverflowBehavior overflowBehavior = OverflowBehavior::NULLOPT) noexcept {
  char leftPackChar = ((payload >= 0) ? '\0' : '\xFF');
  size_t naturalWidth =
      getIntNaturalWidth<intType>(payload); // how many chars long should payload be with no leading 0's.
  size_t strWidth = naturalWidth;

  if(fixedWidth.has_value()) {
    strWidth = fixedWidth.value();

    if(strWidth < naturalWidth) { // overflow situation
      if(overflowBehavior == OverflowBehavior::EXPAND) {
        strWidth = naturalWidth;
      }
      else if(overflowBehavior == OverflowBehavior::NULLOPT) {
        return std::nullopt;
      } // else overflowBehavior = TRUNCATE
    }
  }

  std::string result(strWidth, leftPackChar);

  const size_t nLeftPackBytes = std::max(static_cast<size_t>(0), strWidth - sizeof(intType));
  const size_t bytesToTransfer = strWidth - nLeftPackBytes;
  for(size_t i = 0; i < bytesToTransfer; i++) {
    // Transfer digits from least significant to most, populating the string from back to front
    result[strWidth - 1 - i] = static_cast<char>((payload >> (8 * i)) & 0xFF);
  }
  return result;
}

/**********************************************************************************************************************/
static inline size_t getStrNaturalWidth(
    const std::string binaryContainer, const bool interpretAsPositive = true) noexcept {
  const char leftpackChar = (interpretAsPositive ? '\0' : '\xFF');
  size_t naturalWidth = binaryContainer.find_first_not_of(leftpackChar);
  return ((naturalWidth == std::string::npos) ? 1 : naturalWidth);
}
/**
 * @brief Interprets the data in the string as a binary representation of an integer
 * The binaryContiainer is interpreted as negative if intType is signed and the first bit of binaryContainer is 1
 * binaryContainer is assumed to be left-packed with the sign bit. The number is not considered to overflow if only the
 * sign-bit-packed segment overlfows. For example, "\0\0\0\0\0\0\0\0\0\x04" fits in intType=int8 without overflowing.
 * Similarly, "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFE" also fits into int8 as -2.
 * This is only compiled when intType is an integer type.
 * @param[in] binaryContainer A string containing the binary representation of an integer. For example, "\xAB\x04".
 * @param[in] truncateIfOverflow When true, overflowing binaryContainer are left-truncated to fit in intType. Otherwise,
 * std::nullopt is returned under that condition.
 * @returns integer of type intType, if possible, representing the data in binaryContainer.
 */
template<typename intType, typename = enableIfIntegral<intType>>
[[nodiscard]] std::optional<intType> intFromBinaryStr(
    const std::string& binaryContainer, const bool truncateIfOverflow = false) noexcept {
  if(binaryContainer.empty()) {
    return 0;
  }

  bool isNegative = (std::is_signed<intType>::value) and (static_cast<unsigned char>((binaryContainer[0]) & 0x80) != 0);
  size_t naturalWidth = getStrNaturalWidth(binaryContainer, not isNegative);
  const size_t maxBytes = sizeof(intType);

  // Validate input length
  if((not truncateIfOverflow) and (naturalWidth > maxBytes)) {
    return std::nullopt;
  }

  intType result = 0;

  // Calculate how many leading F's we need to add if the string is shorter than expected
  const size_t nLeftPackBytes = std::max(static_cast<size_t>(0), maxBytes - binaryContainer.size());
  if(isNegative) {
    uint64_t ff = 0xFF;
    for(size_t i = 0; i < nLeftPackBytes; i++) {
      size_t shift = 8 * (maxBytes - 1 - i);
      result |= static_cast<intType>(ff << shift);
    }
  }

  // Process each byte in the input string
  const size_t bytesToTransfer = maxBytes - nLeftPackBytes;
  const size_t truncationBytes = binaryContainer.size() - bytesToTransfer;
  for(size_t i = 0; i < bytesToTransfer; ++i) {
    size_t shift = 8 * (bytesToTransfer - 1 - i);
    result |= static_cast<intType>(static_cast<unsigned char>(binaryContainer[i + truncationBytes])) << shift;
  }

  return result;
}

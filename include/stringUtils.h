// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include <ChimeraTK/SupportedUserTypes.h>

#include <cassert>
#include <cstring> //for memcpy
#include <optional>
#include <string>
#include <type_traits>
#include <variant>
#include <vector>

/*
 * This is a debugging utility to compare two strings, which are likey to be binary data.
 * If they do not match, this prints to cout, highlighting the differences.
 * @returns true if A and B are identical.
 */
bool strCmp(const std::string& a, const std::string& b) noexcept; // DEBUG

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
 * Returns a lower case copy of str.
 */
std::string getLower(const std::string& str) noexcept;

/**
 * @brief Convert the string of hexidecimal into a string containing the corresponding binary data.
 * @param[in] hexData A string of hexidecimal such as "B0B", case insensitive.
 * @param[in] isSigned Whether to treat hexStr as signed in case hexStr.length() is odd.
 * @returns A string containing the corresponding binary data, with characters like \xFF, of length ceil(input.ceil / 2).
 */
[[nodiscard]] std::string binaryStrFromHexStr(const std::string& hexStr, bool isSigned = false) noexcept;

/**
 * @brief Convert a string container of bytes into the string hexidecimal representation of that data. The hex output
 * and byteStr input can be different lengths.
 * @param[in] byteStr A string container of bytes (like "\xFF"). It must have the length set accurately, since without
 * intervention it will terminate on the furst null character.
 * @param[in] nHexChars Is the length of the output in hexidecimal character count (nibbles).
 * @param[in] isSigned Whether or not to treat the binary input as a signed input - in case the output needs to be
 * left-padded.
 * @returns the hexidecimal representation string of the input, like "FF". Capitals are used for A-F.
 * If nHexChars is not specified, the return length is 2 x byteStr.length.
 * If nHexChars < 2*byteStr.length(), the byte string is left truncated. If nHexChar is larger, it is left padded.
 * string is guarenteed to be exactly twice as long as the input string.
 */
[[nodiscard]] std::string hexStrFromBinaryStr(
    const std::string& byteStr, std::optional<size_t> nHexChars = std::nullopt, bool isSigned = false) noexcept;

/**
 * @brief Case insensitive equality comparison of two strings.
 * @returns true if and only if tolower(a) == tolower(b).
 */
[[nodiscard]] bool caseInsensitiveStrCompare(const std::string& a, const std::string& b) noexcept;

// Returns a string with all instance of the character charToBeReplaced in string s replace with the replacement string.
[[nodiscard]] std::string replaceAll(
    const std::string& s, char charToBeReplaced, const std::string& replacement) noexcept;

// Returns a string with all instance of the string strToBeReplaced in string s replace with the replacement string.
[[nodiscard]] std::string replaceAll(
    const std::string& s, const std::string& strToBeReplaced, const std::string& replacement) noexcept;

// Placeholder tag for null characters with secure randomly generated 60b constant in base 64.
const std::string NULL_TAG = "NULLCHAR_E0xUr3HTw@_";
const size_t NULL_TAG_SIZE = NULL_TAG.size() - 1;

/**
 * Replaces null characters in s with '\'+'0', to make them printable.
 */
[[nodiscard]] std::string printable(const std::string& s) noexcept;

/**
 * Replaces null characters with the NULL_TAG,
 * which is cryptographically unlikely to be stumbled upon by any other input.
 */
[[nodiscard]] std::string denull(const std::string& s) noexcept;

/**
 * This is the reverse operation of denull, replaceing the NULL_TAG with '\0' again.
 */
[[nodiscard]] std::string renull(const std::string& s) noexcept;

/**********************************************************************************************************************/

template<typename T>
using enableIfIntegral = std::enable_if_t<std::is_integral<T>::value || std::is_same_v<T, ChimeraTK::Boolean>>;

template<typename T>
using enableIfNonBoolIntegral =
    std::enable_if_t<std::is_integral<T>::value && !std::is_same_v<T, bool> && !std::is_same_v<T, ChimeraTK::Boolean>>;

template<typename T>
using enableIfBool = std::enable_if_t<std::is_same_v<T, bool> || std::is_same_v<T, ChimeraTK::Boolean>>;

template<typename T>
using enableIfFloat = std::enable_if_t<std::is_floating_point<T>::value>;

template<typename T>
using enableIfNonBoolNumeric = std::enable_if_t<std::is_arithmetic<T>::value && !std::is_same_v<T, bool> &&
    !std::is_same_v<T, ChimeraTK::Boolean>>;

/**********************************************************************************************************************/

// Get the bit that would be the sign bit if it i were signed.
template<typename intType, typename = enableIfNonBoolIntegral<intType>>
inline static bool getFirstBit(const intType i) {
  using signedIntType = typename std::make_signed<intType>::type;
  return (static_cast<signedIntType>(i) < 0);
}

/**********************************************************************************************************************/

template<typename intType, typename = enableIfNonBoolIntegral<intType>>
static size_t getIntNaturalByteWidth(
    const intType payload, const bool isSigned = std::is_signed<intType>::value) noexcept {
  using signedIntType = typename std::make_signed<intType>::type;
  using unsignedIntType = typename std::make_unsigned<intType>::type;

  // Normalize to positives
  auto signedPayload = static_cast<signedIntType>(payload);
  if(isSigned and (signedPayload < 0)) {
    signedPayload *= -1;
  }
  auto unsignedPayload = static_cast<unsignedIntType>(signedPayload);
  // Count leading 0 bytes
  constexpr size_t start = sizeof(intType) - 1;
  size_t naturalWidth = 1;
  for(size_t i = start; i > 0; --i) {
    uint64_t shiftAmount = 8U * i;
    uint64_t mask = 0xFFU;
    if(((static_cast<uint64_t>(unsignedPayload) >> shiftAmount) & mask) != 0U) {
      // located the first byte that's non-zero.
      naturalWidth = i + 1;
      break;
    }
  }

  // Figure if an extra byte is needed for a the sign bit
  bool extraSignByte = false; // 0
  if(isSigned) {
    bool signBit = getFirstBit(payload); // can't use signed payload because of the -1
    unsignedIntType mask = 0x1U;
    unsignedIntType shiftAmount = 8U * naturalWidth - 1U;
    // NOLINTBEGIN(hicpp-signed-bitwise)
    bool lastPayloadBit = static_cast<bool>((payload >> shiftAmount) & mask);
    // Behavior relies on shifting the payload without converting to unsigned.
    // NOLINTEND(hicpp-signed-bitwise)
    extraSignByte = signBit xor lastPayloadBit;
  }

  return naturalWidth + static_cast<size_t>(extraSignByte);
}

/**********************************************************************************************************************/

enum class WidthOption { COMPACT, TYPE_WIDTH };
enum class OverflowBehavior { NULLOPT, EXPAND, TRUNCATE };

/*
 * @brief Converts numeric types into a string holding the binary representation of the number.
 * @param[in] payload A numeric type
 * @param[in] width If set, and numType is an integer type, this determines the character (byte) width of the output.
 * When width is WidthOption::TYPE_WIDTH, every byte in the payload will be reflected by a byte of the output string,
 * even if that's full of 0 or F. If numType is floating point, this is the only behavior, regardless of how width is
 * set. When width is a size_t that exceeding the size of numType, the output is left-packed with the sign bit of
 * payload. When width is WidthOption::Compact, the output will be the smallest string that fully represents the
 * payload.
 * @param[in] isSigned Whether to treat an int type as signed or unsigned. This overrides the signed status of numType,
 * providing an override to treat signed ints as unsigned and vice versa.
 * @param[in] overflowBehavior Determins the behavior if width is set to a size_t, but the payload value cannot fit. It
 * is not meaningful if width is a WidthOption. When overflowBehavior = NULLOPT,  output size = width if it fits,
 * otherwise return a std::nullopt. When overflowBehavior = EXPAND, output size = max(fixedWidth,natural width).
 * When overflowBehavior = TRUNCATE, output size = fixedWidth
 * @returns an optional to a string container holding the binary representation of the payload number. std::nullopt is
 * returned if the number overflows and overflowBehavior = NULLOPT
 */

template<typename numType>
[[nodiscard]] static std::optional<std::string> binaryStrFromNumber(const numType payload,
    const std::variant<size_t, WidthOption>& width = WidthOption::COMPACT,
    bool isSigned = std::is_signed<numType>::value, const OverflowBehavior overflowBehavior = OverflowBehavior::NULLOPT,
    enableIfNonBoolNumeric<numType>* = nullptr) {
  constexpr size_t numTypeWidth = sizeof(numType);
  static_assert(sizeof(numType) <= sizeof(uint64_t));

  size_t bytesToTransfer;
  size_t strWidth;
  std::string result;

  constexpr bool isFloat = std::is_floating_point_v<numType>;
  if(isFloat or
      (std::holds_alternative<WidthOption>(width) and (std::get<WidthOption>(width) == WidthOption::TYPE_WIDTH))) {
    bytesToTransfer = numTypeWidth;
    result = std::string(numTypeWidth, '\0');
    strWidth = numTypeWidth;
  }
  else if constexpr(std::is_integral_v<numType>) {
    bool isNegative = (isSigned and getFirstBit(payload));
    char leftPackChar = (isNegative ? '\xFF' : '\0');
    size_t naturalWidth = getIntNaturalByteWidth<numType>(
        payload, isSigned); // how many chars long should payload be with no leading 0's.
    if(std::holds_alternative<size_t>(width)) {
      strWidth = std::get<std::size_t>(width);
      if(strWidth < naturalWidth) { // overflow situation
        if(overflowBehavior == OverflowBehavior::NULLOPT) {
          return std::nullopt;
        }
        if(overflowBehavior == OverflowBehavior::EXPAND) {
          strWidth = naturalWidth;
        }
        else if((overflowBehavior == OverflowBehavior::TRUNCATE) and (strWidth == 0)) {
          return std::string("\0", 1);
        }
        // else the following truncates.
      }
    } // end fixed width
    else { // width = WidthOption::COMPACT
      strWidth = naturalWidth;
    }

    result = std::string(strWidth, leftPackChar);
    const size_t nLeftPackBytes = static_cast<size_t>(
        std::max(static_cast<int64_t>(0), static_cast<int64_t>(strWidth) - static_cast<int64_t>(numTypeWidth)));
    bytesToTransfer = strWidth - nLeftPackBytes;
  }

  // Copy into a uint so that we can do bit shifts, even if numType is float
  uint64_t shiftablePayload = 0;
  std::memcpy(&shiftablePayload, &payload, numTypeWidth);

  for(size_t i = 0; i < bytesToTransfer; i++) {
    // Transfer digits from least significant to most, populating the string from back to front
    result[strWidth - 1 - i] = static_cast<char>((shiftablePayload >> (8 * i)) & 0xFFU);
  }
  return result;
}

/*
 * @brief Converts boolean types (bool and ChimeraTK::Boolean) into a string holding the binary representation of the number.
 * @param[in] payload A bool
 * @param[in] width If set, this determines the character (byte) width of the output.
 * When width is WidthOption::TYPE_WIDTH or WidthOption::Compact, the output has size 1.
 * @param[in] isSigned This does nothing at all, and is kept to keep the same interface as above.
 * @param[in] overflowBehavior Determins the behavior if width is set to a size_t = 0.
 * It is not meaningful if width is a WidthOption. When overflowBehavior = NULLOPT,  output size = width if it fits,
 * otherwise return a std::nullopt. When overflowBehavior = EXPAND, output size = 1,
 * When overflowBehavior = TRUNCATE, the return string will then be "\0" regardless of payload.
 * Is not meaningful if width is a WidthOption. When overflowBehavior is not set, output size = natural width. When
 * overflowBehavior = NULLOPT,  output size = width if it fits, otherwise return a std::nullopt. When overflowBehavior =
 * EXPAND, output size = max(fixedWidth,natural width). When overflowBehavior = TRUNCATE, output size = fixedWidth
 * @returns an optional to a string container holding the binary representation of the payload number. std::nullopt is
 * returned if the number overflows and overflowBehavior = NULLOPT
 */

template<typename boolType>
[[nodiscard]] static std::optional<std::string> binaryStrFromNumber(const boolType payload,
    const std::variant<size_t, WidthOption>& width = WidthOption::COMPACT, [[maybe_unused]] bool isSigned = false,
    const OverflowBehavior overflowBehavior = OverflowBehavior::NULLOPT, enableIfBool<boolType>* = nullptr) {
  std::string ret;
  if(std::holds_alternative<size_t>(width)) {
    size_t strWidth = std::get<std::size_t>(width);
    if(strWidth == 0) {
      if(overflowBehavior == OverflowBehavior::NULLOPT) {
        return std::nullopt;
      }
      if(overflowBehavior == OverflowBehavior::TRUNCATE) {
        return std::string("\0", 1);
      }
      // else overflowBehavior == OverflowBehavior::EXPAND
      strWidth = 1;
    }
    ret = std::string(strWidth, '\0');
  }
  else {
    ret = std::string("\0", 1);
  }

  if(payload) {
    ret[ret.size() - 1] = '\x01';
  }
  return ret;
}

/**********************************************************************************************************************/

/**
 * @brief Converts numeric types into a string holding the binary representation of the number.
 * @param[in] payload A floating point type
 * @returns  a string container holding the binary representation of the floating point number. The length of the output
 * is determined by the size of floatType, with every byte in the payload will be reflected by a byte of the output string.
 */
template<typename floatType, typename = enableIfFloat<floatType>>
[[nodiscard]] std::string binaryStrFromFloat(const floatType payload) noexcept {
  return binaryStrFromNumber<floatType>(payload).value_or("");
  // The option returned by binaryStrFromNumber<floatType) is never nullopt.
}

/**********************************************************************************************************************/

/**
 * @brief Converts integer types into a string holding the binary representation of the int.
 * @param[in] payload A numeric type
 * @param[in] width Determines the character (byte) width of the output.
 * When width is WidthOption::TYPE_WIDTH, every byte in the payload will be reflected by a byte of the output string,
 * even if that's full of 0 or F. When width is WidthOption::Compact, the output will be the smallest string that fully
 * represents the payload.
 * @param[in] isSigned Whether to treat an int type as signed or unsigned. This overrides the signed status of numType,
 * providing an override to treat signed ints as unsigned and vice versa. This only does something when width=COMPACT.
 * For example, binaryStrFromInt<int32_t>(-5,COMPACT,true) --> "\xFB", with the leading F's contracted,
 * but binaryStrFromInt<int32_t>(-5,COMPACT,false) --> "\xFF\xFF\xFF\xFB" with all leading F's treated as meaningful data.
 * @returns an optional to a string container holding the binary representation of the payload int. std::nullopt is
 * never returned, and is kept for consistentcy.
 */
template<typename intType, typename = enableIfIntegral<intType>>
[[nodiscard]] std::optional<std::string> binaryStrFromInt(const intType payload,
    const WidthOption width = WidthOption::COMPACT, bool isSigned = std::is_signed<intType>::value) {
  return binaryStrFromNumber<intType>(payload, width, isSigned);
}

/**********************************************************************************************************************/

/**
 * @brief Converts integer types into a string holding the binary representation of the int.
 * @param[in] payload A numeric type
 * @param[in] width Determines the character (byte) width of the output.
 * When width is a size_t that exceeding the size of numType, the output is left-packed with the sign bit of payload.
 * @param[in] isSigned Whether to treat an int type as signed or unsigned. This overrides the signed status of numType,
 * providing an override to treat signed ints as unsigned and vice versa.
 * @param[in] overflowBehavior Determins the behavior when the natrual size of the payload doesn't fit within width in
 * bytes. For example, if the payload is a in64_t, and payload = 2, and width = 1, it fits. If payload = 10000 and width
 * = 1, it doesn't fit. When overflowBehavior = NULLOPT and the intType doesn't fit, nullopt is returned. When
 * overflowBehavior = EXPAND, output size = max(width,natural width). When overflowBehavior = TRUNCATE, output size =
 * width. Overflow behavior comes into play if the natural width of the payload exceeds the value of width. (if payload
 * is an int64 = 255, its natural width is 1 char = '\xFF'. If payload is an int = -257, its natural width is 2 =
 * "\xFE\xFF".
 * @returns an optional to a string container holding the binary representation of the payload int. std::nullopt is
 * returned if numType is > 64bit, or the int overflows and overflowBehavior = NULLOPT
 */
template<typename intType, typename = enableIfIntegral<intType>>
[[nodiscard]] std::optional<std::string> binaryStrFromInt(const intType payload, const size_t width,
    bool isSigned = std::is_signed<intType>::value,
    const OverflowBehavior overflowBehavior = OverflowBehavior::NULLOPT) {
  return binaryStrFromNumber<intType>(payload, width, isSigned, overflowBehavior);
}

/**********************************************************************************************************************/
/**********************************************************************************************************************/

/**
 * @brief Gets the hex string representation of the floating point payload.
 * @returns string containing the hexidecimal representation of the float, with length = 2*sizeof(floatType)
 */
template<typename floatType, typename = enableIfFloat<floatType>>
[[nodiscard]] std::string hexStrFromFloat(const floatType payload) noexcept {
  return hexStrFromBinaryStr(binaryStrFromFloat(payload));
}

/**********************************************************************************************************************/

/**
 * Returns true if the 0th
 */
static inline bool zerothNibbleCanBeRemoved(bool isSigned, bool isNegative, uint8_t byte0) {
  return ((isSigned and (((not isNegative) and (byte0 <= 0x07U)) or (isNegative and (byte0 >= 0xF8U)))) or
      ((not isSigned) and (byte0 <= 0x0FU)));
}
/**********************************************************************************************************************/

/**
 * @brief Returns the hex string representation of the int payload.
 * @param[in] width takes either the value WidthOption::COMPACT, or WidthOption::TYPE_WIDTH. COMPACT minimizes the
 * output length. TYPE_WIDTH makes the output reflect the entire intType, including leading 0's and F's, so the output
 * size will be 2*sizeof(intType).
 * @param[in] isSigned Whether to treat an int type as signed or unsigned. This overrides the signed status of numType,
 * providing an override to treat signed ints as unsigned and vice versa.
 * Returns the hex string representation, wrapped in an optional. The optional wrapping is done for consistentcy, even
 * though nullopt is never returned here.
 */
template<typename intType, typename = enableIfIntegral<intType>>
[[nodiscard]] std::optional<std::string> hexStrFromInt(const intType payload,
    const WidthOption width = WidthOption::COMPACT, bool isSigned = std::is_signed<intType>::value) {
  auto byteStrOpt = binaryStrFromInt<intType>(payload, width, isSigned);
  if(not byteStrOpt) {
    return std::nullopt;
  }
  std::string result = hexStrFromBinaryStr(*byteStrOpt);
  if(width == WidthOption::COMPACT and zerothNibbleCanBeRemoved(isSigned, (payload < 0), (*byteStrOpt)[0])) {
    result.erase(0, 1);
  }
  return result;
}

/**********************************************************************************************************************/

/**
 * @brief Gets the hexidecimal string representation of the integer payload.
 * @param[in] nHexChars The requested number of hex digits in the output string.
 * @param[in] isSigned Whether to treat an int type as signed or unsigned. This overrides the signed status of numType,
 * providing an override to treat signed ints as unsigned and vice versa.
 * @param[in] overflowBehavior Determins the behavior if the payload in nibbles exceeds the size of nHexChars (nHexChars
 * < natrual width of payload in nibbles ). When overflowBehavior = NULLOPT and the payload doesn't fit, std::nullopt is
 * returned. When overflowBehavior = EXPAND, the size of the output enlarges until it fits: output.size() =
 * max(nHexChars,natural width). When overflowBehavior = TRUNCATE, the output is left-truncated to fit: output.size() =
 * nHexChars.
 * @returns a string containing the hexidecimal representation of the integer type.
 */
template<typename intType, typename = enableIfIntegral<intType>>
[[nodiscard]] std::optional<std::string> hexStrFromInt(const intType payload, const size_t nHexChars,
    bool isSigned = std::is_signed<intType>::value,
    const OverflowBehavior overflowBehavior = OverflowBehavior::NULLOPT) {
  if((nHexChars == 0) and (overflowBehavior == OverflowBehavior::TRUNCATE)) {
    return "";
  }
  size_t byteWidth = (nHexChars / 2) + (nHexChars % 2);
  bool nHexCharIsOdd = static_cast<bool>(nHexChars % 2);
  auto byteStrOpt = binaryStrFromInt<intType>(payload, byteWidth, isSigned, overflowBehavior);
  if(not byteStrOpt) {
    return std::nullopt;
  }
  std::string result;

  if(overflowBehavior == OverflowBehavior::EXPAND) {
    result = hexStrFromBinaryStr(*byteStrOpt);

    // We may have over expanded in binaryStrFromNum, expanding bytes rather than nibbles
    if(zerothNibbleCanBeRemoved(isSigned, (payload < 0), (*byteStrOpt)[0])) {
      result.erase(0, 1);
    }
  }
  else {
    result = hexStrFromBinaryStr(*byteStrOpt, nHexChars, isSigned);
    if((overflowBehavior == OverflowBehavior::NULLOPT) and nHexCharIsOdd) {
      auto byte0 = static_cast<unsigned int>(static_cast<unsigned char>((*byteStrOpt)[0]));
      // We could use uint8_t but then multiple implicit casts are needed in the conditional

      if((((not isSigned) or (payload > 0)) and (byte0 & 0xF0U)) or
          (isSigned and (payload < 0) and ((byte0 & 0xF0U) != 0xF0U))) {
        /* nHexChars is odd, but binaryStrFromNumber returns an even byteWidth number of nibbles >= nHexChars.
         * binaryStrFromNumber will go nullopt if payload exceeds byteWidth, but there could still be meaningful content
         * in the first nibble of byteStrOpt that is truncated away in result. So we check that this nibble has no
         * meaningful value -- either leading 0 for positive, or leading F for negative. And if meaningful data would be
         * truncated away, we return std::nullopt
         */
        return std::nullopt;
      }
    }
  }
  return result;
}

/**********************************************************************************************************************/
/**********************************************************************************************************************/

/*
 * @brief Returns the minimum number of bytes needed represent the int in the binaryContainer.
 * param[in] interpretAsPositive Whether or not to binaryContainer as positive or negative.
 */
static inline size_t getStrNaturalByteWidth(
    const std::string& binaryContainer, const bool interpretAsPositive = true) noexcept {
  const char leftpackChar = (interpretAsPositive ? '\0' : '\xFF');
  size_t naturalWidth = binaryContainer.find_first_not_of(leftpackChar);
  return ((naturalWidth == std::string::npos) ? 1 : binaryContainer.size() - naturalWidth);
}

/*--------------------------------------------------------------------------------------------------------------------*/

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
template<typename intType>
[[nodiscard]] std::optional<intType> intFromBinaryStr(const std::string& binaryContainer,
    const bool truncateIfOverflow = false, enableIfNonBoolIntegral<intType>* = nullptr) noexcept {
  if(binaryContainer.empty()) {
    return 0;
  }

  bool isNegative =
      (std::is_signed<intType>::value) and ((static_cast<unsigned char>(binaryContainer[0]) & 0x80U) != 0);
  size_t naturalWidth = getStrNaturalByteWidth(binaryContainer, not isNegative);
  const size_t maxBytes = sizeof(intType);

  // Validate input length
  if((not truncateIfOverflow) and (naturalWidth > maxBytes)) {
    return std::nullopt;
  }

  using uintType = typename std::make_unsigned<intType>::type;
  uintType result = 0;

  // Calculate how many leading F's we need to add if the string is shorter than expected
  const size_t nLeftPackBytes = static_cast<size_t>(
      std::max(static_cast<int64_t>(0), static_cast<int64_t>(maxBytes) - static_cast<int64_t>(binaryContainer.size())));
  if(isNegative) {
    uint64_t ff = 0xFFU;
    for(size_t i = 0; i < nLeftPackBytes; i++) {
      size_t shift = 8 * (maxBytes - 1 - i);
      result |= static_cast<uintType>(ff << shift);
    }
  }

  // Process each byte in the input string
  const size_t bytesToTransfer = maxBytes - nLeftPackBytes;
  const size_t truncationBytes = binaryContainer.size() - bytesToTransfer;
  for(size_t i = 0; i < bytesToTransfer; ++i) {
    size_t shift = 8 * (bytesToTransfer - 1 - i);
    result |= static_cast<uintType>(
        static_cast<uintType>(static_cast<unsigned char>(binaryContainer[i + truncationBytes]))
        << shift); // Double cast necessary to keep left most bit of the char from binaryContainer from filling left.
  }

  return static_cast<intType>(result);
}

/*--------------------------------------------------------------------------------------------------------------------*/

template<typename boolType>
[[nodiscard]] std::optional<boolType> intFromBinaryStr(const std::string& binaryContainer,
    const bool truncateIfOverflow = false, enableIfBool<boolType>* = nullptr) noexcept {
  if(binaryContainer.empty()) {
    return false; // TODO FIXME overflowBehavior
  }
  if(truncateIfOverflow) {
    return static_cast<boolType>(static_cast<unsigned char>(binaryContainer[0]) & 0x01U);
  }
  unsigned int orOfBytes = 0;
  for(char b : binaryContainer) {
    orOfBytes |= static_cast<unsigned char>(b);
  }
  return (orOfBytes != 0);
}

/**********************************************************************************************************************/

/**
 * Converts a string containing the binary representation of a floating poitn number into the floating point number.
 */
template<typename floatType, typename = enableIfFloat<floatType>>
[[nodiscard]] std::optional<floatType> floatFromBinaryStr(const std::string& binaryContainer) noexcept {
  constexpr size_t nBytes = sizeof(floatType);
  static_assert(nBytes <= sizeof(uint64_t));

  if(binaryContainer.empty()) {
    return 0;
  }

  // validate the binary container's size.
  if(nBytes != binaryContainer.size()) {
    return std::nullopt;
  }
  uint64_t result_uint = 0;
  for(size_t i = 0; i < nBytes; ++i) {
    size_t shift = 8 * (nBytes - 1 - i);
    result_uint |= static_cast<uint64_t>(static_cast<unsigned char>(binaryContainer[i])) << shift;
  }

  floatType result;
  std::memcpy(&result, &result_uint, nBytes);
  return result;
}

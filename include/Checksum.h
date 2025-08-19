// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "CommandBasedBackendRegisterInfo.h"
#include "mapFileKeys.h"

#include <functional>
#include <string>

namespace ChimeraTK {

  class InteractionInfo;

  // Checksumers perform the chain: checksumOutputAdapter(checksumAlgorithm(checksumInputAdapter(data)))
  using Checksumer = std::function<std::string(const std::string&)>;

  /*
   * The checksumAlgorithm performs the checksum computation.
   * It always takes in a binary string and returns a hexidecimal string
   */
  using checksumAlgorithm = std::string (*)(const std::string&);

  enum class interactionType : int { CMD = 0, RESP };

  /**
   * @brief Construct the Checksumer functions so that they correctly interpret the checksum input and output.
   * @param iType An enum indicating whether this is a Command or a Response
   * @param iInfo Holds all other needed and potentially useful details.
   * @returns a vector of std::functions that will do the checksums.
   * @throws ChimeraTK::logic_error if the interactionInfo has a checksum enum that isn't mapped to an function.
   */
  std::vector<Checksumer> makeChecksumers(interactionType iType, const InteractionInfo& iInfo);

  /**
   * @brief Retrieves a checksum function by its name.
   * @param cs Enum indicating which checksum to get
   * @return The checksumAlgorithm function pointer, if found.
   * @throws ChimeraTK::logic_error if cs isn't mapped.
   */
  checksumAlgorithm getChecksumAlgorithm(checksum cs);

  /**
   * @brief Retrieves a regex string to match the checksum
   * @param cs Enum indicating which checksum to get
   * @return A string representation of a regex
   * @throws ChimeraTK::logic_error if cs isn't mapped.
   */
  [[nodiscard]] std::string getRegexString(checksum cs);

  /**
   * Validates the checksum tags in the pattern string.
   * @param[in] pattern The inja pattern string
   * @param[in] errorMessageDetail Orienting details to include in the error messages
   * @throws ChimeraTK::logic_error if its invalid.
   */
  void validateChecksumPattern(const std::string& pattern, const std::string& errorMessageDetail);

  /**
   * @brief Parse the inja pattern and count the number of CS tags.
   * @param[in] pattern The inja pattern string
   * @param[in] errorMessageDetail Orienting details to include in the error messages
   * @return The number of checksums tags
   * @throws ChimeraTK::logic_error if the pattern is invalid
   */
  [[nodiscard]] size_t getNChecksums(const std::string& pattern, const std::string& errorMessageDetail);

  /**
   * @brief Parses the inja pattern and fetcehs the checksum payloads between the start and end tags.
   * @param[in] pattern The inja pattern string
   * @param[in] errorMessageDetail Orienting details to include in the error messages
   * @return A vector of strings corresponding to the checksum payloads
   * @throws ChimeraTK::logic_error if the pattern is invalid
   */
  [[nodiscard]] std::vector<std::string> getChecksumPayloadSnippets(
      const std::string& pattern, const std::string& errorMessageDetail);

} // end namespace ChimeraTK

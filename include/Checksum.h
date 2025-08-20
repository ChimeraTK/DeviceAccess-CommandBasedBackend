// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "mapFileKeys.h"

#include <functional>
#include <string>

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

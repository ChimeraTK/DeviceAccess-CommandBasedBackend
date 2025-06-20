// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include <functional>
#include <string>
#include <unordered_map>

// Type alias for our checksum function signature
using checksumFunction = std::function<std::string(const std::string&)>;

/**
 * @brief Checks if a checksum name is valid (registered).
 * @param name The case-insensitive name of the checksum.
 * @return True if the name is mapped, false otherwise.
 * @throws ChimeraTK::logic_error if name is multiply defined in the checksumMap
 */
bool isValidChecksumName(std::string name);

/**
 * @brief Retrieves a checksum function by its name.
 * @param name The case-insensitive name of the checksum function.
 * @return The checksumFunction if found.
 * @throws ChimeraTK::logic_error if isValidChecksumName(name) is false
 */
checksumFunction getChecksumer(std::string name);

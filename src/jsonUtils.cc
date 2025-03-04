// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "jsonUtils.h"

#include <nlohmann/json.hpp>
#include <unordered_map>

#include <optional>
#include <string>

namespace ChimeraTK {
  /********************************************************************************************************************/

  std::optional<json> caseInsensitiveGetValueOption(
      const json& j, const std::string& caseInsensitiveKeyString) noexcept {
    for(auto& [jsonKey, value] : j.items()) {
      if(caseInsensitiveStrCompare(jsonKey, caseInsensitiveKeyString)) {
        return value;
      }
    }
    return std::nullopt;
  }

  /********************************************************************************************************************/
} // end namespace ChimeraTK

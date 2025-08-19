// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "injaUtils.h"

#include <ChimeraTK/Exception.h> //for ChimeraTK::logic_error

namespace ChimeraTK {
  /********************************************************************************************************************/

  std::string injaRender(const std::string& injaTemplate, const inja::json& j, const std::string& errorMessageDetail) {
    try {
      return inja::render(injaTemplate, j);
    }
    catch(const inja::InjaError& e) {
      throw ChimeraTK::runtime_error("injaRender: " + std::string(e.what()) + " for " + errorMessageDetail +
          " with inja template " + injaTemplate);
    }
  }

  /********************************************************************************************************************/

  std::regex injaRenderRegex(
      const std::string& injaTemplate, const inja::json& j, const std::string& errorMessageDetail) {
    std::string regexText = injaRender(injaTemplate, j, errorMessageDetail);
    try {
      return std::regex(regexText);
    }
    catch(const ChimeraTK::runtime_error& e) {
      throw ChimeraTK::logic_error(e.what());
    }
    catch(const std::regex_error& e) {
      throw ChimeraTK::logic_error("injaRenderRegex: Regex error " + errorMessageDetail + ": " + e.what() + " " +
          regexText + " from inja template " + injaTemplate);
    }
  }

  /********************************************************************************************************************/
} // end namespace ChimeraTK

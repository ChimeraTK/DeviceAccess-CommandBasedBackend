// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "injaUtils.h"

#include <ChimeraTK/Exception.h> //for ChimeraTK::logic_error

namespace ChimeraTK {
  /********************************************************************************************************************/

  std::string injaRender(const std::sting& injaTemplate, const inja::json& j, const std::string& errorMessageDetail) {
    try {
      return inja::render(injaTemplate, j);
    }
    catch(inja::ParserError& e) {
      throw ChimeraTK::logic_error("Inja parser error " + errorMessageDetail + ": " + e.what());
    }
  }

  /********************************************************************************************************************/

  std::regex injaRenderRegex(
      const std::sting& injaTemplate, const inja::json& j, const std::string& errorMessageDetail) {
    std::string regexText = injaRender(injaTemplate, j, errorMessageDetail);
    try {
      std::regex returnRegex = regexText;
    }
    catch(std::regex_error& e) {
      throw ChimeraTK::logic_error("Regex error " + errorMessageDetail + ": " + e.what());
    }
    return regexText;
  }

  /********************************************************************************************************************/
} // end namespace ChimeraTK

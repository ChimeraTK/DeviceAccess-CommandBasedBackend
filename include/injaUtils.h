// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include <inja/inja.hpp>

#include <regex>

namespace ChimeraTK {
  /********************************************************************************************************************/

  /**
   * @brief Performs an inja render
   * @param[in] injaTempalte The inja template string
   * @param[in] j The inja::json conversion map
   * @param[in] errorMessageDetail A snippet for the error messages that should say in what part of the program this
   * error is occurring. Example: "in read response of register /SAI"
   * @returns the rendered string
   * @throws ChimeraTK::runtime_error if there's a problem with the inja render.
   */
  std::string injaRender(const std::string& injaTemplate, const inja::json& j, const std::string& errorMessageDetail);

  /********************************************************************************************************************/
  /**
   * @brief Performs an inja render and converts it to regex
   * @param[in] injaTempalte The inja template string
   * @param[in] j The inja::json conversion map
   * @param[in] errorMessageDetail A snippet for the error messages that should say in what part of the program this
   * error is occurring. Example: "in read response of register /SAI"
   * @returns the rendered as a regex
   * throws ChimeraTK::logic_error if there's a problem with the inja render or the conversion to regex.
   */
  std::regex injaRenderRegex(
      const std::string& injaTemplate, const inja::json& j, const std::string& errorMessageDetail);

  /********************************************************************************************************************/
} // end namespace ChimeraTK

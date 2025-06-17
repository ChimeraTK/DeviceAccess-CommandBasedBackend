// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#include "CommandHandler.h"

#include <cassert>
#include <string>
#include <variant>

/**********************************************************************************************************************/

std::string CommandHandler::toString(const WritableDelimiter& delimOption) const noexcept {
  if(std::holds_alternative<CommandHandlerDefaultDelimiter>(delimOption)) {
    return delimiter;
  }
  if(std::holds_alternative<std::string>(delimOption)) {
    return std::get<std::string>(delimOption);
  }
  // else delimOption is NoDelimiter
  return "";
}

/**********************************************************************************************************************/

std::string CommandHandler::toString(const ReadableDelimiter& delimOption) const noexcept {
  if(std::holds_alternative<CommandHandlerDefaultDelimiter>(delimOption)) {
    return delimiter;
  }
  // delimOption is a string
  std::string s = std::get<std::string>(delimOption);
  assert(not s.empty());
  return s;
}

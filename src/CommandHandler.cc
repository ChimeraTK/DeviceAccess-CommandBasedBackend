// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#include "CommandHandler.h"

#include <cassert>
#include <string>
#include <variant>

/**********************************************************************************************************************/

std::string CommandHandler::toString(const Delimiter& delimOption) const noexcept {
  if(std::holds_alternative<CommandHandlerDefaultDelimiter>(delimOption)) {
    return delimiter;
  }
  // else delimOption is a string
  return std::get<std::string>(delimOption);
}

/**********************************************************************************************************************/

std::string CommandHandler::toStringGuarded(const Delimiter& delimOption) const {
  if(std::holds_alternative<CommandHandlerDefaultDelimiter>(delimOption)) {
    return delimiter;
  }
  // else delimOption is a string
  std::string s = std::get<std::string>(delimOption);
  assert(not s.empty());
  return s;
}

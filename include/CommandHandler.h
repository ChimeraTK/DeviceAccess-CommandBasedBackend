// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once
#include <string>
#include <vector>

/**
 * This interface gives control over SCPI devics over
 * various communication protocols defined in child classes.
 */
class CommandHandler {
 public:
  /**
   * Send a command to a SCPI device, read back one line of responce.
   */
  virtual std::string sendCommand(std::string cmd) = 0;

  /**
   * Send a command to a SCPI device, read back nLinesExpected line of responce.
   * Resulting vector will be nLinesExpected long or else throw
   */
  virtual std::vector<std::string> sendCommand(std::string cmd, const size_t nLinesExpected) = 0;

  virtual ~CommandHandler() = default;
};

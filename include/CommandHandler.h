// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once
#include <chrono>
#include <string>
#include <vector>

/**
 * This interface gives control over SCPI devics over
 * various communication protocols defined in child classes.
 */
class CommandHandler {
 public:
  /**
   * @brief Constructor for the CommandHandler class.
   * @param[in] delimiter The delimiter string used for communication.
   * @param[in] timeoutInMilliseconds The timeout duration in milliseconds.
   */
  CommandHandler(const std::string& delimiter = "\r\n", ulong timeoutInMilliseconds = 1000)
  : _delimiter(delimiter), _timeout(timeoutInMilliseconds) {}

  /**
   * Send a command to a SCPI device, read back nLinesExpected line of responce.
   * Resulting vector will be nLinesExpected long or else throw a ChimeraTK::runtime_error
   */
  virtual std::vector<std::string> sendCommand(std::string cmd, size_t nLinesExpected) = 0;

  virtual ~CommandHandler() = default;

 protected:
  /**
   * The default line delimiter appended ot the end of writes, and delimiting line reads.
   */
  const std::string _delimiter;

  /**
   * Timeout parameter in milliseconds used to to timeout the sendComman functions.
   */
  std::chrono::milliseconds _timeout;
};

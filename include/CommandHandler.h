// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once
#include <chrono>
#include <optional>
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
   * @brief Send a command to a SCPI device, read back nLinesExpected line of responce.
   * Resulting vector will be nLinesExpected long or else throw a ChimeraTK::runtime_error
   * @param[in] cmd The command to be sent
   * @param[in] nLinesExpected The number of lines expected in reply to the sent command cmd, and the length of hte
   * @param[in] overrideWriteDelimiter if set, this overrides the default _delimiter the writing operation in this call.
   * It can be set to "" to send a raw binary command.
   * @param[in] overrideReadDelimiter if set, this overrides the default _delimiter for the reading operation in this call.
   * Since empty string cannot be a line delimitier, if overrideReadDelimiter is "", the default delimiter will be used.
   * @returns A vector, of length nLinesExpected, of strings containing the responce lines.
   * @throws ChimeraTK::runtime_error if those returns do not occur within timeout.
   */
  virtual std::vector<std::string> sendCommandAndReadLines(std::string cmd, size_t nLinesExpected = 1,
      const std::optional<std::string>& overrideWriteDelimiter = std::nullopt,
      const std::string& overrideReadDelimiter = "") = 0;

  /**
   * @brief Send a command to a SCPI device, read back a set number of bytes of responce.
   * Resulting string will be nBytesExpected long or else throw a ChimeraTK::runtime_error
   * @param[in] cmd The command to be sent
   * @param[in] nBytesToRead The number of bytes expected in reply to the sent command cmd, and the length of the
   * @param[in] overrideWriteDelimiter if set, this overrides the default _delimiter the writing operation in this call.
   * It can be set to "" to send a raw binary command.
   * @returns A string as a container of bytes containing the responce. The return string is not null terminated.
   * @throws ChimeraTK::runtime_error if those returns do not occur within timeout.
   */
  virtual std::string sendCommandAndReadBytes(std::string cmd, size_t nBytesToRead,
      const std::optional<std::string>& overrideWriteDelimiter = std::nullopt) = 0;

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

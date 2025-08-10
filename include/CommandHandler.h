// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include <chrono>
#include <optional>
#include <string>
#include <utility> //for move()
#include <variant>
#include <vector>

class CommandHandlerDefaultDelimiter {};
using Delimiter = std::variant<CommandHandlerDefaultDelimiter, std::string>;

/**********************************************************************************************************************/

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
  explicit CommandHandler(std::string _delimiter = "\r\n", ulong timeoutInMilliseconds = 1000)
  : delimiter(std::move(_delimiter)), timeout(timeoutInMilliseconds) {}

  /**
   * @brief Send a command to a SCPI device, read back nLinesToRead line of response.
   * Resulting vector will be nLinesToRead long or else throw a ChimeraTK::runtime_error
   * @param[in] cmd The command to be sent, which should have no delimiter
   * @param[in] nLinesToRead The number of lines required in the reply to the sent command cmd, and the length of the
   * return vector. If 0, then no read is attempted.
   * @param[in] writeDelimiter if set, this overrides the default delimiter the writing operation in this call.
   * It can be set to "" to send a raw binary command.
   * @param[in] readDelimiter if set, this overrides the default delimiter for the reading operation in this call.
   * Since empty string cannot be a line delimitier, if overrideReadDelimiter is "", the default delimiter will be used.
   * @returns A vector, of length nLinesToRead, of strings containing the response lines.
   * @throws ChimeraTK::runtime_error if those returns do not occur within timeout.
   */
  std::vector<std::string> sendCommandAndReadLines(std::string cmd, size_t nLinesToRead = 1,
      const Delimiter& writeDelimiter = CommandHandlerDefaultDelimiter{},
      const Delimiter& readDelimiter = CommandHandlerDefaultDelimiter{}) {
    return sendCommandAndReadLinesImpl(std::move(cmd), nLinesToRead, writeDelimiter, readDelimiter);
  }

  /**
   * @brief Send a command to a SCPI device, read back a set number of bytes of response.
   * Resulting string will be nBytesToRead long or else throw a ChimeraTK::runtime_error
   * Since this is binary oriented, no write delimiter is used by default.
   * @param[in] cmd The command to be sent, which should have no delimiter
   * @param[in] nBytesToRead The number of bytes required in reply to the sent command cmd. If 0, no read is attempted.
   * @param[in] writeDelimiter if set, the specified write delimiter is added for this call, which can be a string or
   * CommandHandlerDefaultDelimiter{}.
   * @returns A string as a container of bytes containing the response. The return string is not null terminated.
   * @throws ChimeraTK::runtime_error if those returns do not occur within timeout.
   */
  std::string sendCommandAndReadBytes(std::string cmd, size_t nBytesToRead, const Delimiter& writeDelimiter = "") {
    return sendCommandAndReadBytesImpl(std::move(cmd), nBytesToRead, writeDelimiter);
  }

  virtual ~CommandHandler() = default;

  /**
   * The default line delimiter appended ot the end of writes, and delimiting line reads.
   */
  const std::string delimiter;

  /**
   * Timeout parameter in milliseconds used to to timeout the sendComman functions.
   */
  std::chrono::milliseconds timeout;

  [[nodiscard]] std::string toString(
      const Delimiter& delimOption) const noexcept; //!< Converts a delimiter option to string

  /**
   * @brief Converts a delimiter option to string, but throws if delimOption is emptyString. Use for readDelimiters.
   * Asserts delimOption is not ""
   */
  [[nodiscard]] std::string toStringGuarded(const Delimiter& delimOption) const;

 protected:
  virtual std::vector<std::string> sendCommandAndReadLinesImpl(
      std::string cmd, size_t nLinesToRead, const Delimiter& writeDelimiter, const Delimiter& readDelimiter) = 0;

  virtual std::string sendCommandAndReadBytesImpl(
      std::string cmd, size_t nBytesToRead, const Delimiter& writeDelimiter) = 0;
};

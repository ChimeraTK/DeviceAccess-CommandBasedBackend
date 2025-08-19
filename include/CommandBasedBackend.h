// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "CommandBasedBackendRegisterAccessor.h"
#include "CommandBasedBackendRegisterInfo.h"
#include "CommandHandler.h"

#include <ChimeraTK/AccessMode.h>
#include <ChimeraTK/BackendFactory.h>
#include <ChimeraTK/BackendRegisterCatalogue.h>
#include <ChimeraTK/DeviceAccessVersion.h>
#include <ChimeraTK/DeviceBackendImpl.h>

#include <boost/make_shared.hpp>

#include <memory>
#include <mutex>

namespace ChimeraTK {

  /**
   * Provides a communications backend for command based communications.
   * This can work with both serail and network/ethernet communications.
   */
  class CommandBasedBackend : public DeviceBackendImpl {
    /******************************************************************************************************************/
   public:
    /**
     * SERIAL indicates serial communications, such as USB
     * ETHERNET indicates TCP/IP network communications.
     */
    enum class CommandBasedBackendType { SERIAL, ETHERNET };

    CommandBasedBackend(
        CommandBasedBackendType type, std::string instance, std::map<std::string, std::string> parameters);

    ~CommandBasedBackend() override = default;

    void open() override;
    void close() override;

    /**
     * @brief Send a single command through and receive a vector of responses.
     * This takes care of the details of whether or reading lines or bytes.
     * @param[in] command Is the exact string sent. This may differ from iInfo.commandPattern due to the use of inja templates.
     * @returns a vector of responces, corresponding to lines if we're reading lines. If we're reading bytes, the return
     * vector will have length 1.
     * @throws ChimeraTK::runtime_error if any line of reply doesn't come before a timeout for that line.
     */
    std::vector<std::string> sendCommandAndRead(const std::string& cmd, const InteractionInfo& iInfo);

    /**
     * @brief Send a single command through and receive a vector (of length nLinesToRead) responses.
     * @param[in] cmd The command sent to the device.
     * @param[in] nLinesToRead The required number of lines in reply to sending command cmd.  If 0, then no read is attempted.
     * @param[in] writeDelimiter if set, this overrides the default _delimiter the writing operation in this call.
     * It can be set to "" to send a raw binary command.
     * @param[in] readDelimiter if set, this overrides the default _delimiter for the reading operation in this call.
     * @returns a vector of length nLinesToRead containing the response to cmd, with one line of response per entry in
     * the vector.
     * @throws ChimeraTK::runtime_error if any line of reply doesn't come before a timeout for that line.
     */
    std::vector<std::string> sendCommandAndReadLines(std::string cmd, size_t nLinesToRead = 1,
        const Delimiter& writeDelimiter = CommandHandlerDefaultDelimiter{},
        const Delimiter& readDelimiter = CommandHandlerDefaultDelimiter{});

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
    std::string sendCommandAndReadBytes(std::string cmd, size_t nBytesToRead, const Delimiter& writeDelimiter = "");

    template<typename UserType>
    // NOLINTNEXTLINE(readability-identifier-naming)
    boost::shared_ptr<NDRegisterAccessor<UserType>> getRegisterAccessor_impl(
        const RegisterPath& registerPathName, size_t numberOfWords, size_t wordOffsetInRegister, AccessModeFlags flags);

    static boost::shared_ptr<DeviceBackend> createInstanceSerial(
        std::string instance, std::map<std::string, std::string> parameters);

    static boost::shared_ptr<DeviceBackend> createInstanceEthernet(
        std::string instance, std::map<std::string, std::string> parameters);

    struct BackendRegisterer {
      BackendRegisterer();
    };

    RegisterCatalogue getRegisterCatalogue() const override;

    std::string readDeviceInfo() override;

    /*----------------------------------------------------------------------------------------------------------------*/

   protected:
    CommandBasedBackendType _commandBasedBackendType; /**< Indicates whether serial or ethernet (aka network) */

    /**  The device node for serial communication, or the host name for network communication. */
    std::string _instance;

    /**
     * Port parameters for network based communication.
     * Used when _commandBasedBackendType = CommandBasedBackendType::ETHERNET
     */
    std::string _port;

    /**
     * The timeout parameter given to the command handler upon open().
     * This becomes the timeout parameter of sendCommand
     * Its reported in readDeviceInfo().
     */
    ulong _timeoutInMilliseconds = 1000;

    std::mutex _mux; /**< mutex for protecting ordered port access */
    std::unique_ptr<CommandHandler> _commandHandler;

    // Obtained from map file
    std::string _defaultRecoveryRegister;
    std::string _serialDelimiter; /**< The line delimiter between messages in serial communications. */
    BackendRegisterCatalogue<CommandBasedBackendRegisterInfo> _backendCatalogue;

    /** The last register that was attempted to be written. Might have failed and is re-tried on open. */
    ChimeraTK::RegisterPath _lastWrittenRegister;

    /**
     * Fill in the backend register catalog from the json map file.
     * @param[in] mapFileName The name and path to the map file.
     * @throws ChimeraTK::logic_error if map file not found
     */
    void parseJsonAndPopulateCatalogue(const std::string& mapFileName);

    template<typename UserType>
    friend class CommandBasedBackendRegisterAccessor;

  }; // end class CommandBasedBackend

  /********************************************************************************************************************/
  /********************************************************************************************************************/
  /********************************************************************************************************************/

  template<typename UserType>
  // NOLINTNEXTLINE(readability-identifier-naming)
  boost::shared_ptr<NDRegisterAccessor<UserType>> CommandBasedBackend::getRegisterAccessor_impl(
      const RegisterPath& registerPathName, size_t numberOfWords, size_t wordOffsetInRegister, AccessModeFlags flags) {
    auto registerInfo = _backendCatalogue.getBackendRegister(registerPathName);

    return boost::make_shared<CommandBasedBackendRegisterAccessor<UserType>>(
        DeviceBackend::shared_from_this(), registerInfo, registerPathName, numberOfWords, wordOffsetInRegister, flags);
  }

} // end namespace ChimeraTK

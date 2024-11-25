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
     * Send a single command through and receive response.
     * Use this when responces are on a single line.
     * @param[in] cmd The command sent to the device.
     * @returns The first line of responce from the device.
     * @throws ChimeraTK::runtime_error if the reply doesn't come before a timeout.
     */
    std::string sendCommand(std::string cmd);

    /**
     * Send a single command through and receive a vector (len nLinesExpected) responses.
     * @param[in] cmd The command sent to the device.
     * @param[in] nLinesExpected The number of expected lines in reply to sending command cmd
     * @returns a vector of length nLinesExpected containing the responce to cmd, with one line of responce per entry in
     * the vector.
     * @throws ChimeraTK::runtime_error if any line of reply doesn't come before a timeout for that line.
     */
    std::vector<std::string> sendCommand(std::string cmd, size_t nLinesExpected);

    template<typename UserType>
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
     * Also reported in readDeviceInfo().
     * Avoid 1000 due to race conditions with a 1000ms timeout on
     * CommandBasedBackendRegisterAccessor::doReadTransferSynchronously
     */
    ulong _timeoutInMilliseconds = 2000;

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
    void parseJsonAndPopulateCatalogue(const std::string& file_name);

    template<typename UserType>
    friend class CommandBasedBackendRegisterAccessor;

  }; // end class CommandBasedBackend

  /********************************************************************************************************************/
  /********************************************************************************************************************/
  /********************************************************************************************************************/

  template<typename UserType>
  boost::shared_ptr<NDRegisterAccessor<UserType>> CommandBasedBackend::getRegisterAccessor_impl(
      const RegisterPath& registerPathName, size_t numberOfWords, size_t wordOffsetInRegister, AccessModeFlags flags) {
    auto registerInfo = _backendCatalogue.getBackendRegister(registerPathName);

    return boost::make_shared<CommandBasedBackendRegisterAccessor<UserType>>(
        DeviceBackend::shared_from_this(), registerInfo, registerPathName, numberOfWords, wordOffsetInRegister, flags);
  }

} // end namespace ChimeraTK

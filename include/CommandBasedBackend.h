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

  class CommandBasedBackend : public DeviceBackendImpl {
    /******************************************************************************************************************/
   public:
    enum class CommandBasedBackendType { SERIAL, ETHERNET };

    CommandBasedBackend(
        CommandBasedBackendType type, std::string instance, std::map<std::string, std::string> parameters);

    ~CommandBasedBackend() override = default;

    void open() override;
    void close() override;

    /** Send a single command through and receive response. */
    std::string sendCommand(std::string cmd);

    /** Send a single command through and receive a vector (len nLinesExpected) responses. */
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
    CommandBasedBackendType _commandBasedBackendType;

    ///  The device node for serial communication, or the host name for network communication
    std::string _instance;

    // parameters for network based communication
    std::string _port;

    ulong _timeoutInMilliseconds = 2000;
    // avoid 1000 due to race conditions with a 1000ms timeout on CommandBasedBackendRegisterAccessor::doReadTransferSynchronously

    /// mutex for protecting ordered port access
    std::mutex _mux;
    std::unique_ptr<CommandHandler> _commandHandler;
    BackendRegisterCatalogue<CommandBasedBackendRegisterInfo> _backendCatalogue;

    ChimeraTK::RegisterPath _defaultRecoveryRegister; // used if last accessed register is write only or at first open()
    /** The last register that war attempted to be written. Might have failed and is re-tried on open. */
    ChimeraTK::RegisterPath _lastWrittenRegister;

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

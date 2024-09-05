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
    enum class CommandBasedBackendType { SERIAL, ETHERNET } _commandBasedBackendType;

    // Constructor for Serial communication
    CommandBasedBackend(std::string serialDevice);

    ~CommandBasedBackend() override = default;

    void open() override;
    void close() override;

    /** Send a single command through and receive response. */
    std::string sendCommand(std::string cmd);

    /** Send a single command through and receive a vector (len nLinesExpected) responses. */
    std::vector<std::string> sendCommand(std::string cmd, const size_t nLinesExpected);

    template<typename UserType>
    boost::shared_ptr<NDRegisterAccessor<UserType>> getRegisterAccessor_impl(
        const RegisterPath& registerPathName, size_t numberOfWords, size_t wordOffsetInRegister, AccessModeFlags flags);

    static boost::shared_ptr<DeviceBackend> createInstance(
        std::string instance, std::map<std::string, std::string> parameters);

    struct BackendRegisterer {
      BackendRegisterer();
    };

    RegisterCatalogue getRegisterCatalogue() const override;

    std::string readDeviceInfo() override;

    /*----------------------------------------------------------------------------------------------------------------*/

   protected:
    std::string _device = "";
    ulong _timeoutInMilliseconds = 2000;
    // avoid 1000 due to race conditions with a 1000ms timeout on CommandBasedBackendRegisterAccessor::doReadTransferSynchronously

    /// mutex for protecting ordered port access
    std::mutex _mux;
    std::unique_ptr<CommandHandler> _commandHandler;
    BackendRegisterCatalogue<CommandBasedBackendRegisterInfo> _backendCatalogue;
  }; // end class CommandBasedBackend

  /*****************************************************************************************************************/
  /*****************************************************************************************************************/
  /*****************************************************************************************************************/

  template<typename UserType>
  boost::shared_ptr<NDRegisterAccessor<UserType>> CommandBasedBackend::getRegisterAccessor_impl(
      const RegisterPath& registerPathName, size_t numberOfWords, size_t wordOffsetInRegister, AccessModeFlags flags) {
    auto registerInfo = _backendCatalogue.getBackendRegister(registerPathName);

    return boost::make_shared<CommandBasedBackendRegisterAccessor<UserType>>(
        DeviceBackend::shared_from_this(), registerInfo, registerPathName, numberOfWords, wordOffsetInRegister, flags);
  }

} // end namespace ChimeraTK

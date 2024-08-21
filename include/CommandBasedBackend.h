// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "CommandBasedBackendRegisterInfo.h"
#include "CommandHandler.h"
#include "SerialCommandHandler.h"

#include <ChimeraTK/AccessMode.h>
#include <ChimeraTK/BackendFactory.h>
#include <ChimeraTK/BackendRegisterCatalogue.h>
#include <ChimeraTK/DeviceAccessVersion.h>
#include <ChimeraTK/DeviceBackendImpl.h>

#include <boost/make_shared.hpp>

#include <memory>
#include <mutex>

namespace ChimeraTK {

  template<typename UserType>
  class CommandBasedBackendRegisterAccessor;

  class CommandBasedBackend : public DeviceBackendImpl {
    /******************************************************************************************************************/
   public:
    enum class CommandBasedBackendType { SERIAL, ETHERNET } _commandBasedBackendType;

    // Constructor for Serial communication
    CommandBasedBackend(std::string serialDevice);

    // Constructor for Ethernet network communication
    CommandBasedBackend();

    ~CommandBasedBackend() override = default;

    /** Open the device */
    void open();

    /** Close the device */
    void close() override {
      _commandHandler.reset();
      _opened.store(false);
    }

    /** Send a single command through and receive response. */
    std::string sendCommand(std::string cmd);

    /** Send a single command through and receive a vector (len nLinesExpected) responses. */
    std::vector<std::string> sendCommand(std::string cmd, const size_t nLinesExpected);

    /** Get a NDRegisterAccessor object from the register name. */
    // boost::shared_ptr<NDRegisterAccessor<UserType>> getRegisterAccessor(
    // template<typename UserType>
    // boost::shared_ptr<NDRegisterAccessor<UserType>> getRegisterAccessor_impl(
    // CommandBasedBackendRegisterInfo& registerInfo, const RegisterPath& registerPathName, AccessModeFlags flags) {

    template<typename UserType>
    boost::shared_ptr<NDRegisterAccessor<UserType>> getRegisterAccessor_impl(const RegisterPath& registerPathName,
        size_t numberOfWords, size_t wordOffsetInRegister, AccessModeFlags flags) {
      // TODO look into the catalogue.
      if(registerPathName != "/cwFrequency") {
        throw ChimeraTK::logic_error("unknown register " + registerPathName);
      }
      DataDescriptor dataDescriptor(DataDescriptor::FundamentalType::numeric, true, true, 11);
      // DataDescriptor(FundamentalType fundamentalType_, bool isIntegral_ = false, bool isSigned_ = false,
      //  size_t nDigits_ = 0, size_t nFractionalDigits_ = 0, DataType rawDataType_ = DataType::none,
      //  DataType transportLayerDataType_ = DataType::none)
      auto registerInfo = CommandBasedBackendRegisterInfo("SOUR:FREQ:CW", registerPathName, 1, 1,
          CommandBasedBackendRegisterInfo::IoDirection::READ_WRITE, {}, dataDescriptor); // placeholder catalog info

      return boost::make_shared<CommandBasedBackendRegisterAccessor<UserType>>(DeviceBackend::shared_from_this(),
          registerInfo, registerPathName, numberOfWords, wordOffsetInRegister, flags);
    }

    static boost::shared_ptr<DeviceBackend> createInstance(
        std::string instance, [[maybe_unused]] std::map<std::string, std::string> parameters) {
      return boost::make_shared<CommandBasedBackend>(instance);
    }

    struct BackendRegisterer {
      BackendRegisterer() {
        BackendFactory::getInstance().registerBackendType(
            "CommandBasedTTY", &CommandBasedBackend::createInstance, {}, CHIMERATK_DEVICEACCESS_VERSION);
      }
    };

    /**
     *  Return the register catalogue with detailed information on all registers.
     *  for now this returns an empty RegisterCatalog
     */
    RegisterCatalogue getRegisterCatalogue() const override {
      return RegisterCatalogue(_registerMap->clone()); // TODO
      // return RegisterCatalogue(nullptr);//_registerMap->clone()); //TODO
      // Trying to call
      //  RegisterCatalogue(std::unique_ptr<BackendRegisterCatalogueBase>&& impl);
      // std::unique_ptr<BackendRegisterCatalogue<CommandBasedBackendRegisterInfo>> _registerMap;
      // expect to use copy constructor

      // CommandBAsedBackendRegisterInfo line 68:
      /* std::unique_ptr<BackendRegisterInfoBase> clone() const override {
                return std::make_unique<CommandBasedBackendRegisterInfo>(*this);
            }
      */
    }

    /**
     *  Return the device metadata catalogue
     *  Placeholder: return an empty one.
     */
    // MetadataCatalogue getMetadataCatalogue() const override { return {}; }

    /** Get a NDRegisterAccessor object from the register name. */
    /*template<typename UserType>
    boost::shared_ptr<NDRegisterAccessor<UserType>> getRegisterAccessor(
        const RegisterPath& registerPathName, size_t numberOfWords, size_t wordOffsetInRegister, AccessModeFlags flags);
        */

    /** Return a device information string containing hardware details like the
     * firmware version number or the slot number used by the board. The format
     * and contained information of this string is completely backend
     * implementation dependent, so the string may only be printed to the user as
     * an informational output. Do not try to parse this string or extract
     * information from it programmatically. */
    std::string readDeviceInfo() override {
      return "Device: " + _device + " timeout: " + std::to_string(_timeoutInMilliseconds);
    }

    void activateAsyncRead() noexcept override {} // Martin: leave empty.

    /*----------------------------------------------------------------------------------------------------------------*/

   protected:
    std::string _device = "";
    ulong _timeoutInMilliseconds = 2000;
    // avoid 1000 due to race conditions with a 1000ms timeout on CommandBasedBackendRegisterAccessor::doReadTransferSynchronously

    /// mutex for protecting ordered port access
    std::mutex _mux;
    std::unique_ptr<CommandHandler> _commandHandler;
    std::unique_ptr<BackendRegisterCatalogue<CommandBasedBackendRegisterInfo>> _registerMap;
  }; // end class CommandBasedBackend

} // end namespace ChimeraTK

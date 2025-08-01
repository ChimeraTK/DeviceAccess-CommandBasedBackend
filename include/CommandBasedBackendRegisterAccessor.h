// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "CommandBasedBackendRegisterInfo.h"

#include <ChimeraTK/AccessMode.h>
#include <ChimeraTK/BackendRegisterCatalogue.h>
#include <ChimeraTK/NDRegisterAccessor.h>
#include <ChimeraTK/RegisterPath.h>

#include <functional>
#include <memory>
#include <regex>

namespace ChimeraTK {

  template<typename UserType>
  using ToUserTypeFunc =
      std::function<UserType(const std::string&, const CommandBasedBackendRegisterInfo::InteractionInfo&)>;

  template<typename UserType>
  using ToTransportLayerFunc =
      std::function<std::string(const UserType&, const CommandBasedBackendRegisterInfo::InteractionInfo&)>;

  class CommandBasedBackend;

  /********************************************************************************************************************/
  /**
   * Implementation of the NDRegisterAccessor for CommandBasedBackend for scalar and 1D registers.
   */
  template<typename UserType> //, DataConverterType>
  class CommandBasedBackendRegisterAccessor : public NDRegisterAccessor<UserType> {
   public:
    CommandBasedBackendRegisterAccessor(const boost::shared_ptr<ChimeraTK::DeviceBackend>& dev,
        CommandBasedBackendRegisterInfo& registerInfo, const RegisterPath& registerPathName, size_t numberOfElements,
        size_t elementOffsetInRegister, AccessModeFlags flags, bool isRecoveryTestAccessor = false);

    // Overridden functions should use Impl version if they may ever be called from the constructor.
    [[nodiscard]] bool isReadOnly() const override { return isReadOnlyImpl(); }

    [[nodiscard]] bool isReadable() const override { return isReadableImpl(); }

    [[nodiscard]] bool isWriteable() const override { return isWriteableImpl(); }

    std::vector<boost::shared_ptr<TransferElement>> getHardwareAccessingElements() override {
      return {TransferElement::shared_from_this()}; // Returns a shared pointer to `this`
    }

    std::list<boost::shared_ptr<TransferElement>> getInternalElements() override {
      // Return an empty list.
      return {};
    }

    void replaceTransferElement([[maybe_unused]] boost::shared_ptr<TransferElement> newElement) override {
    } // mayReplaceTransferElement = false, so this is just empty.

    /*----------------------------------------------------------------------------------------------------------------*/
   protected:
    using NDRegisterAccessor<UserType>::buffer_2D;

    size_t _numberOfElements;
    size_t _elementOffsetInRegister;
    CommandBasedBackendRegisterInfo _registerInfo;

    [[nodiscard]] bool isReadOnlyImpl() const { return _registerInfo.isReadable() && !isWriteable(); }
    [[nodiscard]] bool isReadableImpl() const { return _registerInfo.isReadable(); }
    [[nodiscard]] bool isWriteableImpl() const { return _registerInfo.isWriteable(); }

    /**
     *  Flag whether the accessor is used internally to test functionality during open().
     *  Setting this flag bypasses the tests whether the backend is opened or functional
     *  when reading.
     *
     *  Attention: An accessor with this flag turned on does not comply to the specification
     *  and must not be given out!
     */
    bool _isRecoveryTestAccessor;

    /** the backend to use for the actual hardware access */
    boost::shared_ptr<CommandBasedBackend> _backend;

    std::vector<std::string> _readTransferBuffer;
    std::string _writeTransferBuffer;

    ToTransportLayerFunc<UserType> _transportLayerTypeFromUserType;
    ToUserTypeFunc<UserType> _userTypeFromTransportLayerType;

    void doPreRead([[maybe_unused]] TransferType) override;

    void doPostRead([[maybe_unused]] TransferType t, bool updateDataBuffer) override;

    void doPreWrite([[maybe_unused]] TransferType, [[maybe_unused]] VersionNumber) override;

    bool doWriteTransfer([[maybe_unused]] ChimeraTK::VersionNumber versionNumber) override;

    void doReadTransferSynchronously() override;

    std::regex _readResponseRegex;

  }; // end class CommandBasedBackendRegisterAccessor

  DECLARE_TEMPLATE_FOR_CHIMERATK_USER_TYPES(CommandBasedBackendRegisterAccessor);
} // namespace ChimeraTK

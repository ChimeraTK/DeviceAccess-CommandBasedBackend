// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include <memory>
//#include "createDataConverter.h"
//#include "FixedPointConverter.h"
//#include "ForwardDeclarations.h"
#include <ChimeraTK/AccessMode.h>
//#include "BackendRegisterInfoBase.h"
//#include "CommandHandler.h"
//#include "DataDescriptor.h"
//#include "IEEE754_SingleConverter.h"
#include <ChimeraTK/NDRegisterAccessor.h>
//#include "NumericAddressedLowLevelTransferElement.h"
//#include <ChimeraTK/cppext/finally.hpp>
#include "CommandBasedBackend.h" //TODO move to cpp file
#include "CommandBasedBackendRegisterInfo.h"

#include <ChimeraTK/BackendRegisterCatalogue.h>
#include <ChimeraTK/RegisterPath.h>

namespace ChimeraTK {

  class CommandBasedBackend;

  /**
   * Implementation of the NDRegisterAccessor for CommandBasedBackend for scalar and 1D registers.
   */
  template<typename UserType> //, DataConverterType>
  class CommandBasedBackendRegisterAccessor : public NDRegisterAccessor<UserType> {
   public:
    CommandBasedBackendRegisterAccessor(const boost::shared_ptr<ChimeraTK::DeviceBackend>& dev,
        CommandBasedBackendRegisterInfo& registerInfo, const RegisterPath& registerPathName, size_t numberOfWords,
        size_t wordOffsetInRegister, AccessModeFlags flags);

    [[nodiscard]] bool isReadOnly() const override { return _registerInfo.isReadOnly(); }

    [[nodiscard]] bool isReadable() const override { return _registerInfo.isReadable(); }

    [[nodiscard]] bool isWriteable() const override { return _registerInfo.isWriteable(); }

    /*void setExceptionBackend(boost::shared_ptr<DeviceBackend> exceptionBackend) override {
      _exceptionBackend = std::move(exceptionBackend);
    }*/     //TODO is it ok to just use the parent class implementation here? It looks perfectly good.

    /*virtual bool mayReplaceOther(const boost::shared_ptr<TransferElement const>& other) const {
      (void)other; // prevent warning
      return false;
    }*/

    /**
     *  Obtain the underlying TransferElements with actual hardware access. If
     * this transfer element is directly reading from / writing to the hardware,
     * it will return a list just containing a shared pointer of itself.
     *
     *  Note: Avoid using this in application code, since it will break the
     * abstraction!
     */
    std::vector<boost::shared_ptr<TransferElement>> getHardwareAccessingElements() override {
      return {TransferElement::shared_from_this()}; // Returns a shared pointer to `this`
    }

    /**
     *  Obtain the full list of TransferElements internally used by this
     * TransferElement. The function is recursive, i.e. elements used by the
     * elements returned by this function are also added to the list. It is
     * guaranteed that the directly used elements are first in the list and the
     * result from recursion is appended to the list.
     *
     *  Example: A decorator would return a list with its target TransferElement
     * followed by the result of getInternalElements() called on its target
     * TransferElement.
     *
     *  If this TransferElement is not using any other element, it should return
     * an empty vector. Thus those elements which return a list just containing
     * themselves in getHardwareAccessingElements() will return an empty list here
     * in getInternalElements().
     *
     *  Note: Avoid using this in application code, since it will break the
     * abstraction!
     */
    std::list<boost::shared_ptr<TransferElement>> getInternalElements() override {
      // return and empty list.
      return std::list<boost::shared_ptr<TransferElement>>();
    }

    virtual void replaceTransferElement([[maybe_unused]] boost::shared_ptr<TransferElement> newElement) override {
    } // mayReplaceTransferElement = false, so this is just empty.

    /*----------------------------------------------------------------------------------------------------------------*/
   protected:
    /** Address, size and fixed-point representation information of the register
     * from the map file */
    // NumericAddressedRegisterInfo _registerInfo;
    using NDRegisterAccessor<UserType>::buffer_2D;

    size_t _numberOfElements;
    size_t _elementOffsetInRegister;
    CommandBasedBackendRegisterInfo _registerInfo;

    /** the backend to use for the actual hardware access */
    boost::shared_ptr<CommandBasedBackend> _dev; // a mixture of the two: _rawAccessor and _dev.

    std::vector<std::string> readTransferBuffer;
    std::string writeTransferBuffer;

    /** Backend specific implementation of preRead(). preRead() will call this function, but it will make sure that
     *  it gets called only once per transfer.
     *
     *  No actual communication may be done. Hence, no runtime_error exception may be thrown by this function. Also it
     *  must be acceptable to call this function while the device is closed or not functional (see isFunctional()) and
     *  no exception is thrown. */
    void doPreRead([[maybe_unused]] TransferType) override;

    /** Backend specific implementation of postRead(). postRead() will call this function, but it will make sure that
     *  it gets called only once per transfer.
     *
     *  No actual communication may be done. Hence, no runtime_error exception may be thrown by this function. Also it
     *  must be acceptable to call this function while the device is closed or not functional (see isFunctional()) and
     *  no exception is thrown.
     *
     *  Notes for backend implementations:
     *  - If the flag updateDataBuffer is false, the data buffer must stay unaltered. Full implementations (backends)
     * must also leave the meta data (version number and data validity) unchanged. Decorators are allowed to change the
     * meta data (for instance set the DataValidity::faulty).
     */
    void doPostRead([[maybe_unused]] TransferType t, bool updateDataBuffer) override;

    /** Backend specific implementation of preWrite(). preWrite() will call this function, but it will make sure that
     *  it gets called only once per transfer.
     *
     *  No actual communication may be done. Hence, no runtime_error exception may be thrown by this function. Also it
     *  must be acceptable to call this function while the device is closed or not functional (see isFunctional()) and
     *  no exception is thrown. */
    void doPreWrite([[maybe_unused]] TransferType, [[maybe_unused]] VersionNumber) override;
    // Martin: Swapps user buffer with the buffer sent to hardware
    // Takes data coming in, a number, put it into a string format. Here we prepare the write command that will get sent. TODO

    /** Backend specific implementation of postWrite(). postWrite() will call this function, but it will make sure that
     *  it gets called only once per transfer.
     *
     *  No actual communication may be done. Hence, no runtime_error exception may be thrown by this function. Also it
     *  must be acceptable to call this function while the device is closed or not functional (see isFunctional()) and
     *  no exception is thrown. */
    // virtual void doPostWrite(TransferType, VersionNumber) {} //Martin: this is empty for us.

    /**
     *  Implementation version of writeTransfer(). This function must be implemented by the backend. For the
     *  functional description read the documentation of writeTransfer().
     *
     *  Implementation notes:
     *  - Decorators must delegate the call to writeTransfer() of the decorated target.
     *
     *  **From TransferElement::writeTransfer()**
     *  Write the data to the device. This function must be called after preWrite() and before postWrite(). If the
     *  return value is true, old data was lost on the write transfer (e.g. due to an buffer overflow). In case of an
     *  unbuffered write transfer, the return value will always be false.
     *
     *  This function internally calls doWriteTransfer(), which is implemented by the backend. runtime_error exceptions
     *  thrown in doWriteTransfer() are caught and rethrown in postWrite().
     */
    bool doWriteTransfer([[maybe_unused]] ChimeraTK::VersionNumber versionNumber) override;

    /**
     *  Implementation version of readTransfer() for synchronous reads. This function must be implemented by the
     * backend. For the functional description read the documentation of readTransfer().
     *
     *  Implementation notes:
     *  - This function must return within ~1 second after boost::thread::interrupt() has been called on the thread
     *    calling this function.
     *  - Decorators must delegate the call to readTransfer() of the decorated target.
     *  - Delegations within the same object should go to the "do" version, e.g. to
     * BaseClass::doReadTransferSynchronously()
     */
    void doReadTransferSynchronously() override;

  }; // end class CommandBasedBackendRegisterAccessor

  DECLARE_TEMPLATE_FOR_CHIMERATK_USER_TYPES(CommandBasedBackendRegisterAccessor);
} // namespace ChimeraTK

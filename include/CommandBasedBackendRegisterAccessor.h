// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

//#include "createDataConverter.h"
//#include "FixedPointConverter.h"
//#include "ForwardDeclarations.h"
#include "AccessMode.h"
#include "BackendRegisterInfoBase.h"
#include "CommandHandler.h"
#include "DataDescriptor.h"
#include "IEEE754_SingleConverter.h"
#include "NDRegisterAccessor.h"
#include "NumericAddressedLowLevelTransferElement.h"

#include <mutex>

//#include <ChimeraTK/cppext/finally.hpp>
//

namespace ChimeraTK {
  template<typename UserType> //, typename DataConverterType>
  class CommandBasedBackendRegisterAccessor;

  /**
  Holds info all about the command you're sending, but about the device you're sending it to.
   */
  class CommandBasedBackendRegisterInfo : public BackendRegisterInfoBase {
   public:
    CommandBasedBackendRegisterInfo() {} // end constructor
    CommandBasedBackendRegisterInfo(std::string commandName, RegisterPath registerPath, uint nChannels, uint nElements,
        IoDirection ioDir, AccessModeFlags accessModeFlags, const DataDescriptor& dataDescriptor)
    : _nChannels(nChannels), _nElements(nElements), _ioDir(ioDir), _accessModeFlags(accessModeFlags),
      _registerPath(registerPath), _commandName(commandName), _dataDescriptor(dataDescriptor) {}

    // Copy constructor
    CommandBasedBackendRegisterInfo(const CommandBasedBackendRegisterInfo& other)
    : _nChannels(other._nChannels), _nElements(other._nElements), _ioDir(other._ioDir),
      _accessModeFlags(other._accessModeFlags), _registerPath(other._registerPath), _commandName(other._commandName),
      _dataDescriptor(other._dataDescriptor) {}

    ~CommandBasedBackendRegisterInfo() override = default;

    /** Return full path name of the register (including modules) */
    [[nodiscard]] virtual RegisterPath getRegisterName() const override { return _registerPath; }
    // The name identified in the DA interface.
    // coming in through the constructor.

    /** Return number of elements per channel */
    [[nodiscard]] unsigned int getNumberOfElements() override { return _nElements; }

    /** Return number of channels in register */
    [[nodiscard]] unsigned int getNumberOfChannels() override { return _nChannels; }

    /** Return number of dimensions of this register */
    [[nodiscard]] unsigned int getNumberOfDimensions() override { return getNumberOfDimensions(); }

    /** Return desciption of the actual payload data for this register. See the
     * description of DataDescriptor for more information. */
    [[nodiscard]] const DataDescriptor& getDataDescriptor() const override { return _dataDescriptor; }

    /** Return whether the register is readable. */
    bool isReadable() override { return _ioDir == READ_ONLY or _ioDir == READ_WRITE; }

    /** Return whether the register is writeable. */
    bool isWriteable() override { return _ioDir == WRITE_ONLY or _ioDir == READ_WRITE; }

    bool isReadOnly() { return _ioDir == READ_ONLY; } // not in interface.

    /** Return all supported AccessModes for this register */
    [[nodiscard]] AccessModeFlags getSupportedAccessModes() override { return _accessModeFlags; }

    /** Create copy of the object */
    [[nodiscard]] std::unique_ptr<CommandBasedBackendRegisterInfo> clone() const override {
      return std::make_unique<CommandBasedBackendRegisterInfo>(*this);
    }

    enum class IoDirection { // TODO surely there's something like this already
      READ_ONLY,
      WRITE_ONLY,
      READ_WRITE
    };

   protected:
    uint _nChannels = 1;
    uint _nElements = 1;
    IoDirection _ioDir; // RW flags
    AccessModeFlags _accessModeFlags;
    RegisterPath _registerPath;

    /**The SCPI base command.
     */
    std::string _commandName;

    const DataDescriptor _dataDescriptor;
  }; // end CommandBasedBackendRegisterInfo

  /********************************************************************************************************************/
  // backend: inheriting from DeviceBackendImpl -- mostly implementing virtual things from DeviceBackend.
  // as you put the backendRegister

  class CommandBasedBackend : public DeviceBackendImpl {
    //***************** Virtual items from DeviceBackend ******************
   public:
    CommandBasedBackend(std::string serialDevice) : _device(serialDevice) {
      // Creates the CommandBasedRegisterAccessor
    }

    ~CommandBasedBackend() override = default;

    /** Open the device */
    void open() {
      // Martin: instantiate CommandHandler here.
      // make it a unique pointer
      // here set make_unique. if it throws, catch ChimeraTK::runtime_error and setException.
      // else its ok. setOpenAndClearException

      // TODO open the device.
      if(ok) {
        setOpenedAndClearException();
      }
      else {
        setException("Error opening CommandBased Backend for " + _device)
      }
      // Backends should call this function at the end of a (successful) open() call
    }

    /** Close the device */
    void close() override {
      // TODO close the device.
      _opened.store(false);
    }

    std::string sendCommand(std::string cmd) {
      _mux.lock();
      std::string result = _dev->sendCommand(cmd);
      _mux.unlock();
      return result;
    }

    std::vector<std::string> sendCommand(std::string cmd, const size_t nLinesExpected) {
      _mux.lock();
      std::vector<std::string> result = _dev->sendCommand(cmd, nLinesExpected);
      _mux.unlock();
      return result;
    }

    /** Get a NDRegisterAccessor object from the register name. */
    template<typename UserType>
    boost::shared_ptr<CommandBasedBackendRegisterAccessor<UserType>> getRegisterAccessor(
        CommandBasedBackendRegisterInfo& registerInfo, const RegisterPath& registerPathName,
        AccessModeFlags flags) override {
      return boost::shared_ptr<CommandBasedBackendRegisterAccessor<UserType>>(
          new CommandBasedBackendRegisterAccessor<UserType>(
              DeviceBackend::shared_from_this(), registerInfo registerPathName, flags));
    }

    /**
     *  Return the register catalogue with detailed information on all registers.
     */
    RegisterCatalogue getRegisterCatalogue() const override { // TODO
      // return RegisterCatalogue(_registerMap.clone()); //for now return an empty RegisterCatalog
    }

    /**
     *  Return the device metadata catalogue
     */
    MetadataCatalogue getMetadataCatalogue() const override { // TODO
      // return _metadataCatalogue; //TODO: for now return an empty metadata catalog
    }

    /** Get a NDRegisterAccessor object from the register name. */
    /*template<typename UserType>
    boost::shared_ptr<NDRegisterAccessor<UserType>> getRegisterAccessor(
        const RegisterPath& registerPathName, size_t numberOfWords, size_t wordOffsetInRegister, AccessModeFlags flags);

    DEFINE_VIRTUAL_FUNCTION_TEMPLATE_VTABLE(getRegisterAccessor_impl,
        boost::shared_ptr<NDRegisterAccessor<T>>(const RegisterPath&, size_t, size_t, AccessModeFlags));*/

    /** Return a device information string containing hardware details like the
     * firmware version number or the slot number used by the board. The format
     * and contained information of this string is completely backend
     * implementation dependent, so the string may only be printed to the user as
     * an informational output. Do not try to parse this string or extract
     * information from it programmatically. */
    std::string readDeviceInfo() override {
      return "Device: " + _device + " timeout: " + std::string(_timeoutInMilliseconds);
    }

    void activateAsyncRead() noexcept override {} // Martin: leave empty.

    //--------------------------------------------------------------------------------------------------------------------
    /*virtual void setExceptionImpl() noexcept {
      _asyncIsActive = false;//TODO done in NumericAddressedBackend
    } */

   protected:
    std::string _device;
    ulong _timeoutInMilliseconds;

    /// mutex for protecting ordered port access
    std::mutex _mux;
    unique_ptr<CommandHandler> _dev;
  };
  /********************************************************************************************************************/

  /**
   * Implementation of the NDRegisterAccessor for CommandBasedBackend for scalar and 1D registers.
   */
  template<typename UserType> //, DataConverterType>
  class CommandBasedBackendRegisterAccessor : public NDRegisterAccessor<UserType> {
   public:
    CommandBasedBackendRegisterAccessor(const boost::shared_ptr<DeviceBackend>& dev,
        CommandBasedBackendRegisterInfo& registerInfo, const RegisterPath& registerPathName, AccessModeFlags flags)
    : NDRegisterAccessor<UserType>(registerPathName, flags), _registerInfo(registerInfo),
      //_dataConverter(registerPathName), //TODO what is this?
      _dev(boost::dynamic_pointer_cast<CommandBasedBackend>(
          dev)) { //_dev(boost::dynamic_pointer_cast<NumericAddressedBackend>(dev)) {

      // allocated the buffers
      NDRegisterAccessor<UserType>::buffer_2D.resize(_registerInfo.dimension);
      // NDRegisterAccessor<UserType>::buffer_2D.resize(1);
      NDRegisterAccessor<UserType>::buffer_2D[0].resize(_registerInfo.nElements);

      //---------------
      // check for unknown flags
      flags.checkForUnknownFlags({});

      // check device backend
      //_dev = boost::dynamic_pointer_cast<NumericAddressedBackend>(dev); //Duplicate!
      /*if(!_dev) {
        throw ChimeraTK::logic_error("CommandBasedBackendRegisterAccessor is used with a backend which is not "
                                     "a NumericAddressedBackend.");
      }*/

      // obtain register information
      //_registerInfo = _dev->getRegisterInfo(registerPathName);
      if(_registerInfo.channels <= 0 or _registerInfo.nElements <= 0) {
        throw ChimeraTK::logic_error;
      }

      /*if(_registerInfo.elementPitchBits % 8 != 0) {
        throw ChimeraTK::logic_error("NumericAddressedBackendRegisterAccessor: Elements must be byte aligned.");
      }

      if(_registerInfo.channels.size() > 1) {
        throw ChimeraTK::logic_error("NumericAddressedBackendRegisterAccessor is used with a 2D register.");
      }

      if(_registerInfo.channels.front().bitOffset > 0) {
        throw ChimeraTK::logic_error("NumericAddressedBackendRegisterAccessor: Registers must be byte aligned.");
      }

      // check number of words
      if(_registerInfo.channels.front().dataType == NumericAddressedRegisterInfo::Type::VOID) {
        // in void registers we always create one element
        if(numberOfWords == 0) {
          numberOfWords = 1;
        }
        if(numberOfWords > 1) {
          throw ChimeraTK::logic_error(
              "Requested number of words is larger than 1 in VOID register '" + registerPathName + "'!");
        }
        if(wordOffsetInRegister > 0) {
          throw ChimeraTK::logic_error("No offset allowed in VOID register '" + registerPathName + "'!");
        }
      }
      else { // do the regular consistency check
        if(numberOfWords == 0) {
          numberOfWords = _registerInfo.getNumberOfElements();
        }
        if(numberOfWords + wordOffsetInRegister > _registerInfo.getNumberOfElements()) {
          throw ChimeraTK::logic_error(
              "Requested number of words exceeds the size of the register '" + registerPathName + "'!");
        }
        if(wordOffsetInRegister >= _registerInfo.getNumberOfElements()) {
          throw ChimeraTK::logic_error("Requested offset exceeds the size of the register'" + registerPathName + "'!");
        }
      }*/

      // change registerInfo (local copy!) to account for given offset and length override
      //_registerInfo.address += wordOffsetInRegister * _registerInfo.elementPitchBits / 8;
      //_registerInfo.nElements = numberOfWords;

      //// create low-level transfer element handling the actual data transfer to the hardware with raw data
      // assert(_registerInfo.elementPitchBits % 8 == 0);

      /*_rawAccessor = boost::make_shared<NumericAddressedLowLevelTransferElement>(
          _dev, _registerInfo.bar, _registerInfo.address, _registerInfo.nElements * _registerInfo.elementPitchBits / 8);*/
      // TODO do we need this?? It seems not. We'll also have a hard time passing _dev into this.
      // or else we need to make a real "backend" to instantiate _dev
      // We seem to need this, but it's unclear how to fill it in.

      // allocated the buffers
      NDRegisterAccessor<UserType>::buffer_2D.resize(1);
      NDRegisterAccessor<UserType>::buffer_2D[0].resize(1);
      // NDRegisterAccessor<UserType>::buffer_2D[0].resize(_registerInfo.nElements);

      // We don't have to fill it in a special way if the accessor is raw
      // because we have an overloaded, more efficient implementation
      // in this case. So we can use it in setAsCooked() and getAsCooked()
      //_dataConverter = detail::createDataConverter<DataConverterType>(_registerInfo);
      // TODO

      /*if(flags.has(AccessMode::raw)) {
        if(DataType(typeid(UserType)) != _registerInfo.getDataDescriptor().rawDataType()) {
          throw ChimeraTK::logic_error("Given UserType when obtaining the NumericAddressedBackendRegisterAccessor in "
                                       "raw mode does not match the expected type. Use an " +
              _registerInfo.getDataDescriptor().rawDataType().getAsString() +
              " instead! (Register name: " + registerPathName + "')");
        }
      }*/

      // FILL_VIRTUAL_FUNCTION_TEMPLATE_VTABLE(getAsCooked_impl);
      // FILL_VIRTUAL_FUNCTION_TEMPLATE_VTABLE(setAsCooked_impl);
    }

    [[nodiscard]] bool isReadOnly() const override { return _registerInfo.isReadOnly; }

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

    virtual void replaceTransferElement([[maybe_unused]] boost::shared_ptr<TransferElement> newElement) {
    } // mayReplaceTransferElement = false, so this is just empty.

    /** Create a CopyRegisterDecorator of the right type decorating this
     * TransferElement. This is used by
     *  TransferElementAbstractor::replaceTransferElement() to decouple two
     * accessors which are replaced on the abstractor level. */
    virtual boost::shared_ptr<TransferElement>
        makeCopyRegisterDecorator() = 0; // look at DOOCS backend to see how it's done. TODO

    /*--------------------------------------------------------------------------------------------------------------------------------*/
   protected:
    /** Address, size and fixed-point representation information of the register
     * from the map file */
    // NumericAddressedRegisterInfo _registerInfo;
    CommandBasedBackendRegisterInfo& _registerInfo;

    /** raw accessor */
    // boost::shared_ptr<NumericAddressedLowLevelTransferElement> _rawAccessor; // TODO needed???  // need a mixture of
    // the two: _rawAccessor and _dev.
    //  in a numeric address backend, this is doing the actual transfer. not needed!
    //  this is supposed to be whats implementing doreadtransfer and dowritetransfer.

    /** the backend to use for the actual hardware access */
    // boost::shared_ptr<NumericAddressedBackend> _dev; //Looks like this is what we're replacing with our backend.
    boost::shared_ptr<CommandBasedBackend> _dev;
    // need a mixture of the two: _rawAccessor and _dev.

    std::vector<std::string> readTransferBuffer;
    std::string writeTransferBuffer;

    /** Backend specific implementation of preRead(). preRead() will call this function, but it will make sure that
     *  it gets called only once per transfer.
     *
     *  No actual communication may be done. Hence, no runtime_error exception may be thrown by this function. Also it
     *  must be acceptable to call this function while the device is closed or not functional (see isFunctional()) and
     *  no exception is thrown. */
    // virtual void doPreRead(TransferType) {} //Martin: PreRead is empty.

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
    void doPostRead(TransferType t, bool updateDataBuffer) override {
      // Martin: converts text response to number. Fill it into the user buffer. TODO
      if(updateDataBuffer) {
        // take std::vector<std::string> readTransferBuffer;
        // convert strings to numbers
        // fill them into buffer2D
      }
    }

    /** Backend specific implementation of preWrite(). preWrite() will call this function, but it will make sure that
     *  it gets called only once per transfer.
     *
     *  No actual communication may be done. Hence, no runtime_error exception may be thrown by this function. Also it
     *  must be acceptable to call this function while the device is closed or not functional (see isFunctional()) and
     *  no exception is thrown. */
    virtual void doPreWrite(TransferType, VersionNumber) {
    } // Martin: Swapps user buffer with the buffer sent to hardware
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
     */
    virtual bool doWriteTransfer(ChimeraTK::VersionNumber versionNumber) = 0; // actual write call to the backend

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
    virtual void doReadTransferSynchronously() = 0; // actual read call to backend.
  };

} // namespace ChimeraTK

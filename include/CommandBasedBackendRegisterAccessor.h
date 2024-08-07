// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

//#include "createDataConverter.h"
//#include "FixedPointConverter.h"
//#include "ForwardDeclarations.h"
#include "IEEE754_SingleConverter.h"
#include "NDRegisterAccessor.h"
#include "NumericAddressedLowLevelTransferElement.h"

#include "AccessMode.h"
#include "DataDescriptor.h"
#include "BackendRegisterInfoBase.h"

//#include <ChimeraTK/cppext/finally.hpp>
//

namespace ChimeraTK {
    template<typename UserType, typename DataConverterType, bool isRaw>
    class CommandBasedBackendRegisterAccessor;

  class CommandBasedBackendRegisterInfo: public BackendRegisterInfoBase{
   public:
    CommandBasedBackendRegisterInfo(){

    }//end constructor
    CommandBasedBackendRegisterInfo(
      std::string commandName,
      RegisterPath registerPath,
      uint nChannels,
      uint nElements,
      IoDirection ioDir,
      AccessModeFlags accessModeFlags,
      const DataDescriptor& dataDescriptor) 
        : _nChannels(nChannels), _nElements(nElements), _ioDir(ioDir), _accessModeFlags(accessModeFlags), _registerPath(registerPath), _commandName(commandName),  _dataDescriptor(dataDescriptor) {}

   // Copy constructor
    CommandBasedBackendRegisterInfo(const CommandBasedBackendRegisterInfo& other)
        : _nChannels(other._nChannels), _nElements(other._nElements), _ioDir(other._ioDir), _accessModeFlags(other._accessModeFlags), _registerPath(other._registerPath), _commandName(other._commandName), _dataDescriptor(other._dataDescriptor) {}

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
    [[nodiscard]] unsigned int getNumberOfDimensions() override {return getNumberOfDimensions(); }

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
    [[nodiscard]] std::unique_ptr<CommandBasedBackendRegisterInfo> clone() const override{
      return std::make_unique<CommandBasedBackendRegisterInfo>(*this);
    }

    enum class IoDirection { //TODO surely there's something like this already
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
     std::string _commandName;
     const DataDescriptor _dataDescriptor;
  }; //end CommandBasedBackendRegisterInfo


  /********************************************************************************************************************/
  //backend: inheriting from DeviceBackendImpl -- mostly implementing virtual things from DeviceBackend. 
  //as you put the backendRegister

  /********************************************************************************************************************/

  /**
   * Implementation of the NDRegisterAccessor for CommandBasedBackend for scalar and 1D registers.
   */
  template<typename UserType, typename DataConverterType, bool isRaw>
  class CommandBasedBackendRegisterAccessor : public NDRegisterAccessor<UserType> {
   public:
       CommandBasedBackendRegisterAccessor(
               const boost::shared_ptr<CommandHandler>& dev,
                
               CommandBasedBackendRegisterInfo& registerInfo,
               //std::string name, //const RegisterPath& registerPathName, 
               //size_t numberOfWords, 
               //size_t wordOffsetInRegister, 
               AccessModeFlags flags)
    : NDRegisterAccessor<UserType>(name, flags),  //todo set name
    _registerInfo(registerInfo),
    //_dataConverter(registerPathName),
    _dev(boost::dynamic_pointer_cast<CommandHandler>(dev)) { //_dev(boost::dynamic_pointer_cast<NumericAddressedBackend>(dev)) {

                // allocated the buffers
          NDRegisterAccessor<UserType>::buffer_2D.resize(_registerInfo.dimension);
          //NDRegisterAccessor<UserType>::buffer_2D.resize(1);
          NDRegisterAccessor<UserType>::buffer_2D[0].resize(_registerInfo.nElements);

          //---------------
      // check for unknown flags
      flags.checkForUnknownFlags({AccessMode::raw});

      // check device backend
      //_dev = boost::dynamic_pointer_cast<NumericAddressedBackend>(dev); //Duplicate!
      /*if(!_dev) {
        throw ChimeraTK::logic_error("CommandBasedBackendRegisterAccessor is used with a backend which is not "
                                     "a NumericAddressedBackend.");
      }*/

      // obtain register information
      //_registerInfo = _dev->getRegisterInfo(registerPathName); 
      assert(_registerInfo.channels > 0 ); //TODO make into throw ChimeraTK::logic_error.
      assert(_registerInfo.nElements > 0 );

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
      //assert(_registerInfo.elementPitchBits % 8 == 0);

      _rawAccessor = boost::make_shared<NumericAddressedLowLevelTransferElement>( _dev, _registerInfo.bar, _registerInfo.address, _registerInfo.nElements * _registerInfo.elementPitchBits / 8);
      //TODO do we need this?? It seems not. We'll also have a hard time passing _dev into this.
      //or else we need to make a real "backend" to instantiate _dev
      //We seem to need this, but it's unclear how to fill it in. 

      // allocated the buffers
      NDRegisterAccessor<UserType>::buffer_2D.resize(1);
      NDRegisterAccessor<UserType>::buffer_2D[0].resize(1);
      //NDRegisterAccessor<UserType>::buffer_2D[0].resize(_registerInfo.nElements);

      // We don't have to fill it in a special way if the accessor is raw
      // because we have an overloaded, more efficient implementation
      // in this case. So we can use it in setAsCooked() and getAsCooked()
      _dataConverter = detail::createDataConverter<DataConverterType>(_registerInfo);
      //TODO

      /*if(flags.has(AccessMode::raw)) {
        if(DataType(typeid(UserType)) != _registerInfo.getDataDescriptor().rawDataType()) {
          throw ChimeraTK::logic_error("Given UserType when obtaining the NumericAddressedBackendRegisterAccessor in "
                                       "raw mode does not match the expected type. Use an " +
              _registerInfo.getDataDescriptor().rawDataType().getAsString() +
              " instead! (Register name: " + registerPathName + "')");
        }
      }*/

      //FILL_VIRTUAL_FUNCTION_TEMPLATE_VTABLE(getAsCooked_impl);
      //FILL_VIRTUAL_FUNCTION_TEMPLATE_VTABLE(setAsCooked_impl);
    }

    [[nodiscard]] bool isReadOnly() const override { return _registerInfo.isReadOnly; }

    [[nodiscard]] bool isReadable() const override { return _registerInfo.isReadable(); }

    [[nodiscard]] bool isWriteable() const override { return _registerInfo.isWriteable(); }

   protected:
    /** Address, size and fixed-point representation information of the register
     * from the map file */
    //NumericAddressedRegisterInfo _registerInfo; 
    CommandBasedBackendRegisterInfo& _registerInfo;

    /** raw accessor */
    boost::shared_ptr<NumericAddressedLowLevelTransferElement> _rawAccessor; //TODO needed???
    //in a numeric address backend, this is doing the actual transfer. not needed! 
    //this is supposed to be whats implementing doreadtransfer and dowritetransfer.

    /** the backend to use for the actual hardware access */
    //boost::shared_ptr<NumericAddressedBackend> _dev; //Looks like this is what we're replacing with our backend.
    boost::shared_ptr<CommandHandler> _dev; //no, need a CommandBasedBackend.
    //need a mixture of the two: _rawAccessor and _dev.



    std::vector<boost::shared_ptr<TransferElement>> getHardwareAccessingElements() override {
      return _rawAccessor->getHardwareAccessingElements();
    }

    std::list<boost::shared_ptr<TransferElement>> getInternalElements() override {
        return {_rawAccessor}; // the rawAccessor always returns an empty list
    }
  };

}//end ChimeraTK namespace

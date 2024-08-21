// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
//
#include "CommandBasedBackendRegisterAccessor.h"

#include "CommandBasedBackend.h"
#include "CommandBasedBackendRegisterInfo.h"

namespace ChimeraTK {

  template<typename UserType>

  CommandBasedBackendRegisterAccessor<UserType>::CommandBasedBackendRegisterAccessor(
      const boost::shared_ptr<ChimeraTK::DeviceBackend>& dev, CommandBasedBackendRegisterInfo& registerInfo,
      const RegisterPath& registerPathName, size_t numberOfElements, size_t elementOffsetInRegister,
      AccessModeFlags flags)
  : NDRegisterAccessor<UserType>(registerPathName, flags), _numberOfElements(numberOfElements),
    _elementOffsetInRegister(elementOffsetInRegister), _registerInfo(registerInfo),
    _dev(boost::dynamic_pointer_cast<CommandBasedBackend>(dev)) {
    assert(_registerInfo.getNumberOfChannels() != 0);
    assert(_registerInfo.getNumberOfElements() != 0);
    assert(_registerInfo.getNumberOfDimensions() < 2); // implementation only for scalar and 1D

    // 0 means all elements descrived in registerInfo
    if(_numberOfElements == 0) {
      _numberOfElements = _registerInfo.getNumberOfElements();
    }
    if(_elementOffsetInRegister + _numberOfElements > _registerInfo.getNumberOfElements()) {
      throw ChimeraTK::logic_error("Requested offset + nElemements exceeds register size in " + this->getName());
    }

    flags.checkForUnknownFlags({}); // require no flags.

    if(!_dev) {
      throw ChimeraTK::logic_error("CommandBasedBackendRegisterAccessor is used with a backend which is not "
                                   "a CommandBasedBackend.");
    }
    // You're supposed to be allowed to make accessors before the device is functional. So don't test functionality here.
    /*else if(not _dev->isFunctional()) { // Make sure the backend is open and working
      throw ChimeraTK::runtime_error("Device is not functional when creating CommandBasedBackendRegisterAccessor");
    }*/

    // allocated the buffers
    NDRegisterAccessor<UserType>::buffer_2D.resize(_registerInfo.getNumberOfChannels()); // dimension);
    NDRegisterAccessor<UserType>::buffer_2D[0].resize(_registerInfo.getNumberOfElements());
    readTransferBuffer.resize(1); //_registerInfo.getNumberOfElements());

    //_dataConverter = detail::createDataConverter<DataConverterType>(_registerInfo);
  } // end constructor CommandBasedBackendRegisterAccessor

  /*******************************************************************************************************************/
  template<typename UserType>
  void CommandBasedBackendRegisterAccessor<UserType>::doPreRead([[maybe_unused]] TransferType) {
    if(!_dev->isOpen()) {
      throw ChimeraTK::logic_error("Device not opened.");
    }
    if(_registerInfo.isNotReadable()) {
      throw ChimeraTK::logic_error(
          "NumericAddressedBackend: Writing to a non-writeable register is not allowed (Register name: " +
          _registerInfo.getRegisterName() + ").");
    }
  }

  /*******************************************************************************************************************/
  template<typename UserType>
  void CommandBasedBackendRegisterAccessor<UserType>::doReadTransferSynchronously() {
    if(!_dev->isFunctional()) {
      throw ChimeraTK::runtime_error("Device not functional when reading " + this->getName());
    }

    // actual read call to backend.

    std::string readCommand = _registerInfo.getCommandName() + "?";

    // FIXME: This implementation is not general. Often there is only one line of answer
    // for scalar and 1D.
    switch(_registerInfo.getNumberOfDimensions()) {
      case 0:
        readTransferBuffer[0] = _dev->sendCommand(readCommand);
        break;
      case 1:
        readTransferBuffer = _dev->sendCommand(readCommand, _registerInfo.getNumberOfElements());
        break;
      default:
        throw ChimeraTK::logic_error(
            "Dimension 2. CommandBasedBackendRegisterAccessor is used only with a scalars and 1D register.");
    } // end switch

  } // end doReadTransferSync

  /*******************************************************************************************************************/
  template<typename UserType>
  void CommandBasedBackendRegisterAccessor<UserType>::doPostRead(
      [[maybe_unused]] TransferType t, bool updateDataBuffer) {
    // Martin: converts text response to number. Fill it into the user buffer. TODO
    // TODO make use of UserType
    /*
     *  Transfer type is an enum in TransferElement.h
     *  options are:
     *  TransferType::{read, readNonBlocking, readLatest, write, writeDestructively }
     */
    if(updateDataBuffer) {
      size_t end = _registerInfo.getNumberOfElements();
      if(readTransferBuffer.size() < end) {
        end = readTransferBuffer.size();
      }

      for(size_t i = 0; i < end; i++) {
        buffer_2D[0][i] = userTypeToUserType<UserType, std::string>(readTransferBuffer[i]);
      }
      this->_versionNumber = {};
    }
  } // end doPostRead

  /*******************************************************************************************************************/
  /*******************************************************************************************************************/

  template<typename UserType>
  void CommandBasedBackendRegisterAccessor<UserType>::doPreWrite(
      [[maybe_unused]] TransferType type, [[maybe_unused]] VersionNumber versionNumber) {
    // TODO, what to do with the version number and TransferType?
    // Martin: Swapps user buffer with the buffer sent to hardware
    // TODO make use of UserType
    // Takes data coming in, a number, put it into a string format. Here we prepare the write command that will get
    // sent. We also get a ChimeraTK::VersionNumber

    if(!_dev->isOpen()) throw ChimeraTK::logic_error("Device not opened.");

    if(_registerInfo.isNotWritable()) {
      throw ChimeraTK::logic_error(
          "NumericAddressedBackend: Writing to a non-writeable register is not allowed (Register name: " +
          _registerInfo.getRegisterName() + ").");
    }

    // make a comma seperated list of values out of buffer_2D
    std::ostringstream numericPart;
    numericPart << buffer_2D[0][0];
    for(size_t i = 1; i < _registerInfo.getNumberOfElements(); ++i) {
      numericPart << "," << buffer_2D[0][0];
    }

    // Form the write command.
    writeTransferBuffer = _registerInfo.getCommandName() + " " + numericPart.str();
  } // end doPreWrite

  /*******************************************************************************************************************/

  template<typename UserType>
  bool CommandBasedBackendRegisterAccessor<UserType>::doWriteTransfer(
      [[maybe_unused]] ChimeraTK::VersionNumber versionNumber) {
    if(!_dev->isFunctional()) {
      throw ChimeraTK::runtime_error("Device not functional when reading " + this->getName());
    }

    _dev->sendCommand(writeTransferBuffer, 0);
    return false; // no data was lost
  }

  /*******************************************************************************************************************/

  // Magic from SupportedUserTypes.h
  INSTANTIATE_TEMPLATE_FOR_CHIMERATK_USER_TYPES(CommandBasedBackendRegisterAccessor);

} // end namespace ChimeraTK

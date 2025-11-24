// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "CommandBasedBackendRegisterAccessor.h"

#include "CommandBasedBackend.h"
#include "CommandBasedBackendRegisterInfo.h"
#include "stringUtils.h"

#include <inja/inja.hpp>

#include <regex>
#include <sstream>
#include <string>

namespace ChimeraTK {
  template<typename UserType>

  CommandBasedBackendRegisterAccessor<UserType>::CommandBasedBackendRegisterAccessor(
      const boost::shared_ptr<ChimeraTK::DeviceBackend>& dev, CommandBasedBackendRegisterInfo& registerInfo,
      const RegisterPath& registerPathName, size_t numberOfElements, size_t elementOffsetInRegister,
      AccessModeFlags flags, bool isRecoveryTestAccessor)
  : NDRegisterAccessor<UserType>(registerPathName, flags), _numberOfElements(numberOfElements),
    _elementOffsetInRegister(elementOffsetInRegister), _registerInfo(registerInfo),
    _isRecoveryTestAccessor(isRecoveryTestAccessor), _backend(boost::dynamic_pointer_cast<CommandBasedBackend>(dev)) {
    assert(_registerInfo.getNumberOfChannels() != 0);
    assert(_registerInfo.getNumberOfElements() != 0);
    assert(_registerInfo.getNumberOfDimensions() < 2); // Implementation is only for scalar and 1D.

    // 0 indicates all elements are described in registerInfo.
    if(_numberOfElements == 0) {
      _numberOfElements = _registerInfo.getNumberOfElements();
    }
    if(_elementOffsetInRegister + _numberOfElements > _registerInfo.getNumberOfElements()) {
      throw ChimeraTK::logic_error("Requested offset + nElemements exceeds register size in " + this->getName());
    }

    flags.checkForUnknownFlags({}); // Ensure that the user has not requested any flags, as none are supported.

    if(!_backend) {
      throw ChimeraTK::logic_error("CommandBasedBackendRegisterAccessor is used with a backend which is not "
                                   "a CommandBasedBackend.");
    }
    // You're supposed to be allowed to make accessors before the device is functional. So don't test functionality here.

    // Allocate the buffers
    NDRegisterAccessor<UserType>::buffer_2D.resize(_registerInfo.getNumberOfChannels());
    NDRegisterAccessor<UserType>::buffer_2D[0].resize(_numberOfElements);
    _readTransferBuffer.resize(1);

    this->_exceptionBackend = dev;

    std::string valueRegex;
    if(_registerInfo.internalType == CommandBasedBackendRegisterInfo::InternalType::INT64) {
      valueRegex = "([+-]?[0-9]+)";
    }
    if(_registerInfo.internalType == CommandBasedBackendRegisterInfo::InternalType::UINT64) {
      valueRegex = "([+]?[0-9]+)";
    }
    if(_registerInfo.internalType == CommandBasedBackendRegisterInfo::InternalType::HEX) {
      valueRegex = "([0-9A-Fa-f]+)";
    }
    if(_registerInfo.internalType == CommandBasedBackendRegisterInfo::InternalType::DOUBLE) {
      valueRegex = "([+-]?[0-9]+\\.?[0-9]*)";
    }
    if(_registerInfo.internalType == CommandBasedBackendRegisterInfo::InternalType::STRING) {
      valueRegex = "(.*)";
    }

    if(_registerInfo.internalType != CommandBasedBackendRegisterInfo::InternalType::VOID) {
      assert(!valueRegex.empty());
    }

    if(!_registerInfo.isReadable()) {
      return;
    }

    // Prepare the readResponseRegex.
    inja::json replacePatterns;
    replacePatterns["x"] = {};
    for(size_t i = 0; i < _registerInfo.nElements; ++i) {
      // FIXME: does not know about formating. TODO ticket 13534.
      replacePatterns["x"].push_back(valueRegex);
    }

    try {
      auto regexText = inja::render(_registerInfo.readResponsePattern, replacePatterns);
      _readResponseRegex = regexText;
    }
    catch(std::regex_error& e) {
      throw ChimeraTK::logic_error(
          "Regex error in readResponsePattern of " + _registerInfo.registerPath + ": " + e.what());
    }
    catch(inja::ParserError& e) {
      throw ChimeraTK::logic_error(
          "Inja parser error in readResponsePattern of " + _registerInfo.registerPath + ": " + e.what());
    }
    // Alignment between the mark_count and nElements can be enforced by using non-capture groups: (?:   )
    if(_readResponseRegex.mark_count() != _registerInfo.nElements) {
      throw ChimeraTK::logic_error("Wrong number of capture groups (" +
          std::to_string(_readResponseRegex.mark_count()) + ") in readResponsePattern \"" +
          _registerInfo.readResponsePattern + "\" of " + _registerInfo.registerPath + ", expected " +
          std::to_string(_registerInfo.nElements));
    }

  } // end constructor CommandBasedBackendRegisterAccessor

  /********************************************************************************************************************/
  template<typename UserType>
  void CommandBasedBackendRegisterAccessor<UserType>::doPreRead([[maybe_unused]] TransferType) {
    if(!_backend->isOpen() && !_isRecoveryTestAccessor) {
      throw ChimeraTK::logic_error("Device not opened.");
    }
    if(!_registerInfo.isReadable()) {
      throw ChimeraTK::logic_error(
          "CommandBasedBackend: Commanding read to a non-readable register is not allowed (Register name: " +
          _registerInfo.getRegisterName() + ").");
    }
    _backend->_lastWrittenRegister = _registerInfo.registerPath;
  }

  /********************************************************************************************************************/
  template<typename UserType>
  void CommandBasedBackendRegisterAccessor<UserType>::doReadTransferSynchronously() {
    if(!_backend->isFunctional() && !_isRecoveryTestAccessor) {
      throw ChimeraTK::runtime_error("Device not functional when reading " + this->getName());
    }

    // FIXME: properly create the read command through the template engine //TODO make sure this is done as we think it is
    auto readCommand = _registerInfo.readCommandPattern;

    _readTransferBuffer = _backend->sendCommand(_registerInfo.readCommandPattern, _registerInfo.nLinesReadResponse);
  }

  /********************************************************************************************************************/
  template<typename UserType>
  void CommandBasedBackendRegisterAccessor<UserType>::doPostRead(
      [[maybe_unused]] TransferType t, bool updateDataBuffer) {
    // Transfer type enum options: {read, readNonBlocking, readLatest, write, writeDestructively }

    if(updateDataBuffer) {
      // For technical reasons the response has been read line by line.
      // Combine them here back into a single response string.
      std::string combinedReadString;
      for(const auto& line : _readTransferBuffer) {
        combinedReadString += line + _registerInfo.delimiter;
      }

      std::smatch valueMatch;
      if(!std::regex_match(combinedReadString, valueMatch, _readResponseRegex)) {
        throw ChimeraTK::runtime_error("Could not extract values from \"" + replaceNewLines(combinedReadString) +
            "\" in " + _registerInfo.registerPath);
      }

      std::string hexIndicator =
          (_registerInfo.internalType == CommandBasedBackendRegisterInfo::InternalType::HEX ? "0x" : "");
      for(size_t i = 0; i < _numberOfElements; ++i) {
        buffer_2D[0][i] =
            userTypeToUserType<UserType, std::string>(hexIndicator + valueMatch.str(i + _elementOffsetInRegister + 1));
      }
      this->_versionNumber = {};
      this->_dataValidity = DataValidity::ok;
    }
  } // end doPostRead

  /********************************************************************************************************************/
  /********************************************************************************************************************/

  template<typename UserType>
  void CommandBasedBackendRegisterAccessor<UserType>::doPreWrite(
      [[maybe_unused]] TransferType type, [[maybe_unused]] VersionNumber versionNumber) {
    if(!_backend->isOpen()) {
      throw ChimeraTK::logic_error("Device not opened.");
    }

    if(!_registerInfo.isWriteable()) {
      throw ChimeraTK::logic_error(
          "CommandBasedBackend: Writing to a non-writeable register is not allowed (Register name: " +
          _registerInfo.getRegisterName() + ").");
    }

    inja::json replacePatterns;
    replacePatterns["x"] = {};
    if(_registerInfo.internalType == CommandBasedBackendRegisterInfo::InternalType::HEX) {
      for(size_t i = 0; i < _numberOfElements; ++i) {
        std::ostringstream oss;
        oss << std::hex << userTypeToUserType<uint64_t, UserType>(buffer_2D[0][i]);
        replacePatterns["x"].push_back(oss.str());
      }
    }
    else {
      for(size_t i = 0; i < _numberOfElements; ++i) {
        // FIXME: does not know about formating. TODO ticket 13534.
        // May need leading zeros or other formatting to satisfy the hardware interface.
        replacePatterns["x"].push_back(userTypeToUserType<std::string, UserType>(buffer_2D[0][i]));
      }
    }

    // Form the write command.
    try {
      _writeTransferBuffer = inja::render(_registerInfo.writeCommandPattern, replacePatterns);
    }
    catch(inja::ParserError& e) {
      throw ChimeraTK::logic_error(
          "Inja parser error in writeCommandPattern of " + _registerInfo.registerPath + ": " + e.what());
    }

    // remember this register as the last used one if the register is readable
    if(_registerInfo.isReadable()) {
      _backend->_lastWrittenRegister = _registerInfo.registerPath;
    }
    else { // if not readable use the default read register
      _backend->_lastWrittenRegister = _backend->_defaultRecoveryRegister;
    }
  } // end doPreWrite

  /********************************************************************************************************************/

  template<typename UserType>
  bool CommandBasedBackendRegisterAccessor<UserType>::doWriteTransfer(
      [[maybe_unused]] ChimeraTK::VersionNumber versionNumber) {
    if(!_backend->isFunctional()) {
      throw ChimeraTK::runtime_error("Device not functional when reading " + this->getName());
    }

    _backend->sendCommand(_writeTransferBuffer, 0);
    return false; // no data was lost
  }

  /********************************************************************************************************************/

  template<typename UserType>
  bool CommandBasedBackendRegisterAccessor<UserType>::mayReplaceOther(
      const boost::shared_ptr<TransferElement const>& other) const {
    auto rhsCasted = boost::dynamic_pointer_cast<const CommandBasedBackendRegisterAccessor<UserType>>(other);
    if(!rhsCasted) {
      return false;
    }
    if(rhsCasted.get() == this) {
      return false;
    }
    if(_registerInfo != rhsCasted->_registerInfo) {
      return false;
    }
    if(_numberOfElements != rhsCasted->_numberOfElements) {
      return false;
    }
    if(_elementOffsetInRegister != rhsCasted->_elementOffsetInRegister) {
      return false;
    }
    if(_backend != rhsCasted->_backend) {
      return false;
    }
    if(_isRecoveryTestAccessor != rhsCasted->_isRecoveryTestAccessor) {
      return false;
    }
    if(_registerInfo != rhsCasted->_registerInfo) {
      return false;
    }
    // no need to check the _readResponseRegex. It is generated from the readResponsePattern in the constructor.
    return true;
  }

  /********************************************************************************************************************/

  // Magic from SupportedUserTypes.h
  INSTANTIATE_TEMPLATE_FOR_CHIMERATK_USER_TYPES(CommandBasedBackendRegisterAccessor);

} // end namespace ChimeraTK

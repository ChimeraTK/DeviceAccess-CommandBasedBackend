// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
//
#include "CommandBasedBackendRegisterAccessor.h"

#include "CommandBasedBackend.h"
#include "CommandBasedBackendRegisterInfo.h"
#include <inja/inja.hpp>

#include <regex>
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
    assert(_registerInfo.getNumberOfDimensions() < 2); // implementation only for scalar and 1D

    // 0 means all elements descrived in registerInfo
    if(_numberOfElements == 0) {
      _numberOfElements = _registerInfo.getNumberOfElements();
    }
    if(_elementOffsetInRegister + _numberOfElements > _registerInfo.getNumberOfElements()) {
      throw ChimeraTK::logic_error("Requested offset + nElemements exceeds register size in " + this->getName());
    }

    flags.checkForUnknownFlags({}); // require no flags.

    if(!_backend) {
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

    this->_exceptionBackend = dev;

    std::string valueRegex;
    if(_registerInfo.internalType == CommandBasedBackendRegisterInfo::InternalType::INT64) {
      valueRegex = "([+-]?[0-9]+)";
    }
    if(_registerInfo.internalType == CommandBasedBackendRegisterInfo::InternalType::UINT64) {
      valueRegex = "([+-]?[0-9]+)";
    }
    if(_registerInfo.internalType == CommandBasedBackendRegisterInfo::InternalType::DOUBLE) {
      valueRegex = "([+-]?[0-9]+\\.?[0-9]*)";
    }
    if(_registerInfo.internalType == CommandBasedBackendRegisterInfo::InternalType::STRING) {
      valueRegex = "(.*)";
    }
    assert(!valueRegex.empty());
    std::cout << "!!!DEBUG: valueRegex: " << valueRegex << std::endl;
    std::cout << "!!!DEBUG: pattern: " << _registerInfo.readResponsePattern << std::endl;

    // prepare the readResponseRegex
    inja::json replacePatterns;
    replacePatterns["x"] = {};
    for(size_t i = 0; i < _registerInfo.nElements; ++i) {
      // Fixme: does not know about formating
      replacePatterns["x"].push_back(valueRegex);
    }

    // FIXME: pass this through the template engine
    try {
      auto regexText = inja::render(_registerInfo.readResponsePattern, replacePatterns);
      std::cout << "!!!DEBUG: RegexText: " << regexText << std::endl;
      readResponseRegex = regexText;
    }
    catch(std::regex_error& e) {
      throw ChimeraTK::logic_error(
          "Regex error in readResponsePattern of " + _registerInfo.registerPath + ": " + e.what());
    }
    catch(inja::ParserError& e) {
      throw ChimeraTK::logic_error(
          "Inja parser error in readResponsePattern of " + _registerInfo.registerPath + ": " + e.what());
    }
    // FIXME: checking the mark_count is not reliable. There might be additional
    // capture groups. Possible solution:
    // Replace the {{x.i}} with REPLACEMEi., spit the string at the REPLACEMEs and
    // scan the snippets between for capture groups. Remember the positions and amounts of the
    // capture groups as they will show up in the scan result.
    // Also remember which index is found at which position so you know which
    // subgroup has the value you are looking for. Then re-run
    // inja on the original input and put in the matching regexps.
    if(readResponseRegex.mark_count() != _registerInfo.nElements) {
      throw ChimeraTK::logic_error("Wrong number of capture groups (" + std::to_string(readResponseRegex.mark_count()) +
          ") in readResponsePattern \"" + _registerInfo.readResponsePattern + "\" of " + _registerInfo.registerPath +
          ", expected " + std::to_string(_registerInfo.nElements));
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
          "NumericAddressedBackend: Writing to a non-writeable register is not allowed (Register name: " +
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

    // FIXME: properly create the read command through the template engine
    auto readCommand = _registerInfo.readCommandPattern;

    readTransferBuffer = _backend->sendCommand(_registerInfo.readCommandPattern, _registerInfo.nLinesReadResponse);

  } // end doReadTransferSync

  /********************************************************************************************************************/
  template<typename UserType>
  void CommandBasedBackendRegisterAccessor<UserType>::doPostRead(
      [[maybe_unused]] TransferType t, bool updateDataBuffer) {
    // Martin: converts text response to number. Fill it into the user buffer.
    /*
     *  Transfer type is an enum in TransferElement.h
     *  options are:
     *  TransferType::{read, readNonBlocking, readLatest, write, writeDestructively }
     */
    if(updateDataBuffer) {
      // For technical reasons the response has been read line by line.
      // Combine them back into a single response string.
      std::string combinedReadString;
      for(const auto& line : readTransferBuffer) {
        combinedReadString += line + _registerInfo.delimiter;
      }

      std::smatch valueMatch;
      if(!std::regex_match(combinedReadString, valueMatch, readResponseRegex)) {
        throw ChimeraTK::runtime_error(
            "Could not extract values from \"" + combinedReadString + "\" in " + _registerInfo.registerPath);
      }

      std::cout << "!!!DEBUG: found matches ";
      for(size_t i = 1; i < valueMatch.size(); ++i) {
        std::cout << " \"" << valueMatch[1] << "\"";
      }
      std::cout << " in \"" << combinedReadString << "\"" << std::endl;

      for(size_t i = 0; i < _numberOfElements; ++i) {
        buffer_2D[0][i] = userTypeToUserType<UserType, std::string>(valueMatch[i + 1]);
      }
      this->_versionNumber = {};
    }
  } // end doPostRead

  /********************************************************************************************************************/
  /********************************************************************************************************************/

  template<typename UserType>
  void CommandBasedBackendRegisterAccessor<UserType>::doPreWrite(
      [[maybe_unused]] TransferType type, [[maybe_unused]] VersionNumber versionNumber) {
    if(!_backend->isOpen()) throw ChimeraTK::logic_error("Device not opened.");

    if(!_registerInfo.isWriteable()) {
      throw ChimeraTK::logic_error(
          "NumericAddressedBackend: Writing to a non-writeable register is not allowed (Register name: " +
          _registerInfo.getRegisterName() + ").");
    }

    inja::json replacePatterns;
    replacePatterns["x"] = {};
    for(size_t i = 0; i < _numberOfElements; ++i) {
      // Fixme: does not know about formating
      replacePatterns["x"].push_back(userTypeToUserType<std::string, UserType>(buffer_2D[0][i]));
    }

    // Form the write command.
    try {
      writeTransferBuffer = inja::render(_registerInfo.writeCommandPattern, replacePatterns);
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

    _backend->sendCommand(writeTransferBuffer, 0);
    return false; // no data was lost
  }

  /********************************************************************************************************************/

  // Magic from SupportedUserTypes.h
  INSTANTIATE_TEMPLATE_FOR_CHIMERATK_USER_TYPES(CommandBasedBackendRegisterAccessor);

} // end namespace ChimeraTK

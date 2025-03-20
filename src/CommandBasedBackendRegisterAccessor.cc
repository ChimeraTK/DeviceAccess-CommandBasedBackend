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

using InteractionInfo = CommandBasedBackendRegisterInfo::InteractionInfo;
using TransportLayerType = CommandBasedBackendRegisterInfo::TransportLayerType;

/**
 * @brief: Fetches the appropriate regex given the TransportLayerType, and handles errors
 * This facilitates code reuse once write responces are implemented.
 * @param[in] type The TransportLayerType used to pick the numerical regex segment.
 * @param[in] requiredElements How many elements to check against the regex mark_count
 * @param[in] errorMessageDetial Info useful in the error message, preceeded in error strings by "for ".
 * @throws std::regex_error
 * @throws inja::ParserError
 */
std::regex getRegex(const TransportLayerType type, const size_t requiredElements, const std::string errorMessageDetail);

/** Return the functional for the given TransportLayerType for converting data from the transport layer format to
 * UserType representation*/
template<typename UserType>
ToUserTypeFunc<UserType> getToUserTypeFunction(TransportLayerType transportLayerType);

/** Return the functional for the given TransportLayerType for converting data from the to UserType representation to
 * the transport layer format*/
template<typename UserType>
ToTransportLayerFunc<UserType> getToTransportLayerFunction(TransportLayerType transportLayerType);

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

    if(_registerInfo.isWritable()) {
      _transportLayerTypeFromUserType = getToTransportLayerFunction(_registerInfo.writeInfo.getTransportLayerType());
    }

    if(_registerInfo.isReadable()) {
      _userTypeFromTransportLayerType = getToUserTypeFunction(_registerInfo.readInfo.getTransportLayerType());
      _readResponseRegex = getRegex(_registerInfo.readInfo.getTransportLayerType(),
          _registerInfo.nElements, // TODO Why match to registerInfo::nElements rather than _numberOfElements?
          "read in " + _registerInfo.registerPath);
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

    if(_registerInfo.writeInfo.useReadLines()) {
      auto readLinesInfo = std::get<ReadLinesInfo>(_registerInfo.writeInfo.responceInfo);

      _writeTransferBuffer = _backend->sendCommandAndReadLines(_registerInfo.writeInfo.commandPattern,
          readLinesInfo.nLines, _registerInfo.writeInfo.cmdLineDelimiter, readLinesInfo.delimiter);
    }
    else /*if(_registerInfo.writeInfo.isReadBytes())*/ {
      auto readBytesInfo = std::get<ReadBytesInfo>(_registerInfo.writeInfo.responceInfo);

      _readTransferBuffer[0] = _backend->sendCommandAndReadBytes(_registerInfo.writeInfo.commandPattern,
          readBytesInfo.nBytesReadResponce, _registerInfo.writeInfo.cmdLineDelimiter);
    }
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
        combinedReadString += line + _registerInfo.readDelimiter;
      }

      std::smatch valueMatch;
      if(!std::regex_match(combinedReadString, valueMatch, _readResponseRegex)) {
        throw ChimeraTK::runtime_error("Could not extract values from \"" + replaceNewLines(combinedReadString) +
            "\" in " + _registerInfo.registerPath);
      }

      for(size_t i = 0; i < _numberOfElements; ++i) {
        buffer_2D[0][i] =
            _userTypeFromTransportLayerType(valueMatch.str(i + _elementOffsetInRegister + 1), _registerInfo.readInfo);
      }
      this->_versionNumber = {};
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
    for(size_t i = 0; i < _numberOfElements; ++i) {
      replacePatterns["x"].push_back(_transportLayerTypeFromUserType(buffer_2D[0][i], _registerInfo.writeInfo));
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

    if(_registerInfo.writeInfo.useReadLines()) {
      auto readLinesInfo = std::get<ReadLinesInfo>(_registerInfo.writeInfo.responceInfo);

      _backend->sendCommandAndReadLines(_registerInfo.writeInfo.commandPattern, readLinesInfo.nLines,
          _registerInfo.writeInfo.cmdLineDelimiter, readLinesInfo.delimiter);
    }
    else /*if(_registerInfo.writeInfo.isReadBytes())*/ {
      auto readBytesInfo = std::get<ReadBytesInfo>(_registerInfo.writeInfo.responceInfo);

      _backend->sendCommandAndReadBytes(_registerInfo.writeInfo.commandPattern, readBytesInfo.nBytesReadResponce,
          _registerInfo.writeInfo.cmdLineDelimiter);
    }
    return false; // no data was lost
  }

  /********************************************************************************************************************/

  // Magic from SupportedUserTypes.h
  INSTANTIATE_TEMPLATE_FOR_CHIMERATK_USER_TYPES(CommandBasedBackendRegisterAccessor);
} // end namespace ChimeraTK

/**********************************************************************************************************************/
/**********************************************************************************************************************/

namespace {
  std::regex getRegex(
      const TransportLayerType type, const size_t requiredElements, const std::string errorMessageDetail) {
    std::string valueRegex;
    if(type == TransportLayerType::DEC_INT) {
      valueRegex = "([+-]?[0-9]+)";
    }
    else if(type == TransportLayerType::HEX_INT) {
      valueRegex = "([0-9A-Fa-f]+)";
    }
    else if(type == TransportLayerType::BIN_INT) {
      valueRegex = "(.*)";
    }
    else if(type == TransportLayerType::DEC_FLOAT) {
      valueRegex = "([+-]?[0-9]+\\.?[0-9]*)";
    }
    else if(type == TransportLayerType::STRING) {
      valueRegex = "(.*)";
    }
    else if(type != TransportLayerType::VOID) {
      assert(!valueRegex.empty());
    }

    inja::json replacePatterns;
    replacePatterns["x"] = {};
    for(size_t i = 0; i < _registerInfo.nElements; ++i) {
      // FIXME: does not know about formating. TODO ticket 13534. See below..
      replacePatterns["x"].push_back(valueRegex);
    }

    std::regex returnRegex;
    try {
      auto regexText = inja::render(_registerInfo.readResponsePattern, replacePatterns);
      returnRegex = regexText;
    }
    catch(std::regex_error& e) {
      throw ChimeraTK::logic_error("Regex error in ResponsePattern for " errorMessageDetail + ": " + e.what());
    }
    catch(inja::ParserError& e) {
      throw ChimeraTK::logic_error("Inja parser error in ResponsePattern for " + errorMessageDetail + ": " + e.what());
    }
    // Alignment between the mark_count and nElements can be enforced by using non-capture groups: (?:   )
    if(returnRegex.mark_count() != requiredElements) {
      throw ChimeraTK::logic_error("Wrong number of capture groups (" +
          std::to_string(_readResponseRegex.mark_count()) + ") in readResponsePattern \"" +
          _registerInfo.readResponsePattern + "\" of " + _registerInfo.registerPath + ", required " +
          std::to_string(_registerInfo.nElements));
    }
    return returnRegex;
  } // end getRegex

  /********************************************************************************************************************/

  // FIXME: does not know about formating. TODO ticket 13534.
  // May need leading zeros or other formatting to satisfy the hardware interface.
  // Do this using _registerInfo.writeInfo.fixedSizeNumberWidthOpt in the function pointer implementation.

  // For use in preWrite
  template<typename UserType>
  std::string toTransportLayerDefault(const UserType& val, const InteractionInfo& iInfo) {
    return userTypeToUserType<std::string, UserType>(val);
  }

  template<typename UserType>
  std::string toTransportLayerHexInt(const UserType& val, const InteractionInfo& iInfo) {
    std::ostringstream oss;
    oss << std::hex << userTypeToUserType<uint64_t, UserType>(val);
    return oss.str();
  }

  template<typename UserType>
  std::string toTransportLayerBinInt(const UserType& val, const InteractionInfo& iInfo) {
    auto maybeStr = binaryStrFromInt<UserType>(val, _registerInfo.writeInfo.fixedSizeNumberWidthOpt);
    if(not maybeStr) {
      throw ChimerTK::runtime_error("Unable to fit value into the fixed_width write slot");
    }
    return maybeStr.value;
  }

  // For use in postRead
  template<typename UserType>
  UserType toUserTypeDefault(const std::string& str, const InteractionInfo& iInfo) {
    return userTypeToUserType<UserType, std::string>(str);
  }

  template<typename UserType>
  UserType toUserTypeHexInt(const std::string& str, const InteractionInfo& iInfo) {
    return userTypeToUserType<UserType, std::string>("0x" + str);
  }

  template<typename UserType>
  UserType toUserTypeBinInt(const std::string& str, const InteractionInfo& iInfo) {
    auto maybeInt = intFromBinaryStr<UserType>(str, _registerInfo.readInfo.fixedSizeNumberWidthOpt);
    if(not maybeInt) {
      throw ChimerTK::runtime_error("Unable to fit the read value into UserType.");
    }
    return maybeInt.value();
  }

  /********************************************************************************************************************/

  template<typename UserType>
  ToUserTypeFunc<UserType> getToUserTypeFunction(TransportLayerType transportLayerType) {
    if(transportLayerType == TransportLayerType::HEX_INT) {
      return &toUserTypeHexInt<UserType>;
    }
    else if(transportLayerType == TransportLayerType::BIN_INT) {
      return &toUserTypeBinInt<UserType>;
    }
    else { // DEC_INT, DEC_FLOAT, STRING
      return &toUserTypeDefault<UserType>;
    }
  }

  /********************************************************************************************************************/

  template<typename UserType>
      ToTransportLayerFunc < UserType >> getToTransportLayerFunction(TransportLayerType transportLayerType) {
    if(transportLayerType == TransportLayerType::HEX_INT) {
      return &toTransportLayerHexInt<UserType>
    }
    else if(transportLayerType == TransportLayerType::BIN_INT) {
      return &toTransportLayerBinInt<UserType>;
    }
    else { // DEC_INT, DEC_FLOAT, STRING
      return &toTransportLayerDefault<UserType>;
    }
  }

} // end anonymous namespace

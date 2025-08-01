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
#include <type_traits>

namespace ChimeraTK {

  using InteractionInfo = CommandBasedBackendRegisterInfo::InteractionInfo;

  /** Return the functional for the given TransportLayerType for converting data from the transport layer format to
   * UserType representation*/
  template<typename UserType>
  static ToUserTypeFunc<UserType> getToUserTypeFunction(TransportLayerType transportLayerType);

  /** Return the functional for the given TransportLayerType for converting data from the to UserType representation to
   * the transport layer format*/
  template<typename UserType>
  static ToTransportLayerFunc<UserType> getToTransportLayerFunction(TransportLayerType transportLayerType);

  /********************************************************************************************************************/

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

    if(isWriteableImpl()) {
      _transportLayerTypeFromUserType =
          getToTransportLayerFunction<UserType>(_registerInfo.writeInfo.getTransportLayerType());
    }

    if(isReadableImpl()) {
      _userTypeFromTransportLayerType = getToUserTypeFunction<UserType>(_registerInfo.readInfo.getTransportLayerType());

      // We seek registerInfo.getNumberOfElements() matches in the response regex,
      // which may be more than the number of elements in the the register (_numberOfElements), due to a non-zero
      // _elementOffsetInRegister.
      _readResponseRegex = _registerInfo.getReadResponseRegex();
    }

  } // end constructor CommandBasedBackendRegisterAccessor

  /********************************************************************************************************************/
  template<typename UserType>
  void CommandBasedBackendRegisterAccessor<UserType>::doPreRead([[maybe_unused]] TransferType) {
    if(!_backend->isOpen() && !_isRecoveryTestAccessor) {
      throw ChimeraTK::logic_error("Device not opened.");
    }
    if(!isReadable()) {
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
    std::string readCommand;
    if(_registerInfo.readInfo.isBinary()) {
      readCommand = binaryStrFromHexStr(_registerInfo.readInfo.commandPattern, /*isSigned*/ false);
    }
    else {
      readCommand = _registerInfo.readInfo.commandPattern;
    }

    _readTransferBuffer = _backend->sendCommandAndRead(readCommand, _registerInfo.readInfo);
  }

  /********************************************************************************************************************/
  template<typename UserType>
  void CommandBasedBackendRegisterAccessor<UserType>::doPostRead(
      [[maybe_unused]] TransferType t, bool updateDataBuffer) {
    // Transfer type enum options: {read, readNonBlocking, readLatest, write, writeDestructively }

    if(updateDataBuffer) {
      // For technical reasons the response has been read line by line.
      // Combine them here back into a single response string.
      std::string combinedReadString{};
      if(_registerInfo.readInfo.usesReadLines()) {
        std::string delim = *_registerInfo.readInfo.getResponseLinesDelimiter();
        if(_registerInfo.readInfo.isBinary()) {
          for(const auto& line : _readTransferBuffer) {
            combinedReadString += hexStrFromBinaryStr(line) + delim;
          }
        }
        else {
          for(const auto& line : _readTransferBuffer) {
            combinedReadString += line + delim;
          }
        }
      }
      else if(_registerInfo.readInfo.usesReadBytes()) {
        if(_registerInfo.readInfo.isBinary()) {
          combinedReadString = hexStrFromBinaryStr(_readTransferBuffer[0]);
        }
        else {
          combinedReadString = _readTransferBuffer[0];
        }
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

    if(!isWriteable()) {
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
      _writeTransferBuffer = inja::render(_registerInfo.writeInfo.commandPattern, replacePatterns);
    }
    catch(inja::ParserError& e) {
      throw ChimeraTK::logic_error(
          "Inja parser error in write commandPattern of " + _registerInfo.registerPath + ": " + e.what());
    }

    if(_registerInfo.writeInfo.isBinary()) {
      _writeTransferBuffer = binaryStrFromHexStr(_writeTransferBuffer, /*isSigned*/ false);
    }

    // remember this register as the last used one if the register is readable
    if(isReadable()) {
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

    _backend->sendCommandAndRead(_writeTransferBuffer, _registerInfo.writeInfo);
    return false; // no data was lost
  }

  /********************************************************************************************************************/
  /********************************************************************************************************************/

  // For use in preWrite
  template<typename T>
  using enableIfIntegral = std::enable_if_t<std::is_integral<T>::value || std::is_same_v<T, ChimeraTK::Boolean>>;

  // FIXME: does not know about formating. TODO ticket 13534.
  // May need leading zeros or other formatting to satisfy the hardware interface.
  // Do this using _registerInfo.writeInfo.fixedRegexCharacterWidthOpt in the function pointer implementation.

  template<typename UserType>
  static std::string toTransportLayerDefault(const UserType& val, [[maybe_unused]] const InteractionInfo& iInfo) {
    return ChimeraTK::userTypeToUserType<std::string, UserType>(val);
  }

  /********************************************************************************************************************/

  // This will not try to compile this if UserType is not an integer type.
  template<typename UserType, typename = enableIfIntegral<UserType>>
  static std::string toTransportLayerHexInt(const UserType& val, [[maybe_unused]] const InteractionInfo& iInfo) {
    std::optional<std::string> maybeStr;
    if(iInfo.fixedRegexCharacterWidthOpt) {
      maybeStr = hexStrFromInt(val, *(iInfo.fixedRegexCharacterWidthOpt), iInfo.isSigned, OverflowBehavior::NULLOPT);
    }
    else {
      maybeStr = hexStrFromInt(val, WidthOption::COMPACT, iInfo.isSigned);
    }

    if(not maybeStr) {
      throw ChimeraTK::runtime_error("Unable to fit value into the fixed_width write slot");
    }
    return *maybeStr;
  }

  /********************************************************************************************************************/

  // This will not try to compile this if UserType is not an integer type.
  template<typename UserType, typename = enableIfFloat<UserType>>
  static std::string toTransportLayerHexFloat(const UserType& val, [[maybe_unused]] const InteractionInfo& iInfo) {
    return hexStrFromFloat(val);
  }

  /********************************************************************************************************************/
  /********************************************************************************************************************/
  // For use in postRead

  template<typename UserType>
  UserType toUserTypeDefault(const std::string& str, [[maybe_unused]] const InteractionInfo& iInfo) {
    return ChimeraTK::userTypeToUserType<UserType, std::string>(str);
  }

  /********************************************************************************************************************/

  // This will not try to compile this if UserType is not an integer type.
  template<typename UserType, typename = enableIfIntegral<UserType>>
  static UserType toUserTypeHexInt(const std::string& str, [[maybe_unused]] const InteractionInfo& iInfo) {
    if(iInfo.isSigned) {
      auto maybeInt = intFromBinaryStr<UserType>(binaryStrFromHexStr(str, iInfo.isSigned));
      if(not maybeInt) {
        throw ChimeraTK::runtime_error("Unable to fit the value " + str + " into the UserType for writint");
      }
      return *maybeInt;
    }
    return ChimeraTK::userTypeToUserType<UserType, std::string>("0x" + str); // only supports unsigned conversion
  }
  /********************************************************************************************************************/

  template<typename UserType, typename = enableIfFloat<UserType>>
  static UserType toUserTypeHexFloat(const std::string& str, [[maybe_unused]] const InteractionInfo& iInfo) {
    auto maybeFloat = floatFromBinaryStr<UserType>(binaryStrFromHexStr(str, false));
    if(not maybeFloat) {
      throw ChimeraTK::runtime_error("Unable to fit the value " + str + " into the UserType for writing");
    }
    return *maybeFloat;
  }

  /********************************************************************************************************************/

  template<typename UserType>
  static ToUserTypeFunc<UserType> getToUserTypeFunction(TransportLayerType transportLayerType) {
    if constexpr(std::is_integral_v<UserType>) { // toUserTypeHexInt is only defined when UserType is an integer type.
      if((transportLayerType == TransportLayerType::BIN_INT) or (transportLayerType == TransportLayerType::HEX_INT)) {
        return &toUserTypeHexInt<UserType>;
      }
    }
    else if constexpr(std::is_floating_point_v<UserType>) {
      if(transportLayerType == TransportLayerType::BIN_FLOAT) {
        return &toUserTypeHexFloat<UserType>;
      }
    }
    // DEC_INT, DEC_FLOAT, STRING
    return &toUserTypeDefault<UserType>;
  }

  /********************************************************************************************************************/

  template<typename UserType>
  static ToTransportLayerFunc<UserType> getToTransportLayerFunction(TransportLayerType transportLayerType) {
    if constexpr(std::is_integral_v<UserType>) {
      // toTransportLayerHexInt is only defined if UserType is an integer type.
      if((transportLayerType == TransportLayerType::HEX_INT) or (transportLayerType == TransportLayerType::BIN_INT)) {
        return &toTransportLayerHexInt<UserType>;
      }
    }
    else if constexpr(std::is_floating_point_v<UserType>) {
      if(transportLayerType == TransportLayerType::BIN_FLOAT) {
        return &toTransportLayerHexFloat<UserType>;
      }
    }
    // DEC_INT, DEC_FLOAT, STRING
    return &toTransportLayerDefault<UserType>;
  }

  /********************************************************************************************************************/
  // Magic from SupportedUserTypes.h
  INSTANTIATE_TEMPLATE_FOR_CHIMERATK_USER_TYPES(CommandBasedBackendRegisterAccessor);
} // end namespace ChimeraTK

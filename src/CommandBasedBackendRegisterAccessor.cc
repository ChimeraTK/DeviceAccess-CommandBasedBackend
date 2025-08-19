// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "CommandBasedBackendRegisterAccessor.h"

#include "Checksum.h"
#include "CommandBasedBackend.h"
#include "CommandBasedBackendRegisterInfo.h"
#include "injaUtils.h"
#include "stringUtils.h"

#include <regex>
#include <sstream>
#include <string>
#include <type_traits>

namespace ChimeraTK {

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

      _writeResponseDataRegex = _registerInfo.getWriteResponseDataRegex();
      _writeResponseChecksumRegex = _registerInfo.getWriteResponseChecksumRegex();
      _writeResponseChecksumPayloadRegex = _registerInfo.getWriteResponseChecksumPayloadRegex();

      _writeCommandChecksumers = makeChecksumers(interactionType::CMD, _registerInfo.writeInfo);
      _writeResponseChecksumers = makeChecksumers(interactionType::RESP, _registerInfo.writeInfo);
    }

    if(isReadableImpl()) {
      _userTypeFromTransportLayerType = getToUserTypeFunction<UserType>(_registerInfo.readInfo.getTransportLayerType());

      // We seek registerInfo.getNumberOfElements() matches in the response regex,
      // which may be more than the number of elements in the the register (_numberOfElements), due to a non-zero
      // _elementOffsetInRegister.
      _readResponseDataRegex = _registerInfo.getReadResponseDataRegex();
      _readResponseChecksumRegex = _registerInfo.getReadResponseChecksumRegex();
      _readResponseChecksumPayloadRegex = _registerInfo.getReadResponseChecksumPayloadRegex();

      _readCommandChecksumers = makeChecksumers(interactionType::CMD, _registerInfo.readInfo);
      _readResponseChecksumers = makeChecksumers(interactionType::RESP, _registerInfo.readInfo);
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

    // Compute the checksums of the read command.
    inja::json replacePatterns;
    replacePatterns[toStr(injaTemplatePatternKeys::CHECKSUM_START)] = {};
    replacePatterns[toStr(injaTemplatePatternKeys::CHECKSUM_END)] = {};
    replacePatterns[toStr(injaTemplatePatternKeys::CHECKSUM_POINT)] = {};
    for(size_t i = 0; i < _registerInfo.readInfo.commandChecksumEnums.size(); ++i) {
      /* Skip the potential step of
       * renderedChecksumPayload = injaRender(_registerInfo.readInfo.commandChecksumPayloadStrs[i], replacePatterns, ...)
       */
      auto renderedChecksumPayload = std::string(_registerInfo.readInfo.commandChecksumPayloadStrs[i]);
      replacePatterns[toStr(injaTemplatePatternKeys::CHECKSUM_START)].push_back("");
      replacePatterns[toStr(injaTemplatePatternKeys::CHECKSUM_END)].push_back("");
      replacePatterns[toStr(injaTemplatePatternKeys::CHECKSUM_POINT)].push_back(
          _readCommandChecksumers[i](renderedChecksumPayload));
    }

    std::string renderedReadCommand = injaRender(_registerInfo.readInfo.commandPattern, replacePatterns,
        "in read command pattern of " + _registerInfo.registerPath);

    std::string readCommand;
    if(_registerInfo.readInfo.isBinary()) {
      readCommand = binaryStrFromHexStr(renderedReadCommand, /*isSigned*/ false);
    }
    else {
      readCommand = renderedReadCommand;
    }

    _readTransferBuffer = _backend->sendCommandAndRead(readCommand, _registerInfo.readInfo);
  }

  /********************************************************************************************************************/

  /*
   * For technical reasons the response has been read line by line.
   * Combine them here back into a single response string.
   */
  std::string makeCombinedReadString(const std::vector<std::string>& transferBuffer, const InteractionInfo& iInfo) {
    std::string combinedReadString{};
    if(iInfo.usesReadLines()) {
      std::string delim = *iInfo.getResponseLinesDelimiter();
      if(iInfo.isBinary()) {
        for(const auto& line : transferBuffer) {
          combinedReadString += hexStrFromBinaryStr(line) + delim;
        }
      }
      else {
        for(const auto& line : transferBuffer) {
          combinedReadString += line + delim;
        }
      }
    }
    else if(iInfo.usesReadBytes()) {
      if(iInfo.isBinary()) {
        combinedReadString = hexStrFromBinaryStr(transferBuffer[0]);
      }
      else {
        combinedReadString = transferBuffer[0];
      }
    }
    return combinedReadString;
  }

  /********************************************************************************************************************/

  /**
   * @brief This extracts the checksum and checksum payload from combinedReadString via regexs, computes the checksum on
   * the payload, and compares to the extracted payload
   * @param[in] iInfo The InteractionInfo correspondign to read/write
   * @param[in] responseChecksumPayloadRegex The regex to extract the checksum payload from combinedReadString.
   * @param[in] responseChecksumRegex The regex to extract the checksum from combinedReadString.
   * @param[in] responseChecksumers The functional to compute the checksum from the payload.
   * @param[in] errorMessageDetail Will be replaced by iInfo.errorMessageDetail after ticket 14877
   * @throws ChimeraTK::runtime_error if the
   */
  void inspectChecksum(const std::string& combinedReadString, const InteractionInfo& iInfo,
      const std::regex& responseChecksumPayloadRegex, const std::regex& responseChecksumRegex,
      const std::vector<Checksumer>& responseChecksumers, const std::string& errorMessageDetail) {
    // Second regex match: extract the checksum payloads.
    std::smatch checksumPayloadMatch;
    if(!std::regex_match(combinedReadString, checksumPayloadMatch, responseChecksumPayloadRegex)) {
      throw ChimeraTK::runtime_error(
          "Could not extract checksum payloads with the response checksum payload regex in \"" +
          replaceNewLines(combinedReadString) + "\" for " + errorMessageDetail);
    }

    std::vector<std::string> checksumResults;
    for(size_t i = 0; i < iInfo.responseChecksumEnums.size(); ++i) {
      checksumResults.push_back(responseChecksumers[i](checksumPayloadMatch.str(i + 1)));
    }
    /*----------------------------------------------------------------------------------------------------------------*/
    // Third regex match: extract the checksums received and compare.
    std::smatch checksumMatch;
    if(!std::regex_match(combinedReadString, checksumMatch, responseChecksumRegex)) {
      throw ChimeraTK::runtime_error("Could not extract checksum values with the response checksum regex in \"" +
          replaceNewLines(combinedReadString) + "\" for " + errorMessageDetail);
    }
    for(size_t i = 0; i < iInfo.responseChecksumEnums.size(); ++i) {
      if(checksumMatch.str(i + 1) != checksumResults[i]) {
        throw ChimeraTK::runtime_error("Response checksum " + toStr(iInfo.responseChecksumEnums[i]) + " failed for " +
            errorMessageDetail + ". Received \"" + std::string(checksumMatch.str(i + 1)) + "\" but calculated \"" +
            checksumResults[i] + "\"");
      }
    }
  } // end inspectChecksum

  /********************************************************************************************************************/

  template<typename UserType>
  void CommandBasedBackendRegisterAccessor<UserType>::doPostRead(
      [[maybe_unused]] TransferType t, bool updateDataBuffer) {
    // Transfer type enum options: {read, readNonBlocking, readLatest, write, writeDestructively }

    if(updateDataBuffer) {
      std::string combinedReadString = makeCombinedReadString(_readTransferBuffer, _registerInfo.readInfo);

      /*--------------------------------------------------------------------------------------------------------------*/
      // First regex match: extract the data.
      std::smatch dataMatch;
      if(!std::regex_match(combinedReadString, dataMatch, _readResponseDataRegex)) {
        throw ChimeraTK::runtime_error("Could not extract data values with the read response data regex for \"" +
            replaceNewLines(combinedReadString) + "\" in " + _registerInfo.registerPath);
      }

      for(size_t i = 0; i < _numberOfElements; ++i) {
        buffer_2D[0][i] =
            _userTypeFromTransportLayerType(dataMatch.str(i + _elementOffsetInRegister + 1), _registerInfo.readInfo);
      }
      /*--------------------------------------------------------------------------------------------------------------*/
      inspectChecksum(combinedReadString, _registerInfo.readInfo, _readResponseChecksumPayloadRegex,
          _readResponseChecksumRegex, _readResponseChecksumers, "read for " + _registerInfo.registerPath);
      /*--------------------------------------------------------------------------------------------------------------*/
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
    replacePatterns[toStr(injaTemplatePatternKeys::DATA)] = {};
    for(size_t i = 0; i < _numberOfElements; ++i) {
      replacePatterns[toStr(injaTemplatePatternKeys::DATA)].push_back(
          _transportLayerTypeFromUserType(buffer_2D[0][i], _registerInfo.writeInfo));
    }

    // Compute the checksums
    std::string errorMessageDetail = "in write command checksum pattern for " + _registerInfo.registerPath;
    replacePatterns[toStr(injaTemplatePatternKeys::CHECKSUM_START)] = {};
    replacePatterns[toStr(injaTemplatePatternKeys::CHECKSUM_END)] = {};
    replacePatterns[toStr(injaTemplatePatternKeys::CHECKSUM_POINT)] = {};
    for(size_t i = 0; i < _registerInfo.writeInfo.commandChecksumEnums.size(); ++i) {
      std::string renderedChecksumPayload = injaRender(_registerInfo.writeInfo.commandChecksumPayloadStrs[i],
          replacePatterns, errorMessageDetail + " on the " + std::to_string(i) + "th checksum payload.");
      replacePatterns[toStr(injaTemplatePatternKeys::CHECKSUM_START)].push_back("");
      replacePatterns[toStr(injaTemplatePatternKeys::CHECKSUM_END)].push_back("");
      replacePatterns[toStr(injaTemplatePatternKeys::CHECKSUM_POINT)].push_back(
          _writeCommandChecksumers[i](renderedChecksumPayload));
    }

    // Form the write command with data and checksums.
    _writeTransferBuffer = injaRender(_registerInfo.writeInfo.commandPattern, replacePatterns, errorMessageDetail);

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

    std::vector<std::string> writeResponseBuffer =
        _backend->sendCommandAndRead(_writeTransferBuffer, _registerInfo.writeInfo);

    std::string combinedReadString = makeCombinedReadString(writeResponseBuffer, _registerInfo.writeInfo);
    /*----------------------------------------------------------------------------------------------------------------*/
    // Regex compare: Make sure the write response matches the expected pattern.
    std::smatch payloadMatch;
    if(!std::regex_match(combinedReadString, payloadMatch, _writeResponseDataRegex)) {
      throw ChimeraTK::runtime_error("Write response \"" + replaceNewLines(combinedReadString) +
          "\" does not match the required template regex for " + _registerInfo.registerPath);
    }
    /*----------------------------------------------------------------------------------------------------------------*/
    inspectChecksum(combinedReadString, _registerInfo.writeInfo, _writeResponseChecksumPayloadRegex,
        _writeResponseChecksumRegex, _writeResponseChecksumers, "write for " + _registerInfo.registerPath);

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

// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "CommandBasedBackendRegisterInfo.h"

#include "jsonUtils.h"
#include "mapFileKeys.h"

#include <array>
#include <map>
#include <optional>
#include <utility>

#define FUNC_NAME (std::string(__func__) + ": ")

namespace ChimeraTK {

  using InteractionInfo = CommandBasedBackendRegisterInfo::InteractionInfo;

  /**
   * @brief This enforces that the register is either readable, writeable, or both.
   * @param[in] errorMessageDetail Specifies the registerPath, and maybe other details to orient error messages.
   * @throws ChimeraTK::logic_error
   */
  static void throwIfBadActivation(
      const InteractionInfo& writeInfo, const InteractionInfo& readInfo, const std::string& errorMessageDetail);

  /**
   * @brief This checks for the obvious error of having a command response patterns without corresponding command patterns.
   * @param[in] errorMessageDetail Specifies the registerPath, and maybe other details to orient error messages.
   * @throws ChimeraTK::logic_error
   */
  static void throwIfBadCommandAndResponsePatterns(

      const InteractionInfo& writeInfo, const InteractionInfo& readInfo, const std::string& errorMessageDetail);

  /**
   * @brief Throws unless at least one of the two interaction Infos has a type set.
   * @param[in] errorMessageDetail Specifies the registerPath, and maybe other details to orient error messages.
   * @throws ChimeraTK::logic_error If no type has been set.
   */
  static void throwIfATransportLayerTypeIsNotSet(
      InteractionInfo& writeInfo, InteractionInfo& readInfo, const std::string& errorMessageDetail);

  /**
   * @brief Throws unless both interaction Infos have a type set.
   * @param[in] errorMessageDetail Specifies the registerPath, and maybe other details to orient error messages.
   * @throws ChimeraTK::logic_error
   */
  static void throwIfTransportLayerTypesAreNotBothSet(
      InteractionInfo& writeInfo, InteractionInfo& readInfo, const std::string& errorMessageDetail);

  /**
   * @brief If one InteractionInfo is missing its TransportLayerType, the type is copied from the other InteractionInfo
   * The InteractionInfo is expected to lack a transport type if it is never activated, but the type is needed
   * Doing this synchronization simplifies later logic, such as throwIfBadNElements.
   */
  static void synchronizeTransportLayerTypes(InteractionInfo& writeInfo, InteractionInfo& readInfo);

  /**
   * @brief Validates the line endings for the interaction info.
   * Enforces that responseLinesDelimiter is not empty if the interaction uses read lines.
   * @param[in] iInfo The InteractionInfo to validate.
   * @param[in] errorMessageDetail Specifies the registerPath, and maybe other details to orient error messages.
   * @throws ChimeraTK::logic_error if invalid.
   */
  static void throwIfBadEndings(const InteractionInfo& iInfo, const std::string& errorMessageDetail);

  /**
   * @brief Validates nElements, particularly its interaction with VOID type.
   * Enforces that void type registers must be write-only, have nElements = 1, and have no inja variables in their commands.
   * @throws ChimeraTK::logic_error If nElements is incompatible with void type.
   */
  static void throwIfBadNElements(
      const InteractionInfo& writeInfo, unsigned int nElem, const std::string& errorMessageDetail);

  /**
   * @brief Validates fractionalBitsOpt, and its interactions.
   * Checks interactions with the transportLayerType, fractionalBitsOpt, fixedRegexCharacterWidthOpt, and signed.
   * transportLayerType must be set before running this
   * @param[in] iInfo The InteractionInfo to validate.
   * @param[in] errorMessageDetail Specifies the registerPath, and maybe other details to orient error messages.
   * @throws ChimeraTK::logic_error If fractionalBitsOpt or its interactions is invalid.
   */
  static void throwIfBadFractionalBits(const InteractionInfo& iInfo, const std::string& errorMessageDetail);

  /**
   * @brief Validates fixedRegexCharacterWidthOpt, particularly its interaction with the transportLayerType.
   * transportLayerType must be set before running this
   * Ensures that fixedRegexCharacterWidthOpt is set if iInfo.isBinary().
   * @param[in] iInfo The InteractionInfo to validate.
   * @param[in] errorMessageDetail Specifies the registerPath, and maybe other details to orient error messages.
   * @throws ChimeraTK::logic_error If fixedRegexCharacterWidthOpt is invalid for the type.
   */
  static void throwIfBadFixedWidth(const InteractionInfo& iInfo, const std::string& errorMessageDetail);

  /**
   * @brief Validates the signed property, making sure its a valid property for the TransportLayerType.
   * transportLayerType must be set before running this
   * @param[in] iInfo The InteractionInfo to validate.
   * @param[in] errorMessageDetail Specifies the registerPath, and maybe other details to orient error messages.
   * @throws ChimeraTK::logic_error If the signed property is invalid for the type.
   */
  static void throwIfBadSigned(const InteractionInfo& iInfo, const std::string& errorMessageDetail);

  /*
   * Get the DataType best suited to the InteractionInfo info.
   * This considers iInfo.isSigned, and the fixedRegexCharacterWidthOpt
   * returns the smallest datatype that meets those needs.
   */
  static DataType getDataType(const InteractionInfo& iInfo);

  /**
   * @brief Gets a single coherent data type from the two possible data types in writeInfo and readInfo, for the sake of
   * setting the DataDescriptor.
   * @param[in] errorMessageDetail Specifies the registerPath, and maybe other details to orient error messages.
   * @throws ChimeraTK::logic_error if the readInfo and writeInfo have incompatible data types.
   */
  static DataType getDataType(
      const InteractionInfo& writeInfo, const InteractionInfo& readInfo, const std::string& errorMessageDetail);

  /*
   * Gets a single DataType, if possible, that will work for both iInfoA and iInfoB.
   * The resulting data type will have the larger of the two bit depths needed to suit iInfoB.
   * If either of the iInfo's are signed and call for integers, the result will be a signed integer.
   * If the types cannot be reconciled, then nullopt is returned.
   */
  static std::optional<DataType> getReconciledDataTypes(const InteractionInfo& iInfoA, const InteractionInfo& iInfoB);

  /**
   * @brief Sets iInfo.nElements from JSON, if present, or else sets it to 1.
   * @param[in] j nlohmann::json from the map file
   * @param[in] errorMessageDetail Specifies the registerPath, and maybe other details to orient error messages.
   * @throws ChimeraTK::logic_error if nElements is set <= 0 in the JSON.
   */
  static void setNElementsFromJson(
      CommandBasedBackendRegisterInfo& rInfo, const json& j, const std::string& errorMessageDetail);

  /**
   * @brief Sets the iInfo.TransportLayerType from JSON, if present, with type checking.
   * This must be a template to accomdate keys at the register level and the interaction level.
   * @param[in] j nlohmann::json from the map file
   * @param[in] errorMessageDetail Specifies the registerPath, and maybe other details to orient error messages.
   * @throws ChimeraTK::logic_error if the type in the JSON is invalid.
   */
  template<typename EnumType>
  static void setTypeFromJson(InteractionInfo& iInfo, const json& j, const std::string& errorMessageDetail);

  /**
   * @brief Sets the InteractionInfo's line delimiters, number of lines, and number of bytes, from the JSON,
   * This must be a template to accomdate keys at the register level and the interaction level.
   * @param[in] j nlohmann::json from the map file
   * @param[in] defaultDelimOpt Is a the default serial delimiter, such as that coming from the dmap file.
   * @param[in] errorMessageDetail Specifies the registerPath, and maybe other details to orient error messages.
   * @throws ChimeraTK::logic_error if the delimiters, nLines, and nBytes are in an invalid combination
   */
  template<typename EnumType>
  static void setEndingsFromJson(InteractionInfo& iInfo, const json& j,
      const std::optional<std::string>& defaultDelimOpt, const std::string& errorMessageDetail,
      bool responseIsAbsent = false);

  /**
   * @brief Sets iInfo.fixedRegexCharacterWidthOpt from JSON.
   * This must be a template to accomdate keys at the register level and the interaction level.
   * This depends on the transportLayerType being set.
   * @param[in] j nlohmann::json from the map file
   * @param[in] errorMessageDetail Specifies the registerPath, and maybe other details to orient error messages.
   * @throws ChimeraTK::logic_error if the JSON lists a value <= 0, or transportLayerType is not set.
   */
  template<typename EnumType>
  static void setFixedWidthFromJson(InteractionInfo& iInfo, const json& j, const std::string& errorMessageDetail);

  /**
   * @brief Sets iInfo.fractionalBitsOpt from JSON.
   * This must be a template to accomdate keys at the register level and the interaction level.
   * This depends on the transportLayerType being set, since it depends on iInfo.isBinary.
   * @param[in] j nlohmann::json from the map file
   * @param[in] errorMessageDetail Specifies the registerPath, and maybe other details to orient error messages.
   * @throws ChimeraTK::logic_error if transportLayerType is not set.
   */
  template<typename EnumType>
  static void setFractionalBitsFromJson(InteractionInfo& iInfo, const json& j, const std::string& errorMessageDetail);

  /**
   * @brief Sets iInfo.signed from JSON.
   * This must be a template to accomdate keys at the register level and the interaction level.
   * Acceptable json values are "true", and "false".
   * If no vaue is set, this sets a default value based on the type, so
   * this depends on the transportLayerType being set.
   * @param[in] j nlohmann::json from the map file
   * @param[in] errorMessageDetail Specifies the registerPath, and maybe other details to orient error messages.
   * @throws ChimeraTK::logic_error if the JSON lists unrecognizable value, or transportLayerType is not set.
   */
  template<typename EnumType>
  static void setSignedFromJson(InteractionInfo& iInfo, const json& j, const std::string& errorMessageDetail);

  /********************************************************************************************************************/
  /********************************************************************************************************************/

  void CommandBasedBackendRegisterInfo::validate(std::string& errorMessageDetail) {
    std::string errorMessageDetailRead = errorMessageDetail + " for read";
    std::string errorMessageDetailWrite = errorMessageDetail + " for write";
    /*----------------------------------------------------------------------------------------------------------------*/
    // Validite data in readInfo and writeInfo
    throwIfBadActivation(writeInfo, readInfo, errorMessageDetail);
    throwIfTransportLayerTypesAreNotBothSet(writeInfo, readInfo, errorMessageDetail);
    throwIfBadCommandAndResponsePatterns(writeInfo, readInfo, errorMessageDetail);
    throwIfBadEndings(writeInfo, errorMessageDetailWrite);
    throwIfBadEndings(readInfo, errorMessageDetailWrite);
    throwIfBadNElements(writeInfo, getNumberOfElementsImpl(), errorMessageDetail);
    throwIfBadFixedWidth(writeInfo, errorMessageDetailWrite);
    throwIfBadFixedWidth(readInfo, errorMessageDetailRead);
    throwIfBadFractionalBits(writeInfo, errorMessageDetailWrite);
    throwIfBadFractionalBits(readInfo, errorMessageDetailRead);
    throwIfBadSigned(writeInfo, errorMessageDetailWrite);
    throwIfBadSigned(readInfo, errorMessageDetailRead);
  }

  /********************************************************************************************************************/

  void CommandBasedBackendRegisterInfo::finalize() {
    std::string errorMessageDetail = "register " + registerPath;

    validate(errorMessageDetail);

    // Check that the data types are compatible and set dataDescriptor
    dataDescriptor = DataDescriptor(getDataType(writeInfo, readInfo, errorMessageDetail));
  } // end init

  /********************************************************************************************************************/

  CommandBasedBackendRegisterInfo::CommandBasedBackendRegisterInfo(
      const RegisterPath& registerPath_, InteractionInfo readInfo_, InteractionInfo writeInfo_, uint nElements_)
  : nElements(nElements_), registerPath(registerPath_), readInfo(std::move(readInfo_)),
    writeInfo(std::move(writeInfo_)) {
    std::string registerPathStr = std::string(registerPath);
    if(registerPathStr.empty() or (registerPath == "/")) { // if registerPath is empty
      // CommandBasedBackendRegisterInfo is initalized as an empty placeholder, so don't validate its data.
      return;
    }
    finalize(); // Performs data validation
  }

  /********************************************************************************************************************/

  CommandBasedBackendRegisterInfo::CommandBasedBackendRegisterInfo(
      const RegisterPath& registerPath_, const json& j, const std::string& defaultSerialDelimiter)
  : registerPath(registerPath_) {
    /*
     * Here we extract data from the json,
     * Checking only that we don't have invalid json keys or unpassable negative numbers.
     * Then call finalize() for data validation and tasks common to all constructors.
     */
    std::string errorMessageDetail = "register " + registerPath;
    std::string errorMessageDetailRead = errorMessageDetail + " read";
    std::string errorMessageDetailWrite = errorMessageDetail + " write";
    auto readOpt = caseInsensitiveGetValueOption(j, toStr(mapFileRegisterKeys::READ));
    auto writeOpt = caseInsensitiveGetValueOption(j, toStr(mapFileRegisterKeys::WRITE));

    // Validate retister-level json keys
    throwIfHasInvalidJsonKeyCaseInsensitive(j, getMapForEnum<mapFileRegisterKeys>(), "Map file " + errorMessageDetail);
    /*----------------------------------------------------------------------------------------------------------------*/
    // SET CONTENT BASED ON TOP-LEVEL JSON

    // N_ELEM,
    setNElementsFromJson(*this, j, errorMessageDetail);

    // TYPE
    setTypeFromJson<mapFileRegisterKeys>(readInfo, j, errorMessageDetail);
    setTypeFromJson<mapFileRegisterKeys>(writeInfo, j, errorMessageDetail);
    if(readOpt) {
      setTypeFromJson<mapFileInteractionInfoKeys>(readInfo, readOpt->get<json>(), errorMessageDetailRead);
    }
    if(writeOpt) {
      setTypeFromJson<mapFileInteractionInfoKeys>(writeInfo, writeOpt->get<json>(), errorMessageDetailWrite);
    }
    throwIfATransportLayerTypeIsNotSet(writeInfo, readInfo, errorMessageDetail);
    synchronizeTransportLayerTypes(writeInfo, readInfo);
    throwIfTransportLayerTypesAreNotBothSet(writeInfo, readInfo, errorMessageDetail); // DEBUG

    // DELIMITER, COMMAND_DELIMITER, RESPONSE_DELIMITER, N_RESPONSE_LINES, N_RESPONSE_BYTES
    setEndingsFromJson<mapFileRegisterKeys>(readInfo, j, defaultSerialDelimiter, errorMessageDetail);
    setEndingsFromJson<mapFileRegisterKeys>(writeInfo, j, defaultSerialDelimiter, errorMessageDetail);
    // NOTE: These delimiter settings may be overrided by populateFromJson below.

    // BIT_WIDTH, CHARACTER_WIDTH, depends on transportLayerType
    setFixedWidthFromJson<mapFileRegisterKeys>(readInfo, j, errorMessageDetail);
    setFixedWidthFromJson<mapFileRegisterKeys>(writeInfo, j, errorMessageDetail);

    // FRACTIONAL_BITS, depends on transportLayerType
    setFractionalBitsFromJson<mapFileRegisterKeys>(
        readInfo, j, errorMessageDetail); // These must come after setFixedWidthFromJson
    setFractionalBitsFromJson<mapFileRegisterKeys>(writeInfo, j, errorMessageDetail);

    // SIGNED, depends on transportLayerType
    setSignedFromJson<mapFileRegisterKeys>(readInfo, j, errorMessageDetail);
    setSignedFromJson<mapFileRegisterKeys>(writeInfo, j, errorMessageDetail);

    // READ,
    // Override settings from the top level based on the "read" key's contents
    if(readOpt) {
      readInfo.populateFromJson(readOpt->get<json>(), errorMessageDetailRead, true);
    }

    // WRITE
    // Override settings from the top level based on the "write" key's contents
    if(writeOpt) {
      writeInfo.populateFromJson(writeOpt->get<json>(), errorMessageDetailWrite, true);
    }
    /*----------------------------------------------------------------------------------------------------------------*/
    // FIXME: extract the number of lines in write response from pattern; Ticket 13531

    finalize(); // performs data validation
  } // end constructor

  /********************************************************************************************************************/
  /********************************************************************************************************************/

  void InteractionInfo::populateFromJson(const json& j, const std::string& errorMessageDetail, bool skipSetType) {
    // This is not just a constructor because we want to fill in json

    // Validate json keys at the InteractionInfo level
    throwIfHasInvalidJsonKeyCaseInsensitive(
        j, getMapForEnum<mapFileInteractionInfoKeys>(), "Map file Interaction for " + errorMessageDetail);

    // COMMAND
    if(auto opt = caseInsensitiveGetValueOption(j, toStr(mapFileInteractionInfoKeys::COMMAND))) {
      commandPattern = opt->get<std::string>();
    } // Resist the temptation for an early return here; we may later decide to change the definition
      // of an active interaction to involve type

    // TYPE
    if(not skipSetType) {
      setTypeFromJson<mapFileInteractionInfoKeys>(*this, j, errorMessageDetail);
    }

    if(not isActive()) {
      return;
    }

    // RESPESPONSE,
    bool responseIsAbsent = true;
    if(auto opt = caseInsensitiveGetValueOption(j, toStr(mapFileInteractionInfoKeys::RESPESPONSE))) {
      responseIsAbsent = false;
      responsePattern = opt->get<std::string>();
    }

    // DELIMITER, COMMAND_DELIMITER, RESPONSE_DELIMITER, N_RESPONSE_LINES, N_RESPONSE_BYTES
    setEndingsFromJson<mapFileInteractionInfoKeys>(*this, j, std::nullopt, errorMessageDetail, responseIsAbsent);

    // BIT_WIDTH, CHARACTER_WIDTH
    setFixedWidthFromJson<mapFileInteractionInfoKeys>(*this, j, errorMessageDetail);

    // FRACTIONAL_BITS
    setFractionalBitsFromJson<mapFileInteractionInfoKeys>(
        *this, j, errorMessageDetail); // Must come after setFixedWidthFromJson

    // SIGNED
    setSignedFromJson<mapFileInteractionInfoKeys>(*this, j, errorMessageDetail);
  } // populateFromJson

  /********************************************************************************************************************/

  std::optional<size_t> InteractionInfo::getResponseNLines() const noexcept {
    if(usesReadLines()) {
      return std::get<ResponseLinesInfo>(_responseInfo).nLines;
    }
    return std::nullopt;
  }

  /********************************************************************************************************************/

  std::optional<std::string> InteractionInfo::getResponseLinesDelimiter() const noexcept {
    if(usesReadLines()) {
      return std::get<ResponseLinesInfo>(_responseInfo).delimiter;
    }
    return std::nullopt;
  }

  /********************************************************************************************************************/

  std::optional<size_t> InteractionInfo::getResponseBytes() const noexcept {
    if(usesReadBytes()) {
      return std::get<ResponseBytesInfo>(_responseInfo).nBytesReadResponse;
    }
    return std::nullopt;
  }

  /********************************************************************************************************************/

  void InteractionInfo::setResponseDelimiter(std::string delimiter) {
    if(not usesReadLines()) {
      _responseInfo = ResponseLinesInfo{};
    }
    std::get<ResponseLinesInfo>(_responseInfo).delimiter = std::move(delimiter);
  }

  /********************************************************************************************************************/

  void InteractionInfo::setResponseNLines(size_t nLines) {
    if(not usesReadLines()) {
      _responseInfo = ResponseLinesInfo{};
    }
    std::get<ResponseLinesInfo>(_responseInfo).nLines = nLines;
  }

  /********************************************************************************************************************/
  void InteractionInfo::setTransportLayerType(TransportLayerType& type) noexcept {
    _transportLayerType = type;
    _isBinary = ((type == TransportLayerType::BIN_INT) or (type == TransportLayerType::BIN_FLOAT));
  }

  /********************************************************************************************************************/
  /********************************************************************************************************************/

  static void throwIfBadActivation(
      const InteractionInfo& writeInfo, const InteractionInfo& readInfo, const std::string& errorMessageDetail) {
    bool readable = readInfo.isActive();
    bool writeable = writeInfo.isActive();

    if(not(readable or writeable)) {
      throw ChimeraTK::logic_error(FUNC_NAME + "A non-empty read:" + toStr(mapFileInteractionInfoKeys::COMMAND) +
          " or write " + toStr(mapFileInteractionInfoKeys::COMMAND) +
          " tags is required, and neither are present for " + errorMessageDetail);
    }

    if(writeInfo.getTransportLayerType() == TransportLayerType::VOID) {
      if(readInfo.isActive() or not writeInfo.isActive()) {
        throw ChimeraTK::logic_error(FUNC_NAME + "Void type must be write-only but has a " +
            toStr(mapFileRegisterKeys::READ) + " key for " + errorMessageDetail);
      }
    }
  } // end throwIfBadActivation

  /********************************************************************************************************************/

  static void throwIfBadCommandAndResponsePatterns(
      const InteractionInfo& writeInfo, const InteractionInfo& readInfo, const std::string& errorMessageDetail) {
    bool lacksReadCommand = readInfo.commandPattern.empty();
    bool lacksWriteCommand = writeInfo.commandPattern.empty();
    bool hasReadResponse = not readInfo.responsePattern.empty();
    bool hasWriteResponse = not writeInfo.responsePattern.empty();
    // Throw if there are responses without corresponding commands.
    if(hasReadResponse and lacksReadCommand) {
      throw ChimeraTK::logic_error(FUNC_NAME + "A non-empty read " + toStr(mapFileInteractionInfoKeys::RESPESPONSE) +
          " without a non-empty read " + toStr(mapFileInteractionInfoKeys::COMMAND) + " for " + errorMessageDetail);
    }

    if(hasWriteResponse and lacksWriteCommand) {
      throw ChimeraTK::logic_error(FUNC_NAME + "A non-empty write " + toStr(mapFileInteractionInfoKeys::RESPESPONSE) +
          " without a non-empty write " + toStr(mapFileInteractionInfoKeys::COMMAND) + " for " + errorMessageDetail);
    }

    if(writeInfo.getTransportLayerType() == TransportLayerType::VOID) {
      if(writeInfo.commandPattern.find("{{x") != std::string::npos) {
        throw ChimeraTK::logic_error(FUNC_NAME + "Illegal inja template in write " +
            toStr(mapFileInteractionInfoKeys::COMMAND) + " = \"" + writeInfo.commandPattern + "\" for void-type for " +
            errorMessageDetail);
      }
    }
  } // end throwIfBadCommandAndResponsePatterns

  /********************************************************************************************************************/

  static void throwIfBadEndings(const InteractionInfo& iInfo, const std::string& errorMessageDetail) {
    if(iInfo.usesReadLines() and iInfo.getResponseLinesDelimiter() == "") {
      throw ChimeraTK::logic_error(
          FUNC_NAME + "Illegally set response delimiter to empty string for" + errorMessageDetail);
    }
  } // end throwIfBadEndings

  /********************************************************************************************************************/

  static void throwIfBadNElements(
      const InteractionInfo& writeInfo, unsigned int nElem, const std::string& errorMessageDetail) {
    if(writeInfo.getTransportLayerType() == TransportLayerType::VOID) {
      if(nElem != 1) {
        throw ChimeraTK::logic_error(FUNC_NAME + "Void type must only have 1 element but has " +
            toStr(mapFileRegisterKeys::N_ELEM) + " = " + std::to_string(nElem) + " for " + errorMessageDetail);
      }
    }
    if(nElem == 0) {
      throw ChimeraTK::logic_error(
          FUNC_NAME + "Invalid zero " + toStr(mapFileRegisterKeys::N_ELEM) + " for " + errorMessageDetail);
    }
  } // end if void type

  /********************************************************************************************************************/

  static void throwIfTransportLayerTypesAreNotBothSet(
      InteractionInfo& writeInfo, InteractionInfo& readInfo, const std::string& errorMessageDetail) {
    if(not(writeInfo.hasTransportLayerType())) {
      throw ChimeraTK::logic_error(FUNC_NAME + "Type is required but is missing for write " + errorMessageDetail);
    }
    if(not(readInfo.hasTransportLayerType())) {
      throw ChimeraTK::logic_error(FUNC_NAME + "Type is required but is missing for read " + errorMessageDetail);
    }
  }

  /********************************************************************************************************************/

  static void throwIfATransportLayerTypeIsNotSet(
      InteractionInfo& writeInfo, InteractionInfo& readInfo, const std::string& errorMessageDetail) {
    // Throw if type/transportLayerType is not set.
    if(not(writeInfo.hasTransportLayerType() or readInfo.hasTransportLayerType())) {
      throw ChimeraTK::logic_error(FUNC_NAME + "Type is required but is missing for " + errorMessageDetail);
    }
    if(readInfo.isActive() and not readInfo.hasTransportLayerType()) {
      throw ChimeraTK::logic_error(FUNC_NAME + "Type is required but is missing on read for " + errorMessageDetail);
    }
    if(writeInfo.isActive() and not writeInfo.hasTransportLayerType()) {
      throw ChimeraTK::logic_error(FUNC_NAME + "Type is required but is missing on write for " + errorMessageDetail);
    }
  }

  /********************************************************************************************************************/

  static void synchronizeTransportLayerTypes(InteractionInfo& writeInfo, InteractionInfo& readInfo) {
    if(readInfo.hasTransportLayerType() and not writeInfo.hasTransportLayerType()) {
      TransportLayerType type = readInfo.getTransportLayerType();
      writeInfo.setTransportLayerType(type);
    }
    else if(writeInfo.hasTransportLayerType() and not readInfo.hasTransportLayerType()) {
      TransportLayerType type = writeInfo.getTransportLayerType();
      readInfo.setTransportLayerType(type);
    }
  }

  /********************************************************************************************************************/

  static void throwIfBadFractionalBits(const InteractionInfo& iInfo, const std::string& errorMessageDetail) {
    if((not iInfo.fractionalBitsOpt) or (not iInfo.isActive())) {
      return;
    }
    if(not iInfo.fixedRegexCharacterWidthOpt) {
      throw ChimeraTK::logic_error(FUNC_NAME + toStr(mapFileRegisterKeys::FRACTIONAL_BITS) + " is set but " +
          toStr(mapFileRegisterKeys::BIT_WIDTH) + " is not set for " + errorMessageDetail);
    }
    if((iInfo.getTransportLayerType() != TransportLayerType::BIN_INT) and
        (iInfo.getTransportLayerType() != TransportLayerType::HEX_INT)) {
      throw ChimeraTK::logic_error(FUNC_NAME + toStr(mapFileRegisterKeys::FRACTIONAL_BITS) +
          " is set for incompatible " + toStr(mapFileRegisterKeys::TYPE) + " " + toStr(iInfo.getTransportLayerType()) +
          " for " + errorMessageDetail + " (only " + toStr(TransportLayerType::BIN_INT) + " and " +
          toStr(TransportLayerType::HEX_INT) + " are compatible)");
    }
    // case: fractionalBits is too big
    // TODO confirm these formulas .. can fractional bits really not exceed the size of the container?
    size_t width = *(iInfo.fixedRegexCharacterWidthOpt);
    if(iInfo.isSigned) {
      if(static_cast<int>(width * 4) < (iInfo.fractionalBitsOpt.value() + 1)) {
        throw ChimeraTK::logic_error(FUNC_NAME + toStr(mapFileRegisterKeys::FRACTIONAL_BITS) + " exceeds the " +
            toStr(mapFileRegisterKeys::BIT_WIDTH) + " minus the sign bit for " + errorMessageDetail);
      }
    }
    else {
      if(static_cast<int>(width * 4) < (iInfo.fractionalBitsOpt.value())) {
        throw ChimeraTK::logic_error(FUNC_NAME + toStr(mapFileRegisterKeys::FRACTIONAL_BITS) + " exceeds the " +
            toStr(mapFileRegisterKeys::BIT_WIDTH) + " for " + errorMessageDetail);
      }
    }
    // TODO if fractionalBits is super negative, throw
  }

  /********************************************************************************************************************/

  static void throwIfBadFixedWidth(const InteractionInfo& iInfo, const std::string& errorMessageDetail) {
    if(not iInfo.isActive()) {
      return;
    }
    std::string bitWidthTag = toStr(mapFileRegisterKeys::BIT_WIDTH);
    std::string charWidthTag = toStr(mapFileRegisterKeys::CHARACTER_WIDTH);
    std::string eitherWidthTag = bitWidthTag + " or " + charWidthTag;
    if(not iInfo.fixedRegexCharacterWidthOpt) {
      if(iInfo.isBinary()) {
        throw ChimeraTK::logic_error(
            FUNC_NAME + bitWidthTag + " must be set for binary type for " + errorMessageDetail);
      }
      return;
    }
    size_t regexCharacterWidth = *(iInfo.fixedRegexCharacterWidthOpt);
    TransportLayerType type = iInfo.getTransportLayerType();

    // VOID
    if(type == TransportLayerType::VOID) {
      throw ChimeraTK::logic_error(FUNC_NAME + eitherWidthTag + " is set for " + toStr(type) + "=" +
          toStr(TransportLayerType::VOID) + " for " + errorMessageDetail);
    }

    if(regexCharacterWidth == 0) {
      throw ChimeraTK::logic_error(FUNC_NAME + "Invalid zero " + eitherWidthTag + " for " + errorMessageDetail);
    }

    // FLOAT
    if(type == TransportLayerType::BIN_FLOAT or type == TransportLayerType::DEC_FLOAT) {
      std::array<size_t, 3> validWidths = {16, 8, 4}; // widths are in nibbles.

      if(std::find(validWidths.begin(), validWidths.end(), regexCharacterWidth) == validWidths.end()) {
        throw ChimeraTK::logic_error(FUNC_NAME + "Invalid " + bitWidthTag + " " +
            std::to_string(4 * regexCharacterWidth) + " bits for type " + toStr(type) + " for " + errorMessageDetail);
      }
    }
    else if(type == TransportLayerType::DEC_INT) {
      /*Here regexCharacterWidth means the character width for the decimal representation
       * The widest possible DEC_INT that can fit in an int64 is  -9223372036854775808 (19 characters + sign char)
       * The widest possible DEC_INT that can fit in an uint64 is 18446744073709551615 (20 chars)
       */
      size_t maxWidth = iInfo.isSigned ? 19 : 20;
      if(regexCharacterWidth > maxWidth) {
        throw ChimeraTK::logic_error(FUNC_NAME + "Invalid " + charWidthTag + " " + std::to_string(regexCharacterWidth) +
            " digits for type " + toStr(type) + " for " + errorMessageDetail +
            ". That cannot be fit into 64 bits. Allowed range is 1-" + std::to_string(maxWidth));
      }
    }
    // INT read as hexidecimal by the regex
    else if((type == TransportLayerType::HEX_INT) or (type == TransportLayerType::BIN_INT)) {
      // Here regexCharacterWidth means the number of hexideicimal characters (nibbles) for the transport layer representation.
      if(regexCharacterWidth > 16 /*16 nibbles = 64 bits*/) {
        throw ChimeraTK::logic_error(FUNC_NAME + "Invalid " + bitWidthTag + " " +
            std::to_string(4 * regexCharacterWidth) + " bits for type " + toStr(type) + " for " + errorMessageDetail +
            ". That cannot be fit into 64 bits. Allowed range is 1-16.");
      }
    }
  } // end throwIfBadFixedWidth

  /********************************************************************************************************************/

  static void throwIfBadSigned(const InteractionInfo& iInfo, const std::string& errorMessageDetail) {
    if((not iInfo.isSigned) or (not iInfo.isActive())) {
      return;
    }
    TransportLayerType type = iInfo.getTransportLayerType();

    if(type == TransportLayerType::VOID or type == TransportLayerType::STRING) {
      throw ChimeraTK::logic_error(
          FUNC_NAME + toStr(mapFileRegisterKeys::TYPE) + " " + toStr(type) + "is signed for " + errorMessageDetail);
    }
  }

  /********************************************************************************************************************/
  /********************************************************************************************************************/

  static DataType getDataTypeFromSizeCode(int sizeCode, const std::map<DataType, int>& map) {
    for(const auto& [key, value] : map) {
      if(value == sizeCode) {
        return key;
      }
    }
    throw ChimeraTK::logic_error(FUNC_NAME + " No DataType corresponds to size code " + std::to_string(sizeCode));
  }

  // Size codes have absoute value = bit depth of the type, and are negative if the type is signed.
  static const std::map<DataType, int> dataTypeSizeCodeMap = {
      {DataType::Boolean, 1},
      {DataType::int8, -8},
      {DataType::uint8, 8},
      {DataType::int16, -16},
      {DataType::uint16, 16},
      {DataType::int32, -32},
      {DataType::uint32, 32},
      {DataType::int64, -64},
      {DataType::uint64, 64},
  };

  /********************************************************************************************************************/

  /*
   * Given some number of bits, minBits, get the next largest number of bits corresponding to a DataType.
   */
  static int getMinBitCountForADataType(size_t minBits) {
    if(minBits <= 1) {
      return 1;
    }
    if(minBits <= 8) {
      return 8;
    }
    if(minBits <= 16) {
      return 16;
    }
    if(minBits <= 32) {
      return 32;
    }
    return 64;
  }

  /*
   * Get the DataType best suited to the InteractionInfo info.
   * This considers iInfo.isSigned, and the fixedRegexCharacterWidthOpt
   * returns the smallest datatype that meets those needs.
   */
  static DataType getDataType(const InteractionInfo& iInfo) {
    const TransportLayerType type = iInfo.getTransportLayerType();
    const bool isSigned = iInfo.isSigned;
    const auto& map = (isSigned ? signedTransportLayerTypeToDataTypeMap : unsignedTransportLayerTypeToDataTypeMap);
    auto it = map.find(type); // std::unordered_map<TransportLayerType, DataType>::const_iterator it;

    if(it == map.end()) {
      throw ChimeraTK::logic_error(FUNC_NAME + "Type " + toStr(type) + " is not in " + (isSigned ? "" : "un") +
          "signedtransportLayerTypeToDataTypeMap. Fix it in mapFileKeys.h");
    }

    // Get the default DataType for this type and signed state
    DataType d = it->second;

    if(d == DataType::none) {
      throw ChimeraTK::logic_error(FUNC_NAME +
          "Invalid DataType::none. Go fix the unsigned/signedTransportLayerTypeToDataTypeMap "
          "in mapFileKeys.h to not use 'none'.");
    }

    // Consider shrinking the DataType based on the width setting.
    if(iInfo.fixedRegexCharacterWidthOpt) {
      size_t width = *(iInfo.fixedRegexCharacterWidthOpt);
      if(type == TransportLayerType::HEX_INT or type == TransportLayerType::BIN_INT or
          type == TransportLayerType::DEC_INT) { // Any int type

        bool adjustSizeFromFixedWidth = false;
        size_t fixedWidthSizeInBits;
        if(type == TransportLayerType::DEC_INT) {
          adjustSizeFromFixedWidth = true;
          // width is the number of base10 characters
          fixedWidthSizeInBits = static_cast<size_t>(std::lround(
              std::ceil((isSigned ? 1.0 : 0.0) + (std::log(static_cast<double>(width + 1)) / std::log(2.0)))));
          // When C++23 becomes available, replace std::lround(std::ceil( with std::ceil2int
        }
        else {
          adjustSizeFromFixedWidth = true;
          // width is the number of hexidecimal characters, aka nibbles.
          fixedWidthSizeInBits = (width * 4);
        }

        if(adjustSizeFromFixedWidth) {
          // fixedWidthSizeInBits might be something like 7. Get the next largest number of bits that makes a viable
          // DataType, equal to the absolute value of the size code.
          int minimumFixedWidthContainerSizeInBits = getMinBitCountForADataType(fixedWidthSizeInBits);

          // Use the smallest size that fits
          int newSizeCode = (minimumFixedWidthContainerSizeInBits * (isSigned ? -1 : 1));
          d = getDataTypeFromSizeCode(newSizeCode, dataTypeSizeCodeMap);
        }
      }
      else if((type == TransportLayerType::DEC_FLOAT) or (type == TransportLayerType::BIN_FLOAT)) {
        // TODO add `or (type == TransportLayerType::HEX_FLOAT)` to the above conditional once HEX_FLOAT is implemented

        size_t maxSizeToAllowFloat = 8; // BIN_FLOAT, HEX_FLOAT case: 0-8 nibbles, fitting into 32 bits.
        if(type == TransportLayerType::DEC_FLOAT) {
          maxSizeToAllowFloat = 7;
        }
        d = ((width <= maxSizeToAllowFloat) ? DataType::float32 : DataType::float64);
      }
    }

    return d;
  } // end getDataType

  /********************************************************************************************************************/

  static DataType getDataType(
      const InteractionInfo& writeInfo, const InteractionInfo& readInfo, const std::string& errorMessageDetail) {
    assert(writeInfo.isActive() or readInfo.isActive());

    if(writeInfo.isActive() and not readInfo.isActive()) {
      return getDataType(writeInfo);
    }
    if(readInfo.isActive() and not writeInfo.isActive()) {
      return getDataType(readInfo);
    }
    // both are active, have to reconcile them.
    auto mergedDataType = getReconciledDataTypes(writeInfo, readInfo);
    if(not mergedDataType) {
      throw ChimeraTK::logic_error(FUNC_NAME + "Read and Write have incompatible DataTypes for " + errorMessageDetail);
    }
    return *mergedDataType;
  }

  /********************************************************************************************************************/

  static std::optional<DataType> getReconciledDataTypes(const InteractionInfo& iInfoA, const InteractionInfo& iInfoB) {
    DataType a = getDataType(iInfoA);
    DataType b = getDataType(iInfoB);

    bool invalidCombination = false;
    invalidCombination |= (a.isNumeric() != b.isNumeric()); // Invalid if one is numeric and the other is not

    invalidCombination |= (a.isIntegral() != b.isIntegral());
    /* Invalid if one is integral and the other is not
     * Together with the numeric xor: invalid if one is floating point and the other isn't.
     */
    invalidCombination |= ((not a.isNumeric()) and (a != b));
    // Invalid if a missmatched pair of in {String, Void, none}

    if(invalidCombination) {
      return std::nullopt;
    }

    if(a == b) {
      return a;
    }

    // a and b now must be numeric and must be reconciled.
    if(a.isIntegral()) { // b is also integral
      // Figure out a DataType that's large enough for both and has a sign bit if one of the two needs it.
      bool mergedIsSigned = (a.isSigned() or b.isSigned());
      int aSizeCode = std::abs(dataTypeSizeCodeMap.at(a));
      int bSizeCode = std::abs(dataTypeSizeCodeMap.at(b));
      int mergedSizeCode =
          std::max(aSizeCode, bSizeCode) * (mergedIsSigned ? -1 : 1); // Use the larger of the two bit depths.
      return getDataTypeFromSizeCode(mergedSizeCode, dataTypeSizeCodeMap);
    }
    // a and b are both floating point and not equal, therefore one is float64.
    return DataType::float64;
  }

  /********************************************************************************************************************/
  /********************************************************************************************************************/

  static void setNElementsFromJson(
      CommandBasedBackendRegisterInfo& rInfo, const json& j, const std::string& errorMessageDetail) {
    std::string keyStr = toStr(mapFileRegisterKeys::N_ELEM);
    int64_t nElementsUnprotected = caseInsensitiveGetValueOr(j, keyStr, 1L);
    if(nElementsUnprotected < 1L) {
      throw ChimeraTK::logic_error(FUNC_NAME + "Invalid non-positive " + keyStr + " " +
          std::to_string(nElementsUnprotected) + " for " + errorMessageDetail);
    }
    if(nElementsUnprotected > static_cast<int64_t>(std::numeric_limits<unsigned int>::max())) {
      throw ChimeraTK::logic_error(FUNC_NAME + keyStr +
          " is too large to fit in a uint: " + std::to_string(nElementsUnprotected) + " for " + errorMessageDetail);
    }
    rInfo.nElements = static_cast<unsigned int>(nElementsUnprotected);
  } // end setNElementsFromJson

  /********************************************************************************************************************/

  template<typename EnumType>
  static void setTypeFromJson(InteractionInfo& iInfo, const json& j, const std::string& errorMessageDetail) {
    std::string keyStr = toStr(EnumType::TYPE);
    std::optional<std::string> typeStrOpt = caseInsensitiveGetValueOption(j, keyStr);
    if(typeStrOpt) {
      std::optional<TransportLayerType> typeEnumOpt = strToEnumOpt<TransportLayerType>(*typeStrOpt);
      if(not typeEnumOpt) {
        throw ChimeraTK::logic_error(
            FUNC_NAME + "Unknown value for " + keyStr + ": " + *typeStrOpt + " for " + errorMessageDetail);
      }
      iInfo.setTransportLayerType(*typeEnumOpt);
    }
  } // end setTypeFromJson

  /********************************************************************************************************************/

  template<typename EnumType>
  static void setEndingsFromJson(InteractionInfo& iInfo, const json& j,
      const std::optional<std::string>& defaultDelimOpt, const std::string& errorMessageDetail,
      const bool responseIsAbsent) {
    bool explicitlySetToReadLines = false;
    std::string keyStr;
    /*----------------------------------------------------------------------------------------------------------------*/
    if(iInfo.isBinary()) { // If binary mode and no command delimiter is set, we default to undelimited.
      if(iInfo.usesReadBytes()) {
        iInfo.cmdLineDelimiter = "";
      }
      else {
        iInfo.setResponseBytes(0);
      }
    }
    else if(defaultDelimOpt) {
      // If we're in a binary mode, don't allow the use of the default line delimiter.
      // Line delimination must be explicitly set.
      iInfo.cmdLineDelimiter = *defaultDelimOpt;
      iInfo.setResponseDelimiter(*defaultDelimOpt);
    }
    // DELIMITER
    keyStr = toStr(EnumType::DELIMITER);
    if(auto opt = caseInsensitiveGetValueOption(j, keyStr)) {
      // Don't set explicitlySetToReadLines, so if there's a readBytes, DELIMITER acts like COMMAND_DELIMITER
      iInfo.cmdLineDelimiter = *opt;
      iInfo.setResponseDelimiter(*opt);
    }

    // COMMAND_DELIMITER, override all other settings
    keyStr = toStr(EnumType::COMMAND_DELIMITER);
    if(auto opt = caseInsensitiveGetValueOption(j, keyStr)) {
      iInfo.cmdLineDelimiter = *opt;
    }

    // RESPONSE_DELIMITER, override all other settings
    keyStr = toStr(EnumType::RESPONSE_DELIMITER);
    if(auto opt = caseInsensitiveGetValueOption(j, keyStr)) {
      explicitlySetToReadLines = true;
      iInfo.setResponseDelimiter(*opt);
    }
    /*----------------------------------------------------------------------------------------------------------------*/
    // N_RESPONSE_LINES,
    keyStr = toStr(EnumType::N_RESPONSE_LINES);
    if(auto opt = caseInsensitiveGetValueOption(j, keyStr)) {
      explicitlySetToReadLines = true;
      int n = opt->get<int>();
      if(n < 0) {
        throw ChimeraTK::logic_error(FUNC_NAME + "Invalid negative " + toStr(EnumType::N_RESPONSE_LINES) + " " +
            std::to_string(n) + " for " + errorMessageDetail);
      }
      if(responseIsAbsent and (n != 0)) {
        throw ChimeraTK::logic_error(FUNC_NAME + "Response is absent but " + toStr(EnumType::N_RESPONSE_LINES) + " = " +
            std::to_string(n) + " for " + errorMessageDetail);
      }
      iInfo.setResponseNLines(
          static_cast<size_t>(n)); // Gets Overwritten if responseIsAbsent, yet ensures usesReadBytes() is true
    }
    /*----------------------------------------------------------------------------------------------------------------*/
    // N_RESPONSE_BYTES, Set to Binary Mode, destroying the previous delimiter and nLines settings
    keyStr = toStr(EnumType::N_RESPONSE_BYTES);
    if(auto opt = caseInsensitiveGetValueOption(j, keyStr)) {
      if(explicitlySetToReadLines) {
        throw ChimeraTK::logic_error(
            FUNC_NAME + "Invalid mixture of read-lines and read-bytes for " + errorMessageDetail);
      }

      int n = opt->get<int>();
      if(n < 0) {
        throw ChimeraTK::logic_error(
            FUNC_NAME + "Invalid negative " + keyStr + " " + std::to_string(n) + " for " + errorMessageDetail);
      }

      if(responseIsAbsent and (n != 0)) {
        throw ChimeraTK::logic_error(FUNC_NAME + "Response is absent but " + toStr(EnumType::N_RESPONSE_BYTES) + " = " +
            std::to_string(n) + " for " + errorMessageDetail);
      }

      iInfo.setResponseBytes(
          static_cast<size_t>(n)); // Gets Overwritten if responseIsAbsent, yet ensures usesReadLines() is true
    }
    /*----------------------------------------------------------------------------------------------------------------*/
    if(responseIsAbsent) {
      if(iInfo.usesReadLines()) {
        iInfo.setResponseNLines(0);
      }
      else if(iInfo.usesReadBytes()) {
        iInfo.setResponseBytes(0);
      }
    }
  } // end setEndingsFromJson

  /********************************************************************************************************************/

  template<typename EnumType>
  static void setFixedWidthFromJson(InteractionInfo& iInfo, const json& j, const std::string& errorMessageDetail) {
    std::string bitWidthKeyStr = toStr(EnumType::BIT_WIDTH);
    std::string charWidthKeyStr = toStr(EnumType::CHARACTER_WIDTH);
    auto bitWidthOpt = caseInsensitiveGetValueOption(j, bitWidthKeyStr);
    auto charWidthOpt = caseInsensitiveGetValueOption(j, charWidthKeyStr);

    if(not bitWidthOpt and not charWidthOpt) {
      return;
    }

    if(bitWidthOpt and charWidthOpt) {
      throw ChimeraTK::logic_error(
          FUNC_NAME + bitWidthKeyStr + " and " + charWidthKeyStr + " cannot both be set. See: " + errorMessageDetail);
    }

    int nRegexChars{};
    if(not iInfo.hasTransportLayerType()) {
      throw ChimeraTK::logic_error(FUNC_NAME + " A transportLayerType must be set." + errorMessageDetail);
    }
    auto type = iInfo.getTransportLayerType();
    if(bitWidthOpt) { // BIT_WIDTH
      int nBits = bitWidthOpt->get<int>();
      if(nBits <= 0) {
        throw ChimeraTK::logic_error(FUNC_NAME + "Invalid non-positive " + bitWidthKeyStr + " " +
            std::to_string(nBits) + " for " + errorMessageDetail);
      }
      if(nBits % 4 != 0) { // n bits must represent a nibble.
        throw ChimeraTK::logic_error(FUNC_NAME + "Invalid " + bitWidthKeyStr + "=" + std::to_string(nBits) +
            " must be a multiple of 4 bits. See: " + errorMessageDetail);
      }
      if((type == TransportLayerType::VOID) or (type == TransportLayerType::DEC_INT) or
          (type == TransportLayerType::STRING)) {
        throw ChimeraTK::logic_error(FUNC_NAME + "Invalid combination of " + bitWidthKeyStr + " and " +
            toStr(EnumType::TYPE) + " for " + errorMessageDetail + ". Did you mean " + charWidthKeyStr + "?");
      }
      nRegexChars = nBits / 4;
    }
    else if(charWidthOpt) { // CHARACTER_WIDTH
      if((type == TransportLayerType::VOID) or (type == TransportLayerType::BIN_INT) or
          (type == TransportLayerType::BIN_FLOAT) or (type == TransportLayerType::DEC_FLOAT)) {
        throw ChimeraTK::logic_error(FUNC_NAME + "Invalid combination of " + charWidthKeyStr + " and " +
            toStr(EnumType::TYPE) + " for " + errorMessageDetail + ". Did you mean " + bitWidthKeyStr + "?");
      }
      nRegexChars = charWidthOpt->get<int>();
      if(nRegexChars <= 0) {
        throw ChimeraTK::logic_error(FUNC_NAME + "Invalid non-positive " + charWidthKeyStr + " " +
            std::to_string(nRegexChars) + " for " + errorMessageDetail);
      }
      // Screen types
    }

    iInfo.fixedRegexCharacterWidthOpt = static_cast<size_t>(nRegexChars);
  } // end setFixedWidthFromJson

  /********************************************************************************************************************/

  template<typename EnumType>
  static void setFractionalBitsFromJson(InteractionInfo& iInfo, const json& j, const std::string& errorMessageDetail) {
    std::string keyStr = toStr(EnumType::FRACTIONAL_BITS);
    if(auto opt = caseInsensitiveGetValueOption(j, keyStr)) {
      iInfo.fractionalBitsOpt = opt->get<int>();
      return;
    }
    if(not iInfo.hasTransportLayerType()) {
      throw ChimeraTK::logic_error(FUNC_NAME + "A transportLayerType must be set." + errorMessageDetail);
    }
    auto type = iInfo.getTransportLayerType();
    if(((type == TransportLayerType::BIN_INT) or (type == TransportLayerType::HEX_INT)) and
        iInfo.fixedRegexCharacterWidthOpt.has_value()) {
      iInfo.fractionalBitsOpt = 0;
    }
  }

  /********************************************************************************************************************/

  template<typename EnumType>
  static void setSignedFromJson(InteractionInfo& iInfo, const json& j, const std::string& errorMessageDetail) {
    std::string keyStr = toStr(EnumType::SIGNED);
    if(auto opt = caseInsensitiveGetValueOption(j, keyStr)) {
      iInfo.isSigned = opt->get<bool>();
    }
    else { // if not set, default to sensible values given the transportLayerType
      if(not iInfo.hasTransportLayerType()) {
        throw ChimeraTK::logic_error(
            FUNC_NAME + " A transportLayerType or " + keyStr + " must be set." + errorMessageDetail);
      }

      TransportLayerType type = iInfo.getTransportLayerType();
      iInfo.isSigned = (type == TransportLayerType::DEC_INT or type == TransportLayerType::DEC_FLOAT or
          type == TransportLayerType::BIN_FLOAT);
    } // iInfo.isSigned defaults to value in InteractionInfo
  } // end setSignedFromJson

  /********************************************************************************************************************/

  // Operators for cout << InteractionInfo
  std::ostream& operator<<(std::ostream& os, const InteractionInfo& iInfo) {
    return os << "isActive: " << iInfo.isActive() << ", isBinary: " << iInfo.isBinary() << ", transportLayerType: "
              << (iInfo.hasTransportLayerType() ? toStr(iInfo.getTransportLayerType()) : std::string("not set"))
              << ", interpret patterns as hex: "
              << (iInfo.hasTransportLayerType() ?
                         ((iInfo.getTransportLayerType() == TransportLayerType::BIN_INT) or
                             (iInfo.getTransportLayerType() == TransportLayerType::BIN_FLOAT)) :
                         false)
              << ", isSigned: " << iInfo.isSigned <<
        // interpret as hex?
        ", commandPattern: \"" << replaceNewLines(iInfo.commandPattern) << "\", cmdLineDelimiter: \""
              << replaceNewLines(iInfo.cmdLineDelimiter) << "\", responsePattern: \""
              << replaceNewLines(iInfo.responsePattern)
              << "\", getResponseNLines: " << (iInfo.usesReadLines() ? (int)iInfo.getResponseNLines().value() : -1)
              << " getResponseLinesDelimiter: \""
              << (iInfo.usesReadLines() ? replaceNewLines(iInfo.getResponseLinesDelimiter().value()) : "nullopt")
              << "\", getResponseBytes: " << (iInfo.usesReadBytes() ? (int)iInfo.getResponseBytes().value() : -1)
              << ", fixedRegexCharacterWidthOpt: "
              << (iInfo.fixedRegexCharacterWidthOpt ? (int)iInfo.fixedRegexCharacterWidthOpt.value() : -1)
              << ", fractionalBitsOpt: " << (iInfo.fractionalBitsOpt ? (int)iInfo.fractionalBitsOpt.value() : -1);
  }

  /********************************************************************************************************************/

} // namespace ChimeraTK

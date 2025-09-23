// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "CommandBasedBackendRegisterInfo.h"

#include "Checksum.h"
#include "injaUtils.h"
#include "jsonUtils.h"
#include "mapFileKeys.h"

#include <array>
#include <map>
#include <optional>
#include <utility>

#define LOCATION (std::string(__FILE__) + "::" + std::string(__func__) + "::" + std::to_string(__LINE__))
#define ERR_LOC_PREFIX (LOCATION + ": ")

namespace ChimeraTK {

  /*
   * Gets a single DataType, if possible, that will work for both iInfoA and iInfoB.
   * The resulting data type will have the larger of the two bit depths needed to suit iInfoB.
   * If either of the iInfo's are signed and call for integers, the result will be a signed integer.
   * If the types cannot be reconciled, then nullopt is returned.
   */
  static std::optional<DataType> getReconciledDataTypes(const InteractionInfo& iInfoA, const InteractionInfo& iInfoB);

  /**
   * @brief Sets the iInfo.TransportLayerType from JSON, if present, with type checking.
   * This must be a template to accomdate keys at the register level and the interaction level.
   * @param[in] j nlohmann::json from the map file
   * @throws ChimeraTK::logic_error if the type in the JSON is invalid.
   */
  template<typename EnumType>
  static void setTypeFromJson(InteractionInfo& iInfo, const json& j);

  /**
   * @brief Sets the InteractionInfo's line delimiters, number of lines, and number of bytes, from the JSON,
   * This must be a template to accomdate keys at the register level and the interaction level.
   * @param[in] j nlohmann::json from the map file
   * @param[in] defaultDelimOpt Is a the default serial delimiter, such as that coming from the dmap file.
   * @throws ChimeraTK::logic_error if the delimiters, nLines, and nBytes are in an invalid combination
   */
  template<typename EnumType>
  static void setEndingsFromJson(InteractionInfo& iInfo, const json& j,
      const std::optional<std::string>& defaultDelimOpt, bool responseIsAbsent = false);

  /**
   * @brief Sets iInfo.fixedRegexCharacterWidthOpt from JSON.
   * This must be a template to accomdate keys at the register level and the interaction level.
   * This depends on the transportLayerType being set.
   * @param[in] j nlohmann::json from the map file
   * @throws ChimeraTK::logic_error if the JSON lists a value <= 0, or transportLayerType is not set.
   */
  template<typename EnumType>
  static void setFixedWidthFromJson(InteractionInfo& iInfo, const json& j);

  /**
   * @brief Sets iInfo.fractionalBitsOpt from JSON.
   * This must be a template to accomdate keys at the register level and the interaction level.
   * This depends on the transportLayerType being set, since it depends on iInfo.isBinary.
   * @param[in] j nlohmann::json from the map file
   * @throws ChimeraTK::logic_error if transportLayerType is not set.
   */
  template<typename EnumType>
  static void setFractionalBitsFromJson(InteractionInfo& iInfo, const json& j);

  /**
   * @brief Sets iInfo.signed from JSON.
   * This must be a template to accomdate keys at the register level and the interaction level.
   * Acceptable json values are "true", and "false".
   * If no vaue is set, this sets a default value based on the type, so
   * this depends on the transportLayerType being set.
   * @param[in] j nlohmann::json from the map file
   * @throws ChimeraTK::logic_error if the JSON lists unrecognizable value, or transportLayerType is not set.
   */
  template<typename EnumType>
  static void setSignedFromJson(InteractionInfo& iInfo, const json& j);

  /**
   * @brief Set all checksum relevent members in the iInfo.
   * @param[in] j nlohmann::json from the map file
   * @throws ChimeraTK::logic_error if there are undefined checksum identifiers in the map file, or if the
   * commandPattern's checksum tags are ill formed.
   */
  template<typename EnumType>
  static void setChecksumsFromJson(InteractionInfo& iInfo, const json& j);

  /********************************************************************************************************************/
  /********************************************************************************************************************/

  void CommandBasedBackendRegisterInfo::validate() const {
    throwIfBadActivation();
    writeInfo.throwIfTransportLayerTypesIsNotSet();
    readInfo.throwIfTransportLayerTypesIsNotSet();
    throwIfBadNElements();
    throwIfBadCommandAndResponsePatterns();

    writeInfo.throwIfBadEndings();
    writeInfo.throwIfBadFixedWidth();
    writeInfo.throwIfBadFractionalBits();
    writeInfo.throwIfBadSigned();
    writeInfo.throwIfBadChecksum(getNumberOfElementsImpl());

    readInfo.throwIfBadEndings();
    readInfo.throwIfBadFixedWidth();
    readInfo.throwIfBadFractionalBits();
    readInfo.throwIfBadSigned();
    readInfo.throwIfBadChecksum(getNumberOfElementsImpl());
  }

  /********************************************************************************************************************/
  void CommandBasedBackendRegisterInfo::setErrorMessageDetail() {
    _errorMessageDetail = "register " + getRegisterNameImpl();
    readInfo.setErrorMessageDetail(_errorMessageDetail);
    writeInfo.setErrorMessageDetail(_errorMessageDetail);
  }

  /********************************************************************************************************************/

  void CommandBasedBackendRegisterInfo::finalize() {
    validate();

    // Check that the data types are compatible and set dataDescriptor
    dataDescriptor = DataDescriptor(getDataType());
  } // end init

  /********************************************************************************************************************/

  CommandBasedBackendRegisterInfo::CommandBasedBackendRegisterInfo(
      const RegisterPath& registerPath_, InteractionInfo readInfo_, InteractionInfo writeInfo_, uint nElements_)
  : nElements(nElements_), registerPath(registerPath_), readInfo(std::move(readInfo_)),
    writeInfo(std::move(writeInfo_)) {
    setErrorMessageDetail();
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
    setErrorMessageDetail();
    auto readOpt = caseInsensitiveGetValueOption(j, toStr(mapFileRegisterKeys::READ));
    auto writeOpt = caseInsensitiveGetValueOption(j, toStr(mapFileRegisterKeys::WRITE));

    // Validate retister-level json keys
    throwIfHasInvalidJsonKeyCaseInsensitive(j, getMapForEnum<mapFileRegisterKeys>(), "Map file " + _errorMessageDetail);
    /*----------------------------------------------------------------------------------------------------------------*/
    // SET CONTENT BASED ON TOP-LEVEL JSON

    // N_ELEM,
    setNElementsFromJson(j);

    // TYPE
    setTypeFromJson<mapFileRegisterKeys>(readInfo, j);
    setTypeFromJson<mapFileRegisterKeys>(writeInfo, j);
    if(readOpt) {
      setTypeFromJson<mapFileInteractionInfoKeys>(readInfo, readOpt->get<json>());
    }
    if(writeOpt) {
      setTypeFromJson<mapFileInteractionInfoKeys>(writeInfo, writeOpt->get<json>());
    }
    throwIfATransportLayerTypeIsNotSet();
    synchronizeTransportLayerTypes();

    // DELIMITER, COMMAND_DELIMITER, RESPONSE_DELIMITER, N_RESPONSE_LINES, N_RESPONSE_BYTES
    writeInfo.setResponseNLines(0);
    setEndingsFromJson<mapFileRegisterKeys>(readInfo, j, defaultSerialDelimiter);
    setEndingsFromJson<mapFileRegisterKeys>(writeInfo, j, defaultSerialDelimiter);
    // NOTE: These delimiter settings may be overrided by populateFromJson below.

    // BIT_WIDTH, CHARACTER_WIDTH, depends on transportLayerType
    setFixedWidthFromJson<mapFileRegisterKeys>(readInfo, j);
    setFixedWidthFromJson<mapFileRegisterKeys>(writeInfo, j);

    // FRACTIONAL_BITS, depends on transportLayerType. These must come after setFixedWidthFromJson
    setFractionalBitsFromJson<mapFileRegisterKeys>(readInfo, j);
    setFractionalBitsFromJson<mapFileRegisterKeys>(writeInfo, j);

    // SIGNED, depends on transportLayerType
    setSignedFromJson<mapFileRegisterKeys>(readInfo, j);
    setSignedFromJson<mapFileRegisterKeys>(writeInfo, j);

    // READ,
    // Override settings from the top level based on the "read" key's contents
    if(readOpt) {
      readInfo.populateFromJson(readOpt->get<json>(), true);
    }

    // WRITE
    // Override settings from the top level based on the "write" key's contents
    if(writeOpt) {
      writeInfo.populateFromJson(writeOpt->get<json>(), true);
    }
    /*----------------------------------------------------------------------------------------------------------------*/
    // FIXME: extract the number of lines in write response from pattern; Ticket 13531

    finalize(); // performs data validation
  } // end constructor

  /********************************************************************************************************************/
  /********************************************************************************************************************/

  void InteractionInfo::populateFromJson(const json& j, bool skipSetType) {
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
      setTypeFromJson<mapFileInteractionInfoKeys>(*this, j);
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
    setEndingsFromJson<mapFileInteractionInfoKeys>(*this, j, std::nullopt, responseIsAbsent);

    // BIT_WIDTH, CHARACTER_WIDTH
    setFixedWidthFromJson<mapFileInteractionInfoKeys>(*this, j);

    // FRACTIONAL_BITS This must come after setFixedWidthFromJson
    setFractionalBitsFromJson<mapFileInteractionInfoKeys>(*this, j);

    // SIGNED
    setSignedFromJson<mapFileInteractionInfoKeys>(*this, j);

    // CMD_CHECKSUM, RESP_CHECKSUM
    setChecksumsFromJson<mapFileInteractionInfoKeys>(*this, j);
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

  std::string InteractionInfo::getRegexString() const {
    // Note, these regex's must be parentheses-bound capture groups
    TransportLayerType type = getTransportLayerType();

    std::string valueRegex{};
    if(fixedRegexCharacterWidthOpt) { // a fixedSizeNumberWidth is specified
      std::string width = std::to_string(*fixedRegexCharacterWidthOpt);
      if(type == TransportLayerType::DEC_INT) {
        valueRegex = "([+-]?[0-9]{" + width + "})";
      }
      else if(type == TransportLayerType::HEX_INT or type == TransportLayerType::BIN_FLOAT or
          type == TransportLayerType::BIN_INT) {
        valueRegex = "([0-9A-Fa-f]{" + width + "})";
      }
      else if(type == TransportLayerType::DEC_FLOAT) {
        valueRegex = "([+-]?[0-9]+\\.?[0-9]*)";
      }
      else if(type == TransportLayerType::STRING) {
        valueRegex = "(.{" + width + "})";
      }
      else if(type != TransportLayerType::VOID) {
        assert(!valueRegex.empty());
      }
    }
    else { // no fixedSizeNumberWidth is specified
      if(type == TransportLayerType::DEC_INT) {
        valueRegex = "([+-]?[0-9]+)";
      }
      else if(type == TransportLayerType::HEX_INT or type == TransportLayerType::BIN_FLOAT or
          type == TransportLayerType::BIN_INT) {
        valueRegex = "([0-9A-Fa-f]+)";
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
    }
    return valueRegex;
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

  void CommandBasedBackendRegisterInfo::throwIfBadActivation() const {
    if(not(readInfo.isActive() or writeInfo.isActive())) {
      throw ChimeraTK::logic_error(ERR_LOC_PREFIX + "A non-empty read:" + toStr(mapFileInteractionInfoKeys::COMMAND) +
          " or write " + toStr(mapFileInteractionInfoKeys::COMMAND) +
          " tags is required, and neither are present for " + _errorMessageDetail);
    }

    if(writeInfo.getTransportLayerType() == TransportLayerType::VOID) {
      if(readInfo.isActive() or not writeInfo.isActive()) {
        throw ChimeraTK::logic_error(ERR_LOC_PREFIX + "Void type must be write-only but has a " +
            toStr(mapFileRegisterKeys::READ) + " key for " + _errorMessageDetail);
      }
    }
  } // end throwIfBadActivation

  /********************************************************************************************************************/

  void CommandBasedBackendRegisterInfo::throwIfBadCommandAndResponsePatterns() const {
    auto throwIfResponsesWithoutCommands = [&](const InteractionInfo& iInfo) {
      bool lacksCommand = iInfo.commandPattern.empty();
      bool hasResponse = not iInfo.responsePattern.empty();
      if(hasResponse and lacksCommand) {
        throw ChimeraTK::logic_error(ERR_LOC_PREFIX + "A non-empty " + iInfo.readWriteStr() + " " +
            toStr(mapFileInteractionInfoKeys::RESPESPONSE) + " without a non-empty " + iInfo.readWriteStr() + " " +
            toStr(mapFileInteractionInfoKeys::COMMAND) + " for " + _errorMessageDetail);
      }
    };
    throwIfResponsesWithoutCommands(readInfo);
    throwIfResponsesWithoutCommands(writeInfo);

    if(writeInfo.getTransportLayerType() == TransportLayerType::VOID) {
      if(writeInfo.commandPattern.find("{{x") != std::string::npos) {
        throw ChimeraTK::logic_error(ERR_LOC_PREFIX + "Illegal inja template data tags in write " +
            toStr(mapFileInteractionInfoKeys::COMMAND) + " = \"" + writeInfo.commandPattern + "\" for void-type for " +
            _errorMessageDetail);
      }
    }

    // Alignment between the mark_count and nElements can be enforced by using non-capture groups: (?:   )
    auto throwIfWrongNCaptures = [&](const InteractionInfo& iInfo, size_t nMarks, size_t nMarksRequired) {
      if(nMarks != nMarksRequired) {
        throw ChimeraTK::logic_error(ERR_LOC_PREFIX + "Wrong number of capture groups " + std::to_string(nMarks) + "(" +
            std::to_string(nMarksRequired) + " required) in " + iInfo.readWriteStr() + " responsePattern \"" +
            iInfo.responsePattern + "\" for " + _errorMessageDetail);
      }
    };

    size_t nReadResponseMarks = getReadResponseDataRegex().mark_count();
    if(readInfo.isActive() and (readInfo.getTransportLayerType() != TransportLayerType::VOID)) {
      throwIfWrongNCaptures(readInfo, nReadResponseMarks, getNumberOfElementsImpl());
    }
    else {
      /* nElements = 1 when the type is void, even though no return marks are expected.
       * Also, if it's a write-only register, expect 0 reading marks
       */
      throwIfWrongNCaptures(readInfo, nReadResponseMarks, 0);
    }

    // Ensure no capture groups in the write response.
    size_t nWriteResponseMarks = getWriteResponseDataRegex().mark_count();
    throwIfWrongNCaptures(writeInfo, nWriteResponseMarks, 0);

  } // end throwIfBadCommandAndResponsePatterns

  /********************************************************************************************************************/

  void InteractionInfo::throwIfBadEndings() const {
    if(usesReadLines() and getResponseLinesDelimiter() == "") {
      throw ChimeraTK::logic_error(
          ERR_LOC_PREFIX + "Illegally set response delimiter to empty string for" + errorMessageDetail);
    }
  } // end throwIfBadEndings

  /********************************************************************************************************************/

  void CommandBasedBackendRegisterInfo::throwIfBadNElements() const {
    unsigned int nElem = getNumberOfElementsImpl();
    if(writeInfo.getTransportLayerType() == TransportLayerType::VOID) {
      if(nElem != 1) {
        throw ChimeraTK::logic_error(ERR_LOC_PREFIX + "Void type must only have 1 element but has " +
            toStr(mapFileRegisterKeys::N_ELEM) + " = " + std::to_string(nElem) + " for " + _errorMessageDetail);
      }
    }
    if(nElem == 0) {
      throw ChimeraTK::logic_error(
          ERR_LOC_PREFIX + "Invalid zero " + toStr(mapFileRegisterKeys::N_ELEM) + " for " + _errorMessageDetail);
    }
  } // end if void type

  /********************************************************************************************************************/

  void InteractionInfo::throwIfTransportLayerTypesIsNotSet() const {
    if(not(hasTransportLayerType())) {
      throw ChimeraTK::logic_error(ERR_LOC_PREFIX + "Type is required but is missing for " + errorMessageDetail);
    }
  }

  /********************************************************************************************************************/

  void CommandBasedBackendRegisterInfo::throwIfATransportLayerTypeIsNotSet() const {
    // Throw if type/transportLayerType is not set.
    if(not(writeInfo.hasTransportLayerType() or readInfo.hasTransportLayerType())) {
      throw ChimeraTK::logic_error(ERR_LOC_PREFIX + "Type is required but is missing for " + _errorMessageDetail);
    }
    if(readInfo.isActive() and not readInfo.hasTransportLayerType()) {
      throw ChimeraTK::logic_error(
          ERR_LOC_PREFIX + "Type is required but is missing on " + readInfo.errorMessageDetail);
    }
    if(writeInfo.isActive() and not writeInfo.hasTransportLayerType()) {
      throw ChimeraTK::logic_error(
          ERR_LOC_PREFIX + "Type is required but is missing on " + writeInfo.errorMessageDetail);
    }
  }

  /********************************************************************************************************************/

  void CommandBasedBackendRegisterInfo::synchronizeTransportLayerTypes() {
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

  void InteractionInfo::throwIfBadFractionalBits() const {
    if((not fractionalBitsOpt) or (not isActive())) {
      return;
    }
    if(not fixedRegexCharacterWidthOpt) {
      throw ChimeraTK::logic_error(ERR_LOC_PREFIX + toStr(mapFileRegisterKeys::FRACTIONAL_BITS) + " is set but " +
          toStr(mapFileRegisterKeys::BIT_WIDTH) + " is not set for " + errorMessageDetail);
    }
    if((getTransportLayerType() != TransportLayerType::BIN_INT) and
        (getTransportLayerType() != TransportLayerType::HEX_INT)) {
      throw ChimeraTK::logic_error(ERR_LOC_PREFIX + toStr(mapFileRegisterKeys::FRACTIONAL_BITS) +
          " is set for incompatible " + toStr(mapFileRegisterKeys::TYPE) + " " + toStr(getTransportLayerType()) +
          " for " + errorMessageDetail + " (only " + toStr(TransportLayerType::BIN_INT) + " and " +
          toStr(TransportLayerType::HEX_INT) + " are compatible)");
    }
    // case: fractionalBits is too big
    // TODO confirm these formulas .. can fractional bits really not exceed the size of the container?
    size_t width = *(fixedRegexCharacterWidthOpt);
    if(isSigned) {
      if(static_cast<int>(width * 4) < (fractionalBitsOpt.value() + 1)) {
        throw ChimeraTK::logic_error(ERR_LOC_PREFIX + toStr(mapFileRegisterKeys::FRACTIONAL_BITS) + " exceeds the " +
            toStr(mapFileRegisterKeys::BIT_WIDTH) + " minus the sign bit for " + errorMessageDetail);
      }
    }
    else {
      if(static_cast<int>(width * 4) < (fractionalBitsOpt.value())) {
        throw ChimeraTK::logic_error(ERR_LOC_PREFIX + toStr(mapFileRegisterKeys::FRACTIONAL_BITS) + " exceeds the " +
            toStr(mapFileRegisterKeys::BIT_WIDTH) + " for " + errorMessageDetail);
      }
    }
    // TODO if fractionalBits is super negative, throw
  }

  /********************************************************************************************************************/

  void InteractionInfo::throwIfBadFixedWidth() const {
    if(not isActive()) {
      return;
    }
    std::string bitWidthTag = toStr(mapFileRegisterKeys::BIT_WIDTH);
    std::string charWidthTag = toStr(mapFileRegisterKeys::CHARACTER_WIDTH);
    std::string eitherWidthTag = bitWidthTag + " or " + charWidthTag;
    if(not fixedRegexCharacterWidthOpt) {
      if(isBinary()) {
        throw ChimeraTK::logic_error(
            ERR_LOC_PREFIX + bitWidthTag + " must be set for binary type for " + errorMessageDetail);
      }
      return;
    }
    size_t regexCharacterWidth = *(fixedRegexCharacterWidthOpt);
    TransportLayerType type = getTransportLayerType();

    // VOID
    if(type == TransportLayerType::VOID) {
      throw ChimeraTK::logic_error(ERR_LOC_PREFIX + eitherWidthTag + " is set for " + toStr(type) + "=" +
          toStr(TransportLayerType::VOID) + " for " + errorMessageDetail);
    }

    if(regexCharacterWidth == 0) {
      throw ChimeraTK::logic_error(ERR_LOC_PREFIX + "Invalid zero " + eitherWidthTag + " for " + errorMessageDetail);
    }

    // FLOAT
    if(type == TransportLayerType::BIN_FLOAT or type == TransportLayerType::DEC_FLOAT) {
      std::array<size_t, 3> validWidths = {16, 8, 4}; // widths are in nibbles.

      if(std::find(validWidths.begin(), validWidths.end(), regexCharacterWidth) == validWidths.end()) {
        throw ChimeraTK::logic_error(ERR_LOC_PREFIX + "Invalid " + bitWidthTag + " " +
            std::to_string(4 * regexCharacterWidth) + " bits for type " + toStr(type) + " for " + errorMessageDetail);
      }
    }
    else if(type == TransportLayerType::DEC_INT) {
      /*Here regexCharacterWidth means the character width for the decimal representation
       * The widest possible DEC_INT that can fit in an int64 is  -9223372036854775808 (19 characters + sign char)
       * The widest possible DEC_INT that can fit in an uint64 is 18446744073709551615 (20 chars)
       */
      size_t maxWidth = (isSigned ? 19 : 20);
      if(regexCharacterWidth > maxWidth) {
        throw ChimeraTK::logic_error(ERR_LOC_PREFIX + "Invalid " + charWidthTag + " " +
            std::to_string(regexCharacterWidth) + " digits for type " + toStr(type) + " for " + errorMessageDetail +
            ". That cannot be fit into 64 bits. Allowed range is 1-" + std::to_string(maxWidth));
      }
    }
    // INT read as hexidecimal by the regex
    else if((type == TransportLayerType::HEX_INT) or (type == TransportLayerType::BIN_INT)) {
      // Here regexCharacterWidth means the number of hexideicimal characters (nibbles) for the transport layer representation.
      if(regexCharacterWidth > 16 /*16 nibbles = 64 bits*/) {
        throw ChimeraTK::logic_error(ERR_LOC_PREFIX + "Invalid " + bitWidthTag + " " +
            std::to_string(4 * regexCharacterWidth) + " bits for type " + toStr(type) + " for " + errorMessageDetail +
            ". That cannot be fit into 64 bits. Allowed range is 1-16.");
      }
    }
  } // end throwIfBadFixedWidth

  /********************************************************************************************************************/

  void InteractionInfo::throwIfBadSigned() const {
    if((not isSigned) or (not isActive())) {
      return;
    }

    TransportLayerType type = getTransportLayerType();
    if(type == TransportLayerType::VOID or type == TransportLayerType::STRING) {
      throw ChimeraTK::logic_error(ERR_LOC_PREFIX + toStr(mapFileRegisterKeys::TYPE) + " " + toStr(type) +
          "is signed for " + errorMessageDetail);
    }
  }

  /********************************************************************************************************************/

  /*
   * @brief Validates that the number of checksum entries matches the number of inja checksum payloads in the command
   * and response pattern.
   * @throws ChimeraTK::logic_error If there is a size missmatch, or an invalid checksum inja patterns.
   */
  void InteractionInfo::throwIfBadChecksum(const size_t nElements) const {
    size_t nCsMarks = getResponseChecksumRegex(nElements).mark_count();
    size_t nCsPayloadMarks = getResponseChecksumPayloadRegex(nElements).mark_count();
    // getNChecksums throws ChimeraTK::logic_error if the validates the checksum patterns are ill-formed..
    size_t nRespCS = getNChecksums(responsePattern, errorMessageDetail + "for response checksum");
    size_t nCmdCS = getNChecksums(commandPattern, errorMessageDetail + "for command checksum");

    // Verify that the number of checksum tags found in the patterns match the size of the checksum name array given in the map file.
    if(nRespCS != responseChecksumEnums.size()) {
      throw ChimeraTK::logic_error(ERR_LOC_PREFIX + "The number (" + std::to_string(responseChecksumEnums.size()) +
          ") of " + toStr(mapFileInteractionInfoKeys::RESP_CHECKSUM) + " entries does not match number (" +
          toStr(mapFileInteractionInfoKeys::RESP_CHECKSUM) + ") of checksum tags in the inja response pattern \"" +
          responsePattern + "\" for " + errorMessageDetail);
    }
    if(nCmdCS != commandChecksumEnums.size()) {
      throw ChimeraTK::logic_error(ERR_LOC_PREFIX + "The number (" + std::to_string(commandChecksumEnums.size()) +
          ") of " + toStr(mapFileInteractionInfoKeys::CMD_CHECKSUM) + " entries does not match number (" +
          toStr(mapFileInteractionInfoKeys::CMD_CHECKSUM) + ") of checksum tags in the inja command pattern \"" +
          commandPattern + "\" for " + errorMessageDetail);
    }

    // Verify that the modified regex for capturing checksum insertion points has the right number of captures.
    size_t nCS = commandChecksumEnums.size();
    if(nCsMarks != nCS) {
      throw ChimeraTK::logic_error(ERR_LOC_PREFIX + "The number of capture groups (" + std::to_string(nCsPayloadMarks) +
          ") mismatches the number of " + toStr(injaTemplatePatternKeys::CHECKSUM_POINT) + " checksum tags (" +
          std::to_string(nCS) + ") in responsePattern \"" + responsePattern + "\" for " + errorMessageDetail);
    }

    // Verify that the modified regex for capturing checksum payloads has the right number of captures.
    if(nCsPayloadMarks != nCS) {
      throw ChimeraTK::logic_error(ERR_LOC_PREFIX + "The number of capture groups (" + std::to_string(nCsPayloadMarks) +
          ") mismatches the number of " + toStr(injaTemplatePatternKeys::CHECKSUM_START) + "/" +
          toStr(injaTemplatePatternKeys::CHECKSUM_END) + "checksum payload tags (" + std::to_string(nCS) +
          ") in responsePattern \"" + responsePattern + "\" for " + errorMessageDetail);
    }
  } // end throwIfBadChecksum

  /********************************************************************************************************************/
  /********************************************************************************************************************/

  TransportLayerType InteractionInfo::getTransportLayerType() const {
    if(not hasTransportLayerType()) {
      throw ChimeraTK::logic_error(
          "Attempting to get a TransportLayerType that has not been set for " + errorMessageDetail);
    }
    return _transportLayerType.value();
  }

  /********************************************************************************************************************/

  static DataType getDataTypeFromSizeCode(int sizeCode, const std::map<DataType, int>& map) {
    for(const auto& [key, value] : map) {
      if(value == sizeCode) {
        return key;
      }
    }
    throw ChimeraTK::logic_error(ERR_LOC_PREFIX + " No DataType corresponds to size code " + std::to_string(sizeCode));
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

  DataType InteractionInfo::getDataType() const {
    const TransportLayerType type = getTransportLayerType();
    const auto& map = (isSigned ? signedTransportLayerTypeToDataTypeMap : unsignedTransportLayerTypeToDataTypeMap);
    auto it = map.find(type); // std::unordered_map<TransportLayerType, DataType>::const_iterator it;

    if(it == map.end()) {
      throw ChimeraTK::logic_error(ERR_LOC_PREFIX + "Type " + toStr(type) + " is not in " + (isSigned ? "" : "un") +
          "signedtransportLayerTypeToDataTypeMap. Fix it in mapFileKeys.h");
    }

    // Get the default DataType for this type and signed state
    DataType d = it->second;

    if(d == DataType::none) {
      throw ChimeraTK::logic_error(ERR_LOC_PREFIX +
          "Invalid DataType::none. Go fix the unsigned/signedTransportLayerTypeToDataTypeMap "
          "in mapFileKeys.h to not use 'none'.");
    }

    // Consider shrinking the DataType based on the width setting.
    if(fixedRegexCharacterWidthOpt) {
      size_t width = *fixedRegexCharacterWidthOpt;
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

  DataType CommandBasedBackendRegisterInfo::getDataType() const {
    assert(writeInfo.isActive() or readInfo.isActive());

    if(writeInfo.isActive() and not readInfo.isActive()) {
      return writeInfo.getDataType();
    }
    if(readInfo.isActive() and not writeInfo.isActive()) {
      return readInfo.getDataType();
    }
    // both are active, have to reconcile them.
    auto mergedDataType = getReconciledDataTypes(writeInfo, readInfo);
    if(not mergedDataType) {
      throw ChimeraTK::logic_error(
          ERR_LOC_PREFIX + "Read and Write have incompatible DataTypes for " + _errorMessageDetail);
    }
    return *mergedDataType;
  }

  /********************************************************************************************************************/

  static std::optional<DataType> getReconciledDataTypes(const InteractionInfo& iInfoA, const InteractionInfo& iInfoB) {
    DataType a = iInfoA.getDataType();
    DataType b = iInfoB.getDataType();

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

  void CommandBasedBackendRegisterInfo::setNElementsFromJson(const json& j) {
    std::string keyStr = toStr(mapFileRegisterKeys::N_ELEM);
    int64_t nElementsUnprotected = caseInsensitiveGetValueOr(j, keyStr, 1L);
    if(nElementsUnprotected < 1L) {
      throw ChimeraTK::logic_error(ERR_LOC_PREFIX + "Invalid non-positive " + keyStr + " " +
          std::to_string(nElementsUnprotected) + " for " + _errorMessageDetail);
    }
    if(nElementsUnprotected > static_cast<int64_t>(std::numeric_limits<unsigned int>::max())) {
      throw ChimeraTK::logic_error(ERR_LOC_PREFIX + keyStr +
          " is too large to fit in a uint: " + std::to_string(nElementsUnprotected) + " for " + _errorMessageDetail);
    }
    nElements = static_cast<unsigned int>(nElementsUnprotected);
  } // end setNElementsFromJson

  /********************************************************************************************************************/

  template<typename EnumType>
  static void setTypeFromJson(InteractionInfo& iInfo, const json& j) {
    std::string keyStr = toStr(EnumType::TYPE);
    std::optional<std::string> typeStrOpt = caseInsensitiveGetValueOption(j, keyStr);
    if(typeStrOpt) {
      std::optional<TransportLayerType> typeEnumOpt = strToEnumOpt<TransportLayerType>(*typeStrOpt);
      if(not typeEnumOpt) {
        throw ChimeraTK::logic_error(
            ERR_LOC_PREFIX + "Unknown value for " + keyStr + ": " + *typeStrOpt + " for " + iInfo.errorMessageDetail);
      }
      iInfo.setTransportLayerType(*typeEnumOpt);
    }
  } // end setTypeFromJson

  /********************************************************************************************************************/

  template<typename EnumType>
  static void setEndingsFromJson(InteractionInfo& iInfo, const json& j,
      const std::optional<std::string>& defaultDelimOpt, const bool responseIsAbsent) {
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
        throw ChimeraTK::logic_error(ERR_LOC_PREFIX + "Invalid negative " + toStr(EnumType::N_RESPONSE_LINES) + " " +
            std::to_string(n) + " for " + iInfo.errorMessageDetail);
      }
      if(responseIsAbsent and (n != 0)) {
        throw ChimeraTK::logic_error(ERR_LOC_PREFIX + "Response is absent but " + toStr(EnumType::N_RESPONSE_LINES) +
            " = " + std::to_string(n) + " for " + iInfo.errorMessageDetail);
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
            ERR_LOC_PREFIX + "Invalid mixture of read-lines and read-bytes for " + iInfo.errorMessageDetail);
      }

      int n = opt->get<int>();
      if(n < 0) {
        throw ChimeraTK::logic_error(ERR_LOC_PREFIX + "Invalid negative " + keyStr + " " + std::to_string(n) + " for " +
            iInfo.errorMessageDetail);
      }

      if(responseIsAbsent and (n != 0)) {
        throw ChimeraTK::logic_error(ERR_LOC_PREFIX + "Response is absent but " + toStr(EnumType::N_RESPONSE_BYTES) +
            " = " + std::to_string(n) + " for " + iInfo.errorMessageDetail);
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
  static void setFixedWidthFromJson(InteractionInfo& iInfo, const json& j) {
    std::string bitWidthKeyStr = toStr(EnumType::BIT_WIDTH);
    std::string charWidthKeyStr = toStr(EnumType::CHARACTER_WIDTH);
    auto bitWidthOpt = caseInsensitiveGetValueOption(j, bitWidthKeyStr);
    auto charWidthOpt = caseInsensitiveGetValueOption(j, charWidthKeyStr);

    if(not bitWidthOpt and not charWidthOpt) {
      return;
    }

    if(bitWidthOpt and charWidthOpt) {
      throw ChimeraTK::logic_error(ERR_LOC_PREFIX + bitWidthKeyStr + " and " + charWidthKeyStr +
          " cannot both be set. See: " + iInfo.errorMessageDetail);
    }

    int nRegexChars{};
    if(not iInfo.hasTransportLayerType()) {
      throw ChimeraTK::logic_error(ERR_LOC_PREFIX + " A transportLayerType must be set." + iInfo.errorMessageDetail);
    }
    auto type = iInfo.getTransportLayerType();
    if(bitWidthOpt) { // BIT_WIDTH
      int nBits = bitWidthOpt->get<int>();
      if(nBits <= 0) {
        throw ChimeraTK::logic_error(ERR_LOC_PREFIX + "Invalid non-positive " + bitWidthKeyStr + " " +
            std::to_string(nBits) + " for " + iInfo.errorMessageDetail);
      }
      if(nBits % 4 != 0) { // n bits must represent a nibble.
        throw ChimeraTK::logic_error(ERR_LOC_PREFIX + "Invalid " + bitWidthKeyStr + "=" + std::to_string(nBits) +
            " must be a multiple of 4 bits. See: " + iInfo.errorMessageDetail);
      }
      if((type == TransportLayerType::VOID) or (type == TransportLayerType::DEC_INT) or
          (type == TransportLayerType::STRING)) {
        throw ChimeraTK::logic_error(ERR_LOC_PREFIX + "Invalid combination of " + bitWidthKeyStr + " and " +
            toStr(EnumType::TYPE) + " for " + iInfo.errorMessageDetail + ". Did you mean " + charWidthKeyStr + "?");
      }
      nRegexChars = nBits / 4;
    }
    else if(charWidthOpt) { // CHARACTER_WIDTH
      if((type == TransportLayerType::VOID) or (type == TransportLayerType::BIN_INT) or
          (type == TransportLayerType::BIN_FLOAT) or (type == TransportLayerType::DEC_FLOAT)) {
        throw ChimeraTK::logic_error(ERR_LOC_PREFIX + "Invalid combination of " + charWidthKeyStr + " and " +
            toStr(EnumType::TYPE) + " for " + iInfo.errorMessageDetail + ". Did you mean " + bitWidthKeyStr + "?");
      }
      nRegexChars = charWidthOpt->get<int>();
      if(nRegexChars <= 0) {
        throw ChimeraTK::logic_error(ERR_LOC_PREFIX + "Invalid non-positive " + charWidthKeyStr + " " +
            std::to_string(nRegexChars) + " for " + iInfo.errorMessageDetail);
      }
      // Screen types
    }

    iInfo.fixedRegexCharacterWidthOpt = static_cast<size_t>(nRegexChars);
  } // end setFixedWidthFromJson

  /********************************************************************************************************************/

  template<typename EnumType>
  static void setFractionalBitsFromJson(InteractionInfo& iInfo, const json& j) {
    std::string keyStr = toStr(EnumType::FRACTIONAL_BITS);
    if(auto opt = caseInsensitiveGetValueOption(j, keyStr)) {
      iInfo.fractionalBitsOpt = opt->get<int>();
      return;
    }
    if(not iInfo.hasTransportLayerType()) {
      throw ChimeraTK::logic_error(ERR_LOC_PREFIX + "A transportLayerType must be set." + iInfo.errorMessageDetail);
    }
    auto type = iInfo.getTransportLayerType();
    if(((type == TransportLayerType::BIN_INT) or (type == TransportLayerType::HEX_INT)) and
        iInfo.fixedRegexCharacterWidthOpt.has_value()) {
      iInfo.fractionalBitsOpt = 0;
    }
  }

  /********************************************************************************************************************/

  template<typename EnumType>
  static void setSignedFromJson(InteractionInfo& iInfo, const json& j) {
    std::string keyStr = toStr(EnumType::SIGNED);
    if(auto opt = caseInsensitiveGetValueOption(j, keyStr)) {
      iInfo.isSigned = opt->get<bool>();
    }
    else { // if not set, default to sensible values given the transportLayerType
      if(not iInfo.hasTransportLayerType()) {
        throw ChimeraTK::logic_error(
            ERR_LOC_PREFIX + " A transportLayerType or " + keyStr + " must be set." + iInfo.errorMessageDetail);
      }

      TransportLayerType type = iInfo.getTransportLayerType();
      iInfo.isSigned = (type == TransportLayerType::DEC_INT or type == TransportLayerType::DEC_FLOAT or
          type == TransportLayerType::BIN_FLOAT);
    } // iInfo.isSigned defaults to value in InteractionInfo
  } // end setSignedFromJson

  /********************************************************************************************************************/

  template<typename EnumType>
  static void setChecksumsFromJson(InteractionInfo& iInfo, const json& j) {
    const std::string funcName = ERR_LOC_PREFIX; // keeps the linter happy
    auto processChecksumField = [&](const EnumType& key, const std::string& pattern) -> std::vector<checksum> {
      std::vector<checksum> checksumEnums;
      const std::string keyStr = toStr(key);
      if(auto opt = caseInsensitiveGetValueOption(j, keyStr)) {
        if(opt->is_array()) {
          for(const auto& csStrJson : *opt) {
            std::string csStr = csStrJson.get<std::string>();
            if(auto csEnumOpt = getEnumOptFromStrMapCaseInsensitive<checksum>(csStr, getMapForEnum<checksum>())) {
              checksumEnums.push_back(*csEnumOpt);
            }
            else {
              throw ChimeraTK::logic_error(
                  funcName + "Unknown value " + csStr + " for " + keyStr + " - " + iInfo.errorMessageDetail);
            }
          }
        }
        else if(opt->is_string()) {
          std::string csStr = opt->get<std::string>();
          if(auto csEnumOpt = getEnumOptFromStrMapCaseInsensitive<checksum>(csStr, getMapForEnum<checksum>())) {
            size_t nCS = getNChecksums(pattern, iInfo.errorMessageDetail);
            checksumEnums = std::vector<checksum>(nCS, *csEnumOpt);
          }
          else {
            throw ChimeraTK::logic_error(
                funcName + "Unknown value " + csStr + " for " + keyStr + " - " + iInfo.errorMessageDetail);
          }
        }
        else {
          throw ChimeraTK::logic_error(
              funcName + "Invalid non-array, non-string type for " + keyStr + " for " + iInfo.errorMessageDetail);
        }
      }
      return checksumEnums;
    };

    iInfo.commandChecksumEnums = processChecksumField(EnumType::CMD_CHECKSUM, iInfo.commandPattern);
    iInfo.responseChecksumEnums = processChecksumField(EnumType::RESP_CHECKSUM, iInfo.responsePattern);
    iInfo.commandChecksumPayloadStrs = getChecksumPayloadSnippets(iInfo.commandPattern, iInfo.errorMessageDetail);
  } // end setChecksumsFromJson

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

  std::regex InteractionInfo::getResponseDataRegex(const size_t nElements) const {
    std::string valueRegex = getRegexString(); // A capture group

    inja::json replacePatterns;
    replacePatterns[toStr(injaTemplatePatternKeys::DATA)] = {};
    for(size_t i = 0; i < nElements; ++i) {
      // NumberOfElements may be an overestimate of how many are needed.
      // FIXME: does not know about formating. TODO ticket 13534. See below..
      replacePatterns[toStr(injaTemplatePatternKeys::DATA)].push_back(valueRegex); // render data capture groups
    }

    // Fill checksum components
    if(not commandChecksumEnums.empty()) {
      replacePatterns[toStr(injaTemplatePatternKeys::CHECKSUM_START)] = {};
      replacePatterns[toStr(injaTemplatePatternKeys::CHECKSUM_END)] = {};
      replacePatterns[toStr(injaTemplatePatternKeys::CHECKSUM_POINT)] = {};
      for(const auto& cs : commandChecksumEnums) {
        // render the checksum start and end tags as empty.
        replacePatterns[toStr(injaTemplatePatternKeys::CHECKSUM_START)].push_back("");
        replacePatterns[toStr(injaTemplatePatternKeys::CHECKSUM_END)].push_back("");
        // render checksum insertion point tags as non-capture groups
        replacePatterns[toStr(injaTemplatePatternKeys::CHECKSUM_POINT)].push_back(
            toNonCaptureGroupPattern(ChimeraTK::getRegexString(cs)));
      } // end for
    }

    return injaRenderRegex(
        responsePattern, replacePatterns, "in response data pattern for " + errorMessageDetail + " from " + LOCATION);

  } // end getResponseDataRegex

  /********************************************************************************************************************/

  std::regex InteractionInfo::getResponseChecksumRegex(const size_t nElements) const {
    std::string valueRegex = toNonCaptureGroupPattern(getRegexString());

    inja::json replacePatterns;
    replacePatterns[toStr(injaTemplatePatternKeys::DATA)] = {};
    for(size_t i = 0; i < nElements; ++i) {
      // nElements may be an overestimate of how many are needed.
      // FIXME: does not know about formating. TODO ticket 13534. See below..
      replacePatterns[toStr(injaTemplatePatternKeys::DATA)].push_back(valueRegex); // render data NON-capture groups
    }

    // Fill checksum components
    size_t nCS = commandChecksumEnums.size();
    if(nCS > 0) {
      replacePatterns[toStr(injaTemplatePatternKeys::CHECKSUM_START)] = {};
      replacePatterns[toStr(injaTemplatePatternKeys::CHECKSUM_END)] = {};
      replacePatterns[toStr(injaTemplatePatternKeys::CHECKSUM_POINT)] = {};
      for(const auto& cs : commandChecksumEnums) {
        // render the checksum start and end tags as empty.
        replacePatterns[toStr(injaTemplatePatternKeys::CHECKSUM_START)].push_back("");
        replacePatterns[toStr(injaTemplatePatternKeys::CHECKSUM_END)].push_back("");
        // render checksum insertion point tags as capture groups
        replacePatterns[toStr(injaTemplatePatternKeys::CHECKSUM_POINT)].push_back(ChimeraTK::getRegexString(cs));
      } // end for
    }

    return injaRenderRegex(responsePattern, replacePatterns,
        "in response checksum pattern for " + errorMessageDetail + " from " + LOCATION);
  } // end getResponseChecksumRegex

  /********************************************************************************************************************/

  std::regex InteractionInfo::getResponseChecksumPayloadRegex(const size_t nElements) const {
    std::string valueRegex = toNonCaptureGroupPattern(getRegexString());

    inja::json replacePatterns;
    replacePatterns[toStr(injaTemplatePatternKeys::DATA)] = {};
    for(size_t i = 0; i < nElements; ++i) {
      // getNumberOfElements() may be an overestimate of how many are needed.
      // FIXME: does not know about formating. TODO ticket 13534. See below..
      replacePatterns[toStr(injaTemplatePatternKeys::DATA)].push_back(valueRegex); // render data NON-capture groups
    }

    // Fill checksum components
    size_t nCS = commandChecksumEnums.size();
    if(nCS > 0) {
      replacePatterns[toStr(injaTemplatePatternKeys::CHECKSUM_START)] = {};
      replacePatterns[toStr(injaTemplatePatternKeys::CHECKSUM_END)] = {};
      replacePatterns[toStr(injaTemplatePatternKeys::CHECKSUM_POINT)] = {};
      for(const auto& cs : commandChecksumEnums) {
        // render the checksum start and end tags as the beginning and end of regex capture groups
        replacePatterns[toStr(injaTemplatePatternKeys::CHECKSUM_START)].push_back("(");
        replacePatterns[toStr(injaTemplatePatternKeys::CHECKSUM_END)].push_back(")");
        // render checksum insertion point tags as non-capture groups
        replacePatterns[toStr(injaTemplatePatternKeys::CHECKSUM_POINT)].push_back(
            toNonCaptureGroupPattern(ChimeraTK::getRegexString(cs)));
      } // end for
    }

    return injaRenderRegex(responsePattern, replacePatterns,
        "in " + readWriteStr() + " response checksum payload pattern for " + errorMessageDetail + " from " + LOCATION);

  } // end getResponseChecksumPayloadRegex

  /********************************************************************************************************************/

} // namespace ChimeraTK

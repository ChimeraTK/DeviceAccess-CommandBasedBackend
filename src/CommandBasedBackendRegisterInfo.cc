// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "CommandBasedBackendRegisterInfo.h"

#include "jsonUtils.h"
#include "mapFileKeys.h"

#include <optional>
#include <utility>

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
   * @brief Validates nElements, particularly its interaction with VOID type.
   * Enforces that void type registers must be write-only, have nElements = 1, and have no inja variables in their commands.
   * @throws ChimeraTK::logic_error If nElements is incompatible with void type.
   */
  static void throwIfBadNElements(
      const InteractionInfo& writeInfo, unsigned int nElem, const std::string& errorMessageDetail);

  /**
   * @brief Throws unless at least one of the two interaction Infos has a type set.
   * @param[in] errorMessageDetail Specifies the registerPath, and maybe other details to orient error messages.
   * @throws ChimeraTK::logic_error If no type has been set.
   */
  static void throwIfTransportLayerTypeAreNotSet(
      InteractionInfo& writeInfo, InteractionInfo& readInfo, const std::string& errorMessageDetail);

  /**
   * @brief If one InteractionInfo is missing its TransportLayerType, the type is copied from the other InteractionInfo
   * The InteractionInfo is expected to lack a transport type if it is never activated, but the type is needed
   * Doing this synchronization simplifies later logic, such as throwIfBadNElements.
   */
  static void synchronizeTransportLayerTypes(InteractionInfo& writeInfo, InteractionInfo& readInfo) noexcept;

  /**
   * @brief Gets a single coherent data type from the two possible data types in writeInfo and readInfo, for the sake of
   * setting the DataDescriptor.
   * @param[in] errorMessageDetail Specifies the registerPath, and maybe other details to orient error messages.
   * @throws ChimeraTK::logic_error if the readInfo and writeInfo have incompatible data types.
   */
  static DataType getDataType(
      const InteractionInfo& writeInfo, const InteractionInfo& readInfo, const std::string& errorMessageDetail);

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
      const std::optional<std::string> defaultDelimOpt, const std::string& errorMessageDetail);

  /**
   * @brief Sets iInfo.fixedSizeNumberWidthOpt from JSON.
   * This must be a template to accomdate keys at the register level and the interaction level.
   * @param[in] j nlohmann::json from the map file
   * @param[in] errorMessageDetail Specifies the registerPath, and maybe other details to orient error messages.
   * @throws ChimeraTK::logic_error if the JSON lists a value <= 0.
   */
  template<typename EnumType>
  static void setFixedWidthFromJson(InteractionInfo& iInfo, const json& j, const std::string& errorMessageDetail);

  /********************************************************************************************************************/
  /********************************************************************************************************************/

  void CommandBasedBackendRegisterInfo::init() {
    std::string errorMessageDetail = "register " + registerPath;
    /*----------------------------------------------------------------------------------------------------------------*/
    // Validite data in readInfo and writeInfo
    throwIfBadActivation(writeInfo, readInfo, errorMessageDetail);
    throwIfTransportLayerTypeAreNotSet(writeInfo, readInfo, errorMessageDetail);
    synchronizeTransportLayerTypes(writeInfo, readInfo);
    throwIfBadCommandAndResponsePatterns(writeInfo, readInfo, errorMessageDetail);
    throwIfBadNElements(writeInfo, getNumberOfElements(), errorMessageDetail);
    /*----------------------------------------------------------------------------------------------------------------*/
    // Check that the data types are compatible and set dataDescriptor
    dataDescriptor = DataDescriptor(getDataType(writeInfo, readInfo, errorMessageDetail));
  } // end init

  /********************************************************************************************************************/

  static bool registerPathIsEmpty(const RegisterPath& registerPath) noexcept {
    return (std::string(registerPath).empty() or (std::string(registerPath) == "/"));
  }

  /********************************************************************************************************************/

  CommandBasedBackendRegisterInfo::CommandBasedBackendRegisterInfo(
      const RegisterPath& registerPath_, InteractionInfo readInfo_, InteractionInfo writeInfo_, uint nElements_)
  : nElements(nElements_), registerPath(registerPath_), readInfo(std::move(readInfo_)),
    writeInfo(std::move(writeInfo_)) {
    if(not registerPathIsEmpty(registerPath)) {
      // Don't validate data if CommandBasedBackendRegisterInfo is initalized as an empty placeholder.
      init();
    }
  }

  /********************************************************************************************************************/

  CommandBasedBackendRegisterInfo::CommandBasedBackendRegisterInfo(
      const RegisterPath& registerPath_, const json& j, const std::string& defaultSerialDelimiter)
  : registerPath(registerPath_) {
    /*
     * Here we extract data from the json,
     * Checking only that we don't have invalid json keys or unpassable negative numbers.
     * Then call init() for data validation and tasks common to all constructors.
     */
    std::string errorMessageDetail = "register " + registerPath;

    // Validate retister-level json keys
    throwIfHasInvalidJsonKeyCaseInsensitive(
        j, getMapForEnum<mapFileRegisterKeys>(), "Map file " + errorMessageDetail + " has unknown key");
    /*----------------------------------------------------------------------------------------------------------------*/
    // SET CONTENT BASED ON TOP-LEVEL JSON

    // N_ELEM,
    setNElementsFromJson(*this, j, errorMessageDetail);

    // TYPE
    setTypeFromJson<mapFileRegisterKeys>(readInfo, j, errorMessageDetail);
    setTypeFromJson<mapFileRegisterKeys>(writeInfo, j, errorMessageDetail);

    // DELIMITER, COMMAND_DELIMITER, RESPONSE_DELIMITER, N_RESPONSE_LINES, N_RESPONSE_BYTES
    setEndingsFromJson<mapFileRegisterKeys>(readInfo, j, defaultSerialDelimiter, errorMessageDetail);
    setEndingsFromJson<mapFileRegisterKeys>(writeInfo, j, defaultSerialDelimiter, errorMessageDetail);
    // NOTE: These delimiter settings may be overrided by populateFromJson below.

    // FIXED_SIZE_NUM_WIDTH,
    setFixedWidthFromJson<mapFileRegisterKeys>(readInfo, j, errorMessageDetail);
    setFixedWidthFromJson<mapFileRegisterKeys>(writeInfo, j, errorMessageDetail);

    // READ,
    // Override settings from the top level based on the "read" key's contents
    if(auto readOpt = caseInsensitiveGetValueOption(j, toStr(mapFileRegisterKeys::READ))) {
      readInfo.populateFromJson(readOpt->get<json>(), errorMessageDetail + " read");
    }

    // WRITE
    // Override settings from the top level based on the "write" key's contents
    if(auto writeOpt = caseInsensitiveGetValueOption(j, toStr(mapFileRegisterKeys::WRITE))) {
      writeInfo.populateFromJson(writeOpt->get<json>(), errorMessageDetail + " write");
    }
    /*----------------------------------------------------------------------------------------------------------------*/
    // FIXME: extract the number of lines in write response from pattern; Ticket 13531

    init(); // data validation
  }         // end constructor

  /********************************************************************************************************************/
  /********************************************************************************************************************/

  void InteractionInfo::populateFromJson(const json& j, const std::string& errorMessageDetail, bool skipSetType) {
    // This is not just a constructor because we want to fill in json

    // Validate json keys at the InteractionInfo level
    throwIfHasInvalidJsonKeyCaseInsensitive(j, getMapForEnum<mapFileInteractionInfoKeys>(),
        "Map file Interaction has unknown key for " + errorMessageDetail);

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
    if(auto opt = caseInsensitiveGetValueOption(j, toStr(mapFileInteractionInfoKeys::RESPESPONSE))) {
      responsePattern = opt->get<std::string>();
    }

    // DELIMITER, COMMAND_DELIMITER, RESPONSE_DELIMITER, N_RESPONSE_LINES, N_RESPONSE_BYTES
    setEndingsFromJson<mapFileInteractionInfoKeys>(*this, j, std::nullopt, errorMessageDetail);

    // FIXED_SIZE_NUM_WIDTH,
    setFixedWidthFromJson<mapFileInteractionInfoKeys>(*this, j, errorMessageDetail);
  } // populateFromJson

  /********************************************************************************************************************/

  std::optional<size_t> InteractionInfo::getResponseNLines() const {
    if(usesReadLines()) {
      return std::get<ResponseLinesInfo>(responseInfo).nLines;
    }
    return std::nullopt;
  }

  /********************************************************************************************************************/

  std::optional<std::string> InteractionInfo::getResponseLinesDelimiter() const {
    if(usesReadLines()) {
      return std::get<ResponseLinesInfo>(responseInfo).delimiter;
    }
    return std::nullopt;
  }

  /********************************************************************************************************************/

  std::optional<size_t> InteractionInfo::getResponseBytes() const {
    if(usesReadBytes()) {
      return std::get<ResponseBytesInfo>(responseInfo).nBytesReadResponse;
    }
    return std::nullopt;
  }

  /********************************************************************************************************************/

  void InteractionInfo::setResponseDelimiter(std::string delimiter) {
    if(not usesReadLines()) {
      responseInfo = ResponseLinesInfo{};
    }
    std::get<ResponseLinesInfo>(responseInfo).delimiter = delimiter;
  }

  /********************************************************************************************************************/

  void InteractionInfo::setResponseNLines(size_t nLines) {
    if(not usesReadLines()) {
      responseInfo = ResponseLinesInfo{};
    }
    std::get<ResponseLinesInfo>(responseInfo).nLines = nLines;
  }

  /********************************************************************************************************************/
  void InteractionInfo::setTransportLayerType(TransportLayerType& type) noexcept {
    transportLayerType = type;
    _isBinary = ((type == TransportLayerType::BIN_INT) or (type == TransportLayerType::BIN_FLOAT));
  }

  /********************************************************************************************************************/
  /********************************************************************************************************************/

  static void throwIfBadActivation(
      const InteractionInfo& writeInfo, const InteractionInfo& readInfo, const std::string& errorMessageDetail) {
    bool readable = readInfo.isActive();
    bool writeable = writeInfo.isActive();

    if(not(readable or writeable)) {
      throw ChimeraTK::logic_error("throwIfBadActivation: A non-empty read:" +
          toStr(mapFileInteractionInfoKeys::COMMAND) + " or write " + toStr(mapFileInteractionInfoKeys::COMMAND) +
          " tags is required, and neither are present for " + errorMessageDetail);
    }

    if(writeInfo.getTransportLayerType() == TransportLayerType::VOID) {
      if(readInfo.isActive() or not writeInfo.isActive()) {
        throw ChimeraTK::logic_error("throwIfBadActivation: Void type must be write-only but has a " +
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
      throw ChimeraTK::logic_error("throwIfBadCommandAndResponsePatterns: A non-empty read " +
          toStr(mapFileInteractionInfoKeys::RESPESPONSE) + " without a non-empty read " +
          toStr(mapFileInteractionInfoKeys::COMMAND) + " for " + errorMessageDetail);
    }

    if(hasWriteResponse and lacksWriteCommand) {
      throw ChimeraTK::logic_error("throwIfBadCommandAndResponsePatterns: A non-empty write " +
          toStr(mapFileInteractionInfoKeys::RESPESPONSE) + " without a non-empty write " +
          toStr(mapFileInteractionInfoKeys::COMMAND) + " for " + errorMessageDetail);
    }

    if(writeInfo.getTransportLayerType() == TransportLayerType::VOID) {
      if(writeInfo.commandPattern.find("{{x") != std::string::npos) {
        throw ChimeraTK::logic_error("throwIfBadCommandAndResponsePatterns: Illegal inja template in write " +
            toStr(mapFileInteractionInfoKeys::COMMAND) + " = \"" + writeInfo.commandPattern + "\" for void-type for " +
            errorMessageDetail);
      }
    }
  } // end throwIfBadCommandAndResponsePatterns

  /********************************************************************************************************************/

  static void throwIfBadNElements(
      const InteractionInfo& writeInfo, unsigned int nElem, const std::string& errorMessageDetail) {
    if(writeInfo.getTransportLayerType() == TransportLayerType::VOID) {
      if(nElem != 1) {
        throw ChimeraTK::logic_error("throwIfBadNElements: Void type must only have 1 element but has " +
            toStr(mapFileRegisterKeys::N_ELEM) + " = " + std::to_string(nElem) + " for " + errorMessageDetail);
      }
    }
  } // end if void type

  /********************************************************************************************************************/

  static void throwIfTransportLayerTypeAreNotSet(
      InteractionInfo& writeInfo, InteractionInfo& readInfo, const std::string& errorMessageDetail) {
    // Throw if type/transportLayerType is not set.
    if(not(writeInfo.hasTransportLayerType() or readInfo.hasTransportLayerType())) {
      throw ChimeraTK::logic_error(
          "throwIfTransportLayerTypeAreNotSet: Type is required but is missing for " + errorMessageDetail);
    }
    if(readInfo.isActive() and not readInfo.hasTransportLayerType()) {
      throw ChimeraTK::logic_error(
          "throwIfTransportLayerTypeAreNotSet: Type is required but is missing on read for " + errorMessageDetail);
    }
    if(writeInfo.isActive() and not writeInfo.hasTransportLayerType()) {
      throw ChimeraTK::logic_error(
          "throwIfTransportLayerTypeAreNotSet: Type is required but is missing on write for " + errorMessageDetail);
    }
  }

  /********************************************************************************************************************/

  static void synchronizeTransportLayerTypes(InteractionInfo& writeInfo, InteractionInfo& readInfo) noexcept {
    if(readInfo.hasTransportLayerType() and not writeInfo.hasTransportLayerType()) {
      TransportLayerType type = readInfo.getTransportLayerType();
      writeInfo.setTransportLayerType(type);
    }
    else if(writeInfo.hasTransportLayerType() and not readInfo.hasTransportLayerType()) {
      TransportLayerType type = writeInfo.getTransportLayerType();
      readInfo.setTransportLayerType(type);
    }
    }
  }

  /********************************************************************************************************************/

  static DataType getDataType(
      const InteractionInfo& writeInfo, const InteractionInfo& readInfo, const std::string& errorMessageDetail) {
    assert(writeInfo.isActive() or readInfo.isActive());

    if(writeInfo.isActive()) {
      DataType writeDataType = getDataTypeFromTransportLayerType(writeInfo.getTransportLayerType());

      // If readable and writeable, but incompatible data types, throw.
      if(readInfo.isActive()) {
        DataType readDataType = getDataTypeFromTransportLayerType(readInfo.getTransportLayerType());
        if(writeDataType != readDataType) {
          throw ChimeraTK::logic_error("Read and Write have incompatible DataTypes for " + errorMessageDetail);
        }
      }
      return writeDataType;
    }
    else { // read-only, readInfo must be active.
      DataType readDataType = getDataTypeFromTransportLayerType(readInfo.getTransportLayerType());
      return readDataType;
    }
  }

  /********************************************************************************************************************/

  static void setNElementsFromJson(
      CommandBasedBackendRegisterInfo& rInfo, const json& j, const std::string& errorMessageDetail) {
    std::string keyStr = toStr(mapFileRegisterKeys::N_ELEM);
    long nElementsUnprotected = caseInsensitiveGetValueOr(j, keyStr, static_cast<long>(1));
    if(nElementsUnprotected < 1) {
      throw ChimeraTK::logic_error(
          "Invalid non-positive " + keyStr + " " + std::to_string(nElementsUnprotected) + " for " + errorMessageDetail);
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
        throw ChimeraTK::logic_error("Unknown value for " + keyStr + ": " + *typeStrOpt + " for " + errorMessageDetail);
      }
      iInfo.setTransportLayerType(*typeEnumOpt);
    }
  } // end setTypeFromJson

  /********************************************************************************************************************/

  template<typename EnumType>
  static void setEndingsFromJson(InteractionInfo& iInfo, const json& j,
      const std::optional<std::string> defaultDelimOpt, const std::string& errorMessageDetail) {
    bool explicitlySetToReadLines = false;
    bool explicitlySetCmdDelimiter = false;
    std::string keyStr;
    /*----------------------------------------------------------------------------------------------------------------*/
    if(defaultDelimOpt) {
      iInfo.cmdLineDelimiter = *defaultDelimOpt;
      iInfo.setResponseDelimiter(*defaultDelimOpt);
    }
    // DELIMITER
    keyStr = toStr(EnumType::DELIMITER);
    if(auto opt = caseInsensitiveGetValueOption(j, keyStr)) {
      explicitlySetCmdDelimiter = true;
      // Don't set explicitlySetToReadLines, so if there's a readBytes, DELIMITER acts like COMMAND_DELIMITER
      iInfo.cmdLineDelimiter = *opt;
      iInfo.setResponseDelimiter(*opt);
    }

    // COMMAND_DELIMITER, override all other settings
    keyStr = toStr(EnumType::COMMAND_DELIMITER);
    if(auto opt = caseInsensitiveGetValueOption(j, keyStr)) {
      explicitlySetCmdDelimiter = true;
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
        throw ChimeraTK::logic_error("Invalid negative " + toStr(EnumType::N_RESPONSE_LINES) + " " + std::to_string(n) +
            " for " + errorMessageDetail);
      }
      iInfo.setResponseNLines(static_cast<size_t>(n));
    }
    /*----------------------------------------------------------------------------------------------------------------*/
    // N_RESPONSE_BYTES, Set to Binary Mode, destroying the previous delimiter and nLines settings
    keyStr = toStr(EnumType::N_RESPONSE_BYTES);
    if(auto opt = caseInsensitiveGetValueOption(j, keyStr)) {
      if(explicitlySetToReadLines) {
        throw ChimeraTK::logic_error("Invalid mixture of read-lines and read-bytes for " + errorMessageDetail);
      }

      int n = opt->get<int>();
      if(n < 0) {
        throw ChimeraTK::logic_error(
            "Invalid negative " + keyStr + " " + std::to_string(n) + " for " + errorMessageDetail);
      }
      iInfo.setResponseBytes(static_cast<size_t>(n));

      // If the Command delimiter was not explicitly set, presume we are sending binary with no delimiter.
      // This overrides the metadata and Register level delimiter settings.
      if(not explicitlySetCmdDelimiter) {
        iInfo.cmdLineDelimiter = "";
      }
    }
  } // end setEndingsFromJson

  /********************************************************************************************************************/

  template<typename EnumType>
  static void setFixedWidthFromJson(InteractionInfo& iInfo, const json& j, const std::string& errorMessageDetail) {
    std::string keyStr = toStr(EnumType::FIXED_SIZE_NUM_WIDTH);
    if(auto opt = caseInsensitiveGetValueOption(j, keyStr)) {
      int n = std::stoi(opt->get<std::string>());
      if(n <= 0) {
        throw ChimeraTK::logic_error(
            "Invalid non-positive " + keyStr + " " + std::to_string(n) + " for " + errorMessageDetail);
      }
      iInfo.fixedSizeNumberWidthOpt = static_cast<size_t>(n);
    }
  } // end setFixedWidthFromJson

  /********************************************************************************************************************/

} // namespace ChimeraTK

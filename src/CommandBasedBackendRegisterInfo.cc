// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "CommandBasedBackendRegisterInfo.h"

#include "jsonUtils.h"
#include "mapFileKeys.h"

#include <utility>

namespace ChimeraTK {

  using InteractionInfo = CommandBasedBackendRegisterInfo::InteractionInfo;

  static void throwIfInvalidCommandAndResponce(
      const InteractionInfo& writeInfo, const InteractionInfo& readInfo, const std::string& errorMessageDetail);

  static void throwIfInvalidVoidType(
      const InteractionInfo& writeInfo, const InteractionInfo& readInfo, const std::string& errorMessageDetail);

  // Morphs writeInfo and readInfo, throws logic_error
  static void ensureTransportLayerTypeAreSet(
      InteractionInfo& writeInfo, InteractionInfo& readInfo, const std::string errorMessageDetail);

  static DataType getDataType(
      const InteractionInfo& writeInfo, const InteractionInfo& readInfo, const std::string& errorMessageDetail);

  /********************************************************************************************************************/
  void CommandBasedBackendRegisterInfo::init() {
    // Check the validity of the data in readInfo and writeInfo
    std::string errorMessageDetail = "register " + regKey;

    throwIfInvalidCommandAndResponce(writeInfo, readInfo, errorMessageDetail); // ensures readable or writeable.

    ensureTransportLayerTypeAreSet(writeInfo, readInfo, errorMessageDetail);

    throwIfInvalidVoidType(writeInfo, readInfo, nElements, errorMessageDetail);

    // Check that the data types are compatible and set dataDescriptor
    dataDescriptor = DataDescriptor(getDataType(writeInfo, readInfo, errorMessageDetail));
  }

  /********************************************************************************************************************/

  CommandBasedBackendRegisterInfo::CommandBasedBackendRegisterInfo(
      const RegisterPath& registerPath_, InteractionInfo readInfo_, InteractionInfo writeInfo_, uint nElements_)
  : nElements(nElements_), registerPath(registerPath_), readInfo(std::move(readInfo_)),
    writeInfo(std::move(writeInfo_)) {
    init();
  }

  /********************************************************************************************************************/

  CommandBasedBackendRegisterInfo::CommandBasedBackendRegisterInfo(const json& j, const std::string& regKey) {
    // regKey is the key (register path) whose value is j. This is needed information for debugging.

    throwIfHasInvalidJsonKeyCaseInsensitive(
        j, registerKeyStrs, "Map file registry entry " + regKey + " has unknown key");

    CommandBasedBackendRegisterInfo::InteractionInfo readInfo;
    CommandBasedBackendRegisterInfo::InteractionInfo writeInfo;

    // SET CONTENT BASED ON TOP-LEVEL JSON
    // N_ELEM,
    unsigned int nElem = caseInsensitiveGetValueOr(j, toStr(N_ELEM), static_cast<unsigned int>(1));

    // TYPE
    auto typeStrOption = caseInsensitiveGetValueOption(j, toStr(TYPE));
    if(typeStrOption) { // If type is set at the top level
      std::string typeValue = typeStrOption->get<std::string>();
      auto typeEnumOption = typeStrToEnum(typeValue);
      if(not typeEnumOption) {
        throw ChimeraTK::logic_error("Unknown value for " + toStr(TYPE) + " " + typeValue + " for register" + regKey);
      }
      TransportLayerType eType = typeEnumOption->get<TransportLayerType>();
      readInfo.transportLayerType = eType;
      writeInfo.transportLayerType = eType;
    }

    // Set delimiters based on top-level information and metadata
    std::string cmdDelim = _serialDelimiter;
    std::string respDelim = _serialDelimiter;
    // DELIMITER
    if(auto delimOption = caseInsensitiveGetValueOption(j, toStr(DELIMITER))) {
      cmdDelim = delimOption->get<std::string>();
      respDelim = cmdDelim;
    }
    // COMMAND_DELIMITER,
    if(auto delimOption = caseInsensitiveGetValueOption(j, toStr(COMMAND_DELIMITER))) {
      cmdDelim = delimOption->get<std::string>();
    }
    // RESPONSE_DELIMITER, override setting of DELIMITER
    if(auto delimOption = caseInsensitiveGetValueOption(j, toStr(RESPONSE_DELIMITER))) {
      respDelim = delimOption->get<std::string>();
    }

    readInfo.cmdLineDelimiter = cmdDelim;
    std::get<ResponceLinesInfo>(readInfo.responseInfo).delimiter = respDelim;

    writeInfo.cmdLineDelimiter = cmdDelim;
    std::get<ResponceLinesInfo>(writeInfo.responseInfo).delimiter = respDelim;
    // NOTE: these delimiter settings may be overrided later by populateFromJson below.

    // FIXED_SIZE_NUM_WIDTH,
    auto sizeOption = caseInsensitiveGetValueOption(j, toStr(FIXED_SIZE_NUM_WIDTH));
    readInfo.fixedSizeNumberWidthOpt = sizeOption;
    writeInfo.fixedSizeNumberWidthOpt = sizeOption;

    // READ,
    // Override settings from the top level based on the "read" key's contents
    auto readOpt = caseInsensitiveGetValueOption(j, toStr(READ));
    if(readOpt) {
      readInfo.populateFromJson(readOpt->get<json>(), "register " + regKey + " read");
    }

    // WRITE
    //  Override settings from the top level based on the "write" key's contents
    auto writeOpt = caseInsensitiveGetValueOption(j, toStr(WRITE));
    if(writeOpt) {
      writeInfo.populateFromJson(writeOpt->get<json>(), "register " + regKey + " write");
    }

    // FIXME: extract the number of lines in write responce from pattern; Ticket 13531

    return CommandBasedBackendRegisterInfo(RegisterPath(regKey), readInfo, writeInfo, nElem);
    // We may later add input for nChannels = j.value("nChan",1);
  } // end constructor

  /********************************************************************************************************************/

  void CommandBasedBackendRegisterInfo::InteractionInfo::populateFromJson(
      const json& j, std::string errorMessageDetail) {
    throwIfHasInvalidJsonKeyCaseInsensitive(
        j, interactionInfoKeyStrs, "Map file registry entry has unknown key for " + errorMessageDetail);

    // COMMAND
    if(auto opt = caseInsensitiveGetValueOption(j, toStr(mapFileInteractionInfoKeys::COMMAND))) {
      commandPattern = opt->get<std::string>();
    }
    else {
      return;
    }

    // RESPESPONSE,
    if(auto opt = caseInsensitiveGetValueOption(j, toStr(mapFileInteractionInfoKeys::RESPESPONSE))) {
      responsePattern = opt->get<std::string>();
    }

    // TYPE
    if(auto opt = caseInsensitiveGetValueOption(j, toStr(mapFileInteractionInfoKeys::TYPE))) {
      std::string typeValue = opt->get<std::string>();
      auto typeEnumOption = typeStrToEnum(typeValue);
      if(not typeEnumOption) {
        throw ChimeraTK::logic_error("Unknown value for " + toStr(TYPE) + " " + typeValue + " for register" + regKey);
      }
      transportLayerType = typeEnumOption->get<TransportLayerType>();
    }

    bool explicitlySetToReadLines = false;
    // DELIMITER
    if(auto opt = caseInsensitiveGetValueOption(j, toStr(mapFileInteractionInfoKeys::DELIMITER))) {
      cmdLineDelimiter = opt->get<std::string>();
      std::get<ResponceLinesInfo>(responseInfo).delimiter = opt->get<std::string>();
      // Don't set explicitlySetToReadLines, so if there's a readBytes, DELIMITER acts like COMMAND_DELIMITER
    }

    // COMMAND_DELIMITER, override all other settings
    if(auto opt = caseInsensitiveGetValueOption(j, toStr(mapFileInteractionInfoKeys::COMMAND_DELIMITER))) {
      cmdLineDelimiter = opt->get<std::string>();
    }

    // RESPONSE_DELIMITER, override all other settings
    auto responceOpt = caseInsensitiveGetValueOption(j, toStr(mapFileInteractionInfoKeys::RESPONSE_DELIMITER));
    if(responceOpt) {
      explicitlySetToReadLines = true;
      std::get<ResponceLinesInfo>(responseInfo).delimiter = opt->get<std::string>();
    }
    // N_RESPONCE_LINES,
    if(auto opt = caseInsensitiveGetValueOption(j, toStr(mapFileInteractionInfoKeys::N_RESPONCE_LINES))) {
      explicitlySetToReadLines = true;
      int n = std::stoi(opt->get<std::string>());
      if(n >= 0) {
        std::get<ResponceLinesInfo>(responseInfo).nLines = n;
      }
      else {
        throw ChimeraTK::logic_error("Invalid negative " + toStr(mapFileInteractionInfoKeys::N_RESPONCE_LINES) + " " +
            n + " for " + errorMessageDetail);
      }
    }

    // N_RESPOCNE_BYTES,
    if(auto opt = caseInsensitiveGetValueOption(j, toStr(mapFileInteractionInfoKeys::N_RESPOCNE_BYTES))) {
      int n = std::stoi(opt->get<std::string>());
      if(n >= 0) {
        responseInfo = ResponceBytesInfo{n};
      }
      else {
        throw ChimeraTK::logic_error("Invalid negative " + toStr(mapFileInteractionInfoKeys::N_RESPOCNE_BYTES) + " " +
            n + " for " + errorMessageDetail);
      }
      if(explicitlySetToReadLines) {
        throw ChimeraTK::logic_error("Invalid mixture of readLines and ReadBytes for " + errorMessageDetail);
      }
    }

    // FIXED_SIZE_NUM_WIDTH,
    if(auto opt = caseInsensitiveGetValueOption(j, toStr(mapFileInteractionInfoKeys::FIXED_SIZE_NUM_WIDTH))) {
      int n = std::stoi(opt->get<std::string>());
      if(n > 0) {
        fixedSizeNumberWidthOpt = n;
      }
      else {
        throw ChimeraTK::logic_error("Invalid non-positive " + toStr(mapFileInteractionInfoKeys::FIXED_SIZE_NUM_WIDTH) +
            " " + n + " for " + errorMessageDetail);
      }
    }
  } // populateFromJson

  /**********************************************************************************************************************/
  /**********************************************************************************************************************/

  static void throwIfInvalidCommandAndResponce(
      const InteractionInfo& writeInfo, const InteractionInfo& readInfo, const std::string& errorMessageDetail) {
    // Throw if there isn't a read or write command, or invalid combinations

    bool readable = readInfo.isActive();
    bool writeable = writeInfo.isActive();
    bool hasReadResponce = not readInfo.responsePattern.empty();
    bool hasWriteResponce = not writeInfo.responsePattern.empty();

    if(not(readable or writeable)) {
      throw ChimeraTK::logic_error("A non-empty read:" + toStr(mapFileInteractionInfoKeys::COMMAND) + " or write" +
          toStr(mapFileInteractionInfoKeys::COMMAND) + " tags is required, and neither are present for " +
          errorMessageDetail);
    }

    // Throw if there are responces without corresponding commands.
    if(hasReadResponce and (not readable)) {
      throw ChimeraTK::logic_error("A non-empty read " + toStr(mapFileInteractionInfoKeys::RESPESPONSE) +
          " without a non-empty read " + toStr(mapFileInteractionInfoKeys::COMMAND) + " for " + errorMessageDetail);
    }
    if(hasWriteResponce and (not writeable)) {
      throw ChimeraTK::logic_error("A non-empty write " + toStr(mapFileInteractionInfoKeys::RESPESPONSE) +
          " without a non-empty write " + toStr(mapFileInteractionInfoKeys::COMMAND) + " for " + errorMessageDetail);
    }
  }

  /**********************************************************************************************************************/

  static void throwIfInvalidVoidType(
      const InteractionInfo& writeInfo, const InteractionInfo& readInfo, const std::string errorMessageDetail) {
    if(writeInfo.getTransportLayerType() == TransportLayerType::VOID) {
      if(readInfo.isActive() or not writeInfo.isActive()) {
        throw ChimeraTK::logic_error(
            "Void type must be write-only but has a " + toStr(READ) + " key for " + errorMessageDetail);
      }

      if(nElem != 1) {
        throw ChimeraTK::logic_error("Void type must only have 1 element but has " + toStr(N_ELEM) + " = " + nElem +
            " for " + errorMessageDetail);
      }

      if(writeInfo.commandPattern.find("{{x") != std::string::npos) {
        throw ChimeraTK::logic_error("Illegal inja template in write " + toStr(mapFileInteractionInfoKeys::COMMAND) +
            " = \"" + writeInfo.commandPattern + "\" for void-type for " + errorMessageDetail);
      }
    } // end if void type

    /******************************************************************************************************************/

    static DataType getDataTypeFromTransportLayerType(TransportLayerType type) {
      if(type == TransportLayerType::DEC_INT) {
        return DataType::int64;
      }
      else if(type == TransportLayerType::HEX_INT) {
        return DataType::uint64;
      }
      else if(type == TransportLayerType::BIN_INT) {
        return DataType::uint64;
      }
      else if(type == TransportLayerType::DEC_FLOAT) {
        return DataType::float64;
      }
      else if(type == TransportLayerType::STRING) {
        return DataType::string;
      }
      else if(type == TransportLayerType::VOID) {
        return DataType::Void;
      }
      // Either some new type was added but this list was not updated.
      // or we've been given a type from an uninitalized InteractionInfo
      throw ChimeraTK::logic_error("Type not recognized by getDataTypeFromTransportLayerType");
      return DataType::string;
    }

    /******************************************************************************************************************/

    static void ensureTransportLayerTypeAreSet(
        InteractionInfo & writeInfo, InteractionInfo & readInfo, const std::string errorMessageDetail) {
      // Throw if type/transportLayerType is not set.
      if(not(writeInfo.transportLayerType.has_value() or readInfo.transportLayerType.has_value())) {
        throw ChimeraTK::logic_error("type is required but is missing for " + errorMessageDetail);
      }
      if(readInfo.isActive() and not readInfo.transportLayerType.has_value()) {
        throw ChimeraTK::logic_error("type is required but is missing on read for " + errorMessageDetail);
      }
      if(writeInfo.isActive() and not writeInfo.transportLayerType.has_value()) {
        throw ChimeraTK::logic_error("type is required but is missing on write for " + errorMessageDetail);
      }

      // If one is set but not the other, set the other. This is expected to happen when the one with no value is not
      // active such as in read-only and write-only situations.
      if(readInfo.transportLayerType.has_value() and not writeInfo.transportLayerType.has_value()) {
        writeInfo.transportLayerType = readInfo.transportLayerType;
      }
      else if(writeInfo.transportLayerType.has_value() and not readInfo.transportLayerType.has_value()) {
        readInfo.transportLayerType = writeInfo.transportLayerType;
      }
    }

    /******************************************************************************************************************/

    static DataType getDataType(
        const InteractionInfo& writeInfo, const InteractionInfo& readInfo, const std::string& errorMessageDetail) {
      assert(writeInfo.isActive() or readInfo.isActive());

      if(writeInfo.isActive()) {
        auto writeDataType = getDataTypeFromTransportLayerType(writeInfo.getTransportLayerType());

        // If readable and writeable, but incompatible data types, throw.
        if(readInfo.isActive() and
            (writeDataType != getDataTypeFromTransportLayerType(readInfo.getTransportLayerType()))) {
          throw ChimeraTK::logic_error("Read and Write have incompatible DataTypes for " + errorMessageDetail);
        }
        return writeDataType;
      }
      else { // read-only
        return getDataTypeFromTransportLayerType(readInfo.getTransportLayerType());
      }
    }

  } // namespace ChimeraTK

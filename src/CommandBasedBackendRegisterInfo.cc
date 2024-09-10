// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "CommandBasedBackendRegisterInfo.h"

#include "CommandBasedBackend.h"
#include "stringUtils.h"
#include <nlohmann/json.hpp>

#include <array>
#include <fstream>
#include <iostream>
#include <string>

using json = nlohmann::json;

namespace ChimeraTK {

  // These are found and used by json::get
  void from_json(const json& j, CommandBasedBackend::MapFile& d);
  void from_json(const json& j, CommandBasedBackend::MetaData& m);
  CommandBasedBackendRegisterInfo getJsonToCommandBasedBackendRegisterInfo(const json& j, const std::string& regKey);

  /********************************************************************************************************************/
  namespace CommandBasedBackendJsonParserInfo {

    const int requiredMapFileFormatVersion = 1;

    /*******************************************************************************************************************/

    enum mapFileTopLevelKeys {
      METADATA = 0,
      REGISTERS,
      N_TOP_LEVEL_KEYS // Keep this at the end so as to automatically be the count of keys
    };
    // static constexpr size_t N_TOP_LEVEL_KEYS = mapFileTopLevelKeys::NUMBER_OF_KEYS;
    static const std::array<std::string, N_TOP_LEVEL_KEYS> topLevelKeyStrs = {
        // indexed by mapFileTopLevelKeys so keep them in the same order
        "metadata", "registers"};
    inline std::string to_str(mapFileTopLevelKeys keyEnum) {
      return topLevelKeyStrs[keyEnum];
    }
    /******************************************************************************************************************/

    enum mapFileMetadataKeys {
      MAP_FILE_FORMAT_VERSION = 0,
      COMMAND_BASED_BACKEND_TYPE,
      DEFAULT_RECOVERY_REGISTER,
      DELIM,
      // Add other keys here
      N_METADATA_KEYS // Keep this at the end so as to automatically be the count of keys
    };
    // static constexpr size_t N_METADATA_KEYS = mapFileMetadataKeys::NUMBER_OF_KEYS;
    static const std::array<std::string, N_METADATA_KEYS> metadataKeyStrs = {
        // indexed by mapFileMetadataKeys so keep them in the same order
        "mapFileFormatVersion", "commandBasedBackendType", "defaultRecoveryRegister", "delim"};
    inline std::string to_str(mapFileMetadataKeys keyEnum) {
      return metadataKeyStrs[keyEnum];
    }
    /******************************************************************************************************************/

    enum mapFileRegisterKeys {
      WRITE_CMD = 0,
      WRITE_RESP,
      READ_CMD,
      READ_RESP,
      N_ELEM,
      N_LN_READ,
      TYPE,
      // Add other keys here
      N_REGISTER_KEYS // Keep this at the end so as to automatically be the count of keys
    };
    static const std::array<std::string, N_REGISTER_KEYS> registerKeyStrs = {
        // indexed by mapFileRegisterKeys so keep them in the same order
        "writeCmd", "writeResp", "readCmd", "readResp", "nElem", "nLnRead", "type"};
    inline std::string to_str(mapFileRegisterKeys keyEnum) {
      return registerKeyStrs[keyEnum];
    }

    /******************************************************************************************************************/
    static constexpr size_t N_TYPES = 5;
    static const std::array<std::string, N_TYPES> registerTypeStrs = {
        // indexed by CommandBasedBackendRegisterInfo::InternalType so keep them in the same order
        //{ INT64=0 , UINT64, DOUBLE, STRING, VOID }
        // must be lower case.
        "int64",
        "uint64",
        "double",
        "string",
        "void",
    };
    inline std::string to_str(CommandBasedBackendRegisterInfo::InternalType eType) {
      return registerTypeStrs[static_cast<int>(eType)];
    }

  } // end namespace CommandBasedBackendJsonParserInfo
  using namespace CommandBasedBackendJsonParserInfo;

  /******************************************************************************************************************/
  /******************************************************************************************************************/
  CommandBasedBackendRegisterInfo::CommandBasedBackendRegisterInfo(RegisterPath registerPath_,
      std::string writeCommandPattern_, std::string writeResponsePattern_, std::string readCommandPattern_,
      std::string readResponsePattern_, uint nElements_, size_t nLinesReadResponse_,
      CommandBasedBackendRegisterInfo::InternalType type, std::string delimiter_)
  : nElements(nElements_), registerPath(registerPath_), writeCommandPattern(writeCommandPattern_),
    writeResponsePattern(writeResponsePattern_), readCommandPattern(readCommandPattern_),
    readResponsePattern(readResponsePattern_), nLinesReadResponse(nLinesReadResponse_), internalType(type),
    delimiter(delimiter_) {
    // set dataDescriptor from type.
    switch(type) {
      case InternalType::UINT64:
        dataDescriptor = DataDescriptor(
            DataDescriptor::FundamentalType::numeric, /*isIntegral*/ true, /*isSigned*/ false, /*nDigits*/ 11);
        break;
      case InternalType::STRING:
        dataDescriptor = DataDescriptor(DataDescriptor::FundamentalType::string);
        break;
      case InternalType::DOUBLE:
        // smallest possible 5e-324, largest 2e308
        dataDescriptor = DataDescriptor(DataDescriptor::FundamentalType::numeric, /*isIntegral*/ false,
            /*isSigned*/ true, /*nDigits*/ 3 + 325, /*nFragtionalDigits*/ 325);
        break;
      case InternalType::VOID:
        dataDescriptor = DataDescriptor(DataDescriptor::FundamentalType::nodata);
        break;
      default:
      case InternalType::INT64:
        dataDescriptor = DataDescriptor(
            DataDescriptor::FundamentalType::numeric, /*isIntegral*/ true, /*isSigned*/ true, /*nDigits*/ 11);
    }
  } // end constructor

  /******************************************************************************************************************/

  CommandBasedBackend::MapFile CommandBasedBackend::parse(const std::string& mapFileName) {
    std::ifstream file(mapFileName);
    if(!file.is_open()) {
      throw ChimeraTK::logic_error("Could not open the map file " + mapFileName);
    }

    json j;
    file >> j;
    return j.get<CommandBasedBackend::MapFile>();
    // get() calls from_json(const json& j, CommandBasedBackend::MapFile& d)
  }

  /********************************************************************************************************************/

  /**
   * Throws a ChimeraTK::logic_error if the json key is not in an std::array of valid keys
   */
  template<size_t N>
  void throwIfHasInvalidJsonKey(const json& j, const std::array<std::string, N>& validKeys, std::string errorMessage) {
    for(auto it = j.begin(); it != j.end(); ++it) {
      bool stringIsNotInArray = true;
      for(const std::string& i : validKeys) {
        if(it.key() == i) {
          stringIsNotInArray = false;
          break;
        }
      }

      if(stringIsNotInArray) {
        throw ChimeraTK::logic_error(errorMessage + " " + it.key());
      }
    }
  }

  /********************************************************************************************************************/

  void from_json(const json& j, CommandBasedBackend::MapFile& d) {
    if(!j.contains(to_str(METADATA)) || !j.contains(to_str(REGISTERS))) {
      throw ChimeraTK::logic_error("Missing keys in JSON data");
    }

    throwIfHasInvalidJsonKey(j, topLevelKeyStrs, "Map file top level has unknown key");

    d.metaData = j.at(to_str(METADATA)).template get<CommandBasedBackend::MetaData>();
    // get() calls from_json(const json& j, CommandBasedBackend::MetaData& m)

    for(auto& [key, value] : j.at(to_str(REGISTERS)).items()) {
      throwIfHasInvalidJsonKey(value, registerKeyStrs, "Map file registry entry " + key + " has unknown key");

      CommandBasedBackendRegisterInfo reg = getJsonToCommandBasedBackendRegisterInfo(value, key);
      reg.registerPath = RegisterPath(key);
      d.registers.push_back(reg);
    }
  }

  /********************************************************************************************************************/

  void from_json(const json& j, CommandBasedBackend::MetaData& m) {
    if(!j.contains(to_str(MAP_FILE_FORMAT_VERSION))) {
      throw ChimeraTK::logic_error("Missing mapFileFormatVersion key in metadata");
    }

    throwIfHasInvalidJsonKey(j, metadataKeyStrs, "Map file metadata has unknown key");

    if(int mapFileFormatVersion = j.value(to_str(MAP_FILE_FORMAT_VERSION), 0);
        mapFileFormatVersion != requiredMapFileFormatVersion) {
      throw ChimeraTK::logic_error("Incorrect map file format version " + std::to_string(mapFileFormatVersion) +
          ", version " + std::to_string(requiredMapFileFormatVersion) + " required.");
    }

    m.defaultRecoveryRegister = RegisterPath(j.value(to_str(DEFAULT_RECOVERY_REGISTER), ""));

    j.at(to_str(DELIM)).get_to(m.serialDelimiter);
  }

  /******************************************************************************************************************/

  CommandBasedBackendRegisterInfo getJsonToCommandBasedBackendRegisterInfo(const json& j, const std::string& regKey) {
    // regKey is the key (register path) whose value is j. This is needed information for debugging

    if(not(j.contains(to_str(READ_CMD)) or j.contains(to_str(WRITE_CMD)))) {
      throw ChimeraTK::logic_error("A non-empty " + to_str(READ_CMD) + " or " + to_str(WRITE_CMD) +
          " tags is required, and neither are present for register " + regKey);
    }
    else if((not j.contains(to_str(READ_CMD))) and j.at(to_str(WRITE_CMD)) == "") {
      throw ChimeraTK::logic_error("A non-empty " + to_str(READ_CMD) + " or " + to_str(WRITE_CMD) +
          " tags is required. writeCmd is blankd and readCmd is missing for register " + regKey);
    }
    else if((not j.contains(to_str(WRITE_CMD))) and j.at(to_str(READ_CMD)) == "") {
      throw ChimeraTK::logic_error("A non-empty " + to_str(READ_CMD) + " or " + to_str(WRITE_CMD) +
          " tags is required. readCmd is blankd and writeCmd is missing for register " + regKey);
    }

    // Throw if there are responces without corresponding commands.
    /*/
    if(j.contains(to_str(READ_RESP)) and j.at(to_str(READ_RESP)) != "" and ( (not j.contains(to_str(READ_CMD))) or
    j.at(to_str(READ_CMD) == ""))) { throw ChimeraTK::logic_error("A non-empty "+to_str(READ_RESP)+" without a non-empty
    "+to_str(READ_CMD)+" for register "+regKey);
    }
    if(j.contains(to_str(WRITE_RESP)) and j.at(to_str(WRITE_RESP)) != "" and ( (not j.contains(to_str(WRITE_CMD))) or
    j.at(to_str(WRITE_CMD) == ""))) { throw ChimeraTK::logic_error("A non-empty "+to_str(WRITE_RESP)+" without a
    non-empty "+to_str(WRITE_CMD)+" for register "+regKey);
    }*/

    CommandBasedBackendRegisterInfo::InternalType eType; //{ INT64, UINT64, DOUBLE, STRING, VOID };

    std::string typeStr = j.value(to_str(TYPE), to_str(CommandBasedBackendRegisterInfo::InternalType::STRING));
    toLowerCase(typeStr);

    /*
    if(typeStr == to_str(CommandBasedBackendRegisterInfo::InternalType::VOID) and (
        (j.contains(to_str(READ_CMD)) and j.at(to_str(READ_CMD)) != "" ) or //non-empty read command
        (j.contains(to_str(WRITE_RESP)) and j.at(to_str(WRITE_RESP)) != "") //non-empty write responce
      )) {
        throw ChimeraTK::logic_error("Void type has a "+to_str(READ_CMD)+" or a "+to_str(WRITE_RESP)+" for register "+regKey);
    }*/

    for(size_t iType = 0; iType < N_TYPES;) {
      if(typeStr == registerTypeStrs[iType]) {
        eType = static_cast<CommandBasedBackendRegisterInfo::InternalType>(iType);
        break;
      }
      if(++iType == N_TYPES) {
        throw ChimeraTK::logic_error("Unknown register " + to_str(TYPE) + " " + typeStr + " for regisgter " + regKey);
      }
    }

    // save for later implementations
    // nChannels = j.value("nChan",1);
    // nLinesWriteResponce = j.value("nLnWrite",1);

    /*
       Now feed the CommandBasedBackendRegisterInfo constructor
       CommandBasedBackendRegisterInfo(
       RegisterPath registerPath_ = {},
       std::string writeCommandPattern_ = "",
       std::string writeResponsePattern_ = "",
       std::string readCommandPattern_ = "",
       std::string readResponsePattern_ = "",
       uint nElements_ = 1,
       size_t nLinesReadResponse_ = 1,
       InternalType type = InternalType::INT64
       );
       */
    return {{}, // regpath = key, but the key not accessible here.
                // Rely on from_json(j,MetaData) to set this
        j.value(to_str(WRITE_CMD), ""), j.value(to_str(WRITE_RESP), ""), j.value(to_str(READ_CMD), ""),
        j.value(to_str(READ_RESP), ""), (uint)j.value(to_str(N_ELEM), 1), (size_t)j.value(to_str(N_LN_READ), 1), eType};
  }

} // namespace ChimeraTK

// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "CommandBasedBackend.h"

#include "SerialCommandHandler.h"
#include "stringUtils.h"
#include "TcpCommandHandler.h"
#include <nlohmann/json.hpp>

#include <fstream>
#include <string>

using json = nlohmann::json;

namespace ChimeraTK {

  CommandBasedBackendRegisterInfo registerInfoFromJson(const json& j, const std::string& regKey);

  /********************************************************************************************************************/

  const int requiredMapFileFormatVersion = 1;

  /********************************************************************************************************************/

  enum mapFileTopLevelKeys {
    MAP_FILE_FORMAT_VERSION = 0,
    METADATA,
    REGISTERS,
    N_TOP_LEVEL_KEYS // Keep this at the end so as to automatically be the count of keys
  };

  static const std::array<std::string, N_TOP_LEVEL_KEYS> topLevelKeyStrs = {
      // indexed by mapFileTopLevelKeys so keep them in the same order
      "mapFileFormatVersion", "metadata", "registers"};
  inline std::string to_str(mapFileTopLevelKeys keyEnum) {
    return topLevelKeyStrs[keyEnum];
  }
  /********************************************************************************************************************/

  enum mapFileMetadataKeys {
    DEFAULT_RECOVERY_REGISTER = 0,
    DELIMITER,
    // Add other keys here
    N_METADATA_KEYS // Keep this at the end so as to automatically be the count of keys
  };

  static const std::array<std::string, N_METADATA_KEYS> metadataKeyStrs = {
      // indexed by mapFileMetadataKeys so keep them in the same order
      "defaultRecoveryRegister", "delimiter"};
  inline std::string to_str(mapFileMetadataKeys keyEnum) {
    return metadataKeyStrs[keyEnum];
  }
  /********************************************************************************************************************/

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

  /********************************************************************************************************************/
  static constexpr size_t N_TYPES = 6;
  static const std::array<std::string, N_TYPES> registerTypeStrs = {
      // indexed by CommandBasedBackendRegisterInfo::InternalType so keep them in the same order:
      //{ INT64=0 , UINT64, HEX, DOUBLE, STRING, VOID };
      // must be lower case.
      "int64",
      "uint64",
      "hex",
      "double",
      "string",
      "void",
  };
  inline std::string to_str(CommandBasedBackendRegisterInfo::InternalType eType) {
    return registerTypeStrs[static_cast<int>(eType)];
  }

  /********************************************************************************************************************/

  CommandBasedBackend::CommandBasedBackend(
      CommandBasedBackendType type, std::string instance, std::map<std::string, std::string> parameters)
  : _commandBasedBackendType(type), _instance(std::move(instance)) {
    FILL_VIRTUAL_FUNCTION_TEMPLATE_VTABLE(getRegisterAccessor_impl);

    if(_commandBasedBackendType == CommandBasedBackendType::ETHERNET) {
      if(parameters.count("port") == 0) {
        throw ChimeraTK::logic_error("Missing parameter \"port\" in CDD of backend CommandBasedTCP " + instance);
      }
      _port = parameters.at("port");
    }
    if(parameters.count("map") == 0) {
      throw ChimeraTK::logic_error("No map file parameter");
    }

    // parse map file and copy results to internal catalogues.
    parseJsonAndPopulateCatalogue(parameters["map"]);

    _lastWrittenRegister = _defaultRecoveryRegister;
  }

  /********************************************************************************************************************/

  void CommandBasedBackend::open() {
    switch(_commandBasedBackendType) {
      case CommandBasedBackendType::SERIAL:
        _commandHandler = std::make_unique<SerialCommandHandler>(_instance, _serialDelimiter, _timeoutInMilliseconds);
        break;
      case CommandBasedBackendType::ETHERNET:
        _commandHandler = std::make_unique<TcpCommandHandler>(_instance, _port, _timeoutInMilliseconds);
        break;
      default:
        // This is not part of the proper interface. Throw a std::logic_error as
        // intermediate debugging solution
        throw std::logic_error("CommandBasedBackend: FIXME: Unsupported type");
    }

    // Try to read from the last register that has been used.
    // Do not try writing as we don't have a valid value and would alter the device.
    auto registerInfo = _backendCatalogue.getBackendRegister(_lastWrittenRegister);
    // Accessor with the isRecoveryTestAccessor flag set to true
    CommandBasedBackendRegisterAccessor<std::string> testAccessor(
        DeviceBackend::shared_from_this(), registerInfo, _lastWrittenRegister, 0, 0, {}, true);
    testAccessor.read();

    // Backends have to call this function at the end of a successful open() call
    setOpenedAndClearException();
  } // end open

  /********************************************************************************************************************/

  void CommandBasedBackend::close() {
    _commandHandler.reset();
    _opened = false;
  }

  /********************************************************************************************************************/

  RegisterCatalogue CommandBasedBackend::getRegisterCatalogue() const {
    return RegisterCatalogue(_backendCatalogue.clone());
  }

  /********************************************************************************************************************/

  boost::shared_ptr<DeviceBackend> CommandBasedBackend::createInstanceSerial(
      std::string instance, std::map<std::string, std::string> parameters) {
    return boost::make_shared<CommandBasedBackend>(
        CommandBasedBackend::CommandBasedBackendType::SERIAL, instance, parameters);
  }

  /********************************************************************************************************************/

  boost::shared_ptr<DeviceBackend> CommandBasedBackend::createInstanceEthernet(
      std::string instance, std::map<std::string, std::string> parameters) {
    return boost::make_shared<CommandBasedBackend>(
        CommandBasedBackend::CommandBasedBackendType::ETHERNET, instance, parameters);
  }

  /********************************************************************************************************************/

  std::string CommandBasedBackend::sendCommand(std::string cmd) {
    assert(_opened);
    std::lock_guard<std::mutex> lock(_mux);

    return _commandHandler->sendCommand(std::move(cmd));
  }

  /********************************************************************************************************************/

  std::vector<std::string> CommandBasedBackend::sendCommand(std::string cmd, size_t nLinesExpected) {
    assert(_commandHandler);
    std::lock_guard<std::mutex> lock(_mux);
    return _commandHandler->sendCommand(std::move(cmd), nLinesExpected);
  }

  /********************************************************************************************************************/

  std::string CommandBasedBackend::readDeviceInfo() {
    return "Device: " + _instance + " timeout: " + std::to_string(_timeoutInMilliseconds);
  }

  /********************************************************************************************************************/

  CommandBasedBackend::BackendRegisterer::BackendRegisterer() {
    BackendFactory::getInstance().registerBackendType(
        "CommandBasedTTY", &CommandBasedBackend::createInstanceSerial, {}, CHIMERATK_DEVICEACCESS_VERSION);
    std::cout << "registered backend CommandBasedTTY" << std::endl;
    BackendFactory::getInstance().registerBackendType(
        "CommandBasedTCP", &CommandBasedBackend::createInstanceEthernet, {}, CHIMERATK_DEVICEACCESS_VERSION);
    std::cout << "registered backend CommandBasedTCP" << std::endl;
  }

  /********************************************************************************************************************/

  static CommandBasedBackend::BackendRegisterer gCommandBasedBackendRegisterer;

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

  void CommandBasedBackend::parseJsonAndPopulateCatalogue(const std::string& mapFileName) {
    std::ifstream file(mapFileName);
    if(!file.is_open()) {
      throw ChimeraTK::logic_error("Could not open the map file " + mapFileName);
    }

    json j;
    file >> j;

    throwIfHasInvalidJsonKey(j, topLevelKeyStrs, "Map file top level has unknown key");

    if(j.contains(to_str(MAP_FILE_FORMAT_VERSION))) {
      int mapFileFormatVersion = j.value(to_str(MAP_FILE_FORMAT_VERSION), 0);
      if(mapFileFormatVersion != requiredMapFileFormatVersion) {
        throw ChimeraTK::logic_error("Incorrect map file format version " + std::to_string(mapFileFormatVersion) +
            ", version " + std::to_string(requiredMapFileFormatVersion) + " required.");
      }
    }
    else {
      throw ChimeraTK::logic_error("Missing mapFileFormatVersion key in metadata");
    }

    if(j.contains(to_str(METADATA))) {
      const json& metaDataJson = j.at(to_str(METADATA));
      _defaultRecoveryRegister = RegisterPath(metaDataJson.value(to_str(DEFAULT_RECOVERY_REGISTER), ""));
      _serialDelimiter = j.value(to_str(DELIMITER), "\r\n");
    }
    else {
      throw ChimeraTK::logic_error("Missing keys " + to_str(METADATA) + " in JSON data");
    }

    if(j.contains(to_str(REGISTERS))) {
      for(auto& [key, value] : j.at(to_str(REGISTERS)).items()) {
        _backendCatalogue.addRegister(registerInfoFromJson(value, key));
      }
    }
    else {
      throw ChimeraTK::logic_error("Missing keys " + to_str(REGISTERS) + " in JSON data");
    }
  }

  /********************************************************************************************************************/

  CommandBasedBackendRegisterInfo registerInfoFromJson(const json& j, const std::string& regKey) {
    // regKey is the key (register path) whose value is j. This is needed information for debugging

    throwIfHasInvalidJsonKey(j, registerKeyStrs, "Map file registry entry " + regKey + " has unknown key");

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
    if((j.value(to_str(READ_RESP), "") != "") and (j.value(to_str(READ_CMD), "") == "")) {
      throw ChimeraTK::logic_error(
          "A non-empty " + to_str(READ_RESP) + " without a non-empty " + to_str(READ_CMD) + " for register " + regKey);
    }
    if((j.value(to_str(WRITE_RESP), "") != "") and (j.value(to_str(WRITE_CMD), "") == "")) {
      throw ChimeraTK::logic_error("A non-empty " + to_str(WRITE_RESP) + " without a non-empty " + to_str(WRITE_CMD) +
          " for register " + regKey);
    }

    CommandBasedBackendRegisterInfo::InternalType eType{}; //{ INT64=0 , UINT64, HEX, DOUBLE, STRING, VOID }

    std::string typeStr = j.value(to_str(TYPE), "invalid");
    if(typeStr == "invalid") {
      throw ChimeraTK::logic_error("type is required but is missing for register " + regKey);
    }
    toLowerCase(typeStr);

    // throw if void type and sending reading or writing.
    if(typeStr == to_str(CommandBasedBackendRegisterInfo::InternalType::VOID) and
        ((j.value(to_str(READ_CMD), "") != "") or (j.value(to_str(WRITE_RESP), "") != ""))) {
      throw ChimeraTK::logic_error(
          "Void type has a " + to_str(READ_CMD) + " or a " + to_str(WRITE_RESP) + " for register " + regKey);
    }

    for(size_t iType = 0; iType < N_TYPES;) {
      if(typeStr == registerTypeStrs[iType]) {
        eType = static_cast<CommandBasedBackendRegisterInfo::InternalType>(iType);
        break;
      }
      if(++iType == N_TYPES) {
        throw ChimeraTK::logic_error("Unknown register " + to_str(TYPE) + " " + typeStr + " for regisgter " + regKey);
      }
    }

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
         InternalType type
       );
       */
    return {RegisterPath(regKey), j.value(to_str(WRITE_CMD), ""), j.value(to_str(WRITE_RESP), ""),
        j.value(to_str(READ_CMD), ""), j.value(to_str(READ_RESP), ""), (uint)j.value(to_str(N_ELEM), 1),
        (size_t)j.value(to_str(N_LN_READ), 1), eType};
    // We may later add input for nChannels = j.value("nChan",1);
  }

} // end namespace ChimeraTK

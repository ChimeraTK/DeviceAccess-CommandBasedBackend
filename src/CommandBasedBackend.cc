// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "CommandBasedBackend.h"

#include "mapFileKeys.h"
#include "SerialCommandHandler.h"
#include "stringUtils.h"
#include "TcpCommandHandler.h"
#include <nlohmann/json.hpp>

#include <fstream>
#include <string>

using json = nlohmann::json;

namespace ChimeraTK {

  /********************************************************************************************************************/

  CommandBasedBackend::CommandBasedBackend(
      CommandBasedBackendType type, std::string instance, std::map<std::string, std::string> parameters)
  : _commandBasedBackendType(type), _instance(std::move(instance)) {
    // NOLINTNEXTLINE(readability-identifier-naming)
    FILL_VIRTUAL_FUNCTION_TEMPLATE_VTABLE(getRegisterAccessor_impl);

    if(_commandBasedBackendType == CommandBasedBackendType::ETHERNET) {
      if(parameters.count("port") == 0) {
        throw ChimeraTK::logic_error("Missing parameter \"port\" in CDD of backend CommandBasedTCP " + _instance);
      }
      _port = parameters.at("port");
    }
    if(parameters.count("map") == 0) {
      throw ChimeraTK::logic_error("No map file parameter");
    }

    // Parse map file and copy results to internal catalogues.
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
        _commandHandler =
            std::make_unique<TcpCommandHandler>(_instance, _port, _serialDelimiter, _timeoutInMilliseconds);
        break;
      default:
        // Then this is not part of the proper interface. Throw a std::logic_error as
        // intermediate debugging solution.
        throw std::logic_error("CommandBasedBackend: FIXME: Unsupported type");
    }

    // Try to read from the last register that has been used.
    // Do not try writing as we don't have a valid value and would alter the device.
    auto registerInfo = _backendCatalogue.getBackendRegister(_lastWrittenRegister);

    // testAccessor has isRecoveryTestAccessor flag set to true.
    CommandBasedBackendRegisterAccessor<std::string> testAccessor(
        DeviceBackend::shared_from_this(), registerInfo, _lastWrittenRegister, 0, 0, {}, true);
    testAccessor.read();

    // Backends must call this function at the end of a successful open() call.
    setOpenedAndClearException();
  }

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

  std::vector<std::string> CommandBasedBackend::sendCommandAndReadLines(std::string cmd, size_t nLinesToRead,
      const WritableDelimiter& writeDelimiter, const ReadableDelimiter& readDelimiter) {
    assert(_commandHandler);
    std::lock_guard<std::mutex> lock(_mux);
    return _commandHandler->sendCommandAndReadLines(std::move(cmd), nLinesToRead, writeDelimiter, readDelimiter);
  }

  /********************************************************************************************************************/

  std::string CommandBasedBackend::sendCommandAndReadBytes(
      std::string cmd, size_t nBytesToRead, const WritableDelimiter& writeDelimiter) {
    assert(_commandHandler);
    std::lock_guard<std::mutex> lock(_mux);
    return _commandHandler->sendCommandAndReadBytes(std::move(cmd), nBytesToRead, writeDelimiter);
  }

  /********************************************************************************************************************/

  std::string CommandBasedBackend::readDeviceInfo() {
    return "Device: " + _instance + " timeout: " + std::to_string(_timeoutInMilliseconds);
  }

  /********************************************************************************************************************/

  CommandBasedBackend::BackendRegisterer::BackendRegisterer() {
    BackendFactory::getInstance().registerBackendType(
        "CommandBasedTTY", &CommandBasedBackend::createInstanceSerial, {}, CHIMERATK_DEVICEACCESS_VERSION);
    BackendFactory::getInstance().registerBackendType(
        "CommandBasedTCP", &CommandBasedBackend::createInstanceEthernet, {}, CHIMERATK_DEVICEACCESS_VERSION);
  }

  /********************************************************************************************************************/

  static CommandBasedBackend::BackendRegisterer gCommandBasedBackendRegisterer;

  /********************************************************************************************************************/

  void CommandBasedBackend::parseJsonAndPopulateCatalogue(const std::string& mapFileName) {
    std::ifstream file(mapFileName);
    if(!file.is_open()) {
      throw ChimeraTK::logic_error("Could not open the map file " + mapFileName);
    }

    json j;
    file >> j;

    throwIfHasInvalidJsonKey(j, topLevelKeyStrs, "Map file top level has unknown key");

    if(auto mapFileFormatVersionOpt =
            caseInsensitiveGetValueOption(j, toStr(mapFileTopLevelKeys::MAP_FILE_FORMAT_VERSION))) {
      int mapFileFormatVersion = mapFileFormatVersionOpt->get<int>();
      if(mapFileFormatVersion != requiredMapFileFormatVersion) {
        throw ChimeraTK::logic_error("Incorrect map file format version " + std::to_string(mapFileFormatVersion) +
            ", version " + std::to_string(requiredMapFileFormatVersion) + " required.");
      }
    }
    else {
      throw ChimeraTK::logic_error("Missing mapFileFormatVersion key in metadata");
    }

    if(auto metaDataJsonOpt = caseInsensitiveGetValueOption(j, toStr(mapFileTopLevelKeys::METADATA))) {
      const json& metaDataJson = metaDataJsonOpt->get<json>();
      _defaultRecoveryRegister = RegisterPath(
          caseInsensitiveGetValueOr(metaDataJson, toStr(mapFileMetadataKeys::DEFAULT_RECOVERY_REGISTER), ""));
      _serialDelimiter = caseInsensitiveGetValueOr(metaDataJson, toStr(mapFileMetadataKeys::DELIMITER), "\r\n");
    }
    else {
      throw ChimeraTK::logic_error("Missing keys " + toStr(mapFileTopLevelKeys::METADATA) + " in JSON data");
    }

    if(auto registerOpt = caseInsensitiveGetValueOption(j, toStr(mapFileTopLevelKeys::REGISTERS))) {
      for(const auto& [key, value] : registerOpt->get<json>().items()) {
        _backendCatalogue.addRegister(CommandBasedBackendRegisterInfo(value, key, _serialDelimiter));
      }
    }
    else {
      throw ChimeraTK::logic_error("Missing keys " + toStr(mapFileTopLevelKeys::REGISTERS) + " in JSON data");
    }
  }

  /********************************************************************************************************************/

} // end namespace ChimeraTK

// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "CommandBasedBackend.h"

#include "SerialCommandHandler.h"
#include "TcpCommandHandler.h"

#include <string>

namespace ChimeraTK {

  /********************************************************************************************************************/

  CommandBasedBackend::CommandBasedBackend(
      CommandBasedBackendType type, std::string instance, std::map<std::string, std::string> parameters)
  : _commandBasedBackendType(type), _instance(std::move(instance)) {
    FILL_VIRTUAL_FUNCTION_TEMPLATE_VTABLE(getRegisterAccessor_impl);

    if(_commandBasedBackendType == CommandBasedBackendType::ETHERNET) {
      if(parameters.count("port") > 0) {
        _port = parameters.at("port");
      }
      else {
        throw ChimeraTK::logic_error("Missing parameter \"port\" in CDD of backend CommandBasedTCP " + instance);
      }
    }
    if(parameters.count("map") > 0) {
      // parse map file and copy results to internal catalogues.
      MapFile mapFile = parse(parameters["map"]);
      _metaData = MetaData(mapFile.metaData);
      _lastWrittenRegister = _metaData.defaultRecoveryRegister;
      for(std::size_t i = 0; i < mapFile.registers.size(); ++i) {
        // The delimiter is set in the metadata, but it's needed in the Reg.Accessor
        // so we're copying the delimiter to the Reg.Info for each register
        mapFile.registers[i].delimiter = _metaData.serialDelimiter;

        _backendCatalogue.addRegister(mapFile.registers[i]);
      }
    }
    else {
      throw ChimeraTK::logic_error("No map file parameter");
    }
  }

  /********************************************************************************************************************/

  void CommandBasedBackend::open() {
    switch(_commandBasedBackendType) {
      case CommandBasedBackendType::SERIAL:
        _commandHandler =
            std::make_unique<SerialCommandHandler>(_instance, _metaData.serialDelimiter, _timeoutInMilliseconds);
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

  std::vector<std::string> CommandBasedBackend::sendCommand(std::string cmd, const size_t nLinesExpected) {
    // assert(_opened);
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
} // end namespace ChimeraTK

// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "CommandBasedBackend.h"

#include "CommandBasedBackendRegisterAccessor.h"
#include "SerialCommandHandler.h"

namespace ChimeraTK {

  /*****************************************************************************************************************/

  CommandBasedBackend::CommandBasedBackend(std::string serialDevice) : _device(serialDevice), _mux() {
    _commandBasedBackendType = CommandBasedBackendType::SERIAL;
    FILL_VIRTUAL_FUNCTION_TEMPLATE_VTABLE(getRegisterAccessor_impl);

    // fill the catalogue from the map file
    // WORKAROUND: Hard-Code the entries
    DataDescriptor intDataDescriptor(
        DataDescriptor::FundamentalType::numeric, /*isIntegral*/ true, /*isSigned*/ true, /*nDigits*/ 11);
    _backendCatalogue.addRegister({"SOUR:FREQ:CW", "/cwFrequency", 1, 1,
        CommandBasedBackendRegisterInfo::IoDirection::READ_WRITE, {}, intDataDescriptor});
  }

  /*****************************************************************************************************************/

  void CommandBasedBackend::open() {
    switch(_commandBasedBackendType) {
      case CommandBasedBackendType::SERIAL:
        _commandHandler = std::make_unique<SerialCommandHandler>(_device, _timeoutInMilliseconds);
        break;
      default:
        // This is not part of the proper interface. Throw a std::logic_error as
        // intermediate debugging solution
        throw std::logic_error("CommandBasedBackend: FIXME: Unsupported type");
    }

    // Backends have to call this function at the end of a successful open() call
    setOpenedAndClearException();
  } // end open

  /*****************************************************************************************************************/

  void CommandBasedBackend::close() {
    _commandHandler.reset();
    _opened = false;
  }

  /*****************************************************************************************************************/

  RegisterCatalogue CommandBasedBackend::getRegisterCatalogue() const {
    return RegisterCatalogue(_backendCatalogue.clone());
  }

  /*****************************************************************************************************************/

  boost::shared_ptr<DeviceBackend> CommandBasedBackend::createInstance(
      std::string instance, [[maybe_unused]] std::map<std::string, std::string> parameters) {
    return boost::make_shared<CommandBasedBackend>(instance);
  }

  /*****************************************************************************************************************/

  std::string CommandBasedBackend::sendCommand(std::string cmd) {
    assert(_opened);
    std::lock_guard<std::mutex> lock(_mux);

    return _commandHandler->sendCommand(std::move(cmd));
  }

  /*****************************************************************************************************************/

  std::vector<std::string> CommandBasedBackend::sendCommand(std::string cmd, const size_t nLinesExpected) {
    assert(_opened);
    std::lock_guard<std::mutex> lock(_mux);
    return _commandHandler->sendCommand(std::move(cmd), nLinesExpected);
  }

  /*****************************************************************************************************************/

  std::string CommandBasedBackend::readDeviceInfo() {
    return "Device: " + _device + " timeout: " + std::to_string(_timeoutInMilliseconds);
  }

  /*****************************************************************************************************************/

  CommandBasedBackend::BackendRegisterer::BackendRegisterer() {
    BackendFactory::getInstance().registerBackendType(
        "CommandBasedTTY", &CommandBasedBackend::createInstance, {}, CHIMERATK_DEVICEACCESS_VERSION);
  }

  /*****************************************************************************************************************/

  static CommandBasedBackend::BackendRegisterer gCommandBasedBackendRegisterer;
} // end namespace ChimeraTK

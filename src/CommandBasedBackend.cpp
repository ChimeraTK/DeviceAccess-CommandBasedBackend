// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "CommandBasedBackend.h"

#include "SerialCommandHandler.h"

namespace ChimeraTK {

  /********************************************************************************************************************/

  CommandBasedBackend::CommandBasedBackend(std::string serialDevice) : _device(serialDevice), _mux() {
    _commandBasedBackendType = CommandBasedBackendType::SERIAL;
    FILL_VIRTUAL_FUNCTION_TEMPLATE_VTABLE(getRegisterAccessor_impl);

    // fill this from the map file
    _defaultRecoveryRegister = "/cwFrequency";
    _lastWrittenRegister = _defaultRecoveryRegister;

    // fill the catalogue from the map file
    // WORKAROUND: Hard-Code the entries
    DataDescriptor intDataDescriptor(
        DataDescriptor::FundamentalType::numeric, /*isIntegral*/ true, /*isSigned*/ true, /*nDigits*/ 11);

    // smallest possible 5e-324, largest 2e308
    DataDescriptor doubleDataDescriptor(DataDescriptor::FundamentalType::numeric, /*isIntegral*/ false,
        /*isSigned*/ true, /*nDigits*/ 3 + 325, /*nFragtionalDigits*/ 325);

    DataDescriptor stringDataDescriptor(DataDescriptor::FundamentalType::string);

    _backendCatalogue.addRegister({"/cwFrequency", "SOUR:FREQ:CW {{x.0}}", "", "SOUR:FREQ:CW?", "([0-9]+)\r\n"});
    _backendCatalogue.addRegister({"/cwFrequencyRO", "", "", "SOUR:FREQ:CW?", "([0-9]+)\r\n"});

    _backendCatalogue.addRegister({"/ACC", "ACC AXIS_1 {{x.0}} AXIS_2 {{x.1}}", "", "ACC?",
        "AXIS_1={{x.0}}\r\nAXIS_2={{x.1}}\r\n", 2, 2, CommandBasedBackendRegisterInfo::InternalType::DOUBLE});
    _backendCatalogue.addRegister({"/ACCRO", "", "", "ACC?", "AXIS_1={{x.0}}\r\nAXIS_2={{x.1}}\r\n", 2, 2,
        CommandBasedBackendRegisterInfo::InternalType::DOUBLE});

    _backendCatalogue.addRegister({"/SAI", "", "", "SAI?", R"delim({{x.0}}\r\n{{x.1}}\r\n)delim", 2, 2,
        CommandBasedBackendRegisterInfo::InternalType::STRING});

    _backendCatalogue.addRegister({"/myData", "", "", "CALC1:DATA:TRAC? 'myTrace' SDAT",
        R"delim({% for val in x %}{{val}}{% if not loop.is_last %},{% endif %}{% endfor %}\r\n)delim", 10, 1,
        CommandBasedBackendRegisterInfo::InternalType::DOUBLE});
  }

  /********************************************************************************************************************/

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

  boost::shared_ptr<DeviceBackend> CommandBasedBackend::createInstance(
      std::string instance, [[maybe_unused]] std::map<std::string, std::string> parameters) {
    return boost::make_shared<CommandBasedBackend>(instance);
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
    return "Device: " + _device + " timeout: " + std::to_string(_timeoutInMilliseconds);
  }

  /********************************************************************************************************************/

  CommandBasedBackend::BackendRegisterer::BackendRegisterer() {
    BackendFactory::getInstance().registerBackendType(
        "CommandBasedTTY", &CommandBasedBackend::createInstance, {}, CHIMERATK_DEVICEACCESS_VERSION);
  }

  /********************************************************************************************************************/

  static CommandBasedBackend::BackendRegisterer gCommandBasedBackendRegisterer;
} // end namespace ChimeraTK

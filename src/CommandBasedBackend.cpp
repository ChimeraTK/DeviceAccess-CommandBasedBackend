// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "CommandBasedBackend.h"

#include "CommandBasedBackendRegisterAccessor.h"

namespace ChimeraTK {

  /*****************************************************************************************************************/

  CommandBasedBackend::CommandBasedBackend(std::string serialDevice) : _device(serialDevice), _mux() {
    _commandBasedBackendType = CommandBasedBackendType::SERIAL;
    FILL_VIRTUAL_FUNCTION_TEMPLATE_VTABLE(getRegisterAccessor_impl);
    _registerMap = std::make_unique<
        BackendRegisterCatalogue<CommandBasedBackendRegisterInfo>>(); // store a blank RegisterCatalog, TODO
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

  static CommandBasedBackend::BackendRegisterer gCommandBasedBackendRegisterer;
} // end namespace ChimeraTK

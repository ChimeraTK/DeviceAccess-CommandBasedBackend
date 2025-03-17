// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "CommandBasedBackendRegisterInfo.h"
#include "mapFileKeys.h"

#include <utility>

namespace ChimeraTK {

  CommandBasedBackendRegisterInfo::CommandBasedBackendRegisterInfo(
      const RegisterPath& registerPath_, InteractionInfo readInfo_, InteractionInfo writeInfo_, uint nElements_)
  : nElements(nElements_), registerPath(registerPath_), readInfo(std::move(readInfo_)),
    writeInfo(std::move(writeInfo_)) {
    // Set dataDescriptor from type.
    if(type == TransportLayerType::DEC_INT) {
      dataDescriptor = DataDescriptor(DataType::int64);
    }
    else if(type == TransportLayerType::HEX_INT) {
      dataDescriptor = DataDescriptor(DataType::uint64);
    }
    else if(type == TransportLayerType::DEC_FLOAT) {
      dataDescriptor = DataDescriptor(DataType::float64);
    }
    else if(type == TransportLayerType::STRING) {
      dataDescriptor = DataDescriptor(DataType::string);
    }
    else if(type == TransportLayerType::VOID) {
      dataDescriptor = DataDescriptor(DataType::Void);
    }
  } // end constructor

} // namespace ChimeraTK

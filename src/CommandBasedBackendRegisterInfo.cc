// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "CommandBasedBackendRegisterInfo.h"

#include <utility>

namespace ChimeraTK {

  CommandBasedBackendRegisterInfo::CommandBasedBackendRegisterInfo(const RegisterPath& registerPath_,
      std::string writeCommandPattern_, std::string writeResponsePattern_, std::string readCommandPattern_,
      std::string readResponsePattern_, uint nElements_, size_t nLinesReadResponse_,
      CommandBasedBackendRegisterInfo::TransportLayerType type, std::string delimiter_)
  : nElements(nElements_), registerPath(registerPath_), writeCommandPattern(std::move(writeCommandPattern_)),
    writeResponsePattern(std::move(writeResponsePattern_)), readCommandPattern(std::move(readCommandPattern_)),
    readResponsePattern(std::move(readResponsePattern_)), nLinesReadResponse(nLinesReadResponse_),
    transportLayerType(type), delimiter(std::move(delimiter_)) {
    // Set dataDescriptor from type.
    if(type == TransportLayerType::INT64) {
      dataDescriptor = DataDescriptor(DataType::int64);
    }
    else if(type == TransportLayerType::UINT64 or type == TransportLayerType::HEX) {
      dataDescriptor = DataDescriptor(DataType::uint64);
    }
    else if(type == TransportLayerType::DOUBLE) {
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

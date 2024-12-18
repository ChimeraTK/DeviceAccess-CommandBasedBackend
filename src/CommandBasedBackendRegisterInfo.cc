// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "CommandBasedBackendRegisterInfo.h"

namespace ChimeraTK {

  CommandBasedBackendRegisterInfo::CommandBasedBackendRegisterInfo(RegisterPath registerPath_,
      std::string writeCommandPattern_, std::string writeResponsePattern_, std::string readCommandPattern_,
      std::string readResponsePattern_, uint nElements_, size_t nLinesReadResponse_,
      CommandBasedBackendRegisterInfo::InternalType type, std::string delimiter_)
  : nElements(nElements_), registerPath(registerPath_), writeCommandPattern(writeCommandPattern_),
    writeResponsePattern(writeResponsePattern_), readCommandPattern(readCommandPattern_),
    readResponsePattern(readResponsePattern_), nLinesReadResponse(nLinesReadResponse_), internalType(type),
    delimiter(delimiter_) {
    // Set dataDescriptor from type.
    if(type == InternalType::INT64) {
      dataDescriptor = DataDescriptor(DataType::int64);
    }
    else if(type == InternalType::UINT64 or type == InternalType::HEX) {
      dataDescriptor = DataDescriptor(DataType::uint64);
    }
    else if(type == InternalType::DOUBLE) {
      dataDescriptor = DataDescriptor(DataType::float64);
    }
    else if(type == InternalType::STRING) {
      dataDescriptor = DataDescriptor(DataType::string);
    }
    else if(type == InternalType::VOID) {
      dataDescriptor = DataDescriptor(DataType::Void);
    }
  } // end constructor

} // namespace ChimeraTK

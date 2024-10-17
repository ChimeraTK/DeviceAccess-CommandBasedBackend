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
        // set dataDescriptor from type.
        if (type == InternalType::INT64) {
            dataDescriptor = DataDescriptor(
                    DataDescriptor::FundamentalType::numeric, /*isIntegral*/ true, /*isSigned*/ true, /*nDigits*/ 11);
        } else if (type == InternalType::UINT64) {
            dataDescriptor = DataDescriptor(
                    DataDescriptor::FundamentalType::numeric, /*isIntegral*/ true, /*isSigned*/ false, /*nDigits*/ 11);
        } else if (type == InternalType::DOUBLE) {
            // smallest possible 5e-324, largest 2e308
            dataDescriptor = DataDescriptor(DataDescriptor::FundamentalType::numeric, /*isIntegral*/ false,
                    /*isSigned*/ true, /*nDigits*/ 3 + 325, /*nFragmentalDigits*/ 325);
        } else if (type == InternalType::STRING) {
            dataDescriptor = DataDescriptor(DataDescriptor::FundamentalType::string);
        } else if (type == InternalType::VOID) {
            dataDescriptor = DataDescriptor(DataDescriptor::FundamentalType::nodata);
        } 
  } // end constructor

} // namespace ChimeraTK

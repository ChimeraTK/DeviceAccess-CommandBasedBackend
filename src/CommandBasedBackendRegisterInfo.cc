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
    delimiter(delimiter_) {}

  /********************************************************************************************************************/

} // namespace ChimeraTK

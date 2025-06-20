// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#include "CheckSum.h"

#include "stringUtils.h"

#include <openssl/sha.h>

#include <boost/crc.hpp>

// First put the hex through
// std::string binData = binaryStrFromHexStr(hexData, padLeft);

std::string CheckSum8(std::string binData) {
  uint8_t sum = 0;
  for(unsigned char c : binData) {
    sum += c;
  }
  return hexStrFromInt<uint8_t>(sum, WidthOption::TYPE_WIDTH);
}

std::string CheckSumSha256(std::string binData) {
  std::string result(SHA256_DIGEST_LENGTH, '\0');
  SHA256(
      reinterpret_cast<const unsigned char*>(data.data()), data.size(), reinterpret_cast<unsigned char*>(&result[0]));
  return hexStrFromBinaryStr(result);
}

std::string CheckSumCrcCcit16(std::string binData) {
  boost::crc_optimal<16, 0x1021, 0xFFFF, 0x0000, false, false> crc;
  crc.process_bytes(binData.data(), binData.size());
  uint16_t result = crc.checksum();
  return hexStrFromInt<uint16_t>(result, WidthOption::TYPE_WIDTH);
}

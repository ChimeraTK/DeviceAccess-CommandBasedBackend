// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "Checksum.h"

#include "stringUtils.h"

#include <ChimeraTK/Exception.h> //for ChimeraTK::logic_error

#include <openssl/evp.h> // OpenSSL 3.0 EVP API
#include <openssl/sha.h> // For SHA256_DIGEST_LENGTH

#include <boost/crc.hpp>

#include <memory> // For std::unique_ptr
#include <optional>

// First put the hex through
// std::string binData = binaryStrFromHexStr(hexData, padLeft);

static const checksumFunction checksum8 = [](const std::string binData) -> std::string {
  uint8_t sum = 0;
  for(unsigned char c : binData) {
    sum += c;
  }
  return hexStrFromInt<uint8_t>(sum, WidthOption::TYPE_WIDTH).value();
};

/**********************************************************************************************************************/

static const checksumFunction checksum32 = [](const std::string binData) -> std::string {
  uint32_t sum = 0;
  for(unsigned char c : binData) {
    sum += c;
  }
  return hexStrFromInt<uint32_t>(sum, WidthOption::TYPE_WIDTH).value();
};

/**********************************************************************************************************************/

static const checksumFunction checksumCrcCcit16 = [](const std::string binData) -> std::string {
  boost::crc_optimal<16, 0x1021, 0xFFFF, 0x0000, false, false> crc;
  /*
   * see https://www.scadacore.com/tools/programming-calculators/online-checksum-calculator/
   * 16: CRC width is 16 bits.
   * 0x1021: Polynomial.
   * 0xFFFF: Initial value.
   * 0x0000: Final XOR value.
   * false: ReflectIn - input data bits are not reflected.
   * false: ReflectOut - output CRC bits are not reflected.
   */
  crc.process_bytes(binData.data(), binData.size());
  uint16_t result = crc.checksum();
  return hexStrFromInt<uint16_t>(result, WidthOption::TYPE_WIDTH).value();
};

/**********************************************************************************************************************/

static const checksumFunction checksumSha256 = [](const std::string binData) -> std::string {
  std::string binResult(SHA256_DIGEST_LENGTH, '\0');
  std::unique_ptr<EVP_MD_CTX, decltype(&EVP_MD_CTX_free)> ctx(EVP_MD_CTX_new(), &EVP_MD_CTX_free);
  if(not ctx) {
    throw ChimeraTK::runtime_error("Failed to create EVP_MD_CTX");
  }

  if(EVP_DigestInit_ex(ctx.get(), EVP_sha256(), nullptr) != 1 ||
      EVP_DigestUpdate(ctx.get(), binData.data(), binData.size()) != 1 ||
      EVP_DigestFinal_ex(ctx.get(), reinterpret_cast<unsigned char*>(&binResult[0]), nullptr) != 1) {
    throw ChimeraTK::runtime_error("SHA256 computation failed");
  }

  return hexStrFromBinaryStr(binResult);
};

// Add more checksum functions here

/**********************************************************************************************************************/
/**********************************************************************************************************************/

/*
 * checksumMap defines the relationship between checksum names - as used in the map file - and the functions.
 * The left-hand string names must be all lower case.
 * Strings are converted to lower case before searching checksumMap to keep them case insensitive, so the names do not
 * have to be all lower case in the map file.
 */
static const std::unordered_map<std::string, checksumFunction> checksumMap = {
    // clang-format off
      {"cs8", checksum8},
      {"cs32", checksum32},
      {"sha256", checksumSha256},
      {"crcccit16", checksumCrcCcit16}
    // clang-format on
};

/**********************************************************************************************************************/
/**********************************************************************************************************************/

bool isValidChecksumName(std::string name) {
  toLowerCase(name);
  int n = checksumMap.count(name);
  if(n > 1) {
    throw ChimeraTK::logic_error("Checksum \"" + name + "\" is not uniquely defined.");
  }
  return n > 0;
}

/**********************************************************************************************************************/

checksumFunction getChecksumer(std::string name) {
  toLowerCase(name);
  if(not isValidChecksumName(name)) {
    throw ChimeraTK::logic_error("Unknown checksum: " + name);
  }
  return checksumMap.at(name);
}

// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "CheckSum.h"

#include "stringUtils.h"

#include <ChimeraTK/Exception.h> //for ChimeraTK::logic_error

#include <openssl/evp.h> // OpenSSL 3.0 EVP API
#include <openssl/sha.h> // For SHA256_DIGEST_LENGTH

#include <boost/crc.hpp>

#include <iostream> //DEBUG
#include <optional>
//#include <memory>   // For std::unique_ptr

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
  std::string result;
  result.resize(SHA256_DIGEST_LENGTH);

  EVP_MD_CTX* ctx = EVP_MD_CTX_new();
  if(ctx == nullptr) {
    throw ChimeraTK::runtime_error("Failed to create EVP_MD_CTX");
  }

  if(EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr) != 1 || EVP_DigestUpdate(ctx, binData.data(), binData.size()) != 1 ||
      EVP_DigestFinal_ex(ctx, reinterpret_cast<unsigned char*>(&result[0]), nullptr) != 1) {
    EVP_MD_CTX_free(ctx);
    throw ChimeraTK::runtime_error("SHA256 computation failed");
  }

  EVP_MD_CTX_free(ctx);
  return hexStrFromBinaryStr(result);
};

/*
static const checksumFunction checksumSha256 = [](const std::string binData) -> std::string {
    // SHA256_DIGEST_LENGTH is typically 32 bytes
    // We can still use this macro from openssl/sha.h or define 32 directly
    std::string result_binary(SHA256_DIGEST_LENGTH, '\0');
    unsigned int md_len = 0; // To store the actual length written by EVP_DigestFinal_ex

    // 1. Create a message digest context
    EVP_MD_CTX* mdctx = EVP_MD_CTX_new();
    if (mdctx == nullptr) {
        // Handle error: could not create context
        throw std::runtime_error("Failed to create EVP_MD_CTX");
    }

    // 2. Initialize the digest operation for SHA256
    // EVP_sha256() returns the EVP_MD structure for SHA256
    if (1 != EVP_DigestInit_ex(mdctx, EVP_sha256(), nullptr)) {
        EVP_MD_CTX_free(mdctx); // Ensure cleanup on error
        throw std::runtime_error("Failed to initialize SHA256 digest");
    }

    // 3. Update the digest with the input data
    if (1 != EVP_DigestUpdate(mdctx, binData.c_str(), binData.length())) {
        EVP_MD_CTX_free(mdctx); // Ensure cleanup on error
        throw std::runtime_error("Failed to update SHA256 digest");
    }

    // 4. Finalize the digest operation and get the result
    // The hash bytes are written directly into result_binary.data()
    if (1 != EVP_DigestFinal_ex(mdctx, reinterpret_cast<unsigned char*>(result_binary.data()), &md_len)) {
        EVP_MD_CTX_free(mdctx); // Ensure cleanup on error
        throw std::runtime_error("Failed to finalize SHA256 digest");
    }

    // 5. Clean up the context
    EVP_MD_CTX_free(mdctx);

    // Verify the length (optional but good for debugging/robustness)
    if (md_len != SHA256_DIGEST_LENGTH) {
        throw std::runtime_error("SHA256 digest length mismatch");
    }

    return hexStrFromBinaryStr(result_binary);
};
*/

/*static const checksumFunction checksumSha256 = [](const std::string binData) -> std::string {
    std::string result_binary(SHA256_DIGEST_LENGTH, '\0');
    unsigned int md_len = 0;

    // Use std::unique_ptr with our custom deleter for automatic resource management
    // The mdctx will be automatically freed when it goes out of scope.
    std::unique_ptr<EVP_MD_CTX, EVP_MD_CTX_Deleter> mdctx(EVP_MD_CTX_new());
    if (!mdctx) { // Check if allocation failed
        throw ChimeraTK::runtime_error("Failed to create EVP_MD_CTX");
    }

    // Initialize the digest operation for SHA256
    // Use mdctx.get() to pass the raw pointer to OpenSSL functions
    if (1 != EVP_DigestInit_ex(mdctx.get(), EVP_sha256(), nullptr)) {
        throw ChimeraTK::runtime_error("Failed to initialize SHA256 digest");
    }

    // Update the digest with the input data
    if (1 != EVP_DigestUpdate(mdctx.get(), binData.c_str(), binData.length())) {
        throw ChimeraTK::runtime_error("Failed to update SHA256 digest");
    }

    // Finalize the digest operation and get the result
    if (1 != EVP_DigestFinal_ex(mdctx.get(), reinterpret_cast<unsigned char*>(result_binary.data()), &md_len)) {
        throw ChimeraTK::runtime_error("Failed to finalize SHA256 digest");
    }

    // Verify the length (optional but good for robustness)
    if (md_len != SHA256_DIGEST_LENGTH) {
        throw ChimeraTK::runtime_error("SHA256 digest length mismatch");
    }

    return hexStrFromBinaryStr(result_binary);
};*/

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
  std::cout << "Attempting to fetch " << name << std::endl; // DEBUG
  toLowerCase(name);
  if(not isValidChecksumName(name)) {
    throw ChimeraTK::logic_error("Unknown checksum: " + name);
  }
  return checksumMap.at(name);
}

// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "CheckSum.h"

#include "stringUtils.h"

#include <ChimeraTK/Exception.h>

#include <algorithm>
#include <cctype>
//#include <openssl/sha.h>
#include <boost/crc.hpp>
#include <iostream> //DEBUG
#include <optional>

// Implementations of checksum classes below. Just copy-paste classes to make a new one since they register themselves.
// First put the hex through
// std::string binData = binaryStrFromHexStr(hexData, padLeft);

/**********************************************************************************************************************/

class Checksum8 : public Checksum {
 public:
  static std::string staticName() { return "cs8"; }
  std::string name() const override { return staticName(); }

  std::string operator()(const std::string binData) const override {
    uint8_t sum = 0;
    for(unsigned char c : binData) {
      sum = ((sum + c) % 256);
    }
    return hexStrFromInt<uint8_t>(sum, WidthOption::TYPE_WIDTH).value();
  }
};
static ChecksumRegistrar<Checksum8> regCs8; //registration

/**********************************************************************************************************************/

class Checksum32 : public Checksum {
 public:
  static std::string staticName() { return "cs32"; }
  std::string name() const override { return staticName(); }

  std::string operator()(const std::string binData) const override {
    uint32_t sum = 0;
    for(unsigned char c : binData) {
      sum += c;
    }
    return hexStrFromInt<uint32_t>(sum, WidthOption::TYPE_WIDTH).value();
  }
};
static ChecksumRegistrar<Checksum32> regCs32; //registration

/**********************************************************************************************************************/

class ChecksumCrcCcit16 : public Checksum {
 public:
  static std::string staticName() { return "CrcCcit16"; }
  std::string name() const override { return staticName(); }

  std::string operator()(const std::string binData) const override {
    boost::crc_optimal<16, 0x1021, 0xFFFF, 0x0000, false, false> crc;
    crc.process_bytes(binData.data(), binData.size());
    uint16_t result = crc.checksum();
    return hexStrFromInt<uint16_t>(result, WidthOption::TYPE_WIDTH).value();
  }
};
static ChecksumRegistrar<ChecksumCrcCcit16> regCc16; //registration

/**********************************************************************************************************************/

/*class ChecksumSha256: public Checksum {
public:
    static std::string staticName() { return "SHA256"; }
    std::string name() const override { return staticName(); }

    std::string operator()(const std::string binData) const override {
        //SHA256_DIGEST_LENGTH should be a defined constant in openssl/sha
        std::string result(SHA256_DIGEST_LENGTH, '\0');
        SHA256(
                reinterpret_cast<const unsigned char*>(binData.data()), binData.size(), reinterpret_cast<unsigned
char*>(&result[0])
              );

        return hexStrFromBinaryStr(result);
    }

};
static ChecksumRegistrar<ChecksumSha256> regSha256; //registration
*/

// Add more checksum classes here

/**********************************************************************************************************************/
/**********************************************************************************************************************/

static std::string toLowerCase(std::string s) {
  std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });
  return s;
}

/**********************************************************************************************************************/

void ChecksumFactory::registerChecksumer(const std::string& name, ChecksumCreator creator) {
  std::string lowerName = toLowerCase(name);
  if(_registry.count(lowerName) != 0) {
    throw ChimeraTK::logic_error("Checksum name already registered: " + lowerName);
  }
  std::cout<<"Registering "<<lowerName <<std::endl;
  _registry[lowerName] = std::move(creator);
}

/**********************************************************************************************************************/

std::unique_ptr<Checksum> ChecksumFactory::makeChecksumer(const std::string& name) const {
  std::string lowerName = toLowerCase(name);
  auto it = _registry.find(lowerName );
  std::cout<<"Attempting to fetch "<<lowerName <<std::endl;
  if(it == _registry.end()) {
    throw ChimeraTK::logic_error("Unknown checksum: " + name);
  }
  return it->second();
}


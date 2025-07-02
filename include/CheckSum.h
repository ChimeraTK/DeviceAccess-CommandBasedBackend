// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include <algorithm>
#include <cctype>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

/**
 * Copy-paste the daughters of AutoRegisteredChecksum in the .cc file to add more checksum types.
 *
 * Application:
 * std::string binData = binaryStrFromHexStr(hexData, padLeft); //If you are starting from a string of hexidecimal
 * std::string checksumTag = "myChecksum"; // configuration, case insensitive
 * auto checksumer = makeChecksumer(checksumTag);
 * std::string hexidecimalChecksumResult = (*checksumer)(binData);
 */

/**
 * @brief Abstract base class for checksum calculators.
 * Each derived class must implement the checksum calculation operator and provide a name.
 */
class Checksum {
 public:
  virtual ~Checksum() = default;
  virtual std::string operator()(const std::string data) const = 0;
  virtual std::string name() const = 0; // Stored case sensitive, but evaluated case insensitive.
                                        // static std::string staticName() is also required for each implementation.
};

/**********************************************************************************************************************/

inline std::unique_ptr<Checksum> makeChecksumer(const std::string& name);

/**********************************************************************************************************************/
/**********************************************************************************************************************/
/**********************************************************************************************************************/

/**
 * @brief Singleton factory for creating checksum implementations by name.
 * This class maintains a registry of available checksum types,
 * and allows creation based on string names (case-insensitive).
 */
class ChecksumFactory {
 public:
  using ChecksumCreator = std::function<std::unique_ptr<Checksum>()>;

  /**
   * @brief Get the singleton instance of the factory.
   * @return Reference to the static ChecksumFactory instance.
   */
  static ChecksumFactory& getStaticInstance() {
    static ChecksumFactory instance;
    return instance;
  }

  /**
   * @brief Register a new checksum type with a name.
   * @param name Name of the checksum (case-insensitive).
   * @param creator Function that creates a new instance of the checksum.
   * @throws ChimeraTK::logic_error if the name is already registered.
   */
  void registerChecksumer(const std::string& name, ChecksumCreator creator);

  /**
   * @brief Create a checksum instance by name.
   * @param name Case-insensitive name of the checksum.
   * @return A unique pointer to the checksum implementation.
   * @throws ChimeraTK::logic_error if the name is not found.
   */
  std::unique_ptr<Checksum> makeChecksumer(const std::string& name) const;

 private:
  std::unordered_map<std::string, ChecksumCreator> _registry;
};

/**********************************************************************************************************************/

//Helper template for concise registration
template<typename T>
struct ChecksumRegistrar {
    ChecksumRegistrar() {
        ChecksumFactory::getStaticInstance().registerChecksumer(
            T::staticName(), [] { return std::make_unique<T>(); });
    }
};

/**********************************************************************************************************************/

inline std::unique_ptr<Checksum> makeChecksumer(const std::string& name) {
  return ChecksumFactory::getStaticInstance().makeChecksumer(name);
}

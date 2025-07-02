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

std::unique_ptr<Checksum> makeChecksumer(const std::string& name);

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

/**
 * @brief CRTP base class for automatic registration of checksum implementations.
 *
 * Derived classes must implement:
 * - static std::string staticName();
 * - the checksum operator()
 * - the name() method (typically returning staticName())
 *
 * The constructor of this class automatically registers the derived class
 * with the ChecksumFactory under its staticName.
 *
 * @tparam Derived The derived checksum class.
 */
template<typename Derived>
class AutoRegisteredChecksum : public Checksum {
 protected:
  AutoRegisteredChecksum() {
    /*
     * This line ensures that the actual type of the class being defined (*this) is exactly equal to the Derived
     * template argument. It protect from copy-paste mistakes in derrived class signitures like class XorChecksum :
     * public AutoRegisteredChecksum<XorChecksum> So that the first and second instance of the term XorChecksum must
     * match.
     */
    static_assert(std::is_same<Derived, std::decay_t<decltype(*this)>>::value,
        "CRTP misused: Derived type doesn't match class definition.");

    // Verify that Derived inherits from AutoRegisteredChecksum
    static_assert(std::is_base_of<AutoRegisteredChecksum<Derived>, Derived>::value,
        "CRTP misused: class must inherit AutoRegisteredChecksum<ExactSameType>");
  }

 private:
  struct ChecksumerRegistrar {
    ChecksumerRegistrar() {
      ChecksumFactory::getStaticInstance().registerChecksumer(
          Derived::staticName(), [] { return std::make_unique<Derived>(); });
    }
  };
  static inline ChecksumerRegistrar _registrar;
};

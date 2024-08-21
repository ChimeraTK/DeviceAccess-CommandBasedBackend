// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#define BOOST_TEST_DYN_LINK
// Define a name for the test module.
#define BOOST_TEST_MODULE CommandBasedBackend
// Only after defining the name include the unit test header.
#include <boost/test/unit_test.hpp>
using namespace boost::unit_test_framework;

#include "DummyServer.h"

#include <ChimeraTK/UnifiedBackendTest.h>

using namespace ChimeraTK;

/**********************************************************************************************************************/

static DummyServer dummyServer;

std::string cdd() {
  return "(CommandBasedTTY:" + dummyServer.deviceNode + "?map=test.cmdmap)";
}

/**********************************************************************************************************************/

struct ScalarInt {
  static std::string path() { return "/cwFrequency"; }
  static bool isWriteable() { return true; }
  static bool isReadable() { return true; }
  static ChimeraTK::AccessModeFlags supportedFlags() { return {}; }
  static size_t nChannels() { return 1; }
  static size_t nElementsPerChannel() { return 1; }
  static size_t nRuntimeErrorCases() { return 0; }

  using minimumUserType = int32_t;
  using rawUserType = int32_t;

  static constexpr auto capabilities = TestCapabilities<>()
                                           .disableForceDataLossWrite()
                                           .disableAsyncReadInconsistency()
                                           .disableSwitchReadOnly()
                                           .disableSwitchWriteOnly()
                                           .disableTestWriteNeverLosesData()
                                           .disableTestRawTransfer();

  template<typename Type>
  std::vector<std::vector<Type>> generateValue([[maybe_unused]] bool raw = false) {
    return {{Type(dummyServer.cwFrequency + 3)}};
  }

  template<typename UserType>
  std::vector<std::vector<UserType>> getRemoteValue([[maybe_unused]] bool raw = false) {
    assert(!raw);
    return {{UserType(dummyServer.cwFrequency)}};
  }

  void setRemoteValue() { dummyServer.cwFrequency = generateValue<minimumUserType>()[0][0]; }

  void setForceRuntimeError([[maybe_unused]] bool enable, [[maybe_unused]] size_t caseNr) {}
};

/**********************************************************************************************************************/

struct StringArray {
  static std::string path() { return "/SAI"; }
  static bool isWriteable() { return false; }
  static bool isReadable() { return true; }
  using minimumUserType = std::string;
  static ChimeraTK::AccessModeFlags supportedFlags() { return {}; }

  static size_t nChannels() { return 1; }
  static size_t nElementsPerChannel() { return 2; }
  static size_t writeQueueLength() { return std::numeric_limits<size_t>::max(); }
  static size_t nRuntimeErrorCases() { return 1; }

  static constexpr auto capabilities = TestCapabilities<>()
                                           .disableForceDataLossWrite()
                                           .disableAsyncReadInconsistency()
                                           .disableSwitchReadOnly()
                                           .disableSwitchWriteOnly()
                                           .disableTestWriteNeverLosesData()
                                           .disableTestRawTransfer();

  std::string textBase{"Some $%#@! characters "};
  size_t counter{0};
  const size_t asciiLength{35};

  template<typename UserType>
  std::vector<std::vector<UserType>> generateValue() {
    std::vector<std::string> out(nElementsPerChannel());
    for(auto& t : out) {
      t = textBase + std::to_string(counter++);
    }
    return {out};
  }

  template<typename UserType>
  std::vector<std::vector<UserType>> getRemoteValue() {
    std::vector<std::string> out(nElementsPerChannel());

    for(size_t i = 0; i < nElementsPerChannel(); ++i) {
      out[i] = dummyServer.sai[i];
    }
    return {out};
  }

  void setRemoteValue() {
    auto v = generateValue<minimumUserType>();
    for(size_t i = 0; i < nElementsPerChannel(); ++i) {
      dummyServer.sai[i] = v[0][i];
    }

    std::cout << "setRemoteValue: " << v[0][0] << " , " << v[0][1] << std::endl;
  }

  static void setForceRuntimeError([[maybe_unused]] bool enable, [[maybe_unused]] size_t caseNr) {}
};

/**********************************************************************************************************************/

struct ArrayFloatMultiLine {
  static std::string path() { return "/ACC"; }
  static bool isWriteable() { return true; }
  static bool isReadable() { return true; }
  static ChimeraTK::AccessModeFlags supportedFlags() { return {}; }

  static size_t nChannels() { return 1; }
  static size_t nElementsPerChannel() { return 2; }
  static size_t nRuntimeErrorCases() { return 0; } // FIXME
  using minimumUserType = float;
  using rawUserType = int32_t;

  static constexpr auto capabilities = TestCapabilities<>()
                                           .disableForceDataLossWrite()
                                           .disableAsyncReadInconsistency()
                                           .disableSwitchReadOnly()
                                           .disableSwitchWriteOnly()
                                           .disableTestWriteNeverLosesData()
                                           .disableTestRawTransfer();

  template<typename UserType>
  std::vector<std::vector<UserType>> generateValue([[maybe_unused]] bool raw = false) {
    assert(!raw);
    std::vector<UserType> v(nElementsPerChannel());
    for(uint32_t e = 0; e < nElementsPerChannel(); ++e) {
      v[e] = dummyServer.acc[e] + 1.7F + 3.123F * float(e);
    }
    return {v};
  }

  template<typename UserType>
  std::vector<std::vector<UserType>> getRemoteValue([[maybe_unused]] bool raw = false) {
    assert(!raw);
    std::vector<UserType> v(nElementsPerChannel());
    for(uint32_t e = 0; e < nElementsPerChannel(); ++e) {
      v[e] = dummyServer.acc[e];
    }
    return {v};
  }

  void setRemoteValue() {
    auto val = generateValue<minimumUserType>();
    for(uint32_t e = 0; e < nElementsPerChannel(); ++e) {
      dummyServer.acc[e] = val[0][e];
    }
  }

  void setForceRuntimeError([[maybe_unused]] bool enable, [[maybe_unused]] size_t caseNr) {}
};

struct ArrayFloatSingleLine {
  static std::string path() { return "/myData"; }
  static bool isWriteable() { return false; }
  static bool isReadable() { return true; }
  static ChimeraTK::AccessModeFlags supportedFlags() { return {}; }

  static size_t nChannels() { return 1; }
  static size_t nElementsPerChannel() { return 10; }
  static size_t nRuntimeErrorCases() { return 0; } // FIXME
  using minimumUserType = float;
  using rawUserType = int32_t;

  static constexpr auto capabilities = TestCapabilities<>()
                                           .disableForceDataLossWrite()
                                           .disableAsyncReadInconsistency()
                                           .disableSwitchReadOnly()
                                           .disableSwitchWriteOnly()
                                           .disableTestWriteNeverLosesData()
                                           .disableTestRawTransfer();

  template<typename UserType>
  std::vector<std::vector<UserType>> generateValue(bool raw = false) {
    assert(!raw);
    std::vector<UserType> v(nElementsPerChannel());
    for(uint32_t e = 0; e < nElementsPerChannel(); ++e) {
      v[e] = dummyServer.trace[e] + 2.7F + 2.238F * float(e);
    }
    return {v};
  }

  template<typename UserType>
  std::vector<std::vector<UserType>> getRemoteValue([[maybe_unused]] bool raw = false) {
    assert(!raw);
    std::vector<UserType> v(nElementsPerChannel());
    for(uint32_t e = 0; e < nElementsPerChannel(); ++e) {
      v[e] = dummyServer.trace[e];
    }
    return {v};
  }

  void setRemoteValue() {
    auto v = generateValue<minimumUserType>(true);
    for(uint32_t e = 0; e < nElementsPerChannel(); ++e) {
      dummyServer.trace[e] = v[0][e];
    }
  }

  void setForceRuntimeError([[maybe_unused]] bool enable, size_t) {}
};

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testRegisterAccessor) {
  std::cout << "*** testRegisterAccessor *** " << std::endl;
  ChimeraTK::UnifiedBackendTest<>()
      .addRegister<ScalarInt>()
      .addRegister<ArrayFloatMultiLine>()
      .addRegister<ArrayFloatSingleLine>()
      .addRegister<StringArray>()
      .runTests(cdd());
}

/**********************************************************************************************************************/

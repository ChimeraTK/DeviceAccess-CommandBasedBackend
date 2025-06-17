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

constexpr bool DEBUG = false;

/**********************************************************************************************************************/

static DummyServer dummyServer{true, DEBUG};

std::string cdd() {
  return "(CommandBasedTTY:" + dummyServer.deviceNode + "?map=test.json)";
}

/**********************************************************************************************************************/

// Common parts of the register descriptors to avoid code duplication.
struct RegisterDescriptorBase {
  static bool isReadable() { return true; }
  static ChimeraTK::AccessModeFlags supportedFlags() { return {}; }
  static size_t nChannels() { return 1; }
  static size_t nRuntimeErrorCases() { return 1; }

  static constexpr auto capabilities = TestCapabilities<>()
                                           .disableForceDataLossWrite()
                                           .disableAsyncReadInconsistency()
                                           .disableSwitchReadOnly()
                                           .disableSwitchWriteOnly()
                                           .disableTestWriteNeverLosesData()
                                           .disableTestRawTransfer();

  static void setForceRuntimeError(bool enable, [[maybe_unused]] size_t caseNr) {
    // might be called with arbitrary caseNumber from a derived class
    if(enable) {
      // enable the error case -> deactivate the dummy
      dummyServer.deactivate();
    }
    else {
      // turn off the error -> reactivate
      dummyServer.activate();
    }
  }
};

/**********************************************************************************************************************/

// Handles cases 0 to 3 (wrong/incomplete responses or syntax errors)
void setForceReadError(bool enable, size_t caseNr) {
  switch(caseNr) {
    case 0:
      dummyServer.sendNothing = enable;
      break;
    case 1:
      dummyServer.sendTooFew = enable;
      break;
    case 2:
      dummyServer.responseWithDataAndSyntaxError = enable;
      break;
    case 3:
      dummyServer.sendGarbage = enable;
      break;
    default:
      throw ChimeraTK::logic_error("Unknown error scenario " + std::to_string(caseNr));
  }
}

/**********************************************************************************************************************/

// A "hacked" void write test. The unified backend test is designed to compare values of written data to detect a
// successful write, which conceptually does not work for void. Instead, this register descriptor uses the
// minimalUserType Boolean and returns "true" as generated remote value. The actual test whether writing was successful
// is done in getRemoteValue().
//
// If this tests should fail, consider implementing a proper Void test in the UnifiedBackendTest instead of patching up
// this hack!
struct VoidType : public RegisterDescriptorBase {
  static std::string path() { return "/emergencyStopMovement"; }
  static bool isReadable() { return false; }
  static bool isWriteable() { return true; }
  static size_t nElementsPerChannel() { return 1; }

  using minimumUserType = ChimeraTK::Boolean;

  // Always reset to 0. Not needed as this is only used for the reading test
  static void setRemoteValue() { dummyServer.voidCounter = 0; }

  // GenerateValue usually ensures that the value used for the write test is different from the values used before to
  // make sure the test is sensitive. This does not apply for void. However, it is also the expected value after the
  // write test. In this hack we return "true" and expect "getRemoteValue", which does the
  // actual test here, also to return "true".
  template<typename Type>
  std::vector<std::vector<Type>> generateValue([[maybe_unused]] bool raw = false) {
    // Reset the write counter. If a write was successful, it is increased in the dummy server when it receives the
    // command.
    dummyServer.voidCounter = 0;
    return {{Type(true)}};
  }

  // Return whether the void counter has increased as "value".
  // The actual comparison of the counter, i.e. the logical evaluation of the test, is done here.
  // This allows to print a dedicated warning for this hacked void write test which improves the comprehensibility
  // of the error message. The unified backend test reporting "false is not true" for a void type register is not
  // very helpful.
  template<typename UserType>
  std::vector<std::vector<UserType>> getRemoteValue([[maybe_unused]] bool raw = false) {
    // Only check that the counter is not 0 any more, i.e. the write has been done at least once.
    // It might be that in future the UnifiedBackendTest is calling write multiple times to implement further tests,
    // and this test should not break then. To check that it is written exactly once a dedicated Void test should be
    // introduced to the UnifiedBackendTest.
    // We wait with timeout because the counter is incremented in another thread inside the dummy server.
    auto stopTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(3000);
    while(dummyServer.voidCounter == 0) {
      if(std::chrono::steady_clock::now() > stopTime) {
        return {{UserType(false)}};
      }
      usleep(1000);
    }
    return {{UserType(true)}};
  }
};

/**********************************************************************************************************************/

struct ScalarInt : public RegisterDescriptorBase {
  static std::string path() { return "/cwFrequency"; }
  static bool isWriteable() { return true; }
  static size_t nElementsPerChannel() { return 1; }

  using minimumUserType = int32_t;

  template<typename Type>
  std::vector<std::vector<Type>> generateValue([[maybe_unused]] bool raw = false) {
    return {{Type(dummyServer.cwFrequency + 3)}};
  }

  template<typename UserType>
  std::vector<std::vector<UserType>> getRemoteValue([[maybe_unused]] bool raw = false) {
    return {{UserType(dummyServer.cwFrequency)}};
  }

  void setRemoteValue() { dummyServer.cwFrequency = generateValue<minimumUserType>()[0][0]; }
};

/**********************************************************************************************************************/

// Adding error scenarios which only work in read only as they don't apply for writing.
struct ScalarIntRO : public ScalarInt {
  static std::string path() { return "/cwFrequencyRO"; }
  static size_t nRuntimeErrorCases() { return 4; }
  static bool isWriteable() { return false; }

  static void setForceRuntimeError(bool enable, size_t caseNr) { setForceReadError(enable, caseNr); }
};

/**********************************************************************************************************************/

struct StringArray : public RegisterDescriptorBase {
  static std::string path() { return "/SAI"; }
  static bool isWriteable() { return false; }
  static size_t nElementsPerChannel() { return 2; }
  static size_t writeQueueLength() { return std::numeric_limits<size_t>::max(); }
  static size_t nRuntimeErrorCases() { return 3; }

  using minimumUserType = std::string;

  std::string textBase{"Some $%#@! characters "};
  size_t counter{0};

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
  }

  static void setForceRuntimeError(bool enable, size_t caseNr) {
    // Only the first two tests of setForceReadError are viable for an uninterpreted string.
    if(caseNr < 2) {
      setForceReadError(enable, caseNr);
    }
    else {
      RegisterDescriptorBase::setForceRuntimeError(enable, caseNr);
    }
  }
};

/**********************************************************************************************************************/

struct ArrayFloatMultiLine : public RegisterDescriptorBase {
  static std::string path() { return "/ACC"; }
  static bool isWriteable() { return true; }
  static size_t nElementsPerChannel() { return 2; }

  using minimumUserType = float;

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
};

/**********************************************************************************************************************/

// Adding error scenarios which only work in read only as they don't apply for writing.
struct ArrayFloatMultiLineRO : public ArrayFloatMultiLine {
  static std::string path() { return "/ACCRO"; }
  static size_t nRuntimeErrorCases() { return 4; }
  static bool isWriteable() { return false; }

  static void setForceRuntimeError(bool enable, size_t caseNr) { setForceReadError(enable, caseNr); }
};

/**********************************************************************************************************************/

struct ArrayFloatSingleLine : public RegisterDescriptorBase {
  static std::string path() { return "/myData"; }
  static bool isWriteable() { return false; }
  static size_t nElementsPerChannel() { return 10; }
  static size_t nRuntimeErrorCases() { return 5; }

  using minimumUserType = float;

  template<typename UserType>
  std::vector<std::vector<UserType>> generateValue([[maybe_unused]] bool raw = false) {
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
    auto v = generateValue<minimumUserType>();
    for(uint32_t e = 0; e < nElementsPerChannel(); ++e) {
      dummyServer.trace[e] = v[0][e];
    }
  }
  static void setForceRuntimeError(bool enable, size_t caseNr) {
    if(caseNr < 4) {
      setForceReadError(enable, caseNr);
    }
    else {
      RegisterDescriptorBase::setForceRuntimeError(enable, caseNr);
    }
  }
};

/**********************************************************************************************************************/

struct ArrayHexMultiLine : public RegisterDescriptorBase {
  static std::string path() { return "/myHex"; }
  static bool isWriteable() { return true; }
  static size_t nElementsPerChannel() { return 3; }

  using minimumUserType = uint64_t;

  template<typename UserType>
  std::vector<std::vector<UserType>> generateValue([[maybe_unused]] bool raw = false) {
    assert(!raw);
    std::vector<UserType> v(nElementsPerChannel());
    for(uint32_t e = 0; e < nElementsPerChannel(); ++e) {
      v[e] = userTypeToUserType<UserType>(uint64_t(dummyServer.hex[e])) + 17 + 3 * e;
    }
    return {v};
  }

  template<typename UserType>
  std::vector<std::vector<UserType>> getRemoteValue([[maybe_unused]] bool raw = false) {
    assert(!raw);
    std::vector<UserType> v(nElementsPerChannel());
    for(uint32_t e = 0; e < nElementsPerChannel(); ++e) {
      v[e] = userTypeToUserType<UserType>(uint64_t(dummyServer.hex[e]));
    }
    return {v};
  }

  void setRemoteValue() {
    auto val = generateValue<minimumUserType>();
    for(uint32_t e = 0; e < nElementsPerChannel(); ++e) {
      dummyServer.hex[e] = val[0][e];
    }
  }
};

/**********************************************************************************************************************/

struct ArrayHexInt8 : public ArrayHexMultiLine {
  static std::string paty() { return "/myHex"; }
  using minimumUserType = uint8_t;
};

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testRegisterAccessor) {
  ChimeraTK::UnifiedBackendTest<>()
      .addRegister<ScalarInt>()
      .addRegister<ScalarIntRO>()
      .addRegister<ArrayFloatMultiLine>()
      .addRegister<ArrayFloatMultiLineRO>()
      .addRegister<ArrayFloatSingleLine>()
      .addRegister<StringArray>()
      .addRegister<VoidType>()
      .addRegister<ArrayHexMultiLine>()
      .addRegister<ArrayHexInt8>()
      .runTests(cdd());
}

/**********************************************************************************************************************/

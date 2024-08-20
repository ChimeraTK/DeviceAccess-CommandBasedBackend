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

// common part of the register descriptors to avoid code duplication
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

// handles cases 0 to 3 (wrong/incomplete responses or syntax errors)
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

// Adding error scenarios which only work in read only as they don't apply for writing
struct ScalarIntRO : public ScalarInt {
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
  static size_t nRuntimeErrorCases() { return 5; }

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

    std::cout << "setRemoteValue: " << v[0][0] << " , " << v[0][1] << std::endl;
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

// Adding error scenarios which only work in read only as they don't apply for writing
struct ArrayFloatMultiLineRO : public ArrayFloatMultiLine {
  static size_t nRuntimeErrorCases() { return 4; }

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

BOOST_AUTO_TEST_CASE(testRegisterAccessor) {
  std::cout << "*** testRegisterAccessor *** " << std::endl;
  ChimeraTK::UnifiedBackendTest<>()
      .addRegister<ScalarInt>()
      //.addRegister<ScalarIntRO>()
      .addRegister<ArrayFloatMultiLine>()
      //.addRegister<ArrayFloatMultiLineRO>()
      .addRegister<ArrayFloatSingleLine>()
      .addRegister<StringArray>()
      .runTests(cdd());
}

/**********************************************************************************************************************/

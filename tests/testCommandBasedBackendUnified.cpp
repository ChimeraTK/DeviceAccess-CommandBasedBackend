// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#define BOOST_TEST_DYN_LINK
// Define a name for the test module.
#define BOOST_TEST_MODULE CommandBasedBackend
// Only after defining the name include the unit test header.
#include <boost/test/unit_test.hpp>
using namespace boost::unit_test_framework;

#include <ChimeraTK/BackendFactory.h>
#include <ChimeraTK/Device.h>
#include <ChimeraTK/DummyBackend.h>
#include <ChimeraTK/DummyRegisterAccessor.h>
#include <ChimeraTK/ExceptionDummyBackend.h>
#include <ChimeraTK/TransferGroup.h>
#include <ChimeraTK/UnifiedBackendTest.h>

using namespace ChimeraTK;

/**********************************************************************************************************************/

static std::string cdd("(ExceptionDummy:1?map=mock.map)");
static auto exceptionDummy =
    boost::dynamic_pointer_cast<ExceptionDummy>(BackendFactory::getInstance().createBackend(cdd));

/**********************************************************************************************************************/

struct ScalarInt {
  std::string path() { return "/cwFrequency"; }
  bool isWriteable() { return true; }
  bool isReadable() { return true; }
  ChimeraTK::AccessModeFlags supportedFlags() { return {ChimeraTK::AccessMode::raw}; }
  size_t nChannels() { return 1; }
  size_t nElementsPerChannel() { return 1; }
  size_t nRuntimeErrorCases() { return 1; }
  typedef int32_t minimumUserType;
  typedef int32_t rawUserType;

  static constexpr auto capabilities = TestCapabilities<>()
                                           .disableForceDataLossWrite()
                                           .disableAsyncReadInconsistency()
                                           .disableSwitchReadOnly()
                                           .disableSwitchWriteOnly()
                                           .disableTestWriteNeverLosesData()
                                           .enableTestRawTransfer();

  DummyRegisterAccessor<int32_t> acc{exceptionDummy.get(), "", path()};

  template<typename Type>
  std::vector<std::vector<Type>> generateValue([[maybe_unused]] bool raw = false) {
    return {{Type(acc + 3)}};
  }

  template<typename UserType>
  std::vector<std::vector<UserType>> getRemoteValue([[maybe_unused]] bool raw = false) {
    return {{UserType(acc)}};
  }

  void setRemoteValue() { acc = generateValue<minimumUserType>()[0][0]; }

  void setForceRuntimeError(bool enable, size_t) {
    exceptionDummy->throwExceptionRead = enable;
    exceptionDummy->throwExceptionWrite = enable;
    exceptionDummy->throwExceptionOpen = enable;
  }
};

/**********************************************************************************************************************/

struct StringArray {
  static std::string path() { return "/SAI"; }
  static bool isWriteable() { return false; }
  static bool isReadable() { return true; }
  using minimumUserType = std::string;
  static ChimeraTK::AccessModeFlags supportedFlags() { return {}; }

  static size_t nChannels() { return 1; }
  static size_t nElementsPerChannel() { return 4; }
  static size_t writeQueueLength() { return std::numeric_limits<size_t>::max(); }
  static size_t nRuntimeErrorCases() { return 1; }

  static constexpr auto capabilities = TestCapabilities<>()
                                           .disableForceDataLossWrite()
                                           .disableAsyncReadInconsistency()
                                           .disableSwitchReadOnly()
                                           .disableSwitchWriteOnly()
                                           .disableTestWriteNeverLosesData()
                                           .disableTestRawTransfer();

  // cheating for mock test (we are not testing the dummy here:
  // use front door for convenience
  boost::shared_ptr<NDRegisterAccessor<std::string>> acc{
      exceptionDummy->getRegisterAccessor<std::string>("/SAI/DUMMY_WRITEABLE", 4, 0, {})};

  std::string textBase{"Some $%#@! characters "};
  size_t counter{0};
  const size_t asciiLength{35};

  template<typename UserType>
  std::vector<std::vector<UserType>> generateValue() {
    std::vector<std::string> out(acc->getNumberOfSamples());
    for(auto& t : out) {
      t = textBase + std::to_string(counter++);
    }
    return {out};
  }

  template<typename UserType>
  std::vector<std::vector<UserType>> getRemoteValue() {
    std::vector<std::string> out(acc->getNumberOfSamples());
    auto wasOpen = exceptionDummy->isOpen();
    exceptionDummy->open();
    acc->read();
    for(size_t i = 0; i < acc->getNumberOfSamples(); ++i) {
      out[i] = acc->accessData(i);
    }
    if(!wasOpen) {
      exceptionDummy->close();
    }
    return {out};
  }

  void setRemoteValue() {
    auto v = generateValue<minimumUserType>();
    for(size_t i = 0; i < acc->getNumberOfSamples(); ++i) {
      acc->accessData(i) = v[0][i];
    }
    auto wasOpen = exceptionDummy->isOpen();
    exceptionDummy->open();

    acc->write();

    if(!wasOpen) {
      exceptionDummy->close();
    }
    std::cout << "setRemoteValue: " << v[0][0] << std::endl;
  }

  static void setForceRuntimeError(bool enable, size_t) {
    exceptionDummy->throwExceptionRead = enable;
    exceptionDummy->throwExceptionWrite = enable;
    exceptionDummy->throwExceptionOpen = enable;
  }
};

/**********************************************************************************************************************/

struct ArrayFloatMultiLine {
  std::string path() { return "/ACC"; }
  bool isWriteable() { return true; }
  bool isReadable() { return true; }
  ChimeraTK::AccessModeFlags supportedFlags() { return {ChimeraTK::AccessMode::raw}; }

  size_t nChannels() { return 1; }
  size_t nElementsPerChannel() { return 2; }
  size_t nRuntimeErrorCases() { return 0; } // FIXME
  typedef float minimumUserType;
  typedef int32_t rawUserType;

  static constexpr auto capabilities = TestCapabilities<>()
                                           .disableForceDataLossWrite()
                                           .disableAsyncReadInconsistency()
                                           .disableSwitchReadOnly()
                                           .disableSwitchWriteOnly()
                                           .disableTestWriteNeverLosesData()
                                           .enableTestRawTransfer();

  DummyRegisterAccessor<int32_t> rawAcc{exceptionDummy.get(), "", path()};

  template<typename UserType>
  std::vector<std::vector<UserType>> generateValue(bool raw = false) {
    std::vector<UserType> v(nElementsPerChannel());
    for(uint32_t e = 0; e < nElementsPerChannel(); ++e) {
      auto r = int32_t(rawAcc[e]);
      auto val = *reinterpret_cast<float*>(&r) + 1.7F + 3.123F * float(e);
      if(raw) {
        v[e] = *reinterpret_cast<int32_t*>(&val);
      }
      else {
        v[e] = val;
      }
    }
    return {v};
  }

  template<typename UserType>
  std::vector<std::vector<UserType>> getRemoteValue([[maybe_unused]] bool raw = false) {
    std::vector<UserType> v(nElementsPerChannel());
    for(uint32_t e = 0; e < nElementsPerChannel(); ++e) {
      if(raw) {
        v[e] = rawAcc[e];
      }
      else {
        auto r = int32_t(rawAcc[e]);
        v[e] = *reinterpret_cast<float*>(&r);
      }
    }
    return {v};
  }

  void setRemoteValue() {
    auto rawVal = generateValue<rawUserType>(true);
    for(uint32_t e = 0; e < nElementsPerChannel(); ++e) {
      rawAcc[e] = rawVal[0][e];
    }
  }

  void setForceRuntimeError(bool enable, size_t) {
    exceptionDummy->throwExceptionRead = enable;
    exceptionDummy->throwExceptionWrite = enable;
    exceptionDummy->throwExceptionOpen = enable;
  }
};

struct ArrayFloatSingleLine {
  std::string path() { return "/myData"; }
  bool isWriteable() { return false; }
  bool isReadable() { return true; }
  ChimeraTK::AccessModeFlags supportedFlags() { return {ChimeraTK::AccessMode::raw}; }

  size_t nChannels() { return 1; }
  size_t nElementsPerChannel() { return 10; }
  size_t nRuntimeErrorCases() { return 0; } // FIXME
  typedef float minimumUserType;
  typedef int32_t rawUserType;

  static constexpr auto capabilities = TestCapabilities<>()
                                           .disableForceDataLossWrite()
                                           .disableAsyncReadInconsistency()
                                           .disableSwitchReadOnly()
                                           .disableSwitchWriteOnly()
                                           .disableTestWriteNeverLosesData()
                                           .enableTestRawTransfer();

  DummyRegisterAccessor<float> rawAcc{exceptionDummy.get(), "", path()};

  template<typename UserType>
  std::vector<std::vector<UserType>> generateValue(bool raw = false) {
    std::vector<UserType> v(nElementsPerChannel());
    for(uint32_t e = 0; e < nElementsPerChannel(); ++e) {
      auto r = int32_t(rawAcc[e]);
      auto val = *reinterpret_cast<float*>(&r) + 2.7F + 2.238F * float(e);
      if(raw) {
        v[e] = *reinterpret_cast<int32_t*>(&val);
      }
      else {
        v[e] = val;
      }
    }
    return {v};
  }

  template<typename UserType>
  std::vector<std::vector<UserType>> getRemoteValue([[maybe_unused]] bool raw = false) {
    std::vector<UserType> v(nElementsPerChannel());
    for(uint32_t e = 0; e < nElementsPerChannel(); ++e) {
      if(raw) {
        v[e] = rawAcc[e];
      }
      else {
        auto r = int32_t(rawAcc[e]);
        v[e] = *reinterpret_cast<float*>(&r);
      }
    }
    return {v};
  }

  void setRemoteValue() {
    auto v = generateValue<rawUserType>(true);
    for(uint32_t e = 0; e < nElementsPerChannel(); ++e) {
      rawAcc[e] = v[0][e];
    }
  }

  void setForceRuntimeError(bool enable, size_t) {
    exceptionDummy->throwExceptionRead = enable;
    exceptionDummy->throwExceptionWrite = enable;
    exceptionDummy->throwExceptionOpen = enable;
  }
};

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testRegisterAccessor) {
  std::cout << "*** testRegisterAccessor *** " << std::endl;
  ChimeraTK::UnifiedBackendTest<>()
      .addRegister<ScalarInt>()
      .addRegister<ArrayFloatMultiLine>()
      .addRegister<ArrayFloatSingleLine>()
      .addRegister<StringArray>()
      .runTests(cdd);
}

/**********************************************************************************************************************/

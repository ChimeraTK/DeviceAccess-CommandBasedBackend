// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE SerialPortTest
#include <boost/test/unit_test.hpp>
using namespace boost::unit_test_framework;

#include "DummyServer.h"
#include "SerialCommandHandler.h"

#include <string>

struct SerialCommandHandlerFixture {
  DummyServer dummy;
  SerialCommandHandler s;

  SerialCommandHandlerFixture() : s(dummy.deviceNode) {}
};

BOOST_FIXTURE_TEST_SUITE(SerialCommandHandlerTests, SerialCommandHandlerFixture)

BOOST_AUTO_TEST_CASE(testSimpleReply) {
  for(long l = 0; l < 10; l++) {
    std::string cmd = "reply" + std::to_string(l);
    s.write(cmd);
    std::string res = s.waitAndReadline();
    BOOST_TEST(res == cmd, "Failed simple reply test for iteration " << l);
  }
}

BOOST_AUTO_TEST_CASE(testBasicCommand) {
  std::string cmd = "test1";
  std::string res = s.sendCommand(cmd);
  BOOST_TEST(res == cmd, "Failed basic command test");
}

BOOST_AUTO_TEST_CASE(testLongCommandBuffer) {
  std::string cmd =
      "test2aaafirst100aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaabbbb2ndHund"
      "redbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbcccc201thru249cccccc"
      "ccccccccccccccccccccccccccccc012345";
  std::string res = s.sendCommand(cmd);
  BOOST_TEST(res == cmd, "Failed long command buffer test");
}

BOOST_AUTO_TEST_CASE(testLongCommandBufferFollowup) {
  std::string cmd = "test3";
  std::string res = s.sendCommand(cmd);
  BOOST_TEST(res == cmd, "Failed long command buffer followup test");
}

BOOST_AUTO_TEST_CASE(testDelimiterStraddleBuffer) {
  std::string cmd =
      "test4aaafirst100aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaabbbb2ndHund"
      "redbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbcccc201thru249cccccc"
      "ccccccccccccccccccccccccccccc01234";
  std::string res = s.sendCommand(cmd);
  BOOST_TEST(res == cmd, "Failed delimiter straddle buffer test");
}

BOOST_AUTO_TEST_CASE(testDelimiterStraddleBufferFollowup) {
  std::string cmd = "test5";
  std::string res = s.sendCommand(cmd);
  BOOST_TEST(res == cmd, "Failed delimiter straddle buffer followup test");
}

BOOST_AUTO_TEST_CASE(testCommandExactBufferSize) {
  std::string cmd =
      "test6aaafirst100aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaabbbb2ndHund"
      "redbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbcccc201thru249cccccc"
      "ccccccccccccccccccccccccccccc0123";
  std::string res = s.sendCommand(cmd);
  BOOST_TEST(res == cmd, "Failed exact buffer size command test");
}

BOOST_AUTO_TEST_CASE(testExactBufferSizeFollowup) {
  std::string cmd = "test7";
  std::string res = s.sendCommand(cmd);
  BOOST_TEST(res == cmd, "Failed exact buffer size followup test");
}

BOOST_AUTO_TEST_CASE(testSemicolonParsing) {
  std::string cmd = "qwer;asdf";
  auto ret = s.sendCommand(cmd, 2);
  BOOST_TEST(ret.size() == 2, "Semicolon parsing did not return 2 results");

  if(ret.size() == 2) {
    BOOST_TEST(ret[0] == "qwer", "First semicolon-separated result is incorrect");
    BOOST_TEST(ret[1] == "asdf", "Second semicolon-separated result is incorrect");
  }
}

BOOST_AUTO_TEST_SUITE_END()

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

// Declares and registers a fixture used by all test cases under a test suite.
// Each test case in the subtree of the test suite uses the fixture.
BOOST_FIXTURE_TEST_SUITE(SerialCommandHandlerTests, SerialCommandHandlerFixture)

BOOST_AUTO_TEST_CASE(testSimpleReply) {
  for(long l = 0; l < 10; l++) {
    std::string cmd = "reply" + std::to_string(l);
    s.write(cmd);
    std::string res = s.waitAndReadline();
    BOOST_TEST(res == cmd);
  }
}

BOOST_AUTO_TEST_CASE(testBasicCommand) {
  std::string cmd = "test1";
  std::string res = s.sendCommand(cmd);
  BOOST_TEST(res == cmd);
}

BOOST_AUTO_TEST_CASE(testLongCommandBuffer) {
  // Here the cmd string is longer than 256 character read buffer, forcing a multiple reads.
  // cmd is 255 char long, sending 257 chars
  std::string cmd =
      "test2aaafirst100aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaabbbb2ndHund"
      "redbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbcccc201thru249cccccc"
      "ccccccccccccccccccccccccccccc012345";
  std::string res = s.sendCommand(cmd);
  BOOST_TEST(res == cmd);
}

BOOST_AUTO_TEST_CASE(testLongCommandBufferFollowup) {
  // Intermediate basic tests to see if the responce from the previous test overflows into the next reply.
  std::string cmd = "test3";
  std::string res = s.sendCommand(cmd);
  BOOST_TEST(res == cmd);
}

BOOST_AUTO_TEST_CASE(testDelimiterStraddleBuffer) {
  // cmd is 254 char long, so this sends 256 chars. The read buffer is 256 characters, so the repply comes in over two
  // reads. res has 2-character delimiter straddle read buffer, so the first char of the delimiter comes into the buffer
  // upon the first read, and second delimter char comes into the buffer upon the second read
  std::string cmd =
      "test4aaafirst100aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaabbbb2ndHund"
      "redbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbcccc201thru249cccccc"
      "ccccccccccccccccccccccccccccc01234";
  std::string res = s.sendCommand(cmd);
  BOOST_TEST(res == cmd);
}

BOOST_AUTO_TEST_CASE(testDelimiterStraddleBufferFollowup) {
  std::string cmd = "test5";
  std::string res = s.sendCommand(cmd);
  BOOST_TEST(res == cmd);
}

BOOST_AUTO_TEST_CASE(testCommandExactBufferSize) {
  // cmd is 253 char long, sending 255 chars
  // So the command, together with delimiter, is exactly the size of the buffer.
  std::string cmd =
      "test6aaafirst100aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaabbbb2ndHund"
      "redbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbcccc201thru249cccccc"
      "ccccccccccccccccccccccccccccc0123";
  std::string res = s.sendCommand(cmd);
  BOOST_TEST(res == cmd);
}

BOOST_AUTO_TEST_CASE(testExactBufferSizeFollowup) {
  std::string cmd = "test7";
  std::string res = s.sendCommand(cmd);
  BOOST_TEST(res == cmd);
}

BOOST_AUTO_TEST_CASE(testSemicolonParsing) {
  std::string cmd = "qwer;asdf";
  auto ret = s.sendCommand(cmd, 2);
  BOOST_TEST(ret.size() == 2);

  if(ret.size() == 2) {
    BOOST_TEST(ret[0] == "qwer");
    BOOST_TEST(ret[1] == "asdf");
  }
}

BOOST_AUTO_TEST_SUITE_END()

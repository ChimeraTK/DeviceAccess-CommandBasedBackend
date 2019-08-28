#define BOOST_TEST_MODULE CommandBasedBackendTcpConnectionTest
#include <boost/test/included/unit_test.hpp>
using namespace boost::unit_test_framework;

//#include "CommandBasedHardwareBackend.h"

#include <ChimeraTK/Device.h>
#include <string>
#include <cstdlib>
#include <chrono>
#include <thread>
#include <iostream>

#include <boost/shared_ptr.hpp>
#include <boost/asio.hpp>

//TODO Decide how to do this when serial implementation exists,
//     can this be done once for all tests?
struct TestConfigurator{
  TestConfigurator(){
    std::system("./configureDMap.sh");
  }
};
static TestConfigurator testConfigurator;


BOOST_AUTO_TEST_SUITE( CommandBasedBackendTcpConnectionTestSuite )

inline void sleep_ms(unsigned t_ms){
  std::this_thread::sleep_for(std::chrono::milliseconds(t_ms));
}

struct Fixture{
  Fixture()
    : hostname{boost::asio::ip::host_name()}
  {
    ChimeraTK::setDMapFilePath("devices.dmap");

    std::system("./echo-server.py 10001 &");
    sleep_ms(3000);
  }

  ~Fixture(){
    if(dev.isOpened()){
      dev.close();
    }
    //TODO Only kill the instance we started
    std::system("killall echo-server.py");
  }

  ChimeraTK::Device dev;
  const std::string hostname;
};

BOOST_FIXTURE_TEST_CASE( testReadWrite, Fixture ){
  std::cout << "testTcpConnection/testReadWrite on  " << hostname << std::endl;

  dev.open("COMMAND_BASED_DUMMY_TCP");

  // For now (until we have read/write that work through the front-end) get ptr to backend instance
//  auto backendInst
//    = boost::dynamic_pointer_cast<CommandBasedHardwareBackend>(ChimeraTK::BackendFactory::getInstance().createBackend("COMMAND_BASED_DUMMY_TCP"));

  const std::string cmd{"Hello device!"};
  //backendInst->write(cmd);
  //dev.write<int>(cmd);

//  std::string resp = backendInst->read(1024U);
  //std::string resp = dev.read<unsigned>(1024U);
  //backendInst->read(32U);

  dev.close();
}

BOOST_AUTO_TEST_SUITE_END()

#define BOOST_TEST_MODULE CommandBasedHardwareBackendTest
#include <boost/test/included/unit_test.hpp>
using namespace boost::unit_test_framework;

#include <ChimeraTK/Device.h>
#include <ChimeraTK/Utilities.h>
using namespace ChimeraTK;

BOOST_AUTO_TEST_SUITE( CommandBasedHardwareBackendTestSuite )

// A fixture with a device. Always opens a fresh device. 
struct F{
  F(){
    // using dmap file automatically tests the plugin mechanism
    setDMapFilePath("devices.dmap");
    d.open("COMMAND_BASED_DUMMY");
  }
  
  Device d;
};

BOOST_FIXTURE_TEST_CASE( testOpenClose, F ){
  BOOST_CHECK(d.isOpened());
  d.close();
  BOOST_CHECK(d.isOpened()==false);
  d.open();
  BOOST_CHECK(d.isOpened());
}

// test the write and read setpoint functions and the temperature readout
BOOST_FIXTURE_TEST_CASE( testSetpointTemperature, F ){
  d.write<int>("ENABLE_CONTROLLER",0); // turn off the controller
  d.write<int>("SETPOINT",12000);
  BOOST_CHECK(d.read<int>("SETPOINT") == 12000);
  // a trick of the dummy: the temperature is always one degree higher
  // if the controller is off
  BOOST_CHECK(d.read<int>("TEMPERATURE") == 13000);

  // Set something different to make sure that setting really changes something
  // and the value was not accidentally in the register while writing does not
  // work.
  d.write<int>("SETPOINT",15000);
  BOOST_CHECK(d.read<int>("SETPOINT") == 15000);
  BOOST_CHECK(d.read<int>("TEMPERATURE") == 16000);

  // writing the temperature does nothing (but does not crash e.g.)
  d.write<int>("TEMPERATURE",27000);
  BOOST_CHECK(d.read<int>("SETPOINT") == 15000);
  BOOST_CHECK(d.read<int>("TEMPERATURE") == 16000);  
}

BOOST_FIXTURE_TEST_CASE( testEnableDisable, F ){
  d.write<int>("ENABLE_CONTROLLER",1);
  BOOST_CHECK(d.read<int>("ENABLE_CONTROLLER") == 1);
  
  auto setpoint=d.read<int>("SETPOINT");
  // if the controller is enabled, the temperature is only 1 millidegree
  // above the setpoint
  // Like this we can observe that the change arrived on the hardware
  BOOST_CHECK(d.read<int>("TEMPERATURE") == setpoint+1);

  d.write<int>("ENABLE_CONTROLLER",0);
  BOOST_CHECK(d.read<int>("ENABLE_CONTROLLER") == 0);

  // when the controller is off the temperature is 1000 mk above the setpoint
  BOOST_CHECK(d.read<int>("TEMPERATURE") == setpoint+1000);
}

BOOST_AUTO_TEST_SUITE_END()

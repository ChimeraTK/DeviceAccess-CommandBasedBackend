#include <mtca4u/DeviceBackendImpl.h>
#include <mtca4u/BackendFactory.h>
#include <mtca4u/DeviceAccessVersion.h>
#include <mtca4u/NotImplementedException.h>
#include <boost/make_shared.hpp>
#include <boost/thread.hpp>
#include <boost/chrono.hpp>
#include <fstream>
#include <sstream>

class CommandBasedHardwareBackend : public mtca4u::DeviceBackendImpl{
public:
  CommandBasedHardwareBackend(std::string serialDeviceName) : _serialDeviceName(serialDeviceName){
  }
  
  void open() override{
    throw mtca4u::NotImplementedException("CommandBasedHardwareBackend::open() not implemented yet.");
  }

  void close() override{
    throw mtca4u::NotImplementedException("CommandBasedHardwareBackend::close() not implemented yet.");
  }

  std::string readDeviceInfo() override{
    std::stringstream deviceInfo;
    deviceInfo << "CommandBasedHardwareBackend connected to \'" << _serialDeviceName
	       << "\', device is " << (_opened?"opened":"closed");
    return deviceInfo.str();
  }
    
  static boost::shared_ptr<mtca4u::DeviceBackend> createInstance(std::string /*host*/, std::string /*instance*/, std::list<std::string> parameters, std::string /*mapFileName*/){
    return boost::make_shared<CommandBasedHardwareBackend>(parameters.front());
  }

  struct BackendRegisterer{
    BackendRegisterer(){
      mtca4u::BackendFactory::getInstance().registerBackendType("CommandBasedHW","",&CommandBasedHardwareBackend::createInstance, CHIMERATK_DEVICEACCESS_VERSION);
    }
  };

protected:
  std::string _serialDeviceName;
  std::fstream _fileStream;
};

static CommandBasedHardwareBackend::BackendRegisterer gCommandBasedHardwareBackend;

extern "C"{
  std::string versionUsedToCompile(){
    return CHIMERATK_DEVICEACCESS_VERSION;
  }
}

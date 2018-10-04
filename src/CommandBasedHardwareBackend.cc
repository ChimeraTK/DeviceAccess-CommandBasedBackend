#include <ChimeraTK/DeviceBackendImpl.h>
#include <ChimeraTK/BackendFactory.h>
#include <ChimeraTK/DeviceAccessVersion.h>
#include <ChimeraTK/Exception.h>
#include <boost/make_shared.hpp>
#include <boost/thread.hpp>
#include <boost/chrono.hpp>
#include <fstream>
#include <sstream>

class CommandBasedHardwareBackend : public ChimeraTK::DeviceBackendImpl{
public:
  CommandBasedHardwareBackend(std::string serialDeviceName) : _serialDeviceName(serialDeviceName){
  }
  
  void open() override{
    _fileStream.open(_serialDeviceName);
    //empty input buffer
    std::string inputToken;
    do{
      _fileStream >> inputToken;
    }while(_fileStream.good());
    _fileStream << "\r\n";
    do{
      _fileStream >> inputToken;
      boost::this_thread::sleep_for(boost::chrono::microseconds(10));
    }while(inputToken != ">>");
    _opened = true;
  }

  void close() override{
    throw ChimeraTK::logic_error("CommandBasedHardwareBackend::close() not implemented yet.");
  }

  std::string readDeviceInfo() override{
    std::stringstream deviceInfo;
    deviceInfo << "CommandBasedHardwareBackend connected to \'" << _serialDeviceName
	       << "\', device is " << (_opened?"opened":"closed");
    return deviceInfo.str();
  }
    
  static boost::shared_ptr<ChimeraTK::DeviceBackend> createInstance(std::string /*host*/, std::string /*instance*/, std::list<std::string> parameters, std::string /*mapFileName*/){
    return boost::make_shared<CommandBasedHardwareBackend>(parameters.front());
  }

  struct BackendRegisterer{
    BackendRegisterer(){
      ChimeraTK::BackendFactory::getInstance().registerBackendType("CommandBasedHW","",&CommandBasedHardwareBackend::createInstance, CHIMERATK_DEVICEACCESS_VERSION);
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

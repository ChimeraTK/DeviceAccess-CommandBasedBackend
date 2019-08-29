
#include "CommandBasedHardwareBackend.h"
#include "CommandBasedRegisterAccessor.h"

#include <ChimeraTK/DeviceAccessVersion.h>
#include <ChimeraTK/Exception.h>
#include <boost/make_shared.hpp>

  CommandBasedHardwareBackend::CommandBasedHardwareBackend(std::string address, std::map<std::string,std::string> parameters)
    : _connection{}
  {
    if(parameters["type"].compare("serial") == 0){
      _connection = std::make_unique<CommandBasedBackend::SerialConnection>(address);
    }
    else if(parameters["type"].compare("tcp") == 0){
      _connection = std::make_unique<CommandBasedBackend::TcpConnection>(address, std::stoi(parameters["port"]));
    }
  }
  
  void CommandBasedHardwareBackend::open(){
    _connection->openConnection();
    _opened = true;
  }

  void CommandBasedHardwareBackend::close(){
    _connection->closeConnection();
    _opened = false;
  }

  std::string CommandBasedHardwareBackend::readDeviceInfo(){
    std::stringstream deviceInfo;
    deviceInfo << "CommandBasedHardwareBackend connected to \'" << _connection->getInfo()
	       << "\', device is " << (_opened?"opened":"closed");
    return deviceInfo.str();
  }


  // Temporary implementations of read/write for evaluation -> directly enter the command
  void CommandBasedHardwareBackend::write(const std::string& command){
    //TODO Termination characters
    _connection->send({command.begin(), command.end()});
  }

  std::string CommandBasedHardwareBackend::read(const uint32_t nBytes){
    std::vector<char> data = _connection->recv(nBytes);
    return {data.begin(), data.end()};
  }


  boost::shared_ptr<ChimeraTK::DeviceBackend> CommandBasedHardwareBackend::createInstance(std::string address, std::map<std::string,std::string> parameters){

    if(parameters["type"].empty()){
      throw ChimeraTK::logic_error("CommandBasedBackend: No connection type (serial/tcp) specified.");
    }
    // TODO Check if type is valid, use enum

    if(parameters["port"].empty()){
      parameters["port"] = std::to_string(10000);
    }

    return boost::make_shared<CommandBasedHardwareBackend>(address, parameters);
  }




static CommandBasedHardwareBackend::BackendRegisterer gCommandBasedHardwareBackend;

extern "C"{
  std::string versionUsedToCompile(){
    return CHIMERATK_DEVICEACCESS_VERSION;
  }
}

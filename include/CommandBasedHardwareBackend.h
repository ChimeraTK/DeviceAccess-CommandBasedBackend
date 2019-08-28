#ifndef COMMANDBASEDHARDWAREBACKEND_H
#define COMMANDBASEDHARDWAREBACKEND_H

#include "Protocol.h"


#include <ChimeraTK/DeviceBackendImpl.h>
#include <ChimeraTK/BackendFactory.h>

#include <memory>


class CommandBasedHardwareBackend : public ChimeraTK::DeviceBackendImpl{
public:
  CommandBasedHardwareBackend(std::string address, std::map<std::string,std::string> parameters);

  void open() override;
  void close() override;

  std::string readDeviceInfo() override;

  // Temporary implementations of read/write for evaluation -> directly enter the command
  void write(const std::string& command);
  std::string read(const uint32_t nBytes);


  static boost::shared_ptr<ChimeraTK::DeviceBackend> createInstance(std::string address, std::map<std::string,std::string> parameters);

  struct BackendRegisterer{
    BackendRegisterer(){
      ChimeraTK::BackendFactory::getInstance().registerBackendType("CommandBasedHW", &CommandBasedHardwareBackend::createInstance, {"type", "port"});
    }
  };

protected:
  std::unique_ptr<CommandBasedBackend::Protocol> _connection;
  const std::string msgDelimiter{"\n"}; //TODO Make configurable
};
#endif // COMMANDBASEDHARDWAREBACKEND_H

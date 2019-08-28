#ifndef PROTOCOL_H
#define PROTOCOL_H

#include "TcpCtrl.h"

#include <vector>
#include <string>
#include <fstream>


namespace CommandBasedBackend{
  /**
    * Base class to abstract the actual communication protocol
    */
  class Protocol{
  public:
    Protocol() = default;
    virtual ~Protocol(){}

    virtual void openConnection() = 0;
    virtual void closeConnection() = 0;
    virtual void send(const std::vector<char>& data) = 0;
    virtual std::vector<char> recv(const uint32_t nBytes) = 0;
    virtual std::string getInfo() = 0;
  };

  /// TCP connection
  // TODO For now use TcpCtrl of rebot backend, i.e.
  // this is mostly a wrapper. see if this fits
  class TcpConnection : public Protocol {
  public:
    TcpConnection(std::string ipaddr, int port);
    virtual ~TcpConnection(){};

    virtual void openConnection();
    virtual void closeConnection();
    virtual void send(const std::vector<char>& data);
    virtual std::vector<char> recv(const std::string& msgDelimiter);
    virtual std::vector<char> recv(const uint32_t nBytes); // TODO Needed for some protocols?
    virtual std::string getInfo();

  private:
    TcpCtrl _connection;
  };

  /// Serial connection
  class SerialConnection : public Protocol {
  public:
    SerialConnection(std::string deviceName);
    virtual ~SerialConnection(){};

    virtual void openConnection();
    virtual void closeConnection();
    virtual void send(const std::vector<char>& data);
    virtual std::vector<char> recv(const uint32_t nBytes);
    virtual std::string getInfo();

  private:
    std::string _serialDeviceName;
    std::fstream _fileStream;
  };

} // namespace CommandBasedBackend

#endif // PROTOCOL_H

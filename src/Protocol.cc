
#include "Protocol.h"

#include <thread>
#include <chrono>
#include <iostream>
#include <cstring>


namespace CommandBasedBackend{

  TcpConnection::TcpConnection(std::string ipaddr, int port)
    : Protocol{},
      _connection{ipaddr, port}{}

  void TcpConnection::openConnection(){
    _connection.openConnection();
    std::vector<char> ack{'A', 'C', 'K', 'N', 'O', '\n'};
    _connection.sendData(ack);
  }

  void TcpConnection::closeConnection(){
//    auto resp = recv(4U);
    auto resp = recv("\n");

    std::string respStr{resp.begin(), resp.end()};
    std::cout << "Close: Received: " << respStr << std::endl;

    _connection.closeConnection();
  }

  void TcpConnection::send(const std::vector<char>& data){
    _connection.sendData(data);
  }

  std::vector<char> TcpConnection::recv(const std::string& msgDelimiter){
    return _connection.receiveData(msgDelimiter);
  }

  std::vector<char> TcpConnection::recv(const uint32_t nBytes){
    const uint32_t wordsToRead = (nBytes + sizeof(int32_t) - 1)/sizeof(int32_t);
    std::vector<int32_t> recvWords = _connection.receiveData(wordsToRead);

    // TODO Need a method which returns vector char directly and reads all available chars
    std::vector<char> recvBytes(nBytes);
    std::memcpy(recvBytes.data(), recvWords.data(), nBytes);

    return recvBytes;
  }

  std::string TcpConnection::getInfo(){
    return _connection.getAddress() + ":" + std::to_string(_connection.getPort());
  }

  ////////////// SerialConnection /////////////
  // TODO Not tested, moved from original implementation in CommandBasedHardwareBackend
  SerialConnection::SerialConnection(std::string deviceName)
    : Protocol{},
      _serialDeviceName{deviceName}{}

  void SerialConnection::openConnection(){
    _fileStream.open(_serialDeviceName);
    //empty input buffer
    std::string inputToken;
    do{
      _fileStream >> inputToken;
    }while(_fileStream.good());
    _fileStream << "\r\n";
    do{
      _fileStream >> inputToken;
      std::this_thread::sleep_for(std::chrono::microseconds(10));
    }while(inputToken != ">>");
  }

  void SerialConnection::closeConnection(){
    _fileStream.close();
  }

  void SerialConnection::send(const std::vector<char>& data){/*TODO*/}
  std::vector<char> SerialConnection::recv(const uint32_t nBytes){/*TODO*/return std::vector<char>();}

  std::string SerialConnection::getInfo(){
    return _serialDeviceName;
  }

} //namespace CommandBasedBackend

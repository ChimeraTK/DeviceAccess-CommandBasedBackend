// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once
#include <boost/asio.hpp>

#include <iostream>
#include <string>

namespace ChimeraTK {

  class TcpSocket {
   public:
    TcpSocket(const std::string& host, const std::string& port, ulong timeoutInMilliseconds);
    void send(const std::string& command);
    std::string readResponse();
    void connect();
    void disconnect();
    ~TcpSocket();

   private:
    std::string readWithTimeout(std::chrono::milliseconds timeout);
    boost::asio::io_context _io_context;
    boost::asio::ip::tcp::socket _socket;
    boost::asio::ip::tcp::resolver _resolver;
    std::string _host;
    std::string _port;
    std::chrono::milliseconds _timeout;
    bool _opened = false;

    std::string _delimiter{"\n"}; // FIXME: set from constructor parameter
  };

} // namespace ChimeraTK

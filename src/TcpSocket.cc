// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "TcpSocket.h"

#include <ChimeraTK/Exception.h>

#include <utility>

namespace ChimeraTK {
  TcpSocket::TcpSocket(std::string host, std::string port, ulong timeoutInMilliseconds)
  : _socket(_io_context), _resolver(_io_context), _host(std::move(host)), _port(std::move(port)),
    _timeout(std::chrono::milliseconds(timeoutInMilliseconds)) {}

  /********************************************************************************************************************/

  void TcpSocket::connect() {
    // Resolve the host and port, and connect to the server.
    boost::system::error_code ec;
    try {
      auto endpoints = _resolver.resolve(_host, _port);
      boost::asio::connect(_socket, endpoints);
    }
    catch(std::exception& e) {
      throw ChimeraTK::runtime_error(e.what());
    }
    if(ec) {
      throw ChimeraTK::runtime_error("Connection failed");
    }
    _opened = true;
  }

  /********************************************************************************************************************/

  void TcpSocket::disconnect() {
    boost::system::error_code ec;
    try {
      _socket.cancel(ec);
    }
    catch(std::exception& e) {
      throw ChimeraTK::runtime_error(e.what());
    }
    if(ec) {
      throw ChimeraTK::runtime_error("Error when disconnecting");
    }
    _opened = false;
  }

  /********************************************************************************************************************/

  TcpSocket::~TcpSocket() {
    try {
      disconnect();
    }
    catch(...) {
      // If the destructor throws we are beyond saving the application.
      // We catch here and terminate to make the linter happy.
      std::terminate();
    }
  }

  /********************************************************************************************************************/

  void TcpSocket::send(const std::string& command) {
    assert(_opened);

    boost::system::error_code ec;
    try {
      boost::asio::write(_socket, boost::asio::buffer(command + _delimiter));
    }
    catch(std::exception& e) {
      throw ChimeraTK::runtime_error(e.what());
    }
    if(ec) {
      throw ChimeraTK::runtime_error("Error sending");
    }
  }

  /********************************************************************************************************************/

  std::string TcpSocket::readResponse() {
    assert(_opened);

    try {
      return readWithTimeout(_timeout);
    }
    catch(std::exception& ex) {
      throw ChimeraTK::runtime_error(ex.what());
    }
  }

  /********************************************************************************************************************/

  std::string TcpSocket::readWithTimeout(std::chrono::milliseconds timeout) {
    boost::asio::steady_timer timer(_io_context);
    boost::system::error_code ec;
    std::string response;

    timer.expires_after(timeout);
    timer.async_wait([&](const boost::system::error_code& error) {
      // As boost::asio::error::operation_aborted is not set for timer, it
      // means it has expired.
      if(!error) {
        _socket.cancel(); // Cancel the socket operation on timeout.
      }
    });

    boost::asio::async_read_until(_socket, boost::asio::dynamic_buffer(response), _delimiter,
        [&](const boost::system::error_code& error, std::size_t /*bytes_transferred*/) {
          ec = error;
          timer.cancel(); // Stop the timer if read completes.
                          // If the timer is canceled: error is boost::asio::error::operation_aborted.
        });

    _io_context.run();
    _io_context.reset();

    if(ec == boost::asio::error::operation_aborted) {
      throw ChimeraTK::runtime_error("Read operation timed out");
    }
    if(ec) {
      throw ChimeraTK::runtime_error(ec.message());
    }

    // Return the response without the delimiter.
    return response.substr(0, response.size() - _delimiter.size());
  }

  /********************************************************************************************************************/
} // namespace ChimeraTK

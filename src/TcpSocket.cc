// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "TcpSocket.h"

#include <ChimeraTK/Exception.h>

#include <optional>
#include <utility>

namespace ChimeraTK {
  TcpSocket::TcpSocket(std::string host, std::string port)
  : _socket(_io_context), _resolver(_io_context), _host(std::move(host)), _port(std::move(port)) {}

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
      boost::asio::write(_socket, boost::asio::buffer(command));
    }
    catch(std::exception& e) {
      throw ChimeraTK::runtime_error(e.what());
    }
    if(ec) {
      throw ChimeraTK::runtime_error("Error sending");
    }
  }

  /********************************************************************************************************************/

  std::string TcpSocket::readlineWithTimeout(const std::chrono::milliseconds& timeout, const std::string& delimiter) {
    AsyncReadFn asyncReadFn = [delimiter](auto& stream, auto& buffer, auto doOnReadFinish) {
      boost::asio::async_read_until(stream, buffer, delimiter, doOnReadFinish);
    };
    return readWithTimeout(timeout, asyncReadFn);
  }

  /********************************************************************************************************************/

  std::string TcpSocket::readBytesWithTimeout(size_t nBytesToRead, const std::chrono::milliseconds& timeout) {
    if(nBytesToRead == 0) {
      return "";
    }
    AsyncReadFn asyncReadFn = [nBytesToRead](auto& stream, auto& buffer, auto doOnReadFinish) {
      boost::asio::async_read(stream, buffer, boost::asio::transfer_exactly(nBytesToRead), doOnReadFinish);
    };
    return readWithTimeout(timeout, asyncReadFn);
  }

  /********************************************************************************************************************/

  constexpr boost::system::error_code timeoutError() noexcept {
    // Rename 'operation_aborted' to 'timeoutError' for readability.
    return boost::asio::error::operation_aborted;
  }

  /********************************************************************************************************************/

  std::string TcpSocket::readWithTimeout(const std::chrono::milliseconds& timeout, const AsyncReadFn& asyncReadFn) {
    assert(_opened);
    /*----------------------------------------------------------------------------------------------------------------*/
    // Set a timer, with doOnTimeout executing when it expires.
    boost::asio::steady_timer timer(_io_context);
    timer.expires_after(timeout);
    bool readCompleted = false;
    auto doOnTimeout = [&](const boost::system::error_code& error) {
      if(not(readCompleted or error)) {
        _socket.cancel(); // Causes error = errorCode = timeoutError()
      }
    };
    timer.async_wait(doOnTimeout);
    /*----------------------------------------------------------------------------------------------------------------*/
    // Do read
    boost::asio::streambuf buffer;
    boost::system::error_code errorCode;

    /* doOnReadFinish is the callback handler, executing when the read operation ends, successfully or not.
     * If there's a timeout, doOnTimeout is called before this, with _socket.cancel() causing error = timeoutError()
     */
    auto doOnReadFinish = [&](const boost::system::error_code& error, std::size_t /*bytesTransferred*/) {
      readCompleted = true;
      timer.cancel();
      errorCode = error;
    };

    asyncReadFn(_socket, buffer, doOnReadFinish);
    _io_context.run();
    /*----------------------------------------------------------------------------------------------------------------*/
    // Clean-up
    _io_context.reset();
    /*----------------------------------------------------------------------------------------------------------------*/
    if(errorCode) {
      if(errorCode == timeoutError()) {
        throw ChimeraTK::runtime_error("Readline operation timed out");
      }
      throw ChimeraTK::runtime_error(errorCode.message());
    }
    /*----------------------------------------------------------------------------------------------------------------*/
    // Extract the data from the streambuf
    return std::string{std::istreambuf_iterator<char>(&buffer), {}};
  }

  /********************************************************************************************************************/
} // namespace ChimeraTK

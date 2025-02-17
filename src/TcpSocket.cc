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
    std::lock_guard<std::mutex> socketLock(_socketMutex);
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
    _opened.store(true);
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
    _opened.store(false);
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

  void TcpSocket::send(const std::string& command) const {
    if(not _opened.load()) {
      throw ChimeraTK::runtime_error("Attempting to send from a closed TcpSocket");
    }

    boost::system::error_code ec;
    std::lock_guard<std::mutex> socketLock(_socketMutex);
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

  void TcpSocket::waitForIoContextToStop() {
    std::unique_lock<std::mutex> lock(ioContextMutex);
    ioContextStoppedCondition.wait(lock, [this] { return _io_context.stopped(); });
  }

  /********************************************************************************************************************/

  void TcpSocket::runAndResetIoContext() {
    // The executor_work_guard prevents io_context from stopping prematurely while ensuring run() exits once this
    // function completes. Since io_context is shared across multiple calls, run() may not return if other async
    // operations are pending. Conversely, reset() must only be called when io_context has stopped, or it risks
    // interfering with ongoing tasks.
    boost::asio::executor_work_guard<boost::asio::io_context::executor_type> ioContextLock(_io_context.get_executor());
    _io_context.run(); // asynchronous
    // reset can only be called when io_context has stopped, or it risks interfering with ongoing tasks.
    waitForIoContextToStop();
    _io_context.reset();
  }

  /********************************************************************************************************************/

  std::string TcpSocket::readlineWithTimeout(const std::chrono::milliseconds& timeout, const std::string& delimiter) {
    if(not _opened.load()) {
      throw ChimeraTK::runtime_error("Attempting to readline from a closed TcpSocket");
    }

    try {
      // Use shared_ptr's to avoid problems if lambdas outlive the this reader.
      auto spTimer = std::make_shared<boost::asio::steady_timer>(_io_context);
      std::shared_ptr<std::optional<boost::system::error_code>> spErrorCode =
          std::make_shared<std::optional<boost::system::error_code>>(std::nullopt);
      std::shared_ptr<std::string> spResponse = std::make_shared<std::string>(nBytesToRead, '\0');

      spTimer->expires_after(timeout);
      spTimer->async_wait([spErrorCode, this](const boost::system::error_code& error) {
        // This gets called when the timer expires.
        // We then make sure we don't call cancel twice or prematurely.
        // "not spErrorCode.has_value()" avoids a race condition between the timer and read operation.
        std::lock_guard<std::mutex> socketLock(_socketMutex);
        if((not error) and (not spErrorCode->has_value()) and (_socket.is_open())) {
          _socket.cancel(); // Cancel the socket operation on timeout.
        }
      });

      std::unique_lock<std::mutex> socketLock(_socketMutex);
      if(not _socket.is_open()) {
        throw ChimeraTK::runtime_error("Socket is closed during read operation");
      }

      boost::asio::async_read_until(_socket, boost::asio::dynamic_buffer(*spResponse), delimiter,
          [spErrorCode, spTimer](const boost::system::error_code& error, std::size_t /*bytes_transferred*/) {
            *spErrorCode = error;
            spTimer->cancel(); // Stop the timer if read completes.
                               // If the timer is canceled: error is boost::asio::error::operation_aborted.
          });
      socketLock.unlock();

      runAndResetIoContext();

      if(spErrorCode->has_value()) {
        if(spErrorCode->value() == boost::asio::error::operation_aborted) {
          throw ChimeraTK::runtime_error("Readline operation timed out");
        }
        throw ChimeraTK::runtime_error(spErrorCode->value().message());
      }

      // Return the spResponse without the delimiter.
      return spResponse.substr(0, spResponse.size() - delimiter.size());
    }
    catch(const boost::system::system_error& ex) {
      throw ChimeraTK::runtime_error(static_cast<std::string>("Boost.Asio error: ") + ex.what());
    }
    catch(std::exception& ex) {
      throw ChimeraTK::runtime_error(ex.what());
    }
  }

  /********************************************************************************************************************/





  /********************************************************************************************************************/
} // namespace ChimeraTK

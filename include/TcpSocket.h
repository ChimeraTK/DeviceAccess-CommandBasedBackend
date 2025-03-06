// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once
#include <condition_variable>

#include <boost/asio.hpp>

#include <iostream>
#include <mutex>
#include <string>

namespace ChimeraTK {

  using AsyncReadFn = std::function<void(boost::asio::ip::tcp::socket&, boost::asio::streambuf&,
      std::function<void(const boost::system::error_code&, std::size_t)>)>;

  constexpr const char TCP_DEFAULT_DELIMITER[] = "\r\n";
  /**
   * @class TcpSocket
   * @brief A TCP socket wrapper for communication with a specified host and port.
   *
   * This class provides functionality for establishing a TCP connection, sending commands,
   * and reading responses with support for timeout and delimiter-based communication.
   */
  class TcpSocket {
   public:
    /**
     * @brief Constructor to initialize the TCP socket.
     *
     * @param[in] host The remote host address to connect to.
     * @param[in] port The remote port to connect to.
     * @param[in] timeoutInMilliseconds Timeout duration for read/write operations, in milliseconds.
     */
    TcpSocket(std::string host, std::string port);

    /**
     * @brief Sends a command to the connected remote host.
     * No delimiter is added internally, and must part of the provided command
     * Send cannot be made a const function because of asio.
     * @param[in] command The string command to send.
     * @throws ChimeraTK::runtime_error If the socket is not connected or the send operation fails.
     */
    void send(const std::string& command);

    /**
     * @brief Reads a response from the remote host.
     *
     * Reads data from the socket until the configured delimiter is encountered.
     * @param[in] timeout the timeout in milliseconds
     * @param[in] delimiter The line delimiter string.
     * @return The response as a string.
     * @throws ChimeraTK::runtime_error If the socket is not connected or the read operation fails.
     */
    std::string readlineWithTimeout(
        const std::chrono::milliseconds& timeout, const std::string& delimiter = TCP_DEFAULT_DELIMITER);

    /**
     * @brief Read a the specified number of bytes from the remote host, return-formatted as a string that will not be
     * null-terminated.
     * @param[in] nBytesToRead The number of bytes that it will attempt to read.
     * @param[in] timeout the timeout in milliseconds
     * @return The response as a string.
     * @throws ChimeraTK::runtime_error if timeout exceeded.     */
    std::string readBytesWithTimeout(const size_t nBytesToRead, const std::chrono::milliseconds& timeout);

    /**
     * @brief Establishes a connection to the specified host and port.
     *
     * Resolves the hostname, establishes a TCP connection, and opens the socket.
     * @throws ChimeraTK::runtime_error If the connection cannot be established.
     */
    void connect();

    /**
     * @brief Disconnects the socket.
     *
     * Closes the connection if it is open.
     * @throws ChimeraTK::runtime_error if there error on disconnet.
     */
    void disconnect();

    /**
     * @brief Destructor for the TcpSocket.
     *
     * Ensures the socket is properly closed if still open.
     */
    ~TcpSocket();

   private:
    boost::asio::io_context _io_context;                //!< Boost.Asio I/O context for asynchronous operations.
    std::condition_variable _ioContextStoppedCondition; //!< For synchronization management of io_context state

    boost::asio::ip::tcp::socket _socket; //!< TCP socket used for communication.

    boost::asio::ip::tcp::resolver _resolver; //!< Resolver for hostname and port resolution.
    std::string _host;                        //!< Hostname or IP address of the remote host.
    std::string _port;                        //!< Port number of the remote host.
    std::atomic<bool> _opened{false};         //!< Indicates whether the socket is currently open.

    /**
     * @brief A common underlying read function used by readBytesWithTimeout and readlineWithTimeout.
     * @param[in] timeout the timeout in milliseconds
     * @param[in] asyncReadFn A lambda wrapping the async read function to be used.
     * @throws ChimeraTK::runtime_error if timeout exceeded.
     */
    std::string readWithTimeout(const std::chrono::milliseconds& timeout, AsyncReadFn asyncReadFn);
  };

} // namespace ChimeraTK

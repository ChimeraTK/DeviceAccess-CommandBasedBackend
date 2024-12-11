// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once
#include <boost/asio.hpp>

#include <iostream>
#include <string>

namespace ChimeraTK {
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
    TcpSocket(std::string host, std::string port, ulong timeoutInMilliseconds);

    /**
     * @brief Sends a command to the connected remote host.
     *
     * @param[in] command The string command to send.
     * @throws ChimeraTK::runtime_error If the socket is not connected or the send operation fails.
     */
    void send(const std::string& command);

    /**
     * @brief Reads a response from the remote host.
     *
     * Reads data from the socket until the configured delimiter is encountered.
     * @return The response as a string.
     * @throws ChimeraTK::runtime_error If the socket is not connected or the read operation fails.
     */
    std::string readResponse();

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
    /**
     * @brief Reads data from the socket with a timeout.
     *
     * Attempts to read data until the configured delimiter is encountered,
     * or the operation times out.
     * @param[in] timeout The timeout duration for the read operation.
     * @return The data read as a string.
     * @throws ChimeraTK::runtime_error If the read operation fails or times out.
     */
    std::string readWithTimeout(std::chrono::milliseconds timeout);
    boost::asio::io_context _io_context;      //!< Boost.Asio I/O context for asynchronous operations.
    boost::asio::ip::tcp::socket _socket;     //!< TCP socket used for communication.
    boost::asio::ip::tcp::resolver _resolver; //!< Resolver for hostname and port resolution.
    std::string _host;                        //!< Hostname or IP address of the remote host.
    std::string _port;                        //!< Port number of the remote host.
    std::chrono::milliseconds _timeout;       //!< Timeout duration for read/write operations.
    bool _opened = false;                     //!< Indicates whether the socket is currently open.
    std::string _delimiter{"\n"}; //!< Delimiter used for reading responses (FIXME: make configurable): set from
                                  //!< constructor parameter. Ticket 13532
  };

} // namespace ChimeraTK

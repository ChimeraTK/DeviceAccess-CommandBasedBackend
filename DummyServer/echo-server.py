#!/usr/bin/python3

import socket
import sys

class EchoServer:

  def __init__(self, port):
    self.port = port

  def run(self):
    socketInstance = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

    print("EchoServer: Starting listening on port: " + str(self.port))
    socketInstance.bind((socket.gethostname(), int(self.port)))
    socketInstance.listen(1)

    while True:
      connection, client_addr = socketInstance.accept()
      print("EchoServer: Connection established by " + str(client_addr))

      try:
        while True:

          data = connection.recv(2048)

          if data == b'':
            raise RuntimeError("EchoServer: Connection closed by client.")
          else:
            print("  EchoServer: Received " + data.decode("utf-8"))

          bytesSentTotal = 0
          while bytesSentTotal < len(data):
            bytesSent = connection.send(data)

            if bytesSent == 0:
              raise RuntimeError("EchoServer: Connection broken.")
            bytesSentTotal += bytesSent
             
          
      finally:
        socketInstance.close()
        


if __name__ == "__main__":
  if len(sys.argv) != 2:
    print("Usage: " + sys.argv[0] + " <port>")
    sys.exit(-1)
  server = EchoServer(sys.argv[1])
  server.run()

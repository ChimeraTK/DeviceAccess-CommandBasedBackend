#!/usr/bin/python

import sys
import serial, time, io
from random import randint

class SerialServer:
  def __init__(self, port):
    print('port is ' + port)
    self.serialport = serial.Serial(port, 115200, timeout=0.1, rtscts=True, dsrdtr=True)
    #empty the input buffer on start
    self.serialport.flushInput()
    #initialise the state variables of the controller
    self.setpoint = 25000;
    self.temperature = 26000;
    self.controller_enabled = False;

  def interpret_and_act(self, command):
    if command == 'help':
      self.serialport.write('write_setpoint <mDegC>\r\n')
      self.serialport.write('read_setpoint\r\n')
      self.serialport.write('read_temperature\r\n')
      self.serialport.write('enable_controller\r\n')
      self.serialport.write('disable_controller\r\n')
      self.serialport.write('read_controller_status\r\n')
    elif command == 'read_setpoint':
      self.serialport.write(str(self.setpoint))
    elif command == 'read_temperature':
      self.serialport.write(str(self.temperature))
    elif command == 'enable_controller':
      self.controller_enabled = True
    elif command == 'disable_controller':
      self.controller_enabled = False
    elif command == 'read_controller_status':
      self.read_controller_status()
    elif command[:14] == 'write_setpoint':
      self.write_setpoint(command)
    elif command == '':
      return()
    else:
      self.serialport.write('unknown command')
      
  def read_controller_status(self):
    if self.controller_enabled:
      self.serialport.write('On')
    else:
      self.serialport.write('Off')

  def write_setpoint(self, command):
      params = command.split()
      if len(params) != 2:
        self.serialport.write('invalid parameters')
        return()
      self.setpoint = int(params[1])
      self.temperature = self.setpoint + 1000
      
  def run(self):
    command = ""
  
    #loop until a command is complete, then act
    while 1:
      #use the io reader, which expects \r instead of \n (can be used with putty)
      #store in a temporary sting, it might have timed out
      #use readline like read. advantage: readline gives arbitrary number of bytes
      input_buffer = self.serialport.readline()
      #print 'input_bufffer is '+ input_buffer
      #echo the buffer back to the sender (remote echo)
      #(just strip \r and \n, not spaces. we want spaces echoed)
      self.serialport.write( input_buffer.strip('\r').strip('\n') )
      command += input_buffer
  
      # check the last character
      if input_buffer[-1:] == '\r' or input_buffer[-1:] == '\n':
        print 'complete command is '+ command.strip()
        #complete the echo with replying \r\n to the sender, but only if
        #the command was not empty to avoid an empty line
        if (command.strip() != ''):
          self.serialport.write('\r\n')
        
        self.interpret_and_act(command.strip())
        #finish by sending CR LF and a new prompt
        self.serialport.write('\r\n>>')
        print "send new prompt >>"
        command = ""
  
if __name__ == '__main__':
  if len(sys.argv) != 2:
    print "usage: " + sys.argv[0] + " <serial port>"
    print "example: " + sys.argv[0] + " /dev/pts/18"
    sys.exit(-1)
  
  s = SerialServer( sys.argv[1] )
  s.run()

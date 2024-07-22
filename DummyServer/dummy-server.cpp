#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <iostream>
#include <cstring>

//First start:
//socat -d -d pty,raw,echo=0,link=/tmp/virtual-tty pty,raw,echo=0,link=/tmp/virtual-tty-back

class SerialPort {
    int fd;
    public:
    SerialPort(const char* device) {
        fd = open(device, O_RDWR ); //from fcntl
        //fd = open(device, O_RDWR | O_NOCTTY); //from fcntl
        if (fd == -1) {
            std::cerr << "Unable to open device " << device << std::endl;
            exit(-1); //stdlib
        }

        struct termios tty;
        if(tcgetattr(fd, &tty) != 0) {//from termios
            std::cerr << "Error from tcgetattr\n";
            exit(-1);
        }

           // Set Baud Rate
        cfsetospeed(&tty, (speed_t)B9600);//from termios
        cfsetispeed(&tty, (speed_t)B9600);//from termios
                                          //
        tty.c_cflag     &=  ~PARENB;            // Make 8n1
        tty.c_cflag     &=  ~CSTOPB;
        tty.c_cflag     &=  ~CSIZE;
        tty.c_cflag     |=  CS8; 

        tty.c_cflag     &=  ~CRTSCTS;           // no flow control
        tty.c_cc[VMIN]   =  0;                  // 1: read doesn't block
        tty.c_cc[VTIME]  =  5;                  // 0.5 seconds read timeout
        tty.c_cflag     |=  CREAD | CLOCAL;     // turn on READ & ignore ctrl lines

        // Make raw
        cfmakeraw(&tty);

        // Flush Port, then applies attributes
        tcflush(fd, TCIFLUSH);
        if (tcsetattr(fd, TCSANOW, &tty) != 0) {//from termios
            std::cerr << "Error from tcsetattr\n";
            exit(-1);
        }

    } //end SerialPort constructor

    ~SerialPort() {
        close(fd); //from unistd
    }

    void send(const std::string& str) {
        char junk = write(fd, str.c_str(), str.size()); //from unistd
    }

    std::string receive() {
        char buffer[256];
        memset(buffer, 0, sizeof(buffer));
        char junk = read(fd, buffer, sizeof(buffer) - 1); //from unistd
        return std::string(buffer);
    }
};

void dummyServer() {
    std::cout<<"make port"<<std::endl;
    SerialPort sp("/tmp/virtual-tty-back");

    int i = 0;
    std::cout<<"enter while"<<std::endl;
    while(true){
        std::string data = sp.receive();
        usleep(30); //sleep for 30ms
        std::cout<<"rx'ed "<<data<<std::endl;
        sp.send(data);

        if(++i > 10) break;
    }

    std::cout<<"end"<<std::endl;
}

int main(){
    dummyServer();
    return 0;
}

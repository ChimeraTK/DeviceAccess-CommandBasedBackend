#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <iostream>
#include <cstring>
#include <sstream>
#include <vector>
#include <string>

//First start:
//socat -d -d pty,raw,echo=0,link=/tmp/virtual-tty pty,raw,echo=0,link=/tmp/virtual-tty-back

class SerialPort {
    int fd;
    public:
    SerialPort(const char* device) {
        fd = open(device, O_RDWR | O_NOCTTY); //from fcntl
        //O_NOCTTY is questionalbe
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
        //These can in princiapl fail, and return ints. 
        //If those ints are <0, then failure.
        //see https://www.man7.org/linux/man-pages/man3/termios.3.htmlL
        //serial_port_settings.c_cflag &= ~PARENB;: This line disables parity generation and detection1.

        tty.c_cflag     &=  ~PARENB; //disable parity bit generaton and detection
        tty.c_cflag     &=  ~CSTOPB; //This line sets the number of stop bits to one. If CSTOPB were set, two stop bits would be used1.

        tty.c_cflag     &=  ~CSIZE; //clear the character size mask
        tty.c_cflag     |=  CS8;  //set character size to 8 bits

        tty.c_cflag     &=  ~CRTSCTS;           // disable hardware flow control
        tty.c_cc[VMIN]   =  0;                  // 1: read doesn't block
        //VMIN defines the minimum number of characters to read

        tty.c_cc[VTIME]  =  5;                  // 0.5 seconds read timeout
        tty.c_cflag     |=  CREAD | CLOCAL;     // turn on READ (the CREAD receiver) & ignore modem ctrl lines

        cfmakeraw(&tty);// Make raw

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
        int bytesWritten = write(fd, str.c_str(), str.size()); //from unistd

        if (bytesWritten != static_cast<ssize_t>(str.size())) {
            std::cerr << "Incomplete write" << std::endl;
        }

    }

    std::string receive() {
        char buffer[256];
        memset(buffer, 0, sizeof(buffer));
        char junk = read(fd, buffer, sizeof(buffer) - 1); //from unistd
        return std::string(buffer);
    }
};

std::vector<std::string> splitString(const std::string& input, char delimiter) {
    std::vector<std::string> result;
    std::stringstream ss(input);
    std::string item;
    
    // Split the string based on the delimiter
    while (std::getline(ss, item, delimiter)) {
        result.push_back(item);
    }
    
    return result;
}

void dummyServer() {
    std::cout<<"make port"<<std::endl;
    SerialPort sp("/tmp/virtual-tty-back");

    //int i = 0;
    std::cout<<"enter while"<<std::endl;
    while(true){
        std::string data = sp.receive();

        usleep(50*1000L); //sleep for 50ms

        std::vector<std::string> lines = splitString(data, ';');
        for(const std::string& dat : lines){
            std::cout<<"rx'ed "<<dat<<std::endl;
            sp.send(dat);
            usleep(50*1000L); //sleep for 50ms
        }

        //if(++i > 10) break;
    }

    std::cout<<"end"<<std::endl;
}

int main(){
    dummyServer();
    return 0;
}

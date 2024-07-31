#include <iostream>
#include <string>
#include <fstream>
#include <fcntl.h> 
#include <unistd.h> //POSIX OS API
#include <termios.h> //for termain IO interface
#include <cstring>
#include <future>
//#include <stdexcept>
//#include <errno.h>

//**********************************************************************************************************************
std::string replaceNewlines(const std::string& input) {
    //For debug only
    std::string result = input;

    // Replace '\n' with 'N'
    size_t pos = result.find('\n');
    while (pos != std::string::npos) {
        result.replace(pos, 1, "N");
        pos = result.find('\n', pos + 1);
    }

    // Replace '\r' with 'R'
    pos = result.find('\r');
    while (pos != std::string::npos) {
        result.replace(pos, 1, "R");
        pos = result.find('\r', pos + 1);
    }

    return result;
}

//**********************************************************************************************************************
class SerialPort {
    private:
        int fileDescriptor;
        std::string _serialDeviceName;
        std::string _delim; //Line delimiter for the serial port
        int _delim_size;    //= _delim.size();

    public:
        SerialPort(std::string device, std::string delim="\r\n"): _serialDeviceName(device), _delim(delim) {

            _delim_size = delim.size();
            if(_delim_size < 1){
                _delim = "\n";
                _delim_size  = 1;
            }

            fileDescriptor = open(device.c_str(), O_RDWR | O_NOCTTY); //from fcntl
                                                                      //O_RDWR = open for read + write
                                                                      //O_RDONLY = open read only
                                                                      //O_WRONLY = open write only
                                                                      //O_NOCTTY = no controlling termainl
            if (fileDescriptor == -1) {
                std::cerr << "Unable to open device " << device << std::endl;
                exit(-1); //stdlib
            }

            struct termios tty;
            if(tcgetattr(fileDescriptor, &tty) != 0) {//from termios
                std::cerr << "Error from tcgetattr\n";
                exit(-1);
            }

            //Set baud rate, using termios
            int iCfsetospeed = cfsetospeed(&tty, (speed_t)B9600);
            int iCfsetispeed = cfsetispeed(&tty, (speed_t)B9600);
            if(iCfsetospeed < 0 or iCfsetispeed < 0){
                std::cerr << "Error setting IO speed\n";
                exit(-1);
            }
            //see https://www.man7.org/linux/man-pages/man3/termios.3.htmlL
            //serial_port_settings.c_cflag &= ~PARENB;: This line disables parity generation and detection1.
            tty.c_cflag     &=  ~PARENB;            // Make 8n1
            tty.c_cflag     &=  ~CSTOPB;
            tty.c_cflag     &=  ~CSIZE;
            tty.c_cflag     |=  CS8; 
            tty.c_cflag     &=  ~CRTSCTS;           // no flow control
            tty.c_cc[VMIN]   =  0;                  // 0: read blocks,  1: read doesn't block
                                                    // VMIN defines the minimum number of characters to read
            tty.c_cc[VTIME]  =  5;                  // 0.5 seconds read timeout
            tty.c_cflag     |=  CREAD | CLOCAL;     // turn on READ & ignore ctrl lines

            cfmakeraw(&tty); 

            // Flush Port, then applies attributes
            tcflush(fileDescriptor, TCIFLUSH);
            if (tcsetattr(fileDescriptor, TCSANOW, &tty) != 0) {//from termios
                std::cerr << "Error from tcsetattr\n";
                exit(-1);
            }
        } //end SerialPort constructor

        //**************************************************************************************************************
        ~SerialPort() {
            close(fileDescriptor); //from unistd
        }

        //**************************************************************************************************************
        std::string getDelim(){
            return _delim;
        }

        //**************************************************************************************************************
        void send(const std::string& str) {
            std::string strToWrite = str+_delim;
            //std::cout<<"   sending "<<replaceNewlines( strToWrite)<<std::endl; //DEBUG
            int bytesWritten = write(fileDescriptor, strToWrite.c_str(), strToWrite.size()); //from unistd

            if (bytesWritten != static_cast<ssize_t>(strToWrite.size())) {
                std::cerr << "Incomplete write" << std::endl;
                //TODO throw an error
            }
            //std::cout<<"   send done, bytes written: "<<bytesWritten<<std::endl; //DEBUG
        }

        //**************************************************************************************************************
        bool strEndsInDelim(std::string str){
            int s = str.size()-1;
            int d = _delim_size -1;
            while(s>=0 and d>= 0){
                if (str[s--] != _delim[d--]){
                    return false; 
                }
            }
            return d<0;
        }

        //**************************************************************************************************************
        std::string stripDelim(std::string str){
            if(strEndsInDelim(str)){
                return str.substr(0, str.size() - _delim_size);
            } else {
                return str;
            }
        }

        //**************************************************************************************************************
        std::string receive() { //sort-of working
            char buffer[256];
            //std::cout<<"   receiving (slot size:"<<sizeof(buffer) - 1<<")"<<std::endl;//DEBUG
            std::string outputStr = "";
            int bytes_read;
            //int bytes_read_total = 0; //DEBUG

            do{
                //std::cout << "       reading in loop, current total: " << bytes_read_total << std::endl; // DEBUG
                memset(buffer, 0, sizeof(buffer));
                bytes_read = read(fileDescriptor, buffer, sizeof(buffer) - 1); //from unistd
                                                                               //the -1 makes room for read to insert a '\0' null termination at the end
                                                                               //bytes_read_total += bytes_read>0 ? bytes_read : 0;//DEBUG
                outputStr += std::string(buffer);
                //std::cout << "       bytes read: "<<bytes_read << std::endl; // DEBUG
            } while(bytes_read >= 255 and not strEndsInDelim(outputStr));

            return stripDelim(outputStr); 
            /*std::string ret2 = stripDelim(outputStr); //DEBUG
              std::cout<<"   receive got "<<replaceNewlines(ret2)<<" bytes read " <<bytes_read<<std::endl; //DEBUG
              return ret2;*/
        } //end receive

        //**************************************************************************************************************
        /*std::string receive() { //sort-of working
          std::cout<<"   receiving"<<std::endl;
          char buffer[256];
          memset(buffer, 0, sizeof(buffer));
          int bytes_read = read(fileDescriptor, buffer, sizeof(buffer) - 1); //from unistd
          std::string ret = std::string(buffer);
          std::cout<<"   receive got "<<ret<<" bytes read " <<bytes_read<<std::endl; //DEBUG
          return ret;
          }*/

}; //end SerialPort



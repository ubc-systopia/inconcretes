#include "AS_5600.h"
#include <bitset>
AS_5600::AS_5600(){
    std::cout<<"start read"<<endl;
    int adapter_nr = 1;
    char filename[20];
    // snprintf(filename, 19, "/dev/i2c-%d", adapter_nr);
    // this->file = open(filename, O_RDWR);
    // if (file < 0) {
    //     std::cout << "file less than 0"<<endl;
    //     exit(1);
    // }
    // int addr = 0x36;
    // if (ioctl(file, I2C_SLAVE, addr) < 0) {
    //     std::cout << "i2c read error" <<endl;
    //     exit(1);
    // }

    // open serial port
    unsigned char recievedData[256];
    serialPort = open("/dev/serial0",  O_RDONLY | O_NOCTTY | O_NDELAY);
    if(serialPort==-1){
        std::cout<<"Error opening serial port."<<std::endl;
    }

    // configure the serial port with baudrate, bits per byte and parity setting
    struct termios serialConfig;
    tcgetattr(serialPort, &serialConfig);
    serialConfig.c_cflag = 3261;
    cfsetispeed(&serialConfig, B115200);
    serialConfig.c_cflag &= ~PARENB;
    serialConfig.c_cflag &= ~CSTOPB;
    serialConfig.c_cc[VMIN]=1;
    serialConfig.c_cc[VTIME]=0;
    serialConfig.c_cflag &= ~CSIZE;
    // serialConfig.c_lflag &= ICANON;
    // serialConfig.c_cflag |= CS8;
    // serialConfig.c_cflag &= ~CRTSCTS;
    tcsetattr(serialPort, TCSANOW, &serialConfig);
    std::cout<<serialConfig.c_cflag<<endl;
}

int AS_5600::getAngleUART(){
    int retval=-1;
    int angle=0;
    int iter=0;
    // only flush input buffer, don't need to flush output
    tcflush(serialPort, TCIFLUSH);
    while(retval<3){
        iter++;
        // auto loopStartTime=chrono::steady_clock::now();
        // if(retval>0){
        //     cout<<recievedData<<endl;
        // } else{
        //     cout<<"ret val 0"<<endl;
        // }
        retval=read(serialPort, &recievedData, sizeof(recievedData));
        if(recievedData[0]!='n'){
            retval=0;
        }
        // cout<< "serial read time: "<< chrono::duration_cast<chrono::milliseconds>(chrono::steady_clock::now()-
        // loopStartTime).count()<<" | "<< retval <<endl;
    }
    // std::cout<<iter<< " | ";
    // cout<<retval<<" | ";

    for(int i=1;i<retval-1;i++){
        angle+=(int(recievedData[i])-48)*(pow(10,retval-2-i));
    }

    // std::couts<<angle<<endl;
    
    return angle;
}

// int AS_5600::getRawAngle(){
//     return readTwoBytes(_addr_raw_angle);
// }

// int AS_5600::getPowerMode(){
//     return readTwoBytes(_addr_conf);
// }

// double AS_5600::getAngle(){
//     return ((double) readTwoBytes(_addr_angle))*(360.0)/4096.0;
// }

// int AS_5600::getZPosition(){
//     return readTwoBytes(_addr_zpos);
// }

// void AS_5600::writeStartAngle(){
//     return writeTwoBytes(_addr_zpos, getRawAngle());
// }

// void AS_5600::setHystersis(uint8_t hysteresis){
//     __u8 reg=_addr_conf;
//     uint8_t currConf= i2c_smbus_read_word_data(file, reg);
//     if(currConf<0){
//         std::cout<<"err"<<endl;
//     }
//     std::bitset<8> binary1(currConf);
//     std::cout<<binary1<<endl;
//     std::cout<<"done"<<endl;
//     // currConf &= ~HYSTERESIS_BIT_MASK;
//     // currConf |= (hysteresis<<2);
//     // i2c_smbus_write_byte_data(file, (_addr_conf+1), currConf);
// }

// bool AS_5600::detectMagnet(){
//     uint8_t value;
//     __u8 reg = 0x0b;
//     value = i2c_smbus_read_word_data(file, reg);
//     if (value < 0) {
//         std::cout<<"err" <<endl;
//     }
//     std::cout<<(int)(value)<<endl;
//     return (value & 0x20); 
// }

// void AS_5600::writeTwoBytes(int addr_out, int bytes){
//     std::cout<<"bytes: "<<bytes<<endl;
//     uint8_t highByte=(bytes>>8) & 0xFF;
//     uint8_t lowByte = bytes & 0xFF;
//     std::bitset<8> binary(highByte);
//     std::bitset<8> binary1(lowByte);
//     std::cout<<"high byte: "<< binary << endl;
//     std::cout<<"low byte: "<< binary1 << endl;
//     i2c_smbus_write_byte_data(file, (addr_out), highByte);
//     i2c_smbus_write_byte_data(file, (addr_out+1), lowByte);
// }

// int AS_5600::readTwoBytes(int addr_in){
//     __u8 reg = addr_in;
//     uint16_t highByte;
//     uint8_t lowByte;
//     char buf[10];
//     highByte = i2c_smbus_read_word_data(file, reg);
//     lowByte = i2c_smbus_read_word_data(file, reg+1);
//     if (highByte < 0 || lowByte < 0) {
//         std::cout<<"err" <<endl;
//     /* ERROR HANDLING: i2c transaction failed */
//     } else {
//         std::bitset<16> binary(highByte);
//         std::bitset<8> binary1(lowByte);
//         // cout<<"high byte: "<< binary << endl;
//         // cout<<"low byte: "<< binary1 << endl;
//         highByte=highByte<<8;
//         // cout<<"Raw Angle: "<< (highByte | lowByte) << endl;
//     }
//      return ((int)(highByte|lowByte));
// }

// int AS_5600::getOutputMode(){
//     std::cout<<readTwoBytes(_addr_conf)<<endl;
//     return ((readTwoBytes(_addr_conf)>>4) & 0x03);
// }

// bool AS_5600::setOutputMode(uint8_t outputMode){
//     uint8_t currConf = i2c_smbus_read_word_data(file, _addr_conf+1);
//     currConf &= ~OUTPUT_BIT_MASK;
//     currConf |= (outputMode<<4);
//     i2c_smbus_write_byte_data(file, (_addr_conf+1), currConf);
//     return true;
// }



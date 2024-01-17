#ifndef AS_56000
#define AS_56000
// extern "C" {
//     #include <linux/i2c-dev.h>
//     #include <i2c/smbus.h>
// }
#include <stdint.h>
// #include <linux/i2c-dev.h>
// #include <i2c/smbus.h>
#include <cstdio>
#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <sstream>
#include <fstream>
#include <termios.h>
#include <chrono>
#include <cmath>
#include <thread>


using namespace std;

class AS_5600{
    public:
    static const uint8_t _addr_angle=0x0E;
    static const uint8_t _addr_raw_angle=0x0C;
    static const uint8_t _addr_zpos=0x01;
    static const uint8_t _addr_conf=0x07;
    static const uint8_t HYSTERESIS_BIT_MASK=0x0C;
    static const uint8_t OUTPUT_BIT_MASK=0x30;
    
    
    AS_5600(void);
    int file;
    int serialPort;
    unsigned char recievedData[256];

    // bool detectMagnet();
    // int getRawAngle();
    int getAngleUART();
    // double getAngle();
    // int getZPosition();
    // int getPowerMode();
    // int getOutputMode();
    // void setHystersis(uint8_t hysteresis);
    // int readTwoBytes(int addr_in);
    // void writeTwoBytes(int addr_out, int bytes);
    // void writeStartAngle();
    // bool setOutputMode(uint8_t outputMode);
};

#endif
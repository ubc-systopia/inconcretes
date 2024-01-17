#ifndef ULTRASONIC
#define ULTRASONIC

#include <pigpio.h>
#include <iostream>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>

using namespace std;

class Ultrasonic{
    public:
        Ultrasonic(int trigger, int echo);
        void initializePins();
        struct timeval tv;
        int coefficient=17150;
        double getTime();
        bool waitValue(int value, int limit=35000);
        double detectDistance();
    private:
        int TRIG_PIN;
        int ECHO_PIN;
};

#endif
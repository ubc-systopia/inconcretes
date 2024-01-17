#include "Ultrasonic.h"

Ultrasonic::Ultrasonic(int trigger, int echo){
    this->TRIG_PIN=trigger;
    this->ECHO_PIN=echo;
}

double Ultrasonic::getTime(){
    gettimeofday(&tv, NULL);
    return (double) tv.tv_sec+(double)tv.tv_usec*0.000001;
}

void Ultrasonic::initializePins(){
    gpioSetMode(this->TRIG_PIN, PI_OUTPUT);
    gpioSetMode(this->ECHO_PIN, PI_INPUT);
}

bool Ultrasonic::waitValue(int value, int limit){
    for(int i=0; gpioRead(ECHO_PIN)==value;++i){
        if(i>=limit)
            return false;
    }
    return true;
}
double Ultrasonic::detectDistance(){
    gpioWrite(TRIG_PIN, 0);
    usleep(10);
    gpioWrite(TRIG_PIN,1);
    usleep(10);
    gpioWrite(TRIG_PIN,0);

    if(waitValue(0)){
        double pulseStart=getTime();
        if(waitValue(1)){
            double pulseEnd=getTime();
            double duration=pulseEnd-pulseStart;
            double distance=duration*coefficient;
            return distance;
        }
    }
    // cout<<"Measurement Error"<<endl;
    cout<<"err"<<endl;
    return 0.0/0.0;
}


#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <vector>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET 	-1 // This display does not have a reset pin accessible

#define PI1_PWM PB12
#define PI2_PWM PA8
#define PI3_PWM PA10
#define PI4_PWM PB3

#define PI1_DIR PB13
#define PI2_DIR PB15
#define PI3_DIR PA11
#define PI4_DIR PA15
    
#define PWM_OUT PB_6
#define DIR_OUT PB7

TwoWire Wire2(PB11, PB10);
Adafruit_SSD1306 display_handler(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire2, OLED_RESET);
// Serial object to transmit to raspberry pis
int iterCount=0;
int pwmOutFreq=50000;
double piInputPeriod=5.0;
bool emptyVec=0;
long pulseTime1=0;
long pulseTime2=0;
long pulseTime3=0;
long pulseTime4=0;
int dir1=0;
int dir2=0;
int dir3=0;
int dir4=0;
long medianPulseTime=0;
long outputDutyCycle=0;
int dir=0;


void setup_display(Adafruit_SSD1306 &display_handler){
    display_handler.begin(SSD1306_SWITCHCAPVCC, 0x3C);
    // Displays Adafruit logo by default. call clearDisplay immediately if you don't want this.
    display_handler.display();
    delay(2000);

    // Displays "Hello world!" on the screen
    display_handler.clearDisplay();
    display_handler.setTextSize(1);
    display_handler.setTextColor(SSD1306_WHITE);
    display_handler.setCursor(0,0);
    display_handler.println("Hello world!");
    display_handler.display();
}

void initialize_pins(){
    pinMode(PI1_PWM, INPUT);
    pinMode(PI2_PWM, INPUT);
    pinMode(PI3_PWM, INPUT);
    pinMode(PI4_PWM, INPUT);

    pinMode(PI1_DIR, INPUT);
    pinMode(PI2_DIR, INPUT);
    pinMode(PI3_DIR, INPUT);
    pinMode(PI4_DIR, INPUT);

    pinMode(PWM_OUT, OUTPUT);
    pinMode(DIR_OUT, OUTPUT);
    pwm_start(PWM_OUT, pwmOutFreq, 0, TimerCompareFormat_t::RESOLUTION_12B_COMPARE_FORMAT);
}

void read_pi_inputs(){
    pulseTime1=pulseIn(PI1_PWM, HIGH, 1000);//(pow(10,7));
    pulseTime2=pulseIn(PI2_PWM, HIGH, 1000);//(pow(10,7));
    pulseTime3=pulseIn(PI3_PWM, HIGH, 1000);//(pow(10,7));
    pulseTime4=pulseIn(PI4_PWM, HIGH, 1000);//(pow(10,7));

    dir1=digitalRead(PI1_DIR);
    dir2=digitalRead(PI2_DIR);
    dir3=digitalRead(PI3_DIR);
    dir4=digitalRead(PI4_DIR);
}

void compute_median(){
    // create vector of pulse times if pulse time is not 0
    std::vector<long> pulseTimes;
    // if dir1 is high, give pulseTime1 a negative sign 
    if(pulseTime1) {if(dir1) pulseTime1=-pulseTime1; pulseTimes.push_back(pulseTime1);};
    if(pulseTime2) {if(dir2) pulseTime2=-pulseTime2; pulseTimes.push_back(pulseTime2);};
    if(pulseTime3) {if(dir3) pulseTime3=-pulseTime3; pulseTimes.push_back(pulseTime3);};
    if(pulseTime4) {if(dir4) pulseTime4=-pulseTime4; pulseTimes.push_back(pulseTime4);};
    

    // find median of pulse times
    if(!pulseTimes.size()){
        display_handler.clearDisplay();
        display_handler.setCursor(0,0);
        emptyVec=!emptyVec;
        display_handler.println("Empty Vector: " + String(emptyVec));
        display_handler.display();
    } else{
        std::sort(pulseTimes.begin(), pulseTimes.end());
        long range=abs(pulseTimes[pulseTimes.size()-1]-pulseTimes[0]);
        std::sort(pulseTimes.begin(), pulseTimes.end());
        medianPulseTime=double(pulseTimes[pulseTimes.size()/2]);
        outputDutyCycle=abs(double(medianPulseTime))/piInputPeriod;
        dir=medianPulseTime<0;
        display_handler.clearDisplay();
        display_handler.setCursor(0,0);
        display_handler.println("size: " + String(pulseTimes.size()));
        display_handler.println("Med D.C: " + String(abs(double(outputDutyCycle))));
        display_handler.println("pulseTimes[0] :" + String(double(pulseTimes[0])));
        display_handler.println("Med: " + String(double(medianPulseTime)));
        display_handler.println("Pi1 P.T: " + String(pulseTime1));
        display_handler.println("Pi1 Dut: " + String(double(pulseTime1)/piInputPeriod));
        display_handler.println("Range: " + String(range));
        display_handler.println("Direction: " + String(dir));
        display_handler.display();
    }
}

void output_to_motor_driver(){
    pwm_start(PWM_OUT, pwmOutFreq, outputDutyCycle, TimerCompareFormat_t::PERCENT_COMPARE_FORMAT);
    digitalWrite(DIR_OUT, dir);
}

// ARDUINO SETUP LOOP
void setup() {
    setup_display(display_handler);
    initialize_pins();
}


// ARDUINO MAIN LOOP
void loop() {
    unsigned long outputDutyCycle=0;
    int dir=0;

    read_pi_inputs();

    compute_median();
    // outputs previously computed median to the motor driver
    output_to_motor_driver();
    // display_handler.clearDisplay();
    // display_handler.setCursor(0,0);
    // display_handler.println("Itercount: " + String(iterCount++));
    // // // display_handler.println("Pulse time: " + String(pulseTime));
    // display_handler.println("Duty cycle: " + String(outputDutyCycle));
    // display_handler.println("Direction: " + String(dir));
    // // // display_handler.println("Pi number: " + String(piNum));
    // display_handler.display();
    // display_handler.clearDisplay();
};

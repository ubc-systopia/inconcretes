#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <AS5600.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET 	-1 // This display does not have a reset pin accessible
#define AS5600_I2C_ERROR_VAL 3839
TwoWire Wire2(PB11, PB10);
Adafruit_SSD1306 display_handler(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire2, OLED_RESET);
AS5600 i2cRotaryEncoder;
int iterCount=0;
int UARTUnavailableCount=0;
int AS5600UnavailableCount=0;
int rawAngle=0;

void setup_display(){
  display_handler.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display_handler.display();
  delay(2000);
  display_handler.clearDisplay();
  display_handler.setTextSize(1);
  display_handler.setTextColor(SSD1306_WHITE);
  display_handler.setCursor(0,0);
  display_handler.println("Hello world!");
  display_handler.display();
}

void try_reset_i2c(){
  Wire.end();
  Wire.begin();
}

void update_i2c_error_count(){
  display_handler.clearDisplay();
  display_handler.setCursor(0,0);
  display_handler.println("AS5600 Unavailable Count:" + String(++AS5600UnavailableCount));
  display_handler.println("UART Unavailable Count:" + String(UARTUnavailableCount));
  display_handler.display();
}

void startup_Serial(){
  Wire.begin();
  Serial1.end();
  Serial1.begin(115200);
}

void send_angle_over_uart(){
  Serial1.println("n"+String(i2cRotaryEncoder.rawAngle()));
}

void debug_print(){
  display_handler.clearDisplay();
  display_handler.setCursor(0,0);
  display_handler.println("Itercount: " + String(iterCount++));
  display_handler.println("Angle: " + String(i2cRotaryEncoder.rawAngle()));
  display_handler.println("Power Mode: " + String(i2cRotaryEncoder.getPowerMode()));
  display_handler.display();
}

// ARDUINO SETUP LOOP
void setup() {
  startup_Serial();
  setup_display();
}

// ARDUINO MAIN LOOP
void loop() {
  // COMMENT OUT DURING PENDULUM USE, SLOWS DOWN LOOP SIGNIFICANTLY
  // debug_print();

  while(rawAngle==AS5600_I2C_ERROR_VAL){ //same as value if encoder is disconnected
    try_reset_i2c();
    rawAngle=i2cRotaryEncoder.rawAngle();
    update_i2c_error_count();
  }

  // while(!Serial.availableForWrite()){
  //     display_handler.clearDisplay();
  //     display_handler.setCursor(0,0);
  //     display_handler.println("AS5600 Unavailable Count:" + String(AS5600UnavailableCount));
  //     display_handler.println("UART Unavailable Count:" + String(++UARTUnavailableCount));
  //     display_handler.display();
  //     Serial1.end();
  //     Serial1.begin(115200);
  // }

  send_angle_over_uart();
};


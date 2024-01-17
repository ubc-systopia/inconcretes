#include "Ultrasonic.h"
#include <pigpio.h>

int  us_trig = 17, us_echo = 27;

void ultrasonic_test() {
  Ultrasonic linear_position_encoder(us_trig, us_echo);
  while (true) {
    std::cout << linear_position_encoder.detectDistance() << std::endl;
  }
}


int main(int argc, char **argv) {
  gpioTerminate();
  gpioInitialise();
  ultrasonic_test();
  return 1;
}
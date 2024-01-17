#include <pigpio.h>
#include <iostream>

int limit_switch=22;

void limit_sw_test() {
  gpioSetMode(limit_switch, PI_INPUT);
  gpioSetPullUpDown(limit_switch, PI_PUD_DOWN);
  while (true) {
    std::cout << gpioRead(limit_switch) << std::endl;
  }
}

int main(int argc, char **argv) {
  gpioTerminate();
  gpioInitialise();
  limit_sw_test();
  return 1;
}
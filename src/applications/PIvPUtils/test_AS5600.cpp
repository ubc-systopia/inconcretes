#include "AS_5600.h"

void AS5600_test() {
  AS_5600 rotary_encoder;
  while (true) {
    std::cout << rotary_encoder.getAngleUART() << std::endl;
  }
}

int main(int argc, char **argv) {
  AS5600_test();
  return 1;
}
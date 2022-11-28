#include "achal/bft_kvs.h"

namespace Achal {

bool simpleMajority(std::vector<std::string> values, std::string &majority) {
  std::map<std::string, int> frequencies;

  for (std::string value : values) {
    if (!CONTAINS_KEY(frequencies, value)) {
      frequencies[value] = 0;
    }
    frequencies[value]++;
  }

  if (frequencies.size() == 0) {
    return false;
  }

  int max_frequency = -1;

  for (auto vf : frequencies) {
    if (vf.second > max_frequency) {
      max_frequency = vf.second;
      majority = vf.first;
    }
  }

  return true;
}

std::string fuse_default(std::vector<std::string> values) {
  std::string majority = "";
  Achal::simpleMajority(values, majority);
  return majority;
}

}  // namesapce Achal

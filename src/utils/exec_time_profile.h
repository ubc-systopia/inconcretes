#ifndef UTILS_EXEC_TIME_PROFILE_H
#define UTILS_EXEC_TIME_PROFILE_H

#include <vector>

namespace Utils {

class ExecTimeProfile {
 public:
  uint64_t exec_time_observed_P00;
  uint64_t exec_time_observed_P25;
  uint64_t exec_time_observed_P50;
  uint64_t exec_time_observed_P75;
  uint64_t exec_time_observed_P90;
  uint64_t exec_time_observed_P99;
  uint64_t exec_time_observed_P100;
  uint64_t wcet_estimated;

  ExecTimeProfile(std::vector<uint64_t>& measurements);
  void print(std::string label = "");
};

} // namespace Utils

#endif  // UTILS_EXEC_TIME_PROFILE_H

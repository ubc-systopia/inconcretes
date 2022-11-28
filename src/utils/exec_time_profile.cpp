#include "exec_time_profile.h"

ExecTimeProfile::ExecTimeProfile(std::vector<uint64_t>& measurements) {
  if (measurements.size() > 0) {
    std::sort(measurements.begin(), measurements.end());
    exec_time_observed_P00 = measurements.front();
    exec_time_observed_P25 = measurements[measurements.size() * 0.25];
    exec_time_observed_P50 = measurements[measurements.size() * 0.50];
    exec_time_observed_P75 = measurements[measurements.size() * 0.75];
    exec_time_observed_P90 = measurements[measurements.size() * 0.90];
    exec_time_observed_P99 = measurements[measurements.size() * 0.99];
    exec_time_observed_P100 = measurements.back();
    wcet_estimated = exec_time_observed_P100 * 2;
  }
}

void ExecTimeProfile::print(std::string label) {
  std::cout << label << " profile "
            << "P00:" << exec_time_observed_P00 << "ns, "
            << "P25:" << exec_time_observed_P25 << "ns, "
            << "P50:" << exec_time_observed_P50 << "ns, "
            << "P75:" << exec_time_observed_P75 << "ns, "
            << "P90:" << exec_time_observed_P90 << "ns, "
            << "P99:" << exec_time_observed_P99 << "ns, "
            << "P100:" << exec_time_observed_P100 << "ns, "
            << "WCET (estimated):" << wcet_estimated << "ns = "
            << NS_TO_US(wcet_estimated) << "us = " << NS_TO_MS(wcet_estimated) << "ms"
            << std::endl;
}

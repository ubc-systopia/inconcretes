#ifndef ACHAL_KVS_INTERFACE_H
#define ACHAL_KVS_INTERFACE_H

#include <vector>
#include <algorithm>
#include <iostream>
#include <utility>

#include "utils/periodic_task.h"

#define GET_SUCCESS(stat) (stat.first)
#define GET_FAILURE(stat) (stat.second)
#define INC_SUCCESS(stat) (stat.first++)
#define INC_FAILURE(stat) (stat.second++)

namespace Achal {

class KVSInterface : public Utils::PeriodicTask {
 public:
  bool use_simple_median = false;

  std::pair<uint64_t, uint64_t> try_read_batch(
      std::vector<std::string>& keys,
      uint64_t no_earlier_than,
      std::vector<std::string>& values) {
    std::pair<uint64_t, uint64_t> result(0, 0);
    for (unsigned i = 0; i < keys.size(); i++) {
      if (try_read(keys[i], no_earlier_than, values[i])) {
        INC_SUCCESS(result);
      } else {
        INC_FAILURE(result);
      }
    }
    return result;
  }
  
  std::pair<uint64_t, uint64_t> try_write_batch(
      std::vector<std::string>& keys,
      uint64_t publish_time,
      std::vector<std::string>& values) {
    std::pair<uint64_t, uint64_t> result(0, 0);
    for (unsigned i = 0; i < keys.size(); i++) {
      if (try_write(keys[i], publish_time, values[i])) {
        INC_SUCCESS(result);
      } else {
        INC_FAILURE(result);
      }
    }
    return result;
  }

  virtual bool try_read(std::string key, uint64_t no_earlier_than,
                        std::string &value) = 0;

  virtual bool try_write(std::string key, uint64_t publish_time,
                         std::string value) = 0;

  virtual void print_stats() = 0;

  bool fuse_redundant_values(std::vector<std::string>& candidate_values,
                             bool use_simple_median,
                             std::string& result) {
    if (use_simple_median) {
      if (candidate_values.size() <= 2) {
        return false;
      }

      double median;
      std::vector<double> values;

      for (std::string v : candidate_values) {
        values.push_back(std::stod(v));
      }
      if (values.empty()) {
        return false;
      }
      std::sort(values.begin(), values.end());
      median = (values[values.size() / 2] + values[(values.size() - 1) / 2]) / 2.0;
      result = std::to_string(median);
    } else {
      if (candidate_values.size() < 2) {
        return false;
      } else if (candidate_values.size() == 2) {
        if (candidate_values[0] == candidate_values[1]) {
          result = candidate_values[0];
        } else {
          return false;
        }
      } else if (candidate_values.size() == 3) {
        if (candidate_values[0] == candidate_values[1]) {
          result = candidate_values[0];
        } else if (candidate_values[1] == candidate_values[2]) {
          result = candidate_values[1];
        } else if (candidate_values[2] == candidate_values[0]) {
          result = candidate_values[2];
        } else {
          return false;
        }
      } else if (candidate_values.size() == 4) {
        if (candidate_values[0] == candidate_values[1] and candidate_values[1] == candidate_values[2]) {
          result = candidate_values[0];
        } else if (candidate_values[0] == candidate_values[1] and candidate_values[1] == candidate_values[3]) {
          result = candidate_values[0];
        } else if (candidate_values[0] == candidate_values[2] and candidate_values[2] == candidate_values[3]) {
          result = candidate_values[0];
        } else if (candidate_values[1] == candidate_values[2] and candidate_values[2] == candidate_values[3]) {
          result = candidate_values[1];
        } else {
          return false;
        }
      } else {
        assert(false);
      }
    }
    return true;
}


  KVSInterface(unsigned id, uint64_t period_ns, uint64_t offset_ns,
               unsigned priority, unsigned cpu, log4cpp::Category *logger)
    : Utils::PeriodicTask(id, period_ns, offset_ns, priority, cpu, logger) {}
};

} // namespace Achal

#endif  // ACHAL_KVS_INTERFACE_H

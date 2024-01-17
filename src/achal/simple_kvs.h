#ifndef ACHAL_SIMPLE_KVS_H
#define ACHAL_SIMPLE_KVS_H

#include <vector>
#include <iostream>

#include "../third-party/abseil-cpp/absl/container/flat_hash_map.h"
#include "kvs_interface.h"
#include "utils/misc.h"
#include "utils/timespec.h"

namespace Achal {

class SimpleKVS : public KVSInterface {
 private:
  config_t config;
public:
  uint64_t total_run_time = 0;
 protected:
  absl::flat_hash_map<std::string, std::string> store;

 public:
  SimpleKVS(config_t conf):KVSInterface(conf.id, conf.period_ns, 
                    conf.offset_ns, conf.priority, conf.cpu, conf.logger),
                    config(conf)
                    {}
  void write(std::string key, std::string value);
  void write(std::string key, uint64_t publish_time, std::string value);
  void write_batch(std::vector<std::string>& keys,
                   std::vector<std::string>& values);
  bool try_write(std::string key, uint64_t publish_time, std::string value);
  bool try_write_batch(std::vector<std::string>& keys, uint64_t publish_time,
                       std::vector<std::string>& values);

  std::string read(std::string key);
  void read_batch(std::vector<std::string>& keys,
                  std::vector<std::string>& values);
  bool try_read_batch(std::vector<std::string>& keys, uint64_t no_earlier_than,
                      std::vector<std::string>& values);
  bool try_read(std::string key, uint64_t no_earlier_than, std::string &value);

  void print_stats(){
    TimeSpec diff_ts;
    timespecsub(&task_finish_ts, &task_start_ts, &diff_ts);
    LDEBUG << "BFTKVS Task " << id << " actual (expected) duration: "
          << TS_TO_MS(&diff_ts) << "ms (" << config.max_jobs * NS_TO_MS(time_period_ns)
          << "ms), BCET (measured): " << bcet_measured_ms
          << "ms, ACET (measured): " << acet_measured_ms
          << "ms, WCET (measured): " << wcet_measured_ms << "ms";
  }

  void job(){
    uint64_t start = Utils::get_time_now_ns();
    num_jobs++;
    total_run_time += (Utils::get_time_now_ns() - start);
  }

  void job_for_profiler(){
    job();
  }

  bool terminate(){
    return this->terminate_flag;
    return num_jobs >= config.max_jobs;
  }

};

} // namespace Achal

#endif  // ACHAL_SIMPLE_KVS_H

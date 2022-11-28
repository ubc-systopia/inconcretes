#ifndef APPLICATIONS_KZH15_H
#define APPLICATIONS_KZH15_H

#include <boost/math/distributions/beta.hpp>

#include "achal/bft_kvs.h"
#include "utils/periodic_task.h"
#include "utils/kzh15_benchmark_generator.h"

namespace Applications {

class KZH15Task : public Utils::PeriodicTask {
 public:
  Achal::KVSInterface* kvs;

  std::vector<std::string> ro_labels_read_keys;
  std::vector<std::string> ro_labels_read_values;
  std::vector<std::string> wo_labels_written_keys;
  std::vector<std::string> wo_labels_written_values;
  std::vector<std::string> rw_inter_task_labels_read_keys;
  std::vector<std::string> rw_inter_task_labels_read_values;
  std::vector<std::string> rw_inter_task_labels_written_keys;
  std::vector<std::string> rw_inter_task_labels_written_values;
  std::vector<std::string> rw_intra_task_labels_read_keys;
  std::vector<std::string> rw_intra_task_labels_read_values;
  std::vector<std::string> rw_intra_task_labels_written_keys;
  std::vector<std::string> rw_intra_task_labels_written_values;

  std::vector<uint64_t> rw_inter_task_labels_read_freshness_constraints;

  uint64_t acet_ns;
  uint64_t bcet_ns;
  uint64_t wcet_ns;

  Utils::ExecTimeProfile *kvs_read_profile;
  Utils::ExecTimeProfile *kvs_write_profile;

  uint64_t max_jobs;
  uint64_t num_jobs_to_ignore;

  std::minstd_rand0* gen;
  UniRealDist* uni_real_dist;
  boost::math::beta_distribution<>* beta_dist;

  log4cpp::Category *logger;

  uint64_t get_random_et_ns();
  void profile_kvs_rw(unsigned int max_iterations);

  //std::vector<bool> read_stats;
  //std::vector<bool> write_stats;

  uint64_t successful_batch_reads = 0;
  uint64_t successful_batch_writes = 0;
  uint64_t successful_reads = 0;
  uint64_t successful_writes = 0;
  uint64_t failed_batch_reads = 0;
  uint64_t failed_batch_writes = 0;
  uint64_t failed_reads = 0;
  uint64_t failed_writes = 0;

  std::pair<uint64_t, uint64_t> read_status_1;
  std::pair<uint64_t, uint64_t> read_status_2;
  std::pair<uint64_t, uint64_t> read_status_3;
  std::pair<uint64_t, uint64_t> write_status_1;
  std::pair<uint64_t, uint64_t> write_status_2;
  std::pair<uint64_t, uint64_t> write_status_3;
  std::pair<uint64_t, uint64_t> write_status_4;

 public:
  KZH15Task(unsigned id, uint64_t period_ns, uint64_t offset_ns, int prio,
            int cpu, Achal::KVSInterface* kvs, uint64_t max_jobs,
            Utils::KZH15::TaskBasic* task, std::map<int, int>& label_sizes,
            int max_label_size, log4cpp::Category *logger);
  KZH15Task(unsigned id, uint64_t period_ns, uint64_t offset_ns, int prio,
            int cpu, Achal::KVSInterface* kvs, uint64_t max_jobs,
            Utils::KZH15::Task* task, log4cpp::Category *logger);
  ~KZH15Task();

  void init_runnables(std::vector<Utils::KZH15::Runnable*>& runnables);

  uint64_t get_wcet_ns() {
    return wcet_ns;
  }

  void job();
  void job_for_profiler();
  bool terminate();

  bool initialize_writes(uint64_t publish_time);

  void print_stats();
};

} // namespace Applications

#endif  // APPLICATIONS_KZH15_H

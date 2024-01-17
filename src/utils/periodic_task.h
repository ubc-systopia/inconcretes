#ifndef UTILS_PERIODIC_TASK_H
#define UTILS_PERIODIC_TASK_H

#include <limits>
#include <thread>
#include <vector>
#include <fstream>
#include "logging.h"
#include "periodic_timer.h"
#define MAX_NUM_JOBS 180000
namespace Utils {

void spin(uint64_t duration_ns);
void spin2(uint64_t duration_ns, TimeSpec& start_ts);

uint64_t flush_cache();

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
  std::string get_percentiles(std::string label = "");
};

class PeriodicTask {
 public:
  unsigned id;
  std::string id_str;
  uint64_t time_period_ns;
  uint64_t offset_ns;
  int prio;
  int cpu;
  PeriodicTimer *timer;
  std::thread context;
  log4cpp::Category *logger;

  ExecTimeProfile *job_profile = NULL;

  //std::vector<TimeSpec> vec_start_ts;
  //std::vector<TimeSpec> vec_finish_ts;

  TimeSpec task_start_ts;
  TimeSpec task_finish_ts;
  TimeSpec job_start_ts;
  TimeSpec job_finish_ts;

  double bcet_measured_ms = std::numeric_limits<double>::max();
  double acet_measured_ms = 0.0;
  double wcet_measured_ms = 0.0;

  uint64_t num_jobs = 0;

  bool collect_ets = false;
  double ets_ms[MAX_NUM_JOBS];

 public:
  PeriodicTask(unsigned id, uint64_t time_period_ns, uint64_t offset_ns,
               int prio, int cpu, log4cpp::Category *logger);

  void update_scheduling_preferences(int prio, int cpu);

  void release(TimeSpec* release_ts = NULL);
  void spawn(TimeSpec* release_ts = NULL);
  void join();

  //void flush_cache();
  void profiler(unsigned int max_iterations = 100);
  void flush_exec_times(std::string log_folder_path);

  uint64_t my_release_time();
  uint64_t my_release_time_without_offset();
  uint64_t others_release_time_without_offset(uint64_t others_period_ns);

  //void print_measurements(TimeSpec* release_ts);

  virtual void job() = 0;
  virtual void job_for_profiler() = 0;
  virtual bool terminate() = 0;
};

class SpinningPeriodicTask : public PeriodicTask {
 public:
  uint64_t wcet_ns;

  SpinningPeriodicTask(unsigned id, uint64_t time_period_ns, uint64_t offset_ns,
                       uint64_t wcet_ns, int prio, int cpu,
                       log4cpp::Category *logger)
    : PeriodicTask(id, time_period_ns, offset_ns, prio, cpu, logger),
      wcet_ns(wcet_ns) {}

  void job();
  void job_for_profiler();
};

} // namespace Utils

#endif  // UTILS_PERIODIC_TASK_H

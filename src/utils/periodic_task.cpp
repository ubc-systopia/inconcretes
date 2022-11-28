#include "periodic_task.h"

//#include <bsd/sys/time.h>
#include <cassert>
#include <cstring>

#include <algorithm>
#include <thread>
#include <vector>
#include <sstream>

#include "misc.h"
#include "macros.h"
#include "sched.h"
#include "logging.h"
#include "timespec.h"

namespace Utils {

log4cpp::Category *logger = NULL;

// Global variables, used to flush cache
// (https://stackoverflow.com/a/34461372/149138)
// As per Wikipedia (https://en.wikipedia.org/wiki/Raspberry_Pi),
// the Raspberry Pi 4 uses a Broadcom BCM2711 SoC with a 1.5 GHz 64-bit
// quad-core ARM Cortex-A72 processor, with 1 MB shared L2 cache.
const uint64_t llc_size_bytes = 1 * 1024 * 1024; // 1 MB
const uint64_t block_size = llc_size_bytes * 2;
long *block = new long[block_size]; // NOT FREEDED ANYWHERE!!!

void spin2(uint64_t duration_ns, TimeSpec& start_ts) {
  uint64_t elapsed_ns = 0;

  TimeSpec curr_ts, diff_ts;
  while(true) {
    clock_gettime(CLOCK_ID, &curr_ts);
    timespecsub(&curr_ts, &start_ts, &diff_ts);
    elapsed_ns = SEC_TO_NS(diff_ts.tv_sec) + diff_ts.tv_nsec;
    if (elapsed_ns >= duration_ns) {
      break;
    }
  }
}

void spin(uint64_t duration_ns) {
  TimeSpec start_ts;
  clock_gettime(CLOCK_ID, &start_ts);
  Utils::spin2(duration_ns, start_ts);
}

uint64_t flush_cache() {
  TimeSpec start_ts, end_ts, diff_ts;
  clock_gettime(CLOCK_ID, &start_ts);
  for(int i = 0; i < block_size; i++) {
    block[i] = rand();
  }
  clock_gettime(CLOCK_ID, &end_ts);
  timespecsub(&end_ts, &start_ts, &diff_ts);
  return SEC_TO_NS(diff_ts.tv_sec) + diff_ts.tv_nsec;
}

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
  std::cout << get_percentiles(label) << std::endl;
}

std::string ExecTimeProfile::get_percentiles(std::string label) {
  std::stringstream ss;
  ss << label << " profile "
     << "P00:" << exec_time_observed_P00 << "ns, "
     << "P25:" << exec_time_observed_P25 << "ns, "
     << "P50:" << exec_time_observed_P50 << "ns, "
     << "P75:" << exec_time_observed_P75 << "ns, "
     << "P90:" << exec_time_observed_P90 << "ns, "
     << "P99:" << exec_time_observed_P99 << "ns, "
     << "P100:" << exec_time_observed_P100 << "ns, "
     << "WCET (estimated):" << wcet_estimated << "ns = "
     << NS_TO_US(wcet_estimated) << "us = " << NS_TO_MS(wcet_estimated) << "ms";
  return ss.str();
}

PeriodicTask::PeriodicTask(unsigned id, uint64_t time_period_ns,
                           uint64_t offset_ns, int prio, int cpu,
                           log4cpp::Category *logger)
  : id(id),
    id_str(std::to_string(id) + "_"),
    time_period_ns(time_period_ns),
    offset_ns(offset_ns),
    prio(prio),
    cpu(cpu),
    timer(new PeriodicTimer(time_period_ns, offset_ns)),
    logger(logger) {
  assert(logger != NULL);
}

void PeriodicTask::update_scheduling_preferences(int prio, int cpu) {
  // TODO Add a check that the task is not yet released
  // TODO If the task is released, then we also need to call the system calls
  // for setting thr scheduling preferences, like we do in release()
  this->prio = prio;
  this->cpu = cpu;
}

void PeriodicTask::release(TimeSpec* release_ts) {
  set_my_sched_fifo_priority(prio);
  pin_me_to_core(cpu);

  std::stringstream ss;
  ss << "Task " << id << ": period " << NS_TO_MS(time_period_ns)
     << "ms, offset " << NS_TO_MS(offset_ns) << "ms, priority " << prio << ", CPU(s)";
  cpu_set_t cpu_set;
  sched_getaffinity(0, sizeof(cpu_set_t), &cpu_set);
  for (int i = 0; i < Utils::get_max_cpus(); i++) {
    if (CPU_ISSET(i, &cpu_set)) {
      ss << " " << i;
    }
  }
  LDEBUG << ss.str();

  TimeSpec diff_ts;
  timer->trigger(release_ts);

  clock_gettime(CLOCK_ID, &task_start_ts);
  while(!terminate()) {
    clock_gettime(CLOCK_ID, &job_start_ts);

    job();

    clock_gettime(CLOCK_ID, &job_finish_ts);
    timespecsub(&job_finish_ts, &job_start_ts, &diff_ts);
    acet_measured_ms = (acet_measured_ms * (num_jobs - 1) + TS_TO_MS(&diff_ts))
                       / (double)num_jobs;
    bcet_measured_ms = std::min(bcet_measured_ms, TS_TO_MS(&diff_ts));
    wcet_measured_ms = std::max(wcet_measured_ms, TS_TO_MS(&diff_ts));
    assert(num_jobs <= MAX_NUM_JOBS);

    if (collect_ets) {
      ets_ms[num_jobs - 1] = TS_TO_MS(&diff_ts);
    }

    timer->wait();
  }

  clock_gettime(CLOCK_ID, &task_finish_ts);
}

void PeriodicTask::spawn(TimeSpec* release_ts) {
  context = std::thread(&PeriodicTask::release, this, release_ts);

  // TODO Is this needed? Seems to block multiple spawns across tasks.
  // For now, I have moved this to a separate function that can be called from the outside.
  //context.join();
}

void PeriodicTask::join() {
  context.join();
}

//void PeriodicTask::flush_cache() {
//  uint64_t llc_size_bytes = 30 * 1024 * 1024;
//  uint64_t block_size_bytes = llc_size_bytes * 2;
//  char *block = (char *) malloc (block_size_bytes);
//  memset(block, 1, block_size_bytes);
//  for(int i = 0; i < block_size_bytes; i++) {
//    block[i] = rand();
//  }
//  free(block);
//}

void PeriodicTask::profiler(unsigned int max_iterations) {
  unsigned curr_iteration = 0;
  TimeSpec start_ts, end_ts, diff_ts;
  std::vector<uint64_t> job_exec_times;
  std::vector<uint64_t> flush_exec_times;

  while ((++curr_iteration) <= max_iterations) {
    // Flush cache and run job
    flush_exec_times.push_back(flush_cache());
    clock_gettime(CLOCK_ID, &start_ts);
    job_for_profiler();
    clock_gettime(CLOCK_ID, &end_ts);
    timespecsub(&end_ts, &start_ts, &diff_ts);
    job_exec_times.push_back(SEC_TO_NS(diff_ts.tv_sec) + diff_ts.tv_nsec);
  }

  //// Print flush execution time measurements
  ExecTimeProfile flush_profile(flush_exec_times);
  //flush_profile.print("CacheFlushJob");

  // Print job execution time measurements
  if (job_profile != NULL) {
    delete job_profile;
  }
  job_profile = new ExecTimeProfile(job_exec_times);
  job_profile->print("PeriodicJob");
}

uint64_t PeriodicTask::my_release_time() {
  return offset_ns + (((get_time_now_ns() - offset_ns) / time_period_ns) *
                      time_period_ns);
}

uint64_t PeriodicTask::my_release_time_without_offset() {
  return ((get_time_now_ns() / time_period_ns) * time_period_ns);
}

uint64_t PeriodicTask::others_release_time_without_offset(uint64_t others_period_ns) {
  return ((get_time_now_ns() / others_period_ns) * others_period_ns);
}

//void PeriodicTask::print_measurements(TimeSpec* release_ts) {
//  //TimeSpec start_ts, finish_ts, diff_ts;
//  //std::vector<uint64_t> ets;
//  //std::stringstream ss;
//  //ss << "Task " << id << " release times: ";
//  //for (uint64_t i = 0; i < timer->release_times.size(); i++) {
//  //  ss << timer->release_times[i] << "ns ";
//  //}
//  //ss << std::endl;
//  //ss << "Task " << id << " start and finish times: ";
//  //for (uint64_t i = 0; i < vec_start_ts.size(); i++) {
//  //  timespecsub(&vec_start_ts[i], release_ts, &start_ts);
//  //  timespecsub(&vec_finish_ts[i], release_ts, &finish_ts);
//  //  timespecsub(&finish_ts, &start_ts, &diff_ts);
//  //  ss << "(" << TS_TO_MS(&start_ts) << "ms, " << TS_TO_MS(&finish_ts) << "ms) ";
//  //  ets.push_back(TS_TO_NS(&diff_ts));
//  //}
//  //LDEBUG << ss.str();
//  //Utils::ExecTimeProfile profile(ets);
//  //LDEBUG << profile.get_percentiles("Task " + std::to_string(id) + " execution times");
//}

// Periodic task job that just sleeps for the specified WCET
//void SpinningPeriodicTask::job() {
//  TimeSpec start_ts, end_ts;
//  clock_gettime(CLOCK_ID, &start_ts);
//  end_ts.tv_sec = start_ts.tv_sec;
//  end_ts.tv_nsec = start_ts.tv_nsec + wcet_ns;
//  fix_ts(&end_ts);
//
//  int err = EINTR;
//  while (err != 0 and err == EINTR) {
//    err = clock_nanosleep(CLOCK_ID, TIMER_ABSTIME, &end_ts, nullptr);
//  }
//  if (err != 0) { perror("Error"); }
//}

// Periodic task job that actually spins for the specified WCET
void SpinningPeriodicTask::job() {
  spin(wcet_ns);
}

// Periodic task job that actually spins for the specified WCET
void SpinningPeriodicTask::job_for_profiler() {
  job();
}

} // namespace Utils

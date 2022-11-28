#include "periodic_timer.h"

#include <time.h>
//#include <bsd/sys/time.h>

#include <cassert>
#include <iostream>

#include "timespec.h"

namespace Utils {

//inline void add_ns_to_ts(TimeSpec *ts, uint64_t ns) {
//  // Translate ns into X seconds + Y nanoseconds (where Y < ONE_SEC_TO_NS)
//  // Add X and Y to ts->tv_sec and ts->tv_nsec, respectively
//  // Afterwards, check if ts->tv_nsec still carries over ONE_SEC_TO_NS,
//  // and correct if necessary
//  ts->tv_sec += ns / ONE_SEC_TO_NS;
//  ts->tv_nsec += ns % ONE_SEC_TO_NS;
//  if (ts->tv_nsec >= ONE_SEC_TO_NS) {
//    ts->tv_sec += 1;
//    ts->tv_nsec = ts->tv_nsec % ONE_SEC_TO_NS;
//  }
//}

void fix_ts(TimeSpec* ts) {
  ts->tv_sec += ts->tv_nsec / SEC_TO_NS(1);
  ts->tv_nsec = ts->tv_nsec % SEC_TO_NS(1);
}

void print_ts(TimeSpec* ts, std::string prefix) {
  std::cout << prefix << ": " << ts->tv_sec << "s, "
            << (ts->tv_nsec / MS_TO_NS(1)) << "ms, "
            << ((ts->tv_nsec % MS_TO_NS(1)) / US_TO_NS(1)) << "us, "
            << ((ts->tv_nsec % MS_TO_NS(1)) % US_TO_NS(1)) << "ns" << std::endl;
}

int wait_until_ts(TimeSpec* ts) {
  int err = EINTR;
  while (err != 0 and err == EINTR) {
    err = clock_nanosleep(CLOCK_ID, TIMER_ABSTIME, ts, nullptr);
  }
  if (err != 0) {
    perror("Error");
  }
  return err;
}

PeriodicTimer::PeriodicTimer(uint64_t time_period_ns)
  : PeriodicTimer(time_period_ns, 0) {}

PeriodicTimer::PeriodicTimer(uint64_t time_period_ns, uint64_t offset_ns)
  : time_period_ns(time_period_ns),
    offset_ns(offset_ns),
    last_inter_arrival_ns(0) {
  timespecclear(&last_activation_ts);
  clock_gettime(CLOCK_ID, &base_ts);
}

void PeriodicTimer::get_next_release(TimeSpec* now_ts, TimeSpec* next_ts,
                                     bool now_ts_provided) {
  if (now_ts_provided == false) {
    clock_gettime(CLOCK_ID, now_ts);
  }
  uint64_t total_ns = ((TS_TO_NS(now_ts) / time_period_ns) + 1) * time_period_ns
                      + offset_ns;
  next_ts->tv_sec = total_ns / SEC_TO_NS(1);
  next_ts->tv_nsec = total_ns % SEC_TO_NS(1);
  //release_times.push_back(total_ns);
}

void PeriodicTimer::wait() {
  TimeSpec now_ts, next_ts;
  get_next_release(&now_ts, &next_ts);
  wait_until_ts(&next_ts);
  update_stats();
}

void PeriodicTimer::trigger() {
  wait();
}

void PeriodicTimer::trigger(uint64_t delay_ns) {
  TimeSpec release_ts;
  clock_gettime(CLOCK_ID, &release_ts);
  release_ts.tv_nsec += delay_ns;
  fix_ts(&release_ts);
  wait_until_ts(&release_ts);
  wait();
}

void PeriodicTimer::trigger(TimeSpec* ts) {
  if (ts != NULL) {
    wait_until_ts(ts);
  }
  wait();
}

void PeriodicTimer::update_stats() {
  if (timespecisset(&last_activation_ts) == false) {  // First activation
    clock_gettime(CLOCK_ID, &last_activation_ts);
  } else {
    TimeSpec prev_activation_ts, diff_ts;
    prev_activation_ts.tv_sec = last_activation_ts.tv_sec;
    prev_activation_ts.tv_nsec = last_activation_ts.tv_nsec;
    clock_gettime(CLOCK_ID, &last_activation_ts);
    timespecsub(&last_activation_ts, &prev_activation_ts, &diff_ts);
    last_inter_arrival_ns = SEC_TO_NS(diff_ts.tv_sec) + diff_ts.tv_nsec;
  }
}

void PeriodicTimer::reset() {
  last_inter_arrival_ns = 0;
  timespecclear(&last_activation_ts);
  clock_gettime(CLOCK_ID, &base_ts);
}

void PeriodicTimer::print_relative_ts(TimeSpec* ts, std::string prefix) {
  TimeSpec relative_ts;
  timespecsub(ts, &base_ts, &relative_ts);
  print_ts(&relative_ts, prefix);
}

} // namespace Utils

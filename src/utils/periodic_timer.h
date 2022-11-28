#ifndef UTILS_PERIODIC_TIMER_H
#define UTILS_PERIODIC_TIMER_H

#include <time.h>

#include <iostream>

#include "macros.h"

namespace Utils {

//inline void add_ns_to_ts(TimeSpec *ts, uint64_t ns);
void fix_ts(TimeSpec* ts);
void print_ts(TimeSpec* ts, std::string prefix);
int wait_until_ts(TimeSpec* ts);

class PeriodicTimer {
 public:
  uint64_t time_period_ns;
  uint64_t offset_ns;
  uint64_t last_inter_arrival_ns;
  TimeSpec last_activation_ts;
  TimeSpec base_ts;

  //std::vector<uint64_t> release_times;

 public:
  PeriodicTimer(uint64_t time_period_ns);
  PeriodicTimer(uint64_t time_period_ns, uint64_t offset_ns);

  // Get the next release point after now_ts based on periodicity and offset
  // If now_ts_provided is true, now_ts must be computed by the caller
  void get_next_release(TimeSpec* now_ts, TimeSpec* next_ts,
                        bool now_ts_provided = false);

  void wait();  // Wait until the next release point

  void trigger();                  // Trigger at the next release point,
  void trigger(uint64_t delay_ns); // at the next release point after delay_ns,
  void trigger(TimeSpec* ts);      // at the next release point after time ts

  void update_stats();
  void reset();

  void print_relative_ts(TimeSpec* ts, std::string prefix);
};

} // namespace Utils

#endif  // UTILS_PERIODIC_TIMER_H

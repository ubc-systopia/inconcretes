#ifndef UTILS_SCHED_H
#define UTILS_SCHED_H

// Utilization bound for Uniprocessor Rate Monotonic scheduling
const double rm_uni_util_bound = 0.69;

void set_my_sched_fifo_priority(int prio);
//void set_my_sched_deadline_params(uint64_t time_period_ns,
//                                  uint64_t deadline_ns);
void pin_me_to_core(int id);

#endif  // UTILS_SCHED_H

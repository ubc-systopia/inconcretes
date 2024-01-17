#include "sched.h"

#include <stdio.h>
#include <sched.h>
#include <iostream>

void set_my_sched_fifo_priority(int prio) {
  struct sched_param param;
  param.sched_priority = prio;
  std::cout<<"prio: "<<prio<<std::endl;
  if (sched_setscheduler(0, SCHED_FIFO, &param) < 0) {
    perror("Error (sched_setscheduler)");
  }
}

//void set_my_sched_deadline_params(uint64_t time_period_ns,
//                                  uint64_t deadline_ns) {
//  struct sched_attr attr;
//  if (sched_getattr(0, &attr, sizeof(attr), 0) < 0) {
//    perror("Error (sched_getattr)");
//  }
//  attr.sched_policy = SCHED_DEADLINE;
//  attr.sched_runtime = time_period_ns;
//  attr.sched_deadline = deadline_ns;
//
//  if (sched_setattr(0, &attr, 0) < 0) {
//    perror("Error (sched_getattr)");
//  }
//}

void pin_me_to_core(int id) {
  cpu_set_t cpu_set;
  CPU_ZERO(&cpu_set);
  CPU_SET(id, &cpu_set);
  if (sched_setaffinity(0, sizeof(cpu_set_t), &cpu_set) < 0) {
    perror("Error (sched_setaffinity)");
  }
}

#ifndef APPLICATIONS_INVERTED_PENDULUM_SIM_H
#define APPLICATIONS_INVERTED_PENDULUM_SIM_H

#include <iostream>
#include <cmath>

#include "utils/misc.h"
#include "utils/timespec.h"
#include "utils/periodic_task.h"
#include "achal/simple_kvs.h"

#define MAX_AP_VALUE_LOGGED 72000

namespace Applications {

class IvPSim;

class Dynamics {
 public:
  IvPSim* task;

  //double mass;                // D:M
  //double cart_mass;           // D:CM
  //double rod_length;          // D:RL
  //double gravity;             // D:G
  //double lin_friction;        // D:LF
  //double ang_friction;        // D:AF
  //double current_force;       // D:CF
  //double previous_force;      // D:PF
  //double angular_position;    // D:AP
  //double angular_velocity;    // D:AV
  //double cart_position;       // D:CP
  //double cart_velocity;       // D:CV
  //double time_step;           // D:TS
  //double time_tag;            // D:TT

 public:
  Dynamics(IvPSim* task) : task(task) {
    //mass = 1.0;
    //cart_mass = 10.0;
    //rod_length = 1.0;
    //gravity = 9.8;
    //lin_friction = 0.0;  // Proportional to translational friction force
    //ang_friction = 1.0;  // Proportional to rotational friction force

    //current_force = 0.0;
    //previous_force = 0.0;
    //angular_position = 0.1;  // z1
    //angular_velocity = 0.0;  // z2
    //cart_position = 0.0;     // z3
    //cart_velocity = 0.0;     // z4

    //time_step = 0.05;
    //time_tag = 0.0;
  }

  //void setTimeStep(double ts) {
  //  time_step = ts;
  //}
  //void setTimeTag(double tt) {
  //  time_tag = tt;
  //}
  //void setCurrentForce(double force) {
  //  current_force = force;
  //}

  //double getTimeStep(void) {
  //  return time_step;
  //}
  //double getTimeTag(void) {
  //  return time_tag;
  //}

  double sqr(double value) {
    return value * value;
  }

  void integrateForwardEuler(void);
  void integrateForwardRungeKutta4(void);
  void integrateForwardEuler(double step);
  void integrateForwardRungeKutta4(double step);
};

class IvPController {
 public:
  IvPSim* task;

 public:
  IvPController(IvPSim* task);

  //void update_ivp_state(double angular_position, bool faulty);

  //double compute_actr_cmds();
  void compute_actr_cmds(double dt);
};

class IvPSim : public Utils::PeriodicTask {
 public:
  Achal::KVSInterface* kvs;

  //double kp = 1000; // I:KP
  //double kd = 100;  // I:KD
  //double ki = 100;  // I:KI

  //double pre_error = 0; // I:PE
  //double integral = 0;  // I:I

  uint64_t successful_batch_reads = 0;
  uint64_t successful_batch_writes = 0;
  uint64_t successful_reads = 0;
  uint64_t successful_writes = 0;
  uint64_t failed_batch_reads = 0;
  uint64_t failed_batch_writes = 0;
  uint64_t failed_reads = 0;
  uint64_t failed_writes = 0;

  Dynamics* env;
  IvPController* ctrl;

  double angular_position;
  double angular_positions[MAX_AP_VALUE_LOGGED] = { 0.0 };

  uint64_t max_jobs;
  uint64_t publishing_time;
  uint64_t published_time;

  log4cpp::Category *logger;

  std::map<std::string, std::string> state;

  double multiplier = 1.0;

 public:
  IvPSim(unsigned id, uint64_t time_period_ns, uint64_t offset_ns, int prio,
         int cpu, Achal::KVSInterface* kvs, uint64_t max_jobs,
         log4cpp::Category *logger);
  ~IvPSim();

  void job();

  void job_for_profiler() {
    job();
  }

  bool terminate() {
    return num_jobs >= max_jobs;
  }

  void print_stats();
  bool did_ivp_stabilize();
};

} // namespace Applications

#endif  // APPLICATIONS_INVERTED_PENDULUM_SIM_H

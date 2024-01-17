#ifndef APPLICATIONS_PHYSICAL_IVP_H
#define APPLICATIONS_PHYSICAL_IVP_H

#include "PIvPUtils/AS_5600.h"
#include "PIvPUtils/Ultrasonic.h"
#include "utils/periodic_task.h"
#include <cmath>
#include <iomanip>
#include <iostream>
#include <pigpio.h>
#include <queue>

#include "achal/simple_kvs.h"
#include "utils/misc.h"
#include "utils/timespec.h"

namespace Applications {
class PositionTask;
class AngleTask;

class PIDController {
public:
  PIDController(double Kp, double Kd, double Ki, double set_point,
                double derivative_multiplier, double integral_multiplier,
                int derivative_store_size);
  double compute_output(double value, double dt);
  void reset_control();
  double set_point = 0, Kp = 0.0, Kd = 0.0, Ki = 0.0, integral = 0.0,
         derivative = 0.0, prev_error = 0.0, prop_correction,
         derivative_correction, int_correction, derivative_multiplier = 1.0,
         integral_multiplier = 1.0;

  struct DataPoint {
    long long task_start_time;
    long long task_prev_end_time;
    long long timestamp;

    long long pid_compute_time;
    long long angle_rec_time;
    long long pos_filter_time;
    long long pos_measure_time;

    double position;
    double raw_angle;
    double filtered_position;
    double error;
    double prev_error;
    double derivative;
    double filtered_derivative;
    double integral;

    double prop_correction;
    double der_correction;
    double integral_correction;
    double output;
    double dt;
  };
  DataPoint data_point;
  vector<DataPoint> data_points;
  long long start_time = 0;
  long long end_time = 0;

private:
  int derivative_store_size = 1;
  queue<double> prev_derivatives, prev_values;
  bool med_filter, first_iter_flag;
  TimeSpec prev_ts;
  TimeSpec dt_ts;
  uint64_t dt_ns;
  double dt;
};

class PositionTask : public Utils::PeriodicTask {
public:
  string log_folder_path;
  Achal::KVSInterface *kvs;
  uint64_t successful_batch_reads = 0;
  uint64_t successful_batch_writes = 0;
  uint64_t successful_reads = 0;
  uint64_t successful_writes = 0;
  uint64_t failed_batch_reads = 0;
  uint64_t failed_batch_writes = 0;
  uint64_t failed_reads = 0;
  uint64_t failed_writes = 0;

  PIDController *position_controller;
  AngleTask *angle_task;

  uint64_t max_jobs;
  uint64_t publishing_time;
  uint64_t published_time;
  uint8_t node_id;

  std::atomic<double> current_position;
  std::atomic<double> ang_off;

  double set_pos = 0;

  // not sure what this is here for but it looks like a conversion factor for
  // timer
  double multiplier = 1.0;
  bool stop_flag = false;
  bool flushed = false;
  bool terminate_flag = false;

  log4cpp::Category *logger;

  std::map<std::string, std::string> state;

  PositionTask(uint8_t node_id, int motor_pin, int dir_pin, int us_trig,
               int us_echo, int lim_sw, double ang_Kp, double ang_Kd,
               double ang_Ki, int angle_set_point, double pos_Kp, double pos_Kd,
               double pos_Ki, double set_pos, unsigned int position_task_id,
               unsigned int angle_task_id, uint64_t time_period_ns_pos,
               uint64_t time_period_ns_ang, uint64_t offset_ns_pos,
               uint64_t offset_ns_ang, int pos_prio, int ang_prio, int cpu_pos,
               int cpu_ang, Achal::KVSInterface *kvs, uint64_t max_jobs,
               log4cpp::Category *logger);
  ~PositionTask();

  void job();
  void job_for_profiler();
  bool terminate() { return this->terminate_flag; }
  void limit_switch_ISR(int gpio, int level, uint32_t tick);
  void update_pid_params(double ang_Kp, double ang_Kd, double ang_Ki,
                         double pos_Kp, double pos_Kd, double pos_Ki);
  void reset_control();
  void flush_log();
};

class AngleTask : public Utils::PeriodicTask {
public:
  AS_5600 rotary_encoder;
  PIDController *angle_controller;
  PositionTask *position_task;
  Ultrasonic linear_position_encoder;
  queue<double> prev_positions;
  double prev_pos = 0;

  uint64_t max_jobs;
  uint64_t publishing_time;
  uint64_t published_time;
  uint8_t node_id;

  // not sure what this is here for but it looks like a conversion factor for
  // timer
  double multiplier = 1.0;
  bool stop_flag = false;
  bool flushed = false;
  bool terminate_flag = false;

  int motor_pin, dir_pin, lim_sw, us_trig, us_echo;

  log4cpp::Category *logger;

  std::map<std::string, std::string> state;

  AngleTask(PositionTask *position_task, uint8_t node_id, int motor_pin,
            int dir_pin, int lim_sw, int us_trig, int us_echo, double ang_Kp,
            double ang_Kd, double ang_Ki, int angle_set_point,
            unsigned int angle_task_id, uint64_t time_period_ns,
            uint64_t offset_ns, int prio, int cpu, uint64_t max_jobs,
            log4cpp::Category *logger);

  ~AngleTask();

  void job();
  void job_for_profiler();
  void reset_control();
  void flush_log(string log_folder_path);
  bool terminate() { return this->terminate_flag; }

  void output_to_motor(double output);
  void update_pid_params(double ang_Kp, double ang_Kd, double ang_Ki);
};

} // namespace Applications
#endif
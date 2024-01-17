#include "physical_ivp.h"
namespace Applications {
PIDController::PIDController(double Kp, double Kd, double Ki, double set_point,
                             double derivative_multiplier,
                             double integral_multiplier,
                             int derivative_store_size)
    : Kp(Kp), Kd(Kd), Ki(Ki), set_point(set_point),
      derivative_multiplier(derivative_multiplier),
      integral_multiplier(integral_multiplier),
      derivative_store_size(derivative_store_size) {
  std::cout << "done controller" << std::endl;
}

double PIDController::compute_output(double value, double dt) {
  // std::cout << "Controller Input: " << value << std::endl;

  double error = value - this->set_point;
  // std::cout<<"value: "<< value<< " | set point: " <<
  // this->set_point<<std::endl;
  this->prop_correction = Kp * error;
  this->integral += this->integral_multiplier * error * dt;
  double sum = 0.0;
  if (dt)
    this->derivative =
        this->derivative_multiplier * ((error - this->prev_error) / dt);
  data_point.derivative = this->derivative;
  this->prev_derivatives.push(this->derivative);

  if (this->prev_derivatives.size() > derivative_store_size)
    prev_derivatives.pop();

  for (int i = 0; i < prev_derivatives.size(); i++) {
    sum += this->prev_derivatives.front();
    this->prev_derivatives.push(this->prev_derivatives.front());
    this->prev_derivatives.pop();
  }

  this->derivative = sum / this->prev_derivatives.size();

  this->derivative_correction = this->Kd * this->derivative;
  this->int_correction = this->Ki * this->integral;
  data_point.prev_error = this->prev_error;
  this->prev_error = error;
  data_point.error = error;
  data_point.der_correction = this->derivative_correction;
  data_point.prop_correction = this->prop_correction;
  data_point.integral_correction = this->int_correction;
  data_point.dt = dt;
  data_point.filtered_derivative = this->derivative;
  data_point.output = this->prop_correction + this->derivative_correction +
                      this->int_correction;
  data_point.integral = this->integral;

  this->data_points.push_back(data_point);

  return data_point.output;
}

void PIDController::reset_control() {
  this->data_points.clear();
  this->integral = 0;
  while (!this->prev_derivatives.empty()) {
    this->prev_derivatives.pop();
  }

  this->prop_correction = 0;
  this->derivative_correction = 0;
  this->int_correction = 0;

  this->prev_error = 0;
}

PositionTask::PositionTask(uint8_t node_id, int motor_pin, int dir_pin,
                           int us_trig, int us_echo, int lim_sw, double ang_Kp,
                           double ang_Kd, double ang_Ki, int angle_set_point,
                           double pos_Kp, double pos_Kd, double pos_Ki,
                           double set_pos, unsigned int position_task_id,
                           unsigned int angle_task_id,
                           uint64_t time_period_ns_pos,
                           uint64_t time_period_ns_ang, uint64_t offset_ns_ang,
                           uint64_t offset_ns_pos, int pos_prio, int ang_prio,
                           int cpu_pos, int cpu_ang, Achal::KVSInterface *kvs,
                           uint64_t max_jobs, log4cpp::Category *logger)
    : Utils::PeriodicTask(position_task_id, time_period_ns_pos, offset_ns_pos,
                          pos_prio, cpu_pos, logger),
      kvs(kvs), max_jobs(max_jobs), publishing_time(0), published_time(0),
      set_pos(set_pos), node_id(node_id),

      position_controller(
          new PIDController(pos_Kp, pos_Kd, pos_Ki, set_pos, (1000.0), 1, 20)),
      logger(logger) {
  std::cout << "cpu ang: " << cpu_ang << std::endl;
  std::cout << "Pos Task Prio: " << pos_prio << " | Ang Task Prio " << ang_prio
            << std::endl;
  angle_task = new AngleTask(this, node_id, motor_pin, dir_pin, lim_sw, us_trig,
                             us_echo, ang_Kp, ang_Kd, ang_Ki, angle_set_point,
                             angle_task_id, time_period_ns_ang, offset_ns_ang,
                             ang_prio, cpu_ang, max_jobs, logger);
  current_position.store(set_pos);
  ang_off.store(0);

  switch (node_id) {
  case 1:
    state["angoff1"] = std::to_string(3);
    state["rawpos1"] = std::to_string(3);
    state["angoff2"] = "pass";
    state["rawpos2"] = "pass";
    state["angoff3"] = "pass";
    state["angoff4"] = "pass";
    break;
  case 2:
    state["angoff1"] = "pass";
    state["rawpos1"] = "pass";
    state["angoff2"] = std::to_string(3);
    state["rawpos2"] = std::to_string(3);
    state["angoff3"] = "pass";
    state["angoff4"] = "pass";
    break;

  case 3:
    state["angoff1"] = "pass";
    state["rawpos1"] = "pass";
    state["angoff2"] = "pass";
    state["rawpos2"] = "pass";
    state["angoff3"] = std::to_string(3);
    state["angoff4"] = "pass";
    break;
  case 4:
    state["angoff2"] = "pass";
    state["rawpos1"] = "pass";
    state["angoff1"] = "pass";
    state["rawpos2"] = "pass";
    state["angoff3"] = "pass";
    state["angoff4"] = 3;
    break;

  default:
    throw std::runtime_error("invalid node id");
  }
  state["ctrlout"] = std::to_string(3);
}

PositionTask::~PositionTask() {
  delete position_controller;
  delete angle_task;
}

void PositionTask::job() {
  // std::cout<<"Starting position job"<<std::endl;
  position_controller->data_point.timestamp =
      (std::chrono::duration_cast<std::chrono::milliseconds>(
           std::chrono::high_resolution_clock::now().time_since_epoch()))
          .count();
  position_controller->data_point.task_start_time = Utils::get_time_now_ns();
  publishing_time = my_release_time() + time_period_ns;
  published_time = my_release_time() - time_period_ns;
  // std::cout<< "current time: " <<Utils::get_time_now_ns() <<std::endl;
  // std::cout<<"publishing time: "<<publishing_time<<std::endl;
  // std::cout<<"starting read"<<std::endl;
  std::pair<uint64_t, uint64_t> read_status(0, 0);
  if (!stop_flag) {
    if (num_jobs == 0 || num_jobs == 1) {
      read_status.first = state.size();
      read_status.second = 0;
    } else {

      Achal::read_status status;
      // CHANGE TO ONLY READ NECESSARY VALUES
      for (auto it = state.begin(); it != state.end(); it++) {
        // std::cout<<"trying to read"<<std::endl;
        status = kvs->logging_read(it->first, published_time, it->second);
        if (status.status) {
          INC_SUCCESS(read_status);
        } else {
          INC_FAILURE(read_status);
        }
      }
    }
    // std::cout<<"done read"<<std::endl;
    // try_read

    // update stats
    successful_reads += GET_SUCCESS(read_status);
    failed_reads += GET_FAILURE(read_status);
    // std::cout<<"successful reads: " << successful_reads <<std::endl;
    // std::cout<<"failed reads: "<< failed_reads <<std::endl;

    if (GET_FAILURE(read_status) == 0) {
      successful_batch_reads++;
    } else {
      failed_batch_reads++;
    }

    switch (node_id) {
    case 1:
      state["angoff1"] = std::to_string(position_controller->compute_output(
          this->current_position.load(),
          NS_TO_MS(timer->last_inter_arrival_ns) * multiplier));
      this->ang_off.store(std::stod(this->state["angoff1"]));
      state["rawpos1"] = std::to_string(this->current_position.load());
      state["angoff2"] = "pass";
      state["rawpos2"] = "pass";
      state["angoff3"] = "pass";
      state["angoff4"] = "pass" ;
      break;
    case 2:
      state["rawpos2"] =
          std::to_string(45.0 - (4.6 + this->current_position.load()));

      try {
        this->current_position.store(std::stod(state["rawpos1"]));
      } catch (std::invalid_argument e) {
        std::cout << "Key \"rawpos1\" has an invalid value: "
                  << state["rawpos1"] << " cm" << std::endl;
        this->current_position.store(std::stod(state["rawpos2"]));
      }
      state["angoff2"] = std::to_string(position_controller->compute_output(
          this->current_position.load(),
          NS_TO_MS(timer->last_inter_arrival_ns) * multiplier));

      try {
        this->ang_off.store(std::stod(this->state["angoff1"]));
      } catch (std::invalid_argument e) {
        std::cout
            << "Value for key angoff1 could not be converted to double, value: "
            << state["angoff1"] << std::endl;
        this->ang_off.store(std::stod(this->state["angoff2"]));
      }

      state["rawpos1"] = "pass";
      state["angoff1"] = "pass";
      state["angoff3"] = "pass";
      state["angoff4"] = "pass";
      break;

    case 3:
      try {
        this->current_position.store(std::stod(state["rawpos1"]));
      } catch (std::invalid_argument e) {
        std::cout << "Key \"rawpos1\" has an invalid value: "
                  << state["rawpos1"] << " cm" << std::endl;
        try {
          this->current_position.store(std::stod(state["rawpos2"]));

        } catch (std::invalid_argument e) {
          std::cout << "Key \"rawpos2\" has an invalid value: "
                    << state["rawpos2"] << " cm" << std::endl;
          this->current_position.store(this->set_pos);
        }
      }

      state["angoff3"] = std::to_string(position_controller->compute_output(
          this->current_position.load(),
          NS_TO_MS(timer->last_inter_arrival_ns) * multiplier));
      try {
        this->ang_off.store(std::stod(this->state["angoff1"]));
      } catch (std::invalid_argument e) {
        std::cout
            << "Value for key angoff1 could not be converted to double, value: "
            << state["angoff1"] << std::endl;
        try {
          this->ang_off.store(std::stod(this->state["angoff2"]));
        } catch (std::invalid_argument e) {
          std::cout << "Value for key angoff2 could not be converted to "
                       "double, value: "
                    << state["angoff2"] << std::endl;
          this->ang_off.store(std::stod(this->state["angoff3"]));
        }
      }
      state["rawpos1"] = "pass";
      state["angoff1"] = "pass";
      state["angoff2"] = "pass";
      state["rawpos2"] = "pass";
      state["angoff4"] = "pass";
      break;
    case 4:
      try {
        this->current_position.store(std::stod(state["rawpos1"]));
      } catch (std::invalid_argument e) {
        std::cout << "Key \"rawpos1\" has an invalid value: "
                  << state["rawpos1"] << " cm" << std::endl;
        try {
          this->current_position.store(std::stod(state["rawpos2"]));

        } catch (std::invalid_argument e) {
          std::cout << "Key \"rawpos2\" has an invalid value: "
                    << state["rawpos2"] << " cm" << std::endl;
        }
      }

      state["angoff4"] = std::to_string(position_controller->compute_output(
          this->current_position.load(),
          NS_TO_MS(timer->last_inter_arrival_ns) * multiplier));
      try {
        this->ang_off.store(std::stod(this->state["angoff1"]));
      } catch (std::invalid_argument e) {
        std::cout
            << "Value for key angoff1 could not be converted to double, value: "
            << state["angoff1"] << std::endl;
        try {
          this->ang_off.store(std::stod(this->state["angoff2"]));
        } catch (std::invalid_argument e) {
          std::cout << "Value for key angoff2 could not be converted to "
                       "double, value: "
                    << state["angoff2"] << std::endl;
          try {
            this->ang_off.store(std::stod(this->state["angoff3"]));
          } catch (std::invalid_argument e) {
            std::cout << "Value for key angoff3 could not be converted to "
                         "double, value: "
                      << state["angoff3"] << std::endl;
            this->ang_off.store(std::stod(this->state["angoff4"]));
          }
        }
      }

      state["rawpos1"] = "pass";
      state["angoff1"] = "pass";
      state["angoff2"] = "pass";
      state["rawpos2"] = "pass";
      state["angoff3"] = "pass";
      break;

    default:
      throw std::runtime_error("invalid node id");
    }

    std::pair<uint64_t, uint64_t> write_status(0, 0);
    for (auto it = state.begin(); it != state.end(); it++) {
      if (kvs->try_write(it->first, publishing_time, it->second)) {
        INC_SUCCESS(write_status);
      } else {
        INC_FAILURE(write_status);
      }
    }

    // update stats
    successful_writes += GET_SUCCESS(write_status);
    failed_writes += GET_FAILURE(write_status);

    // std::cout<<"successful writes: " << successful_writes <<std::endl;
    // std::cout<<"failed writes: "<< failed_writes <<std::endl;

    if (GET_FAILURE(write_status) == 0) {
      successful_batch_writes++;
    } else {
      failed_batch_writes++;
    }
    num_jobs++;
  } else {
    if (!flushed) {
      this->flush_log();
      std::cout << "flushed logs" << std::endl;
      flushed = true;
    }
    this->angle_task->stop_flag = true;
    // std::cout<<"stopped"<<std::endl;
    // this_thread::sleep_for(2000ms);
    this->terminate_flag = true;
    this->kvs->terminate_flag = true;
  }
  position_controller->data_point.task_prev_end_time = Utils::get_time_now_ns();
}

void PositionTask::reset_control() {
  this->position_controller->reset_control();
  this->angle_task->reset_control();
  this->stop_flag = false;
  this->terminate_flag = false;
  this->kvs->terminate_flag = false;
  this->flushed = false;
  this->timer->reset();
  this->num_jobs = 0;
}

void PositionTask::job_for_profiler() { job(); }

void PositionTask::flush_log() {
  auto now = std::chrono::system_clock::now();
  std::time_t time = std::chrono::system_clock::to_time_t(now);
  std::stringstream ss;
  ss << "./PiCtlLogs/";
  ss << std::put_time(std::localtime(&time), "%Y-%m-%d_%H-%M-%S");
  ss << "_Pi" << std::to_string(node_id);

  if (!(mkdir(ss.str().c_str(), 0777))) {
    std::ofstream file(ss.str() + "/pos_ctrl_log.csv");
    std::cout << ss.str() << std::endl;
    if (!file.is_open()) {
      std::cerr << "" << endl;
      return;
    }

    file << "Pos K_p, " << this->position_controller->Kp << endl;
    file << "Pos K_d, " << this->position_controller->Kd << endl;
    file << "Pos K_i, " << this->position_controller->Ki << endl;
    file << "Set Position," << this->position_controller->set_point << endl;

    file << "Task Start Time (ns),Prev Task End Time (ns),Timestamp "
            "(ms),DT,Error,Control Signal,Prev Error,Prop Correction,Filtered "
            "Derivative,Derivative,Integral,Derivative Correction,Integral "
            "Correction"
         << endl;

    for (const auto &dataPoint : this->position_controller->data_points) {
      file << dataPoint.task_start_time << "," << dataPoint.task_prev_end_time
           << "," << dataPoint.timestamp << "," << dataPoint.dt << ","
           << dataPoint.error << "," << dataPoint.output << ","
           << dataPoint.prev_error << "," << dataPoint.prop_correction << ","
           << dataPoint.filtered_derivative << "," << dataPoint.derivative
           << "," << dataPoint.integral << "," << dataPoint.der_correction
           << "," << dataPoint.integral_correction << std::endl;
    }

    std::cout << "done flushing" << endl;
    file.close();
  } else {
    std::cout << "folder creation failed for log output" << std::endl;
  }
  this->position_controller->data_points.clear();
  this->angle_task->flush_log(ss.str());
  this->log_folder_path = ss.str();
  this->flush_exec_times(ss.str());
  return;
}

void PositionTask::update_pid_params(double ang_Kp, double ang_Kd,
                                     double ang_Ki, double pos_Kp,
                                     double pos_Kd, double pos_Ki) {
  this->position_controller->Kp = pos_Kp;
  this->position_controller->Kd = pos_Kd;
  this->position_controller->Ki = pos_Ki;
  this->angle_task->angle_controller->Kp = ang_Kp;
  this->angle_task->angle_controller->Kd = ang_Kd;
  this->angle_task->angle_controller->Ki = ang_Ki;
}

AngleTask::AngleTask(PositionTask *position_task, uint8_t node_id,
                     int motor_pin, int dir_pin, int lim_sw, int us_trig,
                     int us_echo, double ang_Kp, double ang_Kd, double ang_Ki,
                     int angle_set_point, unsigned int angle_task_id,
                     uint64_t time_period_ns, uint64_t offset_ns, int prio,
                     int cpu, uint64_t max_jobs, log4cpp::Category *logger)
    : Utils::PeriodicTask(angle_task_id, time_period_ns, offset_ns, prio, cpu,
                          logger),
      angle_controller(new PIDController(ang_Kp, ang_Kd, ang_Ki,
                                         static_cast<double>(angle_set_point) *
                                             (360.0 / 4096.0),
                                         200.0, 1.0 / 1000.0, 50)),
      node_id(node_id), motor_pin(motor_pin), dir_pin(dir_pin), lim_sw(lim_sw),
      position_task(position_task), linear_position_encoder(us_trig, us_echo) {

  std::cout << "gpioInitialise(): " << gpioInitialise() << std::endl;
  std::cout << "angle task cpu " << cpu << std::endl;
  std::cout << "angle task priority " << prio << std::endl;
  // this_thread::sleep_for(2000ms);
  gpioSetMode(this->motor_pin, PI_OUTPUT);
  gpioSetMode(this->dir_pin, PI_OUTPUT);
  gpioSetPWMfrequency(this->motor_pin, 2000);
}

AngleTask::~AngleTask() { delete this->angle_controller; }

void AngleTask::job() {
  angle_controller->data_point.timestamp =
      (std::chrono::duration_cast<std::chrono::milliseconds>(
           std::chrono::high_resolution_clock::now().time_since_epoch()))
          .count();
  this->angle_controller->data_point.task_start_time = Utils::get_time_now_ns();
  publishing_time = my_release_time() + time_period_ns;
  published_time = my_release_time() - time_period_ns;

  long long start_time;
  long long end_time;
  switch (node_id) {
  case 1:
  case 2:
    // read, filter and update position value for position task
    long long start_time = Utils::get_time_now_ns();
    double position = this->linear_position_encoder.detectDistance();
    long long end_time = Utils::get_time_now_ns();
    this->angle_controller->data_point.pos_measure_time = end_time - start_time;
    // double position = 2;
    // if ultrasonic errors out, then assume position is the same as previous
    start_time = Utils::get_time_now_ns();
    if (isnan(position)) {
      position = this->prev_pos;
    }
    this->angle_controller->data_point.position = position;

    // // keep only last 5 positions, got 5 as smallest possible range for an
    // effective filter from testing
    this->prev_positions.push(position);
    if (this->prev_positions.size() > 30)
      this->prev_positions.pop();

    // move all prev positions to a temporary list to be sorted
    vector<double> temp;
    for (int i = 0; i < this->prev_positions.size(); i++) {
      temp.push_back(this->prev_positions.front());
      this->prev_positions.push(this->prev_positions.front());
      this->prev_positions.pop();
    }
    std::sort(temp.begin(), temp.end());

    // find median val, take avg of 2 middle values in case of even sized array
    // (on startup)
    if (temp.size() % 2 == 0) {
      position = (temp[temp.size() / 2 - 1] + temp[temp.size() / 2]) / 2;
    } else {
      position = temp[temp.size() / 2];
    }

    position_task->current_position.store(position);
    // std::cout << "Position: " << position << std::endl;
    start_time = Utils::get_time_now_ns();
    this->angle_controller->data_point.filtered_position = position;
    end_time = Utils::get_time_now_ns();
    this->angle_controller->data_point.pos_filter_time = end_time - start_time;
    break;
  }

  // get angle for angle task
  start_time = Utils::get_time_now_ns();
  double raw_angle = this->rotary_encoder.getAngleUART();
  end_time = Utils::get_time_now_ns();
  this->angle_controller->data_point.angle_rec_time = end_time - start_time;
  this->angle_controller->data_point.raw_angle = raw_angle;
  // std::cout << "Raw Angle from UART: " << raw_angle << std::endl;
  // std::cout<<"angle_offset: "<< this->position_task->ang_off<<std::endl;
  if (!this->stop_flag) {
    this->angle_controller->data_point.pid_compute_time =
        this->angle_controller->end_time - this->angle_controller->start_time;
    this->angle_controller->start_time = Utils::get_time_now_ns();
    output_to_motor(this->angle_controller->compute_output(
        static_cast<double>(raw_angle * (360.0) / 4096) +
            this->position_task->ang_off.load(),
        NS_TO_MS(timer->last_inter_arrival_ns) * multiplier));
    this->angle_controller->end_time = Utils::get_time_now_ns();
  } else {
    output_to_motor(0);
    this->terminate_flag = true;
  }
  this->angle_controller->data_point.task_prev_end_time =
      Utils::get_time_now_ns();
  num_jobs++;
}

void AngleTask::reset_control() {
  while (!this->prev_positions.empty()) {
    this->prev_positions.pop();
  }

  this->angle_controller->reset_control();
  this->stop_flag = false;
  this->terminate_flag = false;
  this->flushed = false;
  this->timer->reset();
  this->num_jobs = 0;
}

void AngleTask::flush_log(string log_folder_path) {
  std::ofstream file(log_folder_path + "/ang_ctrl_log.csv");
  if (!file.is_open()) {
    std::cerr << "" << endl;
    return;
  }

  file << "Pos K_p, " << this->angle_controller->Kp << endl;
  file << "Pos K_d, " << this->angle_controller->Kd << endl;
  file << "Pos K_i, " << this->angle_controller->Ki << endl;
  file << "Set Position," << this->angle_controller->set_point << endl;

  file << "PID Compute Time,Angle Reception Time,Position Filtering "
          "Time,Position Measurement Time,Task Start Time (ns),Prev Task End "
          "Time (ns),Timestamp (ms),DT,Raw Angle,Filtered "
          "Position,Position,Error,Previous Error,Control Signal,Filtered "
          "Derivative,Derivative,Integral,Prop Correction,Derivative "
          "Correction,Integral Correction"
       << endl;

  for (const auto &dataPoint : this->angle_controller->data_points) {
    file << dataPoint.pid_compute_time << "," << dataPoint.angle_rec_time << ","
         << dataPoint.pos_filter_time << "," << dataPoint.pos_measure_time
         << "," << dataPoint.task_start_time << ","
         << dataPoint.task_prev_end_time << "," << dataPoint.timestamp << ","
         << dataPoint.dt << "," << dataPoint.raw_angle << ","
         << dataPoint.filtered_position << "," << dataPoint.position << ","
         << dataPoint.error << "," << dataPoint.prev_error << ","
         << dataPoint.output << "," << dataPoint.filtered_derivative << ","
         << dataPoint.derivative << "," << dataPoint.integral << ","
         << dataPoint.prop_correction << "," << dataPoint.der_correction << ","
         << dataPoint.integral_correction << std::endl;
  }

  std::cout << "done flushing angle" << endl;
  this->angle_controller->data_points.clear();
  file.close();
  this->flush_exec_times(log_folder_path);
}

void AngleTask::output_to_motor(double output) {
  this->position_task->state["ctrlout"] = std::to_string(output);
  gpioWrite(this->dir_pin, output > 0);
  output = abs(output) + 25;
  if (output > 255)
    output = 255;
  // std::cout<<"output: " << output<<std::endl;
  gpioPWM(this->motor_pin, output);
}

void AngleTask::job_for_profiler() { job(); }

} // namespace Applications
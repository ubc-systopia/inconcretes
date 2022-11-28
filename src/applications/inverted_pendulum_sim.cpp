#include "inverted_pendulum_sim.h"
#include "achal/simple_kvs.h"

namespace Applications {

void Dynamics::integrateForwardEuler(void) {
  integrateForwardEuler(std::stod(task->state["D:TS"]));
}

void Dynamics::integrateForwardRungeKutta4(void) {
  integrateForwardRungeKutta4(std::stod(task->state["D:TS"]));
}

void Dynamics::integrateForwardEuler(double step) {
  double m = std::stod(task->state["D:M"]);
  double M = std::stod(task->state["D:CM"]);
  double l = std::stod(task->state["D:RL"]);
  double f_lin = std::stod(task->state["D:LF"]);
  double f_ang = std::stod(task->state["D:AF"]);
  double c_f = std::stod(task->state["D:CF"]);
  double p_f = std::stod(task->state["D:PF"]);
  double g = std::stod(task->state["D:G"]);
  double a_p = std::stod(task->state["D:AP"]);
  double a_v = std::stod(task->state["D:AV"]);
  double c_p = std::stod(task->state["D:CP"]);
  double c_v = std::stod(task->state["D:CV"]);
  double h = step;

  double z1 = 0.0;
  double z2 = 0.0;
  double z3 = 0.0;
  double z4 = 0.0;

  z1 = a_p + h * a_v;

  z2 = a_v +
       h * (
         p_f * cos(a_p)
         - (M + m) * g * sin(a_p)
         + m * l * cos(a_p) * sin(a_p) * sqr(a_v)
         + f_lin * cos(a_p) * c_v
         + (M + m) * f_ang / m * a_v
       ) / (m * l * sqr(cos(a_p)) - (M + m) * l);

  z3 = c_p + h * c_v;

  z4 = c_v +
       h * (
         p_f
         + m * l * sin(a_p) * sqr(a_v)
         - m * g * cos(a_p) * sin(a_p)
         + cos(a_p) * f_ang * a_v
       ) / (M + m - m * sqr(cos(a_p)));

  task->state["D:AP"] = std::to_string(z1);
  task->state["D:AV"] = std::to_string(z2);
  task->state["D:CP"] = std::to_string(z3);
  task->state["D:CV"] = std::to_string(z4);

  task->state["D:PF"] = task->state["D:CF"];
  task->state["D:TT"] = std::to_string(std::stod(task->state["D:TT"]) +
                                       std::stod(task->state["D:TS"]));
}

void Dynamics::integrateForwardRungeKutta4(double step) {
  double K1 = 0.0f;
  double K2 = 0.0f;
  double K3 = 0.0f;
  double K4 = 0.0f;

  double L1 = 0.0f;
  double L2 = 0.0f;
  double L3 = 0.0f;
  double L4 = 0.0f;

  double M1 = 0.0f;
  double M2 = 0.0f;
  double M3 = 0.0f;
  double M4 = 0.0f;

  double N1 = 0.0f;
  double N2 = 0.0f;
  double N3 = 0.0f;
  double N4 = 0.0f;

  double m     = std::stod(task->state["D:M"]);
  double M     = std::stod(task->state["D:CM"]);
  double l     = std::stod(task->state["D:RL"]);
  double g     = std::stod(task->state["D:G"]);
  double f_lin = std::stod(task->state["D:LF"]);
  double f_ang = std::stod(task->state["D:AF"]);
  double c_f   = std::stod(task->state["D:CF"]);
  double p_f   = std::stod(task->state["D:PF"]);
  double a_p   = std::stod(task->state["D:AP"]);
  double a_v   = std::stod(task->state["D:AV"]);
  double c_p   = std::stod(task->state["D:CP"]);
  double c_v   = std::stod(task->state["D:CV"]);
  double h = step;

  double z1 = 0.0;
  double z2 = 0.0;
  double z3 = 0.0;
  double z4 = 0.0;

  K1 = std::stod(task->state["D:AV"]);

  L1 = (
         p_f * cos(a_p)
         - (M + m) * g * sin(a_p)
         + m * l * cos(a_p) * sin(a_p) * sqr(a_v)
         + f_lin * cos(a_p) * c_v
         + (M + m) * f_ang / m * a_v
       ) / (m * l * sqr(cos(a_p)) - (M + m) * l);

  M1 = c_v;

  N1 = (
         p_f
         + m * l * sin(a_p) * sqr(a_v)
         - m * g * cos(a_p) * sin(a_p)
         + cos(a_p) * f_ang * a_v
       ) / (M + m - m * sqr(cos(a_p)));

  K2 = a_v + h / 2.0 * L1;

  L2 = (
         (p_f + c_f) / 2.0 * cos(a_p + h / 2.0 * K1)
         - (M + m) * g * sin(a_p + h / 2.0 * K1)
         + m * l * cos(a_p + h / 2.0 * K1) * sin(a_p + h / 2.0
             * K1) * sqr(a_v + h / 2.0 * L1)
         + f_lin * cos(a_p + h / 2.0 * K1) * (c_v + h / 2.0 * N1)
         + (M + m) * f_ang / m * (a_v + h / 2.0 * L1)
       ) / (m * l * sqr(cos(a_p + h / 2.0 * K1)) - (M + m) * l);

  M2 = c_v + h / 2.0 * N1;

  N2 = (
         (p_f + c_f) / 2.0
         + m * l * sin(a_p + h / 2.0 * K1) * sqr(a_v + h / 2.0
             * L1)
         - m * g * cos(a_p + h / 2.0 * K1) * sin(a_p + h / 2.0
             * K1)
         + cos(a_p + h / 2.0 * K1) * f_ang * (a_v + h / 2.0 *
             L1)
       ) / (M + m - m * sqr(cos(a_p + h / 2.0 * K1)));

  K3 = a_v + h / 2.0 * L2;

  L3 = (
         (p_f + c_f) / 2.0 * cos(a_p + h / 2.0 * K2)
         - (M + m) * g * sin(a_p + h / 2.0 * K2)
         + m * l * cos(a_p + h / 2.0 * K2) * sin(a_p + h / 2.0
             * K2) * sqr(a_v + h / 2.0 * L2)
         + f_lin * cos(a_p + h / 2.0 * K2) * (c_v + h / 2.0 * N2)
         + (M + m) * f_ang/m * (a_v + h / 2.0 * L2)
       ) / (m * l * sqr(cos(a_p + h / 2.0 * K2)) - (M + m) * l);

  M3 = c_v + h / 2.0 * N2;

  N3 = (
         (p_f + c_f) / 2.0
         + m * l * sin(a_p
                       + h / 2.0 * K2) * sqr(a_v + h / 2.0 * L2)
         - m * g * cos(a_p + h / 2.0 * K2) * sin(a_p + h / 2.0
             * K2)
         + cos(a_p + h / 2.0 * K2) * f_ang * (a_v + h / 2.0 *
             L2)
       ) / (M + m - m*sqr(cos(a_p + h / 2.0 * K2)));

  K4 = a_v + h * L3;

  L4 = (
         p_f * cos(a_p + h * K3)
         - (M + m) * g * sin(a_p + h * K3)
         + m * l * cos(a_p + h * K3) * sin(a_p + h * K3) * sqr(
           a_v + h * L3)
         + f_lin * cos(a_p + h * K3) * (c_v + h * N3)
         + (M + m) * f_ang/m * (a_v + h * L3)
       ) / (m * l * sqr(cos(a_p + h * K3)) - (M + m) * l);

  M4 = c_v + h * N3;

  N4 = (
         p_f
         + m * l * sin(a_p + h * K3) * sqr(a_v + h * L3)
         - m * g * cos(a_p + h * K3) * sin(a_p + h * K3)
         + cos(a_p + h * K3) * f_ang * (a_v + h * L3)
       ) / (M + m - m*sqr(cos(a_p + h * K3)));

  z1 = a_p + h * (1.0 / 6.0 * K1 + 2.0 / 6.0 * K2 + 2.0 / 6.0 * K3 +
                               1.0 / 6.0 * K4);
  z2 = a_v + h * (1.0 / 6.0 * L1 + 2.0 / 6.0 * L2 + 2.0 / 6.0 * L3 +
                               1.0 / 6.0 * L4);
  z3 = c_p    + h * (1.0 / 6.0 * M1 + 2.0 / 6.0 * M2 + 2.0 / 6.0 * M3 +
                               1.0 / 6.0 * M4);
  z4 = c_v    + h * (1.0 / 6.0 * N1 + 2.0 / 6.0 * N2 + 2.0 / 6.0 * N3 +
                               1.0 / 6.0 * N4);

  task->state["D:AP"] = std::to_string(z1);
  task->state["D:AV"] = std::to_string(z2);
  task->state["D:CP"] = std::to_string(z3);
  task->state["D:CV"] = std::to_string(z4);

  task->state["D:PF"] = task->state["D:CF"];
  task->state["D:TT"] = std::to_string(std::stod(task->state["D:TT"]) +
                                       std::stod(task->state["D:TS"]));
}

IvPController::IvPController(IvPSim* task)
  : task(task) {
}

//void IvPController::update_ivp_state(double angular_position, bool faulty) {
//  // TODO Ignoring the "faulty" flag for now, update later for fault injection
//}

//double IvPController::compute_actr_cmds() {
//  return compute_actr_cmds(NS_TO_SEC(task->time_period_ns));
//}

void IvPController::compute_actr_cmds(double dt) {
  if (dt == 0) {
    dt = NS_TO_SEC(task->time_period_ns);
  }

  // From previous iteration
  double pre_error = std::stod(task->state["I:PE"]);
  double integral = std::stod(task->state["I:I"]);
  double k_p = std::stod(task->state["I:KP"]);
  double k_d = std::stod(task->state["I:KD"]);
  double k_i = std::stod(task->state["I:KI"]);
  double a_p = std::stod(task->state["D:AP"]);

  double error = a_p - 0; // Calculate error
  double pout = k_p * error;            // Calculate proportional error

  integral += error * dt;                    // Calculate integral
  double iout = k_i * integral;         // Calculate proportional integral

  double derivative = (error - pre_error) / dt;   // Calculate derivative
  double dout = k_d * derivative;            // Calculate proportional derivate

  double force = pout + iout + dout;         // Calculate total force (output)
  pre_error = error;                         // Save error for future iterations

  // For actuation
  task->state["D:CF"] = std::to_string(force);
}

IvPSim::IvPSim(unsigned id, uint64_t time_period_ns, uint64_t offset_ns,
               int prio, int cpu,  Achal::KVSInterface* kvs, uint64_t max_jobs,
               log4cpp::Category *logger)
  : Utils::PeriodicTask(id, time_period_ns, offset_ns, prio, cpu, logger),
    kvs(kvs),
    max_jobs(max_jobs),
    publishing_time(0),
    published_time(0),
    env(new Dynamics(this)),
    ctrl(new IvPController(this)),
    logger(logger) {

  assert(max_jobs <= MAX_AP_VALUE_LOGGED);
  LDEBUG << "IvPSim " << id << " mapped to BFTKVS " << kvs->id;

  state["I:PE"] = std::to_string(0);
  state["I:I"]  = std::to_string(0);
  state["I:KP"] = std::to_string(125); // 1000
  state["I:KI"] = std::to_string(25);  // 100
  state["I:KD"] = std::to_string(1);   // 100
  state["D:M"]  = std::to_string(1);
  state["D:CM"] = std::to_string(10);
  state["D:RL"] = std::to_string(1);
  state["D:G"]  = std::to_string(9.8);
  state["D:LF"] = std::to_string(0);
  state["D:AF"] = std::to_string(1);
  state["D:CF"] = std::to_string(0);
  state["D:PF"] = std::to_string(0);
  state["D:AP"] = std::to_string(0.1);
  state["D:AV"] = std::to_string(0);
  state["D:CP"] = std::to_string(0);
  state["D:CV"] = std::to_string(0);
  state["D:TS"] = std::to_string(0.05);
  state["D:TT"] = std::to_string(0);

  //state["D:TS"] = std::to_string(NS_TO_SEC(time_period_ns));

  //for (auto it = state.begin(); it != state.end(); it++) {
  //  assert(kvs->try_write(it->first, Utils::get_time_now_ns() + SEC_TO_NS(1), it->second));
  //}

  //uint64_t initial_delay_s = 2;
  //std::this_thread::sleep_for(std::chrono::seconds(initial_delay_s));

  multiplier = std::stod(state["D:TS"]) / NS_TO_SEC(time_period_ns);
}

IvPSim::~IvPSim() {
  delete env;
  delete ctrl;
}

void IvPSim::job() {
  publishing_time = my_release_time() + time_period_ns;
  published_time = my_release_time() - time_period_ns;

  // KVS Read
  std::pair<uint64_t, uint64_t> read_status(0, 0);
  if (num_jobs == 0) {
    read_status.first = state.size();
    read_status.second = 0;
  } else {
    for (auto it = state.begin(); it != state.end(); it++) {
      if (kvs->try_read(it->first, published_time, it->second)) {
          INC_SUCCESS(read_status);
      } else {
          INC_FAILURE(read_status);
      }
    }
  }

  // Update stats
  successful_reads += GET_SUCCESS(read_status);
  failed_reads += GET_FAILURE(read_status);

  if (GET_FAILURE(read_status) == 0) {
    //LDEBUG << "successful READS";
    successful_batch_reads++;
  } else {
    //LDEBUG << num_jobs << ": FAILED READS ????????????????";
    failed_batch_reads++;
  }

  // Simulate
  env->integrateForwardRungeKutta4();
  angular_positions[num_jobs] = std::stod(state["D:AP"]);

  // Sense, compute, and actuate
  ctrl->compute_actr_cmds(NS_TO_SEC(timer->last_inter_arrival_ns) * multiplier);

  // KVS Write
  std::pair<uint64_t, uint64_t> write_status(0, 0);
  for (auto it = state.begin(); it != state.end(); it++) {
    if (kvs->try_write(it->first, publishing_time, it->second)) {
      INC_SUCCESS(write_status);
    } else {
      INC_FAILURE(write_status);
    }
  }

  // Update stats
  successful_writes += GET_SUCCESS(write_status);
  failed_writes += GET_FAILURE(write_status);

  if (GET_FAILURE(write_status) == 0) {
    //LDEBUG << "successful WRITES";
    successful_batch_writes++;
  } else {
    //LDEBUG << "FAILED WRITES ????????????????";
    failed_batch_writes++;
  }

  num_jobs++;
}

void IvPSim::print_stats() {
  TimeSpec diff_ts;
  timespecsub(&task_finish_ts, &task_start_ts, &diff_ts);
  LDEBUG << "IvPSim Task " << id << " actual (expected) duration: "
         << TS_TO_MS(&diff_ts) << "ms (" << max_jobs * NS_TO_MS(time_period_ns)
         << "ms), BCET (measured): " << bcet_measured_ms
         << "ms, ACET (measured): " << acet_measured_ms
         << "ms, WCET (measured): " << wcet_measured_ms << "ms";
  LDEBUG << "Task " << id << " stats: Reads: " << successful_reads
         << "S, " << failed_reads << "F, " << successful_batch_reads << "BS, "
         << failed_batch_reads << "BF; Writes: " << successful_writes
         << "S, " << failed_writes << "F, " << successful_batch_writes << "BS, "
         << failed_batch_writes << "BF";
}

bool IvPSim::did_ivp_stabilize() {
  for (unsigned i = num_jobs - 10; i < num_jobs; i++) {
    if (angular_positions[i] > 1e-05 or angular_positions[i] < -1e-05) {
      return false;
    }
  }
  return true;
}

} // namespace Applications

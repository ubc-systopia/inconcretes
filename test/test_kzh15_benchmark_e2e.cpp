#include "catch.hpp"

#include "main.h"
#include "utils/misc.h"
#include "utils/macros.h"
#include "utils/periodic_timer.h"
#include "utils/kzh15_benchmark_generator.h"
#include "applications/kzh15.h"
#include "achal/bft_kvs.h"


// TEST_CASE("Test KZH15 benchmark end-to-end 1", "[KZH15_E2E]") {

//   uint64_t smallest_period_ns = MS_TO_NS(10);

//   LDEBUG << "Initializing constants";
//   uint64_t test_duration_ns = SEC_TO_NS(10);
//   unsigned int max_cpus = Utils::get_max_cpus();
//   unsigned int cpus_used = max_cpus - 1;
//   uint64_t kvs_default_period_ns = smallest_period_ns;
//   uint64_t kvs_default_offset_ns = smallest_period_ns / 2;
//   uint64_t kvs_default_priority = 1;
//   uint64_t kvs_default_nw_delay_ns = US_TO_NS(100);
//   unsigned kvs_default_port = 8084;

//   LDEBUG << "Initializing TCP nodes";
//   std::vector<Achal::TCP::Node> nodes;
//   for (int i = 0; i < cpus_used; i++) {
//     nodes.push_back(Achal::TCP::Node{"127.0.0.1",
//                                      std::to_string(kvs_default_port + i)});
//     LDEBUG << "Node #" << i << " " << std::string(nodes.back());
//   }

//   LDEBUG << "Generating KVS instances";
//   std::vector<Achal::BFTKVS *> kvs_instances;
//   for (int i = 0; i < cpus_used; i++) {
//     std::vector<Achal::TCP::Node> kvs_peers(nodes);
//     kvs_peers.erase(kvs_peers.begin() + i);
//     unsigned kvs_id = 1000 * (i + 1);
//     Achal::BFTKVS* kvs_instance = new Achal::BFTKVS(
//       kvs_id,
//       kvs_default_period_ns,
//       kvs_default_offset_ns,
//       kvs_default_priority,
//       i,
//       test_duration_ns/kvs_default_period_ns,
//       kvs_default_port + i,
//       kvs_peers,
//       kvs_default_nw_delay_ns,
//       1,
//       Achal::fuse_default,
//       logger);
//     kvs_instances.push_back(kvs_instance);
//   }

//   double scale = 0.025;
//   Utils::KZH15::Config* cfg;
//   Utils::KZH15::BenchmarkInstance* instance;
//   LDEBUG << "Generating a KZH15 benchmark instance of scale" << scale;
//   while (1) {
//     cfg = new Utils::KZH15::Config(scale);
//     instance = new Utils::KZH15::BenchmarkInstance(*cfg, logger, true);
//     double utilization = instance->get_total_utilization();
//     LDEBUG << "Benchmark instance itilization = " << 100 * utilization << "%";
//     if (utilization <= 0.5) {
//       break;
//     }
//     delete instance;
//     delete cfg;
//     LDEBUG << "Trying again since utilization was in excess of 50%";
//   }
//   instance->print_summary();

//   LDEBUG << "Map the benchmark instance to a periodic task set";
//   uint64_t benchmark_default_offset_ns = MS_TO_NS(0);
//   uint64_t benchmark_default_priority = 25;
//   std::vector<Applications::KZH15Task*> tasks;
//   for (int cpu = 0; cpu < cpus_used; cpu++) {
//     double cpu_utilization = 0.0;
//     for (Utils::KZH15::Task* t : instance->get_tasks()) {
//       uint64_t period_ms =
//         Utils::KZH15::Runnable::get_min_period_ms(t->period_ms);
//       uint64_t period_ns = MS_TO_NS(period_ms);
//       unsigned task_id = 1000 * (cpu + 1) + t->id + 1;
//       if (period_ns < smallest_period_ns) {
//         continue;
//       }
//       //LDEBUG << "Generating periodic task " << task_id << " with time period "
//       //       << period_ms << "ms for CPU" << cpu;
//       uint64_t priority;
//       if (period_ns <= MS_TO_NS(1)) {
//         priority = 25;
//       } else if (period_ns <= MS_TO_NS(2)) {
//         priority = 24;
//       } else if (period_ns <= MS_TO_NS(5)) {
//         priority = 23;
//       } else if (period_ns <= MS_TO_NS(10)) {
//         priority = 22;
//       } else if (period_ns <= MS_TO_NS(20)) {
//         priority = 21;
//       } else if (period_ns <= MS_TO_NS(50)) {
//         priority = 20;
//       } else if (period_ns <= MS_TO_NS(100)) {
//         priority = 19;
//       } else if (period_ns <= MS_TO_NS(200)) {
//         priority = 18;
//       } else {
//         priority = 17;
//       }
//       Applications::KZH15Task* periodic_task =
//         new Applications::KZH15Task(
//         task_id,
//         period_ns,
//         benchmark_default_offset_ns,
//         priority,
//         cpu,
//         kvs_instances[cpu],
//         test_duration_ns/period_ns,
//         t,
//         logger);
//       tasks.push_back(periodic_task);
//       //if (cpu == 0) {
//       //  periodic_task->profiler(100);
//       //  REQUIRE(periodic_task->job_profile->wcet_estimated <=
//       //          periodic_task->time_period_ns);
//       //  double task_utilization = periodic_task->job_profile->wcet_estimated /
//       //                            (double)periodic_task->time_period_ns;
//       //  cpu_utilization += task_utilization;
//       //  LDEBUG << "Task utilization " << task_utilization * 100 << "%, CPU"
//       //         << cpu << " utilization " << cpu_utilization * 100 << "%";
//       //}
//     }
//   }

//   //// By default, all KVS instances are assigned to CPU 0
//   //// By default, all benchmark tasks are assigned to CPU 0
//   //// Assign any one KVS instance to the benchmark periodic tasks
//   //// Profile each benchmark periodic task to update their WCETs
//   //for (Applications::KZH15Task* t : tasks()) {
//   //  t->kvs = kvs_instances[0];
//   //  t->profiler(100);
//   //  REQUIRE(t->wcet_ns <= t->period_ns);
//   //}

//   LDEBUG << "Release all tasks";

//   TimeSpec release_ts;
//   clock_gettime(CLOCK_ID, &release_ts);
//   release_ts.tv_sec += 5;
//   release_ts.tv_nsec = 0;
//   Utils::fix_ts(&release_ts);

//   for (int i = 0; i < kvs_instances.size(); i++) {
//     kvs_instances[i]->spawn(&release_ts);
//   }

//   for (int i = 0; i < tasks.size(); i++) {
//     tasks[i]->spawn(&release_ts);
//   }

//   for (int i = 0; i < tasks.size(); i++) {
//     tasks[i]->join();
//   }

//   for (int i = 0; i < kvs_instances.size(); i++) {
//     kvs_instances[i]->join();
//   }

//   for (int i = 0; i < kvs_instances.size(); i++) {
//     //if (i > 0) { break; }
//     kvs_instances[i]->print_stats();
//     //kvs_instances[i]->print_measurements(&release_ts);
//   }

//   for (int i = 0; i < tasks.size(); i++) {
//     //if (i >= 5) { break; }
//     tasks[i]->print_stats();
//     //tasks[i]->print_measurements(&release_ts);
//   }
// }

TEST_CASE("Test KZH15 benchmark end-to-end 2", "[KZH15_E2E_2]") {

  uint64_t smallest_period_ms = 100;
  uint64_t smallest_period_ns = MS_TO_NS(smallest_period_ms);

  LDEBUG << "Initializing constants";
  uint64_t test_duration_ns = SEC_TO_NS(10);
  unsigned int max_cpus = Utils::get_max_cpus();
  unsigned int cpus_used = max_cpus - 1;
  uint64_t kvs_default_period_ns = smallest_period_ns;
  uint64_t kvs_default_offset_ns = smallest_period_ns / 2;
  uint64_t kvs_default_priority = 1;
  uint64_t kvs_default_nw_delay_ns = US_TO_NS(100);
  unsigned kvs_default_port = 8084;
  unsigned kvs_default_num_rounds = 2;

  LDEBUG << "Initializing config for the KVS instances";
  Achal::config_t config;
  config.period_ns = kvs_default_period_ns;
  config.offset_ns = kvs_default_offset_ns;
  config.priority = kvs_default_priority;
  config.logger = logger;
  const char *ports[] = {"8080", "8081", "8082", "8083"};
  for (int i = 0; i < cpus_used; i++) {
    Achal::process_t p;
    p.id = i;
    p.ip = "127.0.0.1";
    p.port = ports[i];
    config.peers.push_back(p);
  }
  config.max_network_delay_ns = kvs_default_nw_delay_ns;
  config.max_rounds = kvs_default_num_rounds;
  config.max_jobs = test_duration_ns / config.period_ns;

  LDEBUG << "Generating KVS instances";
  std::vector<Achal::BFTKVS<256, 7, 15>*> kvs_instances;
  for (int i = 0; i < cpus_used; i++) {
    config.port = 8080 + i;
    config.id = 1000 * (i + 1);
    config.cpu = i;
    config.my_process_id = i;
    kvs_instances.push_back(new Achal::BFTKVS<256, 7, 15>(config));
  }

  for (auto s : kvs_instances) {
    s->connect_to_servers();
  }

  for (auto s : kvs_instances) {
    s->accept_clients();
  }

  std::string config_file = Utils::get_project_directory() +
                            "/config/benchmarks/kzh15_42pcu_1031l.cfg";
  Utils::KZH15::BenchmarkInstanceBasic instance(config_file, logger);

  std::set<int> labels_to_be_deleted;
  for (Utils::KZH15::RunnableBasic* r : instance.runnables) {
    if (r->period_ms < smallest_period_ms) {
      for (int lid : r->rw_inter_task_labels_written) {
        labels_to_be_deleted.insert(lid);
        LDEBUG << "Erase all reads of label " << lid;
      }
    }
  }
  for (Utils::KZH15::RunnableBasic* r : instance.runnables) {
    for (int i = 0; i < r->rw_inter_task_labels_read.size(); i++) {
      int lid = r->rw_inter_task_labels_read[i];
      if (labels_to_be_deleted.find(lid) != labels_to_be_deleted.end()) {
        r->rw_inter_task_labels_read.erase(r->rw_inter_task_labels_read.begin() + i);
        LDEBUG << "Erasing label " << lid << " from runnable " << r->id << "'s "
               << "rw_inter_task_labels_read list";
      }
    }
  }

  LDEBUG << "Map the benchmark instance to a periodic task set";
  uint64_t benchmark_default_offset_ns = MS_TO_NS(0);
  uint64_t benchmark_default_priority = 25;
  std::vector<Applications::KZH15Task*> tasks;
  for (int cpu = 0; cpu < cpus_used; cpu++) {
    double cpu_utilization = 0.0;
    for (Utils::KZH15::TaskBasic* t : instance.tasks) {
      uint64_t period_ms = t->period_ms;
      uint64_t period_ns = MS_TO_NS(period_ms);
      unsigned task_id = 1000 * (cpu + 1) + t->id + 1;
      if (period_ns < smallest_period_ns) {
        continue;
      }
      //LDEBUG << "Generating periodic task " << task_id << " with time period "
      //       << period_ms << "ms for CPU" << cpu;
      uint64_t priority;
      if (period_ns <= MS_TO_NS(1)) {
        priority = 25;
      } else if (period_ns <= MS_TO_NS(2)) {
        priority = 24;
      } else if (period_ns <= MS_TO_NS(5)) {
        priority = 23;
      } else if (period_ns <= MS_TO_NS(10)) {
        priority = 22;
      } else if (period_ns <= MS_TO_NS(20)) {
        priority = 21;
      } else if (period_ns <= MS_TO_NS(50)) {
        priority = 20;
      } else if (period_ns <= MS_TO_NS(100)) {
        priority = 19;
      } else if (period_ns <= MS_TO_NS(200)) {
        priority = 18;
      } else {
        priority = 17;
      }
      Applications::KZH15Task* periodic_task =
        new Applications::KZH15Task(
        task_id,
        period_ns,
        benchmark_default_offset_ns,
        priority,
        cpu,
        kvs_instances[cpu],
        test_duration_ns/period_ns,
        t,
        instance.label_sizes,
        4,
        logger);
      tasks.push_back(periodic_task);
      //if (cpu == 0) {
      //  periodic_task->profiler(100);
      //  REQUIRE(periodic_task->job_profile->wcet_estimated <=
      //          periodic_task->time_period_ns);
      //  double task_utilization = periodic_task->job_profile->wcet_estimated /
      //                            (double)periodic_task->time_period_ns;
      //  cpu_utilization += task_utilization;
      //  LDEBUG << "Task utilization " << task_utilization * 100 << "%, CPU"
      //         << cpu << " utilization " << cpu_utilization * 100 << "%";
      //}
    }
  }

  //// By default, all KVS instances are assigned to CPU 0
  //// By default, all benchmark tasks are assigned to CPU 0
  //// Assign any one KVS instance to the benchmark periodic tasks
  //// Profile each benchmark periodic task to update their WCETs
  //for (Applications::KZH15Task* t : tasks()) {
  //  t->kvs = kvs_instances[0];
  //  t->profiler(100);
  //  REQUIRE(t->wcet_ns <= t->period_ns);
  //}

  LDEBUG << "Release all tasks";

  TimeSpec release_ts;
  clock_gettime(CLOCK_ID, &release_ts);
  release_ts.tv_sec += 5;
  release_ts.tv_nsec = 0;
  Utils::fix_ts(&release_ts);

  for (int i = 0; i < kvs_instances.size(); i++) {
    kvs_instances[i]->spawn(&release_ts);
  }

  for (int i = 0; i < tasks.size(); i++) {
    tasks[i]->spawn(&release_ts);
  }

  for (int i = 0; i < tasks.size(); i++) {
    tasks[i]->join();
  }

  for (int i = 0; i < kvs_instances.size(); i++) {
    kvs_instances[i]->join();
  }

  for (int i = 0; i < kvs_instances.size(); i++) {
    //if (i > 0) { break; }
    kvs_instances[i]->print_stats();
    //kvs_instances[i]->print_measurements(&release_ts);
  }

  for (int i = 0; i < tasks.size(); i++) {
    //if (i >= 5) { break; }
    tasks[i]->print_stats();
    //tasks[i]->print_measurements(&release_ts);
  }
}

// GENERATE CONFIG
// Start with a random scale parameter x
// Generate a random config using x

// SKIP THIS PART, FOR NOW !!!
// Find out the total number of labels in the config, say kMaxLabels
// Assume a kMaxInstancesPerLabel, one instance for every timestamp (e.g., 10)
// Profile the read and write latency of the preferred hash map used by our KVS
// Based on this profiling, estimate kKVSMaxReadTimeUs, kKVSMaxWriteTimeUs

// GENERATE PERIODIC TASKS FOR BENCHMARK
// Generate a benchmark instance from the config (DO NOT merge runnables)
// Generate a periodic task set from the benchmark instance
// Generate an Achal instance for m - 1 cores as a low frequency periodic task

// PROFILING
// Profile all benchmark tasks and update their WCETs
// Tighten Achal instances?

// PARTITION PERIODIC TASKS
// Map each Achal instance to its respective core
// Partition remaining tasks onto the m - 1 cores
// Do a response-time analysis to ensure schedulability
// Repeat partitioning if needed!
// If possible, merge tasks mapped to the same core and with the same period

// DEPLOY
// Spawn all Achal instances
// Spawn all tasks
// Wait ...
// Join
// Check data ...

// TODO
// In kzh15_benchmark_generator.cpp,
// in BenchmarkInstance::BenchmarkInstance(...),
// Shouldn't generate_task_instances_from_runnables() be called after
// map_labels_to_runnables()?

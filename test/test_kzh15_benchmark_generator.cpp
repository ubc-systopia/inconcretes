#include "catch.hpp"

#include <stdio.h>
#include <cstdio>
#include <random>
#include <map>
#include <unordered_map>

//#include "bytell_hash_map.hpp" // Compilation error on Raspberry Pi
//#include "flat_hash_map.hpp" // Compilation error on Raspberry Pi
#include "absl/container/flat_hash_map.h"
#include "absl/container/node_hash_map.h"
//#include "tsl/robin_map.h"
//#include "tsl/hopscotch_map.h"
//#include "tsl/array_map.h"

#include "main.h"
#include "utils/misc.h"
#include "utils/sched.h"
#include "utils/logging.h"
#include "utils/timespec.h"
#include "utils/kzh15_benchmark_generator.h"
#include "applications/kzh15.h"

//TEST_CASE("KZH15 config file parser", "[KZH15]") {
//  Utils::KZH15::Config cfg;
//  //cfg.print();
//}
//
//TEST_CASE("KZH15 benchmark generator", "[KZH15]") {
//  Utils::KZH15::Config cfg(1);
//  Utils::KZH15::BenchmarkInstance instance(cfg);
//  //instance.print_summary();
//}

template <typename KVSType>
void profile_hash_map(std::string kvs_type,
                      std::vector<Utils::KZH15::Label*> labels,
                      unsigned int max_iterations) {
  KVSType kvs;
  std::vector<std::string> labels_local;
  for (Utils::KZH15::Label* l : labels) {
    labels_local.push_back(std::string(l->size_bytes, 'a'));
    kvs[std::to_string(l->id)] = std::string(l->size_bytes, 'a');
  }

  unsigned working_set_size = labels.size() / 10;

  std::mt19937 gen;
  std::uniform_int_distribution<> uni_int_dist(0, labels.size() - 1);
  std::vector<int> lids;
  std::vector<std::string> lid_strs;
  for (int i = 0; i < working_set_size; i++) {
    int id = uni_int_dist(gen);
    lids.push_back(id);
    lid_strs.push_back(std::to_string(id));
  }

  unsigned curr_iteration = 0;
  TimeSpec start_ts, end_ts, diff_ts;
  std::vector<uint64_t> kvs_read_ets;
  while ((++curr_iteration) <= max_iterations) {
    Utils::flush_cache();

    clock_gettime(CLOCK_ID, &start_ts);
    for (int i = 0; i < lids.size(); i++) {
      labels_local[lids[i]] = kvs[lid_strs[i]];
    }
    clock_gettime(CLOCK_ID, &end_ts);
    timespecsub(&end_ts, &start_ts, &diff_ts);
    kvs_read_ets.push_back(SEC_TO_NS(diff_ts.tv_sec) + diff_ts.tv_nsec);
  }

  Utils::ExecTimeProfile read_profile(kvs_read_ets);
  read_profile.print(kvs_type + + " read profile: ");

  for (int i = 0; i > labels.size(); i++) {
    labels_local[i] = std::string(labels[i]->size_bytes, 'b');
  }

  lids.clear();
  lid_strs.clear();
  for (int i = 0; i < working_set_size; i++) {
    int id = uni_int_dist(gen);
    lids.push_back(id);
    lid_strs.push_back(std::to_string(id));
  }

  curr_iteration = 0;
  std::vector<uint64_t> kvs_write_ets;
  while ((++curr_iteration) <= max_iterations) {
    Utils::flush_cache();

    clock_gettime(CLOCK_ID, &start_ts);
    for (int i = 0; i < lids.size(); i++) {
      kvs[lid_strs[i]] = labels_local[lids[i]];
    }
    clock_gettime(CLOCK_ID, &end_ts);
    timespecsub(&end_ts, &start_ts, &diff_ts);
    kvs_write_ets.push_back(SEC_TO_NS(diff_ts.tv_sec) + diff_ts.tv_nsec);
  }

  Utils::ExecTimeProfile write_profile(kvs_write_ets);
  write_profile.print(kvs_type + + " write profile: ");
}

TEST_CASE("KVS Benchmark for a KZH15 benchmark instance", "[KZH15]") {
  Utils::KZH15::Config cfg(1);
  LDEBUG << std::string(cfg);
  Utils::KZH15::BenchmarkInstance instance(cfg, logger);

  unsigned int max_iterations = 100;
  profile_hash_map<std::map<std::string, std::string>>(
        "C++ STL's ordered map", instance.get_labels(), max_iterations);
  profile_hash_map<std::unordered_map<std::string, std::string>>(
        "C++ STL's unordered map", instance.get_labels(), max_iterations);
  profile_hash_map<absl::node_hash_map<std::string, std::string>>(
        "Abseil's node hash map", instance.get_labels(), max_iterations);
  profile_hash_map<absl::flat_hash_map<std::string, std::string>>(
        "Abseil's flat hash map", instance.get_labels(), max_iterations);
  //profile_hash_map<ska::flat_hash_map<std::string, std::string>>(
  //  "Skarupke's flat hash map", instance.get_labels(), max_iterations);
  //profile_hash_map<ska::bytell_hash_map<std::string, std::string>>(
  //  "Skarupke's bytell hash map", instance.get_labels(), max_iterations);
  //profile_hash_map<tsl::robin_map<std::string, std::string>>(
  //      "Tessil's robin hash map", instance.get_labels(), max_iterations);
  //profile_hash_map<tsl::hopscotch_map<std::string, std::string>>(
  //      "Tessil's hopscotch hash map", instance.get_labels(), max_iterations);
  //profile_hash_map<tsl::array_map<std::string, std::string>>(
  //  "Tessil's array hash map", instance.get_labels(), max_iterations);
}

//TEST_CASE("Periodic task set based on a KZH15 benchmark instance", "[KZH15]") {
//  Utils::KZH15::Config cfg(0.1);
//  cfg.print();
//  Utils::KZH15::BenchmarkInstance instance(cfg);
//  instance.print_summary();
//  Achal::KVS kvs;
//  const unsigned int max_cpus = Utils::get_max_cpus();
//  LDEBUG << "Max CPUs = " << max_cpus;
//  unsigned int cpu = 0;
//  double utilization = 0;
//  uint64_t duration_ms = SEC_TO_MS(60);
//  uint64_t offset_ns = 0;
//  std::vector<Applications::KZH15Task*> tasks;
//  for (Utils::KZH15::Task* t : instance.get_tasks()) {
//    int period_ms = t->get_min_period_ms();
//    uint64_t period_ns = MS_TO_NS(period_ms);
//    if (utilization + t->get_utilization() > Utils::KZH15::kMaxUtilPerCPU) {
//      cpu++;
//      assert(cpu < max_cpus);
//      utilization = 0;
//      offset_ns = 0;
//    }
//    uint64_t max_jobs = duration_ms / period_ms;
//    utilization += t->get_utilization();
//    LDEBUG << "CPU " << cpu << " utilization increased to " << utilization;
//    Applications::KZH15Task* task =
//      new Applications::KZH15Task(t->id, period_ns, MS_TO_NS(0), 1,
//          cpu, &kvs, max_jobs, t, logger);
//    tasks.push_back(task);
//
//    // TODO The offset currently does not take into account KVS access times,
//    // but only the application logic specific spin times.
//    // When the task is constructed above, these could be profiled and added
//    // as a parameter value to the task.
//    offset_ns += task->get_wcet_ns();
//  }
//
//  for (Applications::KZH15Task* task : tasks) {
//    task->spawn();
//  }
//
//  for (Applications::KZH15Task* task : tasks) {
//    task->join();
//  }
//}

//TEST_CASE("Binpacking KZH15 benchmark instance", "[KZH15]") {
//  Utils::KZH15::Config cfg(0.1);
//  cfg.print();
//  Utils::KZH15::BenchmarkInstance instance(cfg);
//  instance.print_summary();
//  Achal::KVS kvs;
//  const unsigned int max_cpus = Utils::get_max_cpus();
//  LDEBUG << "Max CPUs = " << max_cpus;
//
//  std::map<unsigned, double> task_utilizations;
//  for (Utils::KZH15::Task* t : instance.get_tasks()) {
//    REQUIRE(task_utilizations.find(t->id) == task_utilizations.end());
//    task_utilizations[t->id] = t->get_utilization();
//  }
//
//  BinPack binpack(task_utilizations, max_cpus, rm_uni_util_bound);
//  REQUIRE(binpack.any_fit());
//
//  uint64_t duration_ms = SEC_TO_MS(60);
//  std::vector<Applications::KZH15Task*> tasks;
//  for (Utils::KZH15::Task* t : instance.get_tasks()) {
//    int period_ms = t->get_min_period_ms();
//    uint64_t period_ns = MS_TO_NS(period_ms);
//    uint64_t max_jobs = duration_ms / period_ms;
//    REQUIRE(mapping.find(t->id) == mapping.end());
//    Applications::KZH15Task* task =
//      new Applications::KZH15Task(t->id, period_ns, MS_TO_NS(0), 1,
//                                             mapping[t->id], &kvs, max_jobs, t,
//                                             logger);
//    tasks.push_back(task);
//  }
//
//  for (Applications::KZH15Task* task : tasks) {
//    task->spawn();
//  }
//
//  for (Applications::KZH15Task* task : tasks) {
//    task->join();
//  }
//}

TEST_CASE("Export/Import for a KZH15 benchmark instance", "[KZH15_EXPORT]") {
  Utils::KZH15::Config cfg(0.01);
  LDEBUG << std::string(cfg);
  Utils::KZH15::BenchmarkInstance instance(cfg, logger);
  instance.print_summary();
  LDEBUG << "Exporting to file";
  std::string config_file = instance.export_to_file();
  LDEBUG << "Exported to " << config_file;
  LDEBUG << "Importing from " << config_file;
  Utils::KZH15::BenchmarkInstanceBasic basic_instance(config_file, logger);
  LDEBUG << "Imported from " << config_file;
  basic_instance.print_summary();
  LDEBUG << "Remove " << config_file;
  remove(config_file.c_str());
}

TEST_CASE("Generate random KZH15 benchmark instances", "[KZH15_GEN]") {
  uint64_t smallest_period_ms = 50;
  unsigned max_label_size = 16;
  for (int i = 0; i < 10; i++) {
    for (double scale = 0.01; scale <= 0.16; scale += 0.01) {
      LDEBUG << "Iteration " << i << ", scale " << scale;
      Utils::KZH15::Config cfg(scale);
      Utils::KZH15::BenchmarkInstance instance(cfg, logger);
      instance.print_summary();
      std::string config_file = instance.export_to_file();
      LDEBUG << "Exported to " << config_file;
      Utils::KZH15::BenchmarkInstanceBasic benchmark_instance(config_file, logger);
      Utils::KZH15::resolve_label_dependencies(benchmark_instance, smallest_period_ms);
      Utils::KZH15::trim_benchmark(benchmark_instance, smallest_period_ms);
      Utils::KZH15::shrink_label_sizes(benchmark_instance, max_label_size);
      benchmark_instance.print_summary();
      //LDEBUG << "Remove " << config_file;
      //remove(config_file.c_str());
    }
  }
}

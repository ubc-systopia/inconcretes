#include "catch.hpp"

#include <exception>
#include <unordered_set>
#include <sw/redis++/redis++.h>

#include "main.h"
#include "utils/misc.h"
#include "utils/macros.h"
#include "utils/kzh15_benchmark_generator.h"
#include "applications/kzh15.h"
#include "achal/redis_kvs.h"

using namespace sw::redis;

TEST_CASE("Redis Getting Started Guide", "[Redis]") {
  try {
    // Create an Redis object, which is movable but NOT copyable.
    auto redis = Redis("tcp://127.0.0.1:6379");

    // ***** STRING commands *****

    redis.set("key", "val");
    auto val = redis.get("key");
    REQUIRE(*val == "val");

    // ***** LIST commands *****

    // std::vector<std::string> to Redis LIST.
    std::vector<std::string> vec = {"a", "b", "c"};
    redis.rpush("list", vec.begin(), vec.end());

    // Redis LIST to std::vector<std::string>.
    vec.clear();
    redis.lrange("list", 0, -1, std::back_inserter(vec));
    REQUIRE(vec[0] == "a");
    REQUIRE(vec[1] == "b");
    REQUIRE(vec[2] == "c");

    // std::initializer_list to Redis LIST.
    redis.rpush("list", {"a", "b", "c"});

    // Redis LIST to std::vector<std::string>.
    vec.clear();
    redis.lrange("list", 0, -1, std::back_inserter(vec));
    REQUIRE(vec[0] == "a");
    REQUIRE(vec[1] == "b");
    REQUIRE(vec[2] == "c");

    // ***** HASH commands *****

    // std::unordered_map<std::string, std::string> to Redis HASH.
    std::unordered_map<std::string, std::string> m = {
      {"field1", "val1"},
      {"field2", "val2"}
    };
    redis.hmset("hash", m.begin(), m.end());

    // Redis HASH to std::unordered_map<std::string, std::string>.
    m.clear();
    redis.hgetall("hash", std::inserter(m, m.begin()));

    REQUIRE(m["field1"] == "val1");
    REQUIRE(m["field2"] == "val2");

    // Get value only.
    // NOTE: since field might NOT exist, so we need to parse it to OptionalString.
    std::vector<OptionalString> vals;
    redis.hmget("hash", {"field1", "field2"}, std::back_inserter(vals));

    REQUIRE(vals[0] == "val1");
    REQUIRE(vals[1] == "val2");

    // ***** SORTED SET commands *****

    redis.zadd("sorted_set", "m1", 1.3);

    // std::unordered_map<std::string, double> to Redis SORTED SET.
    std::unordered_map<std::string, double> scores = {
      {"m2", 2.3},
      {"m3", 4.5}
    };
    redis.zadd("sorted_set", scores.begin(), scores.end());

    // Redis SORTED SET to std::vector<std::pair<std::string, double>>.
    // NOTE: The return results of zrangebyscore are ordered, if you save the results
    // in to `std::unordered_map<std::string, double>`, you'll lose the order.
    std::vector<std::pair<std::string, double>> zset_result;
    redis.zrangebyscore("sorted_set",
                        UnboundedInterval<double> {},           // (-inf, +inf)
                        std::back_inserter(zset_result));

    REQUIRE((zset_result[0].first == "m1" and zset_result[0].second == 1.3));
    REQUIRE((zset_result[1].first == "m2" and zset_result[1].second == 2.3));
    REQUIRE((zset_result[2].first == "m3" and zset_result[2].second == 4.5));

    // Only get member names:
    // pass an inserter of std::vector<std::string> type as output parameter.
    std::vector<std::string> without_score;
    redis.zrangebyscore("sorted_set",
                        BoundedInterval<double>(2.3, 4.5, BoundType::CLOSED),   // [2.3, 4.5]
                        std::back_inserter(without_score));

    REQUIRE(without_score[0] == "m2");
    REQUIRE(without_score[1] == "m3");

    // Get both member names and scores:
    // pass an back_inserter of std::vector<std::pair<std::string, double>> as output parameter.
    std::vector<std::pair<std::string, double>> with_score;
    redis.zrangebyscore("sorted_set",
                        BoundedInterval<double>(2.3, 4.5, BoundType::LEFT_OPEN),    // (2.3, 4.5]
                        std::back_inserter(with_score));

    REQUIRE((with_score[0].first == "m3" and with_score[0].second == 4.5));
  } catch (std::exception& e) {
    REQUIRE(std::string(e.what()) ==
            std::string("Failed to connect to Redis: Connection refused"));
  }
}

TEST_CASE("Test KZH15 benchmark end-to-end with Redis", "[Redis]") {

  uint64_t smallest_period_ms = 100;
  uint64_t smallest_period_ns = MS_TO_NS(smallest_period_ms);

  LDEBUG << "Initializing constants";
  uint64_t test_duration_ns = SEC_TO_NS(10);
  unsigned int max_cpus = Utils::get_max_cpus();
  unsigned int cpus_used = max_cpus - 1;
  uint64_t kvs_default_period_ns = smallest_period_ns;
  uint64_t kvs_default_offset_ns = smallest_period_ns / 2;
  uint64_t kvs_default_priority = 1;

  LDEBUG << "Generating KVS instances";
  std::vector<Achal::RedisKVS1 *> kvs_instances;
  for (int i = 0; i < cpus_used; i++) {
    unsigned kvs_id = 1000 * (i + 1);
    unsigned port = 7000 + i;
    std::string uri = std::string("tcp://127.0.0.1:") + std::to_string(port);
    Achal::RedisKVS1* kvs_instance = new Achal::RedisKVS1(kvs_id,
        kvs_default_period_ns,
        kvs_default_offset_ns,
        kvs_default_priority,
        i,
        test_duration_ns/kvs_default_period_ns,
        port,
        logger);
    kvs_instances.push_back(kvs_instance);
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

  std::vector<std::string> label_keys;
  for (const auto &item : instance.label_sizes) {
    label_keys.push_back(std::to_string(item.first));
  }
  for (Achal::RedisKVS1* kvs_instance : kvs_instances) {
    kvs_instance->add_new_keys(label_keys);
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

  for (int i = 0; i < tasks.size(); i++) {
    tasks[i]->spawn(&release_ts);
  }

  for (int i = 0; i < tasks.size(); i++) {
    tasks[i]->join();
  }

  for (int i = 0; i < kvs_instances.size(); i++) {
    //if (i > 0) { break; }
    //kvs_instances[i]->print_stats();
    //kvs_instances[i]->print_measurements(&release_ts);
  }

  for (int i = 0; i < tasks.size(); i++) {
    //if (i >= 5) { break; }
    tasks[i]->print_stats();
    //tasks[i]->print_measurements(&release_ts);
  }
}

TEST_CASE("Test KZH15 benchmark end-to-end with RedisKVS2", "[Redis2]") {

  uint64_t smallest_period_ms = 20;
  uint64_t smallest_period_ns = MS_TO_NS(smallest_period_ms);

  LDEBUG << "Initializing constants";
  uint64_t test_duration_ns = SEC_TO_NS(10);
  unsigned int max_cpus = Utils::get_max_cpus();
  unsigned int cpus_used = max_cpus - 1;
  uint64_t kvs_default_period_ns = smallest_period_ns;
  uint64_t kvs_default_offset_ns = smallest_period_ns / 2;
  uint64_t kvs_default_priority = 1;

  LDEBUG << "Generating KVS instances";
  std::vector<Achal::RedisKVS2*> kvs_instances;
  for (int i = 0; i < cpus_used; i++) {
    unsigned kvs_id = 1000 * (i + 1);
    unsigned port = 7000 + i;
    std::string uri = std::string("tcp://127.0.0.1:") + std::to_string(port);

    std::vector<Achal::TCP::Node> peers;
    for (int j = 0; j < cpus_used; j++) {
      if (j == i) {
        continue;
      }
      Achal::TCP::Node peer = {"127.0.0.1", std::to_string(7000 + j)};
      peers.push_back(peer);
    }

    Achal::RedisKVS2* kvs_instance = new Achal::RedisKVS2(
        kvs_id,
        kvs_default_period_ns,
        kvs_default_offset_ns,
        kvs_default_priority,
        i,
        test_duration_ns/kvs_default_period_ns,
        port,
        peers,
        logger);
    kvs_instances.push_back(kvs_instance);
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

  std::vector<std::string> label_keys;
  for (const auto &item : instance.label_sizes) {
    label_keys.push_back(std::to_string(item.first));
  }
  for (Achal::RedisKVS2* kvs_instance : kvs_instances) {
    kvs_instance->add_new_keys(label_keys);
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

  for (int i = 0; i < tasks.size(); i++) {
    tasks[i]->spawn(&release_ts);
  }

  for (int i = 0; i < tasks.size(); i++) {
    tasks[i]->join();
  }

  for (int i = 0; i < kvs_instances.size(); i++) {
    //if (i > 0) { break; }
    //kvs_instances[i]->print_stats();
    //kvs_instances[i]->print_measurements(&release_ts);
  }

  for (int i = 0; i < tasks.size(); i++) {
    //if (i >= 5) { break; }
    tasks[i]->print_stats();
    //tasks[i]->print_measurements(&release_ts);
  }
}

using namespace std::chrono_literals;

TEST_CASE("Redis KVS2 Basic", "[redis_kvs2]") {
    std::vector<Achal::RedisKVS2 *> kvs;
    for (int me = 0; me < 4; me++) {
        std::vector<Achal::TCP::Node> peers_minus_self;
        for (int peer = 0; peer < 4; peer++) {
            if (peer == me) {
                continue;
            }
            peers_minus_self.push_back(
                Achal::TCP::Node{"127.0.0.1", std::to_string(8080 + peer)});
        }
        auto temp = new Achal::RedisKVS2(me, SEC_TO_NS(1), 0, 1, me, 10, 8080 + me,
                                   peers_minus_self, logger);
        temp->update_fuse_function();
        kvs.push_back(temp);
    }

    uint64_t now = Utils::get_time_now_ns();
    uint64_t last_sec = now - (now % SEC_TO_NS(1));

    uint64_t no_earlier_than = last_sec - SEC_TO_NS(1);
    uint64_t pt1 = last_sec + SEC_TO_NS(2);
    uint64_t pt2 = last_sec + SEC_TO_NS(6);

    std::string value;

    for (int me = 0; me < 4; me++) {
        // write 2 version of values, one after 2 sec and one after 6 sec
        REQUIRE(kvs[me]->try_write("kvs_test_key", pt1, "10.0") == true);
        REQUIRE(kvs[me]->try_write("kvs_test_key", pt2, "20.0") == true);

        // immediate read should fail
        REQUIRE(kvs[me]->try_read("kvs_test_key", no_earlier_than, value) ==
                false);
    }

    // after 4 seconds, read should succeed and should get 10 (published after 2
    // sec)
    std::this_thread::sleep_for(4s);

    for (int me = 0; me < 4; me++) {
        REQUIRE(kvs[me]->try_read("kvs_test_key", no_earlier_than, value) ==
                true);
        REQUIRE(std::stod(value) == 10.0);
    }

    // after 4 + 4 = 8 seconds, read should succeed and should get 20 (published
    // after 6 sec)
    std::this_thread::sleep_for(4s);

    for (int me = 0; me < 4; me++) {
        REQUIRE(kvs[me]->try_read("kvs_test_key", no_earlier_than, value) == true);
        REQUIRE(std::stod(value) == 20.0);
    }

    for(auto k : kvs){
        delete k;
    }
}


#include <iostream>
#include <vector>

#include "catch.hpp"
#include "main.h"

#include "utils/misc.h"
#include "utils/macros.h"
#include "achal/distributed_kvs_backed_periodic_task.h"

#define TIMES 10
#define PERIOD MS_TO_NS((uint64_t)2000)
#define KV_STORE_OFFSET MS_TO_NS((uint64_t)1000)
#define KV_STORE_DELAY MS_TO_NS((uint64_t)300)

class TestDistributedPeriodicTask : public
  Achal::DistributedKVSBacked::PeriodicTask {
 public:
  using Achal::DistributedKVSBacked::PeriodicTask::PeriodicTask;
  int my_dice;
  int times = TIMES;

  void job() {
    if (times == TIMES) {
      return;
    }
    uint64_t now = Utils::get_time_now_ns();
    uint64_t publish_time = now - now % PERIOD + PERIOD;
    std::string value = "good";
    if (times % 4 == my_dice) {
      value = "bad";
    }
    REQUIRE(kvs_try_write_ss("some_key", publish_time, value) == true);
  }

  void job_for_profiler() {
    job();
  }

  void report() {
    sync_kvs->print();
  }

  void assert_has_value() {
    std::string value;
    uint64_t now = Utils::get_time_now_ns();
    uint64_t publish_time = now - now % PERIOD - PERIOD * TIMES;
    REQUIRE(kvs_try_read_ss("some_key", publish_time, value) == true);
    REQUIRE(value == "good");
  }

  bool terminate() {
    return (times--) <= 0;
  }
};

std::string fuse(std::vector<std::string> values) {
  std::string majority = "";
  Achal::simpleMajority(values, majority);
  return majority;
}

#define DEFAULT_PORT 8084

TEST_CASE("Distributed Periodic Task", "[distributed]") {

  std::vector<Achal::BFTKVS *> kv_stores;
  for (int i = 0; i < 4; i++) {
    std::vector<Achal::TCP::Node> peers;
    for (int j = 0; j < 4; j++) {
      if (i == j) {
        continue;
      }
      int port = DEFAULT_PORT + j;
      peers.push_back(Achal::TCP::Node{"127.0.0.1", std::to_string(port)});
    }

    int port = DEFAULT_PORT + i;
    int id = DEFAULT_PORT + i;
    auto kvs = new Achal::BFTKVS(id, PERIOD, KV_STORE_OFFSET, 1, i, 10, port, peers,
                                 KV_STORE_DELAY, 2,fuse, logger);
    kv_stores.push_back(kvs);
  }

  for (int i = 0; i < 4; i++) {
    kv_stores[i]->connect_to_peer_server_sockets();
  }
  for(int i = 0; i < 4; i++) {
    kv_stores[i]->accept_peer_client_sockets();
  }

  std::vector<TestDistributedPeriodicTask *> tasks;
  for (int i = 0; i < 4; i++) {
    int id = i;
    auto task = new TestDistributedPeriodicTask(kv_stores[i], id, PERIOD, 0, 1, i,
        logger);
    task->my_dice = i;
    tasks.push_back(task);
  }

  for (int i = 0; i < 4; i++) {
    kv_stores[i]->spawn();
    tasks[i]->spawn();
  }

  for (int i = 0; i < 4; i++) {
    tasks[i]->join();
  }

  for (auto task : tasks) {
    task->report();
  }

  for (auto task : tasks) {
    task->assert_has_value();
    // delete task;
  }

  // for (auto kvs : kv_stores) {
  //   delete kvs;
  // }
}

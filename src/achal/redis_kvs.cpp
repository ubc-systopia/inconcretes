#include "achal/redis_kvs.h"

#include <sstream>

#include "utils/misc.h"
#include "utils/timespec.h"
#include "achal/kvs_interface.h"

namespace Achal {

sw::redis::Redis* RedisKVS1::get_redis_client(std::string uri) {
  sw::redis::Redis* redis = NULL;

  try {
    LDEBUG << "Connecting to Redis server at " << uri;
    redis = new sw::redis::Redis(uri);
    redis->set("test_key", "test_val");
    auto val = redis->get("test_key");
    assert(val);
    assert(*val == "test_val");
    redis->flushall();
  } catch(std::exception& e) {
    LDEBUG << "Error connecting to Redis server at " << uri << ": " << e.what();
    if (redis != NULL) {
      delete redis;
      redis = NULL;
    }
  }

  return redis;
}

sw::redis::Redis* RedisKVS2::get_redis_client(std::string uri) {
  sw::redis::Redis* redis = NULL;

  try {
    LDEBUG << "Connecting to Redis server at " << uri;
    redis = new sw::redis::Redis(uri);
    redis->set("test_key", "test_val");
    auto val = redis->get("test_key");
    assert(val);
    assert(*val == "test_val");
    redis->flushall();
  } catch(std::exception& e) {
    LDEBUG << "Error connecting to Redis server at " << uri << ": " << e.what();
    if (redis != NULL) {
      delete redis;
      redis = NULL;
    }
  }

  return redis;
}

RedisKVS1::RedisKVS1(unsigned id,
                     uint64_t period_ns,
                     uint64_t offset_ns,
                     unsigned priority,
                     unsigned cpu,
                     uint64_t max_jobs,
                     unsigned port,
                     log4cpp::Category *logger)
  : KVSInterface(id, period_ns, offset_ns, priority, cpu, logger),
    max_jobs(max_jobs),
    logger(logger) {
  LDEBUG << "Constructing Redis KVS instance # " << id;
  redis = get_redis_client(std::string("tcp://127.0.0.1:") +
                           std::to_string(port));
  //redis = get_redis_client(uri);
  if (redis == NULL) {
    assert(false);
  }
}

RedisKVS1::~RedisKVS1() {
  redis->flushall();
}

void RedisKVS1::job() {
  // garbage collection
  for (const std::string& key : keys) {
    unsigned int count = redis->zcount(key, sw::redis::UnboundedInterval<double> {});
    if (count > 10) {
      redis->zremrangebyrank(key, 0, count - 10 - 1);
    }
  }

  num_jobs++;
}

// Using the Redis data type of sorted sets
bool RedisKVS1::try_read(std::string key, uint64_t no_earlier_than,
                         std::string &value) {
  uint64_t now = Utils::get_time_now_ns();
  //LDEBUG << "RedisKVS" << id << "::try_read " << key << " [" << no_earlier_than
  //       << ", " << now << "]";
  if (no_earlier_than > now) {
    return false;
  }

  std::vector<std::string> without_score;
  redis->zrangebyscore(
    key,
    sw::redis::BoundedInterval<double>(no_earlier_than, now,
                                       sw::redis::BoundType::CLOSED),
    std::back_inserter(without_score));

  if (without_score.empty()) {
    return false;
  }

  value = without_score.back();
  return true;
}

//// Using the Redis data type of hash map
//bool RedisKVS1::try_read(std::string key, uint64_t no_earlier_than,
//                      std::string &value) {
//  //LDEBUG << "RedisKVS" << id << "::try_read " << key << " " << no_earlier_than;
//  bool status = false;
//  uint64_t now = Utils::get_time_now_ns();
//  if (no_earlier_than > now) {
//    return status;
//  }
//
//  std::unordered_map<std::string, std::string> m;
//  redis->hgetall(key, std::inserter(m, m.begin()));
//  for (auto i = m.begin(); i != m.end(); i++) {
//    uint64_t ts;
//    std::istringstream iss(i->first);
//    iss >> ts;
//    if (ts <= now and ts >= no_earlier_than) {
//      no_earlier_than = ts;
//      value = i->second;
//      status = true;
//    }
//  }
//
//  return status;
//}

// Using the Redis data type of sorted sets
bool RedisKVS1::try_write(std::string key, uint64_t publish_time,
                          std::string value) {
  //LDEBUG << "RedisKVS" << id << "::try_write " << key << " " << publish_time
  //       << " " << value;
  if (publish_time <= Utils::get_time_now_ns()) {
    return false;
  }
  redis->zadd(key, value, publish_time);
  return true;
}

//// Using the Redis data type of hash maps
//bool RedisKVS1::try_write(std::string key, uint64_t publish_time,
//                       std::string value) {
//  //LDEBUG << "RedisKVS" << id << "::try_write " << key << " " << publish_time
//  //       << " " << value;
//  if (publish_time <= Utils::get_time_now_ns()) {
//    return false;
//  }
//  redis->hset(key, std::to_string(publish_time), value);
//  return true;
//}

void RedisKVS1::add_new_keys(std::vector<std::string>& new_keys) {
  keys.insert(new_keys.begin(), new_keys.end());
}

void RedisKVS1::print_stats() {
  TimeSpec diff_ts;
  timespecsub(&task_finish_ts, &task_start_ts, &diff_ts);
  LDEBUG << "Task " << id << " actual (expected) duration: "
         << TS_TO_MS(&diff_ts) << "ms (" << max_jobs * NS_TO_MS(time_period_ns)
         << "ms), ACET (measured): " << acet_measured_ms << "ms";
  //LDEBUG << "RedisKVS" << id << "::print_everything";
  //for (const std::string& key : keys) {
  //  LDEBUG << "Key: " << key;
  //  std::vector<std::pair<std::string, double>> with_score;
  //  redis->zrangebyscore(key,
  //                       sw::redis::UnboundedInterval<double> {},
  //                       std::back_inserter(with_score));
  //  for (const auto& item : with_score) {
  //    LDEBUG << item.first << " " << item.second;
  //  }
  //}
}

RedisKVS2::RedisKVS2(unsigned id,
                     uint64_t period_ns,
                     uint64_t offset_ns,
                     unsigned priority,
                     unsigned cpu,
                     uint64_t max_jobs,
                     unsigned port,
                     std::vector<TCP::Node> peers_minus_self,
                     log4cpp::Category *logger)
  : KVSInterface(id, period_ns, offset_ns, priority, cpu, logger),
    max_jobs(max_jobs),
    local_port(std::to_string(port)),
    logger(logger) {
  LDEBUG << "Constructing Redis KVS instance # " << id;
  local_client = get_redis_client(std::string("tcp://127.0.0.1:") +
                                  local_port);
  if (local_client == NULL) {
    assert(false);
  }
  all_clients.push_back(local_client);
  all_ports.push_back(local_port);
  for (TCP::Node& peer : peers_minus_self) {
    sw::redis::Redis* redis = get_redis_client(peer.get_uri());
    if (local_client == NULL) {
      assert(false);
    }
    remote_clients.push_back(redis);
    all_clients.push_back(redis);
    all_ports.push_back(peer.port);
  }
}

RedisKVS2::~RedisKVS2() {
  //if (local_client != NULL) {
  //  local_client->flushall();
  //  delete local_client;
  //}
  for (sw::redis::Redis* c : all_clients) {
    if (c != NULL) {
      c->flushall();
      delete c;
    }
  }
}

void RedisKVS2::job() {
  // garbage collection
  for (const std::string& key : keys) {
    unsigned int count = local_client->zcount(key,
                         sw::redis::UnboundedInterval<double> {});
    if (count > 10) {
      local_client->zremrangebyrank(key, 0, count - 10 - 1);
    }
  }

  num_jobs++;
}

// Using the Redis data type of sorted sets
bool RedisKVS2::try_read(std::string key, uint64_t no_earlier_than,
                         std::string &value) {
  uint64_t now = Utils::get_time_now_ns();
  //LDEBUG << "RedisKVS2" << id << "::try_read " << key << " [" << no_earlier_than
  //       << ", " << now << "]";
  if (no_earlier_than > now) {
    return false;
  }

  std::vector<std::string> candidate_values;

  for (sw::redis::Redis* c : all_clients) {
    std::vector<std::string> without_score;
    c->zrangebyscore(
      key,
      sw::redis::BoundedInterval<double>(no_earlier_than, now,
                                         sw::redis::BoundType::CLOSED),
      std::back_inserter(without_score));
    if (!without_score.empty()) {
      candidate_values.push_back(without_score.back());
    }
  }

  return fuse_redundant_values(candidate_values, use_simple_median, value);
}

// Using the Redis data type of sorted sets
bool RedisKVS2::try_write(std::string key, uint64_t publish_time,
                          std::string value) {
  //LDEBUG << "RedisKVS2" << id << "::try_write " << key << " " << publish_time
  //       << " " << value;

  if (publish_time <= Utils::get_time_now_ns()) {
    return false;
  }

  local_client->zadd(key, value, publish_time);
  keys.insert(key);
  return true;
}

void RedisKVS2::add_new_keys(std::vector<std::string>& new_keys) {
  keys.insert(new_keys.begin(), new_keys.end());
}

void RedisKVS2::print_stats() {
  TimeSpec diff_ts;
  timespecsub(&task_finish_ts, &task_start_ts, &diff_ts);
  LDEBUG << "RedisKVS2 Task " << id << " actual (expected) duration: "
         << TS_TO_MS(&diff_ts) << "ms (" << max_jobs * NS_TO_MS(time_period_ns)
         << "ms), BCET (measured): " << bcet_measured_ms
         << "ms, ACET (measured): " << acet_measured_ms
         << "ms, WCET (measured): " << wcet_measured_ms << "ms";
  //LDEBUG << "RedisKVS2" << id << "::print_everything";
  //for (const std::string& key : keys) {
  //  LDEBUG << "Key: " << key;
  //  std::vector<std::pair<std::string, double>> with_score;
  //  local_client->zrangebyscore(key,
  //                              sw::redis::UnboundedInterval<double> {},
  //                              std::back_inserter(with_score));
  //  for (const auto& item : with_score) {
  //    LDEBUG << item.first << " " << item.second;
  //  }
  //}
}

}

#ifndef REDIS_KVS_H
#define REDIS_KVS_H

#include <sw/redis++/redis++.h>

#include "achal/tcp/client.h"
#include "achal/simple_kvs.h"
#include "achal/kvs_interface.h"
#include "utils/logging.h"
#include "utils/periodic_task.h"

namespace Achal {

//class RedisHost {
// public:
//  unsigned port;
//  std::string ip;
//  std::string hostname;
//
//  RedisHost(unsigned port, std::string ip, std::string hostname = "")
//    : port(port), ip(ip), hostname(hostname) {}
//
//  std::string get_uri() {
//    return std::string("tcp://") + ip + ":" + std::to_string(port);
//  }
//};

class RedisKVS1 : public KVSInterface {
 public:
  unsigned id;
  log4cpp::Category* logger;
  uint64_t max_jobs;
  sw::redis::Redis* redis = NULL;
  std::set<std::string> keys;

  RedisKVS1(unsigned id,
            uint64_t period_ns,
            uint64_t offset_ns,
            unsigned priority,
            unsigned cpu,
            uint64_t max_jobs,
            unsigned port,
            log4cpp::Category *logger);
  ~RedisKVS1();

  void job();

  void job_for_profiler() {
    job();
  }

  bool terminate() {
    return num_jobs >= max_jobs;
  }

  bool try_read(std::string key, uint64_t no_earlier_than, std::string &value);

  bool try_write(std::string key, uint64_t publish_time, std::string value);

  void add_new_keys(std::vector<std::string>& new_keys);

  void print_stats();

  sw::redis::Redis* get_redis_client(std::string uri);

  void update_fuse_function() {
    use_simple_median = true;
  }

};

class RedisKVS2 : public KVSInterface {
 public:
  unsigned id;
  log4cpp::Category* logger;
  uint64_t max_jobs;
  sw::redis::Redis* local_client;
  std::string local_port;
  std::vector<sw::redis::Redis*> remote_clients;
  std::vector<sw::redis::Redis*> all_clients;
  std::vector<std::string> all_ports;
  std::set<std::string> keys;

  RedisKVS2(unsigned id,
            uint64_t period_ns,
            uint64_t offset_ns,
            unsigned priority,
            unsigned cpu,
            uint64_t max_jobs,
            unsigned port,
            std::vector<TCP::Node> peers_minus_self,
            log4cpp::Category *logger);
  ~RedisKVS2();

  void job();

  void job_for_profiler() {
    job();
  }

  bool terminate() {
    return num_jobs >= max_jobs;
  }

  bool try_read(std::string key, uint64_t no_earlier_than, std::string &value);

  bool try_write(std::string key, uint64_t publish_time, std::string value);

  void add_new_keys(std::vector<std::string>& new_keys);

  void print_stats();

  sw::redis::Redis* get_redis_client(std::string uri);

  void update_fuse_function() {
    use_simple_median = true;
  }

};

}

#endif  // REDIS_KVS_H 

#ifndef ETCD_KVS_H
#define ETCD_KVS_H

#include <etcd/SyncClient.hpp>

#include "achal/tcp/client.h"
#include "achal/simple_kvs.h"
#include "achal/kvs_interface.h"
#include "utils/logging.h"
#include "utils/periodic_task.h"

namespace Achal {

class EtcdKVS : public KVSInterface {
 public:
  unsigned id;
  log4cpp::Category* logger;
  uint64_t max_jobs;
  etcd::SyncClient* local_client;
  std::string local_port;
  //std::vector<etcd::SyncClient*> remote_clients;
  //std::vector<etcd::SyncClient*> all_clients;
  std::vector<std::string> all_ports;
  std::set<std::string> keys;
  const int ttl_sec = 20;
  std::string etcd_port = "2379";

  EtcdKVS(unsigned id,
            uint64_t period_ns,
            uint64_t offset_ns,
            unsigned priority,
            unsigned cpu,
            uint64_t max_jobs,
            unsigned port,
            std::vector<TCP::Node> peers_minus_self,
            log4cpp::Category *logger);
  ~EtcdKVS();

  void job();

  void job_for_profiler() {
    job();
  }

  bool terminate() {
    return num_jobs >= max_jobs;
  }

  bool try_read(std::string key, uint64_t no_earlier_than, std::string &value);

  bool try_write(std::string key, uint64_t publish_time, std::string value);

  void print_stats();

  etcd::SyncClient* get_etcd_client(std::string url);

  void update_fuse_function() {
    use_simple_median = true;
  }

};
}

#endif  // ETCD_KVS_H 

#include "etcd_kvs.h"

#include <sstream>

#include "utils/misc.h"
#include "utils/timespec.h"

namespace Achal {

etcd::SyncClient* EtcdKVS::get_etcd_client(std::string url) {
  etcd::SyncClient* etcd = NULL;

  try {
    LDEBUG << "Connecting to etcd server at " << url;
    etcd = new etcd::SyncClient(url);
    assert(etcd->head().is_ok());
    //etcd->rmdir("/key", true);
    //assert(etcd->ls("/key").values().size() == 0);

    //etcd->put("/key/t1", "v1");
    //assert(etcd->get("/key/t1").value().as_string() == "v1");
    //assert(etcd->ls("/key").is_ok() == true);
    //assert(etcd->ls("/key").values().size() == 1);
    //assert(etcd->ls("/key").values()[0].key() == "/key/t1");

    //etcd->put("/key/t2", "v2");
    //assert(etcd->get("/key/t2").value().as_string() == "v2");
    //assert(etcd->ls("/key").is_ok() == true);
    //assert(etcd->ls("/key").values().size() == 2);
    //assert(etcd->ls("/key").values()[0].key() == "/key/t1");
    //assert(etcd->ls("/key").values()[1].key() == "/key/t2");

    //etcd->rmdir("/key", true);
    //assert(etcd->ls("/key").values().size() == 0);
  } catch(std::exception& e) {
    LDEBUG << "Error connecting to etcd server at " << url << ": " << e.what();
    if (etcd != NULL) {
      delete etcd;
      etcd = NULL;
    }
  }

  return etcd;
}

EtcdKVS::EtcdKVS(unsigned id, uint64_t period_ns, uint64_t offset_ns,
                 unsigned priority, unsigned cpu, uint64_t max_jobs,
                 unsigned port, std::vector<TCP::Node> peers_minus_self,
                 log4cpp::Category *logger)
    : KVSInterface(id, period_ns, offset_ns, priority, cpu, logger),
      max_jobs(max_jobs),
      local_port(std::to_string(port)),
      logger(logger) {
  LDEBUG << "Constructing etcd KVS instance # " << id;
  local_client = get_etcd_client(std::string("http://127.0.0.1:") +
                                 etcd_port);
  if (local_client == NULL) {
    assert(false);
  }
  //all_clients.push_back(local_client);
  all_ports.push_back(local_port);
  for (TCP::Node& peer : peers_minus_self) {
    //etcd::SyncClient* etcd = get_etcd_client(peer.get_url());
    //if (local_client == NULL) {
    //  assert(false);
    //}
    //remote_clients.push_back(etcd);
    //all_clients.push_back(etcd);
    all_ports.push_back(peer.port);
  }
}

EtcdKVS::~EtcdKVS() {
  if (local_client != NULL) {
    local_client->rmdir("/key", true);
    delete local_client;
  }
  //for (etcd::SyncClient* c : all_clients) {
  //  if (c != NULL) {
  //    c->rmdir("/key", true);
  //    delete c;
  //  }
  //}
}

void EtcdKVS::job() {
  // garbage collection not required!

  num_jobs++;
}

bool EtcdKVS::try_read(std::string key, uint64_t no_earlier_than,
                       std::string &value) {
  uint64_t now = Utils::get_time_now_ns();
  // LDEBUG << "RedisKVS2" << id << "::try_read " << key << " [" << no_earlier_than
  //       << ", " << now << "]";
  if (no_earlier_than > now) {
    return false;
  }

  std::vector<std::string> candidate_values;

  for (std::string port : all_ports) {
    etcd::Response res = local_client->ls("/" + port + ":" + key + "/");

    uint64_t max_published_time = no_earlier_than;
    std::string max_published_value;

    bool found = false;

    for (auto v : res.values()) {
        uint64_t time = std::stoull(v.key().substr(key.size() + port.size() + 3));

        if (no_earlier_than <= time && time <= now) {
            found = true;
            if (time >= max_published_time) {
                max_published_value = v.as_string();
            }
        }
    }

    if (found) {

      candidate_values.push_back(max_published_value);
    }
  }

  return fuse_redundant_values(candidate_values, use_simple_median, value);
}

bool EtcdKVS::try_write(std::string key, uint64_t publish_time,
                        std::string value) {
  // LDEBUG << "RedisKVS2" << id << "::try_write " << key << " " << publish_time
  //       << " " << value;
  if (publish_time <= Utils::get_time_now_ns()) {
    return false;
  }

  return local_client->set(
    "/" + local_port + ":" + key + "/" + std::to_string(publish_time),
    value,
    ttl_sec).is_ok();
}

void EtcdKVS::print_stats() {
  TimeSpec diff_ts;
  timespecsub(&task_finish_ts, &task_start_ts, &diff_ts);
  LDEBUG << "EtcdKVS Task " << id << " actual (expected) duration: "
         << TS_TO_MS(&diff_ts) << "ms (" << max_jobs * NS_TO_MS(time_period_ns)
         << "ms), ACET (measured): " << acet_measured_ms << "ms";
}

}  // namespace Achal

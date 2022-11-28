#include <fstream>
#include <iostream>

#include "utils/misc.h"
#include "utils/macros.h"
#include "utils/logging.h"
#include "utils/kzh15_benchmark_generator.h"
#include "achal/bft_kvs.h"
#include "achal/redis_kvs.h"
#include "achal/etcd_kvs.h"
#include "applications/kzh15.h"

#include "common.h"

#define BFT_KVS "BFTKVS"
#define REDIS_KVS1 "RedisKVS1"
#define REDIS_KVS2 "RedisKVS2"
#define ETCD_KVS "ETCDKVS"

#define EXP_TYPE_DISTRIBUTED "distributed"
#define EXP_TYPE_LOCAL "local"

#define MAX_LABEL_SIZE 16

#define BFT_KVS_DEFAULT_PRIORITY 1
#define BFT_KVS_DEFAULT_NETWORK_DELAY_US 100
#define BFT_KVS_DEFAULT_NUM_ROUNDS 2
#define BFT_KVS_TASK_ID(NODE_ID, CPU_ID) ((1000 * NODE_ID) + (100 * CPU_ID))
#define BFT_KVS_PORT(CPU_ID) (8080 + CPU_ID)

#define REDIS_KVS_PORT(CPU_ID) (7000 + CPU_ID)

#define PERIODIC_TASK_OFFSET_NS 0
#define PERIODIC_TASK_ID(NODE_ID, CPU_ID, UNIQUE_TASK_ID) \
  ((1000 * NODE_ID) + (100 * CPU_ID) + (UNIQUE_TASK_ID + 1))

typedef Achal::BFTKVS<100, 7, 15> BFTKVS_NT100_KS7_VS15;
typedef Achal::BFTKVS<200, 7, 15> BFTKVS_NT200_KS7_VS15;
typedef Achal::BFTKVS<300, 7, 15> BFTKVS_NT300_KS7_VS15;
typedef Achal::BFTKVS<400, 7, 15> BFTKVS_NT400_KS7_VS15;
typedef Achal::BFTKVS<500, 7, 15> BFTKVS_NT500_KS7_VS15;
typedef Achal::BFTKVS<600, 7, 15> BFTKVS_NT600_KS7_VS15;
typedef Achal::BFTKVS<700, 7, 15> BFTKVS_NT700_KS7_VS15;
typedef Achal::BFTKVS<800, 7, 15> BFTKVS_NT800_KS7_VS15;
typedef Achal::BFTKVS<900, 7, 15> BFTKVS_NT900_KS7_VS15;
typedef Achal::BFTKVS<1000, 7, 15> BFTKVS_NT1000_KS7_VS15;
typedef Achal::BFTKVS<1100, 7, 15> BFTKVS_NT1100_KS7_VS15;
typedef Achal::BFTKVS<1200, 7, 15> BFTKVS_NT1200_KS7_VS15;
typedef Achal::BFTKVS<1300, 7, 15> BFTKVS_NT1300_KS7_VS15;
typedef Achal::BFTKVS<1400, 7, 15> BFTKVS_NT1400_KS7_VS15;
typedef Achal::BFTKVS<1500, 7, 15> BFTKVS_NT1500_KS7_VS15;
typedef Achal::BFTKVS<1600, 7, 15> BFTKVS_NT1600_KS7_VS15;
typedef Achal::BFTKVS<1700, 7, 15> BFTKVS_NT1700_KS7_VS15;
typedef Achal::BFTKVS<1800, 7, 15> BFTKVS_NT1800_KS7_VS15;
typedef Achal::BFTKVS<1900, 7, 15> BFTKVS_NT1900_KS7_VS15;
typedef Achal::BFTKVS<2000, 7, 15> BFTKVS_NT2000_KS7_VS15;

log4cpp::Category *logger;

void bftkvs_init_from_config(unsigned NT,
                             std::vector<Achal::KVSInterface *>& kvs_instances,
                             Achal::config_t& bft_kvs_config) {
  if (NT < 100) {
    BFTKVS_NT100_KS7_VS15* kvs_instance = new BFTKVS_NT100_KS7_VS15(bft_kvs_config);
    kvs_instance->update_fuse_function(Achal::strong_simple_majority_of_strings);
    kvs_instances.push_back(kvs_instance);
  } else if (NT < 200) {
    BFTKVS_NT200_KS7_VS15* kvs_instance = new BFTKVS_NT200_KS7_VS15(bft_kvs_config);
    kvs_instance->update_fuse_function(Achal::strong_simple_majority_of_strings);
    kvs_instances.push_back(kvs_instance);
  } else if (NT < 300) {
    BFTKVS_NT300_KS7_VS15* kvs_instance = new BFTKVS_NT300_KS7_VS15(bft_kvs_config);
    kvs_instance->update_fuse_function(Achal::strong_simple_majority_of_strings);
    kvs_instances.push_back(kvs_instance);
  } else if (NT < 400) {
    BFTKVS_NT400_KS7_VS15* kvs_instance = new BFTKVS_NT400_KS7_VS15(bft_kvs_config);
    kvs_instance->update_fuse_function(Achal::strong_simple_majority_of_strings);
    kvs_instances.push_back(kvs_instance);
  } else if (NT < 500) {
    BFTKVS_NT500_KS7_VS15* kvs_instance = new BFTKVS_NT500_KS7_VS15(bft_kvs_config);
    kvs_instance->update_fuse_function(Achal::strong_simple_majority_of_strings);
    kvs_instances.push_back(kvs_instance);
  } else if (NT < 600) {
    BFTKVS_NT600_KS7_VS15* kvs_instance = new BFTKVS_NT600_KS7_VS15(bft_kvs_config);
    kvs_instance->update_fuse_function(Achal::strong_simple_majority_of_strings);
    kvs_instances.push_back(kvs_instance);
  } else if (NT < 700) {
    BFTKVS_NT700_KS7_VS15* kvs_instance = new BFTKVS_NT700_KS7_VS15(bft_kvs_config);
    kvs_instance->update_fuse_function(Achal::strong_simple_majority_of_strings);
    kvs_instances.push_back(kvs_instance);
  } else if (NT < 800) {
    BFTKVS_NT800_KS7_VS15* kvs_instance = new BFTKVS_NT800_KS7_VS15(bft_kvs_config);
    kvs_instance->update_fuse_function(Achal::strong_simple_majority_of_strings);
    kvs_instances.push_back(kvs_instance);
  } else if (NT < 900) {
    BFTKVS_NT900_KS7_VS15* kvs_instance = new BFTKVS_NT900_KS7_VS15(bft_kvs_config);
    kvs_instance->update_fuse_function(Achal::strong_simple_majority_of_strings);
    kvs_instances.push_back(kvs_instance);
  } else if (NT < 1000) {
    BFTKVS_NT1000_KS7_VS15* kvs_instance = new BFTKVS_NT1000_KS7_VS15(bft_kvs_config);
    kvs_instance->update_fuse_function(Achal::strong_simple_majority_of_strings);
    kvs_instances.push_back(kvs_instance);
  } else if (NT < 1100) {
    BFTKVS_NT1100_KS7_VS15* kvs_instance = new BFTKVS_NT1100_KS7_VS15(bft_kvs_config);
    kvs_instance->update_fuse_function(Achal::strong_simple_majority_of_strings);
    kvs_instances.push_back(kvs_instance);
  } else if (NT < 1200) {
    BFTKVS_NT1200_KS7_VS15* kvs_instance = new BFTKVS_NT1200_KS7_VS15(bft_kvs_config);
    kvs_instance->update_fuse_function(Achal::strong_simple_majority_of_strings);
    kvs_instances.push_back(kvs_instance);
  } else if (NT < 1300) {
    BFTKVS_NT1300_KS7_VS15* kvs_instance = new BFTKVS_NT1300_KS7_VS15(bft_kvs_config);
    kvs_instance->update_fuse_function(Achal::strong_simple_majority_of_strings);
    kvs_instances.push_back(kvs_instance);
  } else if (NT < 1400) {
    BFTKVS_NT1400_KS7_VS15* kvs_instance = new BFTKVS_NT1400_KS7_VS15(bft_kvs_config);
    kvs_instance->update_fuse_function(Achal::strong_simple_majority_of_strings);
    kvs_instances.push_back(kvs_instance);
  } else if (NT < 1500) {
    BFTKVS_NT1500_KS7_VS15* kvs_instance = new BFTKVS_NT1500_KS7_VS15(bft_kvs_config);
    kvs_instance->update_fuse_function(Achal::strong_simple_majority_of_strings);
    kvs_instances.push_back(kvs_instance);
  } else if (NT < 1600) {
    BFTKVS_NT1600_KS7_VS15* kvs_instance = new BFTKVS_NT1600_KS7_VS15(bft_kvs_config);
    kvs_instance->update_fuse_function(Achal::strong_simple_majority_of_strings);
    kvs_instances.push_back(kvs_instance);
  } else if (NT < 1700) {
    BFTKVS_NT1700_KS7_VS15* kvs_instance = new BFTKVS_NT1700_KS7_VS15(bft_kvs_config);
    kvs_instance->update_fuse_function(Achal::strong_simple_majority_of_strings);
    kvs_instances.push_back(kvs_instance);
  } else if (NT < 1800) {
    BFTKVS_NT1800_KS7_VS15* kvs_instance = new BFTKVS_NT1800_KS7_VS15(bft_kvs_config);
    kvs_instance->update_fuse_function(Achal::strong_simple_majority_of_strings);
    kvs_instances.push_back(kvs_instance);
  } else if (NT < 1900) {
    BFTKVS_NT1900_KS7_VS15* kvs_instance = new BFTKVS_NT1900_KS7_VS15(bft_kvs_config);
    kvs_instance->update_fuse_function(Achal::strong_simple_majority_of_strings);
    kvs_instances.push_back(kvs_instance);
  } else if (NT < 2000) {
    BFTKVS_NT2000_KS7_VS15* kvs_instance = new BFTKVS_NT2000_KS7_VS15(bft_kvs_config);
    kvs_instance->update_fuse_function(Achal::strong_simple_majority_of_strings);
    kvs_instances.push_back(kvs_instance);
  } else {
    assert(false);
  }
}

void bftkvs_connect_to_severs(unsigned NT,
                              std::vector<Achal::KVSInterface*>& kvs_instances) {
  for (Achal::KVSInterface* kvs_instance : kvs_instances) {
    if (NT < 100) {
      ((BFTKVS_NT100_KS7_VS15*)kvs_instance)->connect_to_servers();
    } else if (NT < 200) {
      ((BFTKVS_NT200_KS7_VS15*)kvs_instance)->connect_to_servers();
    } else if (NT < 300) {
      ((BFTKVS_NT300_KS7_VS15*)kvs_instance)->connect_to_servers();
    } else if (NT < 400) {
      ((BFTKVS_NT400_KS7_VS15*)kvs_instance)->connect_to_servers();
    } else if (NT < 500) {
      ((BFTKVS_NT500_KS7_VS15*)kvs_instance)->connect_to_servers();
    } else if (NT < 600) {
      ((BFTKVS_NT600_KS7_VS15*)kvs_instance)->connect_to_servers();
    } else if (NT < 700) {
      ((BFTKVS_NT700_KS7_VS15*)kvs_instance)->connect_to_servers();
    } else if (NT < 800) {
      ((BFTKVS_NT800_KS7_VS15*)kvs_instance)->connect_to_servers();
    } else if (NT < 900) {
      ((BFTKVS_NT900_KS7_VS15*)kvs_instance)->connect_to_servers();
    } else if (NT < 1000) {
      ((BFTKVS_NT1000_KS7_VS15*)kvs_instance)->connect_to_servers();
    } else if (NT < 1100) {
      ((BFTKVS_NT1100_KS7_VS15*)kvs_instance)->connect_to_servers();
    } else if (NT < 1200) {
      ((BFTKVS_NT1200_KS7_VS15*)kvs_instance)->connect_to_servers();
    } else if (NT < 1300) {
      ((BFTKVS_NT1300_KS7_VS15*)kvs_instance)->connect_to_servers();
    } else if (NT < 1400) {
      ((BFTKVS_NT1400_KS7_VS15*)kvs_instance)->connect_to_servers();
    } else if (NT < 1500) {
      ((BFTKVS_NT1500_KS7_VS15*)kvs_instance)->connect_to_servers();
    } else if (NT < 1600) {
      ((BFTKVS_NT1600_KS7_VS15*)kvs_instance)->connect_to_servers();
    } else if (NT < 1700) {
      ((BFTKVS_NT1700_KS7_VS15*)kvs_instance)->connect_to_servers();
    } else if (NT < 1800) {
      ((BFTKVS_NT1800_KS7_VS15*)kvs_instance)->connect_to_servers();
    } else if (NT < 1900) {
      ((BFTKVS_NT1900_KS7_VS15*)kvs_instance)->connect_to_servers();
    } else if (NT < 2000) {
      ((BFTKVS_NT2000_KS7_VS15*)kvs_instance)->connect_to_servers();
    } else {
      assert(false);
    }
  }
}

void bftkvs_connect_to_clients(unsigned NT,
                               std::vector<Achal::KVSInterface*>& kvs_instances) {
  for (Achal::KVSInterface* kvs_instance : kvs_instances) {
    if (NT < 100) {
      ((BFTKVS_NT100_KS7_VS15*)kvs_instance)->accept_clients();
    } else if (NT < 200) {
      ((BFTKVS_NT200_KS7_VS15*)kvs_instance)->accept_clients();
    } else if (NT < 300) {
      ((BFTKVS_NT300_KS7_VS15*)kvs_instance)->accept_clients();
    } else if (NT < 400) {
      ((BFTKVS_NT400_KS7_VS15*)kvs_instance)->accept_clients();
    } else if (NT < 500) {
      ((BFTKVS_NT500_KS7_VS15*)kvs_instance)->accept_clients();
    } else if (NT < 600) {
      ((BFTKVS_NT600_KS7_VS15*)kvs_instance)->accept_clients();
    } else if (NT < 700) {
      ((BFTKVS_NT700_KS7_VS15*)kvs_instance)->accept_clients();
    } else if (NT < 800) {
      ((BFTKVS_NT800_KS7_VS15*)kvs_instance)->accept_clients();
    } else if (NT < 900) {
      ((BFTKVS_NT900_KS7_VS15*)kvs_instance)->accept_clients();
    } else if (NT < 1000) {
      ((BFTKVS_NT1000_KS7_VS15*)kvs_instance)->accept_clients();
    } else if (NT < 1100) {
      ((BFTKVS_NT1100_KS7_VS15*)kvs_instance)->accept_clients();
    } else if (NT < 1200) {
      ((BFTKVS_NT1200_KS7_VS15*)kvs_instance)->accept_clients();
    } else if (NT < 1300) {
      ((BFTKVS_NT1300_KS7_VS15*)kvs_instance)->accept_clients();
    } else if (NT < 1400) {
      ((BFTKVS_NT1400_KS7_VS15*)kvs_instance)->accept_clients();
    } else if (NT < 1500) {
      ((BFTKVS_NT1500_KS7_VS15*)kvs_instance)->accept_clients();
    } else if (NT < 1600) {
      ((BFTKVS_NT1600_KS7_VS15*)kvs_instance)->accept_clients();
    } else if (NT < 1700) {
      ((BFTKVS_NT1700_KS7_VS15*)kvs_instance)->accept_clients();
    } else if (NT < 1800) {
      ((BFTKVS_NT1800_KS7_VS15*)kvs_instance)->accept_clients();
    } else if (NT < 1900) {
      ((BFTKVS_NT1900_KS7_VS15*)kvs_instance)->accept_clients();
    } else if (NT < 2000) {
      ((BFTKVS_NT2000_KS7_VS15*)kvs_instance)->accept_clients();
    } else {
      assert(false);
    }
  }
}

void get_preset_wcets(uint64_t& wcet_50ms,
                      uint64_t& wcet_100ms,
                      uint64_t& wcet_200ms,
                      uint64_t& wcet_1000ms,
                      std::string config_file_name) {
  if (config_file_name == "kzh15_113pcu_1018l.cfg") {
    wcet_50ms = 2;
    wcet_100ms = 2;
    wcet_200ms = 2;
    wcet_1000ms = 2;
  } else if (config_file_name == "kzh15_109pcu_1479l.cfg") {
    wcet_50ms = 2;
    wcet_100ms = 2;
    wcet_200ms = 2;
    wcet_1000ms = 2;
  } else if (config_file_name == "kzh15_108pcu_1557l.cfg") {
    wcet_50ms = 2;
    wcet_100ms = 3;
    wcet_200ms = 2;
    wcet_1000ms = 2;
  } else if (config_file_name == "kzh15_94pcu_1891l.cfg") {
    wcet_50ms = 2;
    wcet_100ms = 3;
    wcet_200ms = 2;
    wcet_1000ms = 2;
  } else if (config_file_name == "kzh15_140pcu_2017l.cfg") {
    wcet_50ms = 2;
    wcet_100ms = 3;
    wcet_200ms = 2;
    wcet_1000ms = 3;
  } else if (config_file_name == "kzh15_115pcu_2062l.cfg") {
    wcet_50ms = 2;
    wcet_100ms = 3;
    wcet_200ms = 2;
    wcet_1000ms = 2;
  } else if (config_file_name == "kzh15_125pcu_2541l.cfg") {
    wcet_50ms = 2;
    wcet_100ms = 3;
    wcet_200ms = 2;
    wcet_1000ms = 2;
  } else if (config_file_name == "kzh15_103pcu_2846l.cfg") {
    wcet_50ms = 2;
    wcet_100ms = 4;
    wcet_200ms = 2;
    wcet_1000ms = 2;
  } else if (config_file_name == "kzh15_105pcu_2965l.cfg") {
    wcet_50ms = 3;
    wcet_100ms = 4;
    wcet_200ms = 2;
    wcet_1000ms = 3;
  } else if (config_file_name == "kzh15_105pcu_3476l.cfg") {
    wcet_50ms = 3;
    wcet_100ms = 5;
    wcet_200ms = 2;
    wcet_1000ms = 3;
  } else {
    // These are default for 50ms smallest period
    wcet_50ms = 2;
    wcet_100ms = 4;
    wcet_200ms = 2;
    wcet_1000ms = 2;
    // These were used for 100ms smallest period
    //wcet_50ms = 5;
    //wcet_100ms = 5;
    //wcet_200ms = 5;
    //wcet_1000ms = 5;
    //assert(false);
  }
}


class AggregateStats {
 public:
  std::string workload;
  std::string config_file;
  unsigned num_tasks;
  unsigned num_labels;
  double average_label_size_bytes;
  double total_task_utilization_pc;
  uint64_t exp_duration_s;
  int num_cpus_used;
  uint64_t smallest_period_ms;
  std::string kvs_type;
  int node_id;
  double avg_succ_read_pc;
  double avg_succ_write_pc;
  double avg_succ_batch_read_pc;
  double avg_succ_batch_write_pc;
  double app_bcet_measured_ms;
  double app_acet_measured_ms;
  double app_wcet_measured_ms;
  double kvs_bcet_measured_ms;
  double kvs_acet_measured_ms;
  double kvs_wcet_measured_ms;

  std::string get_string() {
    std::stringstream ss;
    ss << workload << ", "
       << config_file << ", "
       << num_tasks << ", "
       << num_labels << ", "
       << average_label_size_bytes << ", "
       << total_task_utilization_pc << ", "
       << exp_duration_s << ", "
       << num_cpus_used << ", "
       << smallest_period_ms << ", "
       << kvs_type << ", "
       << node_id << ", "
       << avg_succ_read_pc << ", "
       << avg_succ_write_pc << ", "
       << avg_succ_batch_read_pc << ", "
       << avg_succ_batch_write_pc << ", "
       << app_bcet_measured_ms << ", "
       << app_acet_measured_ms << ", "
       << app_wcet_measured_ms << ", "
       << kvs_bcet_measured_ms << ", "
       << kvs_acet_measured_ms << ", "
       << kvs_wcet_measured_ms;
    return ss.str();
  }

  std::string get_header() {
    std::stringstream ss;
    ss << "Workload, "
       << "Benchmark Config, "
       << "#Tasks, "
       << "#Labels, "
       << "Avg. label size (bytes), "
       << "Configured total task utilization (%), "
       << "Duration (s), "
       << "#CPUs, "
       << "Smallest Period (ms), "
       << "KVS Type, "
       << "Node ID, "
       << "Success Read %, "
       << "Success Write %, "
       << "Success Read Iterations %, "
       << "Success Write Iterations %, "
       << "App BCET (ms), "
       << "App ACET (ms), "
       << "App WCET (ms), "
       << "KVS BCET (ms), "
       << "KVS ACET (ms), "
       << "KVS WCET (ms)";
    return ss.str();
  }

  void append_to_file(std::string filepath) {
    bool file_exists = false;

    std::ifstream ifs;
    ifs.open(filepath);
    if(ifs) {
      LDEBUG << filepath << " already exists";
      file_exists = true;
    } else {
      LDEBUG << filepath << " does not exist";
    }
    ifs.close();

    std::ofstream ofs;
    ofs.open(filepath, std::ios_base::app);
    if (!file_exists) {
      ofs << get_header() << std::endl;
    }
    ofs << get_string() << std::endl;
    ofs.close();
  }
};

int get_period_based_priority(uint64_t period_ms) {
  if (period_ms <= 1) {
    return 25;
  } else if (period_ms <= 2) {
    return 24;
  } else if (period_ms <= 5) {
    return 23;
  } else if (period_ms <= 10) {
    return 22;
  } else if (period_ms <= 20) {
    return 21;
  } else if (period_ms <= 50) {
    return 20;
  } else if (period_ms <= 100) {
    return 19;
  } else if (period_ms <= 200) {
    return 18;
  }
  
  return 17;
}

std::vector<Achal::TCP::Node> get_peers_minus_self(int cpus_used, int self,
                                                   std::string kvs_type) {
  std::vector<Achal::TCP::Node> peers;
  for (int i = 0; i < cpus_used; i++) {
    if (i != self) {
      if (kvs_type == BFT_KVS) {
        std::string ip = "achal0" + std::to_string(i + 1);
        peers.push_back(Achal::TCP::Node{ip, "8081"});
      } else {
        std::string port = std::to_string(REDIS_KVS_PORT(i));
        peers.push_back(Achal::TCP::Node{"127.0.0.1", port});
      }
    }
  }
  return peers;
}

void show_usage() {
  LDEBUG << "Error: Incorrect usage";
  LDEBUG << "Usage: bosch1 [smallest_period_ms] [benchnark_cfg] [exp_duration_s] [kvs_type] [output_file] [exp_type] [node_id] [total_nodes] [cpus_used]";
  LDEBUG << "\tsmallest_period_ms: typically, between 0 to 100";
  LDEBUG << "\tbenchmark_cfg: *.cfg filename, e.g., kzh15_42pcu_1031l.cfg";
  LDEBUG << "\texp_duration_s: typically, 10";
  LDEBUG << "\tkvs_type: either BFTKVS, RedisKVS1, or RedisKVS2";
  LDEBUG << "\toutput_file: *.data filename, where the aggregate stats are written";
  LDEBUG << "\texp_type: 'local' or 'distributed'";
  LDEBUG << "\tnode_id: set to x for achalx";
  LDEBUG << "\ttotal_nodes: number of nodes being used";
  LDEBUG << "\t\tif 'local', set to 1";
  LDEBUG << "\t\tif 'distributed', typically 3 or 4";
  LDEBUG << "\tcpus_used: number of cpus to use (typically, between 1 and 3)";
  LDEBUG << "\t\tif 'local', each cpu mimics an independent node";
  LDEBUG << "\t\tif 'distributed', each cpu runs identical task sets (not replicas)";
}

int main(int argc, char** argv) {
  bool logger_status = Utils::get_new_logger(
    "Bosch1", log4cpp::Priority::PriorityLevel::DEBUG, &logger);

  if (argc < 10) {
    show_usage();
    return 1;
  }

  uint64_t smallest_period_ms = std::stoul(argv[1]);
  std::string config_file_name = std::string(argv[2]);
  double exp_duration_s = std::stod(argv[3]);
  std::string kvs_type = std::string(argv[4]);
  std::string output_file_name = std::string(argv[5]);
  std::string exp_type = std::string(argv[6]);
  int node_id = std::stoi(argv[7]);
  int total_nodes = std::stoi(argv[8]);
  int cpus_used = std::stoi(argv[9]);

  uint64_t wcet_50ms;
  uint64_t wcet_100ms;
  uint64_t wcet_200ms;
  uint64_t wcet_1000ms;
  get_preset_wcets(wcet_50ms, wcet_100ms, wcet_200ms, wcet_1000ms, config_file_name);
  if (smallest_period_ms > 50) {
    wcet_50ms = 0;
  }
  uint64_t bosch_task_wcet_total_ms = wcet_50ms + wcet_100ms + wcet_200ms + wcet_1000ms;

  if (kvs_type != BFT_KVS) {
    LDEBUG << "Error: kvs_type " << kvs_type << " not supported";
    return 1;
  }

  if (exp_type != EXP_TYPE_DISTRIBUTED) {
    LDEBUG << "Error: exp_type " << exp_type << " not supported";
  }

  int num_nmr_instances = cpus_used;
  int replication_factor = total_nodes;

  std::string config_file_path = Utils::get_project_directory() +
                                 "/config/benchmarks/" + config_file_name;

  unsigned int max_cpus = Utils::get_max_cpus();
  LDEBUG << "Platform has " << max_cpus << " CPUs";

  LDEBUG << "Experiment configuration: " << exp_type << " experiment on node N" << node_id
         << " for " << exp_duration_s << "s";
  LDEBUG << "Task configuration: Bosch benchmark instances with minimum task period "
         << smallest_period_ms << "ms provisioned on " << cpus_used << " CPUs";

  LDEBUG << "Initializing benchmark instance from " << config_file_name;
  Utils::KZH15::BenchmarkInstanceBasic benchmark_instance(config_file_path, logger);
  Utils::KZH15::resolve_label_dependencies(benchmark_instance, smallest_period_ms);
  Utils::KZH15::trim_benchmark(benchmark_instance, smallest_period_ms);
  Utils::KZH15::shrink_label_sizes(benchmark_instance, MAX_LABEL_SIZE);
  benchmark_instance.print_summary();

  std::vector<Achal::config_t> bft_kvs_configs;
  for (unsigned nmr_id = 0; nmr_id < num_nmr_instances; nmr_id++) {
    uint64_t nmr_specific_offset_ns = nmr_id * (MS_TO_NS(smallest_period_ms) / num_nmr_instances);
    LDEBUG << "Initializing KVS configuration parameters for NMR" << nmr_id;

    Achal::config_t bft_kvs_config;
    bft_kvs_config.period_ns = MS_TO_NS(smallest_period_ms);
    // TODO Change to nmr_specific_offset_ns + wcet of each task
    bft_kvs_config.offset_ns = nmr_specific_offset_ns + MS_TO_NS(bosch_task_wcet_total_ms);
    bft_kvs_config.priority = BFT_KVS_DEFAULT_PRIORITY;
    bft_kvs_config.logger = logger;
    bft_kvs_config.max_network_delay_ns = US_TO_NS(BFT_KVS_DEFAULT_NETWORK_DELAY_US);
    bft_kvs_config.max_rounds = BFT_KVS_DEFAULT_NUM_ROUNDS;
    LDEBUG << "Max rounds: " << ((int)bft_kvs_config.max_rounds);
    bft_kvs_config.max_jobs = SEC_TO_NS(exp_duration_s) / bft_kvs_config.period_ns;

    for (int i = 0; i < replication_factor; i++) {
      Achal::process_t p;
      p.id = i;
      if (node_id <= 4) {
        p.ip = node_ips[i];
      } else {
        p.ip = node_ips[i + 4];
      }
      p.port = bft_kvs_ports[nmr_id][i];

      LDEBUG << "Adding peer with IP " << p.ip << " and port " << p.port;
      bft_kvs_config.peers.push_back(p);
    }

    bft_kvs_configs.push_back(bft_kvs_config);
  }

  LDEBUG << "Initializing KVS instances";
  std::vector<Achal::KVSInterface *> kvs_instances;

  if (kvs_type == BFT_KVS) {
    unsigned total_labels = benchmark_instance.get_total_labels();

    for (int i = 0; i < num_nmr_instances; i++) {
      LDEBUG << "Initializing KVS instance " << BFT_KVS_TASK_ID(node_id, i);
      Achal::config_t& bft_kvs_config = bft_kvs_configs[i];
      LDEBUG << "Size of peers: " << bft_kvs_config.peers.size();
      LDEBUG << "Max rounds: " << ((int)bft_kvs_config.max_rounds);
      assert(bft_kvs_config.max_rounds == 2);
      bft_kvs_config.port = std::stoi(bft_kvs_ports[i][(node_id - 1) % 4]);
      bft_kvs_config.id = BFT_KVS_TASK_ID(node_id, i);
      bft_kvs_config.cpu = i;
      bft_kvs_config.my_process_id = (node_id - 1) % 4;
      bftkvs_init_from_config(total_labels, kvs_instances, bft_kvs_config);
    }

    LDEBUG << "Have all KVS instances connect to servers";
    bftkvs_connect_to_severs(total_labels, kvs_instances);

    LDEBUG << "Have all KVS instances connect to clients";
    bftkvs_connect_to_clients(total_labels, kvs_instances);

  } else {

    LDEBUG << "Error: kvs_type " << kvs_type << " not supported";
    return 1;

  }

  LDEBUG << "Mapping benchmark instance to KVS-backed periodic tasks";
  std::vector<Applications::KZH15Task*> tasks;
  for (int cpu = 0; cpu < cpus_used; cpu++) {
    int unique_task = 0;
    for (Utils::KZH15::TaskBasic* t : benchmark_instance.tasks) {
      if (t->period_ms < smallest_period_ms) {
        assert(false);
      }
      uint64_t nmr_specific_offset_ns = cpu * (MS_TO_NS(smallest_period_ms) / cpus_used);
      //uint64_t periodic_task_offset_ms = NS_TO_MS(nmr_specific_offset_ns) + (unique_task * bosch_task_wcet_ms);
      uint64_t periodic_task_offset_ms = NS_TO_MS(nmr_specific_offset_ns);
      if (t->period_ms == 50) {
        periodic_task_offset_ms += 0;
      } else if (t->period_ms == 100) {
        periodic_task_offset_ms += wcet_50ms;
      } else if (t->period_ms == 200) {
        periodic_task_offset_ms += wcet_50ms +
                                   wcet_100ms;
      } else if (t->period_ms == 1000) {
        periodic_task_offset_ms += wcet_50ms +
                                   wcet_100ms +
                                   wcet_200ms;
      }
      Applications::KZH15Task* periodic_task =
        new Applications::KZH15Task(
          PERIODIC_TASK_ID(node_id, cpu, unique_task++),
          MS_TO_NS(t->period_ms),
          MS_TO_NS(periodic_task_offset_ms),
          get_period_based_priority(t->period_ms),
          cpu,
          kvs_instances[cpu],
          (SEC_TO_NS(exp_duration_s) / MS_TO_NS(t->period_ms)),
          t,
          benchmark_instance.label_sizes,
          MAX_LABEL_SIZE,
          logger);
      tasks.push_back(periodic_task);
    }
  }

  LDEBUG << "Updating freshness constraints for all inter-task labels read by every task";
  for (Applications::KZH15Task* t1 : tasks) {
    for (std::string lid_read : t1->rw_inter_task_labels_read_keys) {
      for (Applications::KZH15Task* t2 : tasks) {
        for (std::string lid_written : t2->rw_inter_task_labels_written_keys) {
          if (lid_read == lid_written) {
            t1->rw_inter_task_labels_read_freshness_constraints.push_back(t2->time_period_ns);
          }
        }
      }
    }
    assert(t1->rw_inter_task_labels_read_keys.size() == 
           t1->rw_inter_task_labels_read_freshness_constraints.size());
  }

  LDEBUG << "Releasing all application tasks in 10s";
  TimeSpec release_ts;
  clock_gettime(CLOCK_ID, &release_ts);
  release_ts.tv_sec /= 10;
  release_ts.tv_sec *= 10;
  release_ts.tv_sec += 20;
  release_ts.tv_nsec = 0;
  Utils::fix_ts(&release_ts);
  LDEBUG << "Releasing all application tasks at absolute time " << TS_TO_NS(&release_ts);

  //LDEBUG << "Releasing all KVS instances in 5s";
  //TimeSpec kvs_release_ts;
  //clock_gettime(CLOCK_ID, &kvs_release_ts);
  //kvs_release_ts.tv_sec += 5;
  //kvs_release_ts.tv_nsec = 0;
  //Utils::fix_ts(&kvs_release_ts);

  //for (int i = 0; i < tasks.size(); i++) {
  //  assert(tasks[i]->initialize_writes(TS_TO_NS(&release_ts)));
  //}

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
    kvs_instances[i]->print_stats();
  }

  for (int i = 0; i < tasks.size(); i++) {
    tasks[i]->print_stats();
  }

  AggregateStats stats;

  stats.workload = "Bosch1";
  stats.config_file = config_file_name;
  stats.num_tasks = benchmark_instance.get_total_tasks();
  stats.num_labels = benchmark_instance.get_total_labels();
  stats.average_label_size_bytes = benchmark_instance.get_avg_label_size();
  stats.total_task_utilization_pc = benchmark_instance.get_total_utilization();
  stats.exp_duration_s = exp_duration_s;
  stats.num_cpus_used = cpus_used;
  stats.smallest_period_ms = smallest_period_ms;
  stats.kvs_type = kvs_type;
  stats.node_id = node_id;
  stats.kvs_bcet_measured_ms = std::numeric_limits<double>::max();
  stats.kvs_acet_measured_ms = 0;
  stats.kvs_wcet_measured_ms = std::numeric_limits<double>::min();
  stats.app_bcet_measured_ms = std::numeric_limits<double>::max();
  stats.app_acet_measured_ms = 0;
  stats.app_wcet_measured_ms = std::numeric_limits<double>::min();

  for (int i = 0; i < kvs_instances.size(); i++) {
    if (stats.kvs_bcet_measured_ms > kvs_instances[i]->bcet_measured_ms) {
      stats.kvs_bcet_measured_ms = kvs_instances[i]->bcet_measured_ms;
    }
    if (stats.kvs_wcet_measured_ms < kvs_instances[i]->wcet_measured_ms) {
      stats.kvs_wcet_measured_ms = kvs_instances[i]->wcet_measured_ms;
    }
    stats.kvs_acet_measured_ms += kvs_instances[i]->acet_measured_ms;
  }
  stats.kvs_acet_measured_ms /= kvs_instances.size();

  for (int i = 0; i < tasks.size(); i++) {
    if (stats.app_bcet_measured_ms > tasks[i]->bcet_measured_ms) {
      stats.app_bcet_measured_ms = tasks[i]->bcet_measured_ms;
    }
    if (stats.app_wcet_measured_ms < tasks[i]->wcet_measured_ms) {
      stats.app_wcet_measured_ms = tasks[i]->wcet_measured_ms;
    }
    stats.app_acet_measured_ms += tasks[i]->acet_measured_ms;
  }
  stats.app_acet_measured_ms /= tasks.size();

  uint64_t success_reads = 0;
  uint64_t success_writes = 0;
  uint64_t success_batch_reads = 0;
  uint64_t success_batch_writes = 0;
  uint64_t failed_reads = 0;
  uint64_t failed_writes = 0;
  uint64_t failed_batch_reads = 0;
  uint64_t failed_batch_writes = 0;

  for (auto t : tasks) {
    success_reads += t->successful_reads;
    success_writes += t->successful_writes;
    success_batch_reads += t->successful_batch_reads;
    success_batch_writes += t->successful_batch_writes;
    failed_reads += t->failed_reads;
    failed_writes += t->failed_writes;
    failed_batch_reads += t->failed_batch_reads;
    failed_batch_writes += t->failed_batch_writes;
  }

  stats.avg_succ_read_pc =
    (success_reads * 100.0) / (success_reads + failed_reads);
  stats.avg_succ_write_pc =
    (success_writes * 100.0) / (success_writes + failed_writes);
  stats.avg_succ_batch_read_pc =
    (success_batch_reads * 100.0) / (success_batch_reads + failed_batch_reads);
  stats.avg_succ_batch_write_pc =
    (success_batch_writes * 100.0) / (success_batch_writes + failed_batch_writes);
    
  LDEBUG << "Summary: " << stats.get_string();

  std::string output_file_path = Utils::get_project_directory() +
                                 "/data/" + output_file_name;
  LDEBUG << "Flushing summary to " << output_file_path;
  stats.append_to_file(output_file_path);
  return 0;
}

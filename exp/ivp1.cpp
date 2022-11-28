#include <fstream>
#include <iostream>

#include <stdlib.h>

#include "utils/misc.h"
#include "utils/macros.h"
#include "utils/logging.h"
#include "utils/kzh15_benchmark_generator.h"
#include "achal/bft_kvs.h"
#include "achal/redis_kvs.h"
#include "achal/etcd_kvs.h"
#include "applications/inverted_pendulum_sim.h"

#include "common.h"

#define BFT_KVS "BFTKVS"
#define REDIS_KVS1 "RedisKVS1"
#define REDIS_KVS2 "RedisKVS2"
#define ETCD_KVS "ETCDKVS"

#define EXP_TYPE_DISTRIBUTED "distributed"
#define EXP_TYPE_LOCAL "local"

#define MAX_LABEL_SIZE 4

#define BFT_KVS_DEFAULT_PRIORITY 1
#define BFT_KVS_DEFAULT_NETWORK_DELAY_US 100
#define BFT_KVS_DEFAULT_NUM_ROUNDS 2
#define BFT_KVS_TASK_ID(NODE_ID, CPU_ID) ((1000 * NODE_ID) + (100 * CPU_ID))
#define BFT_KVS_PORT(CPU_ID) (8080 + CPU_ID)

#define REDIS_KVS_PORT(CPU_ID) (7000 + CPU_ID)

#define PERIODIC_TASK_OFFSET_NS 0
#define PERIODIC_TASK_ID(NODE_ID, CPU_ID, UNIQUE_TASK_ID) \
  ((1000 * NODE_ID) + (100 * CPU_ID) + (UNIQUE_TASK_ID + 1))

// === FAULT CONFIG START ===
#define FAULT_MODE_OK 0
#define FAULT_MODE_DEAD 1
#define FAULT_MODE_LIAR 2
// REMEMBER TO CHANGE <100, 7, 15> IF BFTKVS TEMPLATE ARGUMENTS CHANGED!!!
static uint8_t faulty_data[sizeof(struct Achal::eig_node<100, 7, 15>) * 100];
// === FAULT CONFIG END ===

log4cpp::Category *logger;

class AggregateStats {
 public:
  std::string workload;
  uint64_t exp_duration_s;
  int num_tasks_per_cpu;
  int num_cpus_used;
  uint64_t period_ms;
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
  std::string control_status;

  std::string get_string() {
    std::stringstream ss;
    ss << workload << ", "
       << exp_duration_s << ", "
       << num_tasks_per_cpu << ", "
       << num_cpus_used << ", "
       << period_ms << ", "
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
       << kvs_wcet_measured_ms << ", "
       << control_status;
    return ss.str();
  }

  std::string get_header() {
    std::stringstream ss;
    ss << "Workload, "
       << "Duration (s), "
       << "#Tasks/CPU, "
       << "#CPUs, "
       << "Period (ms), "
       << "KVS Type, "
       << "Node ID, "
       << "Successful Reads (%), "
       << "Successful Writes (%), "
       << "Successful Read Iterations (%), "
       << "Successful Write Iterations (%), "
       << "App BCET (ms), "
       << "App ACET (ms), "
       << "App WCET (ms), "
       << "KVS BCET (ms), "
       << "KVS ACET (ms), "
       << "KVS WCET (ms), "
       << "Stable control?";
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

std::vector<Achal::TCP::Node> get_peers_minus_self(
    std::string exp_type, int node_id, int nmr_id, int replication_factor,
    int self_replica_id) {
  LDEBUG << "get_peers_minus_self: nmr_id " << nmr_id << ", self_replica_id "
         << self_replica_id;
  std::string ip;
  std::string port;
  
  std::vector<Achal::TCP::Node> peers;
  for (int i = 0; i < replication_factor; i++) {
    if (i == self_replica_id) {
      continue;
    }
    if (exp_type == EXP_TYPE_LOCAL) {
      ip = node_ips[node_id - 1];
      port = bft_kvs_ports[i][(node_id - 1) % 4];
    } else {
      if (node_id <= 4) {
        ip = node_ips[i];
      } else {
        ip = node_ips[i + 4];
      }
      port = bft_kvs_ports[nmr_id][i];
    }
    
    LDEBUG << "Adding peer with IP " << ip << " and port " << port;
    peers.push_back(Achal::TCP::Node{ip, port});
  }

  return peers;
}

void show_usage() {
  LDEBUG << "Error: Incorrect usage";
  LDEBUG << "Usage: ivp1 [period_ms] [num_tasks_per_cpu] [exp_duration_s] [kvs_type] [output_file] [exp_type] [node_id] [total_nodes] [cpus_used] [ivp_task_wcet_ms] [kvs_task_wcet_ms]";
  LDEBUG << "\tperiod_ms: typically, between 0 and 1000";
  LDEBUG << "\tnum_tasks_per_cpu: number of unique tasks (not replicas) per CPU (at least 1)";
  LDEBUG << "\texp_duration_s: typically, between 10 and 3600";
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
  LDEBUG << "\tivp_task_wcet_ms: if profiled and for BFTKVS, provide the WCET of the IvP task (optional)";
  LDEBUG << "\tkvs_task_wcet_ms: if profiled and for BFTKVS, provide the WCET of the KVS task (optional)";
}

int main(int argc, char** argv) {

  // === FAULT CONFIG START ===
  std::memset((void *)faulty_data, 3, sizeof(faulty_data));
  // === FAULT CONFIG END ===

  bool logger_status = Utils::get_new_logger(
    "IvP1", log4cpp::Priority::PriorityLevel::DEBUG, &logger);

  if (argc < 14) {
    show_usage();
    return 1;
  }

  uint64_t period_ms = std::stoul(argv[1]);
  int num_tasks_per_cpu = std::stoi(argv[2]);
  double exp_duration_s = std::stod(argv[3]);
  std::string kvs_type = std::string(argv[4]);
  std::string output_file_name = std::string(argv[5]);
  std::string exp_type = std::string(argv[6]);
  int node_id = std::stoi(argv[7]);
  int total_nodes = std::stoi(argv[8]);
  int cpus_used = std::stoi(argv[9]);

  int num_nmr_instances = 1;
  if (exp_type == EXP_TYPE_DISTRIBUTED) {
    num_nmr_instances = cpus_used;
  }

  int replication_factor = total_nodes;
  if (exp_type == EXP_TYPE_LOCAL) {
    replication_factor = cpus_used;
  }

  if (kvs_type != BFT_KVS and kvs_type != REDIS_KVS2 and kvs_type != ETCD_KVS) {
    LDEBUG << "Error: kvs_type " << kvs_type << " not supported";
    return 1;
  }

  if (exp_type != EXP_TYPE_LOCAL and exp_type != EXP_TYPE_DISTRIBUTED) {
    LDEBUG << "Error: exp_type " << exp_type << " not supported";
    return 1;
  }

  unsigned int max_cpus = Utils::get_max_cpus();
  LDEBUG << "Platform has " << max_cpus << " CPUs";

  if (cpus_used >= max_cpus) {
    LDEBUG << "Error: cpus_used " << cpus_used << " not supported;"
           << kvs_type << " can use at most " << max_cpus - 1 << " CPUs";
    return 1;
  }

  LDEBUG << "Experiment configuration: " << exp_type << " experiment on node N" << node_id
         << " for " << exp_duration_s << "s";
  LDEBUG << "Task configuration: " << num_tasks_per_cpu
         << " IvPSim tasks with task period " << period_ms << "ms provisioned on " << cpus_used << " CPUs";

  uint64_t ivp_task_wcet_ms;
    ivp_task_wcet_ms = std::stoul(argv[10]);
    LDEBUG << "IvPSim Task WCET: " << ivp_task_wcet_ms << "ms (provided)"; 

  uint64_t kvs_task_wcet_ms;
    kvs_task_wcet_ms = std::stoul(argv[11]);
    LDEBUG << "BFTKVS Task WCET: " << kvs_task_wcet_ms << "ms (provided)"; 

  int fault_mode = std::stoi(argv[12]);
  int number_of_faulty_nodes = std::stoi(argv[13]);

  std::vector<Achal::config_t> bft_kvs_configs;
  for (unsigned nmr_id = 0; nmr_id < num_nmr_instances; nmr_id++) {
    uint64_t nmr_specific_offset_ns = nmr_id * (MS_TO_NS(period_ms) / num_nmr_instances);
    LDEBUG << "Initializing KVS configuration parameters for NMR" << nmr_id;

    LDEBUG << "Is " << num_tasks_per_cpu << " * " << ivp_task_wcet_ms << " + "
           << kvs_task_wcet_ms << " < " << period_ms << "?";
    assert(((num_tasks_per_cpu * ivp_task_wcet_ms) + kvs_task_wcet_ms) <= period_ms);

    Achal::config_t bft_kvs_config;
    bft_kvs_config.period_ns = MS_TO_NS(period_ms);
    bft_kvs_config.offset_ns = nmr_specific_offset_ns + MS_TO_NS((num_tasks_per_cpu * ivp_task_wcet_ms));
    //bft_kvs_config.offset_ns = MS_TO_NS(period_ms - kvs_task_wcet_ms);
    bft_kvs_config.priority = BFT_KVS_DEFAULT_PRIORITY;
    bft_kvs_config.logger = logger;
    bft_kvs_config.max_network_delay_ns = US_TO_NS(BFT_KVS_DEFAULT_NETWORK_DELAY_US);
    bft_kvs_config.max_rounds = BFT_KVS_DEFAULT_NUM_ROUNDS;
    LDEBUG << "Max rounds: " << ((int)bft_kvs_config.max_rounds);
    bft_kvs_config.max_jobs = SEC_TO_NS(exp_duration_s) / bft_kvs_config.period_ns;

    for (int i = 0; i < replication_factor; i++) {
      Achal::process_t p;
      p.id = i;

      if (exp_type == EXP_TYPE_LOCAL) {
        p.ip = node_ips[node_id - 1];
        p.port = bft_kvs_ports[i][(node_id - 1) % 4];
      } else {
        if (node_id <= 4) {
          p.ip = node_ips[i];
        } else {
          p.ip = node_ips[i + 4];
        }
        p.port = bft_kvs_ports[nmr_id][i];
      }

      LDEBUG << "Adding peer with IP " << p.ip << " and port " << p.port;
      bft_kvs_config.peers.push_back(p);
    }
  
    bft_kvs_configs.push_back(bft_kvs_config);
  }


  LDEBUG << "Initializing KVS instances";
  std::vector<Achal::KVSInterface *> kvs_instances;

  if (kvs_type == BFT_KVS) {
    if (exp_type == EXP_TYPE_LOCAL) {
    // For LOCAL, we initiaize replication_factor kvs_instances
      for (int i = 0; i < replication_factor; i++) {
        Achal::config_t& bft_kvs_config = bft_kvs_configs[0];
        bft_kvs_config.port = std::stoi(bft_kvs_ports[i][(node_id - 1) % 4]);
        bft_kvs_config.id = BFT_KVS_TASK_ID(node_id, i);
        bft_kvs_config.cpu = i;
        bft_kvs_config.my_process_id = i;
        kvs_instances.push_back(new Achal::BFTKVS<100, 7, 15>(bft_kvs_config));
        ((Achal::BFTKVS<100, 7, 15>*)kvs_instances.back())->update_fuse_function(Achal::median_of_doubles);
      }
    } else {
    // For DISTRIBUTED, we initialize num_nmr_instances kvs_instances
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
        auto temp = new Achal::BFTKVS<100, 7, 15>(bft_kvs_config);
        temp->update_fuse_function(Achal::median_of_doubles);

        // === FAULT CONFIG START ===
        if (number_of_faulty_nodes > 0) {
          temp->collect_ets = true;
        }
        if(fault_mode == FAULT_MODE_DEAD){
          if(temp->process_id() < number_of_faulty_nodes){
            temp->set_should_die([](uint8_t, uint64_t){
              return true;
            });
          }
        } else if (fault_mode == FAULT_MODE_LIAR){
          if(temp->process_id() < number_of_faulty_nodes){
            temp->set_lie([](uint8_t process_id, uint64_t num_jobs, size_t length){
              return (const uint8_t *) faulty_data;
            });
          }
        }
        // === FAULT CONFIG END ===
        kvs_instances.push_back(temp);
      }
    }

    LDEBUG << "Have all KVS instances connect to servers";
    for (Achal::KVSInterface* kvs_instance : kvs_instances) {
      ((Achal::BFTKVS<100, 7, 15>*)kvs_instance)->connect_to_servers();
    }
    LDEBUG << "Have all KVS instances connect to clients";
    for (Achal::KVSInterface* kvs_instance : kvs_instances) {
      ((Achal::BFTKVS<100, 7, 15>*)kvs_instance)->accept_clients();
    }
  } else if (kvs_type == REDIS_KVS2) {
    if (exp_type == EXP_TYPE_LOCAL) {
      for (int i = 0; i < replication_factor; i++) {
        Achal::config_t& bft_kvs_config = bft_kvs_configs[0];
        Achal::RedisKVS2* kvs_instance = new Achal::RedisKVS2(
            BFT_KVS_TASK_ID(node_id, i), 
            bft_kvs_config.period_ns,
            bft_kvs_config.offset_ns,
            bft_kvs_config.priority,
            i,
            bft_kvs_config.max_jobs,
            std::stoi(bft_kvs_ports[i][(node_id - 1) % 4]),
            get_peers_minus_self(exp_type, node_id, 0, replication_factor, i),
            logger);
        kvs_instance->update_fuse_function();
        kvs_instances.push_back(kvs_instance);
      }
    } else {
      for (int i = 0; i < num_nmr_instances; i++) {
        Achal::config_t& bft_kvs_config = bft_kvs_configs[i];
        Achal::RedisKVS2* kvs_instance = new Achal::RedisKVS2(
            BFT_KVS_TASK_ID(node_id, i), 
            bft_kvs_config.period_ns,
            bft_kvs_config.offset_ns,
            bft_kvs_config.priority,
            i,
            bft_kvs_config.max_jobs,
            std::stoi(bft_kvs_ports[i][(node_id - 1) % 4]),
            get_peers_minus_self(exp_type, node_id, i, replication_factor, (node_id - 1) % 4),
            logger);
        kvs_instance->update_fuse_function();
        kvs_instances.push_back(kvs_instance);
      }
    }
  } else if (kvs_type == ETCD_KVS) {
    if (exp_type == EXP_TYPE_LOCAL) {
      for (int i = 0; i < replication_factor; i++) {
        Achal::config_t& bft_kvs_config = bft_kvs_configs[0];
        Achal::EtcdKVS* kvs_instance = new Achal::EtcdKVS(
            BFT_KVS_TASK_ID(node_id, i), 
            bft_kvs_config.period_ns,
            bft_kvs_config.offset_ns,
            bft_kvs_config.priority,
            i,
            bft_kvs_config.max_jobs,
            std::stoi(bft_kvs_ports[i][(node_id - 1) % 4]),
            get_peers_minus_self(exp_type, node_id, 0, replication_factor, i),
            logger);
        kvs_instance->update_fuse_function();
        kvs_instances.push_back(kvs_instance);
      }
    } else {
      for (int i = 0; i < num_nmr_instances; i++) {
        Achal::config_t& bft_kvs_config = bft_kvs_configs[i];
        Achal::EtcdKVS* kvs_instance = new Achal::EtcdKVS(
            BFT_KVS_TASK_ID(node_id, i), 
            bft_kvs_config.period_ns,
            bft_kvs_config.offset_ns,
            bft_kvs_config.priority,
            i,
            bft_kvs_config.max_jobs,
            std::stoi(bft_kvs_ports[i][(node_id - 1) % 4]),
            get_peers_minus_self(exp_type, node_id, i, replication_factor, (node_id - 1) % 4),
            logger);
        kvs_instance->update_fuse_function();
        kvs_instances.push_back(kvs_instance);
      }
    }
  } else {
    LDEBUG << "Error: kvs_type " << kvs_type << " not supported";
    return 1;
  }

  // LOCAL: #tasks = replication_factor * num_tasks_per_cpu
  // DIDSTRIBUTED: #tasks = num_nmr_instances * num_tasks_per_cpu
  // LOCAL OR DISTRIBUTED: #tasks = cpus_used * num_task_per_cpu
  LDEBUG << "Mapping benchmark instance to KVS-backed periodic tasks";
  std::vector<Applications::IvPSim*> tasks;
  for (int cpu = 0; cpu < cpus_used; cpu++) {
    for (int unique_task = 0; unique_task < num_tasks_per_cpu; unique_task++) {
      uint64_t nmr_specific_offset_ns = cpu * (MS_TO_NS(period_ms) / cpus_used);
      uint64_t periodic_task_offset_ms = NS_TO_MS(nmr_specific_offset_ns) + (unique_task * ivp_task_wcet_ms);
      Applications::IvPSim* periodic_task =
        new Applications::IvPSim(
           PERIODIC_TASK_ID(node_id, cpu, unique_task),
           MS_TO_NS(period_ms),
           MS_TO_NS(periodic_task_offset_ms),
           get_period_based_priority(period_ms),
           cpu,
           kvs_instances[cpu],
           (SEC_TO_NS(exp_duration_s) / MS_TO_NS(period_ms)),
           logger);
      if (number_of_faulty_nodes > 0) {
        periodic_task->collect_ets = true;
      }
      tasks.push_back(periodic_task);
    }
  }

  LDEBUG << "Releasing all tasks in 5s";
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
    kvs_instances[i]->print_stats();
  }

  for (int i = 0; i < tasks.size(); i++) {
    tasks[i]->print_stats();
  }

  AggregateStats stats;

  for (int i = 0; i < tasks.size(); i++) {
    if (tasks[i]->did_ivp_stabilize()) {
      LDEBUG << "IvPSim Task " << tasks[i]->id << " successful";
      stats.control_status = "success";
    } else {
      LDEBUG << "IvPSim Task " << tasks[i]->id << " FAILED";
      stats.control_status = "failure";
    }
  }

  stats.workload = "IvP1";
  stats.exp_duration_s = exp_duration_s;
  stats.num_tasks_per_cpu = num_tasks_per_cpu;
  stats.num_cpus_used = cpus_used;
  stats.period_ms = period_ms;
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

  output_file_path = Utils::get_project_directory() + "/data/" + output_file_name + ".et";
  std::ofstream ofs;
  ofs.open(output_file_path, std::ios_base::app);

  uint64_t num_jobs = SEC_TO_MS(exp_duration_s) / period_ms;
  std::stringstream ss;
  ss << "Job";
  for (int i = 0; i < tasks.size(); i++) {
    ss << ", IvPSim" << tasks[i]->id;
  }
  for (int i = 0; i < kvs_instances.size(); i++) {
    ss << ", KVS" << kvs_instances[i]->id;
  }
  ss << std::endl;
  for (int i = 0; i < num_jobs; i++) {
    ss << i;
    for (int j = 0; j < tasks.size(); j++) {
      if (tasks[j]->collect_ets) {
        ss << ", " << tasks[j]->ets_ms[i];
      }
    }
    for (int k = 0; k < kvs_instances.size(); k++) {
      if (kvs_instances[k]->collect_ets) {
        ss << ", " << kvs_instances[k]->ets_ms[i];
      }
    }
    ss << std::endl;
  }
  ofs << ss.str() << std::endl;
  ofs.close();

  return 0;
}

#ifndef ACHAL_BFT_KVS_H
#define ACHAL_BFT_KVS_H

#include <stdint.h>
#include <vector>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <boost/asio.hpp>
#include <mutex>
#include <set>
#include <cassert>
#include <thread>

#include "utils/misc.h"
#include "utils/timespec.h"
#include "utils/periodic_task.h"
#include "utils/kzh15_benchmark_generator.h"
#include "achal/eig_tree.h"
#include "achal/tcp/client.h"
#include "achal/kvs_interface.h"

#define KVS_GB_KEEP_N_VALUES 20

#define CONTAINS(LIST, ITEM) \
    (std::find((LIST).begin(), (LIST).end(), (ITEM)) != (LIST).end())
#define CONTAINS_KEY(MAP, KEY) ((MAP).find(KEY) != (MAP).end())

using namespace std::chrono_literals;
using boost::asio::ip::tcp;

namespace Achal {

typedef std::string (*FuseFunction)(std::vector<std::string> list);

bool simpleMajority(std::vector<std::string> values, std::string &majority);
std::string fuse_default(std::vector<std::string> values);

template<size_t KS, size_t VS>
struct tuple {
  char key[KS + 1];
  char value[VS + 1];
  uint64_t time;
};

typedef struct process {
  int id;
  const char *ip;
  const char *port;
} process_t;

template<size_t NT, size_t KS, size_t VS>
struct eig_node {
  uint32_t length;
  struct tuple<KS, VS> tuples[NT];
};

template<size_t NT, size_t KS, size_t VS>
using fuse_function_t = void (*)(
  std::map<std::string, std::map<uint64_t, std::string, std::greater<uint64_t>>> &store, 
  struct eig_node<NT, KS, VS> * nodes, 
  int num_nodes
);

using should_die_function_t = bool (*)(uint8_t process_id, uint64_t num_jobs);

using lie_function_t = const uint8_t * (*)(
  uint8_t process_id,
  uint64_t num_jobs,
  size_t length 
);

template<size_t NT, size_t KS, size_t VS>
using record_result_function_t = void (*)(
  uint8_t process_id,
  uint64_t num_jobs,
  const struct eig_node<NT, KS, VS> * vector, 
  size_t length
);

typedef struct config {
  // base class constructor params
  unsigned id;
  uint64_t period_ns;
  uint64_t offset_ns;
  unsigned priority;
  unsigned cpu;
  log4cpp::Category *logger;

  // this class specific params
  unsigned port;
  std::vector<process_t> peers;
  uint64_t max_network_delay_ns;
  uint8_t max_rounds;
  uint8_t my_process_id;
  uint64_t max_jobs;
} config_t;

template<size_t NT, size_t KS, size_t VS>
void median_of_doubles(
  std::map<std::string,
          std::map<uint64_t, std::string, std::greater<uint64_t>>> &store,
  struct Achal::eig_node<NT, KS, VS> *nodes, int num_nodes) {

  assert(num_nodes > 0);
  std::unordered_map<std::string, std::unordered_map<uint64_t, std::unordered_map<int, double>>> map;

  for(int i = 0; i < num_nodes; i++){
    if(nodes[i].length > NT){
      // nodes[i].length is required to be <= NT
      // when it is not, it is likely that either
      // 1. the code is buggy, or
      // 2. the peer that proposed this node is byzantine, or
      // 3. more than 1/3 of the cluster is byzantine.
      // in any case, data proposed by this peer cannot be trusted
      // ignore
      continue;
    }
    for(int j = 0; j < nodes[i].length; j++){
      struct tuple<KS, VS> & t = nodes[i].tuples[j];
      t.key[KS] = '\0';
      t.value[VS] = '\0';
      try{
        double value = std::stod(std::string(t.value));
        map[std::string(t.key)][t.time][i] = value;
      }catch(std::invalid_argument e){
        // std::cout << "String " << t.value << " cannot be converted to double: " << e.what();
        // strings like "0.1" can be converted to double but,
        // strings like "0.ab1" cannot.
        // likely because bug in code, or
        // the peer who proposed this string is byzantine
        // therefore, we ignore this data.
      }
    }
  }

  for(auto &p1 : map){
    auto &bucket = store[p1.first];
    for(auto &p2 : p1.second){
      if(p2.second.size() < 3){
        continue;
      }
      const uint64_t & time = p2.first;
      std::vector<double> values;
      for(auto &p3 : p2.second){
        values.push_back(p3.second);
      }
      std::sort(values.begin(), values.end());
      double median = (values[values.size() / 2] + values[(values.size() - 1) / 2]) / 2;
      bucket[time] = std::to_string(median);
      if(bucket.size() > KVS_GB_KEEP_N_VALUES){
        bucket.erase(--bucket.end());
      }
    }
  }  

}

template<size_t NT, size_t KS, size_t VS>
void strong_simple_majority_of_strings(
  std::map<std::string,
          std::map<uint64_t, std::string, std::greater<uint64_t>>> &store,
  struct Achal::eig_node<NT, KS, VS> *nodes, int num_nodes) {

  assert(num_nodes > 0);
  std::unordered_map<std::string, std::unordered_map<uint64_t, std::unordered_map<int, std::string>>> ktnv;

  for(int i = 0; i < num_nodes; i++){
    if(nodes[i].length > NT){
      continue;
    }
    for(int j = 0; j < nodes[i].length; j++){
      struct tuple<KS, VS> & t = nodes[i].tuples[j];
      t.key[KS] = '\0';
      t.value[VS] = '\0';
      ktnv[std::string(t.key)][t.time][i] = std::string(t.value);
    }
  }

  for(auto &p1 : ktnv){
    auto &bucket = store[p1.first];
    for(auto &p2 : p1.second){
      std::unordered_map<std::string, int> freq;
      for(auto & p3 : p2.second){
        freq[p3.second]++;
      }
      for(auto &p4 : freq){
        if(p4.second > (num_nodes / 2.0)){
          bucket[p2.first] = p4.first;
          if(bucket.size() > KVS_GB_KEEP_N_VALUES){
            bucket.erase(--bucket.end());
          }
          break;
        }
      }
    }
  }  
    
}

template<size_t NT, size_t KS, size_t VS>
class BFTKVS : public KVSInterface {
 private:
  fuse_function_t<NT, KS, VS> fuse_function = NULL;
  config_t config;
  // tcp specific
  boost::asio::io_context io_context;
  boost::asio::deadline_timer timer;
  boost::asio::ip::tcp::acceptor acceptor;
  boost::asio::ip::tcp::resolver resolver;
  std::map<uint8_t, boost::asio::ip::tcp::socket> client;
  std::map<uint8_t, boost::asio::ip::tcp::socket> server;
  std::vector<boost::asio::streambuf> server_buffer;
  std::string delim = "===============================";
  uint64_t one_round_us;

  // kvs specific
 public:
  const static int keep_n_values = KVS_GB_KEEP_N_VALUES;
  std::map<std::string, std::map<uint64_t, std::string, std::greater<uint64_t>>>
  store;
  struct eig_node<NT, KS, VS> unpublished;
  EIGTree<struct eig_node<NT, KS, VS>> tree;
  uint64_t total_run_time = 0;

 // fault tolerance
 private:
  should_die_function_t should_die = NULL;
  lie_function_t lie = NULL;
  record_result_function_t<NT, KS, VS> record_result = NULL;

 public:
  BFTKVS(config_t conf)
    : KVSInterface(conf.id, conf.period_ns, conf.offset_ns, conf.priority,
                  conf.cpu, conf.logger),
      resolver(io_context),
      acceptor(io_context, tcp::endpoint(tcp::v4(), conf.port)),
      config(conf),
      tree(conf.peers.size(), conf.max_rounds),
      timer(io_context),
      server_buffer(conf.peers.size()) {
    assert(conf.peers.size() <= UINT8_MAX);
    //one_round_us = std::max((conf.period_ns - conf.offset_ns) / 2 / conf.max_rounds / 1000, ARPAN_OFFICE_PI_TIMEOUT_US);
    one_round_us = (conf.period_ns - conf.offset_ns) / 2 / conf.max_rounds / 1000;
    //one_round_us = NS_TO_US(((conf.period_ns - conf.offset_ns) / conf.max_rounds) * 0.75);
    std::memset(&unpublished, 0, sizeof(struct eig_node<NT, KS, VS>));
  };

  ~BFTKVS(){
    for(auto &s : server){
      s.second.close();
    }
    for(auto &c : client) {
      c.second.close();
    }
  }

  uint8_t process_id(){
    return config.my_process_id;
  }

  void connect_to_servers(){
    for (process_t &p : config.peers) {
      if (p.id == config.my_process_id) {
        continue;
      }
      tcp::socket s(io_context);
      for (;;) {
        // the client socket connects until successful
        try {
          boost::asio::connect(s, resolver.resolve(p.ip, p.port));
          break;
        } catch (std::exception &e) {
          config.logger->alert("Exception with %s:%s. Message: %s", p.ip, p.port,
                              e.what());
          using namespace std::chrono_literals;
          std::this_thread::sleep_for(500ms);                     
        }
      }
      // client sockets send my process id
      s.send(boost::asio::buffer(&config.my_process_id,
                                sizeof(uint8_t)));
      client.insert(std::make_pair(p.id, std::move(s)));
    }

    // verify that I am connected to every peers
    for (int i = 0; i < config.peers.size(); i++) {
      if (i != config.my_process_id) {
        assert(client.find(i) != client.end());
      }
    }

    // verify I am not connected to myself
    assert(client.find(config.my_process_id) == client.end());
  }

  void accept_clients(){
    for (int i = 0; i < config.peers.size() - 1; i++) {
      // accept clients
      uint8_t process_id;
      tcp::socket s = acceptor.accept();

      // receive and store their id
      s.receive(boost::asio::buffer(&process_id, sizeof(uint8_t)));
      assert(process_id < config.peers.size());
      assert(server.find(process_id) == server.end());
      server.insert(std::make_pair(process_id, std::move(s)));
    }

    // verify that every peers has connected to me
    for (int i = 0; i < config.peers.size(); i++) {
      if (i != config.my_process_id) {
        assert(client.find(i) != client.end());
      }
    }

    // verify I have not connected myself
    assert(client.find(config.my_process_id) == client.end());
  }
  void update_fuse_function(fuse_function_t<NT, KS, VS> ff){
    fuse_function = ff;
  }
  
  void set_should_die(should_die_function_t sdf) {
    should_die = sdf;
  }
  void set_lie(lie_function_t lf){
    lie = lf;
  }
  void set_record_result(record_result_function_t<NT, KS, VS> rrf) {
    record_result = rrf;
  }

  void job(){
    //int node_id = (id / 1000) % 4;
    ////uint64_t num_jobs_in_tens = num_jobs / 10;
    //bool exp_dead = (node_id == 0);
    ////bool exp_dead = (num_jobs_in_tens % 4 == node_id);

    //if(pretend_dead or exp_dead){

    if(should_die != NULL && should_die(config.my_process_id, num_jobs)){
      std::memset(&unpublished, 0, sizeof(struct eig_node<NT, KS, VS>));
      num_jobs++;
      return;
    }
    uint64_t start = Utils::get_time_now_ns();
    // initialization
    *tree.root() = unpublished;
    std::memset(&unpublished, 0, sizeof(struct eig_node<NT, KS, VS>));

    for (int l = 0; l < config.max_rounds; l++) {
      tree.copy_current_level(l, config.my_process_id);

      io_context.reset();
      std::set<uint8_t> send_pending;
      std::set<uint8_t> receive_pending;

      for (auto &p : client) {

        uint8_t pid = p.first;

        std::vector<boost::asio::const_buffer> bufs;
        bufs.push_back(boost::asio::buffer(delim));
        
        size_t len = tree.buffer_length_optimized(l, pid);

        // if configured to lie to other nodes
        const uint8_t * faulty_data = NULL;
        if(lie != NULL){
          faulty_data = lie(config.my_process_id, num_jobs, len);
        }

        if(faulty_data != NULL){
          // send the faulty data instead of actual data
          bufs.push_back(boost::asio::const_buffer(faulty_data, len));
        }else{
          // otherwise send the actual data
          tree.send_buffer_optimized(l, config.my_process_id, bufs);
        }

        send_pending.insert(pid);
        boost::asio::async_write(
          p.second, 
          bufs,
          boost::asio::transfer_all(), 
          [pid, &send_pending](const boost::system::error_code& ec, std::size_t n){
            send_pending.erase(pid);
          }
        );
      }

      for (auto &p : server) {

        uint8_t pid = p.first;
        receive_pending.insert(pid);

        boost::asio::async_read_until(
          p.second, 
          server_buffer[pid], 
          delim, 
          [this, l, pid, &receive_pending](const boost::system::error_code& ec, std::size_t n){

            server_buffer[pid].consume(n);
            size_t len = tree.receive_buffer_optimized(l, pid).size() * sizeof(struct eig_node<NT, KS, VS>);
            
            boost::asio::async_read(
              server.find(pid)->second, 
              server_buffer[pid],
              boost::asio::transfer_exactly(len - server_buffer[pid].size()),
              [this, l, pid, &receive_pending](const boost::system::error_code& ec, std::size_t n){
                for(auto &p : tree.receive_buffer_optimized(l, pid)){
                  server_buffer[pid].sgetn(p.first, p.second);
                }
                receive_pending.erase(pid);
              }
            );
          }
        );
      }

      bool timeout = false;
      auto handler = [&timeout](const boost::system::error_code& ec){
        timeout = true;
      };
      timer.expires_from_now(boost::posix_time::microsec(one_round_us));
      timer.async_wait(handler);

      bool printed = false;

      while(io_context.run_one()){
        if(send_pending.empty() && receive_pending.empty()){
          timer.cancel();
        }else if(timeout){
          if(!printed){
            printed = true;
            //LDEBUG << "Process " << ((int) config.my_process_id) 
            //<< " round " << l 
            //<< " num_jobs " << num_jobs
            //<< " times out after " << (one_round_us / 1000) << "ms";
          }
          for(auto pid : send_pending){
            client.find(pid)->second.cancel();
          }
          for(auto pid : receive_pending){
            server.find(pid)->second.cancel();
          }
        }
      }
    }

    // reduction
    auto result = tree.reduce();

    if (record_result != NULL){
      auto icv = tree.interactive_consistency_vector();
      record_result(config.my_process_id, num_jobs, icv.first, icv.second);
    }

    if (fuse_function != NULL){
      // if a custom fuse function is provided, call it
      auto icv = tree.interactive_consistency_vector();
      fuse_function(store, icv.first, icv.second);
    } else{ // else use the default fuse function, which is simple majority

      // result->length is required to be <= NT
      // the code thrives to keep it that way
      // if this is not true, then there are likely more than f Byzantine peers
      // no one can be trusted, so ignore data
      if(result->length <= NT){ 

        auto &t = result->tuples;
        for (int i = 0; i < result->length; i++) {

          // MEMORY SAFETY
          t[i].key[KS] = '\0';
          t[i].value[VS] = '\0';

          // insert
          auto &bucket = store[std::string(t[i].key)];
          bucket[t[i].time] = std::string(t[i].value);
          // LDEBUG << "Commited value t=" << t[i].time << ", v=" << bucket[t[i].time];

          // after insert, if too many values are in the bucket
          // remove old ones
          if(bucket.size() > keep_n_values){
            auto p = bucket.end();
            //LDEBUG << "Garbage collecting value t=" << p->first << ", v=" << p->second;
            bucket.erase(--bucket.end());
          }
        }
      }
    }

    tree.clear();

    num_jobs++;
    total_run_time += (Utils::get_time_now_ns() - start);
  }
  void job_for_profiler(){
    job();
  }
  bool terminate(){
    return num_jobs >= config.max_jobs;
  }

  bool try_read(std::string key, uint64_t no_earlier_than,
                        std::string &value) {
    // no_earlier_than should be in the past
    uint64_t now = Utils::get_time_now_ns();
    if (no_earlier_than > now) {
      //LDEBUG << "BFTKVS" << id << "::try_read " << key << " [" << no_earlier_than
      //       << ", " << now << "] failed at 1";
      return false;
    }

    // store should have record of this key
    auto r1 = store.find(key);
    if (r1 == store.end()) {
      //LDEBUG << "BFTKVS" << id << "::try_read " << key << " [" << no_earlier_than
      //       << ", " << now << "] failed at 2";
      return false;
    }
    
    for(auto it = r1->second.begin(); it != r1->second.end(); it++){
      uint64_t time = it->first;
      if((time <= now) && (no_earlier_than <= time)){
        value = it->second;
        return true;
      }
      if(no_earlier_than > time){
        //LDEBUG << "BFTKVS" << id << "::try_read " << key << " [" << no_earlier_than
        //       << ", " << now << "] failed at 3";
        return false;
      }
    }

    return false;
  }
  bool try_write(std::string key, uint64_t publish_time, std::string value){

    // publish time must be in the future
    if (publish_time <= Utils::get_time_now_ns()) {
      LDEBUG << "BFTKVS Task " << id << " " << publish_time << " <= "
            << Utils::get_time_now_ns() << "!!!";
      return false;
    }

    // check that the given ktv pair can fit into the tree
    assert(key.size() <= KS);
    if (value.size() > VS) {
      LDEBUG << value << " / " << value.size() << " / " << VS;
    }
    assert(value.size() <= VS);

    // check that we have enough buffer
    // reject write if there is too many unpublished ktv tuples
    if(unpublished.length >= NT){
      LDEBUG << "BFTKVS Task " << id << " " << unpublished.length << " >= "
            << NT << "!!!";
      return false;
    }

    // write data to unpublished data storage
    struct tuple<KS, VS> &t = unpublished.tuples[unpublished.length++];
    strcpy(t.key, key.c_str());
    t.time = publish_time;
    strcpy(t.value, value.c_str());
    return true;
  }

  void print_stats(){
    TimeSpec diff_ts;
    timespecsub(&task_finish_ts, &task_start_ts, &diff_ts);
    LDEBUG << "BFTKVS Task " << id << " actual (expected) duration: "
          << TS_TO_MS(&diff_ts) << "ms (" << config.max_jobs * NS_TO_MS(time_period_ns)
          << "ms), BCET (measured): " << bcet_measured_ms
          << "ms, ACET (measured): " << acet_measured_ms
          << "ms, WCET (measured): " << wcet_measured_ms << "ms";
  }

  void print(){}
};

} // namespace Achal

#endif // ACHAL_BFT_KVS_H

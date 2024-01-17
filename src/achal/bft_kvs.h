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
// #include "utils/kzh15_benchmark_generator.h"
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

template<size_t NT, size_t KS, size_t VS>
void median_of_doubles(
  std::map<std::string,
          std::map<uint64_t, std::string, std::greater<uint64_t>>> &store,
  struct Achal::eig_node<NT, KS, VS> *nodes, int num_nodes) {

  assert(num_nodes > 0);
  std::unordered_map<std::string, std::unordered_map<uint64_t, std::unordered_map<int, double>>> map;

  for(int i = 0; i < num_nodes; i++){
    // std::cout<<"node num: "<<i<<std::endl;
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
        // std::cout<<"key: "<< std::string(t.key)<<" | value: "<<value<<std::endl<<" | time: " <<t.time;
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
  // std::cout<<"entered fuse function"<<std::endl;
  // std::cout<<map.size()<<std::endl;
  for(auto &p1 : map){
    // std::cout<<"iterating over keys"<<std::endl;
    auto &bucket = store[p1.first];
    for(auto &p2 : p1.second){
      // std::cout<<"p2 size: "<<p2.second.size()<<std::endl;
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

  // std::cout<<"Done iterating over keys"<<std::endl;
}

template<size_t NT, size_t KS, size_t VS>
void median_of_doubles_null_ignore(
  std::map<std::string,
          std::map<uint64_t, std::string, std::greater<uint64_t>>> &store,
  struct Achal::eig_node<NT, KS, VS> *nodes, int num_nodes) {
    // std::cout<<"null ignore"<<std::endl;
  // std::cout<<"Start fuse"<<std::endl;
  assert(num_nodes > 0);
  std::unordered_map<std::string, std::unordered_map<uint64_t, std::unordered_map<int, double>>> map;
  std::unordered_map<std::string, std::unordered_map<uint64_t, int>> passer_count;
  for(int i = 0; i < num_nodes; i++){
    // std::cout<<"node num: "<<i<<std::endl;
    const char* compareString="pass";
    if(nodes[i].length > NT){ // there are more terms than possible
      // nodes[i].length is required to be <= NT
      // when it is not, it is likely that either
      // 1. the code is buggy, or
      // 2. the peer that proposed this node is byzantine, or
      // 3. more than 1/3 of the cluster is byzantine.
      // in any case, data proposed by this peer cannot be trusted
      // ignore
      continue;
    }
    for(int j = 0; j < nodes[i].length; j++){ // for each key value pair from node
      struct tuple<KS, VS> & t = nodes[i].tuples[j];
      t.key[KS] = '\0';
      t.value[VS] = '\0';
      int mnop=0;

      if(!strcmp(t.value, compareString)){ // node i has put "pass" as the value for this key
        // std::cout<<"Node "<< i+1 << " is not a writer of key \"" <<t.key << "\" for timestamp: "<< t.time<<std::endl;
        
        // keep track of how many nodes have passed on writing
        if(!(passer_count.count(t.key))) passer_count[t.key][t.time]=1;
        else if(passer_count[t.key].count(t.time)) passer_count[t.key][t.time]++;
        else  passer_count[t.key][t.time]=1;
        
        continue;
      }
      else {
        try{
          // std::cout<<"else entered"<<std::endl;
          double value = std::stod(std::string(t.value));
          map[std::string(t.key)][t.time][i] = value;
          // std::cout<< "node " << i+1 <<" wrote: key: "<< std::string(t.key)<<" | value: "<<value<<" @ time: "<< t.time<<std::endl;

        }catch(std::invalid_argument e){
          // std::cout << "String " << t.value << " cannot be converted to double: " << e.what()<<std::endl;
          // strings like "0.1" can be converted to double but,
          // strings like "0.ab1" cannot.
          // likely because bug in code, or
          // the peer who proposed this string is byzantine
          // therefore, we ignore this data.
        }
      }
    }
  }
  // std::cout<<map.size()<<std::endl;
  // std::cout<<"iterating over keys"<<std::endl;
  for(auto &p1 : map){ // for key in map
    // std::cout<<"key: "<<p1.first<<std::endl;
    auto &bucket = store[p1.first];
    for(auto &p2 : p1.second){ // for timestamp in timestamp-value map (same key)
      // std::cout<<"p2 size: "<<p2.second.size()<<std::endl;
      
      // I don't see why this is necessary so I am commenting this bock out
      // if(p2.second.size()!=num_nodes-passer_count[p1.first][p2.first]){ // if number of nodes - number of passers does not match the size (this doesn't make sense
      // // because we could have nodes that didn't write anything)
      //   // std::cout<< "key \""<< p1.first <<"\" has "<< p2.second.size() <<" values but only " 
      //     // << passer_count[p1.first][p2.first] << " have skipped writing a value out of "
      //     // << num_nodes <<" nodes."<<std::endl;
      //   // std::cout<<"key: "<<p1.first<<" does not have sufficient writers for timestamp "<< p2.first <<std::endl;
      //   continue;
      // }

      const uint64_t & time = p2.first;
      std::vector<double> values;
      for(auto &p3 : p2.second){ // for node in timestamp
        // std::cout<<"value :" << p3.second<<std::endl;
        values.push_back(p3.second);
      }
      std::sort(values.begin(), values.end());
      double median = (values[values.size() / 2] + values[(values.size() - 1) / 2]) / 2;
      bucket[time] = std::to_string(median);
      // std::cout<<"key "<<p1.first<<" has value "<<median<<" for time " <<time<<std::endl;
      if(bucket.size() > KVS_GB_KEEP_N_VALUES){
        bucket.erase(--bucket.end());
      }
    }
  }  

  // std::cout<<"done fuse"<<std::endl;

  // std::cout<<"Done iterating over keys"<<std::endl;
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

template<size_t NT, size_t KS, size_t VS> //KS: keysize VS: value size NT: number of terms?
class BFTKVS : public KVSInterface {
 public: 
  class ReconnectTask : public Utils::PeriodicTask{
    private:
      BFTKVS* bft_kvs;
      config_t config;
      uint8_t expected_connection_count;
    public:
      ReconnectTask(BFTKVS* bft_kvs):  
      Utils::PeriodicTask(bft_kvs->config.id+1, bft_kvs->config.period_ns, bft_kvs->config.offset_ns+bft_kvs->config.wcet_kvs, bft_kvs->config.priority, bft_kvs->config.cpu, bft_kvs->config.logger),
      bft_kvs(bft_kvs), config(bft_kvs->config){
      }
      void job(){
        std::map<uint8_t, boost::asio::ip::tcp::socket> reconnected_clients;
        std::vector<process_t> reconnect_list;
        if(bft_kvs->server.size() < (config.peers.size()-1)){
          std::cout<<"MISSING CONNECTION"<<std::endl;
          // check which ones are missing
          for(process_t &p: config.peers){
            if(p.id==config.my_process_id) continue;
            if(bft_kvs->server.find(p.id)==bft_kvs->server.end()){
            tcp::socket s(bft_kvs->io_context);
            // TODO: make this timed or iteration based
              for (;;) {
                // the client socket connects for some time
                try {
                  boost::asio::connect(s, bft_kvs->resolver.resolve(p.ip, p.port));
                  reconnect_list.push_back(p);
                  break;
                } catch (std::exception &e) {
                  config.logger->alert("Exception with %s:%s. Message: %s", p.ip, p.port,
                                      e.what());
                  using namespace std::chrono_literals;
                  std::this_thread::sleep_for(500ms);                     
                }
              }
              s.send(boost::asio::buffer(&config.my_process_id, sizeof(uint8_t)));
              reconnected_clients.insert(std::make_pair(p.id, std::move(s)));
              std::cout<<"RECEIVER IS CONNECTED"<<std::endl;
            }
          }

          for(int i=0; i<reconnect_list.size();i++){
            std::cout<<"TRY ACCEPT"<<std::endl;
            uint8_t process_id;

            // could make this timeout instead of block.
            // then again, the assumptions of the startup connection code match this. 
            tcp::socket s = bft_kvs->acceptor.accept();
            
            // receive and store their id
            s.receive(boost::asio::buffer(&process_id, sizeof(uint8_t)));
            // if(process_id==reconnect_list[i].id){
            assert(process_id < config.peers.size());
            assert(bft_kvs->server.find(process_id) == bft_kvs->server.end());
            bft_kvs->server.insert(std::make_pair(process_id, std::move(s)));
            // }
            std::cout<<"ACCEPTOR HAS CONNECTED"<<std::endl;
          }

          for (auto& reconnected_client : reconnected_clients) {
              auto key = std::move(reconnected_client.first); // Move the key
              auto value = std::move(reconnected_client.second); // Move the value
              bft_kvs->client.emplace(std::move(key), std::move(value));
              std::cout << "RECONNECTED CLIENT ADDED" << std::endl;
          }

        }
        else{
          std::cout<<"NO MISSING CONNECTIONS"<<std::endl;
        }
        num_jobs++;
      }

      void job_for_profiler(){
        job();
      };

      bool terminate(){
        return bft_kvs->terminate_flag;
      };
  };

 private:
  fuse_function_t<NT, KS, VS> fuse_function = NULL;
  config_t config;
  // tcp specific
  boost::asio::io_context io_context;
  boost::asio::deadline_timer timer;
  boost::asio::ip::tcp::acceptor acceptor;
  boost::asio::ip::tcp::resolver resolver;
  std::map<uint8_t, boost::asio::ip::tcp::socket> client;
  std::map<uint8_t, uint8_t> failed_comm_count;
  std::vector<boost::asio::streambuf> server_buffer;
  std::string delim = "===============================";
  uint64_t one_round_us;
  uint8_t failure_time_multiple;
  uint8_t max_failed_comms;

  // kvs specific
 public:
  std::map<uint8_t, boost::asio::ip::tcp::socket> server;
  const static int keep_n_values = KVS_GB_KEEP_N_VALUES;
  std::map<std::string, std::map<uint64_t, std::string, std::greater<uint64_t>>>
  store;
  struct eig_node<NT, KS, VS> unpublished;
  EIGTree<struct eig_node<NT, KS, VS>> tree;
  uint64_t total_run_time = 0;
  ReconnectTask* reconnect_task;

 // fault tolerance
 private:
  should_die_function_t should_die = NULL;
  lie_function_t lie = NULL;
  record_result_function_t<NT, KS, VS> record_result = NULL;

 public:


  BFTKVS(config_t conf):KVSInterface(conf.id, conf.period_ns, conf.offset_ns, conf.priority,
                  conf.cpu, conf.logger)
                  ,resolver(io_context),
      acceptor(io_context, tcp::endpoint(tcp::v4(), conf.port)),
      config(conf),
      tree(conf.peers.size(), conf.max_rounds),
      timer(io_context),
      server_buffer(conf.peers.size()), failure_time_multiple(conf.failure_time_multiple),
      max_failed_comms(conf.max_failed_comms) 
  {
    assert(config.logger != NULL);
    std::cout<<"bft kvs constructor"<<std::endl;
    reconnect_task = new ReconnectTask(this);
    assert(conf.peers.size() <= UINT8_MAX);
    //one_round_us = std::max((conf.period_ns - conf.offset_ns) / 2 / conf.max_rounds / 1000, ARPAN_OFFICE_PI_TIMEOUT_US);
    one_round_us = (conf.period_ns - conf.offset_ns) / 2 / conf.max_rounds / 1000;
    //one_round_us = NS_TO_US(((conf.period_ns - conf.offset_ns) / conf.max_rounds) * 0.75);
    std::memset(&unpublished, 0, sizeof(struct eig_node<NT, KS, VS>));
  }
  // catch(const std::exception& e){
  //   std::cerr<<"Initialization error: "<< e.what() <<std::endl;
  // };

  ~BFTKVS(){
    for(auto &s : server){
      s.second.close();
    }
    for(auto &c : client) {
      c.second.close();
    }
    delete reconnect_task;
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
      failed_comm_count.insert(std::make_pair(process_id, 0));
    }

    // verify that every peers has connected to me
    std::cout<<"verify everyone is connected"<<std::endl;
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
    // std::cout<<"NT: "<<NT<<std::endl;
    // if(num_jobs<10){
      for (auto &p : server) {
        uint8_t pid = p.first;
        server_buffer[pid].consume(server_buffer[pid].size());
      }
    // }

    if(should_die != NULL && should_die(config.my_process_id, num_jobs)){
      std::memset(&unpublished, 0, sizeof(struct eig_node<NT, KS, VS>));
      num_jobs++;
      return;
    }
    uint64_t start = Utils::get_time_now_ns();
    // initialization
    *tree.root() = unpublished;
    std::memset(&unpublished, 0, sizeof(struct eig_node<NT, KS, VS>));
    std::set<uint8_t> failed_peers;
    std::cout<<"unpublished len from job "<< unpublished.length<<std::endl;

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
        LDEBUG<<"sending to : " <<  static_cast<int>(pid);

        
        for(const boost::asio::const_buffer& buffer:bufs){
          const char* data = boost::asio::buffer_cast<const char*>(buffer);
          std::size_t length = boost::asio::buffer_size(buffer);
          // std::cout<<"Buffer Data: " <<std::string(data,length)<<std::endl;
        }

        boost::asio::async_write(
          p.second, 
          bufs,
          boost::asio::transfer_all(), 
          [pid, &send_pending](const boost::system::error_code& ec, std::size_t n){
            if(!ec){
              send_pending.erase(pid);
              // LDEBUG<<"Bytes written: "<< n << "to PID " << pid;
            } else{
              // std::cout<<"Error during write: " <<ec.message()<<std::endl;
            }
          }
        );
      }

      // std::cout<<"starting receive loop"<<std::endl;

      for (auto &p : server) {

        uint8_t pid = p.first;
        // std::cout<<"trying to rec from: " <<  static_cast<int>(pid) <<std::endl;
        receive_pending.insert(pid);

        boost::asio::async_read_until(
          p.second, 
          server_buffer[pid], 
          delim, 
          [this, l, pid, &receive_pending](const boost::system::error_code& ec, std::size_t n){

            
            // if(!ec){
            //   std::string recieved_data=boost::asio::buffer_cast<const char*>(server_buffer[pid].data());
            //   std::cout<<"Received data fomr peer "<<(int) pid<<": "<<recieved_data<<std::endl;
            //   server_buffer[pid].consume(n);
            // } else{
            //   std::cout<<ec.message()<<std::endl;
            // }
            // receive_pending.erase(pid);
            server_buffer[pid].consume(n);
            // server_buffer[pid].consume(server_buffer[pid].size());
            size_t len = tree.receive_buffer_optimized(l, pid).size() * sizeof(struct eig_node<NT, KS, VS>);
            // std::cout << "Len"<<len <<std::endl;
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
            // failed_comm_count
            printed = true;
            LDEBUG << "Process " << ((int) config.my_process_id) 
            << " round " << l 
            << " num_jobs " << num_jobs
            << " times out after " << (one_round_us / 1000) << "ms";
            for(auto pid : send_pending){
              failed_peers.insert(pid);
              LDEBUG << "Failed to send " << static_cast<int>(pid); 
            }
            for(auto pid : receive_pending){
              LDEBUG << "Failed to receive " << static_cast<int>(pid); 
              failed_peers.insert(pid);
            }
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

    for(uint8_t pid : failed_peers){
      // std::cout<<"failed pid: " << static_cast<int>(pid)<<std::endl;
      failed_comm_count[pid]++;
      // std::cout<<"fail count: "<<static_cast<int>(failed_comm_count[pid])<<std::endl;
    }

    std::set<uint8_t> disconnected_nodes;
    if(!(num_jobs % failure_time_multiple)){
      for(auto& pid : failed_comm_count){
        if(pid.second>max_failed_comms){
          std::cout<<"pid "<<static_cast<int>(pid.first)<<" has failed too often, disconnect peer"<<std::endl;
          disconnected_nodes.insert(pid.first);
          server.erase(server.find(pid.first));
          std::cout<<"ERASED"<<std::endl;
          client.erase(client.find(pid.first));
          std::cout<<"ERASED"<<std::endl;
        }
        failed_comm_count[pid.first]=0;
      }
    }

    for(uint8_t pid: disconnected_nodes){
      failed_comm_count.erase(failed_comm_count.find(pid));
    }

    // reduction
    auto result = tree.reduce();
    // std::cout<<"result length: "<<result->length<<" | NT: "<<NT<<std::endl;

    if (record_result != NULL){
      auto icv = tree.interactive_consistency_vector();
      record_result(config.my_process_id, num_jobs, icv.first, icv.second);
    }

    // std::cout<<"checking results: "<<result->length<<std::endl;
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
        // std::cout<<"results length: "<<result->length<<std::endl;
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
            LDEBUG << "Garbage collecting value t=" << p->first << ", v=" << p->second;
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
    return terminate_flag;
    return num_jobs >= config.max_jobs;
  }

  read_status logging_read(std::string key, uint64_t no_earlier_than,
                        std::string &value) {
    // no_earlier_than should be in the past

    read_status status;
    status.time=no_earlier_than;
  

    uint64_t now = Utils::get_time_now_ns();
    if (no_earlier_than > now) {
      LDEBUG << "BFTKVS" << id << "::try_read " << key << " [" << no_earlier_than
            << ", " << now << "] failed at 1";

      status.status=false;
      status.time=now;
      status.mode=1;
      return status;
    }

    // store should have record of this key
    auto r1 = store.find(key);
    if (r1 == store.end()) {
      LDEBUG << "BFTKVS" << id << "::try_read " << key << " [" << no_earlier_than
            << ", " << now << "] failed at 2";
      status.status=false;
      status.time=now;
      status.mode=2;
      return status;
    }
    
    for(auto it = r1->second.begin(); it != r1->second.end(); it++){ // iterate over time-value pairs (sorted with highest time first)
      uint64_t time = it->first; // time 
      if((time <= now) && (no_earlier_than <= time)){ // time before/is now and time is not too late (has come after the no_earlier than time)
        value = it->second;
        // if(key=="rawpos1") std::cout<<"try_read rawpos1 passed ["<<time<<" , "<<no_earlier_than<<"]";
        status.status=true;
        status.time=time;
        status.mode=4;
        return status;
      }
      if(no_earlier_than > time){ // it's late
        LDEBUG << "BFTKVS" << id << "::try_read " << key << " [" << no_earlier_than
              << ", " << time << "] failed at 3";
        status.status=false;
        status.time=time;
        status.mode=3;
        return status;
      }
    }

    return status;
  }

  bool try_read(std::string key, uint64_t no_earlier_than,
                        std::string &value) {
    // no_earlier_than should be in the past
    uint64_t now = Utils::get_time_now_ns();
    if (no_earlier_than > now) {
      LDEBUG << "BFTKVS" << id << "::try_read " << key << " [" << no_earlier_than
            << ", " << now << "] failed at 1";
      return false;
    }

    // store should have record of this key
    auto r1 = store.find(key);
    if (r1 == store.end()) {
      LDEBUG << "BFTKVS" << id << "::try_read " << key << " [" << no_earlier_than
            << ", " << now << "] failed at 2";
      return false;
    }
    
    for(auto it = r1->second.begin(); it != r1->second.end(); it++){ // iterate over time-value pairs (sorted with highest time first)
      uint64_t time = it->first; // time 
      if((time <= now) && (no_earlier_than <= time)){ // time before/is now and time is not too late (has come after the no_earlier than time)
        value = it->second;
        // if(key=="rawpos1") std::cout<<"try_read rawpos1 passed ["<<time<<" , "<<no_earlier_than<<"]";
        return true;
      }
      if(no_earlier_than > time){ // it's late
        LDEBUG << "BFTKVS" << id << "::try_read " << key << " [" << no_earlier_than
              << ", " << time << "] failed at 3";
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
    // std::cout<<"key: "<<key<<std::endl;
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

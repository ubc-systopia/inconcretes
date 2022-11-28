#include <unordered_map>
#include <unordered_set>
#include <map>
#include <string>
#include <vector>
#include <algorithm>
#include <iostream>
#include <chrono>
#include <cstring>

#include "catch.hpp"
#include "main.h"
#include "achal/eig_tree.h"

#define EIG_TREE_MAX_KEY_SIZE 7
#define EIG_TREE_MAX_VALUE_SIZE 7
#define EIG_TREE_MAX_NUM_TUPLES 1024

typedef struct tuple {
  char key[EIG_TREE_MAX_KEY_SIZE+1];
  uint64_t time;
  char value[EIG_TREE_MAX_VALUE_SIZE+1];
} tuple_t;

typedef struct eig_node {
  uint32_t length;
  tuple_t data[EIG_TREE_MAX_NUM_TUPLES];
} eig_node_t;

class Test {
 public:
  int r;
  std::map<std::string, std::map<uint64_t, std::string>> reference;
  std::vector<std::map<std::string, std::map<uint64_t, std::string>>> stores;
  std::vector<Achal::EIGTree<eig_node_t>> trees;
  Test(int num_processes, int rounds) : r(rounds) {
    for (int i = 0; i < EIG_TREE_MAX_NUM_TUPLES; i++) {
      char buf_key[EIG_TREE_MAX_KEY_SIZE];
      sprintf(buf_key, "%04d", i);
      char buf_value[EIG_TREE_MAX_VALUE_SIZE];
      sprintf(buf_value, "%04d", i + 1);

      reference[std::string(buf_key)][i] = std::string(buf_value);
    }

    for (int i = 0; i < num_processes; i++) {
      trees.push_back(Achal::EIGTree<eig_node_t>(num_processes, rounds));
      stores.push_back(reference);
    }
  }

  void run() {
    // initialize
    for (int my_pid = 0; my_pid < trees.size(); my_pid++) {
      eig_node_t *node = trees[my_pid].root();
      for (auto &p1 : stores[my_pid]) {
        for (auto &p2 : p1.second) {
          uint32_t l = node->length;
          assert(l < EIG_TREE_MAX_NUM_TUPLES);
          assert(p1.first.length() < EIG_TREE_MAX_KEY_SIZE);
          assert(p2.second.length() < EIG_TREE_MAX_KEY_SIZE);
          node->data[l].time = p2.first;
          strcpy(node->data[l].key, p1.first.c_str());

          if (my_pid == trees.size() - 1) {
            // the last node is Byzantine and will write bad values
            strcpy(node->data[l].value, "bad");
          } else {
            strcpy(node->data[l].value, p2.second.c_str());
          }
          node->length++;
        }
      }
    }

    // send and receive
    for (int l = 0; l < r; l++) {
      for (int my_pid = 0; my_pid < trees.size(); my_pid++) {
        trees[my_pid].copy_current_level(l, my_pid);
        for (int other_pid = 0; other_pid < trees.size(); other_pid++) {
          if (other_pid != my_pid) {
            
            uint8_t * send_buf = trees[my_pid].send_buffer(l);
            size_t send_len = trees[my_pid].buffer_length(l);
            uint8_t * receive_buf = trees[other_pid].receive_buffer(l, my_pid);
            
            std::copy(send_buf, send_buf + send_len, receive_buf);
          }
        }
      }
    }

    for (int my_pid = 0; my_pid < trees.size(); my_pid++) {
      auto result = trees[my_pid].reduce();
      auto length = result->length;
      REQUIRE(length == EIG_TREE_MAX_NUM_TUPLES);
      auto data = result->data;
      for (int i = 0; i < result->length; i++) {
        stores[my_pid][data[i].key][data[i].time] = data[i].value;
      }
      trees[my_pid].clear();
      REQUIRE(stores[my_pid] == reference);
    }
  }
};


using namespace std::chrono;
TEST_CASE("EIGTree", "[eigtree][ningfeng]") {
  int nodes = 4;
  Test t(nodes, 2);

  int times = 300;

  auto start = high_resolution_clock::now();
  for (int i = 0; i < times; i++) {
    t.run();
  }
  auto stop = high_resolution_clock::now();

  auto duration = duration_cast<microseconds>(stop - start) / times / nodes;

  std::cout << "Time per node per protocol: "
            << duration.count() << " microseconds" << std::endl;
}

void test_eig_tree(bool root, bool first[4], bool second[16]) {

  Achal::EIGTree<eig_node_t> tree(4, 2);

  // good node
  eig_node_t good_node;
  good_node.length = 1;
  strcpy(good_node.data[0].key, "key");
  good_node.data[0].time = 0x12345678;
  strcpy(good_node.data[0].value, "value");

  // bad node
  eig_node_t bad_node;
  memset(&bad_node, 0, sizeof(eig_node_t));

  // set last level
  for(int i = 0; i < 16; i++) {
    tree.levels[2][i] = second[i] ? good_node : bad_node;
  }

  // reduce
  tree.reduce();

  #define REQUIRE_EIG_NODE_EQUAL(A, B) REQUIRE(memcmp((A), (B), sizeof(eig_node_t)) == 0)

  // verify root
  REQUIRE_EIG_NODE_EQUAL(&tree.levels[0][0], root ? &good_node : &bad_node);

  // verify first level 
  for(int i = 0; i < 4; i++) {
    REQUIRE_EIG_NODE_EQUAL(&tree.levels[1][i], first[i] ? &good_node : &bad_node);
  }

}

// don't care
#define DNTC false

TEST_CASE("EIGTree basic 1", "[eigtree_basic][ningfeng]") {
  bool root = true;

  bool first[4] = {
    true, 
    true, 
    true,
    true,
  };
  
  bool second[16] = {
    DNTC,  true,  true,  true,
    true,  DNTC,  true,  true,
    true,  true,  DNTC,  true,
    true,  true,  true,  DNTC,
  };
  
  test_eig_tree(root, first, second);
}

TEST_CASE("EIGTree basic 2", "[eigtree_basic][ningfeng]") {
  bool root = true;

  bool first[4] = {
    true, 
    true, 
    true,
    true,
  };
  
  bool second[16] = {
    DNTC,  false, true,  false,
    false, DNTC,  false, true,
    true,  true,  DNTC,  true,
    true,  true,  true,  DNTC,
  };
  
  test_eig_tree(root, first, second);
}

TEST_CASE("EIGTree basic 3", "[eigtree_basic][ningfeng]") {
  bool root = true;

  bool first[4] = {
    false, 
    true, 
    true,
    true,
  };
  
  bool second[16] = {
    DNTC,  false, true,  false,
    false, DNTC,  false, true,
    false, true,  DNTC,  true,
    true,  true,  true,  DNTC,
  };
  
  test_eig_tree(root, first, second);
}

TEST_CASE("EIGTree basic 4", "[eigtree_basic][ningfeng]") {
  bool root = true;

  bool first[4] = {
    true, 
    true, 
    false,
    true,
  };
  
  bool second[16] = {
    DNTC,  false, false, false,
    true,  DNTC,  false, true,
    false, true,  DNTC,  true,
    true,  true,  true,  DNTC,
  };
  
  test_eig_tree(root, first, second);
}


TEST_CASE("EIGTree basic 5", "[eigtree_basic][ningfeng]") {
  bool root = false;

  bool first[4] = {
    true, 
    false, 
    false,
    true,
  };
  
  bool second[16] = {
    DNTC,  false, false, false,
    true,  DNTC,  false, true,
    false, false, DNTC,  true,
    true,  true,  true,  DNTC,
  };
  
  test_eig_tree(root, first, second);
}

TEST_CASE("EIGTree basic 6", "[eigtree_basic][ningfeng]") {
  bool root = false;

  bool first[4] = {
    true, 
    false, 
    false,
    true,
  };
  
  bool second[16] = {
    DNTC,  false, false, true,
    true,  DNTC,  false, true,
    true,  false, DNTC,  true,
    true,  true,  true,  DNTC,
  };
  
  test_eig_tree(root, first, second);
}

TEST_CASE("EIGTree basic 7", "[eigtree_basic][ningfeng]") {
  bool root = true;

  bool first[4] = {
    true, 
    false, 
    true,
    true,
  };
  
  bool second[16] = {
    DNTC,  false, true, false,
    true,  DNTC,  true, true,
    true,  false, DNTC, true,
    true,  false, true, DNTC,
  };
  
  test_eig_tree(root, first, second);
}


TEST_CASE("EIGTree Send Buffer Optimized", "[eigtree_basic][ningfeng][send_buf]") {
  Achal::EIGTree<eig_node_t> tree(4, 2);
  std::vector<boost::asio::const_buffer> send_bufs;

  send_bufs.clear();
  tree.send_buffer_optimized(0, 0, send_bufs);
  REQUIRE(send_bufs.size() == 1);
  REQUIRE(send_bufs[0].data() == &tree.levels[0][0]);
  REQUIRE(send_bufs[0].size() == sizeof(eig_node_t));

#define CHECK_SEND_BUF(A, B) REQUIRE(send_bufs[(A)].data() == &tree.levels[1][(B)]); REQUIRE(send_bufs[(A)].size() == sizeof(eig_node_t));

  send_bufs.clear();
  tree.send_buffer_optimized(1, 0, send_bufs);
  REQUIRE(send_bufs.size() == 3);
  CHECK_SEND_BUF(0, 1);
  CHECK_SEND_BUF(1, 2);
  CHECK_SEND_BUF(2, 3);

  send_bufs.clear();
  tree.send_buffer_optimized(1, 1, send_bufs);
  REQUIRE(send_bufs.size() == 3);
  CHECK_SEND_BUF(0, 0);
  CHECK_SEND_BUF(1, 2);
  CHECK_SEND_BUF(2, 3);

  send_bufs.clear();
  tree.send_buffer_optimized(1, 2, send_bufs);
  REQUIRE(send_bufs.size() == 3);
  CHECK_SEND_BUF(0, 0);
  CHECK_SEND_BUF(1, 1);
  CHECK_SEND_BUF(2, 3);

  send_bufs.clear();
  tree.send_buffer_optimized(1, 3, send_bufs);
  REQUIRE(send_bufs.size() == 3);
  CHECK_SEND_BUF(0, 0);
  CHECK_SEND_BUF(1, 1);
  CHECK_SEND_BUF(2, 2);
#undef CHECK_SEND_BUF
}


TEST_CASE("EIGTree Receive Buffer Optimized", "[eigtree_basic][ningfeng][receive_buf]") {
  Achal::EIGTree<eig_node_t> tree(4, 2);
  size_t len;
  auto v = tree.receive_buffer_optimized(0, 0);
  REQUIRE(v.size() == 1);
  REQUIRE(v[0].first == (char *)&tree.levels[1][0]);

  v = tree.receive_buffer_optimized(0, 1);
  REQUIRE(v.size() == 1);
  REQUIRE(v[0].first == (char *)&tree.levels[1][1]);

  v = tree.receive_buffer_optimized(0, 2);
  REQUIRE(v.size() == 1);
  REQUIRE(v[0].first == (char *)&tree.levels[1][2]);

  v = tree.receive_buffer_optimized(0, 3);
  REQUIRE(v.size() == 1);
  REQUIRE(v[0].first == (char *)&tree.levels[1][3]);

  v = tree.receive_buffer_optimized(1, 0);
  REQUIRE(v.size() == 3);
  REQUIRE(v[0].first == (char *)&tree.levels[2][1]);
  REQUIRE(v[1].first == (char *)&tree.levels[2][2]);
  REQUIRE(v[2].first == (char *)&tree.levels[2][3]);

  v = tree.receive_buffer_optimized(1, 1);
  REQUIRE(v.size() == 3);
  REQUIRE(v[0].first == (char *)&tree.levels[2][4]);
  REQUIRE(v[1].first == (char *)&tree.levels[2][6]);
  REQUIRE(v[2].first == (char *)&tree.levels[2][7]);

  v = tree.receive_buffer_optimized(1, 2);
  REQUIRE(v.size() == 3);
  REQUIRE(v[0].first == (char *)&tree.levels[2][8]);
  REQUIRE(v[1].first == (char *)&tree.levels[2][9]);
  REQUIRE(v[2].first == (char *)&tree.levels[2][11]);

  v = tree.receive_buffer_optimized(1, 3);
  REQUIRE(v.size() == 3);
  REQUIRE(v[0].first == (char *)&tree.levels[2][12]);
  REQUIRE(v[1].first == (char *)&tree.levels[2][13]);
  REQUIRE(v[2].first == (char *)&tree.levels[2][14]);
}


TEST_CASE("EIGTree Buffer Length Optimized", "[eigtree_basic][ningfeng][buffer_len]") {
  Achal::EIGTree<eig_node_t> tree(4, 2);
  for(int i = 0; i < 4; i++){ 
    REQUIRE(tree.buffer_length_optimized(0, i) == sizeof(eig_node_t));
  }
  for(int i = 0; i < 4; i++){ 
    REQUIRE(tree.buffer_length_optimized(1, 0) == 3 * sizeof(eig_node_t));
  }
}

#ifndef EIG_TREE_H
#define EIG_TREE_H

#include <stdint.h>
#include <stdlib.h>
#include <boost/asio.hpp>
#include <vector>
#include <algorithm>
#include <cassert>
#include <boost/crc.hpp>
#include <unordered_map>

namespace Achal {

template <typename T>
class EIGTree {
 private:
  uint32_t degree;
  uint32_t height;
  size_t *num_nodes;
  bool **valid;
  std::vector<size_t> **children;
  boost::crc_32_type hasher;
  std::unordered_map<uint32_t, int> freq;
  std::unordered_map<uint32_t, int> index;

 public:
  T **levels;
  EIGTree(uint32_t d, uint32_t h) : degree(d), height(h) {
    assert(height <= degree);
    // initialize fields
    num_nodes = new size_t[h + 1];
    levels = new T *[h + 1];
    valid = new bool *[h + 1];
    children = new std::vector<size_t> *[h + 1];
    freq.reserve(degree);
    index.reserve(degree);

    // for each level allocate space
    for (size_t l = 0; l <= h; l++) {
      size_t nn = l == 0 ? 1 : num_nodes[l - 1] * degree;

      num_nodes[l] = nn;
      levels[l] = new T[nn];
      valid[l] = new bool[nn];
      children[l] = new std::vector<size_t>[nn];
    }

    // for each level
    for (int l = 0; l <= h; l++) {
      // for each node
      for (int n = 0; n < num_nodes[l]; n++) {
        valid[l][n] = true;
        // convert node number to labels
        std::vector<bool> labels(d, false);
        int temp = n;
        for (int i = l - 1; i >= 0; i--) {
          int c = temp / num_nodes[i];
          if (labels[c]) {
            valid[l][n] = false;
          }
          labels[c] = true;
          temp %= num_nodes[i];
        }

        // invalid nodes have no children
        if (!valid[l][n]) {
          continue;
        }

        // valid nodes can only have children with index NOT in its label
        for (int i = 0; i < d; i++) {
          if (!labels[i]) {
            children[l][n].push_back(i * num_nodes[l] + n);
          }
        }
      }
    }

    clear();
  }

  EIGTree(const EIGTree<T> &other) : EIGTree(other.degree, other.height) {}

  inline void clear() {
    for (int i = 0; i <= height; i++) {
      std::memset(levels[i], 0, sizeof(T) * num_nodes[i]);
    }
  }

  inline T *root() {
    return &levels[0][0];
  }

  inline std::pair<T *, size_t> interactive_consistency_vector() {
    return std::make_pair(levels[1], degree);
  }

  inline void copy_current_level(int l, int pid) {
    size_t nn = num_nodes[l];
    std::copy(&levels[l][0], &levels[l][nn], &levels[l + 1][pid * nn]);
  }

  inline size_t buffer_length(int l) {
    return num_nodes[l] * sizeof(T);
  }

  inline size_t buffer_length_optimized(int l, int pid){   
    size_t result = 0; 
    for(int i = 0; i < num_nodes[l]; i++){
      for(int c : children[l][i]){
        if((c / num_nodes[l]) == pid) {
          result += sizeof(T);
          break;
        }
      }
    }
    return result;
  }

  inline uint8_t * send_buffer(int l) {
    return (uint8_t *)&levels[l][0];
  }

  inline void send_buffer_optimized(int l, int pid, std::vector<boost::asio::const_buffer> &send_bufs) {
    for(int i = 0; i < num_nodes[l]; i++){
      for(int c : children[l][i]){
        if((c / num_nodes[l]) == pid) {
          send_bufs.push_back(boost::asio::const_buffer(&levels[l][i], sizeof(T)));
          break;
        }
      }
    }
  }

  inline uint8_t * receive_buffer(int l, int pid) {
    return (uint8_t *)&levels[l + 1][pid * num_nodes[l]];
  }

  inline std::vector<std::pair<char *, size_t>> receive_buffer_optimized(int l, int pid) {
    std::vector<std::pair<char *, size_t>> receive_bufs;
    for(int i = pid * num_nodes[l]; i < (pid + 1) * num_nodes[l]; i++){
      if(valid[l+1][i]){
        receive_bufs.push_back(std::make_pair((char *)&levels[l+1][i], sizeof(T)));
      }
    }
    return receive_bufs;
  }

  T *reduce() {

    for (int l = height - 1; l >= 0; l--) {
      int nn = num_nodes[l];
      for (int n = 0; n < num_nodes[l]; n++) {
        freq.clear();
        index.clear();
        if (!valid[l][n]) {
          continue;
        }

        for (size_t child : children[l][n]) {
          hasher.reset();
          hasher.process_bytes(&levels[l + 1][child], sizeof(T));
          uint32_t hash = hasher.checksum();
          freq[hash]++;
          index[hash] = child;
        }

        int max_frequency = -1;
        int maj = -1;
        for (auto &p : freq) {
          if (p.second > max_frequency) {
            max_frequency = p.second;
            maj = index[p.first];
          }
        }

        if (max_frequency > degree - height - l) {
          // simple majority exists
          levels[l][n] = levels[l + 1][maj];
        }else{
          // empty value
          std::memset(&levels[l][n], 0, sizeof(T));
        }
      }
    }
    return root();
  }

  ~EIGTree() {
    for (size_t l = 0; l <= height; l++) {
      delete[] levels[l];
      delete[] valid[l];
      delete[] children[l];
    }
    delete[] num_nodes;
    delete[] levels;
    delete[] valid;
    delete[] children;
  }
};
}

#endif

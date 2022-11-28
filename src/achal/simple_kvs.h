#ifndef ACHAL_SIMPLE_KVS_H
#define ACHAL_SIMPLE_KVS_H

#include <vector>
#include <iostream>

#include "absl/container/flat_hash_map.h"
#include "kvs_interface.h"

namespace Achal {

class SimpleKVS {
 protected:
  absl::flat_hash_map<std::string, std::string> store;

 public:
  SimpleKVS() {}

  void write(std::string key, std::string value);
  void write(std::string key, uint64_t publish_time, std::string value);
  void write_batch(std::vector<std::string>& keys,
                   std::vector<std::string>& values);
  bool try_write(std::string key, uint64_t publish_time, std::string value);
  bool try_write_batch(std::vector<std::string>& keys, uint64_t publish_time,
                       std::vector<std::string>& values);

  std::string read(std::string key);
  void read_batch(std::vector<std::string>& keys,
                  std::vector<std::string>& values);
  bool try_read_batch(std::vector<std::string>& keys, uint64_t no_earlier_than,
                      std::vector<std::string>& values);
  bool try_read(std::string key, uint64_t no_earlier_than, std::string &value);
};

} // namespace Achal

#endif  // ACHAL_SIMPLE_KVS_H

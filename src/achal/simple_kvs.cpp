#include  "simple_kvs.h"

#include <cassert>

namespace Achal {

void SimpleKVS::write(std::string key, std::string value) {
  store[key] = value;
}

void SimpleKVS::write(std::string key, uint64_t publish_time,
                      std::string value) {
  write(key, value);
}

void SimpleKVS::write_batch(std::vector<std::string>& keys,
                            std::vector<std::string>& values) {
  for (int i = 0; i < keys.size(); i++) {
    store[keys[i]] = values[i];
  }
}

bool SimpleKVS::try_write(std::string key, uint64_t publish_time,
                          std::string value) {
  write(key, value);
  return true;
}

bool SimpleKVS::try_write_batch(std::vector<std::string>& keys,
                                uint64_t publish_time,
                                std::vector<std::string>& values) {
  write_batch(keys, values);
  return true;
}

std::string SimpleKVS::read(std::string key) {
  return store[key];
}

void SimpleKVS::read_batch(std::vector<std::string>& keys,
                           std::vector<std::string>& values) {
  for (int i = 0; i < keys.size(); i++) {
    values[i] = store[keys[i]];
  }
}

bool SimpleKVS::try_read(std::string key, uint64_t no_earlier_than,
                         std::string &value) {
  value = read(key);
  return true;
}

bool SimpleKVS::try_read_batch(std::vector<std::string>& keys,
                               uint64_t no_earlier_than,
                               std::vector<std::string>& values) {
  read_batch(keys, values);
  return true;
}

} // namespace Achal

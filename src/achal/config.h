#ifndef ACHAL_CONFIG_H
#define ACHAL_CONFIG_H

#include <vector>
#include <string>

#include "utils/misc.h"

namespace Achal {

class Host {
 public:
  std::string name;
  std::string ip;
  int port;

  Host() {}
  Host(std::string name, std::string ip, int port)
    : name(name), ip(ip), port(port) {}
};

class Application {
 public:
  std::string name;
  long long period_ms;
  int replicas;

  Application() {}
  Application(std::string name, long long period_ms, int replicas)
    : name(name), period_ms(period_ms), replicas(replicas) {}
};

class Config {
 private:
  std::string config_file;
  std::vector<Host> hosts;
  std::vector<Application> apps;

  void parse_config_file();
 public:
  Config();
  Config(std::string config_file);

  std::vector<Host>& get_hosts() {
    return hosts;
  }
  std::vector<Application>& get_apps() {
    return apps;
  }
};

} // namespace Achal

#endif  // ACHAL_CONFIG_H

#include "config.h"

#include <libconfig.h++>

#include <iostream>

namespace Achal {

void Config::parse_config_file() {
  libconfig::Config cfg;

  try {
    cfg.readFile(config_file.c_str());
  } catch(const libconfig::FileIOException &fioex) {
    std::cerr << "I/O error while reading file " << config_file << std::endl;
  } catch(const libconfig::ParseException &pex) {
    std::cerr << "Parse error in " << config_file << " at " << pex.getFile()
              << ":" << pex.getLine() << " - " << pex.getError() << std::endl;
  }

  const libconfig::Setting& root = cfg.getRoot();

  const libconfig::Setting& hosts = root["AchalConfig"]["hosts"];
  for (int i = 0; i < hosts.getLength(); i++) {
    const libconfig::Setting& host = hosts[i];
    Host myhost;
    if (host.lookupValue("name", myhost.name) and
        host.lookupValue("ip", myhost.ip) and
        host.lookupValue("port", myhost.port)) {
      this->hosts.push_back(myhost);
    } else {
      std::cerr << "Parse error in host " << i << std::endl;
    }
  }

  const libconfig::Setting& apps = root["AchalConfig"]["applications"];
  for (int i = 0; i < apps.getLength(); i++) {
    const libconfig::Setting& app = apps[i];
    Application myapp;
    if (app.lookupValue("name", myapp.name) and
        app.lookupValue("period_ms", myapp.period_ms) and
        app.lookupValue("replicas", myapp.replicas)) {
      this->apps.push_back(myapp);
    } else {
      std::cerr << "Parse error in app " << i << std::endl;
    }
  }
}

Config::Config()
  : config_file(Utils::get_project_directory() + "/config/default.cfg") {
  parse_config_file();
}

Config::Config(std::string config_file) : config_file(config_file) {
  parse_config_file();
}

} // namespace Achal

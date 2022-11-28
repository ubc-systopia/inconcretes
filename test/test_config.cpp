#include "catch.hpp"

#include "utils/misc.h"
#include "achal/config.h"

TEST_CASE("Parsing test.cfg using libconfig++", "[config]") {
  Achal::Config config(Utils::get_project_directory() + "/test/test.cfg");
  unsigned count = 4;
  REQUIRE(config.get_hosts().size() == count);
  REQUIRE(config.get_apps().size() == count);
  for (unsigned i = 0; i < count; i++) {
    REQUIRE(config.get_hosts()[i].name == "brain0" + std::to_string(i + 1));
    REQUIRE(config.get_hosts()[i].ip == "127.0.0." + std::to_string(i + 1));
    REQUIRE(config.get_hosts()[i].port == 8080 + i + 1);
    REQUIRE(config.get_apps()[i].name == "app" + std::to_string(i + 1));
    REQUIRE(config.get_apps()[i].period_ms == 10L * (i + 1));
    REQUIRE(config.get_apps()[i].replicas == i + 1);
  }
}

TEST_CASE("Parsing default.cfg using libconfig++", "[config]") {
  Achal::Config config;
  unsigned count = 2;
  REQUIRE(config.get_hosts().size() == count);
  REQUIRE(config.get_apps().size() == count);
  for (unsigned i = 0; i < count; i++) {
    REQUIRE(config.get_hosts()[i].name == "localhost");
    REQUIRE(config.get_hosts()[i].ip == "127.0.0.1");
    REQUIRE(config.get_hosts()[i].port == 8080 + i + 1);
    REQUIRE(config.get_apps()[i].name == "ivp_sim");
    REQUIRE(config.get_apps()[i].period_ms == 10L);
    REQUIRE(config.get_apps()[i].replicas == 2);
  }
}

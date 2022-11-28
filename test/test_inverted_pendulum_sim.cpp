#include "catch.hpp"

#include "main.h"
#include "utils/macros.h"
#include "utils/logging.h"
#include "utils/timespec.h"
#include "applications/inverted_pendulum_sim.h"

TEST_CASE("Inverted pendulum simulation as a periodic task",
          "[inverted_pendulum_sim]") {
  Applications::IvPSim *task = new Applications::IvPSim(
    0, MS_TO_NS(1000), MS_TO_NS(0), 1, 0, 10, logger);
  task->profiler(100);
  REQUIRE(task->job_profile->exec_time_observed_P00
          <= task->job_profile->exec_time_observed_P25);
  REQUIRE(task->job_profile->exec_time_observed_P25
          <= task->job_profile->exec_time_observed_P50);
  REQUIRE(task->job_profile->exec_time_observed_P50
          <= task->job_profile->exec_time_observed_P75);
  REQUIRE(task->job_profile->exec_time_observed_P75
          <= task->job_profile->exec_time_observed_P90);
  REQUIRE(task->job_profile->exec_time_observed_P90
          <= task->job_profile->exec_time_observed_P99);
  REQUIRE(task->job_profile->exec_time_observed_P99
          <= task->job_profile->exec_time_observed_P100);
  REQUIRE(task->job_profile->exec_time_observed_P100
          <= task->job_profile->wcet_estimated);
  delete task;
}

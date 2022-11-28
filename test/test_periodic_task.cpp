#include "catch.hpp"

#include "main.h"
#include "utils/macros.h"
#include "utils/logging.h"
#include "utils/timespec.h"
#include "utils/periodic_task.h"

class TestPeriodicTask : public Utils::PeriodicTask {
 public:
  uint64_t wcet_ns;
  uint64_t max_jobs;
  uint64_t num_jobs;
  TimeSpec task_start_ts;
  bool set_task_start_ts;

  TestPeriodicTask(unsigned id, uint64_t time_period_ns, uint64_t wcet_ns,
                   uint64_t max_jobs, log4cpp::Category *logger)
    : Utils::PeriodicTask(id, time_period_ns, 0, 1, 0, logger),
      wcet_ns(wcet_ns),
      max_jobs(max_jobs),
      num_jobs(0),
      set_task_start_ts(true) {}

  void job() {
    if (set_task_start_ts) {
      clock_gettime(CLOCK_ID, &task_start_ts);
      set_task_start_ts = false;
      return;
    }

    TimeSpec start_ts, end_ts;
    clock_gettime(CLOCK_ID, &start_ts);
    end_ts.tv_sec = start_ts.tv_sec;
    end_ts.tv_nsec = start_ts.tv_nsec + wcet_ns;
    Utils::fix_ts(&end_ts);

    int err = EINTR;
    while (err != 0 and err == EINTR) {
      err = clock_nanosleep(CLOCK_ID, TIMER_ABSTIME, &end_ts, nullptr);
    }
    if (err != 0) {
      perror("Error");
    }

    num_jobs++;
  }

  void job_for_profiler() {
    job();
  }
};

class WellBehavedTestPeriodicTask : public TestPeriodicTask {
 public:
  WellBehavedTestPeriodicTask(unsigned id, uint64_t time_period_ns,
                              uint64_t wcet_ns,
                              uint64_t max_jobs, log4cpp::Category *logger)
    : TestPeriodicTask(id, time_period_ns, wcet_ns, max_jobs, logger) {}

  bool terminate() {
    if (num_jobs < max_jobs) {
      return false;;
    }

    TimeSpec now_ts, diff_ts;
    clock_gettime(CLOCK_ID, &now_ts);
    timespecsub(&now_ts, &task_start_ts, &diff_ts);
    double duration_s = diff_ts.tv_sec + NS_TO_SEC(diff_ts.tv_nsec);

    REQUIRE(duration_s > (max_jobs - 1) * NS_TO_SEC(time_period_ns));
    REQUIRE(duration_s > (max_jobs - 1) * NS_TO_SEC(time_period_ns + wcet_ns));

    // This assertion is likely to pass, but can fail occasionally
    REQUIRE(duration_s < (max_jobs + 1) * NS_TO_SEC(time_period_ns));

    return true;
  }
};

TEST_CASE("PeriodicTask with well-behaved jobs", "[periodic_task]") {
  WellBehavedTestPeriodicTask *task = new WellBehavedTestPeriodicTask(
    0, MS_TO_NS(200), MS_TO_NS(1), 50, logger);
  task->spawn();
  task->join();
  delete task;
}

class SlowTestPeriodicTask : public TestPeriodicTask {
 public:
  SlowTestPeriodicTask(unsigned id, uint64_t time_period_ns, uint64_t wcet_ns,
                       uint64_t max_jobs, log4cpp::Category *logger)
    : TestPeriodicTask(id, time_period_ns, wcet_ns, max_jobs, logger) {}

  bool terminate() {
    if (num_jobs < max_jobs) {
      return false;;
    }

    TimeSpec now_ts, diff_ts;
    clock_gettime(CLOCK_ID, &now_ts);
    timespecsub(&now_ts, &task_start_ts, &diff_ts);
    double duration_s = diff_ts.tv_sec + NS_TO_SEC(diff_ts.tv_nsec);
    std::cout << "Duration = " << diff_ts.tv_sec << "s, " << diff_ts.tv_nsec << "ns"
              << std::endl;

    REQUIRE(max_jobs < 1000);
    REQUIRE(wcet_ns < time_period_ns * 2);
    REQUIRE(duration_s > (max_jobs * 2) * NS_TO_SEC(time_period_ns));

    // This assertion is likely to pass, but can fail occasionally
    REQUIRE(duration_s < (max_jobs * 2 + 1) * NS_TO_SEC(time_period_ns));

    return true;
  }
};

TEST_CASE("PeriodicTask with slow jobs", "[periodic_task]") {
  SlowTestPeriodicTask *task = new SlowTestPeriodicTask(
    0, MS_TO_NS(200), MS_TO_NS(200) + 1, 50, logger);
  task->spawn();
  task->join();
  delete task;
}

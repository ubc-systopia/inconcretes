#include "catch.hpp"

//#include <time.h>
//#include <bsd/sys/time.h>
//#include <cassert>
//#include <iostream>

#include "utils/macros.h"
#include "utils/periodic_timer.h"
#include "utils/timespec.h"

TEST_CASE("PeriodicTimer CLOCK_REAL_TIME support", "[periodic_timer]") {
  TimeSpec release_ts;
  clock_gettime(CLOCK_ID, &release_ts);
  release_ts.tv_nsec = 0;
  release_ts.tv_sec += 2;
  int32_t err = clock_nanosleep(CLOCK_ID, TIMER_ABSTIME, &release_ts, nullptr);
  REQUIRE(err != EINVAL);
}

TEST_CASE("PeriodicTimer wait APIs", "[periodic_timer]") {
  TimeSpec start_ts, end_ts;
  TimeSpec release_ts;
  time_t wait_duration_sec = 5;

  clock_gettime(CLOCK_ID, &start_ts);
  release_ts.tv_sec = start_ts.tv_sec + wait_duration_sec;
  release_ts.tv_nsec = start_ts.tv_nsec;
  int err = Utils::wait_until_ts(&release_ts);
  clock_gettime(CLOCK_ID, &end_ts);

  REQUIRE(err == 0);
  REQUIRE(timespeccmp(&start_ts, &release_ts, <));
  REQUIRE(timespeccmp(&start_ts, &end_ts, <));
  REQUIRE(timespeccmp(&release_ts, &end_ts, <));

  TimeSpec diff_ts;
  timespecsub(&end_ts, &start_ts, &diff_ts);
  REQUIRE(diff_ts.tv_sec >= wait_duration_sec);
}

TEST_CASE("PeriodicTimer computing aligned timespecs", "[periodic_timer]") {
  TimeSpec now_ts, next_ts, diff_ts;
  uint64_t time_period_ns;

  for (uint64_t time_period_sec = 1; time_period_sec <= 10; time_period_sec++) {
    time_period_ns = SEC_TO_NS(time_period_sec);
    Utils::PeriodicTimer timer(time_period_ns);
    timer.get_next_release(&now_ts, &next_ts);
    timespecsub(&next_ts, &now_ts, &diff_ts);
    REQUIRE(TS_TO_NS(&diff_ts) >= 0);
    REQUIRE(TS_TO_NS(&diff_ts) <= time_period_ns);
    REQUIRE(next_ts.tv_sec % time_period_sec == 0);
    REQUIRE(next_ts.tv_nsec == 0);
  }

  uint64_t time_period_ms = 25;
  time_period_ns = MS_TO_NS(time_period_ms);
  Utils::PeriodicTimer timer1(time_period_ns);
  timer1.get_next_release(&now_ts, &next_ts);
  timespecsub(&next_ts, &now_ts, &diff_ts);
  REQUIRE(TS_TO_NS(&diff_ts) >= 0);
  REQUIRE(TS_TO_NS(&diff_ts) <= time_period_ns);
  REQUIRE(next_ts.tv_nsec % time_period_ns == 0);

  time_period_ns = SEC_TO_NS((uint64_t) 31) + MS_TO_NS(71) + US_TO_NS(59) + 381;
  Utils::PeriodicTimer timer2(time_period_ns);
  timer2.get_next_release(&now_ts, &next_ts);
  for (unsigned i = 0; i < 10; i++) {
    now_ts.tv_sec = next_ts.tv_sec;
    now_ts.tv_nsec = next_ts.tv_nsec;
    timer2.get_next_release(&now_ts, &next_ts, true);
    timespecsub(&next_ts, &now_ts, &diff_ts);
    REQUIRE(TS_TO_NS(&diff_ts) == time_period_ns);
  }
}

TEST_CASE("PeriodicTimer with well-behaved jobs", "[periodic_timer]") {
  TimeSpec now_ts, diff_ts, start_ts, last_activation_ts;
  uint64_t num_jobs = 0;
  uint64_t time_period_ms = 200;
  uint64_t test_duration_s = 10;
  Utils::PeriodicTimer timer(MS_TO_NS(time_period_ms));
  clock_gettime(CLOCK_ID, &start_ts);
  timer.trigger();
  while (true) {
    num_jobs++;  // Start periodic job
    timer.wait();  // End periodic job, sleep until next periodic job
    clock_gettime(CLOCK_ID, &now_ts);
    timespecsub(&now_ts, &start_ts, &diff_ts);
    //print_ts(&diff_ts, "Diff");
    //std::cout << "NumJobsCompleted: " << num_jobs << std::endl;
    if (diff_ts.tv_sec >= test_duration_s) {
      break;
    }
  }
  REQUIRE(num_jobs == (test_duration_s * ONE_SEC_TO_MS) / time_period_ms);
}

TEST_CASE("PeriodicTimer with slow jobs", "[periodic_timer]") {
  TimeSpec now_ts, diff_ts, start_ts;
  uint64_t num_jobs = 0;
  uint64_t time_period_ms = 200;
  uint64_t test_duration_s = 10;
  Utils::PeriodicTimer timer(MS_TO_NS(time_period_ms));
  clock_gettime(CLOCK_ID, &start_ts);
  timer.trigger();
  while (true) {
    num_jobs++;  // Start periodic job
    clock_gettime(CLOCK_ID, &now_ts);
    now_ts.tv_nsec += MS_TO_NS(time_period_ms);
    Utils::fix_ts(&now_ts);
    Utils::wait_until_ts(&now_ts);
    timer.wait();  // End periodic job, sleep until next periodic job
    clock_gettime(CLOCK_ID, &now_ts);
    timespecsub(&now_ts, &start_ts, &diff_ts);
    //print_ts(&diff_ts, "Diff");
    //std::cout << "NumJobsCompleted: " << num_jobs << std::endl;
    if (diff_ts.tv_sec >= test_duration_s) {
      break;
    }
  }
  REQUIRE(num_jobs == ((test_duration_s * ONE_SEC_TO_MS) / time_period_ms) / 2);
}

TEST_CASE("PeriodicTimer with different triggers", "[periodic_timer]") {
  TimeSpec start_ts, diff_ts;
  uint64_t diff_ns;
  uint64_t time_period_ns;

  for (time_period_ns = 1; time_period_ns <= SEC_TO_NS(1);
       time_period_ns *= 10) {
    Utils::PeriodicTimer timer(time_period_ns);

    uint64_t delay_ns = MS_TO_NS(100);
    timer.reset();
    clock_gettime(CLOCK_ID, &start_ts);
    timer.trigger(delay_ns);
    timespecsub(&timer.last_activation_ts, &start_ts, &diff_ts);
    diff_ns = SEC_TO_NS(diff_ts.tv_sec) + diff_ts.tv_nsec;
    REQUIRE(diff_ns >= delay_ns);

    TimeSpec ts;
    clock_gettime(CLOCK_ID, &ts);
    ts.tv_sec += 10;
    timer.reset();
    timer.trigger(&ts);
    REQUIRE(timespeccmp(&timer.last_activation_ts, &ts, >));
  }
}

TEST_CASE("PeriodicTimer computing aligned timespecs with offsets",
          "[periodic_timer]") {
  TimeSpec now_ts, next_ts, diff_ts;
  uint64_t time_period_ns = SEC_TO_NS(1);
  uint64_t offset_ns = MS_TO_NS(191);
  Utils::PeriodicTimer timer2(time_period_ns, offset_ns);
  timer2.get_next_release(&now_ts, &next_ts);
  for (unsigned i = 0; i < 10; i++) {
    now_ts.tv_sec = next_ts.tv_sec;
    now_ts.tv_nsec = next_ts.tv_nsec;
    timer2.get_next_release(&now_ts, &next_ts, true);
    timespecsub(&next_ts, &now_ts, &diff_ts);
    REQUIRE(TS_TO_NS(&diff_ts) == time_period_ns);
    REQUIRE(next_ts.tv_nsec == offset_ns);
  }
}

#include <thread>

#include "misc.h"
#include "macros.h"

namespace Utils {

std::string get_project_directory() {
  int bufsize = 1024;
  char buf[bufsize];
  int len = readlink("/proc/self/exe", buf, bufsize);
  return dirname(dirname(buf));
}

unsigned int get_max_cpus() {
  return std::thread::hardware_concurrency();
}

uint64_t get_time_now_ns() {
  TimeSpec ts;
  clock_gettime(CLOCK_ID, &ts);
  return TS_TO_NS(&ts);
}

} // namespace Utils

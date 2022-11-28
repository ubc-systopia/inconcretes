#ifndef UTILS_MISC_H
#define UTILS_MISC_H

#include <unistd.h>
#include <libgen.h>

#include <string>

namespace Utils {

std::string get_project_directory();
unsigned int get_max_cpus();
uint64_t get_time_now_ns();

} // namespace Utils

#endif  // UTILS_MISC_H

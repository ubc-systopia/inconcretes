#ifndef UTILS_MACROS_H
#define UTILS_MACROS_H

#include <cmath>
#include <random>

// Time conversion macros
#define ONE_US_TO_NS   (uint64_t)1000
#define ONE_MS_TO_US   (uint64_t)1000
#define ONE_MS_TO_NS   (uint64_t)1000000
#define ONE_SEC_TO_MS  (uint64_t)1000
#define ONE_SEC_TO_NS  (uint64_t)1000000000
#define SEC_TO_MS(t) ((t) * ONE_SEC_TO_MS)
#define SEC_TO_US(t) ((t) * ONE_SEC_TO_US)
#define SEC_TO_NS(t) ((t) * ONE_SEC_TO_NS)
#define MS_TO_NS(t)  ((t) * ONE_MS_TO_NS)
#define US_TO_NS(t)  ((t) * ONE_US_TO_NS)
#define US_TO_MS(t)  ((t) / (double)ONE_MS_TO_US)
#define NS_TO_US(t)  ((t) / (double)ONE_US_TO_NS)
#define NS_TO_MS(t)  ((t) / (double)ONE_MS_TO_NS)
#define NS_TO_SEC(t) ((t) / (double)ONE_SEC_TO_NS)
#define NS_TO_US_ROUNDED(t)  ((long)(round((t) / (double)ONE_US_TO_NS)))
#define NS_TO_MS_ROUNDED(t)  ((long)(round((t) / (double)ONE_MS_TO_NS)))
#define NS_TO_SEC_ROUNDED(t) ((long)(round((t) / (double)ONE_SEC_TO_NS)))
#define TS_TO_NS(ts) ((uint64_t)((ts)->tv_sec) * ONE_SEC_TO_NS + (ts)->tv_nsec)
#define TS_TO_MS(ts) (NS_TO_MS(TS_TO_NS(ts)))

// Clock
#define CLOCK_ID CLOCK_REALTIME

// Debugging macros
#define __FILENAME__ ( __builtin_strrchr(__FILE__, '/') \
  ? __builtin_strrchr(__FILE__, '/') + 1 : __FILE__)
#define LOG_MSG(X) std::cout << __FILENAME__ << "::" << __LINE__ << "::" \
  << __FUNCTION__ << " " << (X) << std::endl << std::flush;
#define LOG_ARRIVAL LOG_MSG("")

// namespace Utils { // TODO Do we need the Utils namespace here as well?

// Typedefs
typedef struct timespec TimeSpec;
typedef std::uniform_int_distribution<int> UniIntDist;
typedef std::uniform_real_distribution<double> UniRealDist;

// } // namespace Utils

#endif  // UTILS_MACROS_H

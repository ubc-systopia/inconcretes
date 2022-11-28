#ifndef UTILS_LOGGING_H
#define UTILS_LOGGING_H

#include "log4cpp/Category.hh"
#include "log4cpp/FileAppender.hh"
#include "log4cpp/OstreamAppender.hh"
#include "log4cpp/BasicLayout.hh"

#define LDEBUG   logger->debugStream()
#define LINFO    logger->infoStream()
#define LERROR   logger->errorStream()

namespace Utils {

bool get_new_logger(std::string id, int prio, log4cpp::Category **logger_pp);
bool get_new_logger(std::string id, std::string file, int prio,
                    log4cpp::Category **logger_pp);

} // namespace Utils

#endif // UTILS_LOGGING_H

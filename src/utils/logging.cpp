#include "logging.h"

namespace Utils {

bool get_new_logger(std::string id, int prio, log4cpp::Category **logger_pp) {
  try {
    log4cpp::Appender *app = new log4cpp::OstreamAppender("console", &std::cout);
    log4cpp::Layout *layout = new log4cpp::BasicLayout();
    app->setLayout(layout);
    log4cpp::Category& logger = log4cpp::Category::getInstance(id);
    logger.setAdditivity(false);
    logger.setAppender(app);
    logger.setPriority(prio);
    (*logger_pp) = &logger;
    std::cout << "Created console logger \"" << id << "\"" << std::endl;
    return true;
  } catch(const std::exception& e) {
    std::cerr << "Error: Creation of logger \"" << id << "\" failed "
              << "with exception " << e.what() << std::flush;
    return false;
  }
}

bool get_new_logger(std::string id, std::string file, int prio,
                    log4cpp::Category **logger_pp) {
  try {
    log4cpp::Appender *app = new log4cpp::FileAppender("FileAppender", file);
    log4cpp::Layout *layout = new log4cpp::BasicLayout();
    app->setLayout(layout);
    log4cpp::Category& logger = log4cpp::Category::getInstance(id);
    logger.setAdditivity(false);
    logger.setAppender(app);
    logger.setPriority(prio);
    (*logger_pp) = &logger;
    std::cout << "Created file logger \"" << id << "\" mapped to file \""
              << file << "\"" << std::endl;
    return true;
  } catch(const std::exception& e) {
    std::cerr << "Error: Creation of logger \"" << id << "\" failed "
              << "with exception " << e.what() << std::flush;
    return false;
  }
}

} // namespace Utils

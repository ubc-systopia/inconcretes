// This definition tells Catch to provide a main()
// Define this in ONLY ONE cpp file
// Define this BEFORE including "catch.hpp"
#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include "main.h"
log4cpp::Category *logger = NULL;
bool logger_status = Utils::get_new_logger(
                       "Test", log4cpp::Priority::PriorityLevel::DEBUG, &logger);

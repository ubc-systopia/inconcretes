cmake_minimum_required(VERSION 3.13.4)

project(Achal)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -O2")
set(CXX "g++-8")
set(CC "gcc-8")
set(CMAKE_BUILD_TYPE Debug)
if(${CMAKE_C_COMPILER} STREQUAL "/usr/bin/gcc-8")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-psabi")
endif()


set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
find_package(Boost COMPONENTS thread chrono REQUIRED)

find_library(LIBCONFIGPP_LOCATION libconfig++)
find_library(LIBCONFIG_LOCATION libconfig)
find_package(etcd-cpp-api)

                                                                                                                                                      
find_library(
  log4cpp                                                                                                                                                    
  NAMES log4cpp                                                                                                                                                       
  PATHS /usr/lib /usr/lib64 /usr/local/lib /opt/local/lib                                                                                                                                   
)

include_directories(${ETCD_CPP_INCLUDE_DIRS})

# Process Abseil's CMake build system
add_subdirectory(third-party/abseil-cpp)

#add_subdirectory(pigpio)

include_directories(
  src
  third-party/abseil-cpp
  #third-party/flat_hash_map // Compilation error on Raspberry Pi
  third-party/robin-map/include
  third-party/hopscotch-map/include
  third-party/array-hash/include
  ~/usr/local/include
  /usr/local/include/jsoncpp
  third-party/jsoncpp
)

add_library(
  achal
  src/utils/misc.cpp
  src/utils/sched.cpp
  src/utils/logging.cpp
  src/utils/binpack.cpp
  src/utils/periodic_task.cpp
  src/utils/periodic_timer.cpp
  src/achal/config.cpp
  src/achal/kvs_interface.cpp
  src/achal/simple_kvs.cpp
  src/achal/bft_kvs.cpp
  src/applications/physical_ivp.cpp
  src/applications/PIvPUtils/Ultrasonic.cpp
  src/applications/PIvPUtils/AS_5600.cpp
)

target_link_libraries(
  achal
)


add_executable(
  run_pivp
  exp/pivp1.cpp
)

target_link_libraries(
  run_pivp
  atomic
  pigpio
  jsoncpp
  i2c
  achal
  libconfig++.so
  liblog4cpp.so
  Threads::Threads
  absl::flat_hash_map
  absl::node_hash_map
  Boost::thread 
  Boost::chrono 
  ${ETCD_CPP_LIBRARIES}
)

add_executable(
  test_Ultrasonic
  src/applications/PIvPUtils/test_Ultrasonic.cpp
  src/applications/PIvPUtils/Ultrasonic.cpp
)

add_executable(
  test_AS5600
  src/applications/PIvPUtils/test_AS5600.cpp
  src/applications/PIvPUtils/AS_5600.cpp
)

add_executable(
  test_limit_sw
  src/applications/PIvPUtils/test_limit_sw.cpp
)

#target_link_libraries(test_Ultrasonic PUBLIC pigpio)
#target_link_libraries(test_AS5600 PUBLIC pigpio)

find_library(PIGPIO_LIB pigpio)
if(PIGPIO_LIB)
  message(STATUS "Found pigpio library: ${PIGPIO_LIB}")
  target_link_libraries(test_Ultrasonic ${PIGPIO_LIB})
  target_link_libraries(test_limit_sw ${PIGPIO_LIB})
else()
  message(FATAL_ERROR "pigpio library not found")
endif()

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
                                                                                                                                                      
find_library(log4cpp                                                                                                                                                    
  NAMES log4cpp                                                                                                                                                       
  PATHS /usr/lib /usr/lib64 /usr/local/lib /opt/local/lib                                                                                                                                   
)

include_directories(${ETCD_CPP_INCLUDE_DIRS})

# Process Abseil's CMake build system
add_subdirectory(third-party/abseil-cpp)

include_directories(
  src
  third-party/abseil-cpp
  #third-party/flat_hash_map // Compilation error on Raspberry Pi
  third-party/robin-map/include
  third-party/hopscotch-map/include
  third-party/array-hash/include
  ~/usr/local/include
)

add_library(
  achal
  src/utils/misc.cpp
  src/utils/sched.cpp
  src/utils/logging.cpp
  src/utils/binpack.cpp
  src/utils/periodic_task.cpp
  src/utils/periodic_timer.cpp
  src/utils/kzh15_benchmark_generator.cpp
  src/achal/config.cpp
  src/achal/kvs_interface.cpp
  src/achal/simple_kvs.cpp
  src/achal/bft_kvs.cpp
  src/achal/redis_kvs.cpp
  src/achal/etcd_kvs.cpp
  src/applications/inverted_pendulum_sim.cpp
  src/applications/kzh15.cpp
)
target_link_libraries(
  achal
)

add_executable(
  tests
  test/main.cpp
  test/test_redis.cpp
  test/test_config.cpp
  test/test_binpack.cpp
  test/test_periodic_timer.cpp
  test/test_periodic_task.cpp
  test/test_kzh15_benchmark_e2e.cpp
  test/test_kzh15_benchmark_generator.cpp
  test/test_eigtree.cpp
  test/test_bft_kvs.cpp
  test/test_etcd.cpp
)

target_link_libraries(
  tests
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
  bosch1
  exp/bosch1.cpp
)

target_link_libraries(
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

target_link_libraries(
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

target_link_libraries(
  bosch1
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
  exp_ivp_1
  exp/ivp1.cpp
)

target_link_libraries(
  exp_ivp_1
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

# <------------ add hiredis dependency --------------->
find_path(HIREDIS_HEADER hiredis)
target_include_directories(achal PUBLIC ${HIREDIS_HEADER})
target_include_directories(tests PUBLIC ${HIREDIS_HEADER})
target_include_directories(exp_ivp_1 PUBLIC ${HIREDIS_HEADER})
target_include_directories(bosch1 PUBLIC ${HIREDIS_HEADER})

find_library(HIREDIS_LIB hiredis)
target_link_libraries(achal ${HIREDIS_LIB})
target_link_libraries(tests ${HIREDIS_LIB})
target_link_libraries(exp_ivp_1 ${HIREDIS_LIB})
target_link_libraries(bosch1 ${HIREDIS_LIB})

# <------------ add redis-plus-plus dependency -------------->
# NOTE: this should be *sw* NOT *redis++*
find_path(REDIS_PLUS_PLUS_HEADER sw)
target_include_directories(achal PUBLIC ${REDIS_PLUS_PLUS_HEADER})
target_include_directories(tests PUBLIC ${REDIS_PLUS_PLUS_HEADER})
target_include_directories(exp_ivp_1 PUBLIC ${REDIS_PLUS_PLUS_HEADER})
target_include_directories(bosch1 PUBLIC ${REDIS_PLUS_PLUS_HEADER})

find_library(REDIS_PLUS_PLUS_LIB redis++)
target_link_libraries(achal ${REDIS_PLUS_PLUS_LIB})
target_link_libraries(tests ${REDIS_PLUS_PLUS_LIB})
target_link_libraries(exp_ivp_1 ${REDIS_PLUS_PLUS_LIB})
target_link_libraries(bosch1 ${REDIS_PLUS_PLUS_LIB})

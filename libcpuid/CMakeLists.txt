cmake_minimum_required (VERSION 3.10)

project(libcpuid  VERSION 1.0.0 LANGUAGES C CXX)

add_library(libcpuid STATIC src/cache-and-topology.cpp src/cpuid.cpp src/features.cpp src/hypervisors.cpp src/standard.cpp src/utility.cpp)
target_include_directories(libcpuid PUBLIC  external)
target_include_directories(libcpuid PRIVATE internal)
target_include_directories(libcpuid PRIVATE ${Boost_INCLUDE_DIRS})

target_link_libraries(libcpuid PRIVATE ${Boost_LIBRARIES})
target_link_libraries(libcpuid PRIVATE gsl)
target_link_libraries(libcpuid PRIVATE fmt::fmt)

find_package(Threads REQUIRED)
target_link_libraries(libcpuid PRIVATE Threads::Threads)

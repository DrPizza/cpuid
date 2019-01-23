cmake_minimum_required (VERSION 3.11)

project(libcpuid)

add_library(libcpuid STATIC src/cache-and-topology.cpp src/cpuid.cpp src/features.cpp src/hypervisors.cpp src/standard.cpp)
target_include_directories(libcpuid PUBLIC .)
target_include_directories(libcpuid PRIVATE include)
target_include_directories(libcpuid PRIVATE cpuid)
target_include_directories(libcpuid PRIVATE ${Boost_INCLUDE_DIRS})

target_link_libraries(libcpuid ${Boost_LIBRARIES})
target_link_libraries(libcpuid gsl)
target_link_libraries(libcpuid fmt::fmt)
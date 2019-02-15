cmake_minimum_required (VERSION 3.10)

project(cpuid VERSION 1.0.0 LANGUAGES C CXX)

add_executable(cpuid src/docopt.cpp src/main.cpp)
target_include_directories(cpuid PRIVATE internal)
target_include_directories(cpuid PRIVATE ${Boost_INCLUDE_DIRS})

target_link_libraries(cpuid PRIVATE libcpuid)
target_link_libraries(cpuid PRIVATE ${Boost_LIBRARIES})
target_link_libraries(cpuid PRIVATE gsl)
target_link_libraries(cpuid PRIVATE fmt::fmt)

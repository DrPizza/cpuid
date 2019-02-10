cmake_minimum_required (VERSION 3.10)

project(cpuidexe VERSION 1.0.0 LANGUAGES C CXX)

add_executable(cpuidexe src/docopt.cpp src/main.cpp)
target_include_directories(cpuidexe PRIVATE include)
target_include_directories(cpuidexe PRIVATE ${Boost_INCLUDE_DIRS})

target_link_libraries(cpuidexe libcpuid)
target_link_libraries(cpuidexe ${Boost_LIBRARIES})
target_link_libraries(cpuidexe gsl)
target_link_libraries(cpuidexe fmt::fmt)

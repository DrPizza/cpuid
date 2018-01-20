#ifndef TOPOLOGY_HPP
#define TOPOLOGY_HPP

#include "cpuid.hpp"

void enumerate_extended_topology(cpu_t& cpu);
void print_extended_topology(const cpu_t& cpu);

void determine_topology();

#endif

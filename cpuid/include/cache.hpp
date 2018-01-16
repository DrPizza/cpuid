#ifndef CACHE_HPP
#define CACHE_HPP

#include "cpuid.hpp"

void print_cache_tlb_info(const cpu_t& cpu);

void enumerate_deterministic_cache(cpu_t& cpu);
void print_deterministic_cache(const cpu_t& cpu);

#endif

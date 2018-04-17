#ifndef CACHE_AND_TOPOLOGY_HPP
#define CACHE_AND_TOPOLOGY_HPP

#include "cpuid.hpp"

#include <map>
#include <vector>
#include <cstddef>

struct full_apic_id_t
{
	std::uint32_t logical_id;
	std::uint32_t physical_id;
	std::uint32_t package_id;
};

void print_cache_tlb_info(fmt::Writer& w, const cpu_t& cpu);

void enumerate_deterministic_cache(cpu_t& cpu);
void print_deterministic_cache(fmt::Writer& w, const cpu_t& cpu);

void enumerate_extended_topology(cpu_t& cpu);
void print_extended_topology(fmt::Writer& w, const cpu_t& cpu);

void enumerate_deterministic_tlb(cpu_t& cpu);
void print_deterministic_tlb(fmt::Writer& w, const cpu_t& cpu);

void print_l1_cache_tlb(fmt::Writer& w, const cpu_t& cpu);
void print_l2_cache_tlb(fmt::Writer& w, const cpu_t& cpu);

void print_1g_tlb(fmt::Writer& w, const cpu_t& cpu);

void enumerate_cache_properties(cpu_t& cpu);
void print_cache_properties(fmt::Writer& w, const cpu_t& cpu);

void print_extended_apic(fmt::Writer& w, const cpu_t& cpu);

#endif
#ifndef CACHE_AND_TOPOLOGY_HPP
#define CACHE_AND_TOPOLOGY_HPP

#include "cpuid.hpp"

#include <map>
#include <vector>
#include <cstddef>

struct cache_instance_t
{
	std::vector<std::uint32_t> sharing_ids;
};

struct cache_t
{
	std::uint32_t level;
	std::uint32_t type;
	std::uint32_t ways;
	std::uint32_t sets;
	std::uint32_t line_size;
	std::uint32_t line_partitions;
	std::uint32_t total_size;
	bool fully_associative;
	bool direct_mapped;
	bool self_initializing;
	bool invalidates_lower_levels;
	bool inclusive;
	std::uint32_t sharing_mask;

	std::map<std::uint32_t, cache_instance_t> instances;
};

inline bool operator<(const cache_t& lhs, const cache_t& rhs) noexcept {
	return lhs.level != rhs.level ? lhs.level      < rhs.level
	     : lhs.type  != rhs.type  ? lhs.type       < rhs.type
	     :                          lhs.total_size < rhs.total_size;
}

struct logical_core_t
{
	std::uint32_t full_apic_id;

	std::uint32_t package_id;
	std::uint32_t physical_core_id;
	std::uint32_t logical_core_id;

	std::vector<std::uint32_t> non_shared_cache_ids;
	std::vector<std::uint32_t> shared_cache_ids;
};

struct physical_core_t
{
	std::map<std::uint32_t, logical_core_t> logical_cores;
};

struct package_t
{
	std::map<std::uint32_t, physical_core_t> physical_cores;
};

struct system_t
{
	std::uint32_t logical_mask_width;
	std::uint32_t physical_mask_width;
	std::vector<std::uint32_t> x2_apic_ids;

	std::vector<cache_t> all_caches;
	std::vector<logical_core_t> all_cores;

	std::map<std::uint32_t, package_t> packages;
};

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

void print_topology(fmt::Writer& w, const std::vector<cpu_t>& logical_cpus);

system_t build_topology(const std::vector<cpu_t>& logical_cpus);

#endif

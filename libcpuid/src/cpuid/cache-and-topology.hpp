#ifndef CACHE_AND_TOPOLOGY_HPP
#define CACHE_AND_TOPOLOGY_HPP

#include "cpuid/cpuid.hpp"

#include <map>
#include <vector>
#include <cstddef>

namespace cpuid
{
	struct full_apic_id_t
	{
		std::uint32_t smt_id;
		std::uint32_t core_id;
		std::uint32_t module_id;
		std::uint32_t tile_id;
		std::uint32_t die_id;
		std::uint32_t package_id;
	};

	void print_cache_tlb_info(fmt::memory_buffer& out, const cpu_t& cpu);

	void enumerate_deterministic_cache(cpu_t& cpu);
	void print_deterministic_cache(fmt::memory_buffer& out, const cpu_t& cpu);

	void enumerate_extended_topology(cpu_t& cpu);
	void print_extended_topology(fmt::memory_buffer& out, const cpu_t& cpu);

	void enumerate_extended_topology_v2(cpu_t& cpu);
	void print_extended_topology_v2(fmt::memory_buffer& out, const cpu_t& cpu);

	void enumerate_deterministic_tlb(cpu_t& cpu);
	void print_deterministic_tlb(fmt::memory_buffer& out, const cpu_t& cpu);

	void print_l1_cache_tlb(fmt::memory_buffer& out, const cpu_t& cpu);
	void print_l2_cache_tlb(fmt::memory_buffer& out, const cpu_t& cpu);

	void print_1g_tlb(fmt::memory_buffer& out, const cpu_t& cpu);

	void enumerate_cache_properties(cpu_t& cpu);
	void print_cache_properties(fmt::memory_buffer& out, const cpu_t& cpu);

	void print_extended_apic(fmt::memory_buffer& out, const cpu_t& cpu);
}

#endif

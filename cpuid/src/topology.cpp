#include "stdafx.h"

#include "topology.hpp"

#include <fmt/format.h>
#include <iostream>
#include <iomanip>
#include <thread>

// TODO: enumerating the full topology requires bouncing to every core (even across processor groups) and amalgamating data

void enumerate_extended_topology(cpu_t& cpu) {
	register_set_t regs = { 0 };
	cpuid(regs, leaf_t::extended_topology, subleaf_t::main);
	cpu.leaves[leaf_t::extended_topology][subleaf_t::main] = regs;
	for(subleaf_t sub = subleaf_t{ 1 }; ; ++sub) {
		cpuid(regs, leaf_t::extended_topology, sub);
		if((regs[ecx] & 0x0000'ff00ui32) == 0ui32) {
			break;
		}
		cpu.leaves[leaf_t::extended_topology][sub] = regs;
	}
}

void print_extended_topology(const cpu_t& cpu) {
	const std::uint32_t x2_apic_id = cpu.leaves.at(leaf_t::extended_topology).at(subleaf_t::main)[edx];
	
	std::cout << "Extended topology\n";
	std::cout << "\tx2 APIC id: " << std::hex << x2_apic_id << "\n";
	for(const auto& sub : cpu.leaves.at(leaf_t::extended_topology)) {
		const register_set_t& regs = sub.second;

		const union
		{
			std::uint32_t full;
			struct
			{
				std::uint32_t shift_distance : 5;
				std::uint32_t reserved_1     : 27;
			} split;
		} a = { regs[eax] };

		const union
		{
			std::uint32_t full;
			struct
			{
				std::uint32_t logical_procesors_at_level_type : 16;
				std::uint32_t reserved_1                      : 16;
			} split;
		} b = { regs[ebx] };

		const union
		{
			std::uint32_t full;
			struct
			{
				std::uint32_t level_number : 8;
				std::uint32_t level_type   : 8;
				std::uint32_t reserved_1   : 16;
			} split;
		} c = { regs[ecx] };

		std::cout << "\t\tbits to shift: " << a.split.shift_distance << "\n";
		std::cout << "\t\tlogical processors at level type: " << b.split.logical_procesors_at_level_type << "\n";
		std::cout << "\t\tlevel number: " << c.split.level_number << "\n";
		switch(c.split.level_type) {
		case 0:
			std::cout << "\t\tLevel type: invalid\n";
			break;
		case 1:
			std::cout << "\t\tlevel type: SMT\n";
			break;
		case 2:
			std::cout << "\t\tlevel type: Core\n";
			break;
		default:
			std::cout << "\t\tlevel type: reserved " << std::hex << c.split.level_type << "\n";
			break;
		}
		std::cout << std::endl;
	}
}

struct logical_core_t
{
	std::uint32_t logical_core_id;
};

struct physical_core_t
{
	std::uint32_t physical_core_id;
	std::map<std::uint32_t, logical_core_t> logical_cores;
};

struct package_t
{
	std::uint32_t package_id;
	std::map<std::uint32_t, physical_core_t> physical_cores;
};

struct system_t
{
	std::uint32_t logical_mask_width;
	std::uint32_t physical_mask_width;

	std::map<std::uint32_t, package_t> packages;
};

struct full_apic_id_t
{
	std::uint32_t logical_id;
	std::uint32_t physical_id;
	std::uint32_t package_id;
};

constexpr full_apic_id_t split_apic_id(std::uint32_t id, std::uint32_t logical_mask_width, std::uint32_t physical_mask_width) noexcept {
	const std::uint32_t logical_select_mask  = ~(0xffff'ffffui32 << logical_mask_width);
	const std::uint32_t logical_id = id & logical_select_mask;
	const std::uint32_t core_select_mask = ~(0xffff'ffffui32 << physical_mask_width);
	const std::uint32_t physical_id = (id & core_select_mask) >> logical_mask_width;
	const std::uint32_t package_select_mask = 0xffff'ffffui32 << physical_mask_width;
	const std::uint32_t package_id = (id & package_select_mask) >> physical_mask_width;
	return { logical_id, physical_id, package_id };
}

void determine_topology() {
	system_t machine = {};

	std::thread bouncer = std::thread([&machine]() {
		const WORD total_processor_groups = ::GetMaximumProcessorGroupCount();
		for(WORD group_id = 0; group_id < total_processor_groups; ++group_id) {
			const DWORD processors_in_group = ::GetMaximumProcessorCount(group_id);
			for(DWORD proc = 0; proc < processors_in_group; ++proc) {
				const GROUP_AFFINITY aff = { 1ui64 << proc, group_id};
				::SetThreadGroupAffinity(::GetCurrentThread(), &aff, nullptr);

				cpu_t cpu = { 0 };
				enumerate_extended_topology(cpu);
				print_extended_topology(cpu);
				
				for(const auto& sub : cpu.leaves.at(leaf_t::extended_topology)) {
					const register_set_t& regs = sub.second;

					const union
					{
						std::uint32_t full;
						struct
						{
							std::uint32_t shift_distance : 5;
							std::uint32_t reserved_1     : 27;
						} split;
					} a = { regs[eax] };

					const union
					{
						std::uint32_t full;
						struct
						{
							std::uint32_t level_number : 8;
							std::uint32_t level_type   : 8;
							std::uint32_t reserved_1   : 16;
						} split;
					} c = { regs[ecx] };

					switch(c.split.level_type) {
					case 1:
						if(machine.logical_mask_width == 0ui32) {
							machine.logical_mask_width = a.split.shift_distance;
						}
						break;
					case 2:
						if(machine.physical_mask_width == 0ui32) {
							machine.physical_mask_width = a.split.shift_distance;
						}
						break;
					default:
						break;
					}
				}
			}
		}
	});
	bouncer.join();
}

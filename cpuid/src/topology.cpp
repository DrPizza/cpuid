#include "stdafx.h"

#include "topology.hpp"

#include <fmt/format.h>
#include <iostream>
#include <iomanip>

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
	if(0 == (cpu.leaves.at(leaf_t::extended_topology).at(subleaf_t::main)[ecx] & 0x0000'ff00ui32)) {
		return;
	}

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

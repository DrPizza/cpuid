#include "stdafx.h"

#include "topology.hpp"

#include <fmt/format.h>
#include <iostream>
#include <iomanip>

void enumerate_extended_topology(cpu_t& cpu) {
	register_set_t regs = { 0 };
	cpuid(regs, leaf_t::extended_topology, subleaf_t::zero);
	cpu.features[extended_topology][zero] = regs;
	for(std::uint32_t sub = 1; ; ++sub) {
		cpuid(regs, leaf_t::extended_topology, sub);
		if((regs[ebx] & 0x0000'ffffui32) == 0ui32) {
			break;
		}
		cpu.features[extended_topology][subleaf_t{ sub }] = regs;
	}
}

void print_extended_topology(const cpu_t& cpu) {
	if(cpu.vendor != intel && cpu.vendor != amd) {
		return;
	}
	if(cpu.features.at(extended_topology).at(subleaf_t::zero)[eax] == 0ui32
	&& cpu.features.at(extended_topology).at(subleaf_t::zero)[ebx] == 0ui32) {
		return;
	}

	std::uint32_t x2_apic_id = cpu.features.at(extended_topology).at(subleaf_t::zero)[edx];
	
	std::cout << "Extended topology\n";
	std::cout << "\tx2 APIC id: " << std::hex << x2_apic_id << "\n";
	for(const auto& m : cpu.features.at(extended_topology)) {
		const register_set_t& regs = m.second;
		
		struct topology_a_t
		{
			std::uint32_t shift_distance : 5;
			std::uint32_t reserved_1     : 27;
		};

		struct topology_b_t
		{
			std::uint32_t logical_procesors_at_level_type : 16;
			std::uint32_t reserved_1                      : 16;
		};

		struct topology_c_t
		{
			std::uint32_t level_number : 8;
			std::uint32_t level_type   : 8;
			std::uint32_t reserved_1   : 16;
		};

		union
		{
			topology_a_t a;
			std::uint32_t raw;
		} a;
		a.raw = regs[eax];

		union
		{
			topology_b_t b;
			std::uint32_t raw;
		} b;
		b.raw = regs[ebx];

		union
		{
			topology_c_t c;
			std::uint32_t raw;
		} c;
		c.raw = regs[ebx];

		std::cout << "\tecx: " << std::hex << m.first << "\n";
		std::cout << "\t\tbits to shift: " << a.a.shift_distance << "\n";
		std::cout << "\t\tlogical processors at level type: " << b.b.logical_procesors_at_level_type << "\n";
		std::cout << "\t\tlevel number: " << c.c.level_number << "\n";
		switch(c.c.level_type) {
		case 0:
			std::cout << "\t\tLevel type: invalid\n";
			break;
		case 1:
			std::cout << "\t\tlevel type: SMT\n";
			break;
		case 2:
			std::cout << "\t\tlevel type: Core\n";
		default:
			std::cout << "\t\tlevel type: reserved " << std::hex << c.c.level_type << "\n";
		}
		std::cout << std::flush;
	}
	std::cout << std::endl;
}

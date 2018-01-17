#include "stdafx.h"

#include "standard.hpp"
#include "features.hpp"

#include <fmt/format.h>
#include <iostream>
#include <iomanip>

void print_basic_info(const cpu_t& cpu) {
	const register_set_t& regs = cpu.features.at(leaf_t::basic_info).at(subleaf_t::main);
	std::cout << "Basic Information" << std::endl;
	std::cout << "Maximum basic cpuid leaf: 0x" << std::setw(2) << std::setfill('0') << std::hex << regs[eax] << "\n";

	union
	{
		std::array<char, 12> vndr;
		std::array<std::uint32_t, 3> registers;
	} data;
	data.registers[0] = regs[ebx];
	data.registers[1] = regs[edx];
	data.registers[2] = regs[ecx];

	std::cout << "   vendor: ";
	std::cout.write(data.vndr.data(), data.vndr.size());
	std::cout << "\n";
	std::cout << std::endl;
}

void print_version_info(const cpu_t& cpu) {
	const register_set_t& regs = cpu.features.at(leaf_t::version_info).at(subleaf_t::main);
	std::cout << "Version Information" << std::endl;

	union
	{
		std::uint32_t raw;
		split_model_t m;
	} a;
	
	a.raw = regs[eax];
	std::cout << "signature: 0x" << std::setw(8) << std::setfill('0') << std::hex << regs[eax] << "\n"
	          << "   family: 0x" << std::setw(2) << std::setfill('0') << std::hex << cpu.model.family << "\n"
	          << "    model: 0x" << std::setw(2) << std::setfill('0') << std::hex << cpu.model.model << "\n"
	          << " stepping: 0x" << std::setw(2) << std::setfill('0') << std::hex << cpu.model.stepping << "\n";
	if(cpu.vendor == intel) {
		switch(a.m.type) {
		case 0:
			std::cout << "Original OEM Processor\n";
			break;
		case 1:
			std::cout << "Intel OverDrive Processor\n";
			break;
		case 2:
			std::cout << "Dual processor\n";
			break;
		case 3:
			std::cout << "Intel reserved\n";
			break;
		}
	}
	std::cout << std::endl;

	struct version_b_t
	{
		std::uint8_t brand_id;
		std::uint8_t cache_line_size;
		std::uint8_t maximum_addressable_ids;
		std::uint8_t local_apic_id;
	};

	union
	{
		std::uint32_t raw;
		version_b_t b;
	} b;
	b.raw = regs[ebx];

	if(cpu.vendor == intel) {
		if(b.b.brand_id != 0ui32) {
			std::cout << "brand ID: ";
			switch(b.b.brand_id) {
			case 0x00: break;
			case 0x01: std::cout << "Intel(R) Celeron(R) processor"; break;
			case 0x02: std::cout << "Intel(R) Pentium(R) III processor"; break;
			case 0x03:
				if(regs[eax] == 0x000006b1ui32) {
					std::cout << "Intel(R) Celeron(R) processor";
				} else {
					std::cout << "Intel(R) Pentium(R) III Xeon(R) processor";
				}
			case 0x04: std::cout << "Intel(R) Pentium(R) III processor"; break;
			case 0x06: std::cout << "Mobile Intel(R) Pentium(R) III processor-M"; break;
			case 0x07: std::cout << "Mobile Intel(R) Celeron(R) processor"; break;
			case 0x08: std::cout << "Intel(R) Pentium(R) 4 processor"; break;
			case 0x09: std::cout << "Intel(R) Pentium(R) 4 processor"; break;
			case 0x0a: std::cout << "Intel(R) Celeron(R) processor"; break;
			case 0x0b:
				if(regs[eax] == 0x00000f13ui32) {
					std::cout << "Intel(R) Xeon(R) processor MP";
				} else {
					std::cout << "Intel(R) Xeon(R) processor";
				}
			case 0x0c: std::cout << "Intel(R) Xeon(R) processor MP"; break;
			case 0x0e:
				if(regs[eax] == 0x00000f13ui32) {
					std::cout << "Intel(R) Xeon(R) processor";
				} else {
					std::cout << "Mobile Intel(R) Pentium(R) 4 processor - M";
				}
			case 0x0f: std::cout << "Mobile Intel(R) Celeron(R) processor"; break;
			case 0x11: std::cout << "Mobile Genuine Intel(R) processor"; break;
			case 0x12: std::cout << "Intel(R) Celeron(R) M processor"; break;
			case 0x13: std::cout << "Mobile Intel(R) Celeron(R) processor"; break;
			case 0x14: std::cout << "Intel(R) Celeron(R) processor"; break;
			case 0x15: std::cout << "Mobile Genuine Intel(R) processor"; break;
			case 0x16: std::cout << "Intel(R) Pentium(R) M processor"; break;
			case 0x17: std::cout << "Mobile Intel(R) Celeron(R) processor"; break;
			default:
				break;
			}
			std::cout << "\n";
		}
	}

	std::cout << "cache line size/bytes: " << std::dec << b.b.cache_line_size * 8 << "\n"
	          << "logical processors per package: " << gsl::narrow_cast<std::uint32_t>(b.b.maximum_addressable_ids) << "\n"
	          << "local APIC ID: " << gsl::narrow_cast<std::uint32_t>(b.b.local_apic_id) << "\n";
	std::cout << std::endl;

	std::cout << "Feature identifiers\n";
	print_features(leaf_t::version_info, subleaf_t::main, edx, cpu);
	std::cout << "\n";
	print_features(leaf_t::version_info, subleaf_t::main, ecx, cpu);
	std::cout << std::endl;
}

void print_serial_number(const cpu_t& cpu) {
	using namespace fmt::literals;

	if(cpu.vendor != intel && cpu.vendor != transmeta) {
		return;
	}
	std::cout << "Processor serial number: ";
	if(0 == (cpu.features.at(leaf_t::version_info).at(subleaf_t::main)[edx] & (1ui32 << 18ui32))) {
		std::cout << "N/A" << std::endl;
		return;
	}
	const register_set_t& regs = cpu.features.at(leaf_t::serial_number).at(subleaf_t::main);
	switch(cpu.vendor) {
	case intel:
		{
			const std::uint32_t top = cpu.features.at(leaf_t::version_info).at(subleaf_t::main)[eax];
			const std::uint32_t middle = regs[edx];
			const std::uint32_t bottom = regs[ecx];
			std::cout << "{:04x}-{:04x}-{:04x}-{:04x}-{:04x}-{:04x}"_format(top >> 16ui32, top & 0xffffui32, middle >> 16ui32, middle & 0xffffui32, bottom >> 16ui32, bottom & 0xffffui32) << std::endl;
		}
		break;
	case transmeta:
		std::cout << "{:08x}-{:08x}-{:08x}-{:08x}"_format(regs[eax], regs[ebx], regs[ecx], regs[edx]) << std::endl;
		break;
	}
	std::cout << std::endl;
}

void print_mwait_parameters(const cpu_t& cpu) {
	if(cpu.vendor != intel && cpu.vendor != amd) {
		return;
	}

	const register_set_t& regs = cpu.features.at(leaf_t::monitor_mwait).at(subleaf_t::main);

	if(regs[eax] == 0ui32 && regs[ebx] == 0ui32) {
		return;
	}

	struct monitor_a_t
	{
		std::uint32_t smallest_monitor_line : 16;
		std::uint32_t reserved_1            : 16;
	};
	struct monitor_b_t
	{
		std::uint32_t largest_monitor_line : 16;
		std::uint32_t reserved_1           : 16;
	};
	struct monitor_c_t
	{
		std::uint32_t enumerable           : 1;
		std::uint32_t interrupts_as_breaks : 1;
		std::uint32_t reserved_1           : 30;
	};

	union
	{
		monitor_a_t a;
		std::uint32_t raw;
	} a;
	a.raw = regs[eax];

	union
	{
		monitor_b_t b;
		std::uint32_t raw;
	} b;
	b.raw = regs[ebx];

	union
	{
		monitor_c_t c;
		std::uint32_t raw;
	} c;
	c.raw = regs[ecx];

	using namespace fmt::literals;

	fmt::MemoryWriter w;
	w << "MONITOR/MWAIT leaf\n";
	w << "\tSmallest monitor-line size: {:d} bytes\n"_format(a.a.smallest_monitor_line + 0ui32);
	w << "\tLargest monitor-line size: {:d} bytes\n"_format (b.b.largest_monitor_line  + 0ui32);
	if(c.c.enumerable) {
		if(c.c.interrupts_as_breaks) {
			w << "\tInterrupts break MWAIT, even when disabled\n";
		}
		if(cpu.vendor & intel) {
			const std::uint32_t mask = 0b1111;
			for(std::size_t i = 0; i < 8; ++i) {
				std::uint32_t states = (regs[edx] & (mask << (i * 4))) >> (i * 4);
				w << "\t{:d} C{:d} sub C-states supported using MWAIT\n"_format(states, i);
			}
		}
	}

	std::cout << w.str() << std::flush;
	std::cout << std::endl;
}

void print_thermal_and_power(const cpu_t& cpu) {
	if(cpu.vendor != intel && cpu.vendor != amd) {
		return;
	}

	std::cout << "Thermal and Power Management\n";
	const register_set_t& regs = cpu.features.at(leaf_t::monitor_mwait).at(subleaf_t::main);
	print_features(leaf_t::thermal_and_power, subleaf_t::main, eax, cpu);
	std::cout << std::endl;

	if(cpu.vendor != intel) {
		return;
	}

	struct thermal_b_t
	{
		std::uint32_t interrupt_thresholds : 4;
		std::uint32_t reserved_1           : 28;
	};
	struct thermal_c_t
	{
		std::uint32_t hcf : 1;
		std::uint32_t reserved_1 : 2;
		std::uint32_t bias_preference : 1;
		std::uint32_t reserved_2 : 28;
	};

	union
	{
		thermal_b_t b;
		std::uint32_t raw;
	} b;
	b.raw = regs[ebx];

	union
	{
		thermal_c_t c;
		std::uint32_t raw;
	} c;
	c.raw = regs[ecx];

	if(b.b.interrupt_thresholds) {
		std::cout << b.b.interrupt_thresholds << " interrupt thresholds in Digital Thermal Sensor\n";
	}
	print_features(leaf_t::thermal_and_power, subleaf_t::main, ecx, cpu);
	std::cout << std::endl;
}

void enumerate_extended_features(cpu_t& cpu) {
	register_set_t regs = { 0 };
	cpuid(regs,  leaf_t::extended_features, subleaf_t::main);
	cpu.features[leaf_t::extended_features][subleaf_t::main] = regs;

	const subleaf_t limit = subleaf_t{ regs[eax] };
	for(subleaf_t sub = subleaf_t{ 1 }; sub < limit; ++sub) {
		cpuid(regs,  leaf_t::extended_features, sub);
		cpu.features[leaf_t::extended_features][sub] = regs;
	}
}

void print_extended_features(const cpu_t& cpu) {
	const register_set_t& regs = cpu.features.at(leaf_t::extended_features).at(subleaf_t::main);
	std::cout << "Extended features\n";
	print_features(leaf_t::extended_features, subleaf_t::main, ebx, cpu);
	std::cout << "\n";
	if(cpu.vendor & intel) {
		print_features(leaf_t::extended_features, subleaf_t::main, ecx, cpu);
		std::cout << "\n";
	}
	print_features(leaf_t::extended_features, subleaf_t::main, edx, cpu);
	std::cout << "\n";

	struct extended_c_t
	{
		std::uint32_t reserved_1  : 17;
		std::uint32_t mawau_value : 5;
		std::uint32_t reserve_2   : 10;
	};
	
	union
	{
		extended_c_t c;
		std::uint32_t raw;
	} c;
	c.raw = regs[ecx];

	if(cpu.vendor & intel) {
		std::cout << "MAWAU value: " << c.c.mawau_value << "\n";
	}

	std::cout << std::endl;
}

void print_direct_cache_access(const cpu_t& cpu) {
	if(cpu.vendor != intel) {
		return;
	}
	if(0 == (cpu.features.at(leaf_t::version_info).at(subleaf_t::main).at(ecx) & 0x0004'0000ui32)) {
		return;
	}
	const register_set_t& regs = cpu.features.at(leaf_t::direct_cache_access).at(subleaf_t::main);
	std::cout << "Direct Cache Access\n";
	std::cout << std::setw(8) << std::setfill('0') << std::hex << regs[eax] << "\n";
	std::cout << std::endl;
}

void print_performance_monitoring(const cpu_t& cpu) {
	if(cpu.vendor != intel) {
		return;
	}
	if(0 == (cpu.features.at(leaf_t::version_info).at(subleaf_t::main).at(ecx) & 0x0000'8000ui32)) {
		return;
	}

	const register_set_t& regs = cpu.features.at(leaf_t::performance_monitoring).at(subleaf_t::main);

	struct perfmon_a_t
	{
		std::uint32_t version              : 8;
		std::uint32_t counters_per_logical : 8;
		std::uint32_t counter_bit_width    : 8;
		std::uint32_t ebx_length           : 8;
	};

	struct perfmon_b_t
	{
		std::uint32_t core_cycle           : 1;
		std::uint32_t instructions_retired : 1;
		std::uint32_t reference_cycles     : 1;
		std::uint32_t llc_reference        : 1;
		std::uint32_t llc_misses           : 1;
		std::uint32_t branch_retired       : 1;
		std::uint32_t branch_mispredict    : 1;
		std::uint32_t reserved_1           : 25;
	};

	struct perfmon_d_t
	{
		std::uint32_t fixed_function_counters      : 5;
		std::uint32_t fixed_function_counter_width : 8;
		std::uint32_t reserved_1                   : 2;
		std::uint32_t any_thread                   : 1;
		std::uint32_t reserved_2                   : 16;
	};

	union
	{
		perfmon_a_t a;
		std::uint32_t raw;
	} a;
	a.raw = regs[eax];

	union
	{
		perfmon_b_t b;
		std::uint32_t raw;
	} b;
	b.raw = regs[ebx];

	union
	{
		perfmon_d_t d;
		std::uint32_t raw;
	} d;
	d.raw = regs[edx];

	if(a.a.version == 0) {
		return;
	}

	std::cout << "Architectural Performance Monitoring\n";
	std::cout << "\tVersion: " << a.a.version << "\n";
	std::cout << "\tCounters per logical processor: " << a.a.counters_per_logical << "\n";
	std::cout << "\tCounter bit width: " << a.a.counter_bit_width << "\n";
	std::cout << "\tFixed function counters: " << d.d.fixed_function_counters << "\n";
	std::cout << "\tFixed function counter bit width: " << d.d.fixed_function_counter_width << "\n";

	std::cout << "\tSupported counters\n";
	if(0 == b.b.core_cycle) {
		std::cout << "\t\tCore cycles\n";
	}
	if(0 == b.b.instructions_retired) {
		std::cout << "\t\tInstructions retired\n";
	}
	if(0 == b.b.reference_cycles) {
		std::cout << "\t\tReference cycles\n";
	}
	if(0 == b.b.llc_reference) {
		std::cout << "\t\tLast-level cache reference\n";
	}
	if(0 == b.b.llc_misses) {
		std::cout << "\t\tLast-level cache misses\n";
	}
	if(0 == b.b.branch_retired) {
		std::cout << "\t\tBranch instructions retired\n";
	}
	if(0 == b.b.branch_mispredict) {
		std::cout << "\t\tBranch instructions mispredicted\n";
	}
	std::cout << std::endl;
}

void enumerate_extended_state(cpu_t& cpu) {
	register_set_t regs = { 0 };
	cpuid(regs,  leaf_t::extended_state, subleaf_t::extended_state_main);
	cpu.features[leaf_t::extended_state][subleaf_t::extended_state_main] = regs;
	cpuid(regs,  leaf_t::extended_state, subleaf_t::extended_state_sub);
	cpu.features[leaf_t::extended_state][subleaf_t::extended_state_sub] = regs;

	const std::uint64_t valid_bits = regs[eax] | (gsl::narrow_cast<std::uint64_t>(regs[edx]) << 32ui64);
	std::uint64_t mask = 0x1ui64 << 2ui64;
	for(subleaf_t i = subleaf_t{ 2ui32 }; i < subleaf_t{ 64ui32 }; ++i, mask <<= 1ui64) {
		if(valid_bits & mask) {
			cpuid(regs,  leaf_t::extended_state, i);
			if(regs[ebx] == 0ui32) {
				continue;
			}
			cpu.features[leaf_t::extended_state][i] = regs;
		}
	}
}

void print_extended_state(const cpu_t& cpu) {
	if(cpu.vendor != intel && cpu.vendor != amd) {
		return;
	}

	std::cout << "Extended states" << std::endl;
	for(const auto& sub : cpu.features.at(leaf_t::extended_state)) {
		switch(sub.first) {
		case subleaf_t::extended_state_main:
			{
				struct xsave_a_0_t
				{
					std::uint32_t x87               : 1;
					std::uint32_t sse               : 1;
					std::uint32_t avx               : 1;
					std::uint32_t mpx_bounds        : 1;
					std::uint32_t mpx_config        : 1;
					std::uint32_t avx512_op_mask    : 1;
					std::uint32_t avx512_zmm_hi_256 : 1;
					std::uint32_t avx512_zmm_hi_16  : 1;
					std::uint32_t ia32_xss          : 1;
					std::uint32_t pkru              : 1;
					std::uint32_t reserved_1        : 22;
				};

				union
				{
					xsave_a_0_t a;
					std::uint32_t raw;
				} a;
				a.raw = sub.second[eax];

				std::cout << "\tFeatures supported by xsave: \n";
				if(a.a.x87) {
					std::cout << "\t\tx87\n";
				}
				if(a.a.sse) {
					std::cout << "\t\t128-bit SSE XMM\n";
				}
				if(a.a.avx) {
					std::cout << "\t\t256-bit AVX YMM\n";
				}
				if(a.a.mpx_bounds) {
					std::cout << "\t\tMPX bounds registers\n";
				}
				if(a.a.mpx_config) {
					std::cout << "\t\tMPX bounds configuration\n";
				}
				if(a.a.avx512_op_mask) {
					std::cout << "\t\tAVX512 op mask\n";
				}
				if(a.a.avx512_zmm_hi_256) {
					std::cout << "\t\tAVX512 ZMM\n";
				}
				if(a.a.avx512_zmm_hi_16) {
					std::cout << "\t\tAVX512 ZMM\n";
				}
				if(a.a.ia32_xss) {
					std::cout << "IA32_XSS\n";
				}
				if(a.a.pkru) {
					std::cout << "PKRU\n";
				}
				std::cout << "\tMaximum size for all enabled features  : " << sub.second[ebx] << " bytes\n";
				std::cout << "\tMaximum size for all supported features: " << sub.second[ecx] << " bytes\n";
				std::cout << std::endl;
			}
			break;
		case subleaf_t::extended_state_sub:
			{
				struct xsave_a_1_t
				{
					std::uint32_t xsaveopt   : 1;
					std::uint32_t xsavec     : 1;
					std::uint32_t xgetbv     : 1;
					std::uint32_t xsaves     : 1;
					std::uint32_t reserved_1 : 28;
				};

				union
				{
					xsave_a_1_t a;
					std::uint32_t raw;
				} a;
				a.raw = sub.second[eax];
				std::cout << "\tXSAVE support:\n";
				if(a.a.xsaveopt) {
					std::cout << "\t\tXSAVEOPT\n";
				}
				if(a.a.xsavec) {
					std::cout << "\t\tXSAVEC\n";
				}
				if(a.a.xgetbv) {
					std::cout << "\t\tXGETBV\n";
				}
				if(a.a.xsaves) {
					std::cout << "\t\tXSAVES\n";
				}
				std::cout << "\tSize for enabled features: " << std::dec << sub.second[ebx] << " bytes\n";
				std::cout << std::endl;
			}
			break;
		default:
			{
				struct xsave_c_n_t
				{
					std::uint32_t set_in_xss          : 1;
					std::uint32_t aligned_to_64_bytes : 1;
					std::uint32_t reserved_1          : 30;
				};

				union
				{
					xsave_c_n_t c;
					std::uint32_t raw;
				} c;
				c.raw = sub.second[ecx];
				std::cout << "Extended state " << std::hex << static_cast<std::uint32_t>(sub.first) << " uses " << sub.second[eax] << " bytes at offset " << sub.second[ebx] << "\n";
				if(c.c.set_in_xss) {
					std::cout << "\t\tBit set in XSS MSR\n";
				} else {
					std::cout << "\t\tBit set in XCR0\n";
				}
				if(c.c.aligned_to_64_bytes) {
					std::cout << "\t\tAligned to next 64-byte boundary\n";
				} else {
					std::cout << "\t\tImmediately follows previous component.\n";
				}
				std::cout << std::endl;
			}
			break;
		}
	}
}


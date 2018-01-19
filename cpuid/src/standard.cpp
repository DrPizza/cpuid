#include "stdafx.h"

#include "standard.hpp"
#include "features.hpp"

#include <iostream>
#include <iomanip>
#include <map>
#include <vector>

#include <fmt/format.h>

void print_basic_info(const cpu_t& cpu) {
	const register_set_t& regs = cpu.leaves.at(leaf_t::basic_info).at(subleaf_t::main);
	std::cout << "Basic Information" << std::endl;
	std::cout << "\tMaximum basic cpuid leaf: 0x" << std::setw(2) << std::setfill('0') << std::hex << regs[eax] << "\n";

	const union
	{
		std::array<std::uint32_t, 3> registers;
		std::array<char, 12> vndr;
	} data = { regs[ebx], regs[edx], regs[ecx] };

	std::cout << "\t   vendor: ";
	std::cout.write(data.vndr.data(), data.vndr.size());
	std::cout << "\n";
	std::cout << std::endl;
}

void print_version_info(const cpu_t& cpu) {
	const register_set_t& regs = cpu.leaves.at(leaf_t::version_info).at(subleaf_t::main);
	std::cout << "Version Information" << std::endl;

	const union
	{
		std::uint32_t full;
		split_model_t split;
	} a = { regs[eax] };

	const union
	{
		std::uint32_t full;
		id_info_t split;
	} b = { regs[ebx] };
	
	std::cout << "\tsignature: 0x" << std::setw(8) << std::setfill('0') << std::hex << regs[eax] << "\n"
	          << "\t   family: 0x" << std::setw(2) << std::setfill('0') << std::hex << cpu.model.family << "\n"
	          << "\t    model: 0x" << std::setw(2) << std::setfill('0') << std::hex << cpu.model.model << "\n"
	          << "\t stepping: 0x" << std::setw(2) << std::setfill('0') << std::hex << cpu.model.stepping << "\n";
	if(cpu.vendor == intel) {
		std::cout << "\t";
		switch(a.split.type) {
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

	if(cpu.vendor == intel) {
		if(b.split.brand_id != 0ui32) {
			std::cout << "\tbrand ID: ";
			switch(b.split.brand_id) {
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

	std::cout << "\tcache line size/bytes: " << std::dec << b.split.cache_line_size * 8 << "\n";
	if(0 != (cpu.leaves.at(leaf_t::version_info).at(subleaf_t::main).at(edx) & 0x1000'0000ui32)) {
		std::cout << "\tlogical processors per package: " << gsl::narrow_cast<std::uint32_t>(b.split.maximum_addressable_ids) << "\n";
	}
	std::cout << "\tlocal APIC ID: " << gsl::narrow_cast<std::uint32_t>(b.split.local_apic_id) << "\n";
	std::cout << std::endl;

	std::cout << "\tFeature identifiers\n";
	print_features(leaf_t::version_info, subleaf_t::main, edx, cpu);
	std::cout << "\n";
	print_features(leaf_t::version_info, subleaf_t::main, ecx, cpu);
	std::cout << std::endl;
}

void print_serial_number(const cpu_t& cpu) {
	using namespace fmt::literals;

	std::cout << "Processor serial number: ";
	const register_set_t& regs = cpu.leaves.at(leaf_t::serial_number).at(subleaf_t::main);
	switch(cpu.vendor) {
	case intel:
		{
			const std::uint32_t top = cpu.leaves.at(leaf_t::version_info).at(subleaf_t::main)[eax];
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
	const register_set_t& regs = cpu.leaves.at(leaf_t::monitor_mwait).at(subleaf_t::main);

	if(regs[eax] == 0ui32 && regs[ebx] == 0ui32) {
		return;
	}

	const union
	{
		std::uint32_t full;
		struct
		{
			std::uint32_t smallest_monitor_line : 16;
			std::uint32_t reserved_1            : 16;
		} split;
	} a = { regs[eax] };

	const union
	{
		std::uint32_t full;
		struct
		{
			std::uint32_t largest_monitor_line : 16;
			std::uint32_t reserved_1           : 16;
		} split;
	} b = { regs[ebx] };

	const union
	{
		std::uint32_t full;
		struct
		{
			std::uint32_t enumerable           : 1;
			std::uint32_t interrupts_as_breaks : 1;
			std::uint32_t reserved_1           : 30;
		} split;
	} c = { regs[ecx] };

	using namespace fmt::literals;

	fmt::MemoryWriter w;
	w << "MONITOR/MWAIT leaf\n";
	w << "\tSmallest monitor-line size: {:d} bytes\n"_format(a.split.smallest_monitor_line + 0ui32);
	w << "\tLargest monitor-line size: {:d} bytes\n"_format (b.split.largest_monitor_line  + 0ui32);
	if(c.split.enumerable) {
		if(c.split.interrupts_as_breaks) {
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
	const register_set_t& regs = cpu.leaves.at(leaf_t::thermal_and_power).at(subleaf_t::main);
	std::cout << "Thermal and Power Management\n";
	print_features(leaf_t::thermal_and_power, subleaf_t::main, eax, cpu);
	std::cout << std::endl;

	if(cpu.vendor == intel) {
		const union
		{
			std::uint32_t full;
			struct
			{
				std::uint32_t interrupt_thresholds : 4;
				std::uint32_t reserved_1           : 28;
			} split;
		} b = { regs[ebx] };

		const union
		{
			std::uint32_t full;
			struct
			{
				std::uint32_t hcf : 1;
				std::uint32_t reserved_1 : 2;
				std::uint32_t bias_preference : 1;
				std::uint32_t reserved_2 : 28;
			} split;
		} c = { regs[ecx] };

		if(b.split.interrupt_thresholds) {
			std::cout << b.split.interrupt_thresholds << " interrupt thresholds in Digital Thermal Sensor\n";
		}
		print_features(leaf_t::thermal_and_power, subleaf_t::main, ecx, cpu);
		std::cout << std::endl;
	}
}

void enumerate_extended_features(cpu_t& cpu) {
	register_set_t regs = { 0 };
	cpuid(regs, leaf_t::extended_features, subleaf_t::main);
	cpu.leaves[leaf_t::extended_features][subleaf_t::main] = regs;

	const subleaf_t limit = subleaf_t{ regs[eax] };
	for(subleaf_t sub = subleaf_t{ 1 }; sub < limit; ++sub) {
		cpuid(regs, leaf_t::extended_features, sub);
		cpu.leaves[leaf_t::extended_features][sub] = regs;
	}
}

void print_extended_features(const cpu_t& cpu) {
	const register_set_t& regs = cpu.leaves.at(leaf_t::extended_features).at(subleaf_t::main);
	std::cout << "Extended features\n";
	print_features(leaf_t::extended_features, subleaf_t::main, ebx, cpu);
	std::cout << "\n";
	if(cpu.vendor & intel) {
		print_features(leaf_t::extended_features, subleaf_t::main, ecx, cpu);
		std::cout << "\n";
	}
	print_features(leaf_t::extended_features, subleaf_t::main, edx, cpu);
	std::cout << std::endl;

	const union
	{
		std::uint32_t full;
		struct
		{
			std::uint32_t reserved_1  : 17;
			std::uint32_t mawau_value : 5;
			std::uint32_t reserve_2   : 10;
		} split;
	} c = { regs[ecx] };

	if(cpu.vendor & intel) {
		std::cout << "\tMAWAU value: " << c.split.mawau_value << "\n";
		std::cout << std::endl;
	}
}

void print_direct_cache_access(const cpu_t& cpu) {
	const register_set_t& regs = cpu.leaves.at(leaf_t::direct_cache_access).at(subleaf_t::main);
	std::cout << "Direct Cache Access\n";
	std::cout << "\t" << std::setw(8) << std::setfill('0') << std::hex << regs[eax] << "\n";
	std::cout << std::endl;
}

void print_performance_monitoring(const cpu_t& cpu) {
	const register_set_t& regs = cpu.leaves.at(leaf_t::performance_monitoring).at(subleaf_t::main);

	const union
	{
		std::uint32_t full;
		struct
		{
			std::uint32_t version              : 8;
			std::uint32_t counters_per_logical : 8;
			std::uint32_t counter_bit_width    : 8;
			std::uint32_t ebx_length           : 8;
		} split;
	} a = { regs[eax] };

	const union
	{
		std::uint32_t full;
		struct
		{
			std::uint32_t core_cycle           : 1;
			std::uint32_t instructions_retired : 1;
			std::uint32_t reference_cycles     : 1;
			std::uint32_t llc_reference        : 1;
			std::uint32_t llc_misses           : 1;
			std::uint32_t branch_retired       : 1;
			std::uint32_t branch_mispredict    : 1;
			std::uint32_t reserved_1           : 25;
		} split;
	} b = { regs[ebx] };

	const union
	{
		std::uint32_t full;
		struct
		{
			std::uint32_t fixed_function_counters      : 5;
			std::uint32_t fixed_function_counter_width : 8;
			std::uint32_t reserved_1                   : 2;
			std::uint32_t any_thread                   : 1;
			std::uint32_t reserved_2                   : 16;
		} split;
	} d = { regs[edx] };

	if(a.split.version == 0) {
		return;
	}

	std::cout << "Architectural Performance Monitoring\n";
	std::cout << "\tVersion: " << a.split.version << "\n";
	std::cout << "\tCounters per logical processor: " << a.split.counters_per_logical << "\n";
	std::cout << "\tCounter bit width: " << a.split.counter_bit_width << "\n";
	std::cout << "\tFixed function counters: " << d.split.fixed_function_counters << "\n";
	std::cout << "\tFixed function counter bit width: " << d.split.fixed_function_counter_width << "\n";

	std::cout << "\tSupported counters\n";
	if(0 == b.split.core_cycle) {
		std::cout << "\t\tCore cycles\n";
	}
	if(0 == b.split.instructions_retired) {
		std::cout << "\t\tInstructions retired\n";
	}
	if(0 == b.split.reference_cycles) {
		std::cout << "\t\tReference cycles\n";
	}
	if(0 == b.split.llc_reference) {
		std::cout << "\t\tLast-level cache reference\n";
	}
	if(0 == b.split.llc_misses) {
		std::cout << "\t\tLast-level cache misses\n";
	}
	if(0 == b.split.branch_retired) {
		std::cout << "\t\tBranch instructions retired\n";
	}
	if(0 == b.split.branch_mispredict) {
		std::cout << "\t\tBranch instructions mispredicted\n";
	}
	std::cout << std::endl;
}

void enumerate_extended_state(cpu_t& cpu) {
	register_set_t regs = { 0 };
	cpuid(regs, leaf_t::extended_state, subleaf_t::extended_state_main);
	cpu.leaves[leaf_t::extended_state][subleaf_t::extended_state_main] = regs;
	cpuid(regs, leaf_t::extended_state, subleaf_t::extended_state_sub);
	cpu.leaves[leaf_t::extended_state][subleaf_t::extended_state_sub] = regs;

	const std::uint64_t valid_bits = regs[eax] | (gsl::narrow_cast<std::uint64_t>(regs[edx]) << 32ui64);
	std::uint64_t mask = 0x1ui64 << 2ui64;
	for(subleaf_t i = subleaf_t{ 2ui32 }; i < subleaf_t{ 63ui32 }; ++i, mask <<= 1ui64) {
		if(valid_bits & mask) {
			cpuid(regs, leaf_t::extended_state, i);
			if(regs[ebx] != 0ui32) {
				cpu.leaves[leaf_t::extended_state][i] = regs;
			}
		}
	}
	if(cpu.vendor & amd) {
		cpuid(regs, leaf_t::extended_state, subleaf_t{ 0x3e });
		if(regs[ebx] != 0ui32) {
			cpu.leaves[leaf_t::extended_state][subleaf_t{ 0x3e }] = regs;
		}
	}
}

void print_extended_state(const cpu_t& cpu) {
	static const std::vector<feature_t> saveables = {
		{ intel | amd, 0x0000'0001ui32, "x87"         , "Legacy x87 floating point"    },
		{ intel | amd, 0x0000'0002ui32, "SSE"         , "128-bit SSE XMM"              },
		{ intel | amd, 0x0000'0004ui32, "AVX"         , "256-bit AVX YMM"              },
		{ intel      , 0x0000'0008ui32, "MPX_bounds"  , "MPX bounds registers"         },
		{ intel      , 0x0000'0010ui32, "MPX_CSR"     , "MPX CSR"                      },
		{ intel      , 0x0000'0020ui32, "AVX512_mask" , "AVX-512 OpMask"               },
		{ intel      , 0x0000'0040ui32, "AVX512_hi256", "AVX-512 ZMM0-15 upper bits"   },
		{ intel      , 0x0000'0080ui32, "AVX512_hi16" , "AVX-512 ZMM16-31"             },
		{ intel      , 0x0000'0100ui32, "XSS"         , "Processor Trace"              },
		{ intel      , 0x0000'0200ui32, "PKRU"        , "Protection Keys User Register"},
	};

	static const std::vector<feature_t> optional_features = {
		{ intel | amd, 0x0000'0001ui32, "XSAVEOPT"    , "XSAVEOPT available"           },
		{ intel | amd, 0x0000'0002ui32, "XSAVEC"      , "XSAVEC and compacted XRSTOR"  },
		{ intel | amd, 0x0000'0004ui32, "XGETBV"      , "XGETBV"                       },
		{ intel | amd, 0x0000'0008ui32, "XSAVES"      , "XSAVES/XRSTORS"               },
	};

	std::cout << "Extended states" << std::endl;
	for(const auto& sub : cpu.leaves.at(leaf_t::extended_state)) {
		const register_set_t& regs = sub.second;
		switch(sub.first) {
		case subleaf_t::extended_state_main:
			{
				std::cout << "\tFeatures supported by XSAVE: \n";
				for(const feature_t& feature : saveables) {
					if(cpu.vendor & feature.vendor) {
						if(regs[eax] & feature.mask) {
							std::cout << "\t\t" << feature.description << "\n";
						}
					}
				}
				std::cout << std::endl;

				std::cout << "\tMaximum size for all enabled features  : " << regs[ebx] << " bytes\n";
				std::cout << "\tMaximum size for all supported features: " << regs[ecx] << " bytes\n";
				std::cout << std::endl;
			}
			break;
		case subleaf_t::extended_state_sub:
			{
				std::cout << "\tXSAVE extended features:\n";
				for(const feature_t& feature : optional_features) {
					if(cpu.vendor & feature.vendor) {
						if(regs[eax] & feature.mask) {
							std::cout << "\t\t" << feature.description << "\n";
						}
					}
				}
				std::cout << std::endl;

				std::cout << "\tSize for enabled features: " << std::dec << regs[ebx] << " bytes\n";
				std::cout << std::endl;
			}
			break;
		default:
			{
				const union
				{
					std::uint32_t full;
					struct
					{
						std::uint32_t set_in_xss          : 1;
						std::uint32_t aligned_to_64_bytes : 1;
						std::uint32_t reserved_1          : 30;
					} split;
				} c = { regs[ecx] };

				const std::uint32_t idx = static_cast<std::uint32_t>(sub.first);
				const char* const description = idx < saveables.size() ? saveables[idx].description
				                              : idx == 0xe3ui32        ? "Lightweight Profiling"
				                              :                          "(unknown)";

				std::cout << "\tExtended state for " << description << " (0x" << std::hex << std::setfill('0') << std::setw(2) << static_cast<std::uint32_t>(sub.first) << ") uses " << regs[eax] << " bytes at offset " << regs[ebx] << "\n";
				if(c.split.set_in_xss) {
					std::cout << "\t\tBit set in XSS MSR\n";
				} else {
					std::cout << "\t\tBit set in XCR0\n";
				}
				if(c.split.aligned_to_64_bytes) {
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

void enumerate_rdt_monitoring(cpu_t& cpu) {
	register_set_t regs = { 0 };
	cpuid(regs, leaf_t::rdt_monitoring, subleaf_t::rdt_monitoring_main);
	cpu.leaves[leaf_t::rdt_monitoring][subleaf_t::rdt_monitoring_main] = regs;

	const std::uint32_t valid_bits = regs[edx];
	std::uint32_t mask = 0x1ui32 << 1ui32;
	for(subleaf_t i = subleaf_t::rdt_monitoring_l3; i < subleaf_t{ 32 }; ++i, mask <<= 1ui32) {
		if(valid_bits & mask) {
			cpuid(regs, leaf_t::rdt_monitoring, i);
			cpu.leaves[leaf_t::rdt_monitoring][i] = regs;
		}
	}
}

void print_rdt_monitoring(const cpu_t& cpu) {
	static const std::vector<feature_t> monitorables = {
		{ intel , 0x0000'0001ui32, "O", "Occupancy"       },
		{ intel , 0x0000'0002ui32, "T", "Total Bandwidth" },
		{ intel , 0x0000'0004ui32, "L", "Local Bandwidth" }
	};

	for(const auto& sub : cpu.leaves.at(leaf_t::rdt_monitoring)) {
		const register_set_t& regs = sub.second;
		switch(sub.first) {
		case subleaf_t::rdt_monitoring_main:
			std::cout << "Intel Resource Director Technology monitoring\n";
			std::cout << "\tMaximum Resource Monitoring ID of all types: " << std::hex << regs[ebx] << "\n";
			std::cout << std::endl;
			break;
		case subleaf_t::rdt_monitoring_l3:
			std::cout << "\tL3 cache monitoring\n";
			std::cout << "\t\tConversion factor: " << regs[ebx] << "\n";
			for(const feature_t& mon : monitorables) {
				if(regs[edx] & mon.mask) {
					std::cout << "\t\tL3 " << mon.description << "\n";
				}
			}
			std::cout << std::endl;
			break;
		default:
			std::cout << "\tUnknown resource type (0x" << std::setw(2) << std::setfill('0') << std::hex << static_cast<std::uint32_t>(sub.first) << ") monitoring\n";
			std::cout << "\t\tConversion factor: " << regs[ebx] << "\n";
			for(const feature_t& mon : monitorables) {
				if(regs[edx] & mon.mask) {
					std::cout << "\t\tUnknown resource " << mon.description << "\n";
				}
			}
			std::cout << std::endl;
			break;
		}
	}
}

void enumerate_rdt_allocation(cpu_t& cpu) {
	register_set_t regs = { 0 };
	cpuid(regs, leaf_t::rdt_allocation, subleaf_t::rdt_allocation_main);
	cpu.leaves[leaf_t::rdt_allocation][subleaf_t::rdt_allocation_main] = regs;

	const std::uint32_t valid_bits = regs[edx];
	std::uint32_t mask = 0x1ui32 << 1ui32;
	for(subleaf_t i = subleaf_t::rdt_cat_l3; i < subleaf_t{ 32 }; ++i, mask <<= 1ui32) {
		if(valid_bits & mask) {
			cpuid(regs, leaf_t::rdt_allocation, i);
			cpu.leaves[leaf_t::rdt_allocation][i] = regs;
		}
	}
}

void print_rdt_allocation(const cpu_t& cpu) {
	for(const auto& sub : cpu.leaves.at(leaf_t::rdt_allocation)) {
		const register_set_t& regs = sub.second;

		const union
		{
			std::uint32_t full;
			struct
			{
				std::uint32_t highest_cos_number : 16;
				std::uint32_t reserved_1         : 16;
			} split;
		} d = { regs[edx] };

		switch(sub.first) {
		case subleaf_t::rdt_allocation_main:
			std::cout << "Intel Resource Director Technology allocation\n";
			std::cout << std::endl;
			break;
		case subleaf_t::rdt_cat_l3:
			{
				const union
				{
					std::uint32_t full;
					struct
					{
						std::uint32_t bit_mask_length : 5;
						std::uint32_t reserved_1      : 27;
					} split;
				} a = { regs[eax] };

				std::cout << "\tL3 Cache Allocation Technology\n";
				std::cout << "\tLength of capacity bitmask: " << std::dec << (a.split.bit_mask_length + 1ui32) << "\n";
				std::cout << "\tBitmap of isolation/contention: 0x" << std::setw(8) << std::setfill('0') << regs[ebx] << "\n";
				if(regs[ecx] & 0x0000'0004ui32) {
					std::cout << "\tCode and Data Prioritization supported\n";
				}
				std::cout << "\tHighest COS number for this resource: " << std::dec << d.split.highest_cos_number << std::endl;
				std::cout << std::endl;
			}
			break;
		case subleaf_t::rdt_cat_l2:
			{
				const union
				{
					std::uint32_t full;
					struct
					{
						std::uint32_t bit_mask_length : 5;
						std::uint32_t reserved_1      : 27;
					} split;
				} a = { regs[eax] };

				std::cout << "\tL2 Cache Allocation Technology\n";
				std::cout << "\tLength of capacity bitmask: " << std::dec << (a.split.bit_mask_length + 1ui32) << "\n";
				std::cout << "\tBitmap of isolation/contention: 0x" << std::setw(8) << std::setfill('0') << regs[ebx] << "\n";
				std::cout << "\tHighest COS number for this resource: " << std::dec << d.split.highest_cos_number << std::endl;
				std::cout << std::endl;
			}
			break;
		case subleaf_t::rdt_mba:
			{
				const union
				{
					std::uint32_t full;
					struct
					{
						std::uint32_t max_throttle : 12;
						std::uint32_t reserved_1   : 20;
					} split;
				} a = { regs[eax] };

				std::cout << "\tMemory Bandwidth Allocation\n";
				std::cout << "\tMaximum MBA throttling value: " << std::dec << (a.split.max_throttle + 1ui32) << "\n";
				if(regs[ecx] & 0x0000'0004ui32) {
					std::cout << "\tResponse of delay values is linear\n";
				}
				std::cout << "\tHighest COS number for this resource: " << std::dec << d.split.highest_cos_number << std::endl;
				std::cout << std::endl;
			}
			break;
		default:
			std::cout << "\tUnknown resource type (0x" << std::setw(2) << std::setfill('0') << std::hex << static_cast<std::uint32_t>(sub.first) << ") allocation\n";
			std::cout << "\tHighest COS number for this resource: " << std::dec << d.split.highest_cos_number << std::endl;
			std::cout << std::endl;
			break;
		}
	}
}

void enumerate_sgx_info(cpu_t& cpu) {
	register_set_t regs = { 0 };
	cpuid(regs, leaf_t::sgx_info, subleaf_t::sgx_capabilities);
	cpu.leaves[leaf_t::sgx_info][subleaf_t::sgx_capabilities] = regs;

	cpuid(regs, leaf_t::sgx_info, subleaf_t::sgx_attributes);
	cpu.leaves[leaf_t::sgx_info][subleaf_t::sgx_attributes] = regs;

	for(subleaf_t i = subleaf_t{ 2 }; ; ++i) {
		cpuid(regs, leaf_t::sgx_info, i);
		if(0 == (regs[eax] & 0xfui32)) {
			break;
		}
		cpu.leaves[leaf_t::sgx_info][i] = regs;
	}
}

void print_sgx_info(const cpu_t& cpu) {
	static const std::vector<feature_t> sgx_features = {
		{ intel, 0x0000'0001ui32, "SGX1" , "SGX1 functions available"                      },
		{ intel, 0x0000'0002ui32, "SGX2" , "SGX2 functions available"                      },
		{ intel, 0x0000'0020ui32, "ENCLV", "EINCVIRTCHILD, EDECVIRTCHILD, and ESETCONTEXT" },
		{ intel, 0x0000'0040ui32, "ENCLS", "ETRACKC, ERDINFO, ELDBC, and ELDUC"            },
	};

	for(const auto& sub : cpu.leaves.at(leaf_t::sgx_info)) {
		const register_set_t& regs = sub.second;

		switch(sub.first) {
		case subleaf_t::sgx_capabilities:
			{
				const union
				{
					std::uint32_t full;
					struct
					{
						std::uint32_t max_enclave_32_bit : 8;
						std::uint32_t max_enclave_64_bit : 8;
						std::uint32_t reserved_1         : 16;
					} split;
				} d = { regs[edx] };

				std::cout << "Intel SGX\n";
				std::cout << "\tFeatures:\n";
				for(const feature_t& f : sgx_features) {
					if(regs[eax] & f.mask) {
						std::cout << "\t\t" << f.description << "\n";
					}
				}
				std::cout << std::endl;
				std::cout << "\tMISCSELECT extended features: 0x" << std::setw(8) << std::setfill('0') << std::hex << regs[ebx] << "\n";
				std::cout << "\tMaximum enclave size in 32-bit mode: " << std::dec << (2ui64 << d.split.max_enclave_32_bit) << " bytes\n";
				std::cout << "\tMaximum enclave size in 64-bit mode: " << std::dec << (2ui64 << d.split.max_enclave_64_bit) << " bytes\n";
				std::cout << std::endl;
			}
			break;
		case subleaf_t::sgx_attributes:
			{
				std::cout << "\tSECS.ATTRIBUTES valid bits: " << std::setw(8) << std::setfill('0') << std::hex << regs[edx]
				                                              << std::setw(8) << std::setfill('0') << std::hex << regs[ecx]
				                                              << std::setw(8) << std::setfill('0') << std::hex << regs[ebx]
				                                              << std::setw(8) << std::setfill('0') << std::hex << regs[eax];
				std::cout << std::endl;
			}
			break;
		default:
			{
				const union
				{
					std::uint32_t full;
					struct
					{
						std::uint32_t type                          : 4;
						std::uint32_t reserved_1                    : 8;
						std::uint32_t epc_physical_address_low_bits : 20;
					} split;
				} a = { regs[eax] };

				const union
				{
					std::uint32_t full;
					struct
					{
						std::uint32_t epc_physical_address_hi_bits : 20;
						std::uint32_t reserved_1                   : 12;
					} split;
				} b = { regs[ebx] };

				const union
				{
					std::uint32_t full;
					struct
					{
						std::uint32_t epc_section_properties    : 4;
						std::uint32_t reserved_1                : 8;
						std::uint32_t epc_section_size_low_bits : 20;
					} split;
				} c = { regs[ecx] };
				const union
				{
					std::uint32_t full;
					struct
					{
						std::uint32_t epc_section_size_hi_bits : 20;
						std::uint32_t reserved_1               : 12;
					} split;
				} d = { regs[edx] };

				std::cout << "\tEnclave Page Cache section\n";

				const std::uint64_t physical_address = (gsl::narrow_cast<std::uint64_t>(b.split.epc_physical_address_hi_bits ) << 32ui64)
				                                     | (gsl::narrow_cast<std::uint64_t>(a.split.epc_physical_address_low_bits) << 12ui64);
				const std::uint64_t epc_size = (gsl::narrow_cast<std::uint64_t>(d.split.epc_section_size_hi_bits ) << 32ui64)
				                             | (gsl::narrow_cast<std::uint64_t>(c.split.epc_section_size_low_bits) << 12ui64);
				std::cout << "\t\tEPC physical address: 0x" << std::setw(16) << std::setfill('0') << std::hex << physical_address << "\n";
				std::cout << "\t\tEPC size: 0x"             << std::setw(16) << std::setfill('0') << std::hex << epc_size << "\n";
				std::cout << std::endl;
			}
			break;
		}
	}
}

void enumerate_processor_trace(cpu_t& cpu) {
	register_set_t regs = { 0 };
	cpuid(regs, leaf_t::processor_trace, subleaf_t::main);
	cpu.leaves[leaf_t::processor_trace][subleaf_t::main] = regs;

	const subleaf_t limit = subleaf_t{ regs[eax] };
	for(subleaf_t sub = subleaf_t{ 1 }; sub < limit; ++sub) {
		cpuid(regs, leaf_t::processor_trace, sub);
		cpu.leaves[leaf_t::processor_trace][sub] = regs;
	}
}

void print_processor_trace(const cpu_t& cpu) {
	for(const auto& sub : cpu.leaves.at(leaf_t::processor_trace)) {
		const register_set_t& regs = sub.second;
		switch(sub.first) {
		case subleaf_t::main:
			std::cout << "Processor Trace\n";
			print_features(leaf_t::processor_trace, subleaf_t::main, ebx, cpu);
			std::cout << std::endl;
			print_features(leaf_t::processor_trace, subleaf_t::main, ecx, cpu);
			std::cout << std::endl;
			break;
		default:
			{
				const union
				{
					std::uint32_t full;
					struct
					{
						std::uint32_t number_of_ranges  : 3;
						std::uint32_t reserved_1        : 13;
						std::uint32_t mtc_period_bitmap : 16;
					} split;
				} a = { regs[eax] };

				const union
				{
					std::uint32_t full;
					struct
					{
						std::uint32_t cycle_threshold_bitmap : 16;
						std::uint32_t supported_psb_bitmap : 16;
					} split;
				} b = { regs[ebx] };
				std::cout << "\tNumber of configurable address ranges for filtering: " << std::dec << a.split.number_of_ranges << "\n";
				std::cout << "\tBitmap of supported MTC period encodings: " << std::setw(8) << std::setfill('0') << std::hex << a.split.mtc_period_bitmap << "\n";
				std::cout << "\tBitmap of supported Cycle Treshold value encodings: " << std::setw(8) << std::setfill('0') << std::hex << b.split.cycle_threshold_bitmap << "\n";
				std::cout << "\tBitmap of supported Configurable PSB frequency encodings: " << std::setw(8) << std::setfill('0') << std::hex << b.split.supported_psb_bitmap << "\n";
				std::cout << std::endl;
			}
			break;
		}
	}
}

void print_time_stamp_counter(const cpu_t& cpu) {
	const register_set_t& regs = cpu.leaves.at(leaf_t::time_stamp_counter).at(subleaf_t::main);
	std::cout << "Time Stamp Counter and Nominal Core Crystal Clock\n";
	std::cout << "\tTSC:core crystal clock ratio: " << std::dec << regs[ebx] << ":" << regs[eax] << std::endl;
	std::cout << "\tNominal core crystal clock/Hz: " << std::dec << regs[ecx] << "\n";
	std::cout << "\tTSC frequency/Hz: " << gsl::narrow_cast<std::uint64_t>(regs[ecx] * regs[ebx]) / regs[eax] << "\n";
	std::cout << std::endl;
}

void print_processor_frequency(const cpu_t& cpu) {
	const register_set_t& regs = cpu.leaves.at(leaf_t::processor_frequency).at(subleaf_t::main);

	struct frequency_t
	{
		std::uint32_t frequency : 16;
		std::uint32_t reserved_1 : 16;
	};

	const union
	{
		std::uint32_t full;
		frequency_t split;
	} a = { regs[eax] };

	const union
	{
		std::uint32_t full;
		frequency_t split;
	} b = { regs[ebx] };

	const union
	{
		std::uint32_t full;
		frequency_t split;
	} c = { regs[ecx] };

	std::cout << "Processor frequency\n";
	std::cout << "\tBase frequency/MHz: " << std::dec << a.split.frequency << "\n";
	std::cout << "\tMaximum frequency/MHz: " << std::dec << b.split.frequency << "\n";
	std::cout << "\tBus (reference) frequency/MHz: " << std::dec << c.split.frequency << "\n";
	std::cout << std::endl;
}

void enumerate_system_on_chip_vendor(cpu_t& cpu) {
	register_set_t regs = { 0 };
	cpuid(regs, leaf_t::system_on_chip_vendor, subleaf_t::main);
	cpu.leaves[leaf_t::system_on_chip_vendor][subleaf_t::main] = regs;

	const subleaf_t limit = subleaf_t{ regs[eax] };
	for(subleaf_t sub = subleaf_t{ 1 }; sub < limit; ++sub) {
		cpuid(regs, leaf_t::system_on_chip_vendor, sub);
		cpu.leaves[leaf_t::system_on_chip_vendor][sub] = regs;
	}
}

void print_system_on_chip_vendor(const cpu_t& cpu) {
	for(const auto& sub : cpu.leaves.at(leaf_t::system_on_chip_vendor)) {
		const register_set_t& regs = sub.second;
		switch(sub.first) {
		case subleaf_t::main:
			{
				const union
				{
					std::uint32_t full;
					struct
					{
						std::uint32_t vendor_id                   : 16;
						std::uint32_t is_industry_standard_vendor : 1;
						std::uint32_t reserved_1                  : 15;
					} split;
				} b = { regs[ebx] };

				std::cout << "System-on-chip\n";
				std::cout << "\tVendor ID: " << std::setw(4) << std::setfill('0') << std::hex << b.split.vendor_id << "\n";
				if(b.split.is_industry_standard_vendor) {
					std::cout << "\tVendor ID is assigned by an industry standard scheme\n";
				} else {
					std::cout << "\tVendor ID is assigned by Intel\n";
				}
				std::cout << "\tProject ID: " << std::setw(8) << std::setfill('0') << std::hex << regs[ecx] << "\n";
				std::cout << "\tStepping: " << std::setw(8) << std::setfill('0') << std::hex << regs[edx] << "\n";
				std::cout << std::endl;
			}
			break;
		case subleaf_t{ 1 }:
			{
				const union
				{
					std::array<register_set_t, 3> split;
					std::array<char, 48> full;
				} brand = {
					cpu.leaves.at(leaf_t::system_on_chip_vendor).at(subleaf_t{ 1 }),
					cpu.leaves.at(leaf_t::system_on_chip_vendor).at(subleaf_t{ 2 }),
					cpu.leaves.at(leaf_t::system_on_chip_vendor).at(subleaf_t{ 3 })
				};
				std::cout << "\tSoC brand: " << brand.full.data() << "\n";
				std::cout << std::endl;
			}
			break;
		case subleaf_t{ 2 }:
		case subleaf_t{ 3 }:
			break;
		default:
			std::cout << "\tVendor data:\n";
			std::cout << "\t\t" << std::setw(8) << std::setfill('0') << std::hex << regs[eax] << "\n";
			std::cout << "\t\t" << std::setw(8) << std::setfill('0') << std::hex << regs[ebx] << "\n";
			std::cout << "\t\t" << std::setw(8) << std::setfill('0') << std::hex << regs[ecx] << "\n";
			std::cout << "\t\t" << std::setw(8) << std::setfill('0') << std::hex << regs[edx] << "\n";
			std::cout << std::endl;
			break;
		}
	}
}

void print_extended_limit(const cpu_t& cpu) {
	const register_set_t& regs = cpu.leaves.at(leaf_t::extended_limit).at(subleaf_t::main);
	std::cout << "Extended limit\n";
	std::cout << "\tMaximum extended cpuid leaf: 0x" << std::setw(2) << std::setfill('0') << std::hex << regs[eax] << "\n";
	std::cout << std::endl;
}

void print_extended_signature_and_features(const cpu_t& cpu) {
	const register_set_t& regs = cpu.leaves.at(leaf_t::extended_signature_and_features).at(subleaf_t::main);
	std::cout << "Extended signature and features\n";
	
	if(cpu.vendor & amd) {
		const union
		{
			std::uint32_t full;
			struct
			{
				std::uint32_t brand_id     : 16;
				std::uint32_t reserved_1   : 12;
				std::uint32_t package_type : 4;
			} split;
		} b = { regs[ebx] };
		std::cout << "\tBrand ID: " << std::setw(2) << std::setfill('0') << std::hex << b.split.brand_id << "\n";
		std::cout << "\tPackage: ";
		switch(b.split.package_type) {
		case 0x0:
			std::cout << "FP4 (BGA)/FT3 (BGA)/FT3b (BGA)\n";
			break;
		case 0x1:
			std::cout << "AM3r2/FS1b\n";
			break;
		case 0x2:
			std::cout << "AM4 (\u00b5PGA)\n";
			break;
		case 0x3:
			std::cout << "G34r1/FP4\n";
			break;
		case 0x4:
			std::cout << "FT4 (BGA)\n";
			break;
		case 0x5:
			std::cout << "C32r1\n";
			break;
		default:
			std::cout << "Unknown\n";
			break;
		}
		std::cout << std::endl;
	}
	std::cout << "\tFeature identifiers\n";
	print_features(leaf_t::extended_signature_and_features, subleaf_t::main, ecx, cpu);
	std::cout << std::endl;
	print_features(leaf_t::extended_signature_and_features, subleaf_t::main, edx, cpu);
	std::cout << std::endl;
}

void print_brand_string(const cpu_t& cpu) {
	std::cout << "Processor brand string\n";
	const union
	{
		std::array<register_set_t, 3> split;
		std::array<char, 48> full;
	} brand = {
		cpu.leaves.at(leaf_t::brand_string_0).at(subleaf_t::main),
		cpu.leaves.at(leaf_t::brand_string_1).at(subleaf_t::main),
		cpu.leaves.at(leaf_t::brand_string_2).at(subleaf_t::main)
	};

	std::cout << "\t";
	std::cout.write(brand.full.data(), 48);
	std::cout << "\n";
	std::cout << std::endl;
}

void print_ras_advanced_power_management(const cpu_t& cpu) {
	const register_set_t& regs = cpu.leaves.at(leaf_t::ras_advanced_power_management).at(subleaf_t::main);

	const union
	{
		std::uint32_t full;
		struct
		{
			std::uint32_t number_of_monitors : 8;
			std::uint32_t version            : 8;
			std::uint32_t max_wrap_time      : 16;
		} split;
	} a = { regs[eax] };

	switch(cpu.vendor) {
	case amd:
		std::cout << "Processor feedback capabilities\n";
		std::cout << "\tNumber of monitors: " << std::dec << a.split.number_of_monitors << "\n";
		std::cout << "\tVersion: " << std::dec << a.split.version << "\n";
		std::cout << "\tMaximum seconds between readings to avoid wraps: " << a.split.max_wrap_time << "\n";
		std::cout << std::endl;
		std::cout << "RAS capabilities\n";
		print_features(leaf_t::ras_advanced_power_management, subleaf_t::main, ebx, cpu);
		std::cout << std::endl;
		std::cout << "Advanced Power Management information\n";
		std::cout << "\tCompute unit power sample time period: " << std::dec << regs[ecx] << "\n";
		print_features(leaf_t::ras_advanced_power_management, subleaf_t::main, edx, cpu);
		std::cout << std::endl;
		break;
	case intel:
		std::cout << "Advanced Power Management information\n";
		print_features(leaf_t::ras_advanced_power_management, subleaf_t::main, edx, cpu);
		std::cout << std::endl;
		break;
	}
}

void print_address_limits(const cpu_t& cpu) {
	const register_set_t& regs = cpu.leaves.at(leaf_t::address_limits).at(subleaf_t::main);

	const union
	{
		std::uint32_t full;
		struct
		{
			std::uint32_t physical_address_size : 8;
			std::uint32_t virtual_address_size  : 8;
			std::uint32_t guest_physical_size   : 8;
			std::uint32_t reserved_1            : 8;
		} split;
	} a = { regs[eax] };

	const union
	{
		std::uint32_t full;
		struct
		{
			std::uint32_t package_threads : 8;
			std::uint32_t reserved_1      : 4;
			std::uint32_t apic_id_size    : 4;
			std::uint32_t perf_tsc_size   : 2;
			std::uint32_t reserved_2      : 24;
		} split;
	} c = { regs[ecx] };

	switch(cpu.vendor) {
	case amd:
		std::cout << "Address size limits\n";
		std::cout << "\tPhysical address size/bits: " << std::dec << a.split.physical_address_size << "\n";
		std::cout << "\tVirtual address size/bits: " << std::dec << a.split.virtual_address_size << "\n";
		if(0 == a.split.guest_physical_size) {
			std::cout << "\tGuest physical size matches machine physical size\n";
		}
		std::cout << std::endl;

		std::cout << "\tExtended features\n";
		print_features(leaf_t::address_limits, subleaf_t::main, ebx, cpu);
		std::cout << std::endl;

		std::cout << "\tSize identifiers\n";
		std::cout << "\t\tThreads in package: " << c.split.package_threads + 1ui32 << "\n";
		std::cout << "\t\t" << c.split.apic_id_size << " bits of APIC ID denote threads within a package\n";

		if(0 != (cpu.leaves.at(leaf_t::extended_signature_and_features).at(subleaf_t::main).at(ecx) & 0x0400'0000ui32)) {
			std::cout << "\t\tPerforamnce time-stamp counter size/bits: ";
			switch(c.split.perf_tsc_size) {
			case 0b00ui32:
				std::cout << "40";
				break;
			case 0b01ui32:
				std::cout << "48";
				break;
			case 0b10ui32:
				std::cout << "56";
				break;
			case 0b11ui32:
				std::cout << "64";
				break;
			}
			std::cout << "\n";
		}

		std::cout << std::endl;
		break;
	case intel:
		std::cout << "Address size limits\n";
		std::cout << "\tPhysical address size/bits: " << std::dec << a.split.physical_address_size << "\n";
		std::cout << "\tVirtual address size/bits: " << std::dec << a.split.virtual_address_size << "\n";
		std::cout << std::endl;
		break;
	}
}

void print_secure_virtual_machine(const cpu_t& cpu) {
	const register_set_t& regs = cpu.leaves.at(leaf_t::secure_virtual_machine).at(subleaf_t::main);

	const union
	{
		std::uint32_t full;
		struct
		{
			std::uint32_t svm_revision : 8;
			std::uint32_t reserved_1   : 24;
		} split;
	} a = { regs[eax] };

	std::cout << "Secure Virtual Machine\n";
	std::cout << "\tSVM revision: " << a.split.svm_revision << "\n";
	std::cout << "\tNumber of address space identifiers: " << regs[ebx] << "\n";
	std::cout << "\n";
	print_features(leaf_t::secure_virtual_machine, subleaf_t::main, edx, cpu);
	std::cout << std::endl;
}

void print_performance_optimization(const cpu_t& cpu) {
	std::cout << "Performance Optimization\n";
	print_features(leaf_t::performance_optimization, subleaf_t::main, eax, cpu);
	std::cout << std::endl;
}

void print_instruction_based_sampling(const cpu_t& cpu) {
	std::cout << "Instruction Based Sampling\n";
	print_features(leaf_t::instruction_based_sampling, subleaf_t::main, eax, cpu);
	std::cout << std::endl;
}

void print_lightweight_profiling(const cpu_t& cpu) {
	const register_set_t& regs = cpu.leaves.at(leaf_t::lightweight_profiling).at(subleaf_t::main);

	const union
	{
		std::uint32_t full;
		struct
		{
			std::uint32_t lwpcp_size   : 8;
			std::uint32_t event_size   : 8;
			std::uint32_t max_event_id : 8;
			std::uint32_t event_offset : 8;
		} split;
	} b = { regs[ebx] };

	const union
	{
		std::uint32_t full;
		struct
		{
			std::uint32_t latency_max             : 5;
			std::uint32_t data_address_valid      : 1;
			std::uint32_t latency_rounding        : 3;
			std::uint32_t version                 : 7;
			std::uint32_t minimum_buffer_size     : 8;
			std::uint32_t reserved_1              : 4;
			std::uint32_t branch_prediction       : 1;
			std::uint32_t ip_filtering            : 1;
			std::uint32_t cache_level_filtering   : 1;
			std::uint32_t cache_latency_filtering : 1;
		} split;
	} c = { regs[ecx] };

	std::cout << "Lightweight profiling\n";
	std::cout << "\tLWP version: " << std::dec << c.split.version << "\n";
	print_features(leaf_t::lightweight_profiling, subleaf_t::main, eax, cpu);
	print_features(leaf_t::lightweight_profiling, subleaf_t::main, ecx, cpu);
	std::cout << std::endl;
	std::cout << "\tControl block size/bytes: " << std::dec << (b.split.lwpcp_size * 4ui32) << "\n";
	std::cout << "\tEvent record size/bytes: " << std::dec << b.split.event_size << "\n";
	std::cout << "\tMaximum EventID: " << std::dec << b.split.max_event_id << "\n";
	std::cout << "\tOffset to first interval/bytes: " << std::dec << b.split.event_offset << "\n";
	std::cout << "\tLatency counter size/bits: " << std::dec << c.split.latency_max << "\n";
	std::cout << "\tLatency counter rounding: " << std::dec << c.split.latency_rounding << "\n";
	std::cout << "\tMinimum ring buffer size/32 events: " << std::dec << c.split.minimum_buffer_size << "\n";
	std::cout << std::endl;
}

void print_extended_apic(const cpu_t& cpu) {
	const register_set_t& regs = cpu.leaves.at(leaf_t::extended_apic).at(subleaf_t::main);

	const union
	{
		std::uint32_t full;
		struct
		{
			std::uint32_t core_id          : 8;
			std::uint32_t threads_per_core : 8;
			std::uint32_t reserved_1       : 16;
		} split;
	} b = { regs[ebx] };

	const union
	{
		std::uint32_t full;
		struct
		{
			std::uint32_t node_id             : 8;
			std::uint32_t nodes_per_processor : 3;
			std::uint32_t reserved_1          : 21;
		} split;
	} c = { regs[ecx] };

	std::cout << "Extended APIC\n";
	std::cout << "\tExtended APIC ID: " << std::setw(8) << std::setfill('0') << std::hex << regs[eax] << "\n";
	std::cout << "\tCore ID: " << std::setw(2) << std::setfill('0') << std::hex << b.split.core_id << "\n";
	std::cout << "\tThreads per core: " << std::dec << (b.split.threads_per_core + 1ui32) << "\n";
	std::cout << "\tNode ID: " << std::setw(2) << std::setfill('0') << std::hex << c.split.node_id << "\n";
	std::cout << "\tNodes per processor: " << std::dec << (c.split.nodes_per_processor + 1ui32) << "\n";
	std::cout << std::endl;
}

void print_encrypted_memory(const cpu_t& cpu) {
	const register_set_t& regs = cpu.leaves.at(leaf_t::encrypted_memory).at(subleaf_t::main);

	const union
	{
		std::uint32_t full;
		struct
		{
			std::uint32_t cbit_position              : 6;
			std::uint32_t physical_address_reduction : 6;
		} split;
	} b = { regs[ebx] };

	std::cout << "Encrypted memory\n";
	print_features(leaf_t::encrypted_memory, subleaf_t::main, eax, cpu);
	std::cout << std::endl;
	std::cout << "\tC-bit position in PTE: " << std::dec << b.split.cbit_position << "\n";
	std::cout << "\tPhysical address bit reduction: " << std::dec << b.split.physical_address_reduction << "\n";
	std::cout << "\tNumber of simultaneous encrypted guests: " << std::dec << regs[ecx] << "\n";
	std::cout << "\tMinimum ASID for an SEV-enabled, SEV-ES-disabled gust: " << std::hex << regs[edx] << "\n";
	std::cout << std::endl;
}

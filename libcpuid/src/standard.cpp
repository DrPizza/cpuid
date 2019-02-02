#include "stdafx.h"

#include "standard.hpp"
#include "features.hpp"

#include <map>
#include <vector>

#include <fmt/format.h>

#include "utility.hpp"

void print_basic_info(fmt::memory_buffer& out, const cpu_t& cpu) {
	const register_set_t& regs = cpu.leaves.at(leaf_type::basic_info).at(subleaf_type::main);
	format_to(out, "Basic Information\n");
	format_to(out, "\tMaximum basic cpuid leaf: {:#010x}\n", regs[eax]);

	const union
	{
		std::array<std::uint32_t, 3> registers;
		std::array<char, 12> vndr;
	} data = { regs[ebx], regs[edx], regs[ecx] };

	format_to(out, "\tVendor identifier: {}\n", data.vndr);
	format_to(out, "\tVendor name: {:s}\n", to_string(get_vendor_from_name(regs)));
	format_to(out, "\n");
}

void print_version_info(fmt::memory_buffer& out, const cpu_t& cpu) {
	const register_set_t& regs = cpu.leaves.at(leaf_type::version_info).at(subleaf_type::main);
	format_to(out, "Version Information\n");

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
	
	format_to(out, "\tsignature: {:#010x}\n", regs[eax]);
	format_to(out, "\t   family: {:#02x}\n" , cpu.model.family);
	format_to(out, "\t    model: {:#02x}\n" , cpu.model.model);
	format_to(out, "\t stepping: {:#02x}\n" , cpu.model.stepping);
	format_to(out, "\n");

	if(cpu.vendor & intel) {
		format_to(out, "\t");
		switch(a.split.type) {
		case 0:
			format_to(out, "Original OEM Processor\n");
			break;
		case 1:
			format_to(out, "Intel OverDrive Processor\n");
			break;
		case 2:
			format_to(out, "Dual processor\n");
			break;
		case 3:
			format_to(out, "Intel reserved\n");
			break;
		}
		format_to(out, "\n");

		if(b.split.brand_id != 0_u32) {
			format_to(out, "\tbrand ID: ");
			switch(b.split.brand_id) {
			case 0x00: break;
			case 0x01: format_to(out, "Intel(R) Celeron(R) processor"); break;
			case 0x02: format_to(out, "Intel(R) Pentium(R) III processor"); break;
			case 0x03:
				if(regs[eax] == 0x000006b1_u32) {
					format_to(out, "Intel(R) Celeron(R) processor");
				} else {
					format_to(out, "Intel(R) Pentium(R) III Xeon(R) processor");
				}
			case 0x04: format_to(out, "Intel(R) Pentium(R) III processor"); break;
			case 0x06: format_to(out, "Mobile Intel(R) Pentium(R) III processor-M"); break;
			case 0x07: format_to(out, "Mobile Intel(R) Celeron(R) processor"); break;
			case 0x08: format_to(out, "Intel(R) Pentium(R) 4 processor"); break;
			case 0x09: format_to(out, "Intel(R) Pentium(R) 4 processor"); break;
			case 0x0a: format_to(out, "Intel(R) Celeron(R) processor"); break;
			case 0x0b:
				if(regs[eax] == 0x00000f13_u32) {
					format_to(out, "Intel(R) Xeon(R) processor MP");
				} else {
					format_to(out, "Intel(R) Xeon(R) processor");
				}
			case 0x0c: format_to(out, "Intel(R) Xeon(R) processor MP"); break;
			case 0x0e:
				if(regs[eax] == 0x00000f13_u32) {
					format_to(out, "Intel(R) Xeon(R) processor");
				} else {
					format_to(out, "Mobile Intel(R) Pentium(R) 4 processor - M");
				}
			case 0x0f: format_to(out, "Mobile Intel(R) Celeron(R) processor"); break;
			case 0x11: format_to(out, "Mobile Genuine Intel(R) processor"); break;
			case 0x12: format_to(out, "Intel(R) Celeron(R) M processor"); break;
			case 0x13: format_to(out, "Mobile Intel(R) Celeron(R) processor"); break;
			case 0x14: format_to(out, "Intel(R) Celeron(R) processor"); break;
			case 0x15: format_to(out, "Mobile Genuine Intel(R) processor"); break;
			case 0x16: format_to(out, "Intel(R) Pentium(R) M processor"); break;
			case 0x17: format_to(out, "Mobile Intel(R) Celeron(R) processor"); break;
			default:
				break;
			}
			format_to(out, "\n");
		}
	}
	format_to(out, "\tcache line size/bytes: {:d}\n", b.split.cache_line_size * 8);
	if(0 != (cpu.leaves.at(leaf_type::version_info).at(subleaf_type::main).at(edx) & 0x1000'0000_u32)) {
		format_to(out, "\tlogical processors per package: {:d}\n", gsl::narrow_cast<std::uint32_t>(b.split.maximum_addressable_ids));
	}
	format_to(out, "\tlocal APIC ID: {:#04x}\n", gsl::narrow_cast<std::uint32_t>(b.split.local_apic_id));
	format_to(out, "\n");

	format_to(out, "\tFeature identifiers\n");
	print_features(out, cpu, leaf_type::version_info, subleaf_type::main, edx);
	format_to(out, "\n");
	print_features(out, cpu, leaf_type::version_info, subleaf_type::main, ecx);
	format_to(out, "\n");
}

void print_serial_number(fmt::memory_buffer& out, const cpu_t& cpu) {
	format_to(out, "Processor serial number\n");
	format_to(out, "\t");
	const register_set_t& regs = cpu.leaves.at(leaf_type::serial_number).at(subleaf_type::main);
	switch(cpu.vendor & any_silicon) {
	case intel:
		{
			const std::uint32_t top = cpu.leaves.at(leaf_type::version_info).at(subleaf_type::main)[eax];
			const std::uint32_t middle = regs[edx];
			const std::uint32_t bottom = regs[ecx];
			format_to(out, "Serial number: {:04x}-{:04x}-{:04x}-{:04x}-{:04x}-{:04x}\n", top    >> 16_u32, top    & 0xffff_u32,
			                                                                      middle >> 16_u32, middle & 0xffff_u32,
			                                                                      bottom >> 16_u32, bottom & 0xffff_u32);
		}
		break;
	case transmeta:
		format_to(out, "{:08x}-{:08x}-{:08x}-{:08x}\n", regs[eax], regs[ebx], regs[ecx], regs[edx]);
		break;
	default:
		print_generic(out, cpu, leaf_type::serial_number, subleaf_type::main);
		break;
	}
	format_to(out, "\n");
}

void print_mwait_parameters(fmt::memory_buffer& out, const cpu_t& cpu) {
	const register_set_t& regs = cpu.leaves.at(leaf_type::monitor_mwait).at(subleaf_type::main);

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

	format_to(out, "MONITOR/MWAIT leaf\n");
	format_to(out, "\tSmallest monitor-line size: {:d} bytes\n", (a.split.smallest_monitor_line + 0_u32));
	format_to(out, "\tLargest monitor-line size: {:d} bytes\n", (b.split.largest_monitor_line  + 0_u32));
	if(c.split.enumerable) {
		print_features(out, cpu, leaf_type::monitor_mwait, subleaf_type::main, ecx);
		if(cpu.vendor & intel) {
			const std::uint32_t mask = 0b1111;
			for(std::size_t i = 0; i < 8; ++i) {
				const std::uint32_t states = (regs[edx] & (mask << (i * 4))) >> (i * 4);
				format_to(out, "\t{:d} C{:d} sub C-states supported using MWAIT\n", states, i);
			}
		}
	}
	format_to(out, "\n");
}

void print_thermal_and_power(fmt::memory_buffer& out, const cpu_t& cpu) {
	const register_set_t& regs = cpu.leaves.at(leaf_type::thermal_and_power).at(subleaf_type::main);
	format_to(out, "Thermal and Power Management\n");
	print_features(out, cpu, leaf_type::thermal_and_power, subleaf_type::main, eax);
	format_to(out, "\n");

	if(cpu.vendor & intel) {
		const union
		{
			std::uint32_t full;
			struct
			{
				std::uint32_t interrupt_thresholds : 4;
				std::uint32_t reserved_1           : 28;
			} split;
		} b = { regs[ebx] };

		if(b.split.interrupt_thresholds) {
			format_to(out, "\t{:d} interrupt thresholds in Digital Thermal Sensor\n", b.split.interrupt_thresholds);
		}
		print_features(out, cpu, leaf_type::thermal_and_power, subleaf_type::main, ecx);
		format_to(out, "\n");
	}
}

void enumerate_extended_features(cpu_t& cpu) {
	register_set_t regs = cpuid(leaf_type::extended_features, subleaf_type::main);
	cpu.leaves[leaf_type::extended_features][subleaf_type::main] = regs;

	const subleaf_type limit = subleaf_type{ regs[eax] };
	for(subleaf_type sub = subleaf_type{ 1 }; sub < limit; ++sub) {
		cpu.leaves[leaf_type::extended_features][sub] = cpuid(leaf_type::extended_features, sub);
	}
}

void print_extended_features(fmt::memory_buffer& out, const cpu_t& cpu) {
	for(const auto& sub : cpu.leaves.at(leaf_type::extended_features)) {
		const register_set_t& regs = sub.second;
		switch(sub.first) {
		case subleaf_type::extended_state_main:
			{
				format_to(out, "Extended features\n");
				print_features(out, cpu, leaf_type::extended_features, subleaf_type::extended_features_main, ebx);
				format_to(out, "\n");
				if(cpu.vendor & intel) {
					const union
					{
						std::uint32_t full;
						struct
						{
							std::uint32_t prefetchw        : 1;
							std::uint32_t avx512_vbmi      : 1;
							std::uint32_t umip             : 1;
							std::uint32_t pku              : 1;
							std::uint32_t ospke            : 1;
							std::uint32_t reserved_1       : 1;
							std::uint32_t avx512_vbmi2     : 1;
							std::uint32_t reserved_2       : 1;
							std::uint32_t gfni             : 1;
							std::uint32_t vaes             : 1;
							std::uint32_t vpclmulqdq       : 1;
							std::uint32_t avx512_vnni      : 1;
							std::uint32_t avx512_bitalg    : 1;
							std::uint32_t reserved_3       : 1;
							std::uint32_t avx512_vpopcntdq : 1;
							std::uint32_t reserved_4       : 1;
							std::uint32_t la57             : 1;
							std::uint32_t mawau_value      : 5;
							std::uint32_t rdpid            : 1;
							std::uint32_t reserved_5       : 7;
							std::uint32_t sgx_lc           : 1;
							std::uint32_t reserved_6       : 1;

						} split;
					} c = { regs[ecx] };
					format_to(out, "\tMPX Address-width adjust: {:d}\n", c.split.mawau_value);
					format_to(out, "\n");
					print_features(out, cpu, leaf_type::extended_features, subleaf_type::extended_features_main, ecx);
					format_to(out, "\n");
				}
				print_features(out, cpu, leaf_type::extended_features, subleaf_type::extended_features_main, edx);
				format_to(out, "\n");
			}
			break;
		default:
			print_generic(out, cpu, leaf_type::extended_features, sub.first);
			format_to(out, "\n");
			break;
		}
	}
}

void print_direct_cache_access(fmt::memory_buffer& out, const cpu_t& cpu) {
	const register_set_t& regs = cpu.leaves.at(leaf_type::direct_cache_access).at(subleaf_type::main);
	format_to(out, "Direct Cache Access\n");
	format_to(out, "\tDCA CAP MSR: {:#010x}\n", regs[eax]);
	format_to(out, "\n");
}

void print_performance_monitoring(fmt::memory_buffer& out, const cpu_t& cpu) {
	const register_set_t& regs = cpu.leaves.at(leaf_type::performance_monitoring).at(subleaf_type::main);
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

	format_to(out, "Architectural Performance Monitoring\n");
	format_to(out, "\tVersion: {:d}\n", a.split.version);
	format_to(out, "\tCounters per logical processor: {:d}\n", a.split.counters_per_logical);
	format_to(out, "\tCounter bit width: {:d}\n", a.split.counter_bit_width);
	format_to(out, "\tFixed function counters: {:d}\n", d.split.fixed_function_counters);
	format_to(out, "\tFixed function counter bit width: {:d}\n", d.split.fixed_function_counter_width);
	format_to(out, "\tAnyThread: {:d}\n", d.split.any_thread);

	format_to(out, "\tSupported counters\n");
	if(0 == b.split.core_cycle) {
		format_to(out, "\t\tCore cycles\n");
	}
	if(0 == b.split.instructions_retired) {
		format_to(out, "\t\tInstructions retired\n");
	}
	if(0 == b.split.reference_cycles) {
		format_to(out, "\t\tReference cycles\n");
	}
	if(0 == b.split.llc_reference) {
		format_to(out, "\t\tLast-level cache reference\n");
	}
	if(0 == b.split.llc_misses) {
		format_to(out, "\t\tLast-level cache misses\n");
	}
	if(0 == b.split.branch_retired) {
		format_to(out, "\t\tBranch instructions retired\n");
	}
	if(0 == b.split.branch_mispredict) {
		format_to(out, "\t\tBranch instructions mispredicted\n");
	}
	format_to(out, "\n");
}

void enumerate_extended_state(cpu_t& cpu) {
	register_set_t regs = cpuid(leaf_type::extended_state, subleaf_type::extended_state_main);
	cpu.leaves[leaf_type::extended_state][subleaf_type::extended_state_main] = regs;
	const std::uint64_t valid_bits = regs[eax] | (gsl::narrow_cast<std::uint64_t>(regs[edx]) << 32ui64);

	cpu.leaves[leaf_type::extended_state][subleaf_type::extended_state_sub] = cpuid(leaf_type::extended_state, subleaf_type::extended_state_sub);

	std::uint64_t mask = 0x1ui64 << 2_u32;
	for(subleaf_type i = subleaf_type{ 2_u32 }; i < subleaf_type{ 63_u32 }; ++i, mask <<= 1ui64) {
		if(valid_bits & mask) {
			regs = cpuid(leaf_type::extended_state, i);
			if(regs[eax] != 0_u32
			|| regs[ebx] != 0_u32
			|| regs[ecx] != 0_u32
			|| regs[edx] != 0_u32) {
				cpu.leaves[leaf_type::extended_state][i] = regs;
			}
		}
	}
	if(cpu.vendor & amd) {
		regs = cpuid(leaf_type::extended_state, subleaf_type{ 0x3e });
		if(regs[ebx] != 0_u32) {
			cpu.leaves[leaf_type::extended_state][subleaf_type{ 0x3e }] = regs;
		}
	}
}

void print_extended_state(fmt::memory_buffer& out, const cpu_t& cpu) {
	for(const auto& sub : cpu.leaves.at(leaf_type::extended_state)) {
		const register_set_t& regs = sub.second;
		switch(sub.first) {
		case subleaf_type::extended_state_main:
			{
				format_to(out, "Extended states\n");
				format_to(out, "\tFeatures supported by XSAVE: \n");
				print_features(out, cpu, leaf_type::extended_state, subleaf_type::extended_state_main, eax);
				format_to(out, "\n");

				format_to(out, "\tMaximum size for all enabled features/bytes  : {:d}\n", regs[ebx]);
				format_to(out, "\tMaximum size for all supported features/bytes: {:d}\n", regs[ecx]);
				format_to(out, "\n");
			}
			break;
		case subleaf_type::extended_state_sub:
			{
				format_to(out, "\tXSAVE extended features:\n");
				print_features(out, cpu, leaf_type::extended_state, subleaf_type::extended_state_sub, eax);
				format_to(out, "\n");

				format_to(out, "\tSize for enabled features/bytes: {:d}\n", regs[ebx]);
				format_to(out, "\n");
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
				const auto& saveables = all_features.equal_range(leaf_type::extended_state).first->second.at(subleaf_type::extended_state_main).at(eax);
				const std::string& description = idx < saveables.size() ? saveables[idx].description
				                               : idx == 0xe3_u32        ? "Lightweight Profiling"
				                               :                          "(unknown)";

				format_to(out, "\tExtended state for {:s} ({:#04x}) uses {:d} bytes at offset {:#010x}\n", description, static_cast<std::uint32_t>(sub.first), regs[eax], regs[ebx]);
				if(c.split.set_in_xss) {
					format_to(out, "\t\tBit set in XSS MSR\n");
				} else {
					format_to(out, "\t\tBit set in XCR0\n");
				}
				if(c.split.aligned_to_64_bytes) {
					format_to(out, "\t\tAligned to next 64-byte boundary\n");
				} else {
					format_to(out, "\t\tImmediately follows previous component.\n");
				}
				format_to(out, "\n");
			}
			break;
		}
	}
}

void enumerate_rdt_monitoring(cpu_t& cpu) {
	register_set_t regs = cpuid(leaf_type::rdt_monitoring, subleaf_type::rdt_monitoring_main);
	cpu.leaves[leaf_type::rdt_monitoring][subleaf_type::rdt_monitoring_main] = regs;

	const std::uint32_t valid_bits = regs[edx];
	std::uint32_t mask = 0x1_u32 << 1_u32;
	for(subleaf_type i = subleaf_type::rdt_monitoring_l3; i < subleaf_type{ 32 }; ++i, mask <<= 1_u32) {
		if(valid_bits & mask) {
			cpu.leaves[leaf_type::rdt_monitoring][i] = cpuid(leaf_type::rdt_monitoring, i);
		}
	}
}

void print_rdt_monitoring(fmt::memory_buffer& out, const cpu_t& cpu) {
	static const std::vector<feature_t> monitorables = {
		{ intel , 0x0000'0001_u32, "O", "Occupancy"       },
		{ intel , 0x0000'0002_u32, "T", "Total Bandwidth" },
		{ intel , 0x0000'0004_u32, "L", "Local Bandwidth" }
	};
	for(const auto& sub : cpu.leaves.at(leaf_type::rdt_monitoring)) {
		const register_set_t& regs = sub.second;
		switch(sub.first) {
		case subleaf_type::rdt_monitoring_main:
			format_to(out, "Intel Resource Director Technology monitoring\n");
			format_to(out, "\tMaximum Resource Monitoring ID of all types: {:#010x}\n", regs[ebx]);
			format_to(out, "\n");
			break;
		case subleaf_type::rdt_monitoring_l3:
			format_to(out, "\tL3 cache monitoring\n");
			format_to(out, "\t\tConversion factor: {:d}\n", regs[ebx]);
			for(const feature_t& mon : monitorables) {
				if(regs[edx] & mon.mask) {
					format_to(out, "\t\tL3 {:s}\n", mon.description);
				}
			}
			format_to(out, "\n");
			break;
		default:
			format_to(out, "\tUnknown resource type {:#04x} monitoring\n", static_cast<std::uint32_t>(sub.first));
			format_to(out, "\t\tConversion factor: {:d}\n", regs[ebx]);
			for(const feature_t& mon : monitorables) {
				if(regs[edx] & mon.mask) {
					format_to(out, "\t\tUnknown resource {:s}\n", mon.description);
				}
			}
			format_to(out, "\n");
			break;
		}
	}
}

void enumerate_rdt_allocation(cpu_t& cpu) {
	register_set_t regs = cpuid(leaf_type::rdt_allocation, subleaf_type::rdt_allocation_main);
	cpu.leaves[leaf_type::rdt_allocation][subleaf_type::rdt_allocation_main] = regs;

	const std::uint32_t valid_bits = regs[edx];
	std::uint32_t mask = 0x1_u32 << 1_u32;
	for(subleaf_type i = subleaf_type::rdt_cat_l3; i < subleaf_type{ 32 }; ++i, mask <<= 1_u32) {
		if(valid_bits & mask) {
			cpu.leaves[leaf_type::rdt_allocation][i] = cpuid(leaf_type::rdt_allocation, i);
		}
	}
}

void print_rdt_allocation(fmt::memory_buffer& out, const cpu_t& cpu) {
	static const std::vector<feature_t> allocatables = {
		{ intel , 0x0000'0002_u32, "L3" , "L3 Cache Allocation"},
		{ intel , 0x0000'0004_u32, "L2" , "L2 Cache Allocation"},
		{ intel , 0x0000'0008_u32, "MEM", "Memory Bandwidth Allocation" }
	};

	for(const auto& sub : cpu.leaves.at(leaf_type::rdt_allocation)) {
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
		case subleaf_type::rdt_allocation_main:
			format_to(out, "Intel Resource Director Technology allocation\n");
			for(const feature_t& all : allocatables) {
				if(regs[edx] & all.mask) {
					format_to(out, "\t\tSupports {:s}\n", all.description);
				}
			}
			format_to(out, "\n");
			break;
		case subleaf_type::rdt_cat_l3:
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

				format_to(out, "\tL3 Cache Allocation Technology\n");
				format_to(out, "\tLength of capacity bitmask: {:d}\n", (a.split.bit_mask_length + 1_u32));
				format_to(out, "\tBitmap of isolation/contention: {:#010x}\n", regs[ebx]);
				if(regs[ecx] & 0x0000'0004_u32) {
					format_to(out, "\tCode and Data Prioritization supported\n");
				}
				format_to(out, "\tHighest COS number for this resource: {:d}\n", d.split.highest_cos_number);
				format_to(out, "\n");
			}
			break;
		case subleaf_type::rdt_cat_l2:
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

				format_to(out, "\tL2 Cache Allocation Technology\n");
				format_to(out, "\tLength of capacity bitmask: {:d}\n", (a.split.bit_mask_length + 1_u32));
				format_to(out, "\tBitmap of isolation/contention: {:#010x}\n", regs[ebx]);
				format_to(out, "\tHighest COS number for this resource: {:d}\n", d.split.highest_cos_number);
				format_to(out, "\n");
			}
			break;
		case subleaf_type::rdt_mba:
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

				format_to(out, "\tMemory Bandwidth Allocation\n");
				format_to(out, "\tMaximum MBA throttling value: {:d}\n", (a.split.max_throttle + 1_u32));
				if(regs[ecx] & 0x0000'0004_u32) {
					format_to(out, "\tResponse of delay values is linear\n");
				}
				format_to(out, "\tHighest COS number for this resource: {:d}\n", d.split.highest_cos_number);
				format_to(out, "\n");
			}
			break;
		default:
			format_to(out, "\tUnknown resource type {:#04x} allocation\n", static_cast<std::uint32_t>(sub.first));
			format_to(out, "\tHighest COS number for this resource: {:d}\n", d.split.highest_cos_number);
			format_to(out, "\n");
			break;
		}
	}
}

void enumerate_sgx_info(cpu_t& cpu) {
	cpu.leaves[leaf_type::sgx_info][subleaf_type::sgx_capabilities] = cpuid(leaf_type::sgx_info, subleaf_type::sgx_capabilities);
	cpu.leaves[leaf_type::sgx_info][subleaf_type::sgx_attributes  ] = cpuid(leaf_type::sgx_info, subleaf_type::sgx_attributes  );

	for(subleaf_type i = subleaf_type{ 2 }; ; ++i) {
		register_set_t regs = cpuid(leaf_type::sgx_info, i);
		if((regs[eax] & 0x0000'000f_u32) == 0_u32) {
			break;
		}
		cpu.leaves[leaf_type::sgx_info][i] = regs;
	}
}

void print_sgx_info(fmt::memory_buffer& out, const cpu_t& cpu) {
	for(const auto& sub : cpu.leaves.at(leaf_type::sgx_info)) {
		const register_set_t& regs = sub.second;

		switch(sub.first) {
		case subleaf_type::sgx_capabilities:
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

				format_to(out, "Intel SGX\n");
				format_to(out, "\tFeatures:\n");
				print_features(out, cpu, leaf_type::sgx_info, subleaf_type::sgx_capabilities, eax);
				format_to(out, "\n");
				format_to(out, "\tMISCSELECT extended features: {:#010x}\n", regs[ebx]);
				format_to(out, "\tMaximum enclave size in 32-bit mode/bytes: {:d}\n", (1ui64 << d.split.max_enclave_32_bit));
				format_to(out, "\tMaximum enclave size in 64-bit mode/bytes: {:d}\n", (1ui64 << d.split.max_enclave_64_bit));
				format_to(out, "\n");
			}
			break;
		case subleaf_type::sgx_attributes:
			{
				format_to(out, "\tSECS.ATTRIBUTES valid bits: {:08x}:{:08x}:{:08x}:{:08x}\n", regs[edx], regs[ecx], regs[ebx], regs[eax]);
				format_to(out, "\n");
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

				switch(a.split.type) {
				case 0b0000:
					break;
				case 0b0001:
					const std::uint64_t physical_address = (gsl::narrow_cast<std::uint64_t>(b.split.epc_physical_address_hi_bits ) << 32ui64)
					                                     | (gsl::narrow_cast<std::uint64_t>(a.split.epc_physical_address_low_bits) << 12ui64);
					const std::uint64_t epc_size = (gsl::narrow_cast<std::uint64_t>(d.split.epc_section_size_hi_bits ) << 32ui64)
					                             | (gsl::narrow_cast<std::uint64_t>(c.split.epc_section_size_low_bits) << 12ui64);
					format_to(out, "\tEnclave Page Cache section\n");
					format_to(out, "\t\tSection {:s} confidentiality and integrity protection\n", c.split.epc_section_properties == 0b0001_u32 ? "has" : "does not have");
					format_to(out, "\t\tEPC physical address: {:0#18x}\n", physical_address);
					format_to(out, "\t\tEPC size: {:#018x}\n"            , epc_size);
					format_to(out, "\n");
				}
			}
			break;
		}
	}
}

void enumerate_processor_trace(cpu_t& cpu) {
	register_set_t regs = cpuid(leaf_type::processor_trace, subleaf_type::main);
	cpu.leaves[leaf_type::processor_trace][subleaf_type::main] = regs;

	const subleaf_type limit = subleaf_type{ regs[eax] };
	for(subleaf_type sub = subleaf_type{ 1 }; sub < limit; ++sub) {
		cpu.leaves[leaf_type::processor_trace][sub] = cpuid(leaf_type::processor_trace, sub);
	}
}

void print_processor_trace(fmt::memory_buffer& out, const cpu_t& cpu) {
	for(const auto& sub : cpu.leaves.at(leaf_type::processor_trace)) {
		const register_set_t& regs = sub.second;
		switch(sub.first) {
		case subleaf_type::main:
			format_to(out, "Processor Trace\n");
			print_features(out, cpu, leaf_type::processor_trace, subleaf_type::main, ebx);
			format_to(out, "\n");
			print_features(out, cpu, leaf_type::processor_trace, subleaf_type::main, ecx);
			format_to(out, "\n");
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
				format_to(out, "\tNumber of configurable address ranges for filtering: {:d}\n", a.split.number_of_ranges);
				format_to(out, "\tBitmap of supported MTC period encodings: {:#010x}\n", a.split.mtc_period_bitmap);
				format_to(out, "\tBitmap of supported Cycle Treshold value encodings: {:#010x}\n", b.split.cycle_threshold_bitmap);
				format_to(out, "\tBitmap of supported Configurable PSB frequency encodings: {:#010x}\n", b.split.supported_psb_bitmap);
				format_to(out, "\n");
			}
			break;
		}
	}
}

void print_time_stamp_counter(fmt::memory_buffer& out, const cpu_t& cpu) {
	const register_set_t& regs = cpu.leaves.at(leaf_type::time_stamp_counter).at(subleaf_type::main);
	format_to(out, "Time Stamp Counter and Nominal Core Crystal Clock\n");
	format_to(out, "\tTSC:core crystal clock ratio: {:d}:{:d}\n", regs[ebx], regs[eax]);
	format_to(out, "\tNominal core crystal clock/Hz: {:d}\n", regs[ecx]);
	format_to(out, "\tTSC frequency/Hz: {:d}\n", gsl::narrow_cast<std::uint64_t>(regs[ecx]) * regs[ebx] / regs[eax]);
	format_to(out, "\n");
}

void print_processor_frequency(fmt::memory_buffer& out, const cpu_t& cpu) {
	const register_set_t& regs = cpu.leaves.at(leaf_type::processor_frequency).at(subleaf_type::main);

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

	format_to(out, "Processor frequency\n");
	format_to(out, "\tBase frequency/MHz: {:d}\n", a.split.frequency);
	format_to(out, "\tMaximum frequency/MHz: {:d}\n", b.split.frequency);
	format_to(out, "\tBus (reference) frequency/MHz: {:d}\n", c.split.frequency);
	format_to(out, "\n");
}

void enumerate_system_on_chip_vendor(cpu_t& cpu) {
	register_set_t regs = cpuid(leaf_type::system_on_chip_vendor, subleaf_type::main);
	cpu.leaves[leaf_type::system_on_chip_vendor][subleaf_type::main] = regs;

	const subleaf_type limit = subleaf_type{ regs[eax] };
	for(subleaf_type sub = subleaf_type{ 1 }; sub < limit; ++sub) {
		cpu.leaves[leaf_type::system_on_chip_vendor][sub] = cpuid(leaf_type::system_on_chip_vendor, sub);
	}
}

void print_system_on_chip_vendor(fmt::memory_buffer& out, const cpu_t& cpu) {
	for(const auto& sub : cpu.leaves.at(leaf_type::system_on_chip_vendor)) {
		const register_set_t& regs = sub.second;
		switch(sub.first) {
		case subleaf_type::main:
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

				format_to(out, "System-on-chip\n");
				format_to(out, "\tVendor ID: {:#06x}\n", b.split.vendor_id);
				if(b.split.is_industry_standard_vendor) {
					format_to(out, "\tVendor ID is assigned by an industry standard scheme\n");
				} else {
					format_to(out, "\tVendor ID is assigned by Intel\n");
				}
				format_to(out, "\tProject ID: {:#010x}\n", regs[ecx]);
				format_to(out, "\tStepping: {:#010x}\n", regs[edx]);
				format_to(out, "\n");
			}
			break;
		case subleaf_type{ 1 }:
			{
				const union
				{
					std::array<register_set_t, 3> split;
					std::array<char, 48> full;
				} brand = {
					cpu.leaves.at(leaf_type::system_on_chip_vendor).at(subleaf_type{ 1 }),
					cpu.leaves.at(leaf_type::system_on_chip_vendor).at(subleaf_type{ 2 }),
					cpu.leaves.at(leaf_type::system_on_chip_vendor).at(subleaf_type{ 3 })
				};
				format_to(out, "\tSoC brand: {}\n", brand.full);
				format_to(out, "\n");
			}
			break;
		case subleaf_type{ 2 }:
		case subleaf_type{ 3 }:
			break;
		default:
			format_to(out, "\tVendor data:\n");
			format_to(out, "\t\t{:#010x}\n", regs[eax]);
			format_to(out, "\t\t{:#010x}\n", regs[ebx]);
			format_to(out, "\t\t{:#010x}\n", regs[ecx]);
			format_to(out, "\t\t{:#010x}\n", regs[edx]);
			format_to(out, "\n");
			break;
		}
	}
}

void enumerate_pconfig(cpu_t& cpu) {
	cpu.leaves[leaf_type::pconfig][subleaf_type::main] = cpuid(leaf_type::pconfig, subleaf_type::main);

	for(subleaf_type sub = subleaf_type{ 0x1_u32 }; ; ++sub) {
		register_set_t regs = cpuid(leaf_type::pconfig, sub);
		if(0 == (regs[eax] & 0x0000'0001_u32)) {
			break;
		}
		cpu.leaves[leaf_type::pconfig][sub] = regs;
	}
}

void print_pconfig(fmt::memory_buffer& out, const cpu_t& cpu) {
	for(const auto& sub : cpu.leaves.at(leaf_type::pconfig)) {
		const register_set_t& regs = sub.second;
		switch(sub.first) {
		case subleaf_type::main:
		default:
			{
				const union
				{
					std::uint32_t full;
					struct
					{
						std::uint32_t type       : 12;
						std::uint32_t reserved_1 : 20;
					} split;
				} a = { regs[eax] };

				format_to(out, "PCONFIG information\n");
				switch(a.split.type) {
				case 0:
					format_to(out, "\tInvalid\n");
					break;
				case 1:
					format_to(out, "\tMKTIME\n");
					break;
				default:
					format_to(out, "Unknown target: {:#010x}\n", a.split.type);
					break;
				}
				format_to(out, "\n");
			}
			break;
		}
	}
}

void print_extended_limit(fmt::memory_buffer& out, const cpu_t& cpu) {
	const register_set_t& regs = cpu.leaves.at(leaf_type::extended_limit).at(subleaf_type::main);
	format_to(out, "Extended limit\n");
	format_to(out, "\tMaximum extended cpuid leaf: {:#010x}\n", regs[eax]);
	format_to(out, "\n");
}

void print_extended_signature_and_features(fmt::memory_buffer& out, const cpu_t& cpu) {
	const register_set_t& regs = cpu.leaves.at(leaf_type::extended_signature_and_features).at(subleaf_type::main);
	format_to(out, "Extended signature and features\n");
	
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
		format_to(out, "\tBrand ID: {:#06x}\n", b.split.brand_id);
		format_to(out, "\tPackage: ");
		switch(b.split.package_type) {
		case 0x0:
			format_to(out, "FP4 (BGA)/FT3 (BGA)/FT3b (BGA)\n");
			break;
		case 0x1:
			format_to(out, "AM3r2/FS1b\n");
			break;
		case 0x2:
			format_to(out, "AM4 (\u00b5PGA)\n");
			break;
		case 0x3:
			format_to(out, "G34r1/FP4\n");
			break;
		case 0x4:
			format_to(out, "FT4 (BGA)\n");
			break;
		case 0x5:
			format_to(out, "C32r1\n");
			break;
		default:
			format_to(out, "Unknown\n");
			break;
		}
		format_to(out, "\n");
	}
	format_to(out, "\tFeature identifiers\n");
	print_features(out, cpu, leaf_type::extended_signature_and_features, subleaf_type::main, ecx);
	format_to(out, "\n");
	print_features(out, cpu, leaf_type::extended_signature_and_features, subleaf_type::main, edx);
	format_to(out, "\n");
}

void print_brand_string(fmt::memory_buffer& out, const cpu_t& cpu) {
	format_to(out, "Processor brand string\n");
	const union
	{
		std::array<register_set_t, 3> split;
		std::array<char, 48> full;
	} brand = {
		cpu.leaves.at(leaf_type::brand_string_0).at(subleaf_type::main),
		cpu.leaves.at(leaf_type::brand_string_1).at(subleaf_type::main),
		cpu.leaves.at(leaf_type::brand_string_2).at(subleaf_type::main)
	};

	format_to(out, "\tBrand: {}\n", brand.full);
	format_to(out, "\n");
}

void print_ras_advanced_power_management(fmt::memory_buffer& out, const cpu_t& cpu) {
	const register_set_t& regs = cpu.leaves.at(leaf_type::ras_advanced_power_management).at(subleaf_type::main);

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

	switch(cpu.vendor & any_silicon) {
	case amd:
		format_to(out, "Processor feedback capabilities\n");
		format_to(out, "\tNumber of monitors: {:d}\n", a.split.number_of_monitors);
		format_to(out, "\tVersion: {:d}\n", a.split.version);
		format_to(out, "\tMaximum seconds between readings to avoid wraps: {:d}\n", a.split.max_wrap_time);
		format_to(out, "\n");
		format_to(out, "RAS capabilities\n");
		print_features(out, cpu, leaf_type::ras_advanced_power_management, subleaf_type::main, ebx);
		format_to(out, "\n");
		format_to(out, "Advanced Power Management information\n");
		format_to(out, "\tCompute unit power sample time period: {:d}\n", regs[ecx]);
		print_features(out, cpu, leaf_type::ras_advanced_power_management, subleaf_type::main, edx);
		break;
	case intel:
		format_to(out, "Advanced Power Management information\n");
		print_features(out, cpu, leaf_type::ras_advanced_power_management, subleaf_type::main, edx);
		break;
	default:
		format_to(out, "Advanced Power Management information\n");
		print_generic(out, cpu, leaf_type::ras_advanced_power_management, subleaf_type::main);
		break;
	}
	format_to(out, "\n");
}

void print_address_limits(fmt::memory_buffer& out, const cpu_t& cpu) {
	const register_set_t& regs = cpu.leaves.at(leaf_type::address_limits).at(subleaf_type::main);

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
			std::uint32_t reserved_2      : 16;
		} split;
	} c = { regs[ecx] };

	switch(cpu.vendor & any_silicon) {
	case amd:
		format_to(out, "Address size limits\n");
		format_to(out, "\tPhysical address size/bits: {:d}\n", a.split.physical_address_size);
		format_to(out, "\tVirtual address size/bits: {:d}\n", a.split.virtual_address_size);
		if(0 == a.split.guest_physical_size) {
			format_to(out, "\tGuest physical size matches machine physical size\n");
		}
		format_to(out, "\n");

		format_to(out, "\tExtended features\n");
		print_features(out, cpu, leaf_type::address_limits, subleaf_type::main, ebx);
		format_to(out, "\n");

		format_to(out, "\tSize identifiers\n");
		format_to(out, "\t\tThreads in package: {:d}\n", c.split.package_threads + 1_u32);
		format_to(out, "\t\t{:d} bits of APIC ID denote threads within a package\n", c.split.apic_id_size);

		if(0 != (cpu.leaves.at(leaf_type::extended_signature_and_features).at(subleaf_type::main).at(ecx) & 0x0400'0000_u32)) {
			format_to(out, "\t\tPerformance time-stamp counter size/bits: ");
			switch(c.split.perf_tsc_size) {
			case 0b00_u32:
				format_to(out, "40");
				break;
			case 0b01_u32:
				format_to(out, "48");
				break;
			case 0b10_u32:
				format_to(out, "56");
				break;
			case 0b11_u32:
				format_to(out, "64");
				break;
			}
			format_to(out, "\n");
		}
		break;
	case intel:
		format_to(out, "Address size limits\n");
		format_to(out, "\tPhysical address size/bits: {:d}\n", a.split.physical_address_size);
		format_to(out, "\tVirtual address size/bits: {:d}\n", a.split.virtual_address_size);
		format_to(out, "\n");
		format_to(out, "\tExtended features\n");
		print_features(out, cpu, leaf_type::address_limits, subleaf_type::main, ebx);
		break;
	default:
		format_to(out, "Address size limits\n");
		print_generic(out, cpu, leaf_type::address_limits, subleaf_type::main);
		break;
	}
	format_to(out, "\n");
}

void print_secure_virtual_machine(fmt::memory_buffer& out, const cpu_t& cpu) {
	const register_set_t& regs = cpu.leaves.at(leaf_type::secure_virtual_machine).at(subleaf_type::main);

	const union
	{
		std::uint32_t full;
		struct
		{
			std::uint32_t svm_revision : 8;
			std::uint32_t reserved_1   : 24;
		} split;
	} a = { regs[eax] };

	format_to(out, "Secure Virtual Machine\n");
	format_to(out, "\tSVM revision: {:d}\n", a.split.svm_revision);
	format_to(out, "\tNumber of address space identifiers: {:d}\n", regs[ebx]);
	format_to(out, "\n");
	print_features(out, cpu, leaf_type::secure_virtual_machine, subleaf_type::main, edx);
	format_to(out, "\n");
}

void print_performance_optimization(fmt::memory_buffer& out, const cpu_t& cpu) {
	format_to(out, "Performance Optimization\n");
	print_features(out, cpu, leaf_type::performance_optimization, subleaf_type::main, eax);
	format_to(out, "\n");
}

void print_instruction_based_sampling(fmt::memory_buffer& out, const cpu_t& cpu) {
	format_to(out, "Instruction Based Sampling\n");
	print_features(out, cpu, leaf_type::instruction_based_sampling, subleaf_type::main, eax);
	format_to(out, "\n");
}

void print_lightweight_profiling(fmt::memory_buffer& out, const cpu_t& cpu) {
	const register_set_t& regs = cpu.leaves.at(leaf_type::lightweight_profiling).at(subleaf_type::main);

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

	format_to(out, "Lightweight profiling\n");
	format_to(out, "\tLWP version: {:d}\n", c.split.version);
	print_features(out, cpu, leaf_type::lightweight_profiling, subleaf_type::main, eax);
	print_features(out, cpu, leaf_type::lightweight_profiling, subleaf_type::main, ecx);
	format_to(out, "\n");
	format_to(out, "\tControl block size/bytes: {:d}\n", (b.split.lwpcp_size * 4_u32));
	format_to(out, "\tEvent record size/bytes: {:d}\n", b.split.event_size);
	format_to(out, "\tMaximum EventID: {:d}\n", b.split.max_event_id);
	format_to(out, "\tOffset to first interval/bytes: {:d}\n", b.split.event_offset);
	format_to(out, "\tLatency counter size/bits: {:d}\n", c.split.latency_max);
	format_to(out, "\tLatency counter rounding: {:d}\n", c.split.latency_rounding);
	format_to(out, "\tMinimum ring buffer size/32 events: {:d}\n", c.split.minimum_buffer_size);
	format_to(out, "\n");
}

void print_encrypted_memory(fmt::memory_buffer& out, const cpu_t& cpu) {
	const register_set_t& regs = cpu.leaves.at(leaf_type::encrypted_memory).at(subleaf_type::main);

	const union
	{
		std::uint32_t full;
		struct
		{
			std::uint32_t cbit_position              : 6;
			std::uint32_t physical_address_reduction : 6;
		} split;
	} b = { regs[ebx] };

	format_to(out, "Encrypted memory\n");
	print_features(out, cpu, leaf_type::encrypted_memory, subleaf_type::main, eax);
	format_to(out, "\n");
	format_to(out, "\tC-bit position in PTE: {:d}\n", b.split.cbit_position);
	format_to(out, "\tPhysical address bit reduction: {:d}\n", b.split.physical_address_reduction);
	format_to(out, "\tNumber of simultaneous encrypted guests: {:d}\n", regs[ecx]);
	format_to(out, "\tMinimum ASID for an SEV-enabled, SEV-ES-disabled guest: {:#010x}\n", regs[edx]);
	format_to(out, "\n");
}

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

	const std::array<char, 12> vndr = bit_cast<decltype(vndr)>(
		std::array<std::uint32_t, 3> {
			regs[ebx],
			regs[edx],
			regs[ecx]
		}
	);

	format_to(out, "\tVendor identifier: {}\n", vndr);
	format_to(out, "\tVendor name: {:s}\n", to_string(get_vendor_from_name(regs)));
	format_to(out, "\n");
}

void print_version_info(fmt::memory_buffer& out, const cpu_t& cpu) {
	const register_set_t& regs = cpu.leaves.at(leaf_type::version_info).at(subleaf_type::main);
	format_to(out, "Version Information\n");

	const split_model_t a = bit_cast<decltype(a)>(regs[eax]);
	const id_info_t b = bit_cast<decltype(b)>(regs[ebx]);
	
	format_to(out, "\tsignature: {:#010x}\n", regs[eax]);
	format_to(out, "\t   family: {:#02x}\n" , cpu.model.family);
	format_to(out, "\t    model: {:#02x}\n" , cpu.model.model);
	format_to(out, "\t stepping: {:#02x}\n" , cpu.model.stepping);
	format_to(out, "\n");

	if(cpu.vendor & intel) {
		format_to(out, "\t");
		switch(a.type) {
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

		if(b.brand_id != 0_u32) {
			format_to(out, "\tbrand ID: ");
			switch(b.brand_id) {
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
	format_to(out, "\tcache line size/bytes: {:d}\n", b.cache_line_size * 8);
	if(0 != (cpu.leaves.at(leaf_type::version_info).at(subleaf_type::main).at(edx) & 0x1000'0000_u32)) {
		format_to(out, "\tlogical processors per package: {:d}\n", b.maximum_addressable_ids);
	}
	format_to(out, "\tinitial APIC ID: {:#04x}\n", b.initial_apic_id);
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

	const struct
	{
		std::uint32_t smallest_monitor_line : 16;
		std::uint32_t reserved_1            : 16;
	} a = bit_cast<decltype(a)>(regs[eax]);

	const struct
	{
		std::uint32_t largest_monitor_line : 16;
		std::uint32_t reserved_1           : 16;
	} b = bit_cast<decltype(b)>(regs[ebx]);

	const struct
	{
		std::uint32_t enumerable           : 1;
		std::uint32_t interrupts_as_breaks : 1;
		std::uint32_t reserved_1           : 30;
	} c = bit_cast<decltype(c)>(regs[ecx]);

	format_to(out, "MONITOR/MWAIT leaf\n");
	format_to(out, "\tSmallest monitor-line size: {:d} bytes\n", (a.smallest_monitor_line + 0_u32));
	format_to(out, "\tLargest monitor-line size: {:d} bytes\n", (b.largest_monitor_line  + 0_u32));
	if(c.enumerable) {
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
		const struct
		{
			std::uint32_t interrupt_thresholds : 4;
			std::uint32_t reserved_1           : 28;
		} b = bit_cast<decltype(b)>(regs[ebx]);

		if(b.interrupt_thresholds) {
			format_to(out, "\t{:d} interrupt thresholds in Digital Thermal Sensor\n", b.interrupt_thresholds);
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
					const struct
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

					} c = bit_cast<decltype(c)>(regs[ecx]);
					format_to(out, "\tMPX Address-width adjust: {:d}\n", c.mawau_value);
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
	const struct
	{
		std::uint32_t version              : 8;
		std::uint32_t counters_per_logical : 8;
		std::uint32_t counter_bit_width    : 8;
		std::uint32_t ebx_length           : 8;
	} a = bit_cast<decltype(a)>(regs[eax]);

	const struct
	{
		std::uint32_t core_cycle           : 1;
		std::uint32_t instructions_retired : 1;
		std::uint32_t reference_cycles     : 1;
		std::uint32_t llc_reference        : 1;
		std::uint32_t llc_misses           : 1;
		std::uint32_t branch_retired       : 1;
		std::uint32_t branch_mispredict    : 1;
		std::uint32_t reserved_1           : 25;
	} b = bit_cast<decltype(b)>(regs[ebx]);

	const struct
	{
		std::uint32_t fixed_function_counters      : 5;
		std::uint32_t fixed_function_counter_width : 8;
		std::uint32_t reserved_1                   : 2;
		std::uint32_t any_thread                   : 1;
		std::uint32_t reserved_2                   : 16;
	} d = bit_cast<decltype(d)>(regs[edx]);

	if(a.version == 0) {
		return;
	}

	format_to(out, "Architectural Performance Monitoring\n");
	format_to(out, "\tVersion: {:d}\n", a.version);
	format_to(out, "\tCounters per logical processor: {:d}\n", a.counters_per_logical);
	format_to(out, "\tCounter bit width: {:d}\n", a.counter_bit_width);
	format_to(out, "\tFixed function counters: {:d}\n", d.fixed_function_counters);
	format_to(out, "\tFixed function counter bit width: {:d}\n", d.fixed_function_counter_width);
	format_to(out, "\tAnyThread: {:d}\n", d.any_thread);

	format_to(out, "\tSupported counters\n");
	if(0 == b.core_cycle) {
		format_to(out, "\t\tCore cycles\n");
	}
	if(0 == b.instructions_retired) {
		format_to(out, "\t\tInstructions retired\n");
	}
	if(0 == b.reference_cycles) {
		format_to(out, "\t\tReference cycles\n");
	}
	if(0 == b.llc_reference) {
		format_to(out, "\t\tLast-level cache reference\n");
	}
	if(0 == b.llc_misses) {
		format_to(out, "\t\tLast-level cache misses\n");
	}
	if(0 == b.branch_retired) {
		format_to(out, "\t\tBranch instructions retired\n");
	}
	if(0 == b.branch_mispredict) {
		format_to(out, "\t\tBranch instructions mispredicted\n");
	}
	format_to(out, "\n");
}

void enumerate_extended_state(cpu_t& cpu) {
	register_set_t regs = cpuid(leaf_type::extended_state, subleaf_type::extended_state_main);
	cpu.leaves[leaf_type::extended_state][subleaf_type::extended_state_main] = regs;
	const std::uint64_t valid_bits = regs[eax] | (gsl::narrow_cast<std::uint64_t>(regs[edx]) << 32_u64);

	cpu.leaves[leaf_type::extended_state][subleaf_type::extended_state_sub] = cpuid(leaf_type::extended_state, subleaf_type::extended_state_sub);

	std::uint64_t mask = 0x1_u64 << 2_u32;
	for(subleaf_type i = subleaf_type{ 2_u32 }; i < subleaf_type{ 63_u32 }; ++i, mask <<= 1_u64) {
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
				const struct
				{
					std::uint32_t set_in_xss          : 1;
					std::uint32_t aligned_to_64_bytes : 1;
					std::uint32_t reserved_1          : 30;
				} c = bit_cast<decltype(c)>(regs[ecx]);

				const std::uint32_t idx = static_cast<std::uint32_t>(sub.first);
				const auto& saveables = all_features.equal_range(leaf_type::extended_state).first->second.at(subleaf_type::extended_state_main).at(eax);
				const std::string& description = idx < saveables.size() ? saveables[idx].description
				                               : idx == 0xe3_u32        ? "Lightweight Profiling"
				                               :                          "(unknown)";

				format_to(out, "\tExtended state for {:s} ({:#04x}) uses {:d} bytes at offset {:#010x}\n", description, static_cast<std::uint32_t>(sub.first), regs[eax], regs[ebx]);
				if(c.set_in_xss) {
					format_to(out, "\t\tBit set in XSS MSR\n");
				} else {
					format_to(out, "\t\tBit set in XCR0\n");
				}
				if(c.aligned_to_64_bytes) {
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
			print_features(out, cpu, leaf_type::rdt_monitoring, subleaf_type::rdt_monitoring_main, edx);
			format_to(out, "\n");
			break;
		case subleaf_type::rdt_monitoring_l3:
			format_to(out, "\tL3 cache monitoring\n");
			format_to(out, "\t\tConversion factor: {:d}\n", regs[ebx]);
			format_to(out, "\t\tMaximum Resource Monitoring ID of this type: {:#010x}\n", regs[ecx]);
			print_features(out, cpu, leaf_type::rdt_monitoring, subleaf_type::rdt_monitoring_l3, edx);
			format_to(out, "\n");
			break;
		default:
			format_to(out, "\tUnknown resource type {:#04x} monitoring\n", static_cast<std::uint32_t>(sub.first));
			format_to(out, "\t\tConversion factor: {:d}\n", regs[ebx]);
			format_to(out, "\t\tMaximum Resource Monitoring ID of this type: {:#010x}\n", regs[ecx]);
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
	//static const std::vector<feature_t> allocatables = {
	//	{ intel , 0x0000'0002_u32, "L3" , "L3 Cache Allocation"},
	//	{ intel , 0x0000'0004_u32, "L2" , "L2 Cache Allocation"},
	//	{ intel , 0x0000'0008_u32, "MEM", "Memory Bandwidth Allocation" }
	//};

	for(const auto& sub : cpu.leaves.at(leaf_type::rdt_allocation)) {
		const register_set_t& regs = sub.second;

		const struct
		{
			std::uint32_t highest_cos_number : 16;
			std::uint32_t reserved_1         : 16;
		} d = bit_cast<decltype(d)>(regs[edx]);

		switch(sub.first) {
		case subleaf_type::rdt_allocation_main:
			format_to(out, "Intel Resource Director Technology allocation\n");
			print_features(out, cpu, leaf_type::rdt_allocation, subleaf_type::rdt_allocation_main, edx);
			format_to(out, "\n");
			break;
		case subleaf_type::rdt_cat_l3:
			{
				const struct
				{
					std::uint32_t bit_mask_length : 5;
					std::uint32_t reserved_1      : 27;
				} a = bit_cast<decltype(a)>(regs[eax]);

				format_to(out, "\tL3 Cache Allocation Technology\n");
				format_to(out, "\tLength of capacity bitmask: {:d}\n", (a.bit_mask_length + 1_u32));
				format_to(out, "\tBitmap of isolation/contention: {:#010x}\n", regs[ebx]);
				if(regs[ecx] & 0x0000'0004_u32) {
					format_to(out, "\tCode and Data Prioritization Technology supported\n");
				}
				//print_features(out, cpu, leaf_type::rdt_allocation, subleaf_type::rdt_cat_l3, ecx);
				format_to(out, "\tHighest COS number for this resource: {:d}\n", d.highest_cos_number);
				format_to(out, "\n");
			}
			break;
		case subleaf_type::rdt_cat_l2:
			{
				const struct
				{
					std::uint32_t bit_mask_length : 5;
					std::uint32_t reserved_1      : 27;
				} a = bit_cast<decltype(a)>(regs[eax]);

				format_to(out, "\tL2 Cache Allocation Technology\n");
				format_to(out, "\tLength of capacity bitmask: {:d}\n", (a.bit_mask_length + 1_u32));
				format_to(out, "\tBitmap of isolation/contention: {:#010x}\n", regs[ebx]);
				if(regs[ecx] & 0x0000'0004_u32) {
					format_to(out, "\tCode and Data Prioritization Technology supported\n");
				}
				//print_features(out, cpu, leaf_type::rdt_allocation, subleaf_type::rdt_cat_l2, ecx);
				format_to(out, "\tHighest COS number for this resource: {:d}\n", d.highest_cos_number);
				format_to(out, "\n");
			}
			break;
		case subleaf_type::rdt_mba:
			{
				const struct
				{
					std::uint32_t max_throttle : 12;
					std::uint32_t reserved_1   : 20;
				} a = bit_cast<decltype(a)>(regs[eax]);

				format_to(out, "\tMemory Bandwidth Allocation\n");
				format_to(out, "\tMaximum MBA throttling value: {:d}\n", (a.max_throttle + 1_u32));
				if(regs[ecx] & 0x0000'0004_u32) {
					format_to(out, "\tResponse of delay values is linear\n");
				}
				format_to(out, "\tHighest COS number for this resource: {:d}\n", d.highest_cos_number);
				format_to(out, "\n");
			}
			break;
		default:
			format_to(out, "\tUnknown resource type {:#04x} allocation\n", static_cast<std::uint32_t>(sub.first));
			format_to(out, "\tHighest COS number for this resource: {:d}\n", d.highest_cos_number);
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
				const struct
				{
					std::uint32_t max_enclave_32_bit : 8;
					std::uint32_t max_enclave_64_bit : 8;
					std::uint32_t reserved_1         : 16;
				} d = bit_cast<decltype(d)>(regs[edx]);

				format_to(out, "Intel SGX\n");
				format_to(out, "\tFeatures:\n");
				print_features(out, cpu, leaf_type::sgx_info, subleaf_type::sgx_capabilities, eax);
				format_to(out, "\n");
				format_to(out, "\tMISCSELECT extended features: {:#010x}\n", regs[ebx]);
				format_to(out, "\tMaximum enclave size in 32-bit mode/bytes: {:d}\n", (1_u64 << d.max_enclave_32_bit));
				format_to(out, "\tMaximum enclave size in 64-bit mode/bytes: {:d}\n", (1_u64 << d.max_enclave_64_bit));
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
				const struct
				{
					std::uint32_t type                          : 4;
					std::uint32_t reserved_1                    : 8;
					std::uint32_t epc_physical_address_low_bits : 20;
				} a = bit_cast<decltype(a)>(regs[eax]);

				const struct
				{
					std::uint32_t epc_physical_address_hi_bits : 20;
					std::uint32_t reserved_1                   : 12;
				} b = bit_cast<decltype(b)>(regs[ebx]);

				const struct
				{
					std::uint32_t epc_section_properties    : 4;
					std::uint32_t reserved_1                : 8;
					std::uint32_t epc_section_size_low_bits : 20;
				} c = bit_cast<decltype(c)>(regs[ecx]);

				const struct
				{
					std::uint32_t epc_section_size_hi_bits : 20;
					std::uint32_t reserved_1               : 12;
				} d = bit_cast<decltype(d)>(regs[edx]);

				switch(a.type) {
				case 0b0000:
					//break;
				case 0b0001:
					const std::uint64_t physical_address = (gsl::narrow_cast<std::uint64_t>(b.epc_physical_address_hi_bits ) << 32_u64)
					                                     | (gsl::narrow_cast<std::uint64_t>(a.epc_physical_address_low_bits) << 12_u64);
					const std::uint64_t epc_size = (gsl::narrow_cast<std::uint64_t>(d.epc_section_size_hi_bits ) << 32_u64)
					                             | (gsl::narrow_cast<std::uint64_t>(c.epc_section_size_low_bits) << 12_u64);
					format_to(out, "\tEnclave Page Cache section\n");
					format_to(out, "\t\tSection {:s} confidentiality and integrity protection\n", c.epc_section_properties == 0b0001_u32 ? "has" : "does not have");
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
			format_to(out, "\tMaximum processor trace cpuid leaf: {:#010x}\n", regs[eax]);
			print_features(out, cpu, leaf_type::processor_trace, subleaf_type::main, ebx);
			format_to(out, "\n");
			print_features(out, cpu, leaf_type::processor_trace, subleaf_type::main, ecx);
			format_to(out, "\n");
			break;
		default:
			{
				const struct
				{
					std::uint32_t number_of_ranges  : 3;
					std::uint32_t reserved_1        : 13;
					std::uint32_t mtc_period_bitmap : 16;
				} a = bit_cast<decltype(a)>(regs[eax]);

				const struct
				{
					std::uint32_t cycle_threshold_bitmap : 16;
					std::uint32_t supported_psb_bitmap : 16;
				} b = bit_cast<decltype(b)>(regs[ebx]);
				format_to(out, "\tNumber of configurable address ranges for filtering: {:d}\n", a.number_of_ranges);
				format_to(out, "\tBitmap of supported MTC period encodings: {:#010x}\n", a.mtc_period_bitmap);
				format_to(out, "\tBitmap of supported Cycle Threshold value encodings: {:#010x}\n", b.cycle_threshold_bitmap);
				format_to(out, "\tBitmap of supported Configurable PSB frequency encodings: {:#010x}\n", b.supported_psb_bitmap);
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
	std::uint64_t crystal_frequency = regs[ecx];
	if(crystal_frequency == 0_u64) {
		switch(cpu.model.model) {
		case 0x4e: // skylake mobile
		case 0x5e: // skylake desktop
		case 0x8e: // kaby lake mobile
		case 0x9e: // kaby lake/coffee lake desktop
			crystal_frequency = 24'000'000_u64;
			break;
		case 0x5f: // denverton
			crystal_frequency = 25'000'000_u64;
			break;
		case 0x5c: // apollo lake
			crystal_frequency = 19'200'000_u64;
			break;
		}
	}
	format_to(out, "\tNominal core crystal clock/Hz: {:d}\n", crystal_frequency);
	if(regs[eax] != 0_u32) {
		format_to(out, "\tTSC frequency/Hz: {:d}\n", crystal_frequency * regs[ebx] / regs[eax]);
	}
	format_to(out, "\n");
}

void print_processor_frequency(fmt::memory_buffer& out, const cpu_t& cpu) {
	const register_set_t& regs = cpu.leaves.at(leaf_type::processor_frequency).at(subleaf_type::main);

	struct frequency_t
	{
		std::uint32_t frequency : 16;
		std::uint32_t reserved_1 : 16;
	};

	const frequency_t a = bit_cast<decltype(a)>(regs[eax]);
	const frequency_t b = bit_cast<decltype(b)>(regs[ebx]);
	const frequency_t c = bit_cast<decltype(c)>(regs[ecx]);

	format_to(out, "Processor frequency\n");
	format_to(out, "\tBase frequency/MHz: {:d}\n", a.frequency);
	format_to(out, "\tMaximum frequency/MHz: {:d}\n", b.frequency);
	format_to(out, "\tBus (reference) frequency/MHz: {:d}\n", c.frequency);
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
				const struct
				{
					std::uint32_t vendor_id                   : 16;
					std::uint32_t is_industry_standard_vendor : 1;
					std::uint32_t reserved_1                  : 15;
				} b = bit_cast<decltype(b)>(regs[ebx]);

				format_to(out, "System-on-chip\n");
				format_to(out, "\tMaximum SoC cpuid leaf: {:#010x}\n", regs[eax]);
				format_to(out, "\tVendor ID: {:#06x}\n", b.vendor_id);
				if(b.is_industry_standard_vendor) {
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
				const std::array<char, 48> brand = bit_cast<decltype(brand)>(
					std::array<register_set_t, 3> {
						cpu.leaves.at(leaf_type::system_on_chip_vendor).at(subleaf_type{ 1 }),
						cpu.leaves.at(leaf_type::system_on_chip_vendor).at(subleaf_type{ 2 }),
						cpu.leaves.at(leaf_type::system_on_chip_vendor).at(subleaf_type{ 3 })
					}
				);

				format_to(out, "\tSoC brand: {}\n", brand);
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
				const struct
				{
					std::uint32_t type       : 12;
					std::uint32_t reserved_1 : 20;
				} a = bit_cast<decltype(a)>(regs[eax]);

				format_to(out, "PCONFIG information\n");
				switch(a.type) {
				case 0:
					format_to(out, "\tInvalid\n");
					break;
				case 1:
					format_to(out, "\tMKTIME\n");
					break;
				default:
					format_to(out, "Unknown target: {:#010x}\n", a.type);
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
		const struct
		{
			std::uint32_t brand_id     : 16;
			std::uint32_t reserved_1   : 12;
			std::uint32_t package_type : 4;
		} b = bit_cast<decltype(b)>(regs[ebx]);
		format_to(out, "\tBrand ID: {:#06x}\n", b.brand_id);
		format_to(out, "\tPackage: ");
		switch(b.package_type) {
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

	const std::array<char, 48> brand = bit_cast<decltype(brand)>(
		std::array<register_set_t, 3> {
			cpu.leaves.at(leaf_type::brand_string_0).at(subleaf_type::main),
			cpu.leaves.at(leaf_type::brand_string_1).at(subleaf_type::main),
			cpu.leaves.at(leaf_type::brand_string_2).at(subleaf_type::main)
		}
	);

	format_to(out, "\tBrand: {}\n", brand);
	format_to(out, "\n");
}

void print_ras_advanced_power_management(fmt::memory_buffer& out, const cpu_t& cpu) {
	const register_set_t& regs = cpu.leaves.at(leaf_type::ras_advanced_power_management).at(subleaf_type::main);

	const struct
	{
		std::uint32_t number_of_monitors : 8;
		std::uint32_t version            : 8;
		std::uint32_t max_wrap_time      : 16;
	} a = bit_cast<decltype(a)>(regs[eax]);

	switch(cpu.vendor & any_silicon) {
	case amd:
		format_to(out, "Processor feedback capabilities\n");
		format_to(out, "\tNumber of monitors: {:d}\n", a.number_of_monitors);
		format_to(out, "\tVersion: {:d}\n", a.version);
		format_to(out, "\tMaximum seconds between readings to avoid wraps: {:d}\n", a.max_wrap_time);
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

	const struct
	{
		std::uint32_t physical_address_size : 8;
		std::uint32_t virtual_address_size  : 8;
		std::uint32_t guest_physical_size   : 8;
		std::uint32_t reserved_1            : 8;
	} a = bit_cast<decltype(a)>(regs[eax]);

	const struct
	{
		std::uint32_t package_threads : 8;
		std::uint32_t reserved_1      : 4;
		std::uint32_t apic_id_size    : 4;
		std::uint32_t perf_tsc_size   : 2;
		std::uint32_t reserved_2      : 14;
	} c = bit_cast<decltype(c)>(regs[ecx]);

	switch(cpu.vendor & any_silicon) {
	case amd:
		format_to(out, "Address size limits\n");
		format_to(out, "\tPhysical address size/bits: {:d}\n", a.physical_address_size);
		format_to(out, "\tVirtual address size/bits: {:d}\n", a.virtual_address_size);
		if(0 == a.guest_physical_size) {
			format_to(out, "\tGuest physical size matches machine physical size\n");
		}
		format_to(out, "\n");

		format_to(out, "\tExtended features\n");
		print_features(out, cpu, leaf_type::address_limits, subleaf_type::main, ebx);
		format_to(out, "\n");

		format_to(out, "\tSize identifiers\n");
		format_to(out, "\t\tThreads in package: {:d}\n", c.package_threads + 1_u32);
		format_to(out, "\t\t{:d} bits of APIC ID denote threads within a package\n", c.apic_id_size);

		if(0 != (cpu.leaves.at(leaf_type::extended_signature_and_features).at(subleaf_type::main).at(ecx) & 0x0400'0000_u32)) {
			format_to(out, "\t\tPerformance time-stamp counter size/bits: ");
			switch(c.perf_tsc_size) {
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
		format_to(out, "\tPhysical address size/bits: {:d}\n", a.physical_address_size);
		format_to(out, "\tVirtual address size/bits: {:d}\n", a.virtual_address_size);
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

	const struct
	{
		std::uint32_t svm_revision : 8;
		std::uint32_t reserved_1   : 24;
	} a = bit_cast<decltype(a)>(regs[eax]);

	format_to(out, "Secure Virtual Machine\n");
	format_to(out, "\tSVM revision: {:d}\n", a.svm_revision);
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

	const struct
	{
		std::uint32_t lwpcp_size   : 8;
		std::uint32_t event_size   : 8;
		std::uint32_t max_event_id : 8;
		std::uint32_t event_offset : 8;
	} b = bit_cast<decltype(b)>(regs[ebx]);

	const struct
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
	} c = bit_cast<decltype(c)>(regs[ecx]);

	format_to(out, "Lightweight profiling\n");
	format_to(out, "\tLWP version: {:d}\n", c.version);
	print_features(out, cpu, leaf_type::lightweight_profiling, subleaf_type::main, eax);
	print_features(out, cpu, leaf_type::lightweight_profiling, subleaf_type::main, ecx);
	format_to(out, "\n");
	format_to(out, "\tControl block size/bytes: {:d}\n", (b.lwpcp_size * 4_u32));
	format_to(out, "\tEvent record size/bytes: {:d}\n", b.event_size);
	format_to(out, "\tMaximum EventID: {:d}\n", b.max_event_id);
	format_to(out, "\tOffset to first interval/bytes: {:d}\n", b.event_offset);
	format_to(out, "\tLatency counter size/bits: {:d}\n", c.latency_max);
	format_to(out, "\tLatency counter rounding: {:d}\n", c.latency_rounding);
	format_to(out, "\tMinimum ring buffer size/32 events: {:d}\n", c.minimum_buffer_size);
	format_to(out, "\n");
}

void print_encrypted_memory(fmt::memory_buffer& out, const cpu_t& cpu) {
	const register_set_t& regs = cpu.leaves.at(leaf_type::encrypted_memory).at(subleaf_type::main);

	const struct
	{
		std::uint32_t cbit_position              : 6;
		std::uint32_t physical_address_reduction : 6;
	} b = bit_cast<decltype(b)>(regs[ebx]);

	format_to(out, "Encrypted memory\n");
	print_features(out, cpu, leaf_type::encrypted_memory, subleaf_type::main, eax);
	format_to(out, "\n");
	format_to(out, "\tC-bit position in PTE: {:d}\n", b.cbit_position);
	format_to(out, "\tPhysical address bit reduction: {:d}\n", b.physical_address_reduction);
	format_to(out, "\tNumber of simultaneous encrypted guests: {:d}\n", regs[ecx]);
	format_to(out, "\tMinimum ASID for an SEV-enabled, SEV-ES-disabled guest: {:#010x}\n", regs[edx]);
	format_to(out, "\n");
}

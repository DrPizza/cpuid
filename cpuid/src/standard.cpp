#include "stdafx.h"

#include "standard.hpp"
#include "features.hpp"

#include <map>
#include <vector>

#include <fmt/format.h>

#include "utility.hpp"

void print_basic_info(fmt::Writer& w, const cpu_t& cpu) {
	const register_set_t& regs = cpu.leaves.at(leaf_t::basic_info).at(subleaf_t::main);
	w.write("Basic Information\n");
	w.write("\tMaximum basic cpuid leaf: {:#010x}\n", regs[eax]);

	const union
	{
		std::array<std::uint32_t, 3> registers;
		std::array<char, 12> vndr;
	} data = { regs[ebx], regs[edx], regs[ecx] };

	w.write("\tVendor identifier: {}\n", data.vndr);
	w.write("\tVendor name: {:s}\n", to_string(get_vendor_from_name(regs)));
	w.write("\n");
}

void print_version_info(fmt::Writer& w, const cpu_t& cpu) {
	const register_set_t& regs = cpu.leaves.at(leaf_t::version_info).at(subleaf_t::main);
	w.write("Version Information\n");

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
	
	w.write("\tsignature: {:#010x}\n", regs[eax]);
	w.write("\t   family: {:#02x}\n" , cpu.model.family);
	w.write("\t    model: {:#02x}\n" , cpu.model.model);
	w.write("\t stepping: {:#02x}\n" , cpu.model.stepping);
	if(cpu.vendor & intel) {
		w.write("\t");
		switch(a.split.type) {
		case 0:
			w.write("Original OEM Processor\n");
			break;
		case 1:
			w.write("Intel OverDrive Processor\n");
			break;
		case 2:
			w.write("Dual processor\n");
			break;
		case 3:
			w.write("Intel reserved\n");
			break;
		}
		w.write("\n");

		if(b.split.brand_id != 0ui32) {
			w.write("\tbrand ID: ");
			switch(b.split.brand_id) {
			case 0x00: break;
			case 0x01: w.write("Intel(R) Celeron(R) processor"); break;
			case 0x02: w.write("Intel(R) Pentium(R) III processor"); break;
			case 0x03:
				if(regs[eax] == 0x000006b1ui32) {
					w.write("Intel(R) Celeron(R) processor");
				} else {
					w.write("Intel(R) Pentium(R) III Xeon(R) processor");
				}
			case 0x04: w.write("Intel(R) Pentium(R) III processor"); break;
			case 0x06: w.write("Mobile Intel(R) Pentium(R) III processor-M"); break;
			case 0x07: w.write("Mobile Intel(R) Celeron(R) processor"); break;
			case 0x08: w.write("Intel(R) Pentium(R) 4 processor"); break;
			case 0x09: w.write("Intel(R) Pentium(R) 4 processor"); break;
			case 0x0a: w.write("Intel(R) Celeron(R) processor"); break;
			case 0x0b:
				if(regs[eax] == 0x00000f13ui32) {
					w.write("Intel(R) Xeon(R) processor MP");
				} else {
					w.write("Intel(R) Xeon(R) processor");
				}
			case 0x0c: w.write("Intel(R) Xeon(R) processor MP"); break;
			case 0x0e:
				if(regs[eax] == 0x00000f13ui32) {
					w.write("Intel(R) Xeon(R) processor");
				} else {
					w.write("Mobile Intel(R) Pentium(R) 4 processor - M");
				}
			case 0x0f: w.write("Mobile Intel(R) Celeron(R) processor"); break;
			case 0x11: w.write("Mobile Genuine Intel(R) processor"); break;
			case 0x12: w.write("Intel(R) Celeron(R) M processor"); break;
			case 0x13: w.write("Mobile Intel(R) Celeron(R) processor"); break;
			case 0x14: w.write("Intel(R) Celeron(R) processor"); break;
			case 0x15: w.write("Mobile Genuine Intel(R) processor"); break;
			case 0x16: w.write("Intel(R) Pentium(R) M processor"); break;
			case 0x17: w.write("Mobile Intel(R) Celeron(R) processor"); break;
			default:
				break;
			}
			w.write("\n");
		}
	}
	w.write("\n");
	w.write("\tcache line size/bytes: {:d}\n", b.split.cache_line_size * 8);
	if(0 != (cpu.leaves.at(leaf_t::version_info).at(subleaf_t::main).at(edx) & 0x1000'0000ui32)) {
		w.write("\tlogical processors per package: {:d}\n", gsl::narrow_cast<std::uint32_t>(b.split.maximum_addressable_ids));
	}
	w.write("\tlocal APIC ID: {:#04x}\n", gsl::narrow_cast<std::uint32_t>(b.split.local_apic_id));
	w.write("\n");

	w.write("\tFeature identifiers\n");
	print_features(w, cpu, leaf_t::version_info, subleaf_t::main, edx);
	w.write("\n");
	print_features(w, cpu, leaf_t::version_info, subleaf_t::main, ecx);
	w.write("\n");
}

void print_serial_number(fmt::Writer& w, const cpu_t& cpu) {
	w.write("Processor serial number\n");
	w.write("\t");
	const register_set_t& regs = cpu.leaves.at(leaf_t::serial_number).at(subleaf_t::main);
	switch(cpu.vendor & any_silicon) {
	case intel:
		{
			const std::uint32_t top = cpu.leaves.at(leaf_t::version_info).at(subleaf_t::main)[eax];
			const std::uint32_t middle = regs[edx];
			const std::uint32_t bottom = regs[ecx];
			w.write("Serial number: {:04x}-{:04x}-{:04x}-{:04x}-{:04x}-{:04x}", top    >> 16ui32, top    & 0xffffui32,
			                                                                    middle >> 16ui32, middle & 0xffffui32,
			                                                                    bottom >> 16ui32, bottom & 0xffffui32);
		}
		break;
	case transmeta:
		w.write("{:08x}-{:08x}-{:08x}-{:08x}", regs[eax], regs[ebx], regs[ecx], regs[edx]);
		break;
	default:
		print_generic(w, cpu, leaf_t::serial_number, subleaf_t::main);
		break;
	}
	w.write("\n");
}

void print_mwait_parameters(fmt::Writer& w, const cpu_t& cpu) {
	const register_set_t& regs = cpu.leaves.at(leaf_t::monitor_mwait).at(subleaf_t::main);

	//if(regs[eax] == 0ui32 && regs[ebx] == 0ui32) {
	//	return;
	//}

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

	w.write("MONITOR/MWAIT leaf\n");
	w.write("\tSmallest monitor-line size: {:d} bytes\n", (a.split.smallest_monitor_line + 0ui32));
	w.write("\tLargest monitor-line size: {:d} bytes\n", (b.split.largest_monitor_line  + 0ui32));
	if(c.split.enumerable) {
		print_features(w, cpu, leaf_t::monitor_mwait, subleaf_t::main, ecx);
		if(cpu.vendor & intel) {
			const std::uint32_t mask = 0b1111;
			for(std::size_t i = 0; i < 8; ++i) {
				const std::uint32_t states = (regs[edx] & (mask << (i * 4))) >> (i * 4);
				w.write("\t{:d} C{:d} sub C-states supported using MWAIT\n", states, i);
			}
		}
	}
	w.write("\n");
}

void print_thermal_and_power(fmt::Writer& w, const cpu_t& cpu) {
	const register_set_t& regs = cpu.leaves.at(leaf_t::thermal_and_power).at(subleaf_t::main);
	w.write("Thermal and Power Management\n");
	print_features(w, cpu, leaf_t::thermal_and_power, subleaf_t::main, eax);
	w.write("\n");

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
			w.write("\t{:d} interrupt thresholds in Digital Thermal Sensor\n", b.split.interrupt_thresholds);
		}
		print_features(w, cpu, leaf_t::thermal_and_power, subleaf_t::main, ecx);
		w.write("\n");
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

void print_extended_features(fmt::Writer& w, const cpu_t& cpu) {
	for(const auto& sub : cpu.leaves.at(leaf_t::extended_features)) {
		const register_set_t& regs = sub.second;
		switch(sub.first) {
		case subleaf_t::extended_state_main:
			{
				w.write("Extended features\n");
				print_features(w, cpu, leaf_t::extended_features, subleaf_t::extended_features_main, ebx);
				w.write("\n");
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
					w.write("\tMPX Address-width adjust: {:d}\n", c.split.mawau_value);
					w.write("\n");
					print_features(w, cpu, leaf_t::extended_features, subleaf_t::extended_features_main, ecx);
					w.write("\n");
				}
				print_features(w, cpu, leaf_t::extended_features, subleaf_t::extended_features_main, edx);
				w.write("\n");
			}
			break;
		default:
			print_generic(w, cpu, leaf_t::extended_features, sub.first);
			w.write("\n");
			break;
		}
	}
}

void print_direct_cache_access(fmt::Writer& w, const cpu_t& cpu) {
	const register_set_t& regs = cpu.leaves.at(leaf_t::direct_cache_access).at(subleaf_t::main);
	w.write("Direct Cache Access\n");
	w.write("\tDCA CAP MSR: {:#010x}\n", regs[eax]);
	w.write("\n");
}

void print_performance_monitoring(fmt::Writer& w, const cpu_t& cpu) {
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

	w.write("Architectural Performance Monitoring\n");
	w.write("\tVersion: {:d}\n", a.split.version);
	w.write("\tCounters per logical processor: {:d}\n", a.split.counters_per_logical);
	w.write("\tCounter bit width: {:d}\n", a.split.counter_bit_width);
	w.write("\tFixed function counters: {:d}\n", d.split.fixed_function_counters);
	w.write("\tFixed function counter bit width: {:d}\n", d.split.fixed_function_counter_width);
	w.write("\tAnyThread: {:d}\n", d.split.any_thread);

	w.write("\tSupported counters\n");
	if(0 == b.split.core_cycle) {
		w.write("\t\tCore cycles\n");
	}
	if(0 == b.split.instructions_retired) {
		w.write("\t\tInstructions retired\n");
	}
	if(0 == b.split.reference_cycles) {
		w.write("\t\tReference cycles\n");
	}
	if(0 == b.split.llc_reference) {
		w.write("\t\tLast-level cache reference\n");
	}
	if(0 == b.split.llc_misses) {
		w.write("\t\tLast-level cache misses\n");
	}
	if(0 == b.split.branch_retired) {
		w.write("\t\tBranch instructions retired\n");
	}
	if(0 == b.split.branch_mispredict) {
		w.write("\t\tBranch instructions mispredicted\n");
	}
	w.write("\n");
}

void enumerate_extended_state(cpu_t& cpu) {
	register_set_t regs = { 0 };
	cpuid(regs, leaf_t::extended_state, subleaf_t::extended_state_main);
	cpu.leaves[leaf_t::extended_state][subleaf_t::extended_state_main] = regs;
	const std::uint64_t valid_bits = regs[eax] | (gsl::narrow_cast<std::uint64_t>(regs[edx]) << 32ui64);

	cpuid(regs, leaf_t::extended_state, subleaf_t::extended_state_sub);
	cpu.leaves[leaf_t::extended_state][subleaf_t::extended_state_sub] = regs;

	std::uint64_t mask = 0x1ui64 << 2ui32;
	for(subleaf_t i = subleaf_t{ 2ui32 }; i < subleaf_t{ 63ui32 }; ++i, mask <<= 1ui64) {
		if(valid_bits & mask) {
			cpuid(regs, leaf_t::extended_state, i);
			if(regs[eax] != 0ui32
			|| regs[ebx] != 0ui32
			|| regs[ecx] != 0ui32
			|| regs[edx] != 0ui32) {
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

void print_extended_state(fmt::Writer& w, const cpu_t& cpu) {
	for(const auto& sub : cpu.leaves.at(leaf_t::extended_state)) {
		const register_set_t& regs = sub.second;
		switch(sub.first) {
		case subleaf_t::extended_state_main:
			{
				w.write("Extended states\n");
				w.write("\tFeatures supported by XSAVE: \n");
				print_features(w, cpu, leaf_t::extended_state, subleaf_t::extended_state_main, eax);
				w.write("\n");

				w.write("\tMaximum size for all enabled features/bytes  : {:d}\n", regs[ebx]);
				w.write("\tMaximum size for all supported features/bytes: {:d}\n", regs[ecx]);
				w.write("\n");
			}
			break;
		case subleaf_t::extended_state_sub:
			{
				w.write("\tXSAVE extended features:\n");
				print_features(w, cpu, leaf_t::extended_state, subleaf_t::extended_state_sub, eax);
				w.write("\n");

				w.write("\tSize for enabled features/bytes: {:d}\n", regs[ebx]);
				w.write("\n");
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
				const auto& saveables = all_features.equal_range(leaf_t::extended_state).first->second.at(subleaf_t::extended_state_main).at(eax);
				const std::string& description = idx < saveables.size() ? saveables[idx].description
				                               : idx == 0xe3ui32        ? "Lightweight Profiling"
				                               :                          "(unknown)";

				w.write("\tExtended state for {:s} ({:#04x}) uses {:d} bytes at offset {:#010x}\n", description, static_cast<std::uint32_t>(sub.first), regs[eax], regs[ebx]);
				if(c.split.set_in_xss) {
					w.write("\t\tBit set in XSS MSR\n");
				} else {
					w.write("\t\tBit set in XCR0\n");
				}
				if(c.split.aligned_to_64_bytes) {
					w.write("\t\tAligned to next 64-byte boundary\n");
				} else {
					w.write("\t\tImmediately follows previous component.\n");
				}
				w.write("\n");
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

void print_rdt_monitoring(fmt::Writer& w, const cpu_t& cpu) {
	static const std::vector<feature_t> monitorables = {
		{ intel , 0x0000'0001ui32, "O", "Occupancy"       },
		{ intel , 0x0000'0002ui32, "T", "Total Bandwidth" },
		{ intel , 0x0000'0004ui32, "L", "Local Bandwidth" }
	};
	for(const auto& sub : cpu.leaves.at(leaf_t::rdt_monitoring)) {
		const register_set_t& regs = sub.second;
		switch(sub.first) {
		case subleaf_t::rdt_monitoring_main:
			w.write("Intel Resource Director Technology monitoring\n");
			w.write("\tMaximum Resource Monitoring ID of all types: {:#010x}\n", regs[ebx]);
			w.write("\n");
			break;
		case subleaf_t::rdt_monitoring_l3:
			w.write("\tL3 cache monitoring\n");
			w.write("\t\tConversion factor: {:d}\n", regs[ebx]);
			for(const feature_t& mon : monitorables) {
				if(regs[edx] & mon.mask) {
					w.write("\t\tL3 {:s}\n", mon.description);
				}
			}
			w.write("\n");
			break;
		default:
			w.write("\tUnknown resource type {:#04x} monitoring\n", static_cast<std::uint32_t>(sub.first));
			w.write("\t\tConversion factor: {:d}\n", regs[ebx]);
			for(const feature_t& mon : monitorables) {
				if(regs[edx] & mon.mask) {
					w.write("\t\tUnknown resource {:s}\n", mon.description);
				}
			}
			w.write("\n");
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

void print_rdt_allocation(fmt::Writer& w, const cpu_t& cpu) {
	static const std::vector<feature_t> allocatables = {
		{ intel , 0x0000'0002ui32, "L3" , "L3 Cache Allocation"},
		{ intel , 0x0000'0004ui32, "L2" , "L2 Cache Allocation"},
		{ intel , 0x0000'0008ui32, "MEM", "Memory Bandwidth Allocation" }
	};

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
			w.write("Intel Resource Director Technology allocation\n");
			for(const feature_t& all : allocatables) {
				if(regs[edx] & all.mask) {
					w.write("\t\tSupports {:s}\n", all.description);
				}
			}
			w.write("\n");
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

				w.write("\tL3 Cache Allocation Technology\n");
				w.write("\tLength of capacity bitmask: {:d}\n", (a.split.bit_mask_length + 1ui32));
				w.write("\tBitmap of isolation/contention: {:#010x\n}", regs[ebx]);
				if(regs[ecx] & 0x0000'0004ui32) {
					w.write("\tCode and Data Prioritization supported\n");
				}
				w.write("\tHighest COS number for this resource: {:d}\n", d.split.highest_cos_number);
				w.write("\n");
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

				w.write("\tL2 Cache Allocation Technology\n");
				w.write("\tLength of capacity bitmask: {:d}\n", (a.split.bit_mask_length + 1ui32));
				w.write("\tBitmap of isolation/contention: {:#010x}\n", regs[ebx]);
				w.write("\tHighest COS number for this resource: {:d}\n", d.split.highest_cos_number);
				w.write("\n");
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

				w.write("\tMemory Bandwidth Allocation\n");
				w.write("\tMaximum MBA throttling value: {:d}\n", (a.split.max_throttle + 1ui32));
				if(regs[ecx] & 0x0000'0004ui32) {
					w.write("\tResponse of delay values is linear\n");
				}
				w.write("\tHighest COS number for this resource: {:d}\n", d.split.highest_cos_number);
				w.write("\n");
			}
			break;
		default:
			w.write("\tUnknown resource type {:#04x} allocation\n", static_cast<std::uint32_t>(sub.first));
			w.write("\tHighest COS number for this resource: {:d}\n", d.split.highest_cos_number);
			w.write("\n");
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

void print_sgx_info(fmt::Writer& w, const cpu_t& cpu) {
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

				w.write("Intel SGX\n");
				w.write("\tFeatures:\n");
				for(const feature_t& f : sgx_features) {
					if(regs[eax] & f.mask) {
						w.write("\t\t{:s}\n", f.description);
					}
				}
				w.write("\n");
				w.write("\tMISCSELECT extended features: {:#010x}\n", regs[ebx]);
				w.write("\tMaximum enclave size in 32-bit mode/bytes: {:d}\n", (2ui64 << d.split.max_enclave_32_bit));
				w.write("\tMaximum enclave size in 64-bit mode/bytes: {:d}\n", (2ui64 << d.split.max_enclave_64_bit));
				w.write("\n");
			}
			break;
		case subleaf_t::sgx_attributes:
			{
				w.write("\tSECS.ATTRIBUTES valid bits: {:08x}:{:08x}:{:08x}:{:08x}", regs[edx], regs[ecx], regs[ebx], regs[eax]);
				w.write("\n");
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


				const std::uint64_t physical_address = (gsl::narrow_cast<std::uint64_t>(b.split.epc_physical_address_hi_bits ) << 32ui64)
				                                     | (gsl::narrow_cast<std::uint64_t>(a.split.epc_physical_address_low_bits) << 12ui64);
				const std::uint64_t epc_size = (gsl::narrow_cast<std::uint64_t>(d.split.epc_section_size_hi_bits ) << 32ui64)
				                             | (gsl::narrow_cast<std::uint64_t>(c.split.epc_section_size_low_bits) << 12ui64);
				w.write("\tEnclave Page Cache section\n");
				w.write("\t\tEPC physical address: {:0#18x}\n", physical_address);
				w.write("\t\tEPC size: {:#018x}\n"            , epc_size);
				w.write("\n");
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

void print_processor_trace(fmt::Writer& w, const cpu_t& cpu) {
	for(const auto& sub : cpu.leaves.at(leaf_t::processor_trace)) {
		const register_set_t& regs = sub.second;
		switch(sub.first) {
		case subleaf_t::main:
			w.write("Processor Trace\n");
			print_features(w, cpu, leaf_t::processor_trace, subleaf_t::main, ebx);
			w.write("\n");
			print_features(w, cpu, leaf_t::processor_trace, subleaf_t::main, ecx);
			w.write("\n");
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
				w.write("\tNumber of configurable address ranges for filtering: {:d}\n", a.split.number_of_ranges);
				w.write("\tBitmap of supported MTC period encodings: {:#010x}\n", a.split.mtc_period_bitmap);
				w.write("\tBitmap of supported Cycle Treshold value encodings: {:#010x}\n", b.split.cycle_threshold_bitmap);
				w.write("\tBitmap of supported Configurable PSB frequency encodings: {:#010x}\n", b.split.supported_psb_bitmap);
				w.write("\n");
			}
			break;
		}
	}
}

void print_time_stamp_counter(fmt::Writer& w, const cpu_t& cpu) {
	const register_set_t& regs = cpu.leaves.at(leaf_t::time_stamp_counter).at(subleaf_t::main);
	w.write("Time Stamp Counter and Nominal Core Crystal Clock\n");
	w.write("\tTSC:core crystal clock ratio: {:d}:{:d}\n", regs[ebx], regs[eax]);
	w.write("\tNominal core crystal clock/Hz: {:d}\n", regs[ecx]);
	w.write("\tTSC frequency/Hz: {:d}\n", gsl::narrow_cast<std::uint64_t>(regs[ecx] * regs[ebx]) / regs[eax]);
	w.write("\n");
}

void print_processor_frequency(fmt::Writer& w, const cpu_t& cpu) {
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

	w.write("Processor frequency\n");
	w.write("\tBase frequency/MHz: {:d}\n", a.split.frequency);
	w.write("\tMaximum frequency/MHz: {:d}\n", b.split.frequency);
	w.write("\tBus (reference) frequency/MHz: {:d}\n", c.split.frequency);
	w.write("\n");
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

void print_system_on_chip_vendor(fmt::Writer& w, const cpu_t& cpu) {
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

				w.write("System-on-chip\n");
				w.write("\tVendor ID: {:#06x}\n", b.split.vendor_id);
				if(b.split.is_industry_standard_vendor) {
					w.write("\tVendor ID is assigned by an industry standard scheme\n");
				} else {
					w.write("\tVendor ID is assigned by Intel\n");
				}
				w.write("\tProject ID: {:#010x}\n", regs[ecx]);
				w.write("\tStepping: {:#010x}\n", regs[edx]);
				w.write("\n");
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
				w.write("\tSoC brand: {}\n", brand.full);
				w.write("\n");
			}
			break;
		case subleaf_t{ 2 }:
		case subleaf_t{ 3 }:
			break;
		default:
			w.write("\tVendor data:\n");
			w.write("\t\t{:#010x}\n", regs[eax]);
			w.write("\t\t{:#010x}\n", regs[ebx]);
			w.write("\t\t{:#010x}\n", regs[ecx]);
			w.write("\t\t{:#010x}\n", regs[edx]);
			w.write("\n");
			break;
		}
	}
}

void enumerate_pconfig(cpu_t& cpu) {
	register_set_t regs = { 0 };
	cpuid(regs, leaf_t::pconfig, subleaf_t::main);
	cpu.leaves[leaf_t::pconfig][subleaf_t::main] = regs;

	const subleaf_t sub = subleaf_t{ 0x1ui32 };
	for(;;) {
		cpuid(regs, leaf_t::pconfig, sub);
		if(0 == (regs[eax] & 0x0000'0001ui32)) {
			break;
		}
		cpu.leaves[leaf_t::pconfig][sub] = regs;
	}
}

void print_pconfig(fmt::Writer& w, const cpu_t& cpu) {
	for(const auto& sub : cpu.leaves.at(leaf_t::pconfig)) {
		const register_set_t& regs = sub.second;
		switch(sub.first) {
		case subleaf_t::main:
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

				w.write("PCONFIG information\n");
				switch(a.split.type) {
				case 0:
					w.write("\tInvalid\n");
					break;
				case 1:
					w.write("\tMKTIME\n");
					break;
				default:
					w.write("Unknown target: {:#010x}\n", a.split.type);
					break;
				}
				w.write("\n");
			}
			break;
		default:
			break;
		}
	}
}

void print_extended_limit(fmt::Writer& w, const cpu_t& cpu) {
	const register_set_t& regs = cpu.leaves.at(leaf_t::extended_limit).at(subleaf_t::main);
	w.write("Extended limit\n");
	w.write("\tMaximum extended cpuid leaf: {:#010x}\n", regs[eax]);
	w.write("\n");
}

void print_extended_signature_and_features(fmt::Writer& w, const cpu_t& cpu) {
	const register_set_t& regs = cpu.leaves.at(leaf_t::extended_signature_and_features).at(subleaf_t::main);
	w.write("Extended signature and features\n");
	
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
		w.write("\tBrand ID: {:#06x}\n", b.split.brand_id);
		w.write("\tPackage: ");
		switch(b.split.package_type) {
		case 0x0:
			w.write("FP4 (BGA)/FT3 (BGA)/FT3b (BGA)\n");
			break;
		case 0x1:
			w.write("AM3r2/FS1b\n");
			break;
		case 0x2:
			w.write("AM4 (\u00b5PGA)\n");
			break;
		case 0x3:
			w.write("G34r1/FP4\n");
			break;
		case 0x4:
			w.write("FT4 (BGA)\n");
			break;
		case 0x5:
			w.write("C32r1\n");
			break;
		default:
			w.write("Unknown\n");
			break;
		}
		w.write("\n");
	}
	w.write("\tFeature identifiers\n");
	print_features(w, cpu, leaf_t::extended_signature_and_features, subleaf_t::main, ecx);
	w.write("\n");
	print_features(w, cpu, leaf_t::extended_signature_and_features, subleaf_t::main, edx);
	w.write("\n");
}

void print_brand_string(fmt::Writer& w, const cpu_t& cpu) {
	w.write("Processor brand string\n");
	const union
	{
		std::array<register_set_t, 3> split;
		std::array<char, 48> full;
	} brand = {
		cpu.leaves.at(leaf_t::brand_string_0).at(subleaf_t::main),
		cpu.leaves.at(leaf_t::brand_string_1).at(subleaf_t::main),
		cpu.leaves.at(leaf_t::brand_string_2).at(subleaf_t::main)
	};

	w.write("\tBrand: {}\n", brand.full);
	w.write("\n");
}

void print_ras_advanced_power_management(fmt::Writer& w, const cpu_t& cpu) {
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

	switch(cpu.vendor & any_silicon) {
	case amd:
		w.write("Processor feedback capabilities\n");
		w.write("\tNumber of monitors: {:d}\n", a.split.number_of_monitors);
		w.write("\tVersion: {:d}\n", a.split.version);
		w.write("\tMaximum seconds between readings to avoid wraps: {:d}\n", a.split.max_wrap_time);
		w.write("\n");
		w.write("RAS capabilities\n");
		print_features(w, cpu, leaf_t::ras_advanced_power_management, subleaf_t::main, ebx);
		w.write("\n");
		w.write("Advanced Power Management information\n");
		w.write("\tCompute unit power sample time period: {:d}\n", regs[ecx]);
		print_features(w, cpu, leaf_t::ras_advanced_power_management, subleaf_t::main, edx);
		w.write("\n");
		break;
	case intel:
		w.write("Advanced Power Management information\n");
		print_features(w, cpu, leaf_t::ras_advanced_power_management, subleaf_t::main, edx);
		w.write("\n");
		break;
	default:
		w.write("Advanced Power Management information\n");
		print_generic(w, cpu, leaf_t::ras_advanced_power_management, subleaf_t::main);
		w.write("\n");
		break;
	}
	w.write("\n");
}

void print_address_limits(fmt::Writer& w, const cpu_t& cpu) {
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

	switch(cpu.vendor & any_silicon) {
	case amd:
		w.write("Address size limits\n");
		w.write("\tPhysical address size/bits: {:d}\n", a.split.physical_address_size);
		w.write("\tVirtual address size/bits: {:d}\n", a.split.virtual_address_size);
		if(0 == a.split.guest_physical_size) {
			w.write("\tGuest physical size matches machine physical size\n");
		}
		w.write("\n");

		w.write("\tExtended features\n");
		print_features(w, cpu, leaf_t::address_limits, subleaf_t::main, ebx);
		w.write("\n");


		w.write("\tSize identifiers\n");
		w.write("\t\tThreads in package: {:d}\n", c.split.package_threads + 1ui32);
		w.write("\t\t{:d} bits of APIC ID denote threads within a package\n", c.split.apic_id_size);

		if(0 != (cpu.leaves.at(leaf_t::extended_signature_and_features).at(subleaf_t::main).at(ecx) & 0x0400'0000ui32)) {
			w.write("\t\tPerforamnce time-stamp counter size/bits: ");
			switch(c.split.perf_tsc_size) {
			case 0b00ui32:
				w.write("40");
				break;
			case 0b01ui32:
				w.write("48");
				break;
			case 0b10ui32:
				w.write("56");
				break;
			case 0b11ui32:
				w.write("64");
				break;
			}
			w.write("\n");
		}
		w.write("\n");
		break;
	case intel:
		w.write("Address size limits\n");
		w.write("\tPhysical address size/bits: {:d}\n", a.split.physical_address_size);
		w.write("\tVirtual address size/bits: {:d}\n", a.split.virtual_address_size);
		w.write("\n");
		w.write("\tExtended features\n");
		print_features(w, cpu, leaf_t::address_limits, subleaf_t::main, ebx);
		w.write("\n");
		break;
	default:
		w.write("Address size limits\n");
		print_generic(w, cpu, leaf_t::address_limits, subleaf_t::main);
		w.write("\n");
		break;
	}
	w.write("\n");
}

void print_secure_virtual_machine(fmt::Writer& w, const cpu_t& cpu) {
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

	w.write("Secure Virtual Machine\n");
	w.write("\tSVM revision: {:d}\n", a.split.svm_revision);
	w.write("\tNumber of address space identifiers: {:d}\n", regs[ebx]);
	w.write("\n");
	print_features(w, cpu, leaf_t::secure_virtual_machine, subleaf_t::main, edx);
	w.write("\n");
}

void print_performance_optimization(fmt::Writer& w, const cpu_t& cpu) {
	w.write("Performance Optimization\n");
	print_features(w, cpu, leaf_t::performance_optimization, subleaf_t::main, eax);
	w.write("\n");
}

void print_instruction_based_sampling(fmt::Writer& w, const cpu_t& cpu) {
	w.write("Instruction Based Sampling\n");
	print_features(w, cpu, leaf_t::instruction_based_sampling, subleaf_t::main, eax);
	w.write("\n");
}

void print_lightweight_profiling(fmt::Writer& w, const cpu_t& cpu) {
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

	w.write("Lightweight profiling\n");
	w.write("\tLWP version: {:d}\n", c.split.version);
	print_features(w, cpu, leaf_t::lightweight_profiling, subleaf_t::main, eax);
	print_features(w, cpu, leaf_t::lightweight_profiling, subleaf_t::main, ecx);
	w.write("\n");
	w.write("\tControl block size/bytes: {:d}\n", (b.split.lwpcp_size * 4ui32));
	w.write("\tEvent record size/bytes: {:d}\n", b.split.event_size);
	w.write("\tMaximum EventID: {:d}\n", b.split.max_event_id);
	w.write("\tOffset to first interval/bytes: {:d}\n", b.split.event_offset);
	w.write("\tLatency counter size/bits: {:d}\n", c.split.latency_max);
	w.write("\tLatency counter rounding: {:d}\n", c.split.latency_rounding);
	w.write("\tMinimum ring buffer size/32 events: {:d}\n", c.split.minimum_buffer_size);
	w.write("\n");
}

void print_encrypted_memory(fmt::Writer& w, const cpu_t& cpu) {
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

	w.write("Encrypted memory\n");
	print_features(w, cpu, leaf_t::encrypted_memory, subleaf_t::main, eax);
	w.write("\n");
	w.write("\tC-bit position in PTE: {:d}\n", b.split.cbit_position);
	w.write("\tPhysical address bit reduction: {:d}\n", b.split.physical_address_reduction);
	w.write("\tNumber of simultaneous encrypted guests: {:d}\n", regs[ecx]);
	w.write("\tMinimum ASID for an SEV-enabled, SEV-ES-disabled gust: {:#010x}\n", regs[edx]);
	w.write("\n");
}

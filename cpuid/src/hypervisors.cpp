#include "stdafx.h"

#include "hypervisors.hpp"
#include "features.hpp"

#include <iostream>
#include <iomanip>
#include <array>

#include <fmt/format.h>

void print_hypervisor_limit(const cpu_t& cpu) {
	const register_set_t& regs = cpu.leaves.at(leaf_t::hypervisor_limit).at(subleaf_t::main);
	std::cout << "Hypervisor present\n";
	std::cout << "\tMaximum hypervisor leaf: 0x" << std::setw(2) << std::setfill('0') << std::hex << regs[eax] << "\n";

	const union
	{
		std::array<std::uint32_t, 3> registers;
		std::array<char, 12> vndr;
	} data = { regs[ebx], regs[ecx], regs[edx] };

	std::cout << "\tVendor: ";
	std::cout.write(data.vndr.data(), gsl::narrow_cast<std::streamsize>(data.vndr.size()));
	std::cout << "\n";
	std::cout << "\tVendor name: " << to_string(cpu.vendor) << std::endl;
	std::cout << std::endl;
}

void print_hyper_v_signature(const cpu_t& cpu) {
	using namespace fmt::literals;
	const register_set_t& regs = cpu.leaves.at(leaf_t::hyper_v_signature).at(subleaf_t::main);

	std::cout << "Hyper-V signature\n";
	const union
	{
		std::uint32_t full;
		struct
		{
			std::uint32_t signature_1 : 8;
			std::uint32_t signature_2 : 8;
			std::uint32_t signature_3 : 8;
			std::uint32_t signature_4 : 8;
		} split;
	} a = { regs[eax] };

	std::cout << "\tSignature: {:c}{:c}{:c}{:c}\n"_format(gsl::narrow_cast<char>(a.split.signature_1),
	                                                      gsl::narrow_cast<char>(a.split.signature_2),
	                                                      gsl::narrow_cast<char>(a.split.signature_3),
	                                                      gsl::narrow_cast<char>(a.split.signature_4));
	std::cout << std::endl;
}

void print_hyper_v_system_identity(const cpu_t& cpu) {
	using namespace fmt::literals;

	const register_set_t& regs = cpu.leaves.at(leaf_t::hyper_v_system_identity).at(subleaf_t::main);
	std::cout << "Hyper-V system identiy\n";

	const union
	{
		std::uint32_t full;
		struct
		{
			std::uint32_t minor_version : 16;
			std::uint32_t major_version : 16;
		} split;
	} b = { regs[ebx] };

	const union
	{
		std::uint32_t full;
		struct
		{
			std::uint32_t service_number : 24;
			std::uint32_t service_branch : 8;
		} split;
	} d = { regs[edx] };

	std::cout << "\tBuild number: " << std::dec << regs[eax] << "\n";
	std::cout << "\tVersion : {:d}.{:d}\n"_format(b.split.major_version, b.split.minor_version);
	std::cout << "\tService Pack: " << std::dec << regs[ecx] << "\n";
	std::cout << "\tService number: " << std::dec << d.split.service_number << "\n";
	std::cout << "\tService branch: " << std::dec << d.split.service_branch << "\n";
	std::cout << std::endl;
}

void print_hyper_v_features(const cpu_t& cpu) {
	using namespace fmt::literals;
	const register_set_t& regs = cpu.leaves.at(leaf_t::hyper_v_features).at(subleaf_t::main);
	std::cout << "Hyper-V features\n";
	std::cout << "\tPartition privilege mask: {:08x}:{:08x}\n"_format(regs[ebx], regs[eax]);
	print_features(cpu, leaf_t::hyper_v_features, subleaf_t::main, edx);
	std::cout << std::endl;
}

void print_hyper_v_enlightenment_recs(const cpu_t& cpu) {
	using namespace fmt::literals;
	const register_set_t& regs = cpu.leaves.at(leaf_t::hyper_v_enlightenment_recs).at(subleaf_t::main);
	std::cout << "Hyper-V recommendations\n";
	print_features(cpu, leaf_t::hyper_v_enlightenment_recs, subleaf_t::main, eax);
	std::cout << "\n";
	if(regs[ebx] != 0xffff'ffffui32) {
		std::cout << "\tTimes to retry spinlocks before notifying hypervisor: {:d}\n"_format(regs[ebx]);
	} else {
		std::cout << "\tNever retry spinlocks before notifying hypervisor\n";
	}
	std::cout << std::endl;
}

void print_hyper_v_implementation_limits(const cpu_t& cpu) {
	using namespace fmt::literals;
	const register_set_t& regs = cpu.leaves.at(leaf_t::hyper_v_implementation_limits).at(subleaf_t::main);
	std::cout << "Hyper-V implementation limits\n";
	std::cout << "\tMaximum virtual processors supported: " << std::dec << regs[eax] << "\n";
	std::cout << "\tMaximum logical processors supported: " << std::dec << regs[ebx] << "\n";
	std::cout << "\tMaximum physical interrupt vectors supported: " << std::dec << regs[ecx] << "\n";
	std::cout << std::endl;
}

void print_hyper_v_implementation_hardware(const cpu_t& cpu) {
	using namespace fmt::literals;
	std::cout << "Hyper-V implementation hardware\n";
	print_features(cpu, leaf_t::hyper_v_implementation_hardware, subleaf_t::main, eax);
	std::cout << std::endl;
}

void print_hyper_v_root_cpu_management(const cpu_t& cpu) {
	using namespace fmt::literals;
	std::cout << "Hyper-V root CPU management\n";
	print_features(cpu, leaf_t::hyper_v_root_cpu_management, subleaf_t::main, eax);
	print_features(cpu, leaf_t::hyper_v_root_cpu_management, subleaf_t::main, ebx);
	std::cout << std::endl;
}

void print_hyper_v_shared_virtual_memory(const cpu_t& cpu) {
	using namespace fmt::literals;
	const register_set_t& regs = cpu.leaves.at(leaf_t::hyper_v_shared_virtual_memory).at(subleaf_t::main);

	const union
	{
		std::uint32_t full;
		struct
		{ 
			std::uint32_t svm_supported : 1;
			std::uint32_t reserved_1    : 10;
			std::uint32_t max_pasid_space_count : 21;
		} split;
	} a = { regs[eax] };

	std::cout << "Hyper-V shared virtual memory features\n";
	print_features(cpu, leaf_t::hyper_v_shared_virtual_memory, subleaf_t::main, eax);
	std::cout << std::endl;
	std::cout << "\tMaximum PASID space count: " << std::dec << a.split.max_pasid_space_count << "\n";
	std::cout << std::endl;
}

void print_hyper_v_nested_hypervisor(const cpu_t& cpu) {
	std::cout << "Nested hypervisor features\n";
	print_features(cpu, leaf_t::hyper_v_nested_hypervisor, subleaf_t::main, eax);
	print_features(cpu, leaf_t::hyper_v_nested_hypervisor, subleaf_t::main, edx);
	std::cout << std::endl;
}

void print_hyper_v_nested_features(const cpu_t& cpu) {
	using namespace fmt::literals;
	const register_set_t& regs = cpu.leaves.at(leaf_t::hyper_v_nested_features).at(subleaf_t::main);

	const union
	{
		std::uint32_t full;
		struct
		{
			std::uint32_t low_version            : 8;
			std::uint32_t high_version           : 8;
			std::uint32_t reserved_1             : 1;
			std::uint32_t direct_flush           : 1;
			std::uint32_t flush_guest_physical   : 1;
			std::uint32_t enlightened_msr_bitmap : 1;
			std::uint32_t reserved_2             : 12;
		} split;
	} a = { regs[eax] };

	std::cout << "Nested hypervisor virtualization features\n";
	std::cout << "\tEnlightened VMCS version: {:d}.{:d}\n"_format(a.split.high_version, a.split.low_version);
	print_features(cpu, leaf_t::hyper_v_nested_features, subleaf_t::main, eax);
	std::cout << std::endl;
}

void print_xen_limit(const cpu_t& cpu) {
	const leaf_t leaf = (cpu.vendor & hyper_v) ? leaf_t::xen_limit : leaf_t::xen_limit_offset;
	const register_set_t& regs = cpu.leaves.at(leaf).at(subleaf_t::main);
	std::cout << "Xen with Viridian Extensions present\n";
	std::cout << "\tMaximum Xen leaf: 0x" << std::setw(2) << std::setfill('0') << std::hex << regs[eax] << "\n";

	const union
	{
		std::array<std::uint32_t, 3> registers;
		std::array<char, 12> vndr;
	} data = { regs[ebx], regs[ecx], regs[edx] };

	std::cout << "\tVendor: ";
	std::cout.write(data.vndr.data(), gsl::narrow_cast<std::streamsize>(data.vndr.size()));
	std::cout << "\n";
	std::cout << "\tVendor name: " << to_string(cpu.vendor) << std::endl;
	std::cout << std::endl;
}

void print_xen_version(const cpu_t& cpu) {
	using namespace fmt::literals;
	const leaf_t leaf = (cpu.vendor & hyper_v) ? leaf_t::xen_version : leaf_t::xen_version_offset;
	const register_set_t& regs = cpu.leaves.at(leaf).at(subleaf_t::main);

	const union
	{
		std::uint32_t full;
		struct
		{
			std::uint32_t minor_version : 16;
			std::uint32_t major_version : 16;
		} split;
	} a = { regs[eax] };

	std::cout << "Xen version\n";
	std::cout << "\tVersion : {:d}.{:d}\n"_format(a.split.major_version, a.split.minor_version);
	std::cout << std::endl;
}

void print_xen_features(const cpu_t& cpu) {
	const leaf_t leaf = (cpu.vendor & hyper_v) ? leaf_t::xen_features : leaf_t::xen_features_offset;
	const register_set_t& regs = cpu.leaves.at(leaf).at(subleaf_t::main);

	std::cout << "Xen features\n";
	std::cout << "\tHypercall transfer pages: " << std::dec << regs[eax] << "\n";
	std::cout << "\tXen MSR base address: " << std::setw(8) << std::setfill('0') << std::hex << regs[ebx] << "\n";
	std::cout << std::endl;
}

void enumerate_xen_time(cpu_t& cpu) {
	const leaf_t leaf = (cpu.vendor & hyper_v) ? leaf_t::xen_time : leaf_t::xen_time_offset;
	register_set_t regs = {};
	cpuid(regs, leaf, subleaf_t::xen_time_main);
	cpu.leaves[leaf][subleaf_t::xen_time_main] = regs;
	cpuid(regs, leaf, subleaf_t::xen_time_tsc_offset);
	cpu.leaves[leaf][subleaf_t::xen_time_tsc_offset] = regs;
	cpuid(regs, leaf, subleaf_t::xen_time_host);
	cpu.leaves[leaf][subleaf_t::xen_time_host] = regs;
}

void print_xen_time(const cpu_t& cpu) {
	const leaf_t leaf = (cpu.vendor & hyper_v) ? leaf_t::xen_time : leaf_t::xen_time_offset;
	for(const auto& sub : cpu.leaves.at(leaf)) {
		const register_set_t& regs = sub.second;
		switch(sub.first) {
		case subleaf_t::xen_time_main:
			std::cout << "Xen time features\n";
			print_features(cpu, leaf, subleaf_t::xen_time_main, eax);
			std::cout << "\n";
			std::cout << "\tTSC mode: " << std::setw(8) << std::setfill('0') << std::hex << regs[ebx] << "\n";
			std::cout << "\tTSC/kHz: " << std::dec << regs[ecx] << "\n";
			std::cout << "\tTSC restarts: " << std::dec << regs[ecx] << "\n";
			std::cout << std::endl;
			break;
		case subleaf_t::xen_time_tsc_offset:
			{
				const std::uint64_t tsc_offset = (gsl::narrow_cast<std::uint64_t>(regs[ebx]) << 32ui64) | gsl::narrow_cast<std::uint64_t>(regs[eax]);
				std::cout << "Xen TSC scaling and offset\n";
				std::cout << "\tTSC offset: " << std::dec << tsc_offset << "\n";
				std::cout << "\tTSC to nanoseconds factor: " << std::dec << regs[ecx] << "\n";
				std::cout << "\tTSC to nanoseconds shift: " << gsl::narrow_cast<std::int8_t>(regs[edx]) << "\n";
				std::cout << std::endl;
			}
			break;
		case subleaf_t::xen_time_host:
			std::cout << "Xen host CPU\n";
			std::cout << "\tHost CPU speed/kHz: " << std::dec << regs[eax] << "\n";
			std::cout << std::endl;
			break;
		default:
			__assume(0);
		}
	}
}

void print_xen_hvm_features(const cpu_t& cpu) {
	const leaf_t leaf = (cpu.vendor & hyper_v) ? leaf_t::xen_hvm_features : leaf_t::xen_hvm_features_offset;
	const register_set_t& regs = cpu.leaves.at(leaf).at(subleaf_t::main);
	std::cout << "Xen HVM features\n";
	print_features(cpu, leaf, subleaf_t::main, eax);
	if(0x0000'0008ui32 & cpu.leaves.at(leaf).at(subleaf_t::main).at(eax)) {
		std::cout << "\tVCPU ID: " << std::setw(8) << std::setfill('0') << std::hex << regs[ebx] << "\n";
	}
	std::cout << std::endl;
}

void print_xen_pv_features(const cpu_t& cpu) {
	const leaf_t leaf = (cpu.vendor & hyper_v) ? leaf_t::xen_pv_features : leaf_t::xen_pv_features_offset;
	const register_set_t& regs = cpu.leaves.at(leaf).at(subleaf_t::main);

	const union
	{
		std::uint32_t full;
		struct
		{
			std::uint32_t machine_address_width : 8;
			std::uint32_t reserved_1            : 24;
		} split;
	} b = { regs[ebx] };

	std::cout << "Xen PV features\n";
	std::cout << "\tHighest subleaf: " << std::setw(8) << std::setfill('0') << std::hex << regs[eax] << "\n";
	std::cout << std::endl;
}

void print_vmware_timing(const cpu_t& cpu) {
	const register_set_t& regs = cpu.leaves.at(leaf_t::vmware_timing).at(subleaf_t::main);
	std::cout << "VMware timing\n";
	std::cout << "\tVirtual TSC frequency/kHz: " << std::dec << regs[eax] << "\n";
	std::cout << "\tVirtual bus/APIC frequency/kHz: " << std::dec << regs[ebx] << "\n";
	std::cout << std::endl;
}

void print_kvm_features(const cpu_t& cpu) {
	std::cout << "KVM features\n";
	print_features(cpu, leaf_t::kvm_features, subleaf_t::main, eax);
	std::cout << std::endl;
}

#include "stdafx.h"

#include "hypervisors.hpp"
#include "features.hpp"

#include <array>

#include <fmt/format.h>

#include "utility.hpp"

#if defined(_MSC_VER)
#pragma warning(disable: 26446) // warning c26446: Prefer to use gsl::at() instead of unchecked subscript operator (bounds.4).
#endif

namespace cpuid {

void print_hypervisor_limit(fmt::memory_buffer& out, const cpu_t& cpu) {
	const register_set_t& regs = cpu.leaves.at(leaf_type::hypervisor_limit).at(subleaf_type::main);
	format_to(out, "Hypervisor present\n");
	format_to(out, "\tMaximum hypervisor leaf: {:#010x}\n", regs[eax]);

	const std::array<char, 12> vndr = bit_cast<decltype(vndr)>(
		std::array<std::uint32_t, 3> {
			regs[ebx],
			regs[ecx],
			regs[edx]
		}
	);

	format_to(out, "\tVendor: {}\n", vndr);
	format_to(out, "\tVendor name: {:s}\n", to_string(cpu.vendor));
	format_to(out, "\n");
}

void print_hyper_v_signature(fmt::memory_buffer& out, const cpu_t& cpu) {
	const register_set_t& regs = cpu.leaves.at(leaf_type::hyper_v_signature).at(subleaf_type::main);
	format_to(out, "Hyper-V signature\n");
	const struct
	{
		std::uint32_t signature_1 : 8;
		std::uint32_t signature_2 : 8;
		std::uint32_t signature_3 : 8;
		std::uint32_t signature_4 : 8;
	} a = bit_cast<decltype(a)>(regs[eax]);

	format_to(out, "\tSignature: {:c}{:c}{:c}{:c}\n", gsl::narrow_cast<char>(a.signature_1),
	                                                  gsl::narrow_cast<char>(a.signature_2),
	                                                  gsl::narrow_cast<char>(a.signature_3),
	                                                  gsl::narrow_cast<char>(a.signature_4));
	format_to(out, "\n");
}

void print_hyper_v_system_identity(fmt::memory_buffer& out, const cpu_t& cpu) {
	const register_set_t& regs = cpu.leaves.at(leaf_type::hyper_v_system_identity).at(subleaf_type::main);
	format_to(out, "Hyper-V system identiy\n");

	const struct
	{
		std::uint32_t minor_version : 16;
		std::uint32_t major_version : 16;
	} b = bit_cast<decltype(b)>(regs[ebx]);

	const struct
	{
		std::uint32_t service_number : 24;
		std::uint32_t service_branch : 8;
	} d = bit_cast<decltype(d)>(regs[edx]);

	format_to(out, "\tBuild number: {:d}\n", regs[eax]);
	format_to(out, "\tVersion : {:d}.{:d}\n", b.major_version, b.minor_version);
	format_to(out, "\tService Pack: {:d}\n", regs[ecx]);
	format_to(out, "\tService number: {:d}\n", d.service_number);
	format_to(out, "\tService branch: {:d}\n", d.service_branch);
	format_to(out, "\n");
}

void print_hyper_v_features(fmt::memory_buffer& out, const cpu_t& cpu) {
	const register_set_t& regs = cpu.leaves.at(leaf_type::hyper_v_features).at(subleaf_type::main);
	const struct
	{
		std::uint32_t maximum_power_state : 4;
		std::uint32_t hpet_needed_for_c3  : 1;
		std::uint32_t reserved_1          : 27;
	} c = bit_cast<decltype(c)>(regs[ecx]);

	format_to(out, "Hyper-V features\n");
	print_features(out, cpu, leaf_type::hyper_v_features, subleaf_type::main, eax);
	format_to(out, "\n");
	print_features(out, cpu, leaf_type::hyper_v_features, subleaf_type::main, ebx);
	format_to(out, "\n");
	format_to(out, "\tMaximum power state: C{:d}\n", c.maximum_power_state);
	if(c.hpet_needed_for_c3) {
		format_to(out, "\tHPET needed for C3\n");
	}
	format_to(out, "\n");
	print_features(out, cpu, leaf_type::hyper_v_features, subleaf_type::main, edx);
	format_to(out, "\n");
}

void print_hyper_v_enlightenment_recs(fmt::memory_buffer& out, const cpu_t& cpu) {
	const register_set_t& regs = cpu.leaves.at(leaf_type::hyper_v_enlightenment_recs).at(subleaf_type::main);
	format_to(out, "Hyper-V recommendations\n");
	print_features(out, cpu, leaf_type::hyper_v_enlightenment_recs, subleaf_type::main, eax);
	format_to(out, "\n");
	if(regs[ebx] != 0xffff'ffff_u32) {
		format_to(out, "\tTimes to retry spinlocks before notifying hypervisor: {:d}\n", regs[ebx]);
	} else {
		format_to(out, "\tNever retry spinlocks before notifying hypervisor\n");
	}
	format_to(out, "\n");
}

void print_hyper_v_implementation_limits(fmt::memory_buffer& out, const cpu_t& cpu) {
	const register_set_t& regs = cpu.leaves.at(leaf_type::hyper_v_implementation_limits).at(subleaf_type::main);
	format_to(out, "Hyper-V implementation limits\n");
	format_to(out, "\tMaximum virtual processors supported: {:d}\n", regs[eax]);
	format_to(out, "\tMaximum logical processors supported: {:d}\n", regs[ebx]);
	format_to(out, "\tMaximum physical interrupt vectors supported: {:d}\n", regs[ecx]);
	format_to(out, "\n");
}

void print_hyper_v_implementation_hardware(fmt::memory_buffer& out, const cpu_t& cpu) {
	format_to(out, "Hyper-V implementation hardware\n");
	print_features(out, cpu, leaf_type::hyper_v_implementation_hardware, subleaf_type::main, eax);
	format_to(out, "\n");
}

void print_hyper_v_root_cpu_management(fmt::memory_buffer& out, const cpu_t& cpu) {
	format_to(out, "Hyper-V root CPU management\n");
	print_features(out, cpu, leaf_type::hyper_v_root_cpu_management, subleaf_type::main, eax);
	print_features(out, cpu, leaf_type::hyper_v_root_cpu_management, subleaf_type::main, ebx);
	format_to(out, "\n");
}

void print_hyper_v_shared_virtual_memory(fmt::memory_buffer& out, const cpu_t& cpu) {
	const register_set_t& regs = cpu.leaves.at(leaf_type::hyper_v_shared_virtual_memory).at(subleaf_type::main);

	const struct
	{ 
		std::uint32_t svm_supported : 1;
		std::uint32_t reserved_1    : 10;
		std::uint32_t max_pasid_space_count : 21;
	} a = bit_cast<decltype(a)>(regs[eax]);

	format_to(out, "Hyper-V shared virtual memory features\n");
	print_features(out, cpu, leaf_type::hyper_v_shared_virtual_memory, subleaf_type::main, eax);
	format_to(out, "\n");
	format_to(out, "\tMaximum PASID space count: {:d}\n", a.max_pasid_space_count);
	format_to(out, "\n");
}

void print_hyper_v_nested_hypervisor(fmt::memory_buffer& out, const cpu_t& cpu) {
	format_to(out, "Nested hypervisor features\n");
	print_features(out, cpu, leaf_type::hyper_v_nested_hypervisor, subleaf_type::main, eax);
	print_features(out, cpu, leaf_type::hyper_v_nested_hypervisor, subleaf_type::main, edx);
	format_to(out, "\n");
}

void print_hyper_v_nested_features(fmt::memory_buffer& out, const cpu_t& cpu) {
	const register_set_t& regs = cpu.leaves.at(leaf_type::hyper_v_nested_features).at(subleaf_type::main);
	const struct
	{
		std::uint32_t low_version            : 8;
		std::uint32_t high_version           : 8;
		std::uint32_t reserved_1             : 1;
		std::uint32_t direct_flush           : 1;
		std::uint32_t flush_guest_physical   : 1;
		std::uint32_t enlightened_msr_bitmap : 1;
		std::uint32_t reserved_2             : 12;
	} a = bit_cast<decltype(a)>(regs[eax]);

	format_to(out, "Nested hypervisor virtualization features\n");
	format_to(out, "\tEnlightened VMCS version: {:d}.{:d}\n", a.high_version, a.low_version);
	print_features(out, cpu, leaf_type::hyper_v_nested_features, subleaf_type::main, eax);
	format_to(out, "\n");
}

void print_xen_limit(fmt::memory_buffer& out, const cpu_t& cpu) {
	const leaf_type leaf = (cpu.vendor & vendor_type::hyper_v) == vendor_type::hyper_v ? leaf_type::xen_limit : leaf_type::xen_limit_offset;
	const register_set_t& regs = cpu.leaves.at(leaf).at(subleaf_type::main);
	format_to(out, "Xen{:s} signature\n", (cpu.vendor & vendor_type::hyper_v) == vendor_type::hyper_v ? " with viridian extensions" : "");
	format_to(out, "\tMaximum Xen leaf: {:#010x}\n", regs[eax]);

	const std::array<char, 12> vndr = bit_cast<decltype(vndr)>(
		std::array<std::uint32_t, 3> {
			regs[ebx],
			regs[ecx],
			regs[edx]
		}
	);

	format_to(out, "\tVendor: {}\n", vndr);
	format_to(out, "\tVendor name: {:s}\n", to_string(cpu.vendor));
	format_to(out, "\n");
}

void print_xen_version(fmt::memory_buffer& out, const cpu_t& cpu) {
	const leaf_type leaf = (cpu.vendor & vendor_type::hyper_v) == vendor_type::hyper_v ? leaf_type::xen_version : leaf_type::xen_version_offset;
	const register_set_t& regs = cpu.leaves.at(leaf).at(subleaf_type::main);

	const struct
	{
		std::uint32_t minor_version : 16;
		std::uint32_t major_version : 16;
	} a = bit_cast<decltype(a)>(regs[eax]);

	format_to(out, "Xen version\n");
	format_to(out, "\tVersion : {:d}.{:d}\n", a.major_version, a.minor_version);
	format_to(out, "\n");
}

void print_xen_features(fmt::memory_buffer& out, const cpu_t& cpu) {
	const leaf_type leaf = (cpu.vendor & vendor_type::hyper_v) == vendor_type::hyper_v ? leaf_type::xen_features : leaf_type::xen_features_offset;
	const register_set_t& regs = cpu.leaves.at(leaf).at(subleaf_type::main);

	format_to(out, "Xen features\n");
	format_to(out, "\tHypercall transfer pages: {:d}\n", regs[eax]);
	format_to(out, "\tXen MSR base address: {:#010x}\n", regs[ebx]);
	format_to(out, "\n");
}

void enumerate_xen_time(cpu_t& cpu) {
	const leaf_type leaf = (cpu.vendor & vendor_type::hyper_v) == vendor_type::hyper_v ? leaf_type::xen_time : leaf_type::xen_time_offset;
	cpu.leaves[leaf][subleaf_type::xen_time_main      ] = cpuid(leaf, subleaf_type::xen_time_main      );
	cpu.leaves[leaf][subleaf_type::xen_time_tsc_offset] = cpuid(leaf, subleaf_type::xen_time_tsc_offset);
	cpu.leaves[leaf][subleaf_type::xen_time_host      ] = cpuid(leaf, subleaf_type::xen_time_host      );
}

void print_xen_time(fmt::memory_buffer& out, const cpu_t& cpu) {
	const leaf_type leaf = (cpu.vendor & vendor_type::hyper_v) == vendor_type::hyper_v ? leaf_type::xen_time : leaf_type::xen_time_offset;
	for(const auto& sub : cpu.leaves.at(leaf)) {
		const register_set_t& regs = sub.second;
		switch(sub.first) {
		case subleaf_type::xen_time_main:
			format_to(out, "Xen time features\n");
			print_features(out, cpu, leaf, subleaf_type::xen_time_main, eax);
			format_to(out, "\n");
			format_to(out, "\tTSC mode: {:#010x}\n", regs[ebx]);
			format_to(out, "\tTSC/kHz: {:d}\n", regs[ecx]);
			format_to(out, "\tTSC restarts: {:d}\n", regs[ecx]);
			format_to(out, "\n");
			break;
		case subleaf_type::xen_time_tsc_offset:
			{
				const std::uint64_t tsc_offset = (gsl::narrow_cast<std::uint64_t>(regs[ebx]) << 32_u64) | gsl::narrow_cast<std::uint64_t>(regs[eax]);
				format_to(out, "Xen TSC scaling and offset\n");
				format_to(out, "\tTSC offset: {:d}\n", tsc_offset);
				format_to(out, "\tTSC to nanoseconds factor: {:d}\n", regs[ecx]);
				format_to(out, "\tTSC to nanoseconds shift: {:d}\n", gsl::narrow_cast<std::int8_t>(regs[edx]));
				format_to(out, "\n");
			}
			break;
		case subleaf_type::xen_time_host:
			format_to(out, "Xen host CPU\n");
			format_to(out, "\tHost CPU speed/kHz: {:d}\n", regs[eax]);
			format_to(out, "\n");
			break;
		default:
			format_to(out, "Xen unknown time subleaf\n");
			print_generic(out, cpu, leaf, sub.first);
			format_to(out, "\n");
		}
	}
}

void print_xen_hvm_features(fmt::memory_buffer& out, const cpu_t& cpu) {
	const leaf_type leaf = (cpu.vendor & vendor_type::hyper_v) == vendor_type::hyper_v ? leaf_type::xen_hvm_features : leaf_type::xen_hvm_features_offset;
	const register_set_t& regs = cpu.leaves.at(leaf).at(subleaf_type::main);
	format_to(out, "Xen HVM features\n");
	print_features(out, cpu, leaf, subleaf_type::main, eax);
	if(0x0000'0008_u32 & cpu.leaves.at(leaf).at(subleaf_type::main).at(eax)) {
		format_to(out, "\tVCPU ID: {:#010x}\n", regs[ebx]);
	}
	format_to(out, "\n");
}

void print_xen_pv_features(fmt::memory_buffer& out, const cpu_t& cpu) {
	const leaf_type leaf = (cpu.vendor & vendor_type::hyper_v) == vendor_type::hyper_v ? leaf_type::xen_pv_features : leaf_type::xen_pv_features_offset;
	const register_set_t& regs = cpu.leaves.at(leaf).at(subleaf_type::main);

	const struct
	{
		std::uint32_t machine_address_width : 8;
		std::uint32_t reserved_1            : 24;
	} b = bit_cast<decltype(b)>(regs[ebx]);

	format_to(out, "Xen PV features\n");
	format_to(out, "\tHighest subleaf: {:#010x}\n", regs[eax]);
	format_to(out, "\n");
}

void print_vmware_timing(fmt::memory_buffer& out, const cpu_t& cpu) {
	const register_set_t& regs = cpu.leaves.at(leaf_type::vmware_timing).at(subleaf_type::main);
	format_to(out, "VMware timing\n");
	format_to(out, "\tVirtual TSC frequency/kHz: {:d}\n", regs[eax]);
	format_to(out, "\tVirtual bus/APIC frequency/kHz: {:d}\n", regs[ebx]);
	format_to(out, "\n");
}

void print_kvm_features(fmt::memory_buffer& out, const cpu_t& cpu) {
	format_to(out, "KVM features\n");
	print_features(out, cpu, leaf_type::kvm_features, subleaf_type::main, eax);
	format_to(out, "\n");
}

}

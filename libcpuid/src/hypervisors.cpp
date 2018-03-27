#include "stdafx.h"

#include "hypervisors.hpp"
#include "features.hpp"

#include <array>

#include <fmt/format.h>

#include "utility.hpp"

void print_hypervisor_limit(fmt::Writer& w, const cpu_t& cpu) {
	const register_set_t& regs = cpu.leaves.at(leaf_t::hypervisor_limit).at(subleaf_t::main);
	w.write("Hypervisor present\n");
	w.write("\tMaximum hypervisor leaf: {:#010x}\n", regs[eax]);

	const union
	{
		std::array<std::uint32_t, 3> registers;
		std::array<char, 12> vndr;
	} data = { regs[ebx], regs[ecx], regs[edx] };

	w.write("\tVendor: {}\n", data.vndr);
	w.write("\tVendor name: {:s}\n", to_string(cpu.vendor));
	w.write("\n");
}

void print_hyper_v_signature(fmt::Writer& w, const cpu_t& cpu) {
	const register_set_t& regs = cpu.leaves.at(leaf_t::hyper_v_signature).at(subleaf_t::main);
	w.write("Hyper-V signature\n");
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

	w.write("\tSignature: {:c}{:c}{:c}{:c}\n", gsl::narrow_cast<char>(a.split.signature_1),
	                                           gsl::narrow_cast<char>(a.split.signature_2),
	                                           gsl::narrow_cast<char>(a.split.signature_3),
	                                           gsl::narrow_cast<char>(a.split.signature_4));
	w.write("\n");
}

void print_hyper_v_system_identity(fmt::Writer& w, const cpu_t& cpu) {
	const register_set_t& regs = cpu.leaves.at(leaf_t::hyper_v_system_identity).at(subleaf_t::main);
	w.write("Hyper-V system identiy\n");

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

	w.write("\tBuild number: {:d}\n", regs[eax]);
	w.write("\tVersion : {:d}.{:d}\n", b.split.major_version, b.split.minor_version);
	w.write("\tService Pack: {:d}\n", regs[ecx]);
	w.write("\tService number: {:d}\n", d.split.service_number);
	w.write("\tService branch: {:d}\n", d.split.service_branch);
	w.write("\n");
}

void print_hyper_v_features(fmt::Writer& w, const cpu_t& cpu) {
	const register_set_t& regs = cpu.leaves.at(leaf_t::hyper_v_features).at(subleaf_t::main);
	const union
	{
		std::uint32_t full;
		struct
		{
			std::uint32_t maximum_power_state : 4;
			std::uint32_t hpet_needed_for_c3  : 1;
			std::uint32_t reserved_1          : 27;
		} split;
	} c = { regs[ecx] };

	w.write("Hyper-V features\n");
	print_features(w, cpu, leaf_t::hyper_v_features, subleaf_t::main, eax);
	w.write("\n");
	print_features(w, cpu, leaf_t::hyper_v_features, subleaf_t::main, ebx);
	w.write("\n");
	w.write("\tMaximum power state: C{:d}\n", c.split.maximum_power_state);
	if(c.split.hpet_needed_for_c3) {
		w.write("\tHPET needed for C3\n");
	}
	w.write("\n");
	print_features(w, cpu, leaf_t::hyper_v_features, subleaf_t::main, edx);
	w.write("\n");
}

void print_hyper_v_enlightenment_recs(fmt::Writer& w, const cpu_t& cpu) {
	const register_set_t& regs = cpu.leaves.at(leaf_t::hyper_v_enlightenment_recs).at(subleaf_t::main);
	w.write("Hyper-V recommendations\n");
	print_features(w, cpu, leaf_t::hyper_v_enlightenment_recs, subleaf_t::main, eax);
	w.write("\n");
	if(regs[ebx] != 0xffff'ffffui32) {
		w.write("\tTimes to retry spinlocks before notifying hypervisor: {:d}\n", regs[ebx]);
	} else {
		w.write("\tNever retry spinlocks before notifying hypervisor\n");
	}
	w.write("\n");
}

void print_hyper_v_implementation_limits(fmt::Writer& w, const cpu_t& cpu) {
	const register_set_t& regs = cpu.leaves.at(leaf_t::hyper_v_implementation_limits).at(subleaf_t::main);
	w.write("Hyper-V implementation limits\n");
	w.write("\tMaximum virtual processors supported: {:d}\n", regs[eax]);
	w.write("\tMaximum logical processors supported: {:d}\n", regs[ebx]);
	w.write("\tMaximum physical interrupt vectors supported: {:d}\n", regs[ecx]);
	w.write("\n");
}

void print_hyper_v_implementation_hardware(fmt::Writer& w, const cpu_t& cpu) {
	w.write("Hyper-V implementation hardware\n");
	print_features(w, cpu, leaf_t::hyper_v_implementation_hardware, subleaf_t::main, eax);
	w.write("\n");
}

void print_hyper_v_root_cpu_management(fmt::Writer& w, const cpu_t& cpu) {
	w.write("Hyper-V root CPU management\n");
	print_features(w, cpu, leaf_t::hyper_v_root_cpu_management, subleaf_t::main, eax);
	print_features(w, cpu, leaf_t::hyper_v_root_cpu_management, subleaf_t::main, ebx);
	w.write("\n");
}

void print_hyper_v_shared_virtual_memory(fmt::Writer& w, const cpu_t& cpu) {
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

	w.write("Hyper-V shared virtual memory features\n");
	print_features(w, cpu, leaf_t::hyper_v_shared_virtual_memory, subleaf_t::main, eax);
	w.write("\n");
	w.write("\tMaximum PASID space count: {:d}\n", a.split.max_pasid_space_count);
	w.write("\n");
}

void print_hyper_v_nested_hypervisor(fmt::Writer& w, const cpu_t& cpu) {
	w.write("Nested hypervisor features\n");
	print_features(w, cpu, leaf_t::hyper_v_nested_hypervisor, subleaf_t::main, eax);
	print_features(w, cpu, leaf_t::hyper_v_nested_hypervisor, subleaf_t::main, edx);
	w.write("\n");
}

void print_hyper_v_nested_features(fmt::Writer& w, const cpu_t& cpu) {
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

	w.write("Nested hypervisor virtualization features\n");
	w.write("\tEnlightened VMCS version: {:d}.{:d}\n", a.split.high_version, a.split.low_version);
	print_features(w, cpu, leaf_t::hyper_v_nested_features, subleaf_t::main, eax);
	w.write("\n");
}

void print_xen_limit(fmt::Writer& w, const cpu_t& cpu) {
	const leaf_t leaf = (cpu.vendor & hyper_v) ? leaf_t::xen_limit : leaf_t::xen_limit_offset;
	const register_set_t& regs = cpu.leaves.at(leaf).at(subleaf_t::main);
	w.write("Xen{:s} signature\n", (cpu.vendor & hyper_v) ? " with viridian extensions" : "");
	w.write("\tMaximum Xen leaf: {:#010x}\n", regs[eax]);

	const union
	{
		std::array<std::uint32_t, 3> registers;
		std::array<char, 12> vndr;
	} data = { regs[ebx], regs[ecx], regs[edx] };

	w.write("\tVendor: {}\n", data.vndr);
	w.write("\tVendor name: {:s}\n", to_string(cpu.vendor));
	w.write("\n");
}

void print_xen_version(fmt::Writer& w, const cpu_t& cpu) {
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

	w.write("Xen version\n");
	w.write("\tVersion : {:d}.{:d}\n", a.split.major_version, a.split.minor_version);
	w.write("\n");
}

void print_xen_features(fmt::Writer& w, const cpu_t& cpu) {
	const leaf_t leaf = (cpu.vendor & hyper_v) ? leaf_t::xen_features : leaf_t::xen_features_offset;
	const register_set_t& regs = cpu.leaves.at(leaf).at(subleaf_t::main);

	w.write("Xen features\n");
	w.write("\tHypercall transfer pages: {:d}\n", regs[eax]);
	w.write("\tXen MSR base address: {:#010x}\n", regs[ebx]);
	w.write("\n");
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

void print_xen_time(fmt::Writer& w, const cpu_t& cpu) {
	const leaf_t leaf = (cpu.vendor & hyper_v) ? leaf_t::xen_time : leaf_t::xen_time_offset;
	for(const auto& sub : cpu.leaves.at(leaf)) {
		const register_set_t& regs = sub.second;
		switch(sub.first) {
		case subleaf_t::xen_time_main:
			w.write("Xen time features\n");
			print_features(w, cpu, leaf, subleaf_t::xen_time_main, eax);
			w.write("\n");
			w.write("\tTSC mode: {:#010x}\n", regs[ebx]);
			w.write("\tTSC/kHz: {:d}\n", regs[ecx]);
			w.write("\tTSC restarts: {:d}\n", regs[ecx]);
			w.write("\n");
			break;
		case subleaf_t::xen_time_tsc_offset:
			{
				const std::uint64_t tsc_offset = (gsl::narrow_cast<std::uint64_t>(regs[ebx]) << 32ui64) | gsl::narrow_cast<std::uint64_t>(regs[eax]);
				w.write("Xen TSC scaling and offset\n");
				w.write("\tTSC offset: {:d}\n", tsc_offset);
				w.write("\tTSC to nanoseconds factor: {:d}\n", regs[ecx]);
				w.write("\tTSC to nanoseconds shift: {:d}\n", gsl::narrow_cast<std::int8_t>(regs[edx]));
				w.write("\n");
			}
			break;
		case subleaf_t::xen_time_host:
			w.write("Xen host CPU\n");
			w.write("\tHost CPU speed/kHz: {:d}\n", regs[eax]);
			w.write("\n");
			break;
		default:
			w.write("Xen unknown time subleaf\n");
			print_generic(w, cpu, leaf, sub.first);
			w.write("\n");
		}
	}
}

void print_xen_hvm_features(fmt::Writer& w, const cpu_t& cpu) {
	const leaf_t leaf = (cpu.vendor & hyper_v) ? leaf_t::xen_hvm_features : leaf_t::xen_hvm_features_offset;
	const register_set_t& regs = cpu.leaves.at(leaf).at(subleaf_t::main);
	w.write("Xen HVM features\n");
	print_features(w, cpu, leaf, subleaf_t::main, eax);
	if(0x0000'0008ui32 & cpu.leaves.at(leaf).at(subleaf_t::main).at(eax)) {
		w.write("\tVCPU ID: {:#010x}\n", regs[ebx]);
	}
	w.write("\n");
}

void print_xen_pv_features(fmt::Writer& w, const cpu_t& cpu) {
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

	w.write("Xen PV features\n");
	w.write("\tHighest subleaf: {:#010x}\n", regs[eax]);
	w.write("\n");
}

void print_vmware_timing(fmt::Writer& w, const cpu_t& cpu) {
	const register_set_t& regs = cpu.leaves.at(leaf_t::vmware_timing).at(subleaf_t::main);
	w.write("VMware timing\n");
	w.write("\tVirtual TSC frequency/kHz: {:d}\n", regs[eax]);
	w.write("\tVirtual bus/APIC frequency/kHz: {:d}\n", regs[ebx]);
	w.write("\n");
}

void print_kvm_features(fmt::Writer& w, const cpu_t& cpu) {
	w.write("KVM features\n");
	print_features(w, cpu, leaf_t::kvm_features, subleaf_t::main, eax);
	w.write("\n");
}

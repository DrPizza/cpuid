#ifndef CPUID_HPP
#define CPUID_HPP

#include <cstddef>
#include <array>
#include <map>

#include <gsl/gsl>

enum struct leaf_t : std::uint32_t
{
	basic_info                        = 0x0000'0000ui32,
	version_info                      = 0x0000'0001ui32,
	cache_and_tlb                     = 0x0000'0002ui32,
	serial_number                     = 0x0000'0003ui32,
	deterministic_cache               = 0x0000'0004ui32,
	monitor_mwait                     = 0x0000'0005ui32,
	thermal_and_power                 = 0x0000'0006ui32,
	extended_features                 = 0x0000'0007ui32,
	reserved_1                        = 0x0000'0008ui32,
	direct_cache_access               = 0x0000'0009ui32,
	performance_monitoring            = 0x0000'000aui32,
	extended_topology                 = 0x0000'000bui32,
	reserved_2                        = 0x0000'000cui32,
	extended_state                    = 0x0000'000dui32,
	reserved_3                        = 0x0000'000eui32,
	rdt_monitoring                    = 0x0000'000fui32,
	rdt_allocation                    = 0x0000'0010ui32,
	reserved_4                        = 0x0000'0011ui32,
	sgx_info                          = 0x0000'0012ui32,
	reserved_5                        = 0x0000'0013ui32,
	processor_trace                   = 0x0000'0014ui32,
	time_stamp_counter                = 0x0000'0015ui32,
	processor_frequency               = 0x0000'0016ui32,
	system_on_chip_vendor             = 0x0000'0017ui32,
	deterministic_tlb                 = 0x0000'0018ui32,
	reserved_6                        = 0x0000'0019ui32,
	reserved_7                        = 0x0000'001aui32,
	pconfig                           = 0x0000'001bui32,
	hypervisor_limit                  = 0x4000'0000ui32,

	hyper_v_signature                 = 0x4000'0001ui32,
	hyper_v_system_identity           = 0x4000'0002ui32,
	hyper_v_features                  = 0x4000'0003ui32,
	hyper_v_enlightenment_recs        = 0x4000'0004ui32,
	hyper_v_implementation_limits     = 0x4000'0005ui32,
	hyper_v_implementation_hardware   = 0x4000'0006ui32,
	hyper_v_root_cpu_management       = 0x4000'0007ui32,
	hyper_v_shared_virtual_memory     = 0x4000'0008ui32,
	hyper_v_nested_hypervisor         = 0x4000'0009ui32,
	hyper_v_nested_features           = 0x4000'000aui32,

	xen_limit                         = 0x4000'0000ui32, xen_limit_offset                  = 0x4000'0100ui32,
	xen_version                       = 0x4000'0001ui32, xen_version_offset                = 0x4000'0101ui32,
	xen_features                      = 0x4000'0002ui32, xen_features_offset               = 0x4000'0102ui32,
	xen_time                          = 0x4000'0003ui32, xen_time_offset                   = 0x4000'0103ui32,
	xen_hvm_features                  = 0x4000'0004ui32, xen_hvm_features_offset           = 0x4000'0104ui32,
	xen_pv_features                   = 0x4000'0005ui32, xen_pv_features_offset            = 0x4000'0105ui32,

	vmware_timing                     = 0x4000'0010ui32,

	kvm_features                      = 0x4000'0001ui32,

	extended_limit                    = 0x8000'0000ui32,
	extended_signature_and_features   = 0x8000'0001ui32,
	brand_string_0                    = 0x8000'0002ui32,
	brand_string_1                    = 0x8000'0003ui32,
	brand_string_2                    = 0x8000'0004ui32,
	l1_cache_identifiers              = 0x8000'0005ui32,
	l2_cache_identifiers              = 0x8000'0006ui32,
	ras_advanced_power_management     = 0x8000'0007ui32,
	address_limits                    = 0x8000'0008ui32,
	reserved_8                        = 0x8000'0009ui32,
	secure_virtual_machine            = 0x8000'000aui32,

	reserved_9                        = 0x8000'000bui32,
	reserved_10                       = 0x8000'000cui32,
	reserved_11                       = 0x8000'000dui32,
	reserved_12                       = 0x8000'000eui32,
	reserved_13                       = 0x8000'000fui32,
	reserved_14                       = 0x8000'0010ui32,
	reserved_15                       = 0x8000'0011ui32,
	reserved_16                       = 0x8000'0012ui32,
	reserved_17                       = 0x8000'0013ui32,
	reserved_18                       = 0x8000'0014ui32,
	reserved_19                       = 0x8000'0015ui32,
	reserved_20                       = 0x8000'0016ui32,
	reserved_21                       = 0x8000'0017ui32,
	reserved_22                       = 0x8000'0018ui32,
	
	tlb_1g_identifiers                = 0x8000'0019ui32,
	performance_optimization          = 0x8000'001aui32,
	instruction_based_sampling        = 0x8000'001bui32,
	lightweight_profiling             = 0x8000'001cui32,
	cache_properties                  = 0x8000'001dui32,
	extended_apic                     = 0x8000'001eui32,
	encrypted_memory                  = 0x8000'001fui32,

	none                              = 0x0000'0000ui32,
};

constexpr inline leaf_t operator++(leaf_t& lhs) {
	lhs = static_cast<leaf_t>(static_cast<std::uint32_t>(lhs) + 1);
	return lhs;
}

constexpr inline leaf_t operator+=(leaf_t& lhs, std::uint32_t rhs) {
	lhs = static_cast<leaf_t>(static_cast<std::uint32_t>(lhs) + rhs);
	return lhs;
}

constexpr inline leaf_t operator+(const leaf_t& lhs, std::uint32_t rhs) {
	return static_cast<leaf_t>(static_cast<std::uint32_t>(lhs) + rhs);
}

enum struct subleaf_t : std::uint32_t
{
	main                                   = 0x0000'0000ui32,
	extended_features_main                 = 0x0000'0000ui32,
	extended_state_main                    = 0x0000'0000ui32,
	extended_state_sub                     = 0x0000'0001ui32,
	rdt_monitoring_main                    = 0x0000'0000ui32,
	rdt_monitoring_l3                      = 0x0000'0001ui32,
	rdt_allocation_main                    = 0x0000'0000ui32,
	rdt_cat_l3                             = 0x0000'0001ui32,
	rdt_cat_l2                             = 0x0000'0002ui32,
	rdt_mba                                = 0x0000'0003ui32,
	sgx_capabilities                       = 0x0000'0000ui32,
	sgx_attributes                         = 0x0000'0001ui32,
	processor_trace_main                   = 0x0000'0000ui32,
	processor_trace_sub                    = 0x0000'0001ui32,
	system_on_chip_vendor_main             = 0x0000'0000ui32,
	system_on_chip_vendor_sub              = 0x0000'0001ui32,
	deterministic_address_translation_main = 0x0000'0000ui32,
	deterministic_address_translation_sub  = 0x0000'0001ui32,
	xen_time_main                          = 0x0000'0000ui32,
	xen_time_tsc_offset                    = 0x0000'0001ui32,
	xen_time_host                          = 0x0000'0002ui32,
	none                                   = 0x0000'0000ui32,
};

constexpr inline subleaf_t operator++(subleaf_t& lhs) {
	lhs = static_cast<subleaf_t>(static_cast<std::uint32_t>(lhs) + 1);
	return lhs;
}

constexpr inline subleaf_t operator+=(subleaf_t& lhs, std::uint32_t offset) {
	lhs = static_cast<subleaf_t>(static_cast<std::uint32_t>(lhs) + offset);
	return lhs;
}

enum register_t : std::uint8_t
{
	eax,
	ebx,
	ecx,
	edx,

	none = 0x00ui8,
};

constexpr inline register_t operator++(register_t& lhs) {
	lhs = static_cast<register_t>(static_cast<std::uint8_t>(lhs) + 1);
	return lhs;
}

enum vendor_t : std::uint32_t
{
	unknown        = 0x0000'0000ui32,
	// silicon
	amd            = 0x0000'0001ui32,
	centaur        = 0x0000'0002ui32,
	cyrix          = 0x0000'0004ui32,
	intel          = 0x0000'0008ui32,
	transmeta      = 0x0000'0010ui32,
	nat_semi       = 0x0000'0020ui32,
	nexgen         = 0x0000'0040ui32,
	rise           = 0x0000'0080ui32,
	sis            = 0x0000'0100ui32,
	umc            = 0x0000'0200ui32,
	via            = 0x0000'0400ui32,
	vortex         = 0x0000'0800ui32,
	// hypervisors
	bhyve          = 0x0001'0000ui32,
	kvm            = 0x0002'0000ui32,
	hyper_v        = 0x0004'0000ui32,
	parallels      = 0x0008'0000ui32,
	vmware         = 0x0010'0000ui32,
	xen_hvm        = 0x0020'0000ui32,
	qemu           = 0x0040'0000ui32,
	// for filtering
	any_silicon    = 0x0000'0fffui32,
	any_hypervisor = 0x007f'0000ui32,
	any            = 0xffff'ffffui32,
};

constexpr inline vendor_t operator|(const vendor_t& lhs, const vendor_t& rhs) {
	return static_cast<vendor_t>(static_cast<std::uint32_t>(lhs) | static_cast<std::uint32_t>(rhs));
}

inline std::string to_string(vendor_t vendor) {
	std::string silicon;
	std::string hypervisor;

	switch(vendor & any_silicon) {
	case amd:
		silicon = "AMD";
		break;
	case centaur:
		silicon = "Centaur";
		break;
	case cyrix:
		silicon = "Cyrix";
		break;
	case intel:
		silicon = "Intel";
		break;
	case transmeta:
		silicon = "Transmeta";
		break;
	case nat_semi:
		silicon = "National Semiconductor";
		break;
	case nexgen:
		silicon = "NexGen";
		break;
	case rise:
		silicon = "Rise";
		break;
	case sis:
		silicon = "SiS";
		break;
	case umc:
		silicon = "UMC";
		break;
	case via:
		silicon = "VIA";
		break;
	case vortex:
		silicon = "Vortex";
		break;
	default:
		silicon = "Unknown";
		break;
	}

	switch(vendor & any_hypervisor) {
	case bhyve:
		hypervisor = "bhyve";
		break;
	case kvm:
		hypervisor = "KVM";
		break;
	case hyper_v:
		hypervisor = "Hyper-V";
		break;
	case parallels:
		hypervisor = "Parallels";
		break;
	case vmware:
		hypervisor = "VMware";
		break;
	case xen_hvm:
		hypervisor = "Xen HVM";
		break;
	case xen_hvm | hyper_v:
		hypervisor = "Xen HVM with Viridian Extensions";
		break;
	case qemu:
		hypervisor = "QEMU";
		break;
	}

	return hypervisor.size() != 0 ? hypervisor + " on " + silicon : silicon;
}

using register_set_t = std::array<std::uint32_t, 4>;
using leaves_t = std::map<leaf_t, std::map<subleaf_t, register_set_t>>;

struct id_info_t
{
	std::uint32_t brand_id                : 8;
	std::uint32_t cache_line_size         : 8;
	std::uint32_t maximum_addressable_ids : 8;
	std::uint32_t local_apic_id           : 8;
};

struct split_model_t
{
	std::uint32_t stepping        : 4;
	std::uint32_t model           : 4;
	std::uint32_t family          : 4;
	std::uint32_t type            : 2;
	std::uint32_t reserved_1      : 2;
	std::uint32_t extended_model  : 4;
	std::uint32_t extended_family : 8;
	std::uint32_t reserved_2      : 4;
};

struct model_t
{
	std::uint32_t stepping;
	std::uint32_t model;
	std::uint32_t family;
};

struct cpu_t
{
	std::uint32_t apic_id;
	leaf_t highest_leaf;
	leaf_t highest_hypervisor_leaf;
	leaf_t xen_base;
	leaf_t highest_xen_leaf;
	leaf_t highest_extended_leaf;
	vendor_t vendor;
	model_t model;
	leaves_t leaves;
};

inline void cpuid(register_set_t& regs, leaf_t leaf, subleaf_t subleaf) noexcept {
	std::array<int, 4> raw_regs;
	__cpuidex(raw_regs.data(), gsl::narrow_cast<int>(leaf), gsl::narrow_cast<int>(subleaf));
	regs[eax] = gsl::narrow_cast<std::uint32_t>(raw_regs[eax]);
	regs[ebx] = gsl::narrow_cast<std::uint32_t>(raw_regs[ebx]);
	regs[ecx] = gsl::narrow_cast<std::uint32_t>(raw_regs[ecx]);
	regs[edx] = gsl::narrow_cast<std::uint32_t>(raw_regs[edx]);
}

void print_generic(const cpu_t& cpu, leaf_t leaf, subleaf_t subleaf);

#endif

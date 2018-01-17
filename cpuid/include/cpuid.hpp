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
	deterministic_address_translation = 0x0000'0018ui32,
	extended_limit                    = 0x8000'0000ui32,
	extended_signature_and_features   = 0x8000'0001ui32,
	brand_string_0                    = 0x8000'0002ui32,
	brand_string_1                    = 0x8000'0003ui32,
	brand_string_2                    = 0x8000'0004ui32,
	l1_cache_identifiers              = 0x8000'0005ui32,
	l2_cache_identifiers              = 0x8000'0006ui32,
	advanced_power_management         = 0x8000'0007ui32,
	address_limits                    = 0x8000'0008ui32,
	secure_virtual_machine            = 0x8000'000aui32,
	tlb_1g_identifiers                = 0x8000'0019ui32,
	performance_optimization          = 0x8000'001aui32,
	instruction_based_sampling        = 0x8000'001bui32,
	cache_properties                  = 0x8000'001dui32,
	extended_apic                     = 0x8000'001eui32,
	secure_memory_encryption          = 0x8000'001fui32,
};

constexpr inline leaf_t operator++(leaf_t& lhs) {
	lhs = static_cast<leaf_t>(static_cast<std::uint32_t>(lhs) + 1);
	return lhs;
}

enum struct subleaf_t : std::uint32_t
{
	main                                   = 0x0000'0000ui32,
	extended_state_main                    = 0x0000'0000ui32,
	extended_state_sub                     = 0x0000'0001ui32,
	rdt_monitoring_main                    = 0x0000'0000ui32,
	rdt_monitoring_l3                      = 0x0000'0001ui32,
	rdt_allocation_main                    = 0x0000'0000ui32,
	rdt_cat_l3                             = 0x0000'0001ui32,
	rdt_cat_l2                             = 0x0000'0002ui32,
	rdt_mba                                = 0x0000'0003ui32,
	sgx_info_main                          = 0x0000'0000ui32,
	sgx_info_attributes                    = 0x0000'0001ui32,
	processor_trace_main                   = 0x0000'0000ui32,
	processor_trace_sub                    = 0x0000'0001ui32,
	system_on_chip_vendor_main             = 0x0000'0000ui32,
	system_on_chip_vendor_sub              = 0x0000'0001ui32,
	deterministic_address_translation_main = 0x0000'0000ui32,
	deterministic_address_translation_sub  = 0x0000'0001ui32,
};

constexpr inline subleaf_t operator++(subleaf_t& lhs) {
	lhs = static_cast<subleaf_t>(static_cast<std::uint32_t>(lhs) + 1);
	return lhs;
}

enum register_t : std::uint8_t
{
	eax,
	ebx,
	ecx,
	edx
};

constexpr inline register_t operator++(register_t& lhs) {
	lhs = static_cast<register_t>(static_cast<std::uint8_t>(lhs) + 1);
	return lhs;
}

enum vendor_t : std::uint32_t
{
	unknown   = 0x0000'0000ui32,
	// silicon
	amd       = 0x0000'0001ui32,
	centaur   = 0x0000'0002ui32,
	cyrix     = 0x0000'0004ui32,
	intel     = 0x0000'0008ui32,
	transmeta = 0x0000'0010ui32,
	nat_semi  = 0x0000'0020ui32,
	nexgen    = 0x0000'0040ui32,
	rise      = 0x0000'0080ui32,
	sis       = 0x0000'0100ui32,
	umc       = 0x0000'0200ui32,
	via       = 0x0000'0400ui32,
	vortex    = 0x0000'0800ui32,
	// hypervisors
	bhyve     = 0x0001'0000ui32,
	kvm       = 0x0002'0000ui32,
	hyper_v   = 0x0004'0000ui32,
	parallels = 0x0008'0000ui32,
	vmware    = 0x0010'0000ui32,
	xen_hvm   = 0x0020'0000ui32,
	// for filtering
	any       = 0xffff'ffffui32,
};

constexpr inline vendor_t operator|(const vendor_t& lhs, const vendor_t& rhs) {
	return static_cast<vendor_t>(static_cast<std::uint32_t>(lhs) | static_cast<std::uint32_t>(rhs));
}

using register_set_t = std::array<std::uint32_t, 4>;
using features_t = std::map<leaf_t, std::map<subleaf_t, register_set_t>>;


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
	leaf_t highest_leaf;
	leaf_t highest_extended_leaf;
	vendor_t vendor;
	model_t model;
	features_t features;
};

inline void cpuid(register_set_t& regs, leaf_t leaf, subleaf_t subleaf) noexcept {
	std::array<int, 4> raw_regs;
	__cpuidex(raw_regs.data(), gsl::narrow_cast<int>(leaf), gsl::narrow_cast<int>(subleaf));
	regs[eax] = gsl::narrow_cast<std::uint32_t>(raw_regs[eax]);
	regs[ebx] = gsl::narrow_cast<std::uint32_t>(raw_regs[ebx]);
	regs[ecx] = gsl::narrow_cast<std::uint32_t>(raw_regs[ecx]);
	regs[edx] = gsl::narrow_cast<std::uint32_t>(raw_regs[edx]);
}

#endif

#ifndef CPUID_HPP
#define CPUID_HPP

#include <cstddef>
#include <cstdint>
#include <array>
#include <map>

#include <gsl/gsl>
#include <fmt/format.h>


#if !defined(__cpp_lib_integer_literals)

namespace std
{
	inline namespace literals
	{
		inline namespace integer_literals
		{
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable: 4455) // warning 4455: literal suffix identifiers that do not start with an underscore are reserved
#endif
			constexpr uint_least64_t operator ""u64(unsigned long long arg) {
				return static_cast<uint_least64_t>(arg);
			}
			constexpr uint_least32_t operator ""u32(unsigned long long arg) {
				return static_cast<uint_least32_t>(arg);
			}
			constexpr uint_least16_t operator ""u16(unsigned long long arg) {
				return static_cast<uint_least16_t>(arg);
			}
			constexpr uint_least8_t operator ""u8(unsigned long long arg) {
				return static_cast<uint_least8_t>(arg);
			}

			constexpr int_least64_t operator ""i64(unsigned long long arg) {
				return static_cast<int_least64_t>(arg);
			}
			constexpr int_least32_t operator ""i32(unsigned long long arg) {
				return static_cast<int_least32_t>(arg);
			}
			constexpr int_least16_t operator ""i16(unsigned long long arg) {
				return static_cast<int_least16_t>(arg);
			}
			constexpr int_least8_t operator ""i8(unsigned long long arg) {
				return static_cast<int_least8_t>(arg);
			}
#if defined(_MSC_VER)
#pragma warning(pop)
#endif
		}
	}
}

#endif

using namespace std::integer_literals;

enum struct leaf_t : std::uint32_t
{
	basic_info                        = 0x0000'0000u32,
	version_info                      = 0x0000'0001u32,
	cache_and_tlb                     = 0x0000'0002u32,
	serial_number                     = 0x0000'0003u32,
	deterministic_cache               = 0x0000'0004u32,
	monitor_mwait                     = 0x0000'0005u32,
	thermal_and_power                 = 0x0000'0006u32,
	extended_features                 = 0x0000'0007u32,
	reserved_1                        = 0x0000'0008u32,
	direct_cache_access               = 0x0000'0009u32,
	performance_monitoring            = 0x0000'000au32,
	extended_topology                 = 0x0000'000bu32,
	reserved_2                        = 0x0000'000cu32,
	extended_state                    = 0x0000'000du32,
	reserved_3                        = 0x0000'000eu32,
	rdt_monitoring                    = 0x0000'000fu32,
	rdt_allocation                    = 0x0000'0010u32,
	reserved_4                        = 0x0000'0011u32,
	sgx_info                          = 0x0000'0012u32,
	reserved_5                        = 0x0000'0013u32,
	processor_trace                   = 0x0000'0014u32,
	time_stamp_counter                = 0x0000'0015u32,
	processor_frequency               = 0x0000'0016u32,
	system_on_chip_vendor             = 0x0000'0017u32,
	deterministic_tlb                 = 0x0000'0018u32,
	reserved_6                        = 0x0000'0019u32,
	reserved_7                        = 0x0000'001au32,
	pconfig                           = 0x0000'001bu32,

	hypervisor_limit                  = 0x4000'0000u32,

	hyper_v_signature                 = 0x4000'0001u32,
	hyper_v_system_identity           = 0x4000'0002u32,
	hyper_v_features                  = 0x4000'0003u32,
	hyper_v_enlightenment_recs        = 0x4000'0004u32,
	hyper_v_implementation_limits     = 0x4000'0005u32,
	hyper_v_implementation_hardware   = 0x4000'0006u32,
	hyper_v_root_cpu_management       = 0x4000'0007u32,
	hyper_v_shared_virtual_memory     = 0x4000'0008u32,
	hyper_v_nested_hypervisor         = 0x4000'0009u32,
	hyper_v_nested_features           = 0x4000'000au32,

	xen_limit                         = 0x4000'0000u32, xen_limit_offset                  = 0x4000'0100u32,
	xen_version                       = 0x4000'0001u32, xen_version_offset                = 0x4000'0101u32,
	xen_features                      = 0x4000'0002u32, xen_features_offset               = 0x4000'0102u32,
	xen_time                          = 0x4000'0003u32, xen_time_offset                   = 0x4000'0103u32,
	xen_hvm_features                  = 0x4000'0004u32, xen_hvm_features_offset           = 0x4000'0104u32,
	xen_pv_features                   = 0x4000'0005u32, xen_pv_features_offset            = 0x4000'0105u32,

	vmware_timing                     = 0x4000'0010u32,

	kvm_features                      = 0x4000'0001u32,

	extended_limit                    = 0x8000'0000u32,
	extended_signature_and_features   = 0x8000'0001u32,
	brand_string_0                    = 0x8000'0002u32,
	brand_string_1                    = 0x8000'0003u32,
	brand_string_2                    = 0x8000'0004u32,
	l1_cache_identifiers              = 0x8000'0005u32,
	l2_cache_identifiers              = 0x8000'0006u32,
	ras_advanced_power_management     = 0x8000'0007u32,
	address_limits                    = 0x8000'0008u32,
	reserved_8                        = 0x8000'0009u32,
	secure_virtual_machine            = 0x8000'000au32,

	extended_reserved_1               = 0x8000'000bu32,
	extended_reserved_2               = 0x8000'000cu32,
	extended_reserved_3               = 0x8000'000du32,
	extended_reserved_4               = 0x8000'000eu32,
	extended_reserved_5               = 0x8000'000fu32,
	extended_reserved_6               = 0x8000'0010u32,
	extended_reserved_7               = 0x8000'0011u32,
	extended_reserved_8               = 0x8000'0012u32,
	extended_reserved_9               = 0x8000'0013u32,
	extended_reserved_10              = 0x8000'0014u32,
	extended_reserved_11              = 0x8000'0015u32,
	extended_reserved_12              = 0x8000'0016u32,
	extended_reserved_13              = 0x8000'0017u32,
	extended_reserved_14              = 0x8000'0018u32,
	
	tlb_1g_identifiers                = 0x8000'0019u32,
	performance_optimization          = 0x8000'001au32,
	instruction_based_sampling        = 0x8000'001bu32,
	lightweight_profiling             = 0x8000'001cu32,
	cache_properties                  = 0x8000'001du32,
	extended_apic                     = 0x8000'001eu32,
	encrypted_memory                  = 0x8000'001fu32,

	none                              = 0x0000'0000u32,
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
	main                                   = 0x0000'0000u32,
	extended_features_main                 = 0x0000'0000u32,
	extended_state_main                    = 0x0000'0000u32,
	extended_state_sub                     = 0x0000'0001u32,
	rdt_monitoring_main                    = 0x0000'0000u32,
	rdt_monitoring_l3                      = 0x0000'0001u32,
	rdt_allocation_main                    = 0x0000'0000u32,
	rdt_cat_l3                             = 0x0000'0001u32,
	rdt_cat_l2                             = 0x0000'0002u32,
	rdt_mba                                = 0x0000'0003u32,
	sgx_capabilities                       = 0x0000'0000u32,
	sgx_attributes                         = 0x0000'0001u32,
	processor_trace_main                   = 0x0000'0000u32,
	processor_trace_sub                    = 0x0000'0001u32,
	system_on_chip_vendor_main             = 0x0000'0000u32,
	system_on_chip_vendor_sub              = 0x0000'0001u32,
	deterministic_address_translation_main = 0x0000'0000u32,
	deterministic_address_translation_sub  = 0x0000'0001u32,
	xen_time_main                          = 0x0000'0000u32,
	xen_time_tsc_offset                    = 0x0000'0001u32,
	xen_time_host                          = 0x0000'0002u32,
	none                                   = 0x0000'0000u32,
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
	unknown        = 0x0000'0000u32,
	// silicon
	amd            = 0x0000'0001u32,
	centaur        = 0x0000'0002u32,
	cyrix          = 0x0000'0004u32,
	intel          = 0x0000'0008u32,
	transmeta      = 0x0000'0010u32,
	nat_semi       = 0x0000'0020u32,
	nexgen         = 0x0000'0040u32,
	rise           = 0x0000'0080u32,
	sis            = 0x0000'0100u32,
	umc            = 0x0000'0200u32,
	via            = 0x0000'0400u32,
	vortex         = 0x0000'0800u32,
	// hypervisors
	bhyve          = 0x0001'0000u32,
	kvm            = 0x0002'0000u32,
	hyper_v        = 0x0004'0000u32,
	parallels      = 0x0008'0000u32,
	vmware         = 0x0010'0000u32,
	xen_hvm        = 0x0020'0000u32,
	qemu           = 0x0040'0000u32,
	// for filtering
	any_silicon    = 0x0000'0fffu32,
	any_hypervisor = 0x007f'0000u32,
	any            = 0xffff'ffffu32,
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
	default:
		hypervisor = "";
	}

	return hypervisor.size() != 0 ? hypervisor + " on " + silicon : silicon;
}

using register_set_t = std::array<std::uint32_t, 4>;
using subleaves_t    = std::map<subleaf_t, register_set_t>;
using leaves_t       = std::map<leaf_t, subleaves_t>;

vendor_t get_vendor_from_name(const register_set_t& regs);

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

inline bool operator==(const model_t& lhs, const model_t& rhs) noexcept {
	return lhs.stepping == rhs.stepping
	    && lhs.model    == rhs.model
	    && lhs.family   == rhs.family;
}

struct cpu_t
{
	std::uint32_t apic_id;
	vendor_t vendor;
	model_t model;
	leaves_t leaves;
};

inline bool operator==(const cpu_t& lhs, const cpu_t& rhs) noexcept {
	return lhs.apic_id                 == rhs.apic_id
	    && lhs.vendor                  == rhs.vendor
	    && lhs.model                   == rhs.model
	    && lhs.leaves                  == rhs.leaves;
}

inline register_set_t cpuid(leaf_t leaf, subleaf_t subleaf) noexcept {
	register_set_t regs = {};
	std::array<int, 4> raw_regs;
	__cpuidex(raw_regs.data(), gsl::narrow_cast<int>(leaf), gsl::narrow_cast<int>(subleaf));
	regs[eax] = gsl::narrow_cast<std::uint32_t>(raw_regs[eax]);
	regs[ebx] = gsl::narrow_cast<std::uint32_t>(raw_regs[ebx]);
	regs[ecx] = gsl::narrow_cast<std::uint32_t>(raw_regs[ecx]);
	regs[edx] = gsl::narrow_cast<std::uint32_t>(raw_regs[edx]);
	return regs;
}

void print_generic(fmt::memory_buffer& out, const cpu_t& cpu, leaf_t leaf, subleaf_t subleaf);

enum struct file_format
{
	native,
	etallen,
	libcpuid,
	instlat
};

std::map<std::uint32_t, cpu_t> enumerate_file(std::istream& fin, file_format format);
std::map<std::uint32_t, cpu_t> enumerate_processors(bool brute_force, bool skip_vendor_check, bool skip_feature_check);

void print_dump(fmt::memory_buffer& out, std::map<std::uint32_t, cpu_t> logical_cpus, file_format format);
void print_leaves(fmt::memory_buffer& out, const cpu_t& cpu, bool skip_vendor_check, bool skip_feature_check);

struct flag_spec_t
{
	std::uint32_t selector_eax  = 0u32;
	std::uint32_t selector_ecx  = 0u32;
	register_t    flag_register = eax;
	std::string   flag_name     = "";
	std::uint32_t flag_start    = 0xffff'ffffu32;
	std::uint32_t flag_end      = 0xffff'ffffu32;
};

void print_single_flag(fmt::memory_buffer& out, const cpu_t& cpu, const flag_spec_t& flag_description);
flag_spec_t parse_flag_spec(const std::string& flag_description);
std::string to_string(const flag_spec_t& spec);

inline bool operator==(const flag_spec_t& lhs, const flag_spec_t& rhs) noexcept {
	return std::tie(lhs.selector_eax, lhs.selector_ecx, lhs.flag_register, lhs.flag_name, lhs.flag_start, lhs.flag_end)
	    == std::tie(rhs.selector_eax, rhs.selector_ecx, rhs.flag_register, rhs.flag_name, rhs.flag_start, rhs.flag_end);
}

struct cache_instance_t
{
	std::vector<std::uint32_t> sharing_ids;
};

struct cache_t
{
	std::uint32_t level;
	std::uint32_t type;
	std::uint32_t ways;
	std::uint32_t sets;
	std::uint32_t line_size;
	std::uint32_t line_partitions;
	std::uint32_t total_size;
	bool fully_associative;
	bool direct_mapped;
	bool complex_addressed;
	bool self_initializing;
	bool invalidates_lower_levels;
	bool inclusive;
	std::uint32_t sharing_mask;

	std::map<std::uint32_t, cache_instance_t> instances;
};

inline bool operator<(const cache_t& lhs, const cache_t& rhs) noexcept {
	return lhs.level != rhs.level ? lhs.level      < rhs.level
	     : lhs.type  != rhs.type  ? lhs.type       < rhs.type
	     :                          lhs.total_size < rhs.total_size;
}

struct logical_core_t
{
	std::uint32_t full_apic_id;

	std::uint32_t package_id;
	std::uint32_t physical_core_id;
	std::uint32_t logical_core_id;

	std::vector<std::uint32_t> non_shared_cache_ids;
	std::vector<std::uint32_t> shared_cache_ids;
};

struct physical_core_t
{
	std::map<std::uint32_t, logical_core_t> logical_cores;
};

struct package_t
{
	std::map<std::uint32_t, physical_core_t> physical_cores;
};

struct system_t
{
	std::uint32_t logical_mask_width;
	std::uint32_t physical_mask_width;
	std::vector<std::uint32_t> x2_apic_ids;

	std::vector<cache_t> all_caches;
	std::vector<logical_core_t> all_cores;

	std::map<std::uint32_t, package_t> packages;
};

system_t build_topology(const std::map<std::uint32_t, cpu_t>& logical_cpus);

void print_topology(fmt::memory_buffer& out, const system_t& machine);

#endif

#include "stdafx.h"

#include <map>
#include <iostream>
#include <iomanip>

#include <gsl/gsl>

#include <fmt/format.h>

enum leaf_t : std::uint32_t
{
	basic_info                        = 0x0000'0000ui32,
	version_info                      = 0x0000'0001ui32,
	cache_and_tlb                     = 0x0000'0002ui32,
	serial_number                     = 0x0000'0003ui32,
	cache_parameters                  = 0x0000'0004ui32,
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
	resource_director_monitoring      = 0x0000'000fui32,
	cache_allocation_technology       = 0x0000'0010ui32,
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

leaf_t operator++(leaf_t& lhs) {
	lhs = static_cast<leaf_t>(static_cast<std::uint32_t>(lhs) + 1);
	return lhs;
}

enum subleaf_t : std::uint32_t
{
	zero                                    = 0x0000'0000ui32,
	extended_state_main                     = 0x0000'0000ui32,
	extended_state_sub                      = 0x0000'0001ui32,
	resource_director_monitoring_main       = 0x0000'0000ui32,
	resource_director_monitoring_capability = 0x0000'0001ui32,
	cache_allocation_technology_main        = 0x0000'0000ui32,
	cache_allocation_technology_l3          = 0x0000'0001ui32,
	cache_allocation_technology_l2          = 0x0000'0002ui32,
	cache_allocation_technology_bandwidth   = 0x0000'0003ui32,
	sgx_info_main                           = 0x0000'0000ui32,
	sgx_info_attributes                     = 0x0000'0001ui32,
	processor_trace_main                    = 0x0000'0000ui32,
	processor_trace_sub                     = 0x0000'0001ui32,
	system_on_chip_vendor_main              = 0x0000'0000ui32,
	system_on_chip_vendor_sub               = 0x0000'0001ui32,
	deterministic_address_translation_main  = 0x0000'0000ui32,
	deterministic_address_translation_sub   = 0x0000'0001ui32,
};

subleaf_t operator++(subleaf_t& lhs) {
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

register_t operator++(register_t& lhs) {
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

vendor_t operator|(const vendor_t& lhs, const vendor_t& rhs) {
	return static_cast<vendor_t>(static_cast<std::uint32_t>(lhs) | static_cast<std::uint32_t>(rhs));
}

using register_set_t = std::array<std::uint32_t, 4>;
using features_t = std::map<leaf_t, std::map<subleaf_t, register_set_t>>;

struct model_t
{
	std::uint32_t stepping;
	std::uint32_t model;
	std::uint32_t family;
};

struct cpu_t
{
	std::uint32_t highest_leaf;
	std::uint32_t highest_extended_leaf;
	vendor_t vendor;
	model_t model;
	features_t features;
};

void cpuid(std::array<std::uint32_t, 4>& regs, std::uint32_t lf, std::uint32_t sublf) {
	std::array<int, 4> raw_regs;
	__cpuidex(raw_regs.data(), gsl::narrow_cast<int>(lf), gsl::narrow_cast<int>(sublf));
	regs[eax] = gsl::narrow_cast<std::uint32_t>(raw_regs[eax]);
	regs[ebx] = gsl::narrow_cast<std::uint32_t>(raw_regs[ebx]);
	regs[ecx] = gsl::narrow_cast<std::uint32_t>(raw_regs[ecx]);
	regs[edx] = gsl::narrow_cast<std::uint32_t>(raw_regs[edx]);
}

using leaf_print = void(*)(const cpu_t& cpu);
using leaf_enumerate = void(*)(cpu_t& cpu);

template<std::size_t N, std::size_t... Is>
constexpr std::array<char, N - 1> to_array(const char(&str)[N], std::index_sequence<Is...>) {
	return { str[Is]... };
}

template<std::size_t N>
constexpr std::array<char, N - 1> to_array(const char(&str)[N]) {
	return to_array(str, std::make_index_sequence<N - 1>{});
}

vendor_t get_vendor_from_name(const register_set_t& regs) {
	static const std::map<std::array<char, 12>, vendor_t> vendors = {
		{ to_array("AMDisbetter!"), amd },
		{ to_array("AuthenticAMD"), amd },
		{ to_array("CentaurHauls"), centaur },
		{ to_array("CyrixInstead"), cyrix },
		{ to_array("GenuineIntel"), intel },
		{ to_array("TransmetaCPU"), transmeta },
		{ to_array("GenuineTMx86"), transmeta },
		{ to_array("Geode by NSC"), nat_semi },
		{ to_array("NexGenDriven"), nexgen },
		{ to_array("RiseRiseRise"), rise },
		{ to_array("SiS SiS SiS "), sis },
		{ to_array("UMC UMC UMC "), umc },
		{ to_array("VIA VIA VIA "), via },
		{ to_array("Vortex86 SoC"), vortex },
		{ to_array("bhyve byhve "), bhyve },
		{ to_array("KVMKVMKVM\0\0\0"), kvm },
		{ to_array("Microsoft hv"), hyper_v },
		{ to_array(" lrpepyh vr\0"), parallels },
		{ to_array("VMwareVMware"), vmware },
		{ to_array("XenVMMXenVMM"), xen_hvm }
	};
	union
	{
		std::array<char, 12> vndr;
		std::array<std::int32_t, 3> registers;
	} data;
	data.registers[0] = regs[ebx];
	data.registers[1] = regs[edx];
	data.registers[2] = regs[ecx];

	auto it = vendors.find(data.vndr);
	return it != vendors.end() ? it->second : unknown;
}

struct split_model_t
{
	std::uint32_t stepping : 4;
	std::uint32_t model : 4;
	std::uint32_t family : 4;
	std::uint32_t type : 2;
	std::uint32_t reserved_1 : 2;
	std::uint32_t extended_model : 4;
	std::uint32_t extended_family : 8;
	std::uint32_t reserved_2 : 4;
};

model_t get_model(vendor_t vendor, const register_set_t& regs) {
	union
	{
		std::uint32_t raw;
		split_model_t m;
	} split_model;

	split_model.raw = regs[eax];

	model_t model = {};
	model.family = split_model.m.family;
	model.model = split_model.m.model;
	model.stepping = split_model.m.stepping;
	switch(vendor) {
	case intel:
		{
			if(split_model.m.family == 0xf) {
				model.family += split_model.m.extended_family;
			}
			if(split_model.m.family == 0x6 || split_model.m.family == 0xf) {
				model.model += (split_model.m.extended_model << 4ui32);
			}
		}
		break;
	case amd:
		{
			model.family += split_model.m.extended_family;
			model.model += split_model.m.extended_model << 4ui32;
		}
		break;
	default:
		break;
	}
	return model;
}

struct feature_t
{
	vendor_t vendor;
	std::uint32_t mask;
	const char* mnemonic;
	const char* description;
};

using feature_map_t = std::map<leaf_t, std::map<subleaf_t, std::map<register_t, std::vector<feature_t>>>>;

const feature_map_t all_features = {
	{ version_info, {
		{ zero, {
			{ ecx, {
				{ intel | amd            , 0x0000'0001ui32, "SSE3"        , "SSE3 Extensions" },
				{ intel | amd            , 0x0000'0002ui32, "PCLMULQDQ"   , "Carryless Multiplication" },
				{ intel                  , 0x0000'0004ui32, "DTES64"      , "64-bit DS Area" },
				{ intel | amd            , 0x0000'0008ui32, "MONITOR"     , "MONITOR/MWAIT" },
				{ intel                  , 0x0000'0010ui32, "DS-CPL"      , "CPL Qualified Debug Store" },
				{ intel                  , 0x0000'0020ui32, "VMX"         , "Virtual Machine Extensions" },
				{ intel                  , 0x0000'0040ui32, "SMX"         , "Safer Mode Extensions" },
				{ intel                  , 0x0000'0080ui32, "EIST"        , "Enhanced Intel SpeedStep Technology" },
				{ intel                  , 0x0000'0100ui32, "TM2"         , "Thermal Monitor 2" },
				{ intel | amd            , 0x0000'0200ui32, "SSSE3"       , "SSSE3 Extensions" },
				{ intel                  , 0x0000'0400ui32, "CNXT-ID"     , "L1 Context ID" },
				{ intel                  , 0x0000'0800ui32, "SDBG"        , "IA32_DEBUG_INTERFACE for silicon debug" },
				{ intel | amd            , 0x0000'1000ui32, "FMA"         , "Fused Multiply Add" },
				{ intel | amd            , 0x0000'2000ui32, "CMPXCHG16B"  , "CMPXCHG16B instruction" },
				{ intel                  , 0x0000'4000ui32, "xTPR"        , "xTPR update control" },
				{ intel                  , 0x0000'8000ui32, "PDCM"        , "Perfmon and Debug Capability" },
				{ intel | amd            , 0x0001'0000ui32, ""            , "Reserved"},
				{ intel | amd            , 0x0002'0000ui32, "PCID"        , "Process-context identifiers" },
				{ intel                  , 0x0004'0000ui32, "DCA"         , "Direct Cache Access" },
				{ intel | amd            , 0x0008'0000ui32, "SSE4.1"      , "SSE4.1 Extensions" },
				{ intel | amd            , 0x0010'0000ui32, "SSE4.2"      , "SSE4.2 Extensions" },
				{ intel | amd            , 0x0020'0000ui32, "x2APIC"      , "x2APIC" },
				{ intel | amd            , 0x0040'0000ui32, "MOVBE"       , "MOVBE instruction" },
				{ intel | amd            , 0x0080'0000ui32, "POPCNT"      , "POPCNT instruction" },
				{ intel | amd            , 0x0100'0000ui32, "TSC-Deadline", "One-shot APIC timers using a TSC deadline" },
				{ intel | amd            , 0x0200'0000ui32, "AESNI"       , "AESNI instruction extensions" },
				{ intel | amd            , 0x0400'0000ui32, "XSAVE"       , "XSAVE/XRSTOR feature" },
				{ intel | amd            , 0x0800'0000ui32, "OSXSAVE"     , "OS has set CR4.OSXSAVE" },
				{ intel | amd            , 0x1000'0000ui32, "AVX"         , "AVX instructions" },
				{ intel | amd            , 0x2000'0000ui32, "F16C"        , "16-bit floating-point conversion instructions" },
				{ intel | amd            , 0x4000'0000ui32, "RDRAND"      , "RDRAND instruction" },
				{ any                    , 0x8000'0000ui32, "(hypervisor)", "Hypervisor guest" }
			}},
			{ edx, {
				{ intel | amd | transmeta, 0x0000'0001ui32, "FPU"         , "x87 FPU on chip"},
				{ intel | amd | transmeta, 0x0000'0002ui32, "VME"         , "Virtual 8086 Mode Enhancements"},
				{ intel | amd | transmeta, 0x0000'0004ui32, "DE"          , "Debugging Extensions"},
				{ intel | amd | transmeta, 0x0000'0008ui32, "PSE"         , "Page Size Extension"},
				{ intel | amd | transmeta, 0x0000'0010ui32, "TSC"         , "Time Stamp Counter"},
				{ intel | amd | transmeta, 0x0000'0020ui32, "MSR"         , "RDMSR and WRMSR Instructions"},
				{ intel | amd            , 0x0000'0040ui32, "PAE"         , "Physical Address Extension"},
				{ intel | amd            , 0x0000'0080ui32, "MCE"         , "Machine Check Exception"},
				{ intel | amd | transmeta, 0x0000'0100ui32, "CX8"         , "CMPXCHG8B Instruction"},
				{ intel | amd            , 0x0000'0200ui32, "APIC"        , "APIC On-Chip"},
				{ intel | amd            , 0x0000'0400ui32, ""            , "Reserved"},
				{ intel | amd | transmeta, 0x0000'0800ui32, "SEP"         , "SYSENTER and SYSEXIT Instructions"},
				{ intel | amd            , 0x0000'1000ui32, "MTRR"        , "Memory Type Range Registers"},
				{ intel | amd            , 0x0000'2000ui32, "PGE"         , "Page Global Bit"},
				{ intel | amd            , 0x0000'4000ui32, "MCA"         , "Machine Check Architecture"},
				{ intel | amd | transmeta, 0x0000'8000ui32, "CMOV"        , "Conditional Move Instructions"},
				{ intel | amd            , 0x0001'0000ui32, "PAT"         , "Page Attribute Table"},
				{ intel | amd            , 0x0002'0000ui32, "PSE-36"      , "36-bit Page Size Extension"},
				{ intel       | transmeta, 0x0004'0000ui32, "PSN"         , "Processor Serial Number"},
				{ intel | amd            , 0x0008'0000ui32, "CLFSH"       , "CLFLUSH Instruction"},
				{ intel | amd            , 0x0010'0000ui32, ""            , "Reserved"},
				{ intel                  , 0x0020'0000ui32, "DS"          , "Debug Store"},
				{ intel                  , 0x0040'0000ui32, "ACPI"        , "Thermal Monitoring and Software Controlled Clock Facilities"},
				{ intel | amd | transmeta, 0x0080'0000ui32, "MMX"         , "Intel MMX Technology"},
				{ intel | amd            , 0x0100'0000ui32, "FXSR"        , "FXSAVE and FXRSTOR Instructions"},
				{ intel | amd            , 0x0200'0000ui32, "SSE"         , "SSE Extensions"},
				{ intel | amd            , 0x0400'0000ui32, "SSE2"        , "SSE2 Extensions"},
				{ intel                  , 0x0800'0000ui32, "SS"          , "Self Snoop"},
				{ intel | amd            , 0x1000'0000ui32, "HTT"         , "Max APIC IDs reserved field is Valid"},
				{ intel                  , 0x2000'0000ui32, "TM"          , "Thermal Monitor"},
				{ intel                  , 0x4000'0000ui32, "IA64"        , "IA64 emulating x86"},
				{ intel                  , 0x8000'0000ui32, "PBE"         , "Pending Break Enable"}
			}}
		}}
	}}
};

void print_features(leaf_t leaf, subleaf_t sub, register_t reg, const cpu_t& cpu) {
	const std::vector<feature_t>& features = all_features.at(leaf).at(sub).at(reg);
	const std::uint32_t value              = cpu.features.at(leaf).at(sub).at(reg);

	for(const feature_t& f : features) {
		if(0 != (cpu.vendor & f.vendor)) {
			if(0 != (value & f.mask)) {
				std::cout << "\x1b[32;1m[+]\x1b[0m" << std::setw(16) << std::setfill(' ') << f.mnemonic << " " << f.description << "\n";
			} else {
				std::cout << "\x1b[31;1m[-]\x1b[0m" << std::setw(16) << std::setfill(' ') << f.mnemonic << " " << f.description << "\n";
			}
		}
	}
	std::cout << std::flush;
}

void print_basic_info(const cpu_t& cpu) {
	const register_set_t& regs = cpu.features.at(basic_info).at(subleaf_t::zero);
	union
	{
		std::array<char, 12> vndr;
		std::array<std::uint32_t, 3> registers;
	} data;
	data.registers[0] = regs[ebx];
	data.registers[1] = regs[edx];
	data.registers[2] = regs[ecx];

	std::cout << "   vendor: ";
	std::cout.write(data.vndr.data(), data.vndr.size());
	std::cout << "\n";
}

void print_version_info(const cpu_t& cpu) {
	const register_set_t& regs = cpu.features.at(version_info).at(subleaf_t::zero);

	union
	{
		std::uint32_t raw;
		split_model_t m;
	} model;
	
	model.raw = regs[eax];
	std::cout << "signature: 0x" << std::setw(8) << std::setfill('0') << std::hex << regs[eax] << "\n"
	          << "   family: 0x" << std::setw(2) << std::setfill('0') << std::hex << cpu.model.family << "\n"
	          << "    model: 0x" << std::setw(2) << std::setfill('0') << std::hex << cpu.model.model << "\n"
	          << " stepping: 0x" << std::setw(2) << std::setfill('0') << std::hex << cpu.model.stepping << "\n";
	if(cpu.vendor == intel) {
		switch(model.m.type) {
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

	struct additional_t
	{
		std::uint8_t brand_id;
		std::uint8_t cache_line_size;
		std::uint8_t maximum_addressable_ids;
		std::uint8_t local_apic_id;
	};

	union
	{
		std::uint32_t raw;
		additional_t a;
	} additional;
	additional.raw = regs[ebx];

	if(cpu.vendor == intel) {
		if(additional.a.brand_id != 0ui32) {
			std::cout << "brand ID: ";
			switch(additional.a.brand_id) {
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
	std::cout << "cache line size/bytes: " << std::dec << additional.a.cache_line_size * 8 << "\n"
	          << "logical processors per package: " << gsl::narrow_cast<std::uint32_t>(additional.a.maximum_addressable_ids) << "\n"
	          << "local APIC ID: " << gsl::narrow_cast<std::uint32_t>(additional.a.local_apic_id) << "\n";
	std::cout << std::endl;

	std::cout << "Feature identifiers\n";
	print_features(leaf_t::version_info, subleaf_t::zero, edx, cpu);
	std::cout << "\n";
	print_features(leaf_t::version_info, subleaf_t::zero, ecx, cpu);
	std::cout << std::endl;
}

enum cache_type_t : std::uint8_t
{
	data_tlb        = 0x01ui8,
	instruction_tlb = 0x02ui8,
	unified_tlb     = 0x04ui8,
	all_tlb         = 0x07ui8,
	data            = 0x10ui8,
	instructions    = 0x20ui8,
	unified         = 0x40ui8,
	all_cache       = 0x70ui8,
	trace           = 0x80ui8,
	invalid_type    = 0xffui8,
};

std::string to_string(cache_type_t type) {
	switch(type) {
	case data_tlb:
		return "Data TLB";
	case instruction_tlb:
		return "Instruction TLB";
	case unified_tlb:
		return "Shared TLB";
	case data:
		return "Data cache";
	case instructions:
		return "Instruction cache";
	case unified:
		return "Unified cache";
	case trace:
		return "Trace cache";
	}
}

enum cache_level_t : std::uint8_t
{
	no_cache,
	level_0,
	level_1,
	level_2,
	level_3,
	invalid_level = 0xffui8
};

std::string to_string(cache_level_t level) {
	switch(level) {
	case no_cache:
		return "";
	case level_0:
		return "0th-level";
	case level_1:
		return "1st-level";
	case level_2:
		return "2nd-level";
	case level_3:
		return "3rd-level";
	}
}

enum cache_attributes_t : std::uint8_t
{
	no_attributes      = 0x00ui8,
	pages_4k           = 0x01ui8,
	pages_2m           = 0x02ui8,
	pages_4m           = 0x04ui8,
	pages_1g           = 0x08ui8,
	all_page_sizes     = 0x0fui8,
	sectored_two_lines = 0x10ui8,
};

std::string to_string(cache_attributes_t attrs) {
	fmt::MemoryWriter m;
	std::uint8_t page = attrs & all_page_sizes;
	std::uint8_t mask = 1ui8;
	bool needs_separator = false;
	do {
		switch(attrs & mask) {
		case 0:
			m <<  "";
			break;
		case pages_4k:
			if(needs_separator) {
				m << " | ";
			}
			m <<  "4 KByte pages";
			needs_separator = true;
			break;
		case pages_2m:
			if(needs_separator) {
				m << " | ";
			}
			m << "2 MByte pages";
			needs_separator = true;
			break;
		case pages_4m:
			if(needs_separator) {
				m << " | ";
			}
			m << "4 MByte pages";
			needs_separator = true;
			break;
		case pages_1g:
			if(needs_separator) {
				m << " | ";
			}
			m << "1 GByte pages";
			needs_separator = true;
			break;
		}
		mask <<= 1ui8;
	} while(mask & all_page_sizes);
	return m.str();
}

cache_attributes_t operator|(const cache_attributes_t& lhs, const cache_attributes_t& rhs) {
	return static_cast<cache_attributes_t>(static_cast<std::uint32_t>(lhs) | static_cast<std::uint32_t>(rhs));
}

enum cache_associativity_t : std::uint8_t 
{
	unknown_associativity,
	direct_mapped,
	fully_associative = 0xffui8
};

std::string to_string(cache_associativity_t assoc) {
	using namespace fmt::literals;

	switch(assoc) {
	case 0:
		return "unknown associativity";
	case 1:
		return "direct mapped";
	case 0xffui8:
		return "fully associative";
	default:
		return "{0:d}-way set associative"_format(assoc);
	}
}

struct cache_descriptor_t
{
	cache_type_t type;
	cache_level_t level;
	std::uint32_t size;
	std::uint32_t entries;
	cache_attributes_t attributes;
	cache_associativity_t associativity;
	std::uint32_t line_size;
};

using cache_descriptor_map_t = std::map<std::uint8_t, cache_descriptor_t>;

#define KB * 1024
#define MB KB KB

const cache_descriptor_map_t standard_cache_descriptors = {
	{ 0x01, { instruction_tlb, no_cache, 0      , 32  , pages_4k                      , cache_associativity_t{0x04}, 0  } },
	{ 0x02, { instruction_tlb, no_cache, 0      , 2   , pages_4m                      , fully_associative          , 0  } },
	{ 0x03, { data_tlb       , no_cache, 0      , 64  , pages_4k                      , cache_associativity_t{0x04}, 0  } },
	{ 0x04, { data_tlb       , no_cache, 0      , 8   , pages_4m                      , cache_associativity_t{0x04}, 0  } },
	{ 0x05, { data_tlb       , no_cache, 0      , 32  , pages_4m                      , cache_associativity_t{0x04}, 0  } },
	{ 0x06, { instructions   , level_1 , 8    KB, 0   , no_attributes                 , cache_associativity_t{0x04}, 32 } },
	{ 0x08, { instructions   , level_1 , 16   KB, 0   , no_attributes                 , cache_associativity_t{0x04}, 32 } },
	{ 0x09, { instructions   , level_1 , 32   KB, 0   , no_attributes                 , cache_associativity_t{0x04}, 64 } },
	{ 0x0a, { data           , level_1 , 8    KB, 0   , no_attributes                 , cache_associativity_t{0x02}, 32 } },
	{ 0x0b, { instruction_tlb, level_1 , 0      , 4   , pages_4m                      , cache_associativity_t{0x04}, 0  } },
	{ 0x0c, { data           , level_1 , 16   KB, 0   , no_attributes                 , cache_associativity_t{0x04}, 32 } },
	{ 0x0d, { data           , level_1 , 16   KB, 0   , no_attributes                 , cache_associativity_t{0x04}, 64 } },
	{ 0x0e, { data           , level_1 , 24   KB, 0   , no_attributes                 , cache_associativity_t{0x06}, 64 } },
	{ 0x1d, { unified        , level_2 , 128  KB, 0   , no_attributes                 , cache_associativity_t{0x02}, 64 } },
	{ 0x21, { unified        , level_2 , 256  KB, 0   , no_attributes                 , cache_associativity_t{0x08}, 64 } },
	{ 0x22, { unified        , level_3 , 512  KB, 0   , sectored_two_lines            , cache_associativity_t{0x04}, 64 } },
	{ 0x23, { unified        , level_3 , 1    MB, 0   , sectored_two_lines            , cache_associativity_t{0x08}, 64 } },
	{ 0x24, { unified        , level_2 , 1    MB, 0   , sectored_two_lines            , cache_associativity_t{0x10}, 64 } },
	{ 0x25, { unified        , level_3 , 2    MB, 0   , sectored_two_lines            , cache_associativity_t{0x08}, 64 } },
	{ 0x29, { unified        , level_3 , 4    MB, 0   , sectored_two_lines            , cache_associativity_t{0x08}, 64 } },
	{ 0x2c, { data           , level_1 , 32   KB, 0   , no_attributes                 , cache_associativity_t{0x08}, 64 } },
	{ 0x30, { instructions   , level_1 , 32   KB, 0   , no_attributes                 , cache_associativity_t{0x08}, 64 } },
	{ 0x41, { unified        , level_2 , 128  KB, 0   , no_attributes                 , cache_associativity_t{0x04}, 32 } },
	{ 0x42, { unified        , level_2 , 256  KB, 0   , no_attributes                 , cache_associativity_t{0x04}, 32 } },
	{ 0x43, { unified        , level_2 , 512  KB, 0   , no_attributes                 , cache_associativity_t{0x04}, 32 } },
	{ 0x44, { unified        , level_2 , 1    MB, 0   , no_attributes                 , cache_associativity_t{0x04}, 32 } },
	{ 0x45, { unified        , level_2 , 2    MB, 0   , no_attributes                 , cache_associativity_t{0x04}, 32 } },
	{ 0x46, { unified        , level_3 , 4    MB, 0   , no_attributes                 , cache_associativity_t{0x04}, 64 } },
	{ 0x47, { unified        , level_3 , 8    MB, 0   , no_attributes                 , cache_associativity_t{0x08}, 64 } },
	{ 0x48, { unified        , level_2 , 3    MB, 0   , no_attributes                 , cache_associativity_t{0x0c}, 64 } },
	{ 0x4a, { unified        , level_3 , 6    MB, 0   , no_attributes                 , cache_associativity_t{0x0c}, 64 } },
	{ 0x4b, { unified        , level_3 , 8    MB, 0   , no_attributes                 , cache_associativity_t{0x10}, 64 } },
	{ 0x4c, { unified        , level_3 , 12   MB, 0   , no_attributes                 , cache_associativity_t{0x0c}, 64 } },
	{ 0x4d, { unified        , level_3 , 16   MB, 0   , no_attributes                 , cache_associativity_t{0x10}, 64 } },
	{ 0x4e, { unified        , level_2 , 6    MB, 0   , no_attributes                 , cache_associativity_t{0x18}, 64 } },
	{ 0x4f, { instruction_tlb, no_cache, 0      , 32  , pages_4k                      , unknown_associativity      , 0  } },
	{ 0x50, { instruction_tlb, no_cache, 0      , 64  , pages_4k | pages_2m | pages_4m, unknown_associativity      , 0  } },
	{ 0x51, { instruction_tlb, no_cache, 0      , 128 , pages_4k | pages_2m | pages_4m, unknown_associativity      , 0  } },
	{ 0x52, { instruction_tlb, no_cache, 0      , 256 , pages_4k | pages_2m | pages_4m, unknown_associativity      , 0  } },
	{ 0x55, { instruction_tlb, no_cache, 0      , 7   , pages_2m | pages_4m           , fully_associative          , 0  } },
	{ 0x56, { data_tlb       , level_0 , 0      , 16  , pages_4m                      , cache_associativity_t{0x04}, 0  } },
	{ 0x57, { data_tlb       , level_0 , 0      , 16  , pages_4k                      , cache_associativity_t{0x04}, 0  } },
	{ 0x59, { data_tlb       , level_0 , 0      , 16  , pages_4k                      , fully_associative          , 0  } },
	{ 0x5a, { data_tlb       , no_cache, 0      , 32  , pages_2m | pages_4m           , cache_associativity_t{0x04}, 0  } },
	{ 0x5b, { data_tlb       , no_cache, 0      , 64  , pages_4k | pages_4m           , fully_associative          , 0  } },
	{ 0x5c, { data_tlb       , no_cache, 0      , 128 , pages_4k | pages_4m           , fully_associative          , 0  } },
	{ 0x5d, { data_tlb       , no_cache, 0      , 256 , pages_4k | pages_4m           , fully_associative          , 0  } },
	{ 0x60, { data           , level_1 , 16   KB, 0   , sectored_two_lines            , cache_associativity_t{0x08}, 64 } },
	{ 0x61, { instruction_tlb, no_cache, 0      , 48  , pages_4k                      , fully_associative          , 0  } },
	{ 0x64, { data_tlb       , no_cache, 0      , 512 , pages_4k                      , cache_associativity_t{0x04}, 0  } },
	{ 0x66, { data           , level_1 , 8    KB, 0   , sectored_two_lines            , cache_associativity_t{0x04}, 64 } },
	{ 0x67, { data           , level_1 , 16   KB, 0   , sectored_two_lines            , cache_associativity_t{0x04}, 64 } },
	{ 0x68, { data           , level_1 , 32   KB, 0   , sectored_two_lines            , cache_associativity_t{0x04}, 64 } },
	{ 0x6a, { instruction_tlb, no_cache, 0      , 64  , pages_4k                      , cache_associativity_t{0x08}, 0  } },
	{ 0x6b, { data_tlb       , no_cache, 0      , 256 , pages_4k                      , cache_associativity_t{0x08}, 0  } },
	{ 0x6c, { data_tlb       , no_cache, 0      , 128 , pages_2m | pages_4m           , cache_associativity_t{0x08}, 0  } },
	{ 0x6d, { data_tlb       , no_cache, 0      , 16  , pages_1g                      , fully_associative          , 0  } },
	{ 0x70, { trace          , level_1 , 12   KB, 0   , no_attributes                 , cache_associativity_t{0x08}, 0  } },
	{ 0x71, { trace          , level_1 , 16   KB, 0   , no_attributes                 , cache_associativity_t{0x08}, 0  } },
	{ 0x72, { trace          , level_1 , 32   KB, 0   , no_attributes                 , cache_associativity_t{0x08}, 0  } },
	{ 0x76, { instruction_tlb, no_cache, 0      , 8   , pages_2m | pages_4m           , fully_associative          , 0  } },
	{ 0x78, { unified        , level_2 , 1    MB, 0   , no_attributes                 , cache_associativity_t{0x04}, 64 } },
	{ 0x79, { unified        , level_2 , 128  KB, 0   , sectored_two_lines            , cache_associativity_t{0x08}, 64 } },
	{ 0x7a, { unified        , level_2 , 256  KB, 0   , sectored_two_lines            , cache_associativity_t{0x04}, 64 } },
	{ 0x7b, { unified        , level_2 , 512  KB, 0   , sectored_two_lines            , cache_associativity_t{0x04}, 64 } },
	{ 0x7c, { unified        , level_2 , 1    MB, 0   , sectored_two_lines            , cache_associativity_t{0x04}, 64 } },
	{ 0x7d, { unified        , level_2 , 2    MB, 0   , no_attributes                 , cache_associativity_t{0x08}, 64 } },
	{ 0x7f, { unified        , level_2 , 512  KB, 0   , no_attributes                 , cache_associativity_t{0x02}, 64 } },
	{ 0x80, { unified        , level_2 , 512  KB, 0   , no_attributes                 , cache_associativity_t{0x08}, 64 } },
	{ 0x82, { unified        , level_2 , 256  KB, 0   , no_attributes                 , cache_associativity_t{0x08}, 32 } },
	{ 0x83, { unified        , level_2 , 512  KB, 0   , no_attributes                 , cache_associativity_t{0x08}, 32 } },
	{ 0x84, { unified        , level_2 , 1    MB, 0   , no_attributes                 , cache_associativity_t{0x08}, 32 } },
	{ 0x85, { unified        , level_2 , 2    MB, 0   , no_attributes                 , cache_associativity_t{0x08}, 32 } },
	{ 0x86, { unified        , level_2 , 512  KB, 0   , no_attributes                 , cache_associativity_t{0x04}, 64 } },
	{ 0x87, { unified        , level_2 , 1    MB, 0   , no_attributes                 , cache_associativity_t{0x08}, 64 } },
	{ 0xa0, { data_tlb       , no_cache, 0      , 32  , pages_4k                      , fully_associative          , 0  } },
	{ 0xb0, { instruction_tlb, no_cache, 0      , 128 , pages_4k                      , cache_associativity_t{0x04}, 0  } },
	{ 0xb1, { instruction_tlb, no_cache, 0      , 4   , pages_2m                      , cache_associativity_t{0x04}, 0  } },
	{ 0xb2, { data_tlb       , no_cache, 0      , 64  , pages_4k                      , cache_associativity_t{0x04}, 0  } },
	{ 0xb3, { data_tlb       , no_cache, 0      , 128 , pages_4k                      , cache_associativity_t{0x04}, 0  } },
	{ 0xb4, { data_tlb       , level_1 , 0      , 256 , pages_4k                      , cache_associativity_t{0x04}, 0  } },
	{ 0xb5, { instruction_tlb, no_cache, 0      , 64  , pages_4k                      , cache_associativity_t{0x08}, 0  } },
	{ 0xb6, { instruction_tlb, no_cache, 0      , 128 , pages_4k                      , cache_associativity_t{0x08}, 0  } },
	{ 0xba, { data_tlb       , level_1 , 0      , 64  , pages_4k                      , cache_associativity_t{0x04}, 0  } },
	{ 0xc0, { data_tlb       , no_cache, 0      , 8   , pages_4k | pages_4m           , cache_associativity_t{0x04}, 0  } },
	{ 0xc1, { unified_tlb    , level_2 , 0      , 1024, pages_4k | pages_2m           , cache_associativity_t{0x08}, 0  } },
	{ 0xc2, { data_tlb       , level_1 , 0      , 16  , pages_4k | pages_2m           , cache_associativity_t{0x04}, 0  } },
	{ 0xc2, { data_tlb       , level_1 , 0      , 32  , pages_2m | pages_4m           , cache_associativity_t{0x04}, 0  } },
	{ 0xca, { unified_tlb    , level_2 , 0      , 512 , pages_4k                      , cache_associativity_t{0x04}, 0  } },
	{ 0xd0, { unified        , level_3 , 512  KB, 0   , no_attributes                 , cache_associativity_t{0x04}, 64 } },
	{ 0xd1, { unified        , level_3 , 1    MB, 0   , no_attributes                 , cache_associativity_t{0x04}, 64 } },
	{ 0xd2, { unified        , level_3 , 2    MB, 0   , no_attributes                 , cache_associativity_t{0x04}, 64 } },
	{ 0xd6, { unified        , level_3 , 1    MB, 0   , no_attributes                 , cache_associativity_t{0x08}, 64 } },
	{ 0xd7, { unified        , level_3 , 2    MB, 0   , no_attributes                 , cache_associativity_t{0x08}, 64 } },
	{ 0xd8, { unified        , level_3 , 4    MB, 0   , no_attributes                 , cache_associativity_t{0x08}, 64 } },
	{ 0xdc, { unified        , level_3 , 1536 KB, 0   , no_attributes                 , cache_associativity_t{0x0c}, 64 } },
	{ 0xdd, { unified        , level_3 , 3    MB, 0   , no_attributes                 , cache_associativity_t{0x0c}, 64 } },
	{ 0xde, { unified        , level_3 , 6    MB, 0   , no_attributes                 , cache_associativity_t{0x0c}, 64 } },
	{ 0xe2, { unified        , level_3 , 2    MB, 0   , no_attributes                 , cache_associativity_t{0x10}, 64 } },
	{ 0xe3, { unified        , level_3 , 4    MB, 0   , no_attributes                 , cache_associativity_t{0x10}, 64 } },
	{ 0xe4, { unified        , level_3 , 8    MB, 0   , no_attributes                 , cache_associativity_t{0x10}, 64 } },
	{ 0xea, { unified        , level_3 , 12   MB, 0   , no_attributes                 , cache_associativity_t{0x18}, 64 } },
	{ 0xeb, { unified        , level_3 , 18   MB, 0   , no_attributes                 , cache_associativity_t{0x18}, 64 } },
	{ 0xec, { unified        , level_3 , 24   MB, 0   , no_attributes                 , cache_associativity_t{0x18}, 64 } }
};

using dual_descriptor_t = std::pair<cache_descriptor_t, cache_descriptor_t>;
using dual_descriptors_map_t = std::map<std::uint8_t, dual_descriptor_t>;

const dual_descriptors_map_t dual_cache_descriptors = {
	{ 0x49, {
		{ unified    , level_3 , 4    MB, 0   , no_attributes                 , static_cast<cache_associativity_t>(0x10), 64 },
		{ unified    , level_2 , 4    MB, 0   , no_attributes                 , static_cast<cache_associativity_t>(0x10), 64 }
	} },
	{ 0x63, {
		{ data_tlb   , no_cache, 0      , 32  , pages_2m | pages_4m           , static_cast<cache_associativity_t>(0x04), 0 },
		{ data_tlb   , no_cache, 0      , 4   , pages_1g                      , static_cast<cache_associativity_t>(0x04), 0 }
	} },
	{ 0xc3, {
		{ unified_tlb, level_2 , 0      , 1536, pages_4k | pages_2m           , static_cast<cache_associativity_t>(0x06), 0 },
		{ unified_tlb, level_2 , 0      , 16  , pages_1g                      , static_cast<cache_associativity_t>(0x04), 0 }
	} }
};

#undef MB
#undef KB

std::string to_string(cache_descriptor_t desc) {
	fmt::MemoryWriter w;
	w << to_string(desc.level);
	if(w.size() > 0) {
		w << " ";
	}
	w << to_string(desc.type) << ": ";
	if(desc.type & all_cache) {
		std::uint32_t size = desc.size / 1'024;
		if(size > 1'024) {
			if(size == 1'536) {
				w << "1.5 Mbytes";
			} else {
				size /= 1'024;
				w << size << " Mbytes";
			}
		} else {
			w << size << " Kbytes";
		}
	} else if(desc.type & all_tlb) {
		w << to_string(desc.attributes);
	} else if(desc.type & trace) {
		w << (desc.size / 1'024) << " K-\u00b5op";
	}
	w << ", ";
	w << to_string(desc.associativity);
	if(desc.type & all_cache) {
		w << ", ";
		w << desc.line_size << " byte line size";
		if(desc.attributes & sectored_two_lines) {
			w << ", 2 lines per sector";
		}
	} else if(desc.type & all_tlb) {
		w << ", ";
		w << desc.entries << " entries";
	}
	return w.str();
}

void print_cache_info(const cpu_t& cpu) {
	using namespace fmt::literals;

	const register_set_t& regs = cpu.features.at(cache_and_tlb).at(subleaf_t::zero);

	if((regs[eax] & 0xff) != 0x01) {
		return;
	}

	auto bytes = gsl::as_bytes(gsl::make_span(regs));

	std::vector<std::string> cache_entries;

	std::ptrdiff_t idx = 0;
	for(register_t r = eax; r <= edx; ++r, idx += sizeof(std::uint32_t)) {
		if(regs[r] & 0x8000'0000ui32) {
			continue;
		}
		for(std::ptrdiff_t j = 0; j < sizeof(std::uint32_t); ++j) {
			const std::uint8_t value = gsl::to_integer<std::uint8_t>(bytes[idx + j]);
			switch(value) {
			case 0x00ui8:
				break;
			case 0x40ui8:
				cache_entries.push_back("No 2nd-level cache or, if processor contains a valid 2nd-level cache, no 3rd-level cache");
				break;
			case 0x49ui8:
				{
					auto it = dual_cache_descriptors.find(value);
					if(cpu.model.family == 0x0f && cpu.model.model == 0x06) {
						cache_entries.push_back(to_string(it->second.first));
					} else {
						cache_entries.push_back(to_string(it->second.second));
					}
				}
				break;
			case 0xf0ui8:
				cache_entries.push_back("64-Byte prefetching");
				break;
			case 0xf1ui8:
				cache_entries.push_back("128-Byte prefetching");
				break;
			case 0xfeui8:
			case 0xffui8:
				break;
			default:
				{
					auto dual = dual_cache_descriptors.find(value);
					if(dual != dual_cache_descriptors.end()) {
						cache_entries.push_back(to_string(dual->second.first) + " and "
						                      + to_string(dual->second.second));
						break;
					}
					auto it = standard_cache_descriptors.find(value);
					if(it != standard_cache_descriptors.end()) {
						cache_entries.push_back(to_string(it->second));
						break;
					}
					cache_entries.push_back("Unknown cache type: {:#2x}"_format(value));
				}
				break;
			}
		}
	}
	for(const std::string& s : cache_entries) {
		std::cout << s << std::endl;
	}
	std::cout << std::endl;
}

void print_serial_number(const cpu_t& cpu) {
	using namespace fmt::literals;

	if(cpu.vendor != intel && cpu.vendor != transmeta) {
		return;
	}
	std::cout << "Processor serial number: ";
	if(0 == (cpu.features.at(version_info).at(zero)[edx] & (1ui32 << 18ui32))) {
		std::cout << "N/A" << std::endl;
		return;
	}
	const register_set_t& regs = cpu.features.at(serial_number).at(subleaf_t::zero);
	switch(cpu.vendor) {
	case intel:
		{
			const std::uint32_t top = cpu.features.at(version_info).at(zero)[eax];
			const std::uint32_t middle = regs[edx];
			const std::uint32_t bottom = regs[ecx];
			std::cout << "{:04x}-{:04x}-{:04x}-{:04x}-{:04x}-{:04x}"_format(top >> 16ui32, top & 0xffffui32, middle >> 16ui32, middle & 0xffffui32, bottom >> 16ui32, bottom & 0xffffui32) << std::endl;
		}
		break;
	case transmeta:
		std::cout << "{:08x}-{:08x}-{:08x}-{:08x}"_format(regs[eax], regs[ebx], regs[ecx], regs[edx]) << std::endl;
		break;
	}
}

void enumerate_cache_parameters(cpu_t& cpu) {
	register_set_t regs = { 0 };
	std::uint32_t sub = subleaf_t::zero;
	while(true) {
		cpuid(regs, cache_parameters, sub);
		if((regs[eax] & 0x1fui32) == 0) {
			break;
		}
		cpu.features[cache_parameters][subleaf_t{ sub }] = regs;
		++sub;
	}
}

void print_cache_parameters(const cpu_t& cpu) {
	using namespace fmt::literals;

	struct cache_a_t
	{
		std::uint32_t type                           : 5;
		std::uint32_t level                          : 3;
		std::uint32_t self_initializing              : 1;
		std::uint32_t fully_associative              : 1;
		std::uint32_t reserved_1                     : 4;
		std::uint32_t maximum_addressable_thread_ids : 12;
		std::uint32_t maximum_addressable_core_ids   : 6;
	};

	struct cache_b_t
	{
		std::uint32_t coherency_line_size      : 12;
		std::uint32_t physical_line_partitions : 10;
		std::uint32_t associativity_ways       : 10;
	};

	struct cache_d_t
	{
		std::uint32_t writeback_invalidates : 1;
		std::uint32_t cache_inclusive       : 1;
		std::uint32_t complex_indexing      : 1;
		std::uint32_t reserved_1            : 29;
	};

	for(const auto& m : cpu.features.at(cache_parameters)) {
		const register_set_t& regs = m.second;

		union
		{
			std::uint32_t raw;
			cache_a_t a;
		} a;
		a.raw = regs[eax];

		union
		{
			std::uint32_t raw;
			cache_b_t b;
		} b;
		b.raw = regs[ebx];

		union
		{
			std::uint32_t raw;
			cache_d_t d;
		} d;
		d.raw = regs[edx];
		
		const std::size_t sets = regs[ecx];
		const std::size_t cache_size = (b.b.associativity_ways       + 1ui32)
		                             * (b.b.physical_line_partitions + 1ui32)
		                             * (b.b.coherency_line_size      + 1ui32)
		                             * (sets                         + 1ui32);
		
		fmt::MemoryWriter w;

		switch(a.a.type) {
		case 1:
			w << "Data Cache       , ";
			break;
		case 2:
			w << "Instruction Cache, ";
			break;
		case 3:
			w << "Unified Cache    , ";
			break;
		}
		double printable_cache_size = cache_size / 1'024.0;
		char   cache_scale = 'K';
		if(printable_cache_size > 1'024.0) {
			printable_cache_size /= 1'024.0;
			cache_scale = 'M';
		}
		w << "Level " << a.a.level << ", ";
		w << "{:d} bytes per line \u00d7 {:d} ways \u00d7 {:d} partitions \u00d7 {:d} sets = {:g} {:c}bytes. "_format(b.b.coherency_line_size + 1i32,
		                                                                                                              b.b.associativity_ways + 1ui32,
		                                                                                                              b.b.physical_line_partitions + 1ui32,
		                                                                                                              sets + 1ui32,
		                                                                                                              printable_cache_size,
		                                                                                                              cache_scale);
		if(a.a.self_initializing) {
			w << "Self-initializing. ";
		}
		if(a.a.fully_associative) {
			w << "Fully associative. ";
		} else {
			w << "{:d}-way set associative. "_format(b.b.associativity_ways + 1ui32);
		}
		if(d.d.writeback_invalidates) {
			w << "WBINVD/INVD does not invalidate lower level caches for other threads. ";
		} else {
			w << "WBINVD/INVD invalidate lower level caches for all threads. ";
		}
		w << "Cache is {:s} of lower cache levels. "_format(d.d.cache_inclusive != 0 ? "inclusive" : "exclusive");
		w << "Cache is {:s}direct mapped. "_format(d.d.complex_indexing != 1 ? "" : "not ");
		w << "Cache is shared by up to {:d} threads, with up to {:d} cores in the package."_format(a.a.maximum_addressable_thread_ids + 1, a.a.maximum_addressable_core_ids + 1);
		w << "Reserved: " << a.a.reserved_1;
		std::cout << w.str() << std::endl;
		std::cout << std::endl;
	}
}

struct leaf_descriptor_t
{
	bool has_subleaves;
	leaf_enumerate enumerator;
	leaf_print printer;
};

const std::map<leaf_t, leaf_descriptor_t> descriptors = {
	{ basic_info                        , { false, nullptr                   , print_basic_info       } },
	{ version_info                      , { false, nullptr                   , print_version_info     } },
	{ cache_and_tlb                     , { false, nullptr                   , print_cache_info       } },
	{ serial_number                     , { false, nullptr                   , print_serial_number    } },
	{ cache_parameters                  , { true , enumerate_cache_parameters, print_cache_parameters } },
	{ monitor_mwait                     , { false, nullptr                   , nullptr } },
	{ thermal_and_power                 , { false, nullptr                   , nullptr } },
	{ extended_features                 , { true , nullptr                   , nullptr } },
	{ reserved_1                        , { false, nullptr                   , nullptr } },
	{ direct_cache_access               , { false, nullptr                   , nullptr } },
	{ performance_monitoring            , { false, nullptr                   , nullptr } },
	{ extended_topology                 , { true , nullptr                   , nullptr } },
	{ reserved_2                        , { false, nullptr                   , nullptr } },
	{ extended_state                    , { true , nullptr                   , nullptr } },
	{ reserved_3                        , { false, nullptr                   , nullptr } },
	{ resource_director_monitoring      , { true , nullptr                   , nullptr } },
	{ cache_allocation_technology       , { true , nullptr                   , nullptr } },
	{ reserved_4                        , { false, nullptr                   , nullptr } },
	{ sgx_info                          , { true , nullptr                   , nullptr } },
	{ reserved_5                        , { false, nullptr                   , nullptr } },
	{ processor_trace                   , { true , nullptr                   , nullptr } },
	{ time_stamp_counter                , { false, nullptr                   , nullptr } },
	{ processor_frequency               , { false, nullptr                   , nullptr } },
	{ system_on_chip_vendor             , { true , nullptr                   , nullptr } },
	{ deterministic_address_translation , { true , nullptr                   , nullptr } },
	{ extended_limit                    , { false, nullptr                   , nullptr } },
	{ extended_signature_and_features   , { false, nullptr                   , nullptr } },
	{ brand_string_0                    , { false, nullptr                   , nullptr } },
	{ brand_string_1                    , { false, nullptr                   , nullptr } },
	{ brand_string_2                    , { false, nullptr                   , nullptr } },
	{ l1_cache_identifiers              , { false, nullptr                   , nullptr } },
	{ l2_cache_identifiers              , { false, nullptr                   , nullptr } },
	{ advanced_power_management         , { false, nullptr                   , nullptr } },
	{ address_limits                    , { false, nullptr                   , nullptr } },
	{ secure_virtual_machine            , { false, nullptr                   , nullptr } },
	{ tlb_1g_identifiers                , { false, nullptr                   , nullptr } },
	{ performance_optimization          , { false, nullptr                   , nullptr } },
	{ instruction_based_sampling        , { false, nullptr                   , nullptr } },
	{ cache_properties                  , { false, nullptr                   , nullptr } },
	{ extended_apic                     , { false, nullptr                   , nullptr } },
	{ secure_memory_encryption          , { false, nullptr                   , nullptr } }
};

int main(int, char*[]) {
	HANDLE output = ::GetStdHandle(STD_OUTPUT_HANDLE);
	DWORD mode = 0;
	::GetConsoleMode(output, &mode);
	mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
	::SetConsoleMode(output, mode);
	::SetConsoleCP(CP_UTF8);
	::SetConsoleOutputCP(CP_UTF8);
	std::cout.rdbuf()->pubsetbuf(nullptr, 1024);

	cpu_t cpu = {};
	register_set_t regs = { 0 };

	cpuid(regs, leaf_t::basic_info, subleaf_t::zero);
	cpu.highest_leaf = gsl::narrow_cast<std::uint32_t>(regs[eax]);
	cpu.vendor = get_vendor_from_name(regs);

	cpuid(regs, leaf_t::version_info, subleaf_t::zero);
	cpu.model = get_model(cpu.vendor, regs);

	for(leaf_t lf = leaf_t::basic_info; lf <= cpu.highest_leaf; ++lf) {
		auto it = descriptors.find(lf);
		if(it != descriptors.end()) {
			if(it->second.has_subleaves && it->second.enumerator) {
				it->second.enumerator(cpu);
			} else {
				cpuid(regs, lf, subleaf_t::zero);
				cpu.features[lf][subleaf_t::zero] = regs;
			}
		}
	}

	cpuid(regs, leaf_t::extended_limit, subleaf_t::zero);
	cpu.highest_extended_leaf = regs[eax];

	for(leaf_t lf = leaf_t::extended_limit; lf <= cpu.highest_extended_leaf; ++lf) {
		auto it = descriptors.find(lf);
		if(it != descriptors.end()) {
			if(it->second.has_subleaves && it->second.enumerator) {
				it->second.enumerator(cpu);
			} else {
				cpuid(regs, lf, subleaf_t::zero);
				cpu.features[lf][subleaf_t::zero] = regs;
			}
		}
	}

	for(const auto& lf : cpu.features) {
		auto it = descriptors.find(lf.first);
		if(it != descriptors.end()) {
			if(it->second.printer) {
				it->second.printer(cpu);
			}
		}
	}
}

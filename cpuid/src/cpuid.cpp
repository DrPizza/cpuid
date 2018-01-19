#include "stdafx.h"

#include "cpuid.hpp"
#include "cache.hpp"
#include "features.hpp"
#include "standard.hpp"
#include "topology.hpp"

#include <map>
#include <iostream>
#include <iomanip>

#include <gsl/gsl>

#include <fmt/format.h>

#include "docopt.h"

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

	const union
	{
		std::array<std::uint32_t, 3> registers;
		std::array<char, 12> vndr;
	} data = { regs[ebx], regs[edx], regs[ecx] };

	const auto it = vendors.find(data.vndr);
	return it != vendors.end() ? it->second : unknown;
}

model_t get_model(vendor_t vendor, const register_set_t& regs) noexcept {
	const union
	{
		std::uint32_t full;
		split_model_t split;
	} a = { regs[eax] };

	model_t model = {};
	model.family = a.split.family;
	model.model = a.split.model;
	model.stepping = a.split.stepping;
	switch(vendor) {
	case intel:
		{
			if(a.split.family == 0xf) {
				model.family += a.split.extended_family;
			}
			if(a.split.family == 0x6 || a.split.family == 0xf) {
				model.model += (a.split.extended_model << 4ui32);
			}
		}
		break;
	case amd:
		{
			model.family += a.split.extended_family;
			model.model += a.split.extended_model << 4ui32;
		}
		break;
	default:
		break;
	}
	return model;
}

uint8_t get_local_apic_id(const register_set_t& regs) noexcept {
	const union
	{
		std::uint32_t full;
		id_info_t split;
	} b = { regs[ebx] };
	return b.split.local_apic_id;
}

using leaf_print = void(*)(const cpu_t& cpu);
using leaf_enumerate = void(*)(cpu_t& cpu);

struct filter_t
{
	leaf_t leaf;
	subleaf_t subleaf;
	register_t reg;
	std::uint32_t mask;
};

bool operator==(const filter_t& lhs, const filter_t& rhs) noexcept {
	return lhs.leaf    == rhs.leaf
	    && lhs.subleaf == rhs.subleaf
	    && lhs.reg     == rhs.reg
	    && lhs.mask    == rhs.mask;
}

constexpr static filter_t no_filter = { };

struct leaf_descriptor_t
{
	vendor_t vendor;
	leaf_enumerate enumerator;
	leaf_print printer;
	filter_t filter;
};

void print_null(const cpu_t&) noexcept {
}

const std::map<leaf_t, leaf_descriptor_t> descriptors = {
	{ leaf_t::basic_info                     , { any                    , nullptr                        , print_basic_info                     , {} } },
	{ leaf_t::version_info                   , { any                    , nullptr                        , print_version_info                   , {} } },
	{ leaf_t::cache_and_tlb                  , { intel                  , nullptr                        , print_cache_tlb_info                 , {} } },
	{ leaf_t::serial_number                  , { intel       | transmeta, nullptr                        , print_serial_number                  , { leaf_t::version_info                   , subleaf_t::main, edx, 0x0004'0000ui32 } } },
	{ leaf_t::deterministic_cache            , { intel                  , enumerate_deterministic_cache  , print_deterministic_cache            , {} } },
	{ leaf_t::monitor_mwait                  , { intel | amd            , nullptr                        , print_mwait_parameters               , { leaf_t::version_info                   , subleaf_t::main, ecx, 0x0000'0008ui32 } } },
	{ leaf_t::thermal_and_power              , { intel | amd            , nullptr                        , print_thermal_and_power              , {} } },
	{ leaf_t::extended_features              , { any                    , enumerate_extended_features    , print_extended_features              , {} } },
	{ leaf_t::direct_cache_access            , { intel                  , nullptr                        , print_direct_cache_access            , { leaf_t::version_info                   , subleaf_t::main, ecx, 0x0004'0000ui32 } } },
	{ leaf_t::performance_monitoring         , { intel                  , nullptr                        , print_performance_monitoring         , { leaf_t::version_info                   , subleaf_t::main, ecx, 0x0000'8000ui32 } } },
	{ leaf_t::extended_topology              , { intel                  , enumerate_extended_topology    , print_extended_topology              , {} } },
	{ leaf_t::extended_state                 , { intel | amd            , enumerate_extended_state       , print_extended_state                 , { leaf_t::version_info                   , subleaf_t::main, ecx, 0x0400'0000ui32 } } },
	{ leaf_t::rdt_monitoring                 , { intel                  , enumerate_rdt_monitoring       , print_rdt_monitoring                 , { leaf_t::extended_features              , subleaf_t::main, ebx, 0x0000'1000ui32 } } },
	{ leaf_t::rdt_allocation                 , { intel                  , enumerate_rdt_allocation       , print_rdt_allocation                 , { leaf_t::extended_features              , subleaf_t::main, ebx, 0x0000'8000ui32 } } },
	{ leaf_t::sgx_info                       , { intel                  , enumerate_sgx_info             , print_sgx_info                       , { leaf_t::extended_features              , subleaf_t::main, ebx, 0x0000'0004ui32 } } },
	{ leaf_t::processor_trace                , { intel                  , enumerate_processor_trace      , print_processor_trace                , { leaf_t::extended_features              , subleaf_t::main, ebx, 0x0200'0000ui32 } } },
	{ leaf_t::time_stamp_counter             , { intel                  , nullptr                        , print_time_stamp_counter             , {} } },
	{ leaf_t::processor_frequency            , { intel                  , nullptr                        , print_processor_frequency            , {} } },
	{ leaf_t::system_on_chip_vendor          , { intel                  , enumerate_system_on_chip_vendor, print_system_on_chip_vendor          , {} } },
	{ leaf_t::deterministic_tlb              , { intel                  , enumerate_deterministic_tlb    , print_deterministic_tlb              , {} } },
	{ leaf_t::extended_limit                 , { any                    , nullptr                        , print_extended_limit                 , {} } },
	{ leaf_t::extended_signature_and_features, { any                    , nullptr                        , print_extended_signature_and_features, {} } },
	{ leaf_t::brand_string_0                 , { any                    , nullptr                        , print_brand_string                   , {} } },
	{ leaf_t::brand_string_1                 , { any                    , nullptr                        , print_null                           , {} } },
	{ leaf_t::brand_string_2                 , { any                    , nullptr                        , print_null                           , {} } },
	{ leaf_t::l1_cache_identifiers           , {         amd            , nullptr                        , print_l1_cache_tlb                   , {} } },
	{ leaf_t::l2_cache_identifiers           , { intel | amd            , nullptr                        , print_l2_cache_tlb                   , {} } },
	{ leaf_t::ras_advanced_power_management  , { intel | amd            , nullptr                        , print_ras_advanced_power_management  , {} } },
	{ leaf_t::address_limits                 , { intel | amd            , nullptr                        , print_address_limits                 , {} } },
	{ leaf_t::secure_virtual_machine         , {         amd            , nullptr                        , print_secure_virtual_machine         , { leaf_t::extended_signature_and_features, subleaf_t::main, ecx, 0x0000'0004ui32 } } },
	{ leaf_t::tlb_1g_identifiers             , {         amd            , nullptr                        , print_1g_tlb                         , {} } },
	{ leaf_t::performance_optimization       , {         amd            , nullptr                        , print_performance_optimization       , {} } },
	{ leaf_t::instruction_based_sampling     , {         amd            , nullptr                        , print_instruction_based_sampling     , { leaf_t::extended_signature_and_features, subleaf_t::main, ecx, 0x0000'0400ui32 } } },
	{ leaf_t::lightweight_profiling          , {         amd            , nullptr                        , print_lightweight_profiling          , { leaf_t::extended_signature_and_features, subleaf_t::main, ecx, 0x0000'8000ui32 } } },
	{ leaf_t::cache_properties               , {         amd            , enumerate_cache_properties     , print_cache_properties               , {} } },
	{ leaf_t::extended_apic                  , {         amd            , nullptr                        , print_extended_apic                  , { leaf_t::extended_signature_and_features, subleaf_t::main, ecx, 0x0040'0000ui32 } } },
	{ leaf_t::encrypted_memory               , {         amd            , nullptr                        , print_encrypted_memory               , {} } }
};

void print_generic(const cpu_t& cpu, leaf_t leaf, subleaf_t subleaf) {
	using namespace fmt::literals;
	const register_set_t& regs = cpu.leaves.at(leaf).at(subleaf);
	std::cout << "{:#010x} {:#010x} {:#010x}: {:#010x} {:#010x} {:#010x} {:#010x}"_format(cpu.local_apic_id,
	                                                                                      static_cast<std::uint32_t>(leaf),
	                                                                                      static_cast<std::uint32_t>(subleaf),
	                                                                                      regs[eax],
	                                                                                      regs[ebx],
	                                                                                      regs[ecx],
	                                                                                      regs[edx]) << std::endl;
}

void print_generic(const cpu_t& cpu, leaf_t leaf) {
	for(const auto& sub : cpu.leaves.at(leaf)) {
		print_generic(cpu, leaf, sub.first);
	}
}

void print_generic(const cpu_t& cpu) {
	for(const auto& leaf : cpu.leaves) {
		print_generic(cpu, leaf.first);
	}
}

void enumerate_leaf_brute_force(cpu_t& cpu, leaf_t leaf) {
	register_set_t previous = { 0 };
	for(subleaf_t subleaf = subleaf_t::main; ; ++subleaf) {
		register_set_t regs = { 0 };
		cpuid(regs, leaf, subleaf);
		if(regs[eax] == 0ui32
			&& regs[ebx] == 0ui32
			&& (regs[ecx] == 0ui32 || regs[ecx] == static_cast<std::uint32_t>(subleaf))
			&& regs[edx] == 0ui32) {
			break;
		}
		if(regs[eax] == previous[eax]
			&& regs[ebx] == previous[ebx]
			&& regs[ecx] == previous[ecx]
			&& regs[edx] == previous[edx]) {
			break;
		}
		cpu.leaves[leaf][subleaf] = regs;
		previous = regs;
	}
}

void enumerate_leaf(cpu_t& cpu, leaf_t leaf) {
	register_set_t regs = { 0 };
	const auto it = descriptors.find(leaf);
	if(it != descriptors.end()) {
		if(it->second.vendor & cpu.vendor) {
			const filter_t filter = it->second.filter;
			if(filter == no_filter
			|| filter.mask == (filter.mask & cpu.leaves.at(filter.leaf).at(filter.subleaf).at(filter.reg))) {
				if(it->second.enumerator) {
					it->second.enumerator(cpu);
				} else {
					cpuid(regs, leaf, subleaf_t::main);
					cpu.leaves[leaf][subleaf_t::main] = regs;
				}
			}
		}
	} else {
		enumerate_leaf_brute_force(cpu, leaf);
	}
}

static const char usage_message[] =
R"(cpuid.

	Usage:
		cpuid [--cpu <index>] [--dump]
		cpuid --help
		cpuid --version

	Options:
		--cpu <index>         Index of logical core to get info from
		--dump                Print unparsed output
		--help                Show this text
		--version             Show the version
)";

int main(int argc, char* argv[]) {
	HANDLE output = ::GetStdHandle(STD_OUTPUT_HANDLE);
	DWORD mode = 0;
	::GetConsoleMode(output, &mode);
	mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
	::SetConsoleMode(output, mode);
	::SetConsoleCP(CP_UTF8);
	::SetConsoleOutputCP(CP_UTF8);
	std::cout.rdbuf()->pubsetbuf(nullptr, 1024);

	const std::map<std::string, docopt::value> args = docopt::docopt(usage_message, { argv + 1, argv + argc }, true, "cpuid 0.1");

	// I don't care which thread I run on, just as long as I'm not bouncing between cores.
	const DWORD current_processor = ::GetCurrentProcessorNumber();
	const DWORD_PTR mask = 1ui64 << current_processor;
	::SetThreadAffinityMask(::GetCurrentThread(), mask);

	cpu_t cpu = {};
	register_set_t regs = { 0 };

	cpuid(regs, leaf_t::basic_info, subleaf_t::main);
	cpu.highest_leaf = leaf_t{ regs[eax] };
	cpu.vendor = get_vendor_from_name(regs);

	cpuid(regs, leaf_t::version_info, subleaf_t::main);
	cpu.model = get_model(cpu.vendor, regs);
	cpu.local_apic_id = get_local_apic_id(regs);

	for(leaf_t lf = leaf_t::basic_info; lf <= cpu.highest_leaf; ++lf) {
		enumerate_leaf(cpu, lf);
	}

	cpuid(regs, leaf_t::extended_limit, subleaf_t::main);
	cpu.highest_extended_leaf = leaf_t{ regs[eax] };

	for(leaf_t lf = leaf_t::extended_limit; lf <= cpu.highest_extended_leaf; ++lf) {
		enumerate_leaf(cpu, lf);
	}

	for(const auto& lf : cpu.leaves) {
		const auto it = descriptors.find(lf.first);
		if(it != descriptors.end()) {
			if(it->second.vendor & cpu.vendor) {
				const filter_t filter = it->second.filter;
				if(filter      == no_filter
				|| filter.mask == (filter.mask & cpu.leaves.at(filter.leaf).at(filter.subleaf).at(filter.reg))) {
					if(it->second.printer) {
						it->second.printer(cpu);
					} else {
						print_generic(cpu, lf.first);
						std::cout << std::endl;
					}
				}
			}
		}
	}

	print_generic(cpu);
}

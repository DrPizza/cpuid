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

	auto it = vendors.find(data.vndr);
	return it != vendors.end() ? it->second : unknown;
}

model_t get_model(vendor_t vendor, const register_set_t& regs) noexcept {
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

const std::map<leaf_t, leaf_descriptor_t> descriptors = {
	{ leaf_t::basic_info                        , { any                    , nullptr                      , print_basic_info                     , {} } },
	{ leaf_t::version_info                      , { any                    , nullptr                      , print_version_info                   , {} } },
	{ leaf_t::cache_and_tlb                     , { intel                  , nullptr                      , print_cache_tlb_info                 , {} } },
	{ leaf_t::serial_number                     , { intel       | transmeta, nullptr                      , print_serial_number                  , { leaf_t::version_info     , subleaf_t::main, edx, 0x0004'0000ui32 } } },
	{ leaf_t::deterministic_cache               , { intel                  , enumerate_deterministic_cache, print_deterministic_cache            , {} } },
	{ leaf_t::monitor_mwait                     , { intel | amd            , nullptr                      , print_mwait_parameters               , { leaf_t::version_info     , subleaf_t::main, ecx, 0x0000'0008ui32 } } },
	{ leaf_t::thermal_and_power                 , { intel | amd            , nullptr                      , print_thermal_and_power              , {} } },
	{ leaf_t::extended_features                 , { any                    , enumerate_extended_features  , print_extended_features              , {} } },
	{ leaf_t::direct_cache_access               , { intel                  , nullptr                      , print_direct_cache_access            , { leaf_t::version_info     , subleaf_t::main, ecx, 0x0004'0000ui32 } } },
	{ leaf_t::performance_monitoring            , { intel                  , nullptr                      , print_performance_monitoring         , { leaf_t::version_info     , subleaf_t::main, ecx, 0x0000'8000ui32 } } },
	{ leaf_t::extended_topology                 , { intel                  , enumerate_extended_topology  , print_extended_topology              , {} } },
	{ leaf_t::extended_state                    , { intel | amd            , enumerate_extended_state     , print_extended_state                 , { leaf_t::version_info     , subleaf_t::main, ecx, 0x0400'0000ui32 } } },
	{ leaf_t::rdt_monitoring                    , { intel                  , enumerate_rdt_monitoring     , print_rdt_monitoring                 , { leaf_t::extended_features, subleaf_t::main, ebx, 0x0000'1000ui32 } } },
	{ leaf_t::rdt_allocation                    , { intel                  , enumerate_rdt_allocation     , print_rdt_allocation                 , { leaf_t::extended_features, subleaf_t::main, ebx, 0x0000'8000ui32 } } },
	{ leaf_t::sgx_info                          , { intel                  , enumerate_sgx_info           , print_sgx_info                       , { leaf_t::extended_features, subleaf_t::main, ebx, 0x0000'0004ui32 } } },
	{ leaf_t::processor_trace                   , { intel                  , nullptr                      , nullptr                              , { leaf_t::extended_features, subleaf_t::main, ebx, 0x0200'0000ui32 } } },
	{ leaf_t::time_stamp_counter                , { intel                  , nullptr                      , nullptr                              , {} } },
	{ leaf_t::processor_frequency               , { intel                  , nullptr                      , nullptr                              , {} } },
	{ leaf_t::system_on_chip_vendor             , { intel                  , nullptr                      , nullptr                              , {} } },
	{ leaf_t::deterministic_address_translation , { intel                  , nullptr                      , nullptr                              , {} } },
	{ leaf_t::extended_limit                    , { any                    , nullptr                      , print_extended_limit                 , {} } },
	{ leaf_t::extended_signature_and_features   , { any                    , nullptr                      , print_extended_signature_and_features, {} } },
	{ leaf_t::brand_string_0                    , { any                    , nullptr                      , print_brand_string                   , {} } },
	{ leaf_t::brand_string_1                    , { any                    , nullptr                      , nullptr                              , {} } },
	{ leaf_t::brand_string_2                    , { any                    , nullptr                      , nullptr                              , {} } },
	{ leaf_t::l1_cache_identifiers              , {         amd            , nullptr                      , print_l1_cache_tlb                   , {} } },
	{ leaf_t::l2_cache_identifiers              , { intel | amd            , nullptr                      , print_l2_cache_tlb                   , {} } },
	{ leaf_t::ras_advanced_power_management     , { intel | amd            , nullptr                      , print_ras_advanced_power_management  , {} } },
	{ leaf_t::address_limits                    , { intel | amd            , nullptr                      , print_address_limits                 , {} } },
	{ leaf_t::secure_virtual_machine            , {         amd            , nullptr                      , nullptr                              , {} } },
	{ leaf_t::tlb_1g_identifiers                , {         amd            , nullptr                      , nullptr                              , {} } },
	{ leaf_t::performance_optimization          , {         amd            , nullptr                      , nullptr                              , {} } },
	{ leaf_t::instruction_based_sampling        , {         amd            , nullptr                      , nullptr                              , {} } },
	{ leaf_t::cache_properties                  , {         amd            , nullptr                      , nullptr                              , {} } },
	{ leaf_t::extended_apic                     , {         amd            , nullptr                      , nullptr                              , {} } },
	{ leaf_t::secure_memory_encryption          , {         amd            , nullptr                      , nullptr                              , {} } }
};

void print_generic(const cpu_t& cpu, leaf_t leaf, subleaf_t subleaf) {
	using namespace fmt::literals;
	const register_set_t& regs = cpu.features.at(leaf).at(subleaf);
	std::cout << "{:#010x} {:#010x}: {:#010x} {:#010x} {:#010x} {:#010x}"_format(static_cast<std::uint32_t>(leaf),
	                                                                             static_cast<std::uint32_t>(subleaf),
	                                                                             regs[eax],
	                                                                             regs[ebx],
	                                                                             regs[ecx],
	                                                                             regs[edx]) << std::endl;
}

void print_generic(const cpu_t& cpu, leaf_t leaf) {
	for(const auto& sub : cpu.features.at(leaf)) {
		print_generic(cpu, leaf, sub.first);
	}
}

void print_generic(const cpu_t& cpu) {
	for(const auto& leaf : cpu.features) {
		print_generic(cpu, leaf.first);
	}
}

int main(int, char*[]) {
	HANDLE output = ::GetStdHandle(STD_OUTPUT_HANDLE);
	DWORD mode = 0;
	::GetConsoleMode(output, &mode);
	mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
	::SetConsoleMode(output, mode);
	::SetConsoleCP(CP_UTF8);
	::SetConsoleOutputCP(CP_UTF8);
	std::cout.rdbuf()->pubsetbuf(nullptr, 1024);

	::SetThreadAffinityMask(::GetCurrentThread(), 0x8);

	cpu_t cpu = {};
	register_set_t regs = { 0 };

	cpuid(regs, leaf_t::basic_info, subleaf_t::main);
	cpu.highest_leaf = leaf_t{ regs[eax] };
	cpu.vendor = get_vendor_from_name(regs);

	cpuid(regs, leaf_t::version_info, subleaf_t::main);
	cpu.model = get_model(cpu.vendor, regs);

	for(leaf_t lf = leaf_t::basic_info; lf <= cpu.highest_leaf; ++lf) {
		auto it = descriptors.find(lf);
		if(it != descriptors.end()) {
			if(it->second.enumerator) {
				it->second.enumerator(cpu);
			} else {
				cpuid(regs, lf, subleaf_t::main);
				cpu.features[lf][subleaf_t::main] = regs;
			}
		}
	}

	cpuid(regs, leaf_t::extended_limit, subleaf_t::main);
	cpu.highest_extended_leaf = leaf_t{ regs[eax] };

	for(leaf_t lf = leaf_t::extended_limit; lf <= cpu.highest_extended_leaf; ++lf) {
		auto it = descriptors.find(lf);
		if(it != descriptors.end()) {
			if(it->second.vendor & cpu.vendor) {
				const filter_t filter = it->second.filter;
				if(filter      == no_filter
				|| filter.mask == (filter.mask & cpu.features.at(filter.leaf).at(filter.subleaf).at(filter.reg))) {
					if(it->second.enumerator) {
						it->second.enumerator(cpu);
					} else {
						cpuid(regs, lf, subleaf_t::main);
						cpu.features[lf][subleaf_t::main] = regs;
					}
				}
			}
		}
	}

	for(const auto& lf : cpu.features) {
		auto it = descriptors.find(lf.first);
		if(it != descriptors.end()) {
			if(it->second.vendor & cpu.vendor) {
				const filter_t filter = it->second.filter;
				if(filter      == no_filter
				|| filter.mask == (filter.mask & cpu.features.at(filter.leaf).at(filter.subleaf).at(filter.reg))) {
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

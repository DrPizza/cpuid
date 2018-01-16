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

struct leaf_descriptor_t
{
	bool has_subleaves;
	leaf_enumerate enumerator;
	leaf_print printer;
};

const std::map<leaf_t, leaf_descriptor_t> descriptors = {
	{ basic_info                        , { false, nullptr                      , print_basic_info             } },
	{ version_info                      , { false, nullptr                      , print_version_info           } },
	{ cache_and_tlb                     , { false, nullptr                      , print_cache_tlb_info         } },
	{ serial_number                     , { false, nullptr                      , print_serial_number          } },
	{ deterministic_cache               , { true , enumerate_deterministic_cache, print_deterministic_cache    } },
	{ monitor_mwait                     , { false, nullptr                      , print_mwait_parameters       } },
	{ thermal_and_power                 , { false, nullptr                      , print_thermal_and_power      } },
	{ extended_features                 , { true , enumerate_extended_features  , print_extended_features      } },
	{ direct_cache_access               , { false, nullptr                      , print_direct_cache_access    } },
	{ performance_monitoring            , { false, nullptr                      , print_performance_monitoring } },
	{ extended_topology                 , { true , enumerate_extended_topology  , print_extended_topology      } },
	{ reserved_2                        , { false, nullptr                      , nullptr } },
	{ extended_state                    , { true , nullptr                      , nullptr } },
	{ reserved_3                        , { false, nullptr                      , nullptr } },
	{ resource_director_monitoring      , { true , nullptr                      , nullptr } },
	{ cache_allocation_technology       , { true , nullptr                      , nullptr } },
	{ reserved_4                        , { false, nullptr                      , nullptr } },
	{ sgx_info                          , { true , nullptr                      , nullptr } },
	{ reserved_5                        , { false, nullptr                      , nullptr } },
	{ processor_trace                   , { true , nullptr                      , nullptr } },
	{ time_stamp_counter                , { false, nullptr                      , nullptr } },
	{ processor_frequency               , { false, nullptr                      , nullptr } },
	{ system_on_chip_vendor             , { true , nullptr                      , nullptr } },
	{ deterministic_address_translation , { true , nullptr                      , nullptr } },
	{ extended_limit                    , { false, nullptr                      , nullptr } },
	{ extended_signature_and_features   , { false, nullptr                      , nullptr } },
	{ brand_string_0                    , { false, nullptr                      , nullptr } },
	{ brand_string_1                    , { false, nullptr                      , nullptr } },
	{ brand_string_2                    , { false, nullptr                      , nullptr } },
	{ l1_cache_identifiers              , { false, nullptr                      , nullptr } },
	{ l2_cache_identifiers              , { false, nullptr                      , nullptr } },
	{ advanced_power_management         , { false, nullptr                      , nullptr } },
	{ address_limits                    , { false, nullptr                      , nullptr } },
	{ secure_virtual_machine            , { false, nullptr                      , nullptr } },
	{ tlb_1g_identifiers                , { false, nullptr                      , nullptr } },
	{ performance_optimization          , { false, nullptr                      , nullptr } },
	{ instruction_based_sampling        , { false, nullptr                      , nullptr } },
	{ cache_properties                  , { false, nullptr                      , nullptr } },
	{ extended_apic                     , { false, nullptr                      , nullptr } },
	{ secure_memory_encryption          , { false, nullptr                      , nullptr } }
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

	::SetThreadAffinityMask(::GetCurrentThread(), 0x4);

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

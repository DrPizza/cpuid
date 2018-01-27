#include "stdafx.h"

#include "cpuid.hpp"
#include "cache-and-topology.hpp"
#include "features.hpp"
#include "standard.hpp"
#include "hypervisors.hpp"

#include "utility.hpp"

#include <map>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <tuple>
#include <regex>

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
	};

	const union
	{
		std::array<std::uint32_t, 3> registers;
		std::array<char, 12> vndr;
	} data = { regs[ebx], regs[edx], regs[ecx] };

	const auto it = vendors.find(data.vndr);
	return it != vendors.end() ? it->second : unknown;
}

vendor_t get_hypervisor_from_name(const register_set_t& regs) {
	static const std::map<std::array<char, 12>, vendor_t> vendors = {
		{ to_array("bhyve byhve "), bhyve },
		{ to_array("KVMKVMKVM\0\0\0"), kvm },
		{ to_array("Microsoft Hv"), hyper_v },
		{ to_array(" lrpepyh vr\0"), parallels },
		{ to_array("VMwareVMware"), vmware },
		{ to_array("XenVMMXenVMM"), xen_hvm }
	};

	const union
	{
		std::array<std::uint32_t, 3> registers;
		std::array<char, 12> vndr;
	} data = { regs[ebx], regs[ecx], regs[edx] };

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

uint32_t get_apic_id(const cpu_t& cpu) {
	switch(cpu.vendor & any_silicon) {
	case intel:
		if(cpu.leaves.find(leaf_t::extended_topology) != cpu.leaves.end()) {
			return cpu.leaves.at(leaf_t::extended_topology).at(subleaf_t::main).at(edx);
		}
		break;
	case amd:
		if(cpu.leaves.find(leaf_t::extended_apic) != cpu.leaves.end()) {
			return cpu.leaves.at(leaf_t::extended_apic).at(subleaf_t::main).at(eax);
		}
		break;
	default:
		break;
	}
	return get_local_apic_id(cpu.leaves.at(leaf_t::version_info).at(subleaf_t::main));
}

using leaf_print = void(*)(fmt::Writer& w, const cpu_t& cpu);
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

void enumerate_null(cpu_t&) noexcept {
}

void print_null(fmt::Writer&, const cpu_t&) noexcept {
}

const std::multimap<leaf_t, leaf_descriptor_t> descriptors = {
	{ leaf_t::basic_info                     , { any                    , nullptr                        , print_basic_info                     , {} } },
	{ leaf_t::version_info                   , { any                    , nullptr                        , print_version_info                   , {} } },
	{ leaf_t::cache_and_tlb                  , { intel                  , nullptr                        , print_cache_tlb_info                 , {} } },
	{ leaf_t::serial_number                  , { intel       | transmeta, nullptr                        , print_serial_number                  , { leaf_t::version_info                   , subleaf_t::main, edx, 0x0004'0000ui32 } } },
	{ leaf_t::deterministic_cache            , { intel                  , enumerate_deterministic_cache  , print_deterministic_cache            , {} } },
	{ leaf_t::monitor_mwait                  , { intel | amd            , nullptr                        , print_mwait_parameters               , { leaf_t::version_info                   , subleaf_t::main, ecx, 0x0000'0008ui32 } } },
	{ leaf_t::thermal_and_power              , { intel | amd            , nullptr                        , print_thermal_and_power              , {} } },
	{ leaf_t::extended_features              , { any                    , enumerate_extended_features    , print_extended_features              , {} } },
	{ leaf_t::reserved_1                     , { any                    , enumerate_null                 , print_null                           , {} } },
	{ leaf_t::direct_cache_access            , { intel                  , nullptr                        , print_direct_cache_access            , { leaf_t::version_info                   , subleaf_t::main, ecx, 0x0004'0000ui32 } } },
	{ leaf_t::performance_monitoring         , { intel                  , nullptr                        , print_performance_monitoring         , { leaf_t::version_info                   , subleaf_t::main, ecx, 0x0000'8000ui32 } } },
	{ leaf_t::extended_topology              , { intel                  , enumerate_extended_topology    , print_extended_topology              , {} } },
	{ leaf_t::reserved_2                     , { any                    , enumerate_null                 , print_null                           , {} } },
	{ leaf_t::extended_state                 , { intel | amd            , enumerate_extended_state       , print_extended_state                 , { leaf_t::version_info                   , subleaf_t::main, ecx, 0x0400'0000ui32 } } },
	{ leaf_t::reserved_3                     , { any                    , enumerate_null                 , print_null                           , {} } },
	{ leaf_t::rdt_monitoring                 , { intel                  , enumerate_rdt_monitoring       , print_rdt_monitoring                 , { leaf_t::extended_features              , subleaf_t::main, ebx, 0x0000'1000ui32 } } },
	{ leaf_t::rdt_allocation                 , { intel                  , enumerate_rdt_allocation       , print_rdt_allocation                 , { leaf_t::extended_features              , subleaf_t::main, ebx, 0x0000'8000ui32 } } },
	{ leaf_t::reserved_4                     , { any                    , enumerate_null                 , print_null                           , {} } },
	{ leaf_t::sgx_info                       , { intel                  , enumerate_sgx_info             , print_sgx_info                       , { leaf_t::extended_features              , subleaf_t::main, ebx, 0x0000'0004ui32 } } },
	{ leaf_t::reserved_5                     , { any                    , enumerate_null                 , print_null                           , {} } },
	{ leaf_t::processor_trace                , { intel                  , enumerate_processor_trace      , print_processor_trace                , { leaf_t::extended_features              , subleaf_t::main, ebx, 0x0200'0000ui32 } } },
	{ leaf_t::time_stamp_counter             , { intel                  , nullptr                        , print_time_stamp_counter             , {} } },
	{ leaf_t::processor_frequency            , { intel                  , nullptr                        , print_processor_frequency            , {} } },
	{ leaf_t::system_on_chip_vendor          , { intel                  , enumerate_system_on_chip_vendor, print_system_on_chip_vendor          , {} } },
	{ leaf_t::deterministic_tlb              , { intel                  , enumerate_deterministic_tlb    , print_deterministic_tlb              , {} } },
	{ leaf_t::reserved_6                     , { any                    , enumerate_null                 , print_null                           , {} } },
	{ leaf_t::reserved_7                     , { any                    , enumerate_null                 , print_null                           , {} } },
	{ leaf_t::pconfig                        , { any                    , enumerate_pconfig              , print_pconfig                        , { leaf_t::extended_features              , subleaf_t::main, edx, 0x0004'0000ui32 } } },

	{ leaf_t::hypervisor_limit               , { any                    , nullptr                        , print_hypervisor_limit               , {} } },
	{ leaf_t::hyper_v_signature              , { hyper_v                , nullptr                        , print_hyper_v_signature              , {} } },
	{ leaf_t::hyper_v_system_identity        , { hyper_v                , nullptr                        , print_hyper_v_system_identity        , {} } },
	{ leaf_t::hyper_v_features               , { hyper_v                , nullptr                        , print_hyper_v_features               , {} } },
	{ leaf_t::hyper_v_enlightenment_recs     , { hyper_v                , nullptr                        , print_hyper_v_enlightenment_recs     , {} } },
	{ leaf_t::hyper_v_implementation_limits  , { hyper_v                , nullptr                        , print_hyper_v_implementation_limits  , {} } },
	{ leaf_t::hyper_v_implementation_hardware, { hyper_v                , nullptr                        , print_hyper_v_implementation_hardware, {} } },
	{ leaf_t::hyper_v_root_cpu_management    , { hyper_v                , nullptr                        , print_hyper_v_root_cpu_management    , {} } },
	{ leaf_t::hyper_v_shared_virtual_memory  , { hyper_v                , nullptr                        , print_hyper_v_shared_virtual_memory  , {} } },
	{ leaf_t::hyper_v_nested_hypervisor      , { hyper_v                , nullptr                        , print_hyper_v_nested_hypervisor      , {} } },
	{ leaf_t::hyper_v_nested_features        , { hyper_v                , nullptr                        , print_hyper_v_nested_features        , {} } },
	{ leaf_t::xen_limit                      , { xen_hvm                , nullptr                        , print_null                           , {} } },
	{ leaf_t::xen_version                    , { xen_hvm                , nullptr                        , print_xen_version                    , {} } },
	{ leaf_t::xen_features                   , { xen_hvm                , nullptr                        , print_xen_features                   , {} } },
	{ leaf_t::xen_time                       , { xen_hvm                , enumerate_xen_time             , print_xen_time                       , {} } },
	{ leaf_t::xen_hvm_features               , { xen_hvm                , nullptr                        , print_xen_hvm_features               , {} } },
	{ leaf_t::xen_pv_features                , { xen_hvm                , nullptr                        , print_xen_pv_features                , {} } },
	{ leaf_t::xen_limit_offset               , { xen_hvm                , nullptr                        , print_xen_limit                      , {} } },
	{ leaf_t::xen_version_offset             , { xen_hvm                , nullptr                        , print_xen_version                    , {} } },
	{ leaf_t::xen_features_offset            , { xen_hvm                , nullptr                        , print_xen_features                   , {} } },
	{ leaf_t::xen_time_offset                , { xen_hvm                , enumerate_xen_time             , print_xen_time                       , {} } },
	{ leaf_t::xen_hvm_features_offset        , { xen_hvm                , nullptr                        , print_xen_hvm_features               , {} } },
	{ leaf_t::xen_pv_features_offset         , { xen_hvm                , nullptr                        , print_xen_pv_features                , {} } },
	{ leaf_t::vmware_timing                  , { vmware                 , nullptr                        , print_vmware_timing                  , {} } },
	{ leaf_t::kvm_features                   , { vmware                 , nullptr                        , print_kvm_features                   , {} } },

	{ leaf_t::extended_limit                 , { any                    , nullptr                        , print_extended_limit                 , {} } },
	{ leaf_t::extended_signature_and_features, { any                    , nullptr                        , print_extended_signature_and_features, {} } },
	{ leaf_t::brand_string_0                 , { any                    , nullptr                        , print_brand_string                   , {} } },
	{ leaf_t::brand_string_1                 , { any                    , nullptr                        , print_null                           , {} } },
	{ leaf_t::brand_string_2                 , { any                    , nullptr                        , print_null                           , {} } },
	{ leaf_t::l1_cache_identifiers           , {         amd            , nullptr                        , print_l1_cache_tlb                   , {} } },
	{ leaf_t::l2_cache_identifiers           , { intel | amd            , nullptr                        , print_l2_cache_tlb                   , {} } },
	{ leaf_t::ras_advanced_power_management  , { intel | amd            , nullptr                        , print_ras_advanced_power_management  , {} } },
	{ leaf_t::address_limits                 , { intel | amd            , nullptr                        , print_address_limits                 , {} } },
	{ leaf_t::reserved_8                     , { any                    , enumerate_null                 , print_null                           , {} } },
	{ leaf_t::secure_virtual_machine         , {         amd            , nullptr                        , print_secure_virtual_machine         , { leaf_t::extended_signature_and_features, subleaf_t::main, ecx, 0x0000'0004ui32 } } },
	{ leaf_t::extended_reserved_1            , { any                    , enumerate_null                 , print_null                           , {} } },
	{ leaf_t::extended_reserved_2            , { any                    , enumerate_null                 , print_null                           , {} } },
	{ leaf_t::extended_reserved_3            , { any                    , enumerate_null                 , print_null                           , {} } },
	{ leaf_t::extended_reserved_4            , { any                    , enumerate_null                 , print_null                           , {} } },
	{ leaf_t::extended_reserved_5            , { any                    , enumerate_null                 , print_null                           , {} } },
	{ leaf_t::extended_reserved_6            , { any                    , enumerate_null                 , print_null                           , {} } },
	{ leaf_t::extended_reserved_7            , { any                    , enumerate_null                 , print_null                           , {} } },
	{ leaf_t::extended_reserved_8            , { any                    , enumerate_null                 , print_null                           , {} } },
	{ leaf_t::extended_reserved_9            , { any                    , enumerate_null                 , print_null                           , {} } },
	{ leaf_t::extended_reserved_10           , { any                    , enumerate_null                 , print_null                           , {} } },
	{ leaf_t::extended_reserved_11           , { any                    , enumerate_null                 , print_null                           , {} } },
	{ leaf_t::extended_reserved_12           , { any                    , enumerate_null                 , print_null                           , {} } },
	{ leaf_t::extended_reserved_13           , { any                    , enumerate_null                 , print_null                           , {} } },
	{ leaf_t::extended_reserved_14           , { any                    , enumerate_null                 , print_null                           , {} } },
	{ leaf_t::tlb_1g_identifiers             , {         amd            , nullptr                        , print_1g_tlb                         , {} } },
	{ leaf_t::performance_optimization       , {         amd            , nullptr                        , print_performance_optimization       , {} } },
	{ leaf_t::instruction_based_sampling     , {         amd            , nullptr                        , print_instruction_based_sampling     , { leaf_t::extended_signature_and_features, subleaf_t::main, ecx, 0x0000'0400ui32 } } },
	{ leaf_t::lightweight_profiling          , {         amd            , nullptr                        , print_lightweight_profiling          , { leaf_t::extended_signature_and_features, subleaf_t::main, ecx, 0x0000'8000ui32 } } },
	{ leaf_t::cache_properties               , {         amd            , enumerate_cache_properties     , print_cache_properties               , { leaf_t::extended_signature_and_features, subleaf_t::main, ecx, 0x0040'0000ui32 } } },
	{ leaf_t::extended_apic                  , {         amd            , nullptr                        , print_extended_apic                  , { leaf_t::extended_signature_and_features, subleaf_t::main, ecx, 0x0040'0000ui32 } } },
	{ leaf_t::encrypted_memory               , {         amd            , nullptr                        , print_encrypted_memory               , {} } }
};

void print_generic(fmt::Writer& w, const cpu_t& cpu, leaf_t leaf, subleaf_t subleaf) {
	const register_set_t& regs = cpu.leaves.at(leaf).at(subleaf);
	w.write("{:#010x} {:#010x} {:#010x}: {:#010x} {:#010x} {:#010x} {:#010x}\n", cpu.apic_id,
	                                                                             static_cast<std::uint32_t>(leaf),
	                                                                             static_cast<std::uint32_t>(subleaf),
	                                                                             regs[eax],
	                                                                             regs[ebx],
	                                                                             regs[ecx],
	                                                                             regs[edx]);

}

void print_generic(fmt::Writer& w, const cpu_t& cpu, leaf_t leaf) {
	for(const auto& sub : cpu.leaves.at(leaf)) {
		print_generic(w, cpu, leaf, sub.first);
	}
}

void print_generic(fmt::Writer& w, const cpu_t& cpu) {
	for(const auto& leaf : cpu.leaves) {
		print_generic(w, cpu, leaf.first);
	}
}

void enumerate_leaf_brute_force(cpu_t& cpu, leaf_t leaf) {
	register_set_t regs = { 0 };
	cpuid(regs, leaf, subleaf_t::main);
	cpu.leaves[leaf][subleaf_t::main] = regs;
	for(subleaf_t subleaf = subleaf_t{ 1 }; ; ++subleaf) {
		register_set_t previous = regs;
		regs = { 0 };
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
	}
}

void enumerate_leaf(cpu_t& cpu, leaf_t leaf, bool skip_vendor_check, bool skip_feature_check) {
	register_set_t regs = { 0 };
	const auto it = descriptors.find(leaf);
	if(it != descriptors.end()) {
		if(skip_vendor_check
		|| it->second.vendor & cpu.vendor) {
			const filter_t filter = it->second.filter;
			if(skip_feature_check
			|| filter      == no_filter
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

std::map<std::uint32_t, cpu_t> enumerate_file(const std::string& filename) {
	using namespace fmt::literals;
	const std::regex comment_line("#.*");
	const std::string single_element = "(0[xX][[:xdigit:]]{1,8})";
	const std::string multiple_elements = "{} {} {}: {} {} {} {}"_format(single_element, single_element, single_element, single_element, single_element, single_element, single_element);
	const std::regex data_line(multiple_elements);

	std::map<std::uint32_t, cpu_t> logical_cpus;
	std::ifstream fin(filename);
	std::string line;
	while(std::getline(fin, line)) {
		std::smatch m;
		if(std::regex_search(line, m, data_line)) {
			const std::uint32_t apic_id =                        std::stoul(m[1].str(), nullptr, 16) ;
			const leaf_t        leaf    = static_cast<leaf_t   >(std::stoul(m[2].str(), nullptr, 16));
			const subleaf_t     subleaf = static_cast<subleaf_t>(std::stoul(m[3].str(), nullptr, 16));
			const register_set_t regs   = {
				std::stoul(m[4].str(), nullptr, 16),
				std::stoul(m[5].str(), nullptr, 16),
				std::stoul(m[6].str(), nullptr, 16),
				std::stoul(m[7].str(), nullptr, 16)
			};
			logical_cpus[apic_id].leaves[leaf][subleaf] = regs;
		} else if(!std::regex_search(line, m, comment_line) && line != "") {
			std::cerr << "Unrecognized line: " << line << std::endl;
		}
	}
	for(auto& c: logical_cpus) {
		cpu_t& cpu = c.second;
		register_set_t regs = {};

		regs = cpu.leaves.at(leaf_t::basic_info).at(subleaf_t::main);
		cpu.vendor = get_vendor_from_name(regs);

		regs = cpu.leaves.at(leaf_t::version_info).at(subleaf_t::main);
		cpu.model = get_model(cpu.vendor, regs);

		if(cpu.leaves.find(leaf_t::hypervisor_limit) != cpu.leaves.end()) {
			regs = cpu.leaves.at(leaf_t::hypervisor_limit).at(subleaf_t::main);
			if(regs[eax] != 0ui32) {
				const vendor_t hypervisor = get_hypervisor_from_name(regs);
				// something is set, and it looks like a hypervisor
				if(hypervisor & any_hypervisor) {
					cpu.vendor = cpu.vendor | hypervisor;

					if(hypervisor & hyper_v) {
						// xen with viridian extensions masquerades as hyper-v, and puts its own cpuid leaves 0x100 further up
						if(cpu.leaves.find(leaf_t::xen_limit_offset) != cpu.leaves.end()) {
							regs = cpu.leaves.at(leaf_t::xen_limit_offset).at(subleaf_t::main);
							const vendor_t xen_hypervisor = get_hypervisor_from_name(regs);

							if(xen_hypervisor & xen_hvm) {
								cpu.vendor = cpu.vendor | xen_hypervisor;
							}
						}
					}
				}
			}
		}
		cpu.apic_id = get_apic_id(cpu);
	}
	return logical_cpus;
}

std::map<std::uint32_t, cpu_t> enumerate_processors(bool skip_vendor_check, bool skip_feature_check) {
	std::map<std::uint32_t, cpu_t> logical_cpus;
	run_on_every_core([=, &logical_cpus]() {
		cpu_t cpu = {};
		register_set_t regs = { 0 };

		cpuid(regs, leaf_t::basic_info, subleaf_t::main);
		const leaf_t highest_leaf = leaf_t{ regs[eax] };
		cpu.vendor = get_vendor_from_name(regs);

		cpuid(regs, leaf_t::version_info, subleaf_t::main);
		cpu.model = get_model(cpu.vendor, regs);

		for(leaf_t leaf = leaf_t::basic_info; leaf <= highest_leaf; ++leaf) {
			enumerate_leaf(cpu, leaf, skip_vendor_check, skip_feature_check);
		}

		cpuid(regs, leaf_t::hypervisor_limit, subleaf_t::main);
		if(regs[eax] != 0ui32) {
			const vendor_t hypervisor = get_hypervisor_from_name(regs);
			// something is set, and it looks like a hypervisor
			if(hypervisor & any_hypervisor) {
				cpu.vendor = cpu.vendor | hypervisor;
				const leaf_t highest_hypervisor_leaf = leaf_t{ regs[eax] };

				for(leaf_t leaf = leaf_t::hypervisor_limit; leaf <= highest_hypervisor_leaf; ++leaf) {
					enumerate_leaf(cpu, leaf, skip_vendor_check, skip_feature_check);
				}

				if(hypervisor & hyper_v) {
					// xen with viridian extensions masquerades as hyper-v, and puts its own cpuid leaves 0x100 further up
					cpuid(regs, leaf_t::xen_limit_offset, subleaf_t::main);
					const vendor_t xen_hypervisor = get_hypervisor_from_name(regs);

					if(xen_hypervisor & xen_hvm) {
						cpu.vendor                    = cpu.vendor | xen_hypervisor;
						const leaf_t xen_base         = leaf_t::xen_limit_offset;
						const leaf_t highest_xen_leaf = leaf_t{ regs[eax] };

						for(leaf_t leaf = xen_base; leaf <= highest_xen_leaf; ++leaf) {
							enumerate_leaf(cpu, leaf, skip_vendor_check, skip_feature_check);
						}
					}
				}
			}
		}
		cpuid(regs, leaf_t::extended_limit, subleaf_t::main);
		const leaf_t highest_extended_leaf = leaf_t{ regs[eax] };

		for(leaf_t leaf = leaf_t::extended_limit; leaf <= highest_extended_leaf; ++leaf) {
			enumerate_leaf(cpu, leaf, skip_vendor_check, skip_feature_check);
		}
		
		cpu.apic_id = get_apic_id(cpu);
		logical_cpus[cpu.apic_id] = cpu;
	});
	return logical_cpus;
}

static const char usage_message[] =
R"(cpuid.

	Usage:
		cpuid [--cpu <id> | --read-dump <filename>] [--dump] [--ignore-vendor] [--ignore-feature-bits]
		cpuid --list-ids
		cpuid --help
		cpuid --version

	Options:
		--cpu <id>              ID of logical core to print info from
		--read-dump <filename>  Filename to get info from
		--dump                  Print unparsed output
		--ignore-vendor         Ignore vendor constraints
		--ignore-feature-bits   Ignore feature bit constraints
		--list-ids              List all core IDs
		--help                  Show this text
		--version               Show the version
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
	const bool skip_vendor_check  = std::get<bool>(args.at("--ignore-vendor"));
	const bool skip_feature_check = std::get<bool>(args.at("--ignore-feature-bits"));
	const bool raw_dump           = std::get<bool>(args.at("--dump"));
	const bool list_ids           = std::get<bool>(args.at("--list-ids"));

	std::map<std::uint32_t, cpu_t> logical_cpus;
	if(std::holds_alternative<std::string>(args.at("--read-dump"))) {
		logical_cpus = enumerate_file(std::get<std::string>(args.at("--read-dump")));
	} else {
		logical_cpus = enumerate_processors(skip_vendor_check, skip_feature_check);
	}

	if(list_ids) {
		fmt::MemoryWriter w;
		for(const auto& p : logical_cpus) {
			w.write("{:04x}\n", p.first);
		}
		std::cout << w.str() << std::flush;
		return 0;
	}

	if(raw_dump) {
		fmt::MemoryWriter w;
		w.write("#apic eax ecx: eax ebx ecx edx");
		for(const auto& p : logical_cpus) {
			print_generic(w, p.second);
			w.write("\n");
		}
		std::cout << w.str() << std::flush;
		return 0;
	}

	if(logical_cpus.size() > 0) {
		const cpu_t& cpu = logical_cpus.begin()->second;
		for(const auto& leaf : cpu.leaves) {
			fmt::MemoryWriter w;
			const auto range = descriptors.equal_range(leaf.first);
			if(range.first != range.second) {
				for(auto it = range.first; it != range.second; ++it) {
					if(skip_vendor_check || (it->second.vendor & cpu.vendor)) {
						const filter_t filter = it->second.filter;
						if(skip_feature_check
						|| filter      == no_filter
						|| filter.mask == (filter.mask & cpu.leaves.at(filter.leaf).at(filter.subleaf).at(filter.reg))) {
							if(it->second.printer) {
								it->second.printer(w, cpu);
							} else {
								print_generic(w, cpu, leaf.first);
							}
						}
					}
				}
			} else {
				print_generic(w, cpu, leaf.first);
				w.write("\n");
			}
			std::cout << w.str() << std::flush;
		}

		fmt::MemoryWriter w;
		system_t machine = build_topology(logical_cpus);
		print_topology(w, machine);
		std::cout << w.str() << std::flush;
	} 

	return 0;
}

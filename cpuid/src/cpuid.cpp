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

#include <boost/algorithm/string.hpp>

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

void enumerate_leaf(cpu_t& cpu, leaf_t leaf, bool brute_force, bool skip_vendor_check, bool skip_feature_check) {
	register_set_t regs = { 0 };
	const auto it = descriptors.find(leaf);
	if(!brute_force
	&& it != descriptors.end()) {
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

enum struct file_format
{
	native,
	etallen,
	libcpuid,
	instlat
};

std::map<std::uint32_t, cpu_t> enumerate_file(std::istream& fin, file_format format) {
	std::map<std::uint32_t, cpu_t> logical_cpus;

	switch(format) {
	case file_format::native:
		{
			const std::regex comment_line("#.*");
			const std::string single_element = "(0[xX][[:xdigit:]]{1,8})";
			const std::string multiple_elements = fmt::format("{} {} {}: {} {} {} {}", single_element, single_element, single_element, single_element, single_element, single_element, single_element);
			const std::regex data_line(multiple_elements);

			std::string line;
			while(std::getline(fin, line)) {
				std::smatch m;
				if(std::regex_search(line, m, comment_line) || line == "") {
					continue;
				} else if(std::regex_search(line, m, data_line)) {
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
				} else {
					std::cerr << "Unrecognized line: " << line << std::endl;
				}
			}
		}
		break;
	case file_format::etallen:
		{
			const std::regex comment_line("#.*");
			const std::regex cpu_line("CPU ([[:digit:]]+)");
			const std::string single_element = "(0[xX][[:xdigit:]]{1,8})";
			const std::string multiple_elements = fmt::format("   {} {}: eax={} ebx={} ecx={} edx={}", single_element, single_element, single_element, single_element, single_element, single_element, single_element);
			const std::regex data_line(multiple_elements);
			std::string line;
			std::uint32_t current_cpu = 0xffff'ffffui32;
			while(std::getline(fin, line)) {
				std::smatch m;
				if(std::regex_search(line, m, comment_line) || line == "") {
					continue;
				} else if(std::regex_search(line, m, cpu_line)) {
					current_cpu = std::stoul(m[1].str());
				} else if(std::regex_search(line, m, data_line)) {
					const leaf_t         leaf    = static_cast<leaf_t   >(std::stoul(m[1].str(), nullptr, 16));
					const subleaf_t      subleaf = static_cast<subleaf_t>(std::stoul(m[2].str(), nullptr, 16));
					const register_set_t regs    = {
						std::stoul(m[3].str(), nullptr, 16),
						std::stoul(m[4].str(), nullptr, 16),
						std::stoul(m[5].str(), nullptr, 16),
						std::stoul(m[6].str(), nullptr, 16)
					};
					logical_cpus[current_cpu].leaves[leaf][subleaf] = regs;
				} else {
					std::cerr << "Unrecognized line: " << line << std::endl;
				}
			}
		}
		break;
	case file_format::libcpuid:
		{
			// this is a crappy file format
			const std::regex version_line("version=.*");
			const std::regex build_line("build_date=.*");
			const std::regex delimiter_line("-{80}");
			const std::string single_element = "([[:xdigit:]]{8})";
			const std::string multiple_elements = fmt::format("([[:alnum:]_]+)\\[([[:digit:]]+)\\]={} {} {} {}", single_element, single_element, single_element, single_element);
			const std::regex data_line(multiple_elements);
			const std::uint32_t current_cpu = 0xffff'ffffui32;
			std::string line;
			while(std::getline(fin, line)) {
				std::smatch m;
				if(std::regex_search(line, m, version_line)
				|| std::regex_search(line, m, build_line)) {
					continue;
				} else if(std::regex_search(line, m, data_line)) {
					std::string section       = m[1].str();
					std::uint32_t idx         = std::stoul(m[2].str());
					const register_set_t regs = {
						std::stoul(m[3].str(), nullptr, 16),
						std::stoul(m[4].str(), nullptr, 16),
						std::stoul(m[5].str(), nullptr, 16),
						std::stoul(m[6].str(), nullptr, 16)
					};
					leaf_t       leaf{ idx };
					subleaf_t subleaf{ idx };
					if(section == "basic_cpuid") {
						subleaf = subleaf_t::main;
					} else if(section == "ext_cpuid") {
						leaf = static_cast<leaf_t>(0x8000'0000ui32 + idx);
						subleaf = subleaf_t::main;
					} else if(section == "intel_fn4") {
						leaf = leaf_t::deterministic_cache;
						if((regs[eax] & 0x0000'001fui32) == 0) {
							continue;
						}
					} else if(section == "intel_fn11") {
						leaf = leaf_t::extended_topology;
						if((regs[ecx] & 0x0000'ff00ui32) == 0ui32) {
							continue;
						}
					} else if(section == "intel_fn12h") {
						leaf = leaf_t::sgx_info;
						if((regs[eax] & 0x0000'000fui32) == 0ui32) {
							continue;
						}
					} else if(section == "intel_fn14h") {
						leaf = leaf_t::processor_trace;
					}
					logical_cpus[current_cpu].leaves[leaf][subleaf] = regs;
				} else if(std::regex_search(line, m, delimiter_line)) {
					break;
				} else {
					std::cerr << "Unrecognized line: " << line << std::endl;
				}
			}
			const leaf_t highest_leaf          = leaf_t{ logical_cpus[current_cpu].leaves[leaf_t::basic_info    ][subleaf_t::main][eax] };
			const leaf_t highest_extended_leaf = leaf_t{ logical_cpus[current_cpu].leaves[leaf_t::extended_limit][subleaf_t::main][eax] };
			leaves_t corrected_leaves;
			for(leaf_t leaf = leaf_t::basic_info; leaf <= highest_leaf; ++leaf) {
				corrected_leaves[leaf] = logical_cpus[current_cpu].leaves[leaf];
			}
			for(leaf_t leaf = leaf_t::extended_limit; leaf <= highest_extended_leaf; ++leaf) {
				corrected_leaves[leaf] = logical_cpus[current_cpu].leaves[leaf];
			}
			if(corrected_leaves.find(leaf_t::processor_trace) != corrected_leaves.end()) {
				subleaves_t subleaves = corrected_leaves.at(leaf_t::processor_trace);
				const subleaf_t limit = subleaf_t{ subleaves[subleaf_t::main][eax] };
				for(subleaf_t sub = subleaf_t{ 1 }; sub < limit; ++sub) {
					if(subleaves.find(sub) != subleaves.end()) {
						subleaves[sub] = { 0x0ui32, 0x0ui32, 0x0ui32, 0x0ui32 };
					}
				}
				subleaves.erase(subleaves.lower_bound(limit), subleaves.end());
			}
			logical_cpus[current_cpu].leaves.swap(corrected_leaves);
		}
		break;
	case file_format::instlat:
		{
			const std::string single_element = "([[:xdigit:]]{8})";
			const std::string simple = fmt::format("CPUID {}: {}-{}-{}-{}", single_element, single_element, single_element, single_element, single_element);
			const std::regex simple_line(simple);
			const std::regex subleaf_line(fmt::format("{} \\[SL ([[:digit:]]{{2}})\\]", simple));
			const std::regex description_line(fmt::format("{} \\[.*\\]", simple));
			const std::regex allcpu_line("allcpu:.*");
			const std::regex affinity_mask_line("CPU#.*");
			const std::regex registers_line("CPUID Registers.*");

			std::string line;
			std::uint32_t current_cpu = 0xffff'ffffui32;
			while(std::getline(fin, line)) {
				std::smatch m;
				if(std::regex_search(line, m, allcpu_line)
				|| std::regex_search(line, m, affinity_mask_line)
				|| std::regex_search(line, m, registers_line)) {
					++current_cpu;
				} else if(std::regex_search(line, m, subleaf_line)) {
					const leaf_t         leaf    = static_cast<leaf_t   >(std::stoul(m[1].str(), nullptr, 16));
					const subleaf_t      subleaf = static_cast<subleaf_t>(std::stoul(m[6].str(), nullptr, 10));
					const register_set_t regs    = {
						std::stoul(m[2].str(), nullptr, 16),
						std::stoul(m[3].str(), nullptr, 16),
						std::stoul(m[4].str(), nullptr, 16),
						std::stoul(m[5].str(), nullptr, 16)
					};
					logical_cpus[current_cpu].leaves[leaf][subleaf] = regs;
				} else if(std::regex_search(line, m, description_line)
				       || std::regex_search(line, m, simple_line)) {
					const auto get_subleaf = [&logical_cpus](const std::uint32_t current_cpu, const leaf_t leaf) -> subleaf_t {
						if(logical_cpus.find(current_cpu) != logical_cpus.end()
						&& logical_cpus[current_cpu].leaves.find(leaf) != logical_cpus[current_cpu].leaves.end()
						&& logical_cpus[current_cpu].leaves[leaf].find(subleaf_t::main) != logical_cpus[current_cpu].leaves[leaf].end()) {
							return static_cast<subleaf_t>(gsl::narrow_cast<std::uint32_t>(logical_cpus[current_cpu].leaves[leaf].size()) + 1ui32);
						} else {
							return subleaf_t::main;
						}
					};
					const leaf_t         leaf    = static_cast<leaf_t   >(std::stoul(m[1].str(), nullptr, 16));
					if(leaf == leaf_t::basic_info
					&& logical_cpus.find(current_cpu) != logical_cpus.end()
					&& logical_cpus[current_cpu].leaves.find(leaf) != logical_cpus[current_cpu].leaves.end()) {
						++current_cpu;
					}
					const subleaf_t      subleaf = get_subleaf(current_cpu, leaf);
					const register_set_t regs    = {
						std::stoul(m[2].str(), nullptr, 16),
						std::stoul(m[3].str(), nullptr, 16),
						std::stoul(m[4].str(), nullptr, 16),
						std::stoul(m[5].str(), nullptr, 16)
					};
					logical_cpus[current_cpu].leaves[leaf][subleaf] = regs;
				} else {
					std::cerr << "ignoring line:" << line << std::endl;
				}
			}
		}
		break;
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
	if(format != file_format::native) {
		std::map<std::uint32_t, cpu_t> corrected_ids;
		for(auto& p : logical_cpus) {
			if(corrected_ids.find(p.second.apic_id) != corrected_ids.end()) {
				// duplicate APIC IDs: this happens on old multisocket systems
				// so let's just synthesize some garbage APIC IDs
				p.second.apic_id = gsl::narrow<std::uint32_t>(corrected_ids.size());
			}
			corrected_ids[p.second.apic_id] = p.second;
		}
		logical_cpus.swap(corrected_ids);
	}
	return logical_cpus;
}

std::map<std::uint32_t, cpu_t> enumerate_processors(bool brute_force, bool skip_vendor_check, bool skip_feature_check) {
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
			enumerate_leaf(cpu, leaf, brute_force, skip_vendor_check, skip_feature_check);
		}

		cpuid(regs, leaf_t::hypervisor_limit, subleaf_t::main);
		if(regs[eax] != 0ui32) {
			const vendor_t hypervisor = get_hypervisor_from_name(regs);
			// something is set, and it looks like a hypervisor
			if(hypervisor & any_hypervisor) {
				cpu.vendor = cpu.vendor | hypervisor;
				const leaf_t highest_hypervisor_leaf = leaf_t{ regs[eax] };

				for(leaf_t leaf = leaf_t::hypervisor_limit; leaf <= highest_hypervisor_leaf; ++leaf) {
					enumerate_leaf(cpu, leaf, brute_force, skip_vendor_check, skip_feature_check);
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
							enumerate_leaf(cpu, leaf, brute_force, skip_vendor_check, skip_feature_check);
						}
					}
				}
			}
		}
		cpuid(regs, leaf_t::extended_limit, subleaf_t::main);
		const leaf_t highest_extended_leaf = leaf_t{ regs[eax] };

		for(leaf_t leaf = leaf_t::extended_limit; leaf <= highest_extended_leaf; ++leaf) {
			enumerate_leaf(cpu, leaf, brute_force, skip_vendor_check, skip_feature_check);
		}

		cpu.apic_id = get_apic_id(cpu);
		logical_cpus[cpu.apic_id] = cpu;
	});
	return logical_cpus;
}

void print_single_flag(fmt::Writer& w, const cpu_t& cpu, const std::string& flag_spec) {
	const std::vector<std::string> samples = {
		"CPUID.01:ECX[SSE4.2]",
		"CPUID.01:ECX.MOVBE[bit 22]",
		"CPUID.01H.EDX.SSE[bit 25]",
		"CPUID.(EAX=07H, ECX=0H):EBX.BMI1[bit 3]",
		"CPUID.EAX=80000001H:ECX.LZCNT[bit 5]",
		"CPUID.(EAX=07H, ECX=0H):EBX[bit 9]",
		"CPUID.(EAX=0DH,ECX=0):EAX[4:3]",
		"CPUID.(EAX=0DH,ECX=0):EAX[9]",
		"CPUID.1:ECX.OSXSAVE[bit 27]",
		"CPUID.1:ECX.OSXSAVE",
		"CPUID.(EAX=0DH,ECX=0):EBX",
		"CPUID.0x7.0:EBX.AVX512PF[bit 26]",
		"CPUID.(EAX=0DH, ECX=04H).EBX[31:0]",
		"CPUID.(EAX=07H,ECX=0H):ECX.MAWAU[bits 21:17]",
		"CPUID.(EAX=07H, ECX=0H).EBX.MPX ",
		"CPUID.1.ECX",
		"CPUID.(EAX=07H, ECX=0H):EBX[SGX]",
		"CPUID.80000008H:EAX[7:0]",
		"CPUID.1.EBX[23:16]",
		"CPUID.(EAX=07H, ECX=0H):EBX.INVPCID (bit 10)",
		"CPUID.80000001H:ECX.LAHF-SAHF[bit 0]",
		"CPUID.01H:ECX.POPCNT [Bit 23]",
		"CPUID.(EAX=0DH,ECX=1):EAX.XSS[bit 3]",
		"CPUID.80000008H:EAX[bits 7-0]",
	};

	const std::string hex_number       = "(?:0x)?([[:xdigit:]]+)H?";

	const std::string simple_selector  = "(?:(?:EAX=)?{hex_number:s})";
	const std::string complex_selector = R"((?:(?:\(EAX={hex_number:s}, ?ECX={hex_number:s}\))|(?:{hex_number:s}\.{hex_number:s})))";
	const std::string selector         = "(?:{simple_selector:s}|{complex_selector:s})";
	const std::string full_selector    = fmt::format(fmt::format(selector, fmt::arg("simple_selector", simple_selector),
	                                                                       fmt::arg("complex_selector", complex_selector)), fmt::arg("hex_number", hex_number));

	const std::string reg              = "(EAX|EBX|ECX|EDX)";

	const std::string flagname         = "([[:alnum:]\\._-]+)";
	const std::string single_bit       = "(?:bit )?([[:digit:]]+)";
	const std::string bit_range        = "(?:bits )?([[:digit:]]+)[-:]([[:digit:]]+)";
	const std::string bitspec          = "(?:{single_bit:s}|{bit_range:s})";
	const std::string full_bitspec     = fmt::format(bitspec, fmt::arg("single_bit", single_bit), fmt::arg("bit_range", bit_range));
	const std::string open_bracket     = R"((?:\(|\[))";
	const std::string close_bracket    = R"((?:\)|\]))";
	const std::string field            = R"((?:(\.{flagname:s} ?{open_bracket:s}{bitspec:s}{close_bracket:s})|(?:\.{flagname:s})|({open_bracket:s}{bitspec:s}{close_bracket:s})|(?:{open_bracket:s}{flagname:s}{close_bracket:s}))?)";
	const std::string full_field       = fmt::format(field, fmt::arg("flagname", flagname), fmt::arg("bitspec", full_bitspec), fmt::arg("open_bracket", open_bracket), fmt::arg("close_bracket", close_bracket));

	const std::string pattern          = R"(CPUID\.{selector:s}(?:\.|:){reg:s}{field:s})";

	const std::string full_pattern     = fmt::format(pattern, fmt::arg("selector", full_selector), fmt::arg("reg", reg), fmt::arg("field", full_field));

	std::regex re_pattern(full_pattern, std::regex::optimize | std::regex::icase);
	std::smatch m;
	if(!std::regex_search(flag_spec, m, re_pattern)) {
		throw std::runtime_error(fmt::format("Bad pattern: {:s}", flag_spec));
	}

	std::uint32_t selector_eax  = 0ui32;
	std::uint32_t selector_ecx  = 0ui32;
	register_t    flag_register = eax;
	std::string   flag_name     = "";
	std::uint32_t flag_start    = 0xffff'ffffui32;
	std::uint32_t flag_end      = 0xffff'ffffui32;

	if(m[1].matched) {
		selector_eax = std::stoul(m[1].str(), nullptr, 16);
	} else if(m[2].matched) {
		selector_eax = std::stoul(m[2].str(), nullptr, 16);
		selector_ecx = m[3].matched ? std::stoul(m[3].str(), nullptr, 16) : 0ui32;
	} else if(m[4].matched) {
		selector_eax = std::stoul(m[4].str(), nullptr, 16);
		selector_ecx = m[5].matched ? std::stoul(m[5].str(), nullptr, 16) : 0ui32;
	}

	const std::string reg_name = boost::algorithm::to_lower_copy(m[6].str());
	if(reg_name == "eax") {
		flag_register = eax;
	} else if(reg_name == "ebx") {
		flag_register = ebx;
	} else if(reg_name == "ecx") {
		flag_register = ecx;
	} else if(reg_name == "edx") {
		flag_register = edx;
	}

	if(m[7].matched) {
		flag_name = m[8].str();
		if(m[9].matched) {
			flag_start = std::stoul(m[9].str());
			flag_end   = flag_start;
		} else if(m[10].matched) {
			flag_start = std::stoul(m[11].str());
			flag_end   = std::stoul(m[10].str());
		}
	} else if(m[12].matched) {
		flag_name = m[12].str();
	} else if(m[13].matched) {
		if(m[14].matched) {
			flag_start = std::stoul(m[14].str());
			flag_end   = flag_start;
		} else if(m[15].matched) {
			flag_start = std::stoul(m[16].str());
			flag_end   = std::stoul(m[15].str());
		}
	}
	else if(m[17].matched) {
		flag_name = m[17].str();
	}
	boost::algorithm::to_lower(flag_name);
	const std::string flag_name_alternative = boost::algorithm::replace_all_copy(flag_name, "_", ".");

	const leaf_t    leaf    = static_cast<leaf_t   >(selector_eax);
	const subleaf_t subleaf = static_cast<subleaf_t>(selector_ecx);
	bool handled = false;
	if(cpu.leaves.find(leaf) != cpu.leaves.end()
	&& cpu.leaves.at(leaf).find(subleaf) != cpu.leaves.at(leaf).end()) {
		const std::uint32_t value = cpu.leaves.at(leaf).at(subleaf).at(flag_register);
		if(flag_name == "" && flag_start == 0xffff'ffffui32 && flag_end == 0xffff'ffffui32) {
			w.write("cpu {:#04x} {:s}: {:#010x}\n", cpu.apic_id, flag_spec, value);
			handled = true;
		}
		if(flag_name != "") {
			const auto range = all_features.equal_range(leaf);
			for(auto it = range.first; it != range.second; ++it) {
				if(it->second.find(subleaf) == it->second.end()) {
					continue;
				}
				auto sub = it->second.at(subleaf);
				if(sub.find(flag_register) == sub.end()) {
					continue;
				}
				for(const feature_t& feature : sub.at(flag_register)) {
					const std::string lower_mnemonic = boost::algorithm::to_lower_copy(feature.mnemonic);
					if(lower_mnemonic == flag_name
					|| lower_mnemonic == flag_name_alternative) {
						DWORD shift_amount = 0;
						_BitScanForward(&shift_amount, feature.mask);
						const std::uint32_t result = (value & feature.mask) >> shift_amount;

						w.write("cpu {:#04x} {:s}: {:#010x}\n", cpu.apic_id, flag_spec, result);
						handled = true;
						break;
					}
				}
			}
		}
		if(!handled && flag_start != 0xffff'ffffui32 && flag_end != 0xffff'ffffui32) {
			const std::uint32_t mask = range_mask(flag_start, flag_end);
			DWORD shift_amount = 0;
			_BitScanForward(&shift_amount, mask);
			const std::uint32_t result = (value & mask) >> shift_amount;
			w.write("cpu {:#04x} {:s}: {:#010x}\n", cpu.apic_id, flag_spec, result);
			handled = true;
		}
	}

	if(!handled) {
		w.write("No data found for {:s}\n", flag_spec);
	}
}

void print_leaves(fmt::Writer& w, const cpu_t& cpu, bool skip_vendor_check, bool skip_feature_check) {
	for(const auto& leaf : cpu.leaves) {
		const auto range = descriptors.equal_range(leaf.first);
		if(range.first != range.second) {
			for(auto it = range.first; it != range.second; ++it) {
				if(skip_vendor_check || (it->second.vendor & cpu.vendor)) {
					const filter_t filter = it->second.filter;
					if(skip_feature_check
						|| filter == no_filter
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
	}
}

void print_dump(fmt::Writer& w, std::map<std::uint32_t, cpu_t> logical_cpus, file_format format) {
	switch(format) {
	case file_format::native:
		w.write("#apic eax ecx: eax ebx ecx edx\n");
		for(const auto& p : logical_cpus) {
			print_generic(w, p.second);
			w.write("\n");
		}
		break;
	case file_format::etallen:
		{
			std::uint32_t count = 0ui32;
			for(const auto& c : logical_cpus) {
				w.write("CPU {:d}:\n", count);
				for(const auto& l : c.second.leaves) {
					for(const auto& s : l.second) {
						const cpu_t& cpu = c.second;
						const leaf_t leaf = l.first;
						const subleaf_t subleaf = s.first;
						const register_set_t& regs = cpu.leaves.at(leaf).at(subleaf);
						w.write("   {:#010x} {:#04x}: eax={:#010x} ebx={:#010x} ecx={:#010x} edx={:#010x}\n", static_cast<std::uint32_t>(leaf),
						                                                                                      static_cast<std::uint32_t>(subleaf),
						                                                                                      regs[eax],
						                                                                                      regs[ebx],
						                                                                                      regs[ecx],
						                                                                                      regs[edx]);
					}
				}
				++count;
			}
		}
		break;
	case file_format::libcpuid:
		{
			// this is a crappy file format
			w.write("version=0.4.0\n");
			const cpu_t& cpu = logical_cpus.begin()->second;

			for(std::uint32_t i = 0ui32; i < 32ui32; ++i) {
				const leaf_t leaf{ i };
				if(cpu.leaves.find(leaf) != cpu.leaves.end()) {
					const register_set_t& regs = cpu.leaves.at(leaf).at(subleaf_t::main);
					w.write("basic_cpuid[{:d}]={:08x} {:08x} {:08x} {:08x}\n", i, regs[eax], regs[ebx], regs[ecx], regs[edx]);
				} else {
					w.write("basic_cpuid[{:d}]={:08x} {:08x} {:08x} {:08x}\n", i, 0ui32, 0ui32, 0ui32, 0ui32);
				}
			}
			for(std::uint32_t i = 0ui32; i < 32ui32; ++i) {
				const leaf_t leaf{ i + 0x8000'0000ui32 };
				if(cpu.leaves.find(leaf) != cpu.leaves.end()) {
					const register_set_t& regs = cpu.leaves.at(leaf).at(subleaf_t::main);
					w.write("ext_cpuid[{:d}]={:08x} {:08x} {:08x} {:08x}\n", i, regs[eax], regs[ebx], regs[ecx], regs[edx]);
				} else {
					w.write("ext_cpuid[{:d}]={:08x} {:08x} {:08x} {:08x}\n", i, 0ui32, 0ui32, 0ui32, 0ui32);
				}
			}

			const auto print_detailed_leaves = [&w, &cpu](const leaf_t leaf, const std::uint32_t limit, const std::string& label) {
				if(cpu.leaves.find(leaf) != cpu.leaves.end()) {
					const subleaves_t& subleaves = cpu.leaves.at(leaf);
					std::uint32_t i = 0ui32;
					for(const auto& s : subleaves) {
						const register_set_t& regs = s.second;
						w.write("{:s}[{:d}]={:08x} {:08x} {:08x} {:08x}\n", label, i, regs[eax], regs[ebx], regs[ecx], regs[edx]);
						++i;
					}
					for(; i < limit; ++i) {
						w.write("{:s}[{:d}]={:08x} {:08x} {:08x} {:08x}\n", label, i, 0ui32, 0ui32, 0ui32, 0ui32);
					}
				}
			};

			print_detailed_leaves(leaf_t::deterministic_cache, 8, "intel_fn4");
			print_detailed_leaves(leaf_t::extended_topology, 4, "intel_fn11");
			print_detailed_leaves(leaf_t::sgx_info, 4, "intel_fn12h");
			print_detailed_leaves(leaf_t::processor_trace, 4, "intel_fn14h");
		}
		break;
	case file_format::instlat:
		break;
	}
}

static const char usage_message[] =
R"(cpuid.

Usage:
	cpuid [--read-dump <filename>] [--read-format <format>] [--all-cpus | --cpu <id>] [--ignore-vendor] [--ignore-feature-bits] [--brute-force] [--raw] [--write-dump <filename>] [--write-format <format>] [--single-value <spec>] [--no-topology | --only-topology]
	cpuid --list-ids [--read-dump <filename>] [--read-format <format>]
	cpuid --help
	cpuid --version

Input options:
	--read-dump=<filename>     Read from <filename> rather than the current processors
	--read-format=<format>     Dump format to read: native, etallen, libcpuid, instlat. [default: native]
	--all-cpus                 Show output from every CPU
	--cpu <id>                 Show output from CPU with APIC ID <id>
	--single-value <spec>      Print specific flag value, using Intel syntax (e.g. CPUID.01H.EDX.SSE[bit 25]).
	                           Handles most of the wild inconsistencies found in Intel's documentation.
	--ignore-vendor            Ignore vendor constraints
	--ignore-feature-bits      Ignore feature bit constraints
	--brute-force              Ignore constraints, and enumerate even reserved leaves

Output options:
	--raw                      Write unparsed output to screen
	--write-dump=<filename>    Write unparsed output to <filename>
	--write-format=<format>    Dump format to write: native, etallen, libcpuid, instlat. [default: native]
	--no-topology              Don't print the processor and cache topology
	--only-topology            Only print the processor and cache topology
	--list-ids                 List all core IDs

Other options:
	--help                     Show this text
	--version                  Show the version

)";

static const char version[] = "cpuid 0.1";

int main(int argc, char* argv[]) try {
	HANDLE output = ::GetStdHandle(STD_OUTPUT_HANDLE);
	DWORD mode = 0;
	::GetConsoleMode(output, &mode);
	mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
	::SetConsoleMode(output, mode);
	::SetConsoleCP(CP_UTF8);
	::SetConsoleOutputCP(CP_UTF8);

	std::cout.rdbuf()->pubsetbuf(nullptr, 1024);

	const std::map<std::string, docopt::value> args = docopt::docopt_parse(usage_message, { argv + 1, argv + argc }, true, true);
	const bool skip_vendor_check  = std::get<bool>(args.at("--ignore-vendor"));
	const bool skip_feature_check = std::get<bool>(args.at("--ignore-feature-bits"));
	const bool raw_dump           = std::get<bool>(args.at("--raw"));
	const bool all_cpus           = std::get<bool>(args.at("--all-cpus"));
	const bool list_ids           = std::get<bool>(args.at("--list-ids"));
	const bool brute_force        = std::get<bool>(args.at("--brute-force"));
	const bool no_topology        = std::get<bool>(args.at("--no-topology"));
	const bool only_topology      = std::get<bool>(args.at("--only-topology"));

	std::map<std::uint32_t, cpu_t> logical_cpus;
	if(std::holds_alternative<std::string>(args.at("--read-dump"))) {
		file_format format = file_format::native;
		const std::string format_name = boost::to_lower_copy(std::get<std::string>(args.at("--read-format")));
		if("etallen" == format_name) {
			format = file_format::etallen;
		} else if("libcpuid" == format_name) {
			format = file_format::libcpuid;
		} else if("instlat" == format_name) {
			format = file_format::instlat;
		}
		const std::string filename = std::get<std::string>(args.at("--read-dump"));
		std::ifstream fin;
		if(filename != "-") {
			fin.open(filename);
			if(!fin) {
				throw std::runtime_error(fmt::format("Could not open {:s} for input", filename));
			}
		}
		logical_cpus = enumerate_file(filename != "-" ? fin : std::cin, format);
	} else {
		logical_cpus = enumerate_processors(brute_force, skip_vendor_check, skip_feature_check);
	}

	if(logical_cpus.size() == 0) {
		throw std::runtime_error("No processors found, which is implausible.");
	}

	if(list_ids) {
		fmt::MemoryWriter w;
		for(const auto& p : logical_cpus) {
			w.write("{:#04x}\n", p.first);
		}
		std::cout << w.str() << std::flush;
		return 0;
	}

	if(raw_dump || std::holds_alternative<std::string>(args.at("--write-dump"))) {
		file_format format = file_format::native;
		const std::string format_name = boost::to_lower_copy(std::get<std::string>(args.at("--write-format")));
		if("etallen" == format_name) {
			format = file_format::etallen;
		} else if("libcpuid" == format_name) {
			format = file_format::libcpuid;
		} else if("instlat" == format_name) {
			format = file_format::instlat;
		}
		fmt::MemoryWriter w;
		print_dump(w, logical_cpus, format);
		std::string filename = "-";
		if(std::holds_alternative<std::string>(args.at("--write-dump"))) {
			filename = std::get<std::string>(args.at("--write-dump"));
		}
		std::ofstream fout;
		if(filename != "-") {
			fout.open(filename);
			if(!fout) {
				throw std::runtime_error(fmt::format("Could not open {:s} for output", filename));
			}
		}
		(filename != "-" ? fout : std::cout) << w.str() << std::flush;
		return 0;
	}

	std::vector<std::uint32_t> cpu_ids;

	if(all_cpus) {
		for(const auto& p : logical_cpus) {
			cpu_ids.push_back(p.second.apic_id);
		}
	} else {
		const std::uint32_t chosen_id = std::holds_alternative<std::string>(args.at("--cpu")) ? std::stoul(std::get<std::string>(args.at("--cpu")), nullptr, 16)
		                                                                                      : logical_cpus.begin()->first;
		if(logical_cpus.find(chosen_id) == logical_cpus.end()) {
			throw std::runtime_error(fmt::format("No such CPU ID: {:#04x}\n", chosen_id));
		}
		cpu_ids.push_back(chosen_id);
	}

	if(!only_topology) {
		for(const std::uint32_t chosen_id : cpu_ids) {
			const cpu_t& cpu = logical_cpus.at(chosen_id);
			fmt::MemoryWriter w;
			if(std::holds_alternative<std::string>(args.at("--single-value"))) {
				const std::string flag_spec = std::get<std::string>(args.at("--single-value"));
				print_single_flag(w, cpu, flag_spec);
			} else {
				print_leaves(w, cpu, skip_vendor_check, skip_feature_check);
			}
			std::cout << w.str() << std::flush;
		}
	}

	if(!std::holds_alternative<std::string>(args.at("--single-value"))
	&& !no_topology) {
		fmt::MemoryWriter w;
		system_t machine = build_topology(logical_cpus);
		print_topology(w, machine);
		std::cout << w.str() << std::flush;
	}

	return EXIT_SUCCESS;
} catch(const docopt::exit_help&) {
	std::cout << usage_message << std::endl;
	return EXIT_SUCCESS;
} catch(const docopt::exit_version&) {
	std::cout << version << std::endl;
	return EXIT_SUCCESS;
} catch(const docopt::language_error& e) {
	std::cerr << "Docopt usage string could not be parsed" << std::endl;
	std::cerr << e.what() << std::endl;
	return EXIT_FAILURE;
} catch(const docopt::argument_error& e) {
	std::cerr << e.what() << std::endl;
	std::cerr << usage_message << std::endl;
	return EXIT_FAILURE;
} catch(std::exception& e) {
	std::cerr << e.what() << std::endl;
	return EXIT_FAILURE;
}

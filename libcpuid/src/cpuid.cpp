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

template<std::size_t N, std::size_t... Is>
constexpr std::array<char, N - 1> to_array(const char(&str)[N], std::index_sequence<Is...>) {
	return { str[Is]... };
}

template<std::size_t N>
constexpr std::array<char, N - 1> to_array(const char(&str)[N]) {
	return to_array(str, std::make_index_sequence<N - 1>{});
}

vendor_type get_vendor_from_name(const register_set_t& regs) {
	static const std::map<std::array<char, 12>, vendor_type> vendors = {
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

vendor_type get_hypervisor_from_name(const register_set_t& regs) {
	static const std::map<std::array<char, 12>, vendor_type> vendors = {
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

model_t get_model(vendor_type vendor, const register_set_t& regs) noexcept {
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
				model.model += (a.split.extended_model << 4_u32);
			}
		}
		break;
	case amd:
		{
			model.family += a.split.extended_family;
			model.model += a.split.extended_model << 4_u32;
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
		if(cpu.leaves.find(leaf_type::extended_topology) != cpu.leaves.end()) {
			return cpu.leaves.at(leaf_type::extended_topology).at(subleaf_type::main).at(edx);
		}
		break;
	case amd:
		if(cpu.leaves.find(leaf_type::extended_apic) != cpu.leaves.end()) {
			return cpu.leaves.at(leaf_type::extended_apic).at(subleaf_type::main).at(eax);
		}
		break;
	default:
		break;
	}
	return get_local_apic_id(cpu.leaves.at(leaf_type::version_info).at(subleaf_type::main));
}

using leaf_print = void(*)(fmt::memory_buffer& out, const cpu_t& cpu);
using leaf_enumerate = void(*)(cpu_t& cpu);

struct filter_t
{
	leaf_type leaf;
	subleaf_type subleaf;
	register_type reg;
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
	vendor_type vendor;
	leaf_enumerate enumerator;
	leaf_print printer;
	filter_t filter;
};

void enumerate_null(cpu_t&) noexcept {
}

void print_null(fmt::memory_buffer&, const cpu_t&) noexcept {
}

const std::multimap<leaf_type, leaf_descriptor_t> descriptors = {
	{ leaf_type::basic_info                     , { any                    , nullptr                        , print_basic_info                     , {} } },
	{ leaf_type::version_info                   , { any                    , nullptr                        , print_version_info                   , {} } },
	{ leaf_type::cache_and_tlb                  , { intel                  , nullptr                        , print_cache_tlb_info                 , {} } },
	{ leaf_type::serial_number                  , { intel       | transmeta, nullptr                        , print_serial_number                  , { leaf_type::version_info                   , subleaf_type::main, edx, 0x0004'0000_u32 } } },
	{ leaf_type::deterministic_cache            , { intel                  , enumerate_deterministic_cache  , print_deterministic_cache            , {} } },
	{ leaf_type::monitor_mwait                  , { intel | amd            , nullptr                        , print_mwait_parameters               , { leaf_type::version_info                   , subleaf_type::main, ecx, 0x0000'0008_u32 } } },
	{ leaf_type::thermal_and_power              , { intel | amd            , nullptr                        , print_thermal_and_power              , {} } },
	{ leaf_type::extended_features              , { any                    , enumerate_extended_features    , print_extended_features              , {} } },
	{ leaf_type::reserved_1                     , { any                    , enumerate_null                 , print_null                           , {} } },
	{ leaf_type::direct_cache_access            , { intel                  , nullptr                        , print_direct_cache_access            , { leaf_type::version_info                   , subleaf_type::main, ecx, 0x0004'0000_u32 } } },
	{ leaf_type::performance_monitoring         , { intel                  , nullptr                        , print_performance_monitoring         , { leaf_type::version_info                   , subleaf_type::main, ecx, 0x0000'8000_u32 } } },
	{ leaf_type::extended_topology              , { intel                  , enumerate_extended_topology    , print_extended_topology              , {} } },
	{ leaf_type::reserved_2                     , { any                    , enumerate_null                 , print_null                           , {} } },
	{ leaf_type::extended_state                 , { intel | amd            , enumerate_extended_state       , print_extended_state                 , { leaf_type::version_info                   , subleaf_type::main, ecx, 0x0400'0000_u32 } } },
	{ leaf_type::reserved_3                     , { any                    , enumerate_null                 , print_null                           , {} } },
	{ leaf_type::rdt_monitoring                 , { intel                  , enumerate_rdt_monitoring       , print_rdt_monitoring                 , { leaf_type::extended_features              , subleaf_type::main, ebx, 0x0000'1000_u32 } } },
	{ leaf_type::rdt_allocation                 , { intel                  , enumerate_rdt_allocation       , print_rdt_allocation                 , { leaf_type::extended_features              , subleaf_type::main, ebx, 0x0000'8000_u32 } } },
	{ leaf_type::reserved_4                     , { any                    , enumerate_null                 , print_null                           , {} } },
	{ leaf_type::sgx_info                       , { intel                  , enumerate_sgx_info             , print_sgx_info                       , { leaf_type::extended_features              , subleaf_type::main, ebx, 0x0000'0004_u32 } } },
	{ leaf_type::reserved_5                     , { any                    , enumerate_null                 , print_null                           , {} } },
	{ leaf_type::processor_trace                , { intel                  , enumerate_processor_trace      , print_processor_trace                , { leaf_type::extended_features              , subleaf_type::main, ebx, 0x0200'0000_u32 } } },
	{ leaf_type::time_stamp_counter             , { intel                  , nullptr                        , print_time_stamp_counter             , {} } },
	{ leaf_type::processor_frequency            , { intel                  , nullptr                        , print_processor_frequency            , {} } },
	{ leaf_type::system_on_chip_vendor          , { intel                  , enumerate_system_on_chip_vendor, print_system_on_chip_vendor          , {} } },
	{ leaf_type::deterministic_tlb              , { intel                  , enumerate_deterministic_tlb    , print_deterministic_tlb              , {} } },
	{ leaf_type::reserved_6                     , { any                    , enumerate_null                 , print_null                           , {} } },
	{ leaf_type::reserved_7                     , { any                    , enumerate_null                 , print_null                           , {} } },
	{ leaf_type::pconfig                        , { any                    , enumerate_pconfig              , print_pconfig                        , { leaf_type::extended_features              , subleaf_type::main, edx, 0x0004'0000_u32 } } },

	{ leaf_type::hypervisor_limit               , { any                    , nullptr                        , print_hypervisor_limit               , {} } },
	{ leaf_type::hyper_v_signature              , { hyper_v                , nullptr                        , print_hyper_v_signature              , {} } },
	{ leaf_type::hyper_v_system_identity        , { hyper_v                , nullptr                        , print_hyper_v_system_identity        , {} } },
	{ leaf_type::hyper_v_features               , { hyper_v                , nullptr                        , print_hyper_v_features               , {} } },
	{ leaf_type::hyper_v_enlightenment_recs     , { hyper_v                , nullptr                        , print_hyper_v_enlightenment_recs     , {} } },
	{ leaf_type::hyper_v_implementation_limits  , { hyper_v                , nullptr                        , print_hyper_v_implementation_limits  , {} } },
	{ leaf_type::hyper_v_implementation_hardware, { hyper_v                , nullptr                        , print_hyper_v_implementation_hardware, {} } },
	{ leaf_type::hyper_v_root_cpu_management    , { hyper_v                , nullptr                        , print_hyper_v_root_cpu_management    , {} } },
	{ leaf_type::hyper_v_shared_virtual_memory  , { hyper_v                , nullptr                        , print_hyper_v_shared_virtual_memory  , {} } },
	{ leaf_type::hyper_v_nested_hypervisor      , { hyper_v                , nullptr                        , print_hyper_v_nested_hypervisor      , {} } },
	{ leaf_type::hyper_v_nested_features        , { hyper_v                , nullptr                        , print_hyper_v_nested_features        , {} } },
	{ leaf_type::xen_limit                      , { xen_hvm                , nullptr                        , print_xen_limit                      , {} } },
	{ leaf_type::xen_version                    , { xen_hvm                , nullptr                        , print_xen_version                    , {} } },
	{ leaf_type::xen_features                   , { xen_hvm                , nullptr                        , print_xen_features                   , {} } },
	{ leaf_type::xen_time                       , { xen_hvm                , enumerate_xen_time             , print_xen_time                       , {} } },
	{ leaf_type::xen_hvm_features               , { xen_hvm                , nullptr                        , print_xen_hvm_features               , {} } },
	{ leaf_type::xen_pv_features                , { xen_hvm                , nullptr                        , print_xen_pv_features                , {} } },
	{ leaf_type::xen_limit_offset               , { xen_hvm                , nullptr                        , print_xen_limit                      , {} } },
	{ leaf_type::xen_version_offset             , { xen_hvm                , nullptr                        , print_xen_version                    , {} } },
	{ leaf_type::xen_features_offset            , { xen_hvm                , nullptr                        , print_xen_features                   , {} } },
	{ leaf_type::xen_time_offset                , { xen_hvm                , enumerate_xen_time             , print_xen_time                       , {} } },
	{ leaf_type::xen_hvm_features_offset        , { xen_hvm                , nullptr                        , print_xen_hvm_features               , {} } },
	{ leaf_type::xen_pv_features_offset         , { xen_hvm                , nullptr                        , print_xen_pv_features                , {} } },
	{ leaf_type::vmware_timing                  , { vmware                 , nullptr                        , print_vmware_timing                  , {} } },
	{ leaf_type::kvm_features                   , { vmware                 , nullptr                        , print_kvm_features                   , {} } },

	{ leaf_type::extended_limit                 , { any                    , nullptr                        , print_extended_limit                 , {} } },
	{ leaf_type::extended_signature_and_features, { any                    , nullptr                        , print_extended_signature_and_features, {} } },
	{ leaf_type::brand_string_0                 , { any                    , nullptr                        , print_brand_string                   , {} } },
	{ leaf_type::brand_string_1                 , { any                    , nullptr                        , print_null                           , {} } },
	{ leaf_type::brand_string_2                 , { any                    , nullptr                        , print_null                           , {} } },
	{ leaf_type::l1_cache_identifiers           , {         amd            , nullptr                        , print_l1_cache_tlb                   , {} } },
	{ leaf_type::l2_cache_identifiers           , { intel | amd            , nullptr                        , print_l2_cache_tlb                   , {} } },
	{ leaf_type::ras_advanced_power_management  , { intel | amd            , nullptr                        , print_ras_advanced_power_management  , {} } },
	{ leaf_type::address_limits                 , { intel | amd            , nullptr                        , print_address_limits                 , {} } },
	{ leaf_type::reserved_8                     , { any                    , enumerate_null                 , print_null                           , {} } },
	{ leaf_type::secure_virtual_machine         , {         amd            , nullptr                        , print_secure_virtual_machine         , { leaf_type::extended_signature_and_features, subleaf_type::main, ecx, 0x0000'0004_u32 } } },
	{ leaf_type::extended_reserved_1            , { any                    , enumerate_null                 , print_null                           , {} } },
	{ leaf_type::extended_reserved_2            , { any                    , enumerate_null                 , print_null                           , {} } },
	{ leaf_type::extended_reserved_3            , { any                    , enumerate_null                 , print_null                           , {} } },
	{ leaf_type::extended_reserved_4            , { any                    , enumerate_null                 , print_null                           , {} } },
	{ leaf_type::extended_reserved_5            , { any                    , enumerate_null                 , print_null                           , {} } },
	{ leaf_type::extended_reserved_6            , { any                    , enumerate_null                 , print_null                           , {} } },
	{ leaf_type::extended_reserved_7            , { any                    , enumerate_null                 , print_null                           , {} } },
	{ leaf_type::extended_reserved_8            , { any                    , enumerate_null                 , print_null                           , {} } },
	{ leaf_type::extended_reserved_9            , { any                    , enumerate_null                 , print_null                           , {} } },
	{ leaf_type::extended_reserved_10           , { any                    , enumerate_null                 , print_null                           , {} } },
	{ leaf_type::extended_reserved_11           , { any                    , enumerate_null                 , print_null                           , {} } },
	{ leaf_type::extended_reserved_12           , { any                    , enumerate_null                 , print_null                           , {} } },
	{ leaf_type::extended_reserved_13           , { any                    , enumerate_null                 , print_null                           , {} } },
	{ leaf_type::extended_reserved_14           , { any                    , enumerate_null                 , print_null                           , {} } },
	{ leaf_type::tlb_1g_identifiers             , {         amd            , nullptr                        , print_1g_tlb                         , {} } },
	{ leaf_type::performance_optimization       , {         amd            , nullptr                        , print_performance_optimization       , {} } },
	{ leaf_type::instruction_based_sampling     , {         amd            , nullptr                        , print_instruction_based_sampling     , { leaf_type::extended_signature_and_features, subleaf_type::main, ecx, 0x0000'0400_u32 } } },
	{ leaf_type::lightweight_profiling          , {         amd            , nullptr                        , print_lightweight_profiling          , { leaf_type::extended_signature_and_features, subleaf_type::main, ecx, 0x0000'8000_u32 } } },
	{ leaf_type::cache_properties               , {         amd            , enumerate_cache_properties     , print_cache_properties               , { leaf_type::extended_signature_and_features, subleaf_type::main, ecx, 0x0040'0000_u32 } } },
	{ leaf_type::extended_apic                  , {         amd            , nullptr                        , print_extended_apic                  , { leaf_type::extended_signature_and_features, subleaf_type::main, ecx, 0x0040'0000_u32 } } },
	{ leaf_type::encrypted_memory               , {         amd            , nullptr                        , print_encrypted_memory               , {} } }
};

void print_generic(fmt::memory_buffer& out, const cpu_t& cpu, leaf_type leaf, subleaf_type subleaf) {
	const register_set_t& regs = cpu.leaves.at(leaf).at(subleaf);
	format_to(out, "{:#010x} {:#010x} {:#010x}: {:#010x} {:#010x} {:#010x} {:#010x}\n", cpu.apic_id,
	                                                                                    static_cast<std::uint32_t>(leaf),
	                                                                                    static_cast<std::uint32_t>(subleaf),
	                                                                                    regs[eax],
	                                                                                    regs[ebx],
	                                                                                    regs[ecx],
	                                                                                    regs[edx]);

}

void print_generic(fmt::memory_buffer& out, const cpu_t& cpu, leaf_type leaf) {
	for(const auto& sub : cpu.leaves.at(leaf)) {
		print_generic(out, cpu, leaf, sub.first);
	}
}

void print_generic(fmt::memory_buffer& out, const cpu_t& cpu) {
	for(const auto& leaf : cpu.leaves) {
		print_generic(out, cpu, leaf.first);
	}
}

void enumerate_leaf_brute_force(cpu_t& cpu, leaf_type leaf) {
	register_set_t regs = cpuid(leaf, subleaf_type::main);
	cpu.leaves[leaf][subleaf_type::main] = regs;
	for(subleaf_type subleaf = subleaf_type{ 1 }; ; ++subleaf) {
		register_set_t previous = regs;
		regs = cpuid(leaf, subleaf);
		if(regs[eax] == 0_u32
		&& regs[ebx] == 0_u32
		&& (regs[ecx] == 0_u32 || regs[ecx] == static_cast<std::uint32_t>(subleaf))
		&& regs[edx] == 0_u32) {
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

void enumerate_leaf(cpu_t& cpu, leaf_type leaf, bool brute_force, bool skip_vendor_check, bool skip_feature_check) {
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
					cpu.leaves[leaf][subleaf_type::main] = cpuid(leaf, subleaf_type::main);
				}
			}
		}
	} else {
		enumerate_leaf_brute_force(cpu, leaf);
	}
}

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
					const leaf_type        leaf    = static_cast<leaf_type   >(std::stoul(m[2].str(), nullptr, 16));
					const subleaf_type     subleaf = static_cast<subleaf_type>(std::stoul(m[3].str(), nullptr, 16));
					const register_set_t regs   = {
						gsl::narrow_cast<std::uint32_t>(std::stoul(m[4].str(), nullptr, 16)),
						gsl::narrow_cast<std::uint32_t>(std::stoul(m[5].str(), nullptr, 16)),
						gsl::narrow_cast<std::uint32_t>(std::stoul(m[6].str(), nullptr, 16)),
						gsl::narrow_cast<std::uint32_t>(std::stoul(m[7].str(), nullptr, 16))
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
			std::uint32_t current_cpu = 0xffff'ffff_u32;
			while(std::getline(fin, line)) {
				std::smatch m;
				if(std::regex_search(line, m, comment_line) || line == "") {
					continue;
				} else if(std::regex_search(line, m, cpu_line)) {
					current_cpu = std::stoul(m[1].str());
				} else if(std::regex_search(line, m, data_line)) {
					const leaf_type         leaf    = static_cast<leaf_type   >(std::stoul(m[1].str(), nullptr, 16));
					const subleaf_type      subleaf = static_cast<subleaf_type>(std::stoul(m[2].str(), nullptr, 16));
					const register_set_t regs    = {
						gsl::narrow_cast<std::uint32_t>(std::stoul(m[3].str(), nullptr, 16)),
						gsl::narrow_cast<std::uint32_t>(std::stoul(m[4].str(), nullptr, 16)),
						gsl::narrow_cast<std::uint32_t>(std::stoul(m[5].str(), nullptr, 16)),
						gsl::narrow_cast<std::uint32_t>(std::stoul(m[6].str(), nullptr, 16))
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
			const std::uint32_t current_cpu = 0xffff'ffff_u32;
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
						gsl::narrow_cast<std::uint32_t>(std::stoul(m[3].str(), nullptr, 16)),
						gsl::narrow_cast<std::uint32_t>(std::stoul(m[4].str(), nullptr, 16)),
						gsl::narrow_cast<std::uint32_t>(std::stoul(m[5].str(), nullptr, 16)),
						gsl::narrow_cast<std::uint32_t>(std::stoul(m[6].str(), nullptr, 16))
					};
					leaf_type       leaf{ idx };
					subleaf_type subleaf{ idx };
					if(section == "basic_cpuid") {
						subleaf = subleaf_type::main;
					} else if(section == "ext_cpuid") {
						leaf = static_cast<leaf_type>(0x8000'0000_u32 + idx);
						subleaf = subleaf_type::main;
					} else if(section == "intel_fn4") {
						leaf = leaf_type::deterministic_cache;
						if((regs[eax] & 0x0000'001f_u32) == 0) {
							continue;
						}
					} else if(section == "intel_fn11") {
						leaf = leaf_type::extended_topology;
						if((regs[ecx] & 0x0000'ff00_u32) == 0_u32) {
							continue;
						}
					} else if(section == "intel_fn12h") {
						leaf = leaf_type::sgx_info;
						if((regs[eax] & 0x0000'000f_u32) == 0_u32) {
							continue;
						}
					} else if(section == "intel_fn14h") {
						leaf = leaf_type::processor_trace;
					}
					logical_cpus[current_cpu].leaves[leaf][subleaf] = regs;
				} else if(std::regex_search(line, m, delimiter_line)) {
					break;
				} else {
					std::cerr << "Unrecognized line: " << line << std::endl;
				}
			}
			const leaf_type highest_leaf          = leaf_type{ logical_cpus[current_cpu].leaves[leaf_type::basic_info    ][subleaf_type::main][eax] };
			const leaf_type highest_extended_leaf = leaf_type{ logical_cpus[current_cpu].leaves[leaf_type::extended_limit][subleaf_type::main][eax] };
			leaves_t corrected_leaves;
			for(leaf_type leaf = leaf_type::basic_info; leaf <= highest_leaf; ++leaf) {
				corrected_leaves[leaf] = logical_cpus[current_cpu].leaves[leaf];
			}
			for(leaf_type leaf = leaf_type::extended_limit; leaf <= highest_extended_leaf; ++leaf) {
				corrected_leaves[leaf] = logical_cpus[current_cpu].leaves[leaf];
			}
			if(corrected_leaves.find(leaf_type::processor_trace) != corrected_leaves.end()) {
				subleaves_t subleaves = corrected_leaves.at(leaf_type::processor_trace);
				const subleaf_type limit = subleaf_type{ subleaves[subleaf_type::main][eax] };
				for(subleaf_type sub = subleaf_type{ 1 }; sub < limit; ++sub) {
					if(subleaves.find(sub) != subleaves.end()) {
						subleaves[sub] = { 0x0_u32, 0x0_u32, 0x0_u32, 0x0_u32 };
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
			std::uint32_t current_cpu = 0xffff'ffff_u32;
			while(std::getline(fin, line)) {
				std::smatch m;
				if(std::regex_search(line, m, allcpu_line)
				|| std::regex_search(line, m, affinity_mask_line)
				|| std::regex_search(line, m, registers_line)) {
					++current_cpu;
				} else if(std::regex_search(line, m, subleaf_line)) {
					const leaf_type         leaf    = static_cast<leaf_type   >(std::stoul(m[1].str(), nullptr, 16));
					const subleaf_type      subleaf = static_cast<subleaf_type>(std::stoul(m[6].str(), nullptr, 10));
					const register_set_t regs    = {
						gsl::narrow_cast<std::uint32_t>(std::stoul(m[2].str(), nullptr, 16)),
						gsl::narrow_cast<std::uint32_t>(std::stoul(m[3].str(), nullptr, 16)),
						gsl::narrow_cast<std::uint32_t>(std::stoul(m[4].str(), nullptr, 16)),
						gsl::narrow_cast<std::uint32_t>(std::stoul(m[5].str(), nullptr, 16))
					};
					logical_cpus[current_cpu].leaves[leaf][subleaf] = regs;
				} else if(std::regex_search(line, m, description_line)
				       || std::regex_search(line, m, simple_line)) {
					const auto get_subleaf = [&logical_cpus](const std::uint32_t current_cpu, const leaf_type leaf) -> subleaf_type {
						if(logical_cpus.find(current_cpu) != logical_cpus.end()
						&& logical_cpus[current_cpu].leaves.find(leaf) != logical_cpus[current_cpu].leaves.end()
						&& logical_cpus[current_cpu].leaves[leaf].find(subleaf_type::main) != logical_cpus[current_cpu].leaves[leaf].end()) {
							return static_cast<subleaf_type>(gsl::narrow_cast<std::uint32_t>(logical_cpus[current_cpu].leaves[leaf].size()) + 1_u32);
						} else {
							return subleaf_type::main;
						}
					};
					const leaf_type         leaf    = static_cast<leaf_type   >(std::stoul(m[1].str(), nullptr, 16));
					if(leaf == leaf_type::basic_info
					&& logical_cpus.find(current_cpu) != logical_cpus.end()
					&& logical_cpus[current_cpu].leaves.find(leaf) != logical_cpus[current_cpu].leaves.end()) {
						++current_cpu;
					}
					const subleaf_type      subleaf = get_subleaf(current_cpu, leaf);
					const register_set_t regs    = {
						gsl::narrow_cast<std::uint32_t>(std::stoul(m[2].str(), nullptr, 16)),
						gsl::narrow_cast<std::uint32_t>(std::stoul(m[3].str(), nullptr, 16)),
						gsl::narrow_cast<std::uint32_t>(std::stoul(m[4].str(), nullptr, 16)),
						gsl::narrow_cast<std::uint32_t>(std::stoul(m[5].str(), nullptr, 16))
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

		regs = cpu.leaves.at(leaf_type::basic_info).at(subleaf_type::main);
		cpu.vendor = get_vendor_from_name(regs);

		regs = cpu.leaves.at(leaf_type::version_info).at(subleaf_type::main);
		cpu.model = get_model(cpu.vendor, regs);

		if(cpu.leaves.find(leaf_type::hypervisor_limit) != cpu.leaves.end()) {
			regs = cpu.leaves.at(leaf_type::hypervisor_limit).at(subleaf_type::main);
			if(regs[eax] != 0_u32) {
				const vendor_type hypervisor = get_hypervisor_from_name(regs);
				// something is set, and it looks like a hypervisor
				if(hypervisor & any_hypervisor) {
					cpu.vendor = cpu.vendor | hypervisor;

					if(hypervisor & hyper_v) {
						// xen with viridian extensions masquerades as hyper-v, and puts its own cpuid leaves 0x100 further up
						if(cpu.leaves.find(leaf_type::xen_limit_offset) != cpu.leaves.end()) {
							regs = cpu.leaves.at(leaf_type::xen_limit_offset).at(subleaf_type::main);
							const vendor_type xen_hypervisor = get_hypervisor_from_name(regs);

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
		register_set_t regs = cpuid(leaf_type::basic_info, subleaf_type::main);
		const leaf_type highest_leaf = leaf_type{ regs[eax] };
		cpu.vendor = get_vendor_from_name(regs);

		regs = cpuid(leaf_type::version_info, subleaf_type::main);
		cpu.model = get_model(cpu.vendor, regs);

		for(leaf_type leaf = leaf_type::basic_info; leaf <= highest_leaf; ++leaf) {
			enumerate_leaf(cpu, leaf, brute_force, skip_vendor_check, skip_feature_check);
		}

		regs = cpuid(leaf_type::hypervisor_limit, subleaf_type::main);
		if(regs[eax] != 0_u32) {
			const vendor_type hypervisor = get_hypervisor_from_name(regs);
			// something is set, and it looks like a hypervisor
			if(hypervisor & any_hypervisor) {
				cpu.vendor = cpu.vendor | hypervisor;
				const leaf_type highest_hypervisor_leaf = leaf_type{ regs[eax] };

				for(leaf_type leaf = leaf_type::hypervisor_limit; leaf <= highest_hypervisor_leaf; ++leaf) {
					enumerate_leaf(cpu, leaf, brute_force, skip_vendor_check, skip_feature_check);
				}

				if(hypervisor & hyper_v) {
					// xen with viridian extensions masquerades as hyper-v, and puts its own cpuid leaves 0x100 further up
					regs = cpuid(leaf_type::xen_limit_offset, subleaf_type::main);
					const vendor_type xen_hypervisor = get_hypervisor_from_name(regs);

					if(xen_hypervisor & xen_hvm) {
						cpu.vendor                    = cpu.vendor | xen_hypervisor;
						const leaf_type xen_base         = leaf_type::xen_limit_offset;
						const leaf_type highest_xen_leaf = leaf_type{ regs[eax] };

						for(leaf_type leaf = xen_base; leaf <= highest_xen_leaf; ++leaf) {
							enumerate_leaf(cpu, leaf, brute_force, skip_vendor_check, skip_feature_check);
						}
					}
				}
			}
		}
		regs = cpuid(leaf_type::extended_limit, subleaf_type::main);
		const leaf_type highest_extended_leaf = leaf_type{ regs[eax] };

		for(leaf_type leaf = leaf_type::extended_limit; leaf <= highest_extended_leaf; ++leaf) {
			enumerate_leaf(cpu, leaf, brute_force, skip_vendor_check, skip_feature_check);
		}

		cpu.apic_id = get_apic_id(cpu);
		logical_cpus[cpu.apic_id] = cpu;
	});
	return logical_cpus;
}

flag_spec_t parse_flag_spec(const std::string& flag_description) {
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
	if(!std::regex_search(flag_description, m, re_pattern)) {
		throw std::runtime_error(fmt::format("Bad pattern: {:s}", flag_description));
	}

	flag_spec_t spec = {};

	if(m[1].matched) {
		spec.selector_eax = std::stoul(m[1].str(), nullptr, 16);
	} else if(m[2].matched) {
		spec.selector_eax = std::stoul(m[2].str(), nullptr, 16);
		spec.selector_ecx = m[3].matched ? std::stoul(m[3].str(), nullptr, 16) : 0_u32;
	} else if(m[4].matched) {
		spec.selector_eax = std::stoul(m[4].str(), nullptr, 16);
		spec.selector_ecx = m[5].matched ? std::stoul(m[5].str(), nullptr, 16) : 0_u32;
	}

	const std::string reg_name = boost::algorithm::to_lower_copy(m[6].str());
	if(reg_name == "eax") {
		spec.flag_register = eax;
	} else if(reg_name == "ebx") {
		spec.flag_register = ebx;
	} else if(reg_name == "ecx") {
		spec.flag_register = ecx;
	} else if(reg_name == "edx") {
		spec.flag_register = edx;
	}

	if(m[7].matched) {
		spec.flag_name = m[8].str();
		if(m[9].matched) {
			spec.flag_start = std::stoul(m[9].str());
			spec.flag_end   = spec.flag_start;
		} else if(m[10].matched) {
			spec.flag_start = std::stoul(m[11].str());
			spec.flag_end   = std::stoul(m[10].str());
		}
	} else if(m[12].matched) {
		spec.flag_name = m[12].str();
	} else if(m[13].matched) {
		if(m[14].matched) {
			spec.flag_start = std::stoul(m[14].str());
			spec.flag_end   = spec.flag_start;
		} else if(m[15].matched) {
			spec.flag_start = std::stoul(m[16].str());
			spec.flag_end   = std::stoul(m[15].str());
		}
	}
	else if(m[17].matched) {
		spec.flag_name = m[17].str();
	}
	boost::algorithm::to_lower(spec.flag_name);

	return spec;
}

std::string to_string(register_type reg) {
	switch(reg) {
	case eax:
		return "EAX";
	case ebx:
		return "EBX";
	case ecx:
		return "ECX";
	case edx:
		return "EDX";
	default:
		UNREACHABLE();
	}
}

std::string to_string(const flag_spec_t& spec) {
	// named flag, unknown bit:
	// CPUID.(EAX=xxH, ECX=xxH):reg.name
	// named flag, specific bit:
	// CPUID.(EAX=xxH, ECX=xxH):reg.name[yy]
	// named flag, multibit:
	// CPUID.(EAX=xxH, ECX=xxH):reg.name[high:low]
	// unnamed flag, unknown bit:
	// CPUID.(EAX=xxH, ECX=xxH):reg
	// unnamed flag, single bit:
	// CPUID.(EAX=xxH, ECX=xxH):reg[yy]
	// unnamed flag, multibit:
	// CPUID.(EAX=xxH, ECX=xxH):reg[high:low]
	if(spec.flag_name != "") {
		if(spec.flag_start == 0xffff'ffff_u32 && spec.flag_end == 0xffff'ffff_u32) {
			return fmt::format("CPUID.(EAX={:02X}H, ECX={:02X}H):{:s}.{:s}"       , spec.selector_eax, spec.selector_ecx, to_string(spec.flag_register), boost::algorithm::to_upper_copy(spec.flag_name));
		} else if(spec.flag_start == spec.flag_end) {
			return fmt::format("CPUID.(EAX={:02X}H, ECX={:02X}H):{:s}.{:s}[{}]"   , spec.selector_eax, spec.selector_ecx, to_string(spec.flag_register), boost::algorithm::to_upper_copy(spec.flag_name), spec.flag_end);
		} else {
			return fmt::format("CPUID.(EAX={:02X}H, ECX={:02X}H):{:s}.{:s}[{}:{}]", spec.selector_eax, spec.selector_ecx, to_string(spec.flag_register), boost::algorithm::to_upper_copy(spec.flag_name), spec.flag_end, spec.flag_start);
		}
	} else {
		if(spec.flag_start == 0xffff'ffff_u32 && spec.flag_end == 0xffff'ffff_u32) {
			return fmt::format("CPUID.(EAX={:02X}H, ECX={:02X}H):{:s}"            , spec.selector_eax, spec.selector_ecx, to_string(spec.flag_register));
		} else if(spec.flag_start == spec.flag_end) {
			return fmt::format("CPUID.(EAX={:02X}H, ECX={:02X}H):{:s}[{}]"        , spec.selector_eax, spec.selector_ecx, to_string(spec.flag_register),                                                  spec.flag_end);
		} else {
			return fmt::format("CPUID.(EAX={:02X}H, ECX={:02X}H):{:s}[{}:{}]"     , spec.selector_eax, spec.selector_ecx, to_string(spec.flag_register),                                                  spec.flag_end, spec.flag_start);
		}
	}
}

void print_single_flag(fmt::memory_buffer& out, const cpu_t& cpu, const flag_spec_t& spec) {
	const std::string flag_description = to_string(spec);
	const std::string flag_name_alternative = boost::algorithm::replace_all_copy(spec.flag_name, "_", ".");

	const leaf_type    leaf    = static_cast<leaf_type   >(spec.selector_eax);
	const subleaf_type subleaf = static_cast<subleaf_type>(spec.selector_ecx);
	bool handled = false;
	if(cpu.leaves.find(leaf) != cpu.leaves.end()
	&& cpu.leaves.at(leaf).find(subleaf) != cpu.leaves.at(leaf).end()) {
		const std::uint32_t value = cpu.leaves.at(leaf).at(subleaf).at(spec.flag_register);
		if(spec.flag_name == "" && spec.flag_start == 0xffff'ffff_u32 && spec.flag_end == 0xffff'ffff_u32) {
			format_to(out, "cpu {:#04x} {:s}: {:#010x}\n", cpu.apic_id, flag_description, value);
			handled = true;
		}
		if(spec.flag_name != "") {
			const auto range = all_features.equal_range(leaf);
			for(auto it = range.first; it != range.second; ++it) {
				if(it->second.find(subleaf) == it->second.end()) {
					continue;
				}
				auto sub = it->second.at(subleaf);
				if(sub.find(spec.flag_register) == sub.end()) {
					continue;
				}
				for(const feature_t& feature : sub.at(spec.flag_register)) {
					const std::string lower_mnemonic = boost::algorithm::to_lower_copy(feature.mnemonic);
					if(lower_mnemonic == spec.flag_name
					|| lower_mnemonic == flag_name_alternative) {
						unsigned long shift_amount = 0;
						_BitScanForward(&shift_amount, feature.mask);
						const std::uint32_t result = (value & feature.mask) >> shift_amount;

						format_to(out, "cpu {:#04x} {:s}: {:#010x}\n", cpu.apic_id, flag_description, result);
						handled = true;
						break;
					}
				}
			}
		}
		if(!handled && spec.flag_start != 0xffff'ffff_u32 && spec.flag_end != 0xffff'ffff_u32) {
			const std::uint32_t mask = range_mask(spec.flag_start, spec.flag_end);
			unsigned long shift_amount = 0;
			_BitScanForward(&shift_amount, mask);
			const std::uint32_t result = (value & mask) >> shift_amount;
			format_to(out, "cpu {:#04x} {:s}: {:#010x}\n", cpu.apic_id, flag_description, result);
			handled = true;
		}
	}

	if(!handled) {
		format_to(out, "No data found for {:s}\n", flag_description);
	}
}

void print_leaves(fmt::memory_buffer& out, const cpu_t& cpu, bool skip_vendor_check, bool skip_feature_check) {
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
							it->second.printer(out, cpu);
						} else {
							print_generic(out, cpu, leaf.first);
						}
					}
				}
			}
		} else {
			print_generic(out, cpu, leaf.first);
			format_to(out, "\n");
		}
	}
}

void print_dump(fmt::memory_buffer& out, std::map<std::uint32_t, cpu_t> logical_cpus, file_format format) {
	switch(format) {
	case file_format::native:
		format_to(out, "#apic eax ecx: eax ebx ecx edx\n");
		for(const auto& p : logical_cpus) {
			print_generic(out, p.second);
			format_to(out, "\n");
		}
		break;
	case file_format::etallen:
		{
			std::uint32_t count = 0_u32;
			for(const auto& c : logical_cpus) {
				format_to(out, "CPU {:d}:\n", count);
				for(const auto& l : c.second.leaves) {
					for(const auto& s : l.second) {
						const cpu_t& cpu = c.second;
						const leaf_type leaf = l.first;
						const subleaf_type subleaf = s.first;
						const register_set_t& regs = cpu.leaves.at(leaf).at(subleaf);
						format_to(out, "   {:#010x} {:#04x}: eax={:#010x} ebx={:#010x} ecx={:#010x} edx={:#010x}\n", static_cast<std::uint32_t>(leaf),
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
			format_to(out, "version=0.4.0\n");
			const cpu_t& cpu = logical_cpus.begin()->second;

			for(std::uint32_t i = 0_u32; i < 32_u32; ++i) {
				const leaf_type leaf{ i };
				if(cpu.leaves.find(leaf) != cpu.leaves.end()) {
					const register_set_t& regs = cpu.leaves.at(leaf).at(subleaf_type::main);
					format_to(out, "basic_cpuid[{:d}]={:08x} {:08x} {:08x} {:08x}\n", i, regs[eax], regs[ebx], regs[ecx], regs[edx]);
				} else {
					format_to(out, "basic_cpuid[{:d}]={:08x} {:08x} {:08x} {:08x}\n", i, 0_u32, 0_u32, 0_u32, 0_u32);
				}
			}
			for(std::uint32_t i = 0_u32; i < 32_u32; ++i) {
				const leaf_type leaf{ i + 0x8000'0000_u32 };
				if(cpu.leaves.find(leaf) != cpu.leaves.end()) {
					const register_set_t& regs = cpu.leaves.at(leaf).at(subleaf_type::main);
					format_to(out, "ext_cpuid[{:d}]={:08x} {:08x} {:08x} {:08x}\n", i, regs[eax], regs[ebx], regs[ecx], regs[edx]);
				} else {
					format_to(out, "ext_cpuid[{:d}]={:08x} {:08x} {:08x} {:08x}\n", i, 0_u32, 0_u32, 0_u32, 0_u32);
				}
			}

			const auto print_detailed_leaves = [&out, &cpu](const leaf_type leaf, const std::uint32_t limit, const std::string& label) {
				if(cpu.leaves.find(leaf) != cpu.leaves.end()) {
					const subleaves_t& subleaves = cpu.leaves.at(leaf);
					std::uint32_t i = 0_u32;
					for(const auto& s : subleaves) {
						const register_set_t& regs = s.second;
						format_to(out, "{:s}[{:d}]={:08x} {:08x} {:08x} {:08x}\n", label, i, regs[eax], regs[ebx], regs[ecx], regs[edx]);
						++i;
					}
					for(; i < limit; ++i) {
						format_to(out, "{:s}[{:d}]={:08x} {:08x} {:08x} {:08x}\n", label, i, 0_u32, 0_u32, 0_u32, 0_u32);
					}
				}
			};

			print_detailed_leaves(leaf_type::deterministic_cache, 8, "intel_fn4");
			print_detailed_leaves(leaf_type::extended_topology, 4, "intel_fn11");
			print_detailed_leaves(leaf_type::sgx_info, 4, "intel_fn12h");
			print_detailed_leaves(leaf_type::processor_trace, 4, "intel_fn14h");
		}
		break;
	case file_format::instlat:
		break;
	}
}

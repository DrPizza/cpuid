#ifndef FEATURES_HPP
#define FEATURES_HPP

#include "cpuid/cpuid.hpp"

#include <fmt/format.h>

namespace cpuid
{
	struct feature_t
	{
		vendor_type vendor;
		std::uint32_t mask;
		std::string mnemonic;
		std::string description;
		std::string linux_name;
	};

	using feature_map_t = std::multimap<leaf_type, std::map<subleaf_type, std::map<register_type, std::vector<feature_t>>>>;

	extern const feature_map_t all_features;

	void print_features(fmt::memory_buffer& out, const cpu_t& cpu, leaf_type leaf, subleaf_type sub, register_type reg);

	std::vector<std::string> get_linux_features(const cpu_t& cpu);
	std::vector<std::string> get_linux_bugs(const cpu_t& cpu);
	std::vector<std::string> get_linux_power_management(const cpu_t& cpu);
}

#endif

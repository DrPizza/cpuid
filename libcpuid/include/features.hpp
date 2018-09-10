#ifndef FEATURES_HPP
#define FEATURES_HPP

#include "cpuid.hpp"

#include <fmt/format.h>

struct feature_t
{
	vendor_t vendor;
	std::uint32_t mask;
	std::string mnemonic;
	std::string description;
};

using feature_map_t = std::multimap<leaf_t, std::map<subleaf_t, std::map<register_t, std::vector<feature_t>>>>;

extern const feature_map_t all_features;

void print_features(fmt::memory_buffer& out, const cpu_t& cpu, leaf_t leaf, subleaf_t sub, register_t reg);

#endif

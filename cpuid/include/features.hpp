#ifndef FEATURES_HPP
#define FEATURES_HPP

#include "cpuid.hpp"

#include <fmt/format.h>

struct feature_t
{
	vendor_t vendor;
	std::uint32_t mask;
	const char* mnemonic;
	const char* description;
};

void print_features(fmt::Writer& w, const cpu_t& cpu, leaf_t leaf, subleaf_t sub, register_t reg);

#endif

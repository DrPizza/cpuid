#ifndef FEATURES_HPP
#define FEATURES_HPP

#include "cpuid.hpp"

void print_features(leaf_t leaf, subleaf_t sub, register_t reg, const cpu_t& cpu);

#endif

#ifndef STANDARD_HPP
#define STANDARD_HPP

#include "cpuid.hpp"

void print_basic_info(const cpu_t& cpu);

void print_version_info(const cpu_t& cpu);

void print_serial_number(const cpu_t& cpu);

void print_mwait_parameters(const cpu_t& cpu);

void print_thermal_and_power(const cpu_t& cpu);

void enumerate_extended_features(cpu_t& cpu);
void print_extended_features(const cpu_t& cpu);

void print_direct_cache_access(const cpu_t& cpu);

void print_performance_monitoring(const cpu_t& cpu);

void enumerate_extended_state(cpu_t& cpu);
void print_extended_state(const cpu_t& cpu);

void enumerate_rdt_monitoring(cpu_t& cpu);
void print_rdt_monitoring(const cpu_t& cpu);

void enumerate_rdt_allocation(cpu_t& cpu);
void print_rdt_allocation(const cpu_t& cpu);

void enumerate_sgx_info(cpu_t& cpu);
void print_sgx_info(const cpu_t& cpu);

void print_extended_limit(const cpu_t& cpu);

void print_extended_signature_and_features(const cpu_t& cpu);

void print_brand_string(const cpu_t& cpu);

void print_ras_advanced_power_management(const cpu_t& cpu);

void print_address_limits(const cpu_t& cpu);

void print_secure_virtual_machine(const cpu_t& cpu);

void print_performance_optimization(const cpu_t& cpu);

void print_instruction_based_sampling(const cpu_t& cpu);

#endif

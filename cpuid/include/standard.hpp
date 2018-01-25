#ifndef STANDARD_HPP
#define STANDARD_HPP

#include "cpuid.hpp"

void print_basic_info(fmt::Writer& w, const cpu_t& cpu);

void print_version_info(fmt::Writer& w, const cpu_t& cpu);

void print_serial_number(fmt::Writer& w, const cpu_t& cpu);

void print_mwait_parameters(fmt::Writer& w, const cpu_t& cpu);

void print_thermal_and_power(fmt::Writer& w, const cpu_t& cpu);

void enumerate_extended_features(cpu_t& cpu);
void print_extended_features(fmt::Writer& w, const cpu_t& cpu);

void print_direct_cache_access(fmt::Writer& w, const cpu_t& cpu);

void print_performance_monitoring(fmt::Writer& w, const cpu_t& cpu);

void enumerate_extended_state(cpu_t& cpu);
void print_extended_state(fmt::Writer& w, const cpu_t& cpu);

void enumerate_rdt_monitoring(cpu_t& cpu);
void print_rdt_monitoring(fmt::Writer& w, const cpu_t& cpu);

void enumerate_rdt_allocation(cpu_t& cpu);
void print_rdt_allocation(fmt::Writer& w, const cpu_t& cpu);

void enumerate_sgx_info(cpu_t& cpu);
void print_sgx_info(fmt::Writer& w, const cpu_t& cpu);

void enumerate_processor_trace(cpu_t& cpu);
void print_processor_trace(fmt::Writer& w, const cpu_t& cpu);

void print_time_stamp_counter(fmt::Writer& w, const cpu_t& cpu);

void print_processor_frequency(fmt::Writer& w, const cpu_t& cpu);

void enumerate_system_on_chip_vendor(cpu_t& cpu);
void print_system_on_chip_vendor(fmt::Writer& w, const cpu_t& cpu);

void enumerate_pconfig(cpu_t& cpu);
void print_pconfig(fmt::Writer& w, const cpu_t& cpu);

void print_extended_limit(fmt::Writer& w, const cpu_t& cpu);

void print_extended_signature_and_features(fmt::Writer& w, const cpu_t& cpu);

void print_brand_string(fmt::Writer& w, const cpu_t& cpu);

void print_ras_advanced_power_management(fmt::Writer& w, const cpu_t& cpu);

void print_address_limits(fmt::Writer& w, const cpu_t& cpu);

void print_secure_virtual_machine(fmt::Writer& w, const cpu_t& cpu);

void print_performance_optimization(fmt::Writer& w, const cpu_t& cpu);

void print_instruction_based_sampling(fmt::Writer& w, const cpu_t& cpu);

void print_lightweight_profiling(fmt::Writer& w, const cpu_t& cpu);

void print_encrypted_memory(fmt::Writer& w, const cpu_t& cpu);

#endif

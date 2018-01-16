#ifndef STANDARD_HPP
#define STANDARD_HPP

#include "cpuid.hpp"

void print_basic_info(const cpu_t& cpu);
void print_version_info(const cpu_t& cpu);
void print_serial_number(const cpu_t& cpu);
void print_mwait_parameters(const cpu_t& cpu);
void print_thermal_and_power(const cpu_t& cpu);

#endif

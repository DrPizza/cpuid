#ifndef HYPERVISORS_HPP
#define HYPERVISORS_HPP

#include "cpuid.hpp"

void print_hypervisor_limit(const cpu_t& cpu);

void print_hyper_v_signature(const cpu_t& cpu);

void print_hyper_v_system_identity(const cpu_t& cpu);

void print_hyper_v_features(const cpu_t& cpu);

void print_hyper_v_enlightenment_recs(const cpu_t& cpu);

void print_hyper_v_implementation_limits(const cpu_t& cpu);

void print_hyper_v_implementation_hardware(const cpu_t& cpu);

void print_hyper_v_root_cpu_management(const cpu_t& cpu);

void print_hyper_v_shared_virtual_memory(const cpu_t& cpu);

void print_hyper_v_nested_hypervisor(const cpu_t& cpu);

void print_hyper_v_nested_features(const cpu_t& cpu);

void print_xen_limit(const cpu_t& cpu);

void print_xen_version(const cpu_t& cpu);

void print_xen_features(const cpu_t& cpu);

void enumerate_xen_time(cpu_t& cpu);
void print_xen_time(const cpu_t& cpu);

void print_xen_hvm_features(const cpu_t& cpu);

void print_xen_pv_features(const cpu_t& cpu);

void print_vmware_timing(const cpu_t& cpu);

void print_kvm_features(const cpu_t& cpu);

#endif

#ifndef HYPERVISORS_HPP
#define HYPERVISORS_HPP

#include "cpuid.hpp"

void print_hypervisor_limit(fmt::memory_buffer& out, const cpu_t& cpu);

void print_hyper_v_signature(fmt::memory_buffer& out, const cpu_t& cpu);

void print_hyper_v_system_identity(fmt::memory_buffer& out, const cpu_t& cpu);

void print_hyper_v_features(fmt::memory_buffer& out, const cpu_t& cpu);

void print_hyper_v_enlightenment_recs(fmt::memory_buffer& out, const cpu_t& cpu);

void print_hyper_v_implementation_limits(fmt::memory_buffer& out, const cpu_t& cpu);

void print_hyper_v_implementation_hardware(fmt::memory_buffer& out, const cpu_t& cpu);

void print_hyper_v_root_cpu_management(fmt::memory_buffer& out, const cpu_t& cpu);

void print_hyper_v_shared_virtual_memory(fmt::memory_buffer& out, const cpu_t& cpu);

void print_hyper_v_nested_hypervisor(fmt::memory_buffer& out, const cpu_t& cpu);

void print_hyper_v_nested_features(fmt::memory_buffer& out, const cpu_t& cpu);

void print_xen_limit(fmt::memory_buffer& out, const cpu_t& cpu);

void print_xen_version(fmt::memory_buffer& out, const cpu_t& cpu);

void print_xen_features(fmt::memory_buffer& out, const cpu_t& cpu);

void enumerate_xen_time(cpu_t& cpu);
void print_xen_time(fmt::memory_buffer& out, const cpu_t& cpu);

void print_xen_hvm_features(fmt::memory_buffer& out, const cpu_t& cpu);

void print_xen_pv_features(fmt::memory_buffer& out, const cpu_t& cpu);

void print_vmware_timing(fmt::memory_buffer& out, const cpu_t& cpu);

void print_kvm_features(fmt::memory_buffer& out, const cpu_t& cpu);

#endif

#ifndef HYPERVISORS_HPP
#define HYPERVISORS_HPP

#include "cpuid.hpp"

void print_hypervisor_limit(fmt::Writer& w, const cpu_t& cpu);

void print_hyper_v_signature(fmt::Writer& w, const cpu_t& cpu);

void print_hyper_v_system_identity(fmt::Writer& w, const cpu_t& cpu);

void print_hyper_v_features(fmt::Writer& w, const cpu_t& cpu);

void print_hyper_v_enlightenment_recs(fmt::Writer& w, const cpu_t& cpu);

void print_hyper_v_implementation_limits(fmt::Writer& w, const cpu_t& cpu);

void print_hyper_v_implementation_hardware(fmt::Writer& w, const cpu_t& cpu);

void print_hyper_v_root_cpu_management(fmt::Writer& w, const cpu_t& cpu);

void print_hyper_v_shared_virtual_memory(fmt::Writer& w, const cpu_t& cpu);

void print_hyper_v_nested_hypervisor(fmt::Writer& w, const cpu_t& cpu);

void print_hyper_v_nested_features(fmt::Writer& w, const cpu_t& cpu);

void print_xen_limit(fmt::Writer& w, const cpu_t& cpu);

void print_xen_version(fmt::Writer& w, const cpu_t& cpu);

void print_xen_features(fmt::Writer& w, const cpu_t& cpu);

void enumerate_xen_time(cpu_t& cpu);
void print_xen_time(fmt::Writer& w, const cpu_t& cpu);

void print_xen_hvm_features(fmt::Writer& w, const cpu_t& cpu);

void print_xen_pv_features(fmt::Writer& w, const cpu_t& cpu);

void print_vmware_timing(fmt::Writer& w, const cpu_t& cpu);

void print_kvm_features(fmt::Writer& w, const cpu_t& cpu);

#endif

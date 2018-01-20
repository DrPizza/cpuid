#include "stdafx.h"

#include "features.hpp"

#include <cstddef>
#include <iostream>
#include <iomanip>
#include <map>
#include <vector>

using feature_map_t = std::multimap<leaf_t, std::map<subleaf_t, std::map<register_t, std::vector<feature_t>>>>;

const feature_map_t all_features = {
	{ leaf_t::version_info, {
		{ subleaf_t::main, {
			{ ecx, {
				{ intel | amd            , 0x0000'0001ui32, "SSE3"              , "SSE3 Extensions" },
				{ intel | amd            , 0x0000'0002ui32, "PCLMULQDQ"         , "Carryless Multiplication" },
				{ intel                  , 0x0000'0004ui32, "DTES64"            , "64-bit DS Area" },
				{ intel | amd            , 0x0000'0008ui32, "MONITOR"           , "MONITOR/MWAIT" },
				{ intel                  , 0x0000'0010ui32, "DS-CPL"            , "CPL Qualified Debug Store" },
				{ intel                  , 0x0000'0020ui32, "VMX"               , "Virtual Machine Extensions" },
				{ intel                  , 0x0000'0040ui32, "SMX"               , "Safer Mode Extensions" },
				{ intel                  , 0x0000'0080ui32, "EIST"              , "Enhanced Intel SpeedStep Technology" },
				{ intel                  , 0x0000'0100ui32, "TM2"               , "Thermal Monitor 2" },
				{ intel | amd            , 0x0000'0200ui32, "SSSE3"             , "SSSE3 Extensions" },
				{ intel                  , 0x0000'0400ui32, "CNXT-ID"           , "L1 Context ID" },
				{ intel                  , 0x0000'0800ui32, "SDBG"              , "IA32_DEBUG_INTERFACE for silicon debug" },
				{ intel | amd            , 0x0000'1000ui32, "FMA"               , "Fused Multiply Add" },
				{ intel | amd            , 0x0000'2000ui32, "CMPXCHG16B"        , "CMPXCHG16B instruction" },
				{ intel                  , 0x0000'4000ui32, "xTPR"              , "xTPR update control" },
				{ intel                  , 0x0000'8000ui32, "PDCM"              , "Perfmon and Debug Capability" },
				{ intel | amd            , 0x0002'0000ui32, "PCID"              , "Process-context identifiers" },
				{ intel                  , 0x0004'0000ui32, "DCA"               , "Direct Cache Access" },
				{ intel | amd            , 0x0008'0000ui32, "SSE4.1"            , "SSE4.1 Extensions" },
				{ intel | amd            , 0x0010'0000ui32, "SSE4.2"            , "SSE4.2 Extensions" },
				{ intel | amd            , 0x0020'0000ui32, "x2APIC"            , "x2APIC" },
				{ intel | amd            , 0x0040'0000ui32, "MOVBE"             , "MOVBE instruction" },
				{ intel | amd            , 0x0080'0000ui32, "POPCNT"            , "POPCNT instruction" },
				{ intel | amd            , 0x0100'0000ui32, "TSC-Deadline"      , "One-shot APIC timers using a TSC deadline" },
				{ intel | amd            , 0x0200'0000ui32, "AESNI"             , "AESNI instruction extensions" },
				{ intel | amd            , 0x0400'0000ui32, "XSAVE"             , "XSAVE/XRSTOR feature" },
				{ intel | amd            , 0x0800'0000ui32, "OSXSAVE"           , "OS has set CR4.OSXSAVE" },
				{ intel | amd            , 0x1000'0000ui32, "AVX"               , "AVX instructions" },
				{ intel | amd            , 0x2000'0000ui32, "F16C"              , "16-bit floating-point conversion instructions" },
				{ intel | amd            , 0x4000'0000ui32, "RDRAND"            , "RDRAND instruction" },
				{ any                    , 0x8000'0000ui32, "RAZ"               , "Hypervisor guest" },
			}},
			{ edx, {
				{ intel | amd | transmeta, 0x0000'0001ui32, "FPU"               , "x87 FPU on chip"},
				{ intel | amd | transmeta, 0x0000'0002ui32, "VME"               , "Virtual 8086 Mode Enhancements"},
				{ intel | amd | transmeta, 0x0000'0004ui32, "DE"                , "Debugging Extensions"},
				{ intel | amd | transmeta, 0x0000'0008ui32, "PSE"               , "Page Size Extension"},
				{ intel | amd | transmeta, 0x0000'0010ui32, "TSC"               , "Time Stamp Counter"},
				{ intel | amd | transmeta, 0x0000'0020ui32, "MSR"               , "RDMSR and WRMSR Instructions"},
				{ intel | amd            , 0x0000'0040ui32, "PAE"               , "Physical Address Extension"},
				{ intel | amd            , 0x0000'0080ui32, "MCE"               , "Machine Check Exception"},
				{ intel | amd | transmeta, 0x0000'0100ui32, "CX8"               , "CMPXCHG8B Instruction"},
				{ intel | amd            , 0x0000'0200ui32, "APIC"              , "APIC On-Chip"},
				{ intel | amd | transmeta, 0x0000'0800ui32, "SEP"               , "SYSENTER and SYSEXIT Instructions"},
				{ intel | amd            , 0x0000'1000ui32, "MTRR"              , "Memory Type Range Registers"},
				{ intel | amd            , 0x0000'2000ui32, "PGE"               , "Page Global Bit"},
				{ intel | amd            , 0x0000'4000ui32, "MCA"               , "Machine Check Architecture"},
				{ intel | amd | transmeta, 0x0000'8000ui32, "CMOV"              , "Conditional Move Instructions"},
				{ intel | amd            , 0x0001'0000ui32, "PAT"               , "Page Attribute Table"},
				{ intel | amd            , 0x0002'0000ui32, "PSE-36"            , "36-bit Page Size Extension"},
				{ intel       | transmeta, 0x0004'0000ui32, "PSN"               , "Processor Serial Number"},
				{ intel | amd            , 0x0008'0000ui32, "CLFSH"             , "CLFLUSH Instruction"},
				{ intel                  , 0x0020'0000ui32, "DS"                , "Debug Store"},
				{ intel                  , 0x0040'0000ui32, "ACPI"              , "Thermal Monitoring and Software Controlled Clock Facilities"},
				{ intel | amd | transmeta, 0x0080'0000ui32, "MMX"               , "Intel MMX Technology"},
				{ intel | amd            , 0x0100'0000ui32, "FXSR"              , "FXSAVE and FXRSTOR Instructions"},
				{ intel | amd            , 0x0200'0000ui32, "SSE"               , "SSE Extensions"},
				{ intel | amd            , 0x0400'0000ui32, "SSE2"              , "SSE2 Extensions"},
				{ intel                  , 0x0800'0000ui32, "SS"                , "Self Snoop"},
				{ intel | amd            , 0x1000'0000ui32, "HTT"               , "Max APIC IDs reserved field is Valid"},
				{ intel                  , 0x2000'0000ui32, "TM"                , "Thermal Monitor"},
				{ intel                  , 0x4000'0000ui32, "IA64"              , "IA64 emulating x86"},
				{ intel                  , 0x8000'0000ui32, "PBE"               , "Pending Break Enable"},
			}}
		}}
	}},
	{ leaf_t::thermal_and_power, {
		{ subleaf_t::main, {
			{ eax, {
				{ intel                  , 0x0000'0001ui32, "DTS"               , "Digital temperature sensor" },
				{ intel                  , 0x0000'0002ui32, "TBT"               , "Intel Turbo Boost Technology" },
				{ intel | amd            , 0x0000'0004ui32, "ARAT"              , "APIC-Timer-always-running" },
				{ intel                  , 0x0000'0010ui32, "PLN"               , "Power limit notification controls" },
				{ intel                  , 0x0000'0020ui32, "ECMD"              , "Clock modulation duty cycle extension" },
				{ intel                  , 0x0000'0040ui32, "PTM"               , "Package thermal management" },
				{ intel                  , 0x0000'0080ui32, "HWP"               , "Hardware Managed Performance States: HWP_CAPABILITIES, HWP_REQUEST, HWP_STATUS" },
				{ intel                  , 0x0000'0100ui32, "HWP_N"             , "HWP_Notification                   : HWP_INTERRUPT" },
				{ intel                  , 0x0000'0200ui32, "HWP_AW"            , "HWP_Activity_Window                : HWP_REQUEST[41:32]" },
				{ intel                  , 0x0000'0400ui32, "HWP_EPP"           , "HWP_Energy_Performance_Preference  : HWP_REQUEST[31:24]" },
				{ intel                  , 0x0000'0800ui32, "HWP_PLR"           , "HWP_Package_Level_Request          : HWP_REQUEST_PKG" },
				{ intel                  , 0x0000'2000ui32, "HDC"               , "HDC_CTL, HDC_CTL1, THREAD_STALL" },
				{ intel                  , 0x0000'4000ui32, "TBT3"              , "Intel Turbo Boost Max Technology 3.0" },
			}},
			{ ecx, {
				{ intel                  , 0x0000'0001ui32, "HCF"               , "Hardware Coordination Feedback Capability: MPERF, APERF"},
				{ intel                  , 0x0000'0008ui32, "PERF_BIAS"         , "Performance-energy bias preference       : ENERGY_PERF_BIAS" },
			}}
		}}
	}},
	{ leaf_t::extended_features, {
		{ subleaf_t::extended_features_main, {
			{ ebx, {
				{ intel | amd            , 0x0000'0001ui32, "FSGSBASE"          , "FSGSBASE instructions" },
				{ intel                  , 0x0000'0002ui32, "TSC_ADJUST"        , "TSC_ADJUST MSR" },
				{ intel                  , 0x0000'0004ui32, "SGX"               , "Softward Guard Extensions" },
				{ intel | amd            , 0x0000'0008ui32, "BMI1"              , "Bit Manipulation Instructions 1" },
				{ intel                  , 0x0000'0010ui32, "HLE"               , "Hardware Lock Elision" },
				{ intel | amd            , 0x0000'0020ui32, "AVX2"              , "Advanced Vector Extensions 2.0 instructions" },
				{ intel                  , 0x0000'0040ui32, "FDP_EXCPT"         , "x87 FPU Data Pointer updated only on x87 exceptions" },
				{ intel | amd            , 0x0000'0080ui32, "SMEP"              , "Supervisor-Mode Execution Prevention" },
				{ intel | amd            , 0x0000'0100ui32, "BMI2"              , "Bit Manipulation Instructions 2" },
				{ intel                  , 0x0000'0200ui32, "EREPMOVSB"         , "Enhanced REP MOVSB/REP STOSB" },
				{ intel                  , 0x0000'0400ui32, "INVPCID"           , "INVPCID instruction" },
				{ intel                  , 0x0000'0800ui32, "RTM"               , "Restricted Transactional Memory" },
				{ intel                  , 0x0000'1000ui32, "RDT-M"             , "Resource Director Technology Monitoring" },
				{ intel                  , 0x0000'2000ui32, "FPU-CSDS"          , "x87 FPU CS and DS deprecated" },
				{ intel                  , 0x0000'4000ui32, "MPX"               , "Memory Protection Extensions" },
				{ intel                  , 0x0000'8000ui32, "RDT-A"             , "Resource Director Technology Allocation" },
				{ intel                  , 0x0001'0000ui32, "AVX512F"           , "AVX512 Foundation" },
				{ intel                  , 0x0002'0000ui32, "AVX512DQ"          , "AVX512 Double/Quadword Instructions" },
				{ intel | amd            , 0x0004'0000ui32, "RDSEED"            , "RDSEED instruction" },
				{ intel | amd            , 0x0008'0000ui32, "ADX"               , "Multi-Precision Add-Carry Instructions" },
				{ intel | amd            , 0x0010'0000ui32, "SMAP"              , "Supervisor-Mode Access Prevention" },
				{ intel                  , 0x0020'0000ui32, "AVX512_IFMA"       , "AVX512 Integer FMA" },
				{         amd            , 0x0040'0000ui32, "PCOMMIT"           , "Persistent Commit" },
				{ intel | amd            , 0x0080'0000ui32, "CLFLUSHOPT"        , "CLFLUSHOPT instruction" },
				{ intel                  , 0x0100'0000ui32, "CLWB"              , "CLWB instruction" },
				{ intel                  , 0x0200'0000ui32, "IPT"               , "Intel Processor Trace" },
				{ intel                  , 0x0400'0000ui32, "AVX512PF"          , "AVX512 Prefetch" },
				{ intel                  , 0x0800'0000ui32, "AVX512ER"          , "AVX512 Exponential and Reciprocal Instructions" },
				{ intel                  , 0x1000'0000ui32, "AVX512CD"          , "AVX512 Conflict Detection Instructions" },
				{ intel | amd            , 0x2000'0000ui32, "SHA"               , "SHA Extensions" },
				{ intel                  , 0x4000'0000ui32, "AVX512BW"          , "AVX512 Byte/Word Instructions" },
				{ intel                  , 0x8000'0000ui32, "AVX512VL"          , "AVX512 Vector Length Instructions" },
			}},
			{ ecx, {
				{ intel                  , 0x0000'0001ui32, "PREFETCHW1"        , "PREFETCHW1 instruction" },
				{ intel                  , 0x0000'0002ui32, "AVX512_VBMI"       , "AVX512 Vector Bit Manipulation Instructions" },
				{ intel                  , 0x0000'0004ui32, "UMIP"              , "User Mode Instruction Prevention" },
				{ intel                  , 0x0000'0008ui32, "PKU"               , "Protection Keys for User-mode pages" },
				{ intel                  , 0x0000'0010ui32, "OSPKE"             , "OS has set CR4.PKE" },
				{ intel                  , 0x0040'0000ui32, "RDPID"             , "Read Processor ID" },
				{ intel                  , 0x4000'0000ui32, "SGX_LC"            , "SGX Launch Configuration" },
			}},
			{ edx, {
				{ intel                  , 0x0000'0004ui32, "AVX512_4NNIW"      , "AVX512 4-register Neural Network Instructions" },
				{ intel                  , 0x0000'0008ui32, "AVX512_4FMAPS"     , "AVX512 4-register Neural Network Instructions" },
				{ intel | amd            , 0x0400'0000ui32, "IBRS"              , "Indirect Branch Restricted Speculation and Indirect Branch Predictor Barrier" },
				{ intel | amd            , 0x0800'0000ui32, "STIBP"             , "Single Thread Indirect Branch Predictors" },
				{ intel                  , 0x2000'0000ui32, "ARCH_CAPS"         , "ARCH_CAPABILITIES MSR" },
			}}
		}}
	}},
	{ leaf_t::processor_trace, {
		{ subleaf_t::main, {
			{ ebx, {
				{ intel                  , 0x0000'0001ui32, "CR3Filter"         , "CR3Filter can be set to 1" },
				{ intel                  , 0x0000'0002ui32, "PSB"               , "Configurable PSB and Cycle-Accurate Mode" },
				{ intel                  , 0x0000'0004ui32, "IPFilter"          , "IP Filtering, TraceStop filtering" },
				{ intel                  , 0x0000'0008ui32, "MTC"               , "MTC timing packet supported" },
				{ intel                  , 0x0000'0010ui32, "PTWRITE"           , "PTWRITE supported" },
				{ intel                  , 0x0000'0020ui32, "PET"               , "Power Event Trace" },
			}},
			{ ecx, {
				{ intel                  , 0x0000'0001ui32, "TOPA"              , "ToPA output supported" },
				{ intel                  , 0x0000'0002ui32, "TOPAEntries"       , "ToPA tables can hold any number of entries" },
				{ intel                  , 0x0000'0004ui32, "SRO"               , "Single-Range Output" },
				{ intel                  , 0x0000'0008ui32, "TT"                , "Trace Transport output" },
				{ intel                  , 0x8000'0000ui32, "IPLIP"             , "IP payloads have LIP values" },
			}}
		}}
	}},
	{ leaf_t::hyper_v_features, {
		{ subleaf_t::main, {
			{ edx, {
				{ hyper_v                , 0x0000'0001ui32, "MWAIT"                , "MWAIT available" },
				{ hyper_v                , 0x0000'0002ui32, "GDBG"                 , "Guest debugging" },
				{ hyper_v                , 0x0000'0004ui32, "PM"                   , "Performance Monitoring" },
				{ hyper_v                , 0x0000'0008ui32, "DynamicPartitioning"  , "Physical CPU dynamic partitioning" },
				{ hyper_v                , 0x0000'0010ui32, "XMMHypercallInput"    , "Hypercall parameters in XMM registers" },
				{ hyper_v                , 0x0000'0020ui32, "GuestIdle"            , "Virtual guest idle state available" },
				{ hyper_v                , 0x0000'0040ui32, "HypervisorSleep"      , "Hypervisor sleep state available" },
				{ hyper_v                , 0x0000'0080ui32, "QueryNUMA"            , "NUMA distances queryable" },
				{ hyper_v                , 0x0000'0100ui32, "TimerFrequencies"     , "Determining timer frequencies supported" },
				{ hyper_v                , 0x0000'0200ui32, "MCEInject"            , "Support injecting MCEs" },
				{ hyper_v                , 0x0000'0400ui32, "CrashMSRs"            , "Guest crash MSRs available" },
				{ hyper_v                , 0x0000'0800ui32, "DebugMSRs"            , "Debug MSRs available" },
				{ hyper_v                , 0x0000'1000ui32, "NPIEP"                , "NPIEP supported" },
				{ hyper_v                , 0x0000'2000ui32, "DisableHypervisor"    , "Disable hypervisor supported" },
				{ hyper_v                , 0x0000'4000ui32, "ExtendedGvaRanges"    , "Extended GVA ranges for flush" },
				{ hyper_v                , 0x0000'8000ui32, "XMMHypercallOutput"   , "Hypercall results in XMM registers" },
				{ hyper_v                , 0x0002'0000ui32, "SintPollingMode"      , "Sint polling mode available" },
				{ hyper_v                , 0x0004'0000ui32, "HypercallMsrLock"     , "Hypercall MSR lock available" },
				{ hyper_v                , 0x0008'0000ui32, "SyntheticTimers"      , "Use direct synthetic timers" },
			}}
		}}
	}},
	{ leaf_t::hyper_v_enlightenment_recs, {
		{ subleaf_t::main, {
			{ eax, {
				{ hyper_v                , 0x0000'0001ui32, "MOV_CR3"              , "Use hypercall instead of MOV CR3" },
				{ hyper_v                , 0x0000'0002ui32, "INVLPG"               , "Use hypercall instead of INVLPG or MOV CR3" },
				{ hyper_v                , 0x0000'0004ui32, "IPI"                  , "Use hypercall instead of inter-processor interrupts" },
				{ hyper_v                , 0x0000'0008ui32, "APIC_MSR"             , "Use MSRs for APIC registers" },
				{ hyper_v                , 0x0000'0010ui32, "RESET_MSR"            , "Use MSR for system reset" },
				{ hyper_v                , 0x0000'0020ui32, "RelaxTimings"         , "Use relaxed timings/disable watchdogs" },
				{ hyper_v                , 0x0000'0040ui32, "DMARemapping"         , "Use DMA remapping" },
				{ hyper_v                , 0x0000'0080ui32, "InterruptRemapping"   , "Use interrupt remapping" },
				{ hyper_v                , 0x0000'0100ui32, "x2_APIC_MSR"          , "Use x2 APIC MSRs" },
				{ hyper_v                , 0x0000'0200ui32, "DeprecateAutoEOI"     , "Deprecate AutoEOI" },
				{ hyper_v                , 0x0000'0400ui32, "SyntheticClusterIpi"  , "Use SyntheticClusterIpi hypercall" },
				{ hyper_v                , 0x0000'0800ui32, "ExProcessorMasks"     , "Use ExProcessorMasks interface" },
				{ hyper_v                , 0x0000'1000ui32, "Nested"               , "Running in a nested partition" },
				{ hyper_v                , 0x0000'2000ui32, "INT_MBEC"             , "Use INT for MBEC system calls" },
				{ hyper_v                , 0x0000'4000ui32, "VMCS"                 , "Use VMCS for nested hypervisor" },
			}}
		}}
	}},
	{ leaf_t::hyper_v_implementation_hardware, {
		{ subleaf_t::main, {
			{ eax, {
				{ hyper_v                , 0x0000'0001ui32, "APIC_OVERLAY"          , "APIC overlay assist" },
				{ hyper_v                , 0x0000'0002ui32, "MSR_BITMAPS"           , "MSR bitmaps" },
				{ hyper_v                , 0x0000'0004ui32, "PERF_COUNTERS"         , "Architectural performance counters" },
				{ hyper_v                , 0x0000'0008ui32, "SLAT"                  , "Second Level Address Translation" },
				{ hyper_v                , 0x0000'0010ui32, "DMA_REMAP"             , "DMA remapping" },
				{ hyper_v                , 0x0000'0020ui32, "INTERRUPT_REMAP"       , "Interrupt remapping" },
				{ hyper_v                , 0x0000'0040ui32, "MEMORY_SCRUBBER"       , "Memory patrol scrubber" },
				{ hyper_v                , 0x0000'0080ui32, "DMA_PROTECTION"        , "DMA protection" },
				{ hyper_v                , 0x0000'0100ui32, "HPET"                  , "HPET" },
				{ hyper_v                , 0x0000'0200ui32, "SyntheticTimers"       , "Volatile synthetic timers" },
			}}
		}}
	}},
	{ leaf_t::hyper_v_root_cpu_management, {
		{ subleaf_t::main, {
			{ eax, {
				{ hyper_v                , 0x0000'0001ui32, "StartLogicalProcessor" , "Start logical processor" },
				{ hyper_v                , 0x0000'0002ui32, "CreateRootVirtProc"    , "Create root virtual processor" },
				{ hyper_v                , 0x8000'0000ui32, "ReservedIdentityBit"   , "Reserved identity bit" },
			}},
			{ ebx, {
				{ hyper_v                , 0x0000'0001ui32, "ProcessorPowerMgmt"    , "Processor power management" },
				{ hyper_v                , 0x0000'0002ui32, "MwaitIdleStates"       , "MWAIT idle states" },
				{ hyper_v                , 0x0000'0004ui32, "LogicalProcessorIdling", "Logical processor idling" },
			}}
		}}
	}},
	{ leaf_t::hyper_v_shared_virtual_memory, {
		{ subleaf_t::main, {
			{ eax, {
				{ hyper_v                , 0x0000'0001ui32, "SvmSupported"          , "Shared virtual memory supported" },
			}},
		}}
	}},
	{ leaf_t::hyper_v_nested_hypervisor, {
		{ subleaf_t::main, {
			{ eax, {
				{ hyper_v                , 0x0000'0004ui32, "AccessSynIcRegs"       , "SynIC MSRs" },
				{ hyper_v                , 0x0000'0010ui32, "AccessIntrCtrlRegs"    , "Interrupt Control MSRs" },
				{ hyper_v                , 0x0000'0020ui32, "AccessHypercallMsrs"   , "Hypercall MSRs" },
				{ hyper_v                , 0x0000'0040ui32, "AccessVpIndex"         , "VP Index MSRs" },
				{ hyper_v                , 0x0000'0100ui32, "AccessReenlightenment" , "Reenlightenment controls" },
			}},
			{ edx, {
				{ hyper_v                , 0x0000'0010ui32, "XMMHypercallInput"     , "Hypercall parameters in XMM registers" },
				{ hyper_v                , 0x0000'8000ui32, "XMMHypercallOutput"    , "Hypercall results in XMM registers" },
				{ hyper_v                , 0x0002'0000ui32, "SintPollingAvailable"  , "Sint polling mode available" },
			}},
		}}
	}},
	{ leaf_t::hyper_v_nested_features, {
		{ subleaf_t::main, {
			{ eax, {
				{ hyper_v                , 0x0002'0000ui32, "DirectVirtualFlush"    , "Direct virtual flush hypercalls" },
				{ hyper_v                , 0x0004'0010ui32, "FlushGuestPhysical"    , "HvFlushGuestPhysicalAddressXxx hypercalls" },
				{ hyper_v                , 0x0008'0020ui32, "EnlightenedMSRBitmap"  , "Enlightened MSR bitmap" },
			}},
		}}
	}},
	{ leaf_t::xen_time, {
		{ subleaf_t::xen_time_main, {
			{ eax, {
				{           xen_hvm      , 0x0000'0001ui32, "VTSC"                   , "Virtual RDTSC" },
				{           xen_hvm      , 0x0000'0002ui32, "SafeRDTSC"              , "Host has safe RDTSC" },
				{           xen_hvm      , 0x0000'0004ui32, "RDTSCP"                 , "Host has RDTSCP" },
			}}
		}}
	}},
	{ leaf_t::xen_time_offset, {
		{ subleaf_t::xen_time_main, {
			{ eax, {
				{           xen_hvm      , 0x0000'0001ui32, "VTSC"                   , "Virtual RDTSC" },
				{           xen_hvm      , 0x0000'0002ui32, "SafeRDTSC"              , "Host has safe RDTSC" },
				{           xen_hvm      , 0x0000'0004ui32, "RDTSCP"                 , "Host has RDTSCP" },
			}}
		}}
	}},
	{ leaf_t::xen_hvm_features, {
		{ subleaf_t::xen_time_main, {
			{ eax, {
				{           xen_hvm      , 0x0000'0001ui32, "VAPIC"                  , "Virtualized APIC registers" },
				{           xen_hvm      , 0x0000'0002ui32, "Vx2APIC"                , "Virtualized x2APIC registers" },
				{           xen_hvm      , 0x0000'0004ui32, "IOMMU"                  , "IOMMU mappings from other domains exist" },
				{           xen_hvm      , 0x0000'0008ui32, "VCPU"                   , "VCPU ID is present" },
			}}
		}}
	}},
	{ leaf_t::xen_hvm_features_offset, {
		{ subleaf_t::xen_time_main, {
			{ eax, {
				{           xen_hvm      , 0x0000'0001ui32, "VAPIC"                  , "Virtualized APIC registers" },
				{           xen_hvm      , 0x0000'0002ui32, "Vx2APIC"                , "Virtualized x2APIC registers" },
				{           xen_hvm      , 0x0000'0004ui32, "IOMMU"                  , "IOMMU mappings from other domains exist" },
				{           xen_hvm      , 0x0000'0008ui32, "VCPU"                   , "VCPU ID is present" },
			}}
		}}
	}},
	{ leaf_t::extended_signature_and_features, {
		{ subleaf_t::main, {
			{ ecx, {
				{ intel | amd            , 0x0000'0001ui32, "LahfSahf"               , "LAHF/SAHF supported in 64-bit mode"   },
				{         amd            , 0x0000'0002ui32, "CmpLegacy"              , "Core multi-processing legacy mode"    },
				{         amd            , 0x0000'0004ui32, "SVM"                    , "Secure Virtual Machine Mode"          },
				{         amd            , 0x0000'0008ui32, "ExtApicSpace"           , "Extended APIC register space"         },
				{         amd            , 0x0000'0010ui32, "AltMovCr8"              , "LOCK MOV CR0 is MOV CR8"              },
				{ intel                  , 0x0000'0020ui32, "LZCNT"                  , "LZCNT instruction"                    },
				{         amd            , 0x0000'0020ui32, "ABM"                    , "Advanced Bit Manipulation"            },
				{         amd            , 0x0000'0040ui32, "SSE4A"                  , "SSE4A instructions"                   },
				{         amd            , 0x0000'0080ui32, "MisAlignSse"            , "Misaligned SSE Mode"                  } ,
				{ intel                  , 0x0000'0100ui32, "PREFETCHW"              , "PREFETCHW instruction"                },
				{         amd            , 0x0000'0100ui32, "ThreeDNowPrefetch"      , "PREFETCH and PREFETCHW instructions"  },
				{         amd            , 0x0000'0200ui32, "OSVW"                   , "OS Visible Work-around"               },
				{         amd            , 0x0000'0400ui32, "IBS"                    , "Instruction Based Sampling"           },
				{         amd            , 0x0000'0800ui32, "XOP"                    , "Extended Operation support"           },
				{         amd            , 0x0000'1000ui32, "SKINIT"                 , "SKINIT/STGI instructions"             },
				{         amd            , 0x0000'2000ui32, "WDT"                    , "Watchdog Timer Support"               },
				{         amd            , 0x0000'8000ui32, "LWP"                    , "Lightweight Profiling Support"        },
				{         amd            , 0x0001'0000ui32, "FMA4"                   , "4-operand FMA instruction"            },
				{         amd            , 0x0002'0000ui32, "TCE"                    , "Translation Cache Extension"          },
				{         amd            , 0x0020'0000ui32, "TBM"                    , "Trailing Bit Manipulation"            },
				{         amd            , 0x0040'0000ui32, "TopologyExtensions"     , "Topology Extensions"                  },
				{         amd            , 0x0080'0000ui32, "PerfCtrExtCore"         , "Core Performance Counter Extensions"  },
				{         amd            , 0x0100'0000ui32, "PerfCtrExtNB"           , "NB Performance Counter Extensions"    },
				{         amd            , 0x0400'0000ui32, "DataBreakpointExtension", "Data Breakpoint support"              },
				{         amd            , 0x0400'0000ui32, "PerfTsc"                , "Performance Time Stamp Counter"       },
				{         amd            , 0x1000'0000ui32, "PerfCtrExtL3"           , "L3 performance counter extensions"    },
				{         amd            , 0x2000'0000ui32, "MwaitExtended"          , "MWAITX and MONITORX"                  },
			}},
			{ edx, {
				{         amd            , 0x0000'0001ui32, "FPU"                    , "x87 FPU on chip"                      },
				{         amd            , 0x0000'0002ui32, "VME"                    , "Virtual Mode Enhancements"            },
				{         amd            , 0x0000'0004ui32, "DE"                     , "Debugging extensions"                 },
				{         amd            , 0x0000'0008ui32, "PSE"                    , "Page-size extensions"                 },
				{         amd            , 0x0000'0010ui32, "TSC"                    , "Time Stamp Counter"                   },
				{         amd            , 0x0000'0020ui32, "MSR"                    , "RDMSR and WRMSR Instructions"         },
				{         amd            , 0x0000'0040ui32, "PAE"                    , "Physical Address Extension"           },
				{         amd            , 0x0000'0080ui32, "MCE"                    , "Machine Check Exception"              },
				{         amd            , 0x0000'0100ui32, "CX8"                    , "CMPXCHG8B Instruction"                },
				{         amd            , 0x0000'0200ui32, "APIC"                   , "APIC On-Chip"                         },
				{ intel | amd            , 0x0000'0800ui32, "SysCallSysRet"          , "SYSENTER and SYSEXIT Instructions"    },
				{         amd            , 0x0000'1000ui32, "MTRR"                   , "Memory Type Range Registers"          },
				{         amd            , 0x0000'2000ui32, "PGE"                    , "Page Global Bit"                      },
				{         amd            , 0x0000'4000ui32, "MCA"                    , "Machine Check Architecture"           },
				{         amd            , 0x0000'8000ui32, "CMOV"                   , "Conditional Move Instructions"        },
				{         amd            , 0x0001'0000ui32, "PAT"                    , "Page Attribute Table"                 },
				{         amd            , 0x0002'0000ui32, "PSE36"                  , "36-bit Page Size Extension"           },
				{ intel                  , 0x0010'0000ui32, "XD"                     , "Execute Disable Bit available"        },
				{         amd            , 0x0010'0000ui32, "NX"                     , "No-Execute page protection"           },
				{         amd            , 0x0040'0000ui32, "MmxExt"                 , "AMD extensions to MMX instructions"   },
				{         amd            , 0x0080'0000ui32, "MMX"                    , "MMX instructions"                     },
				{         amd            , 0x0100'0000ui32, "FXSR"                   , "FXSAVE and FXRSTOR instructions"      },
				{         amd            , 0x0200'0000ui32, "FFXSR"                  , "Fast FXSAVE and FXRSTOR"              },
				{ intel | amd            , 0x0400'0000ui32, "Page1GB"                , "1 GB large page support"              },
				{ intel | amd            , 0x0800'0000ui32, "RDTSCP"                 , "RDTSCP instruction"                   },
				{ intel                  , 0x2000'0000ui32, "EM64T"                  , "Intel 64 Architecture (Long Mode)"    },
				{         amd            , 0x2000'0000ui32, "LM"                     , "Long mode"                            },
				{         amd            , 0x4000'0000ui32, "ThreeDNowExt"           , "AMD extensions to 3DNow! instructions"},
				{         amd            , 0x8000'0000ui32, "ThreeDNow"              , "3DNow! instructions"                  },
			}}
		}}
	}},
	{ leaf_t::ras_advanced_power_management, {
		{ subleaf_t::main, {
			{ ebx, {
				{         amd            , 0x0000'0001ui32, "McaOverflowRecov"       , "MCA overflow recovery support"        },
				{         amd            , 0x0000'0002ui32, "SUCCOR"                 , "Software Uncorrectable Error Containment and Recovery"},
				{         amd            , 0x0000'0004ui32, "HWA"                    , "Hardware Assert supported"            },
				{         amd            , 0x0000'0008ui32, "ScalableMca"            , "Scalable MCA supported"               },

			}},
			{ edx, {
				{         amd            , 0x0000'0001ui32, "TS"                     , "Temperature Sensor"                   },
				{         amd            , 0x0000'0002ui32, "FID"                    , "Frequency ID control"                 },
				{         amd            , 0x0000'0004ui32, "VID"                    , "Voltage ID control"                   },
				{         amd            , 0x0000'0008ui32, "TTP"                    , "THERMTRIP"                            },
				{         amd            , 0x0000'0010ui32, "TM"                     , "Hardware thermal control (HTC)"       },
				{         amd            , 0x0000'0040ui32, "100MHzSteps"            , "100 MHz multiplier control"           },
				{         amd            , 0x0000'0080ui32, "HwPstate"               , "Hardware P-state control"             },
				{ intel | amd            , 0x0000'0100ui32, "TscInvariant"           , "TSC invariant"                        },
				{         amd            , 0x0000'0200ui32, "CPB"                    , "Core performance boost"               },
				{         amd            , 0x0000'0400ui32, "EffFreqRO"              , "Read-only effective frequency interface" },
				{         amd            , 0x0000'0800ui32, "ProcFeedbackInterface"  , "Processor feedback interface"         },
				{         amd            , 0x0000'1000ui32, "ApmPwrReporting"        , "APM power reporting"                  },
				{         amd            , 0x0000'2000ui32, "ConnectedStandby"       , "Connected Standby"                    },
				{         amd            , 0x0000'4000ui32, "RAPL"                   , "Running average power limit"          },
			}}
		}}
	}},
	{ leaf_t::address_limits, {
		{ subleaf_t::main, {
			{ ebx, {
				{         amd            , 0x0000'0001ui32, "CLZERO"                 , "CLZERO instruction"                   },
				{         amd            , 0x0000'0002ui32, "IRPerf"                 , "Instructions retired count support"   },
				{         amd            , 0x0000'0002ui32, "XSaveErPtr"             , "XSAVE (etc.) saves error pointer"     },
			}}
		}}
	}},
	{ leaf_t::secure_virtual_machine, {
		{ subleaf_t::main, {
			{ edx, {
				{         amd            , 0x0000'0001ui32, "NP"                     , "Nested paging"                        },
				{         amd            , 0x0000'0002ui32, "LbrVirt"                , "LBR virtualization"                   },
				{         amd            , 0x0000'0004ui32, "SVML"                   , "SVM lock"                             },
				{         amd            , 0x0000'0008ui32, "NRIPS"                  , "NRIP Save"                            },
				{         amd            , 0x0000'0010ui32, "TscRateMsr"             , "MSR-based TSC rate control"           },
				{         amd            , 0x0000'0020ui32, "VmcbClean"              , "VMCB clean bits"                      },
				{         amd            , 0x0000'0040ui32, "FlushByAsid"            , "Flush by ASID"                        },
				{         amd            , 0x0000'0080ui32, "DecodeAssists"          , "Decode assists"                       },
				{         amd            , 0x0000'0400ui32, "PauseFilter"            , "PAUSE intercept filter"               },
				{         amd            , 0x0000'1000ui32, "PauseFilterThreshold"   , "PAUSE filter threshold"               },
				{         amd            , 0x0000'2000ui32, "AVIC"                   , "AMD virtual interrupt controller"     },
				{         amd            , 0x0000'8000ui32, "V_VMSAVE_VMLOAD"        , "Virtualized VMSAVE/VMLOAD"            },
				{         amd            , 0x0001'0000ui32, "vGIF"                   , "Virtualized GIF"                      },

			}}
		}}
	}},
	{ leaf_t::performance_optimization, {
		{ subleaf_t::main, {
			{ eax, {
				{         amd            , 0x0000'0001ui32, "FP128"                  , "Full-width 128-bit SSE instructions"  },
				{         amd            , 0x0000'0002ui32, "MOVU"                   , "Prefer MOVU to MOVL/MOVH"             },
				{         amd            , 0x0000'0004ui32, "FP256"                  , "Full-width AVX256 instructions"       },
			}}
		}}
	}},
	{ leaf_t::instruction_based_sampling, {
		{ subleaf_t::main, {
			{ eax, {
				{         amd            , 0x0000'0001ui32, "IBSFFV"                 , "IBS feature flags valid"              },
				{         amd            , 0x0000'0002ui32, "FetchSam"               , "IBS fetch sampling supported"         },
				{         amd            , 0x0000'0004ui32, "OpSam"                  , "IBS execution sampling supported"     },
				{         amd            , 0x0000'0008ui32, "RdWrOpCnt"              , "Read/write of op counter supported"   },
				{         amd            , 0x0000'0010ui32, "OpCnt"                  , "Op counting mode supported"           },
				{         amd            , 0x0000'0020ui32, "BrnTarget"              , "Branch target address reporting supported" },
				{         amd            , 0x0000'0040ui32, "OpCntExt"               , "Op counters extended by 7 bits"       },
				{         amd            , 0x0000'0080ui32, "RipInvalidChk"          , "Invalid RIP indication supported"     },
				{         amd            , 0x0000'0100ui32, "OpBrnFuse"              , "Fused branch micro-op indication supported" },
				{         amd            , 0x0000'0200ui32, "IbsFetchCtlExtd"        , "IBS fetch control extended MSR supported" },
				{         amd            , 0x0000'0400ui32, "IbsOpData4"             , "IBS op data 4 MSR supported"          },
			}}
		}}
	}},
	{ leaf_t::lightweight_profiling, {
		{ subleaf_t::main, {
			{ eax, {
				{         amd            , 0x0000'0001ui32, "LwpAvail"               , "Lightweight profiling supported"      },
				{         amd            , 0x0000'0002ui32, "LwpVAL"                 , "LWPVAL instruction supported"         },
				{         amd            , 0x0000'0004ui32, "LwpIRE"                 , "Instructions retired event"           },
				{         amd            , 0x0000'0008ui32, "LwpBRE"                 , "Branch retired event"                 },
				{         amd            , 0x0000'0010ui32, "LwpDME"                 , "DC miss event"                        },
				{         amd            , 0x0000'0020ui32, "LwpCNH"                 , "Core clocks not halted"               },
				{         amd            , 0x0000'0040ui32, "LwpRNH"                 , "Core reference clocks not halted"     },
				{         amd            , 0x2000'0000ui32, "LwpCont"                , "Samping in continuous mode"           },
				{         amd            , 0x4000'0000ui32, "LwpPTSC"                , "Performance TSC in event record"      },
				{         amd            , 0x8000'0000ui32, "LwpInt"                 , "Interrupt on threshold overflow"      },
			}},
			{ ecx, {
				{         amd            , 0x0000'0020ui32, "LwpDataAddress"         , "Data cache miss address valid"        },
				{         amd            , 0x1000'0000ui32, "LwpBranchPrediction"    , "Branch prediction filtering supported"},
				{         amd            , 0x2000'0000ui32, "LwpIpFiltering"         , "IP filtering supported"               },
				{         amd            , 0x4000'0000ui32, "LwpCacheLevels"         , "Cache level filtering supported"      },
				{         amd            , 0x8000'0000ui32, "LwpCacheLatency"        , "Cache latency filtering supported"    },
			}}
		}}
	}},
	{ leaf_t::encrypted_memory, {
		{ subleaf_t::main, {
			{ eax, {
				{         amd            , 0x0000'0001ui32, "SME"                    , "Secure Memory Encryption supported"   },
				{         amd            , 0x0000'0002ui32, "SEV"                    , "Secure Encrypted Virtualization supported" },
				{         amd            , 0x0000'0004ui32, "PageFlushMsr"           , "Page Flush MSR available"             },
				{         amd            , 0x0000'0008ui32, "SEV-ES"                 , "SEV Encrypted State available"        },
			}}
		}}
	}}
};

void print_features(const cpu_t& cpu, leaf_t leaf, subleaf_t sub, register_t reg) {
	const auto range = all_features.equal_range(leaf);
	for(auto it = range.first; it != range.second; ++it) {
		if(it->second.find(sub) == it->second.end()) {
			continue;
		}
		auto subleaf = it->second.at(sub);
		if(subleaf.find(reg) == subleaf.end()) {
			continue;
		}

		const std::vector<feature_t>& features = subleaf.at(reg);
		const std::uint32_t value = cpu.leaves.at(leaf).at(sub).at(reg);

		for(const feature_t& f : features) {
			if(cpu.vendor & f.vendor) {
				if(0 != (value & f.mask)) {
					std::cout << std::setw(24) << std::setfill(' ') << f.mnemonic << " \x1b[32;1m[+]\x1b[0m " << f.description << "\n";
				} else {
					std::cout << std::setw(24) << std::setfill(' ') << f.mnemonic << " \x1b[31;1m[-]\x1b[0m " << f.description << "\n";
				}
			}
		}
	}
	std::cout << std::flush;
}

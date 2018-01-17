#include "stdafx.h"

#include "features.hpp"

#include <cstddef>
#include <iostream>
#include <iomanip>
#include <map>
#include <vector>

using feature_map_t = std::map<leaf_t, std::map<subleaf_t, std::map<register_t, std::vector<feature_t>>>>;

const feature_map_t all_features = {
	{ leaf_t::version_info, {
		{ subleaf_t::main, {
			{ ecx, {
				{ intel | amd            , 0x0000'0001ui32, "SSE3"         , "SSE3 Extensions" },
				{ intel | amd            , 0x0000'0002ui32, "PCLMULQDQ"    , "Carryless Multiplication" },
				{ intel                  , 0x0000'0004ui32, "DTES64"       , "64-bit DS Area" },
				{ intel | amd            , 0x0000'0008ui32, "MONITOR"      , "MONITOR/MWAIT" },
				{ intel                  , 0x0000'0010ui32, "DS-CPL"       , "CPL Qualified Debug Store" },
				{ intel                  , 0x0000'0020ui32, "VMX"          , "Virtual Machine Extensions" },
				{ intel                  , 0x0000'0040ui32, "SMX"          , "Safer Mode Extensions" },
				{ intel                  , 0x0000'0080ui32, "EIST"         , "Enhanced Intel SpeedStep Technology" },
				{ intel                  , 0x0000'0100ui32, "TM2"          , "Thermal Monitor 2" },
				{ intel | amd            , 0x0000'0200ui32, "SSSE3"        , "SSSE3 Extensions" },
				{ intel                  , 0x0000'0400ui32, "CNXT-ID"      , "L1 Context ID" },
				{ intel                  , 0x0000'0800ui32, "SDBG"         , "IA32_DEBUG_INTERFACE for silicon debug" },
				{ intel | amd            , 0x0000'1000ui32, "FMA"          , "Fused Multiply Add" },
				{ intel | amd            , 0x0000'2000ui32, "CMPXCHG16B"   , "CMPXCHG16B instruction" },
				{ intel                  , 0x0000'4000ui32, "xTPR"         , "xTPR update control" },
				{ intel                  , 0x0000'8000ui32, "PDCM"         , "Perfmon and Debug Capability" },
				//{ intel | amd            , 0x0001'0000ui32, ""             , "Reserved"},
				{ intel | amd            , 0x0002'0000ui32, "PCID"         , "Process-context identifiers" },
				{ intel                  , 0x0004'0000ui32, "DCA"          , "Direct Cache Access" },
				{ intel | amd            , 0x0008'0000ui32, "SSE4.1"       , "SSE4.1 Extensions" },
				{ intel | amd            , 0x0010'0000ui32, "SSE4.2"       , "SSE4.2 Extensions" },
				{ intel | amd            , 0x0020'0000ui32, "x2APIC"       , "x2APIC" },
				{ intel | amd            , 0x0040'0000ui32, "MOVBE"        , "MOVBE instruction" },
				{ intel | amd            , 0x0080'0000ui32, "POPCNT"       , "POPCNT instruction" },
				{ intel | amd            , 0x0100'0000ui32, "TSC-Deadline" , "One-shot APIC timers using a TSC deadline" },
				{ intel | amd            , 0x0200'0000ui32, "AESNI"        , "AESNI instruction extensions" },
				{ intel | amd            , 0x0400'0000ui32, "XSAVE"        , "XSAVE/XRSTOR feature" },
				{ intel | amd            , 0x0800'0000ui32, "OSXSAVE"      , "OS has set CR4.OSXSAVE" },
				{ intel | amd            , 0x1000'0000ui32, "AVX"          , "AVX instructions" },
				{ intel | amd            , 0x2000'0000ui32, "F16C"         , "16-bit floating-point conversion instructions" },
				{ intel | amd            , 0x4000'0000ui32, "RDRAND"       , "RDRAND instruction" },
				{ any                    , 0x8000'0000ui32, "(hypervisor)" , "Hypervisor guest" }
			}},
			{ edx, {
				{ intel | amd | transmeta, 0x0000'0001ui32, "FPU"          , "x87 FPU on chip"},
				{ intel | amd | transmeta, 0x0000'0002ui32, "VME"          , "Virtual 8086 Mode Enhancements"},
				{ intel | amd | transmeta, 0x0000'0004ui32, "DE"           , "Debugging Extensions"},
				{ intel | amd | transmeta, 0x0000'0008ui32, "PSE"          , "Page Size Extension"},
				{ intel | amd | transmeta, 0x0000'0010ui32, "TSC"          , "Time Stamp Counter"},
				{ intel | amd | transmeta, 0x0000'0020ui32, "MSR"          , "RDMSR and WRMSR Instructions"},
				{ intel | amd            , 0x0000'0040ui32, "PAE"          , "Physical Address Extension"},
				{ intel | amd            , 0x0000'0080ui32, "MCE"          , "Machine Check Exception"},
				{ intel | amd | transmeta, 0x0000'0100ui32, "CX8"          , "CMPXCHG8B Instruction"},
				{ intel | amd            , 0x0000'0200ui32, "APIC"         , "APIC On-Chip"},
				//{ intel | amd            , 0x0000'0400ui32, ""             , "Reserved"},
				{ intel | amd | transmeta, 0x0000'0800ui32, "SEP"          , "SYSENTER and SYSEXIT Instructions"},
				{ intel | amd            , 0x0000'1000ui32, "MTRR"         , "Memory Type Range Registers"},
				{ intel | amd            , 0x0000'2000ui32, "PGE"          , "Page Global Bit"},
				{ intel | amd            , 0x0000'4000ui32, "MCA"          , "Machine Check Architecture"},
				{ intel | amd | transmeta, 0x0000'8000ui32, "CMOV"         , "Conditional Move Instructions"},
				{ intel | amd            , 0x0001'0000ui32, "PAT"          , "Page Attribute Table"},
				{ intel | amd            , 0x0002'0000ui32, "PSE-36"       , "36-bit Page Size Extension"},
				{ intel       | transmeta, 0x0004'0000ui32, "PSN"          , "Processor Serial Number"},
				{ intel | amd            , 0x0008'0000ui32, "CLFSH"        , "CLFLUSH Instruction"},
				//{ intel | amd            , 0x0010'0000ui32, ""             , "Reserved"},
				{ intel                  , 0x0020'0000ui32, "DS"           , "Debug Store"},
				{ intel                  , 0x0040'0000ui32, "ACPI"         , "Thermal Monitoring and Software Controlled Clock Facilities"},
				{ intel | amd | transmeta, 0x0080'0000ui32, "MMX"          , "Intel MMX Technology"},
				{ intel | amd            , 0x0100'0000ui32, "FXSR"         , "FXSAVE and FXRSTOR Instructions"},
				{ intel | amd            , 0x0200'0000ui32, "SSE"          , "SSE Extensions"},
				{ intel | amd            , 0x0400'0000ui32, "SSE2"         , "SSE2 Extensions"},
				{ intel                  , 0x0800'0000ui32, "SS"           , "Self Snoop"},
				{ intel | amd            , 0x1000'0000ui32, "HTT"          , "Max APIC IDs reserved field is Valid"},
				{ intel                  , 0x2000'0000ui32, "TM"           , "Thermal Monitor"},
				{ intel                  , 0x4000'0000ui32, "IA64"         , "IA64 emulating x86"},
				{ intel                  , 0x8000'0000ui32, "PBE"          , "Pending Break Enable"}
			}}
		}}
	}},
	{ leaf_t::thermal_and_power, {
		{ subleaf_t::main, {
			{ eax, {
				{ intel                  , 0x0000'0001ui32, "DTS"          , "Digital temperature sensor" },
				{ intel                  , 0x0000'0002ui32, "TBT"          , "Intel Turbo Boost Technology" },
				{ intel | amd            , 0x0000'0004ui32, "ARAT"         , "APIC-Timer-always-running" },
				//{ intel                  , 0x0000'0008ui32, ""             , "Reserved" },
				{ intel                  , 0x0000'0010ui32, "PLN"          , "Power limit notification controls" },
				{ intel                  , 0x0000'0020ui32, "ECMD"         , "Clock modulation duty cycle extension" },
				{ intel                  , 0x0000'0040ui32, "PTM"          , "Package thermal management" },
				{ intel                  , 0x0000'0080ui32, "HWP"          , "Hardware Managed Performance States: HWP_CAPABILITIES, HWP_REQUEST, HWP_STATUS" },
				{ intel                  , 0x0000'0100ui32, "HWP_N"        , "HWP_Notification                   : HWP_INTERRUPT" },
				{ intel                  , 0x0000'0200ui32, "HWP_AW"       , "HWP_Activity_Window                : HWP_REQUEST[41:32]" },
				{ intel                  , 0x0000'0400ui32, "HWP_EPP"      , "HWP_Energy_Performance_Preference  : HWP_REQUEST[31:24]" },
				{ intel                  , 0x0000'0800ui32, "HWP_PLR"      , "HWP_Package_Level_Request          : HWP_REQUEST_PKG" },
				//{ intel                  , 0x0000'1000ui32, ""             , "Reserved" },
				{ intel                  , 0x0000'2000ui32, "HDC"          , "HDC_CTL, HDC_CTL1, THREAD_STALL" },
				{ intel                  , 0x0000'4000ui32, "TBT3"         , "Intel Turbo Boost Max Technology 3.0" }
			}},
			{ ecx, {
				{ intel                  , 0x0000'0001ui32, "HCF"          , "Hardware Coordination Feedback Capability: MPERF, APERF"},
				{ intel                  , 0x0000'0008ui32, "PERF_BIAS"    , "Performance-energy bias preference       : ENERGY_PERF_BIAS" }
			}}
		}}
	}},
	{ leaf_t::extended_features, {
		{ subleaf_t::main, {
			{ ebx, {
				{ intel | amd            , 0x0000'0001ui32, "FSGSBASE"     , "FSGSBASE instructions" },
				{ intel                  , 0x0000'0002ui32, "TSC_ADJUST"   , "TSC_ADJUST MSR" },
				{ intel                  , 0x0000'0004ui32, "SGX"          , "Softward Guard Extensions" },
				{ intel | amd            , 0x0000'0008ui32, "BMI1"         , "Bit Manipulation Instructions 1" },
				{ intel                  , 0x0000'0010ui32, "HLE"          , "Hardware Lock Elision" },
				{ intel | amd            , 0x0000'0020ui32, "AVX2"         , "Advanced Vector Extensions 2.0 instructions" },
				{ intel                  , 0x0000'0040ui32, "FDP_EXCPT"    , "x87 FPU Data Pointer updated only on x87 exceptions" },
				{ intel | amd            , 0x0000'0080ui32, "SMEP"         , "Supervisor-Mode Execution Prevention" },
				{ intel | amd            , 0x0000'0100ui32, "BMI2"         , "Bit Manipulation Instructions 2" },
				{ intel                  , 0x0000'0200ui32, "EREPMOVSB"    , "Enhanced REP MOVSB/REP STOSB" },
				{ intel                  , 0x0000'0400ui32, "INVPCID"      , "INVPCID instruction" },
				{ intel                  , 0x0000'0800ui32, "RTM"          , "Restricted Transactional Memory" },
				{ intel                  , 0x0000'1000ui32, "RDT-M"        , "Resource Director Technology Monitoring" },
				{ intel                  , 0x0000'2000ui32, "FPU-CSDS"     , "x87 FPU CS and DS deprecated" },
				{ intel                  , 0x0000'4000ui32, "MPX"          , "Memory Protection Extensions" },
				{ intel                  , 0x0000'8000ui32, "RDT-A"        , "Resource Director Technology Allocation" },
				{ intel                  , 0x0001'0000ui32, "AVX512F"      , "AVX512 Foundation" },
				{ intel                  , 0x0002'0000ui32, "AVX512DQ"     , "AVX512 Double/Quadword Instructions" },
				{ intel | amd            , 0x0004'0000ui32, "RDSEED"       , "RDSEED instruction" },
				{ intel | amd            , 0x0008'0000ui32, "ADX"          , "Multi-Precision Add-Carry Instructions" },
				{ intel | amd            , 0x0010'0000ui32, "SMAP"         , "Supervisor-Mode Access Prevention" },
				{ intel                  , 0x0020'0000ui32, "AVX512_IFMA"  , "AVX512 Integer FMA" },
				{         amd            , 0x0040'0000ui32, "PCOMMIT"      , "Persistent Commit" },
				{ intel | amd            , 0x0080'0000ui32, "CLFLUSHOPT"   , "CLFLUSHOPT instruction" },
				{ intel                  , 0x0100'0000ui32, "CLWB"         , "CLWB instruction" },
				{ intel                  , 0x0200'0000ui32, "IPT"          , "Intel Processor Trace" },
				{ intel                  , 0x0400'0000ui32, "AVX512PF"     , "AVX512 Prefetch" },
				{ intel                  , 0x0800'0000ui32, "AVX512ER"     , "AVX512 Exponential and Reciprocal Instructions" },
				{ intel                  , 0x1000'0000ui32, "AVX512CD"     , "AVX512 Conflict Detection Instructions" },
				{ intel | amd            , 0x2000'0000ui32, "SHA"          , "SHA Extensions" },
				{ intel                  , 0x4000'0000ui32, "AVX512BW"     , "AVX512 Byte/Word Instructions" },
				{ intel                  , 0x8000'0000ui32, "AVX512VL"     , "AVX512 Vector Length Instructions" },
			}},
			{ ecx, {
				{ intel                  , 0x0000'0001ui32, "PREFETCHW1"   , "PREFETCHW1 instruction" },
				{ intel                  , 0x0000'0002ui32, "AVX512_VBMI"  , "AVX512 Vector Bit Manipulation Instructions" },
				{ intel                  , 0x0000'0004ui32, "UMIP"         , "User Mode Instruction Prevention" },
				{ intel                  , 0x0000'0008ui32, "PKU"          , "Protection Keys for User-mode pages" },
				{ intel                  , 0x0000'0010ui32, "OSPKE"        , "OS has set CR4.PKE" },
				{ intel                  , 0x0040'0000ui32, "RDPID"        , "Read Processor ID" },
				{ intel                  , 0x4000'0000ui32, "SGX_LC"       , "SGX Launch Configuration" },
			}},
			{ edx, {
				{ intel                  , 0x0000'0004ui32, "AVX512_4NNIW" , "AVX512 4-register Neural Network Instructions" },
				{ intel                  , 0x0000'0008ui32, "AVX512_4FMAPS", "AVX512 4-register Neural Network Instructions" },
				{ intel | amd            , 0x0400'0000ui32, "IBRS"         , "Indirect Branch Restricted Speculation and Indirect Branch Predictor Barrier" },
				{ intel | amd            , 0x0800'0000ui32, "STIBP"        , "Single Thread Indirect Branch Predictors" },
				{ intel                  , 0x2000'0000ui32, "ARCH_CAPS"    , "ARCH_CAPABILITIES MSR" },
			}}
		}}
	}}
};

void print_features(leaf_t leaf, subleaf_t sub, register_t reg, const cpu_t& cpu) {
	const std::vector<feature_t>& features = all_features.at(leaf).at(sub).at(reg);
	const std::uint32_t value              = cpu.features.at(leaf).at(sub).at(reg);

	for(const feature_t& f : features) {
		if(0 != (cpu.vendor & f.vendor)) {
			if(0 != (value & f.mask)) {
				std::cout << std::setw(12) << std::setfill(' ') << f.mnemonic << " \x1b[32;1m[+]\x1b[0m " << f.description << "\n";
			} else {
				std::cout << std::setw(12) << std::setfill(' ') << f.mnemonic << " \x1b[31;1m[-]\x1b[0m " << f.description << "\n";
			}
		}
	}
	std::cout << std::flush;
}

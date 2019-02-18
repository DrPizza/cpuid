#include "stdafx.h"

#include "features.hpp"

#include "utility.hpp"

#include <cstddef>
#include <map>
#include <vector>

#if !defined(_MSC_VER)
#include <x86intrin.h>
#define __popcnt __builtin_popcount
#endif

const feature_map_t all_features = {
	{ leaf_type::version_info, {
		{ subleaf_type::main, {
			{ ecx, {
				{ intel | amd            , 0x0000'0001_u32, "SSE3"              , "SSE3 Extensions"                               , "pni"          },
				{ intel | amd            , 0x0000'0002_u32, "PCLMULQDQ"         , "Carryless Multiplication"                      , "pclmulqdq"    },
				{ intel                  , 0x0000'0004_u32, "DTES64"            , "64-bit DS Area"                                , "dtes64"       },
				{ intel | amd            , 0x0000'0008_u32, "MONITOR"           , "MONITOR/MWAIT"                                 , "monitor"      },
				{ intel                  , 0x0000'0010_u32, "DS-CPL"            , "CPL Qualified Debug Store"                     , "ds_cpl"       },
				{ intel                  , 0x0000'0020_u32, "VMX"               , "Virtual Machine Extensions"                    , "vmx"          },
				{ intel                  , 0x0000'0040_u32, "SMX"               , "Safer Mode Extensions"                         , "smx"          },
				{ intel                  , 0x0000'0080_u32, "EIST"              , "Enhanced Intel SpeedStep Technology"           , "est"          },
				{ intel                  , 0x0000'0100_u32, "TM2"               , "Thermal Monitor 2"                             , "tm2"          },
				{ intel | amd            , 0x0000'0200_u32, "SSSE3"             , "SSSE3 Extensions"                              , "ssse3"        },
				{ intel                  , 0x0000'0400_u32, "CNXT-ID"           , "L1 Context ID"                                 , "cid"          },
				{ intel                  , 0x0000'0800_u32, "SDBG"              , "IA32_DEBUG_INTERFACE for silicon debug"        , "sdbg"         },
				{ intel | amd            , 0x0000'1000_u32, "FMA"               , "Fused Multiply Add"                            , "fma"          },
				{ intel | amd            , 0x0000'2000_u32, "CMPXCHG16B"        , "CMPXCHG16B instruction"                        , "cx16"         },
				{ intel                  , 0x0000'4000_u32, "xTPR"              , "xTPR update control"                           , "xtpr"         },
				{ intel                  , 0x0000'8000_u32, "PDCM"              , "Perfmon and Debug Capability"                  , "pdcm"         },
				{ intel | amd            , 0x0002'0000_u32, "PCID"              , "Process-context identifiers"                   , "pcid"         },
				{ intel                  , 0x0004'0000_u32, "DCA"               , "Direct Cache Access"                           , "dca"          },
				{ intel | amd            , 0x0008'0000_u32, "SSE4.1"            , "SSE4.1 Extensions"                             , "sse4_1"       },
				{ intel | amd            , 0x0010'0000_u32, "SSE4.2"            , "SSE4.2 Extensions"                             , "sse4_2"       },
				{ intel | amd            , 0x0020'0000_u32, "x2APIC"            , "x2APIC"                                        , "x2apic"       },
				{ intel | amd            , 0x0040'0000_u32, "MOVBE"             , "MOVBE instruction"                             , "movbe"        },
				{ intel | amd            , 0x0080'0000_u32, "POPCNT"            , "POPCNT instruction"                            , "popcnt"       },
				{ intel | amd            , 0x0100'0000_u32, "TSC-Deadline"      , "One-shot APIC timers using a TSC deadline"     , "tsc_deadline" },
				{ intel | amd            , 0x0200'0000_u32, "AESNI"             , "AESNI instruction extensions"                  , "aes"          },
				{ intel | amd            , 0x0400'0000_u32, "XSAVE"             , "XSAVE/XRSTOR feature"                          , "xsave"        },
				{ intel | amd            , 0x0800'0000_u32, "OSXSAVE"           , "OS has set CR4.OSXSAVE"                        , ""             },
				{ intel | amd            , 0x1000'0000_u32, "AVX"               , "AVX instructions"                              , "avx"          },
				{ intel | amd            , 0x2000'0000_u32, "F16C"              , "16-bit floating-point conversion instructions" , "f16c"         },
				{ intel | amd            , 0x4000'0000_u32, "RDRAND"            , "RDRAND instruction"                            , "rdrand"       },
				{ any                    , 0x8000'0000_u32, "RAZ"               , "Hypervisor guest"                              , "hypervisor"   },
			}},
			{ edx, {
				{ intel | amd | transmeta, 0x0000'0001_u32, "FPU"               , "x87 FPU on chip"                                             , "fpu"     },
				{ intel | amd | transmeta, 0x0000'0002_u32, "VME"               , "Virtual 8086 Mode Enhancements"                              , "vme"     },
				{ intel | amd | transmeta, 0x0000'0004_u32, "DE"                , "Debugging Extensions"                                        , "de"      },
				{ intel | amd | transmeta, 0x0000'0008_u32, "PSE"               , "Page Size Extension"                                         , "pse"     },
				{ intel | amd | transmeta, 0x0000'0010_u32, "TSC"               , "Time Stamp Counter"                                          , "tsc"     },
				{ intel | amd | transmeta, 0x0000'0020_u32, "MSR"               , "RDMSR and WRMSR Instructions"                                , "msr"     },
				{ intel | amd            , 0x0000'0040_u32, "PAE"               , "Physical Address Extension"                                  , "pae"     },
				{ intel | amd            , 0x0000'0080_u32, "MCE"               , "Machine Check Exception"                                     , "mce"     },
				{ intel | amd | transmeta, 0x0000'0100_u32, "CX8"               , "CMPXCHG8B Instruction"                                       , "cx8"     },
				{ intel | amd            , 0x0000'0200_u32, "APIC"              , "APIC On-Chip"                                                , "apic"    },
				{ intel | amd | transmeta, 0x0000'0800_u32, "SEP"               , "SYSENTER and SYSEXIT Instructions"                           , "sep"     },
				{ intel | amd            , 0x0000'1000_u32, "MTRR"              , "Memory Type Range Registers"                                 , "mtrr"    },
				{ intel | amd            , 0x0000'2000_u32, "PGE"               , "Page Global Bit"                                             , "pge"     },
				{ intel | amd            , 0x0000'4000_u32, "MCA"               , "Machine Check Architecture"                                  , "mca"     },
				{ intel | amd | transmeta, 0x0000'8000_u32, "CMOV"              , "Conditional Move Instructions"                               , "cmov"    },
				{ intel | amd            , 0x0001'0000_u32, "PAT"               , "Page Attribute Table"                                        , "pat"     },
				{ intel | amd            , 0x0002'0000_u32, "PSE-36"            , "36-bit Page Size Extension"                                  , "pse36"   },
				{ intel       | transmeta, 0x0004'0000_u32, "PSN"               , "Processor Serial Number"                                     , "pn"      },
				{ intel | amd            , 0x0008'0000_u32, "CLFSH"             , "CLFLUSH Instruction"                                         , "clflush" },
				{ intel                  , 0x0020'0000_u32, "DS"                , "Debug Store"                                                 , "dts"     },
				{ intel                  , 0x0040'0000_u32, "ACPI"              , "Thermal Monitoring and Software Controlled Clock Facilities" , "acpi"    },
				{ intel | amd | transmeta, 0x0080'0000_u32, "MMX"               , "Intel MMX Technology"                                        , "mmx"     },
				{ intel | amd            , 0x0100'0000_u32, "FXSR"              , "FXSAVE and FXRSTOR Instructions"                             , "fxsr"    },
				{ intel | amd            , 0x0200'0000_u32, "SSE"               , "SSE Extensions"                                              , "sse"     },
				{ intel | amd            , 0x0400'0000_u32, "SSE2"              , "SSE2 Extensions"                                             , "sse2"    },
				{ intel                  , 0x0800'0000_u32, "SS"                , "Self Snoop"                                                  , "ss"      },
				{ intel | amd            , 0x1000'0000_u32, "HTT"               , "Hyperthreading/Max APIC IDs reserved field is Valid"         , "ht"      },
				{ intel                  , 0x2000'0000_u32, "TM"                , "Thermal Monitor"                                             , "tm"      },
				{ intel                  , 0x4000'0000_u32, "IA64"              , "IA64 emulating x86"                                          , "ia64"    },
				{ intel                  , 0x8000'0000_u32, "PBE"               , "Pending Break Enable"                                        , "pbe"     },
			}}
		}}
	}},
	{ leaf_type::monitor_mwait, {
		{ subleaf_type::main, {
			{ ecx, {
				{ intel                  , 0x0000'0002_u32, ""                  , "Supports treating interrupts as a break event." },
			}}
		}}
	}},
	{ leaf_type::thermal_and_power, {
		{ subleaf_type::main, {
			{ eax, {
				{ intel                  , 0x0000'0001_u32, "DTS"               , "Digital temperature sensor"                                                     , "dtherm"         },
				{ intel                  , 0x0000'0002_u32, "TBT"               , "Intel Turbo Boost Technology"                                                   , "ida"            },
				{ intel | amd            , 0x0000'0004_u32, "ARAT"              , "APIC-Timer-always-running"                                                      , "arat"           },
				{ intel                  , 0x0000'0010_u32, "PLN"               , "Power limit notification controls"                                              , "pln"            },
				{ intel                  , 0x0000'0020_u32, "ECMD"              , "Clock modulation duty cycle extension"                                          , ""               },
				{ intel                  , 0x0000'0040_u32, "PTM"               , "Package thermal management"                                                     , "pts"            },
				{ intel                  , 0x0000'0080_u32, "HWP"               , "Hardware Managed Performance States: HWP_CAPABILITIES, HWP_REQUEST, HWP_STATUS" , "hwp"            },
				{ intel                  , 0x0000'0100_u32, "HWP_N"             , "HWP_Notification                   : HWP_INTERRUPT"                             , "hwp_notify"     },
				{ intel                  , 0x0000'0200_u32, "HWP_AW"            , "HWP_Activity_Window                : HWP_REQUEST[41:32]"                        , "hwp_act_window" },
				{ intel                  , 0x0000'0400_u32, "HWP_EPP"           , "HWP_Energy_Performance_Preference  : HWP_REQUEST[31:24]"                        , "hwp_epp"        },
				{ intel                  , 0x0000'0800_u32, "HWP_PLR"           , "HWP_Package_Level_Request          : HWP_REQUEST_PKG"                           , "hwp_pkg_req"    },
				{ intel                  , 0x0000'2000_u32, "HDC"               , "HDC_CTL, HDC_CTL1, THREAD_STALL"                                                , ""               },
				{ intel                  , 0x0000'4000_u32, "TBT3"              , "Intel Turbo Boost Max Technology 3.0"                                           , ""               },
				{ intel                  , 0x0000'8000_u32, "HWP_Cap"           , "HWP Capabilities. Highest Performance change is supported."                     , ""               },
				{ intel                  , 0x0001'0000_u32, "HWP_PECI"          , "HWP PECI Override."                                                             , ""               },
				{ intel                  , 0x0002'0000_u32, "Flexible_HWP"      , "Flexible HWP"                                                                   , ""               },
				{ intel                  , 0x0004'0000_u32, "Fast_Access"       , "Fast access mode for HWP_REQUEST MSR"                                           , ""               },
				{ intel                  , 0x0010'0000_u32, "Ignore_Idle"       , "Ignore Idle Logical Processor HWP request"                                      , ""               },
			}},
			{ ecx, {
				{ intel | amd            , 0x0000'0001_u32, "HCF"               , "Hardware Coordination Feedback Capability: MPERF, APERF"     , "aperfmperf" },
				{ intel                  , 0x0000'0008_u32, "PERF_BIAS"         , "Performance-energy bias preference       : ENERGY_PERF_BIAS" , "epb"        },
			}}
		}}
	}},
	{ leaf_type::extended_features, {
		{ subleaf_type::extended_features_main, {
			{ ebx, {
				{ intel | amd            , 0x0000'0001_u32, "FSGSBASE"          , "FSGSBASE instructions"                               , "fsgsbase"       },
				{ intel                  , 0x0000'0002_u32, "TSC_ADJUST"        , "TSC_ADJUST MSR"                                      , "tsc_adjust"     },
				{ intel                  , 0x0000'0004_u32, "SGX"               , "Softward Guard Extensions"                           , "sgx"            },
				{ intel | amd            , 0x0000'0008_u32, "BMI1"              , "Bit Manipulation Instructions 1"                     , "bmi1"           },
				{ intel                  , 0x0000'0010_u32, "HLE"               , "Hardware Lock Elision"                               , "hle"            },
				{ intel | amd            , 0x0000'0020_u32, "AVX2"              , "Advanced Vector Extensions 2.0 instructions"         , "avx2"           },
				{ intel                  , 0x0000'0040_u32, "FDP_EXCPTN_ONLY"   , "x87 FPU Data Pointer updated only on x87 exceptions" , ""               },
				{ intel | amd            , 0x0000'0080_u32, "SMEP"              , "Supervisor-Mode Execution Prevention"                , "smep"           },
				{ intel | amd            , 0x0000'0100_u32, "BMI2"              , "Bit Manipulation Instructions 2"                     , "bmi2"           },
				{ intel                  , 0x0000'0200_u32, "EREPMOVSB"         , "Enhanced REP MOVSB/REP STOSB"                        , "erms"           },
				{ intel                  , 0x0000'0400_u32, "INVPCID"           , "INVPCID instruction"                                 , "invpcid"        },
				{ intel                  , 0x0000'0400_u32, ""                  , ""                                                    , "invpcid_single" },
				{ intel                  , 0x0000'0800_u32, "RTM"               , "Restricted Transactional Memory"                     , "rtm"            },
				{ intel                  , 0x0000'1000_u32, "RDT-M"             , "Resource Director Technology Monitoring"             , "cqm"            },
				{ intel                  , 0x0000'2000_u32, "FPU-CSDS"          , "x87 FPU CS and DS deprecated"                        , ""               },
				{ intel                  , 0x0000'4000_u32, "MPX"               , "Memory Protection Extensions"                        , "mpx"            },
				{ intel                  , 0x0000'8000_u32, "RDT-A"             , "Resource Director Technology Allocation"             , "rdt_a"          },
				{ intel                  , 0x0001'0000_u32, "AVX512F"           , "AVX512 Foundation"                                   , "avx512f"        },
				{ intel                  , 0x0002'0000_u32, "AVX512DQ"          , "AVX512 Double/Quadword Instructions"                 , "avx512dq"       },
				{ intel | amd            , 0x0004'0000_u32, "RDSEED"            , "RDSEED instruction"                                  , "rdseed"         },
				{ intel | amd            , 0x0008'0000_u32, "ADX"               , "Multi-Precision Add-Carry Instructions"              , "adx"            },
				{ intel | amd            , 0x0010'0000_u32, "SMAP"              , "Supervisor-Mode Access Prevention"                   , "smap"           },
				{ intel                  , 0x0020'0000_u32, "AVX512_IFMA"       , "AVX512 Integer FMA"                                  , "avx512ifma"     },
				{         amd            , 0x0040'0000_u32, "PCOMMIT"           , "Persistent Commit"                                   , ""               },
				{ intel | amd            , 0x0080'0000_u32, "CLFLUSHOPT"        , "CLFLUSHOPT instruction"                              , "clflushopt"     },
				{ intel                  , 0x0100'0000_u32, "CLWB"              , "CLWB instruction"                                    , "clwb"           },
				{ intel                  , 0x0200'0000_u32, "IPT"               , "Intel Processor Trace"                               , "intel_pt"       },
				{ intel                  , 0x0400'0000_u32, "AVX512PF"          , "AVX512 Prefetch"                                     , "avx512pf"       },
				{ intel                  , 0x0800'0000_u32, "AVX512ER"          , "AVX512 Exponential and Reciprocal Instructions"      , "avx512er"       },
				{ intel                  , 0x1000'0000_u32, "AVX512CD"          , "AVX512 Conflict Detection Instructions"              , "avx512cd"       },
				{ intel | amd            , 0x2000'0000_u32, "SHA"               , "SHA Extensions"                                      , "sha_ni"         },
				{ intel                  , 0x4000'0000_u32, "AVX512BW"          , "AVX512 Byte/Word Instructions"                       , "avx512bw"       },
				{ intel                  , 0x8000'0000_u32, "AVX512VL"          , "AVX512 Vector Length Instructions"                   , "avx512vl"       },
			}},
			{ ecx, {
				{ intel                  , 0x0000'0001_u32, "PREFETCHW1"        , "PREFETCHW1 instruction"                        , ""                 },
				{ intel                  , 0x0000'0002_u32, "AVX512_VBMI"       , "AVX512 Vector Bit Manipulation Instructions"   , "avx512bmi"        },
				{ intel                  , 0x0000'0004_u32, "UMIP"              , "User Mode Instruction Prevention"              , "umip"             },
				{ intel                  , 0x0000'0008_u32, "PKU"               , "Protection Keys for User-mode pages"           , "pku"              },
				{ intel                  , 0x0000'0010_u32, "OSPKE"             , "OS has set CR4.PKE"                            , "ospke"            },
				{ intel                  , 0x0000'0020_u32, "WAITPKG"           , "Wait and pause enhancements"                   , ""                 },
				{ intel                  , 0x0000'0040_u32, "AVX512_VBMI2"      , "AVX512 Vector Bit Manipulation Instructions 2" , "avx512_vbmi2"     },
				{ intel                  , 0x0000'0100_u32, "GFNI"              , "Galois Field NI"                               , "gfni"             },
				{ intel                  , 0x0000'0200_u32, "VAES"              , "VEX-AES-NI"                                    , "vaes"             },
				{ intel                  , 0x0000'0400_u32, "VPCLMULQDQ"        , "VEX-PCLMUL"                                    , "vpclmulqdq"       },
				{ intel                  , 0x0000'0800_u32, "AVX512_VNNI"       , "AVX512 Vector Neural Net Instructions"         , "avx512_vnni"      },
				{ intel                  , 0x0000'1000_u32, "AVX512_BITALG"     , "AVX512 Bitwise Algorithms"                     , "avx512_bitalg"    },
				{ intel                  , 0x0000'2000_u32, "TME"               , "Intel Total Memory Encryption"                 , "tme"              },
				{ intel                  , 0x0000'4000_u32, "AVX512_VPOPCNTDQ"  , "AVX512 VPOPCNTDQ"                              , "avx512_vpopcntdq" },
				{ intel                  , 0x0001'0000_u32, "LA57"              , "5-level page tables/57-bit virtual addressing" , "la57"             },
				{ intel                  , 0x003e'0000_u32, "MAWAU"             , "MPX Address Width Adjust for User addresses"   , ""                 },
				{ intel                  , 0x0040'0000_u32, "RDPID"             , "Read Processor ID"                             , "rdpid"            },
				{ intel                  , 0x0200'0000_u32, "CLDEMOTE"          , "Cache line demote"                             , "cldemote"         },
				{ intel                  , 0x0800'0000_u32, "MOVDIRI"           , "32-bit direct stores"                          , "movdiri"          },
				{ intel                  , 0x1000'0000_u32, "MOVDIRI64B"        , "64-bit direct stores"                          , "movdir64b"        },
				{ intel                  , 0x4000'0000_u32, "SGX_LC"            , "SGX Launch Configuration"                      , "sgx_lc"           },
			}},
			{ edx, {
				{ intel                  , 0x0000'0004_u32, "AVX512_4NNIW"      , "AVX512 4-register Neural Network Instructions"                                , "avx512_4vnniw"     },
				{ intel                  , 0x0000'0008_u32, "AVX512_4FMAPS"     , "AVX512 4-register Multiply Accumulate Single Precision"                       , "avx512_4fmaps"     },
				{ intel                  , 0x0000'0010_u32, "REPMOVS"           , "Fast short REP MOV"                                                           , ""                  },
				{ intel                  , 0x0004'0000_u32, "PCONFIG"           , "Platform configuration for MKTME"                                             , ""                  },
				{ intel                  , 0x0400'0000_u32, "IBRS"              , "Indirect Branch Restricted Speculation and Indirect Branch Predictor Barrier" , "pconfig"           },
				{ intel                  , 0x0800'0000_u32, "STIBP"             , "Single Thread Indirect Branch Predictors"                                     , ""                  },
				{ intel | amd            , 0x1000'0000_u32, "L1TF"              , "L1 Data Cache flush"                                                          , "flush_l1d"         },
				{ intel                  , 0x2000'0000_u32, "ARCH_CAPS"         , "ARCH_CAPABILITIES MSR"                                                        , "arch_capabilities" },
				{ intel                  , 0x4000'0000_u32, "CORE_CAPS"         , "CORE_CAPABILITIES MSR"                                                        , ""                  },
				{ intel                  , 0x8000'0000_u32, "SSBD"              , "Speculative Store Bypass Disable"                                             , ""                  },
			}}
		}}
	}},
	{ leaf_type::extended_state, {
		{ subleaf_type::extended_state_main, {
			{ eax, {
				{ intel | amd            , 0x0000'0001_u32, "x87"               , "Legacy x87 floating point"     },
				{ intel | amd            , 0x0000'0002_u32, "SSE"               , "128-bit SSE XMM"               },
				{ intel | amd            , 0x0000'0004_u32, "AVX"               , "256-bit AVX YMM"               },
				{ intel                  , 0x0000'0008_u32, "MPX_bounds"        , "MPX bounds registers"          },
				{ intel                  , 0x0000'0010_u32, "MPX_CSR"           , "MPX CSR"                       },
				{ intel                  , 0x0000'0020_u32, "AVX512_mask"       , "AVX-512 OpMask"                },
				{ intel                  , 0x0000'0040_u32, "AVX512_hi256"      , "AVX-512 ZMM0-15 upper bits"    },
				{ intel                  , 0x0000'0080_u32, "AVX512_hi16"       , "AVX-512 ZMM16-31"              },
				{ intel                  , 0x0000'0100_u32, "PT"                , "Processor Trace"               },
				{ intel                  , 0x0000'0200_u32, "PKRU"              , "Protection Keys User Register" },
				{ intel                  , 0x0000'1000_u32, "HDC"               , "Hardware Duty Cycling"         },
			}}
		}},
		{ subleaf_type::extended_state_sub, {
			{ eax, {
				{ intel | amd            , 0x0000'0001_u32, "XSAVEOPT"          , "XSAVEOPT available"          , "xsaveopt" },
				{ intel | amd            , 0x0000'0002_u32, "XSAVEC"            , "XSAVEC and compacted XRSTOR" , "xsavec"   },
				{ intel | amd            , 0x0000'0004_u32, "XG1"               , "XGETBV"                      , "xgetbv1"  },
				{ intel | amd            , 0x0000'0008_u32, "XSSS"              , "XSAVES/XRSTORS"              , "xsaves"   },
			}}
		}}
	}},
	{ leaf_type::rdt_monitoring, {
		{ subleaf_type::main, {
			{ edx, {
				{ intel                  , 0x0000'0002_u32, "cqm_llc"      , "Supports L3 Cache RDT monitoring", "cqm_llc"       },
			}}
		}},
		{ subleaf_type::rdt_monitoring_l3, {
			{ edx, {
				{ intel                  , 0x0000'0001_u32, "cqm_occup_llc", "LLC occupancy monitoring"        , "cqm_occup_llc" },
				{ intel                  , 0x0000'0002_u32, "cqm_mbm_total", "LLC Total MBM monitoring"        , "cqm_mbm_total" },
				{ intel                  , 0x0000'0004_u32, "cqm_mbm_local", "LLC Local MBM monitoring"        , "cqm_mbm_local" },
			}}
		}}
	}},
	{ leaf_type::rdt_allocation, {
		{ subleaf_type::main, {
			{ ebx, {
				{ intel                  , 0x0000'0002_u32, "L3" , "L3 Cache Allocation"         , "cat_l3" },
				{ intel                  , 0x0000'0004_u32, "L2" , "L2 Cache Allocation"         , "cat_l2" },
				{ intel                  , 0x0000'0008_u32, "MEM", "Memory Bandwidth Allocation" , "mba"    }
			}}
		}},
		{ subleaf_type::rdt_cat_l3, {
			{ ecx, {
				{ intel                  , 0x0000'0004_u32, "CDP", "L3 Code and data prioritization support", "cdt_l3" },
			}}
		}},
		{ subleaf_type::rdt_cat_l2, {
			{ ecx, {
				{ intel                  , 0x0000'0004_u32, "CDP", "L2 Code and data prioritization support", "cdt_l2" },
			}}
		}},
		{ subleaf_type::rdt_mba, {
			{ ecx, {
				{ intel                  , 0x0000'0004_u32, "Linear", "Response of the delay values is linear", "" },
			}}
		}},
	}},
	{ leaf_type::sgx_info, {
		{ subleaf_type::sgx_capabilities, {
			{ eax, {
				{ intel                  , 0x0000'0001_u32, "SGX1"              , "SGX1 functions available"                      },
				{ intel                  , 0x0000'0002_u32, "SGX2"              , "SGX2 functions available"                      },
				{ intel                  , 0x0000'0020_u32, "ENCLV"             , "EINCVIRTCHILD, EDECVIRTCHILD, and ESETCONTEXT" },
				{ intel                  , 0x0000'0040_u32, "ENCLS"             , "ETRACKC, ERDINFO, ELDBC, and ELDUC"            },
			}}
		}}
	}},
	{ leaf_type::processor_trace, {
		{ subleaf_type::main, {
			{ ebx, {
				{ intel                  , 0x0000'0001_u32, "CR3Filter"         , "CR3Filter can be set to 1"                },
				{ intel                  , 0x0000'0002_u32, "PSB"               , "Configurable PSB and Cycle-Accurate Mode" },
				{ intel                  , 0x0000'0004_u32, "IPFilter"          , "IP Filtering, TraceStop filtering"        },
				{ intel                  , 0x0000'0008_u32, "MTC"               , "MTC timing packet supported"              },
				{ intel                  , 0x0000'0010_u32, "PTWRITE"           , "PTWRITE supported"                        },
				{ intel                  , 0x0000'0020_u32, "PET"               , "Power Event Trace"                        },
			}},
			{ ecx, {
				{ intel                  , 0x0000'0001_u32, "TOPA"              , "ToPA output supported"                      },
				{ intel                  , 0x0000'0002_u32, "TOPAEntries"       , "ToPA tables can hold any number of entries" },
				{ intel                  , 0x0000'0004_u32, "SRO"               , "Single-Range Output"                        },
				{ intel                  , 0x0000'0008_u32, "TT"                , "Trace Transport output"                     },
				{ intel                  , 0x8000'0000_u32, "IPLIP"             , "IP payloads have LIP values"                },
			}}
		}}
	}},
	{ leaf_type::hyper_v_features, {
		{ subleaf_type::main, {
			{ eax, {
				{ hyper_v                , 0x0000'0001_u32, "accessVpRunTimeReg"              , "VP_RUNTIME MSR"              },
				{ hyper_v                , 0x0000'0002_u32, "accessParititonReferenceCounter" , "TIME_REF_COUNT MSR"          },
				{ hyper_v                , 0x0000'0004_u32, "acccessSynicRegs"                , "SynIC MSRs"                  },
				{ hyper_v                , 0x0000'0008_u32, "accessSyntheticTimerRegs"        , "SynIC timer MSRs"            },
				{ hyper_v                , 0x0000'0010_u32, "accessIntrCtrlRegs"              , "APIC MSRs"                   },
				{ hyper_v                , 0x0000'0020_u32, "accessHypercallMsrs"             , "Hypercall MSRs"              },
				{ hyper_v                , 0x0000'0040_u32, "accessVpIndex"                   , "Virtual Processor Index MSR" },
				{ hyper_v                , 0x0000'0080_u32, "accessResetReg"                  , "System reset MSR"            },
				{ hyper_v                , 0x0000'0100_u32, "accessStatsReg"                  , "Statistics pages MSR"        },
				{ hyper_v                , 0x0000'0200_u32, "accessPartitionReferenceTsc"     , "Reference TSC MSR"           },
				{ hyper_v                , 0x0000'0400_u32, "accessGuestIdleReg"              , "Guest idle state MSR"        },
				{ hyper_v                , 0x0000'0800_u32, "accessFrequencyRegs"             , "TSC and APIC frequency MSRs" },
				{ hyper_v                , 0x0000'1000_u32, "accessDebugRegs"                 , "Guest debugging MSRs"        },
				{ hyper_v                , 0x0000'2000_u32, "accessReenlightenmentControls"   , "Reenlightenment controls"    },
			}},
			{ ebx, {
				{ hyper_v                , 0x0000'0001_u32, "createPartitions"          , "HvCreatePartition hypercall"                 },
				{ hyper_v                , 0x0000'0002_u32, "accessPartitionId"         , "HvGetPartitionId hypercall"                  },
				{ hyper_v                , 0x0000'0004_u32, "accessMemoryPool"          , "HvDepositMemory/etc. hypercalls"             },
				{ hyper_v                , 0x0000'0008_u32, "adjustMessageBuffers"      , "Adjust Message Buffers"                      },
				{ hyper_v                , 0x0000'0010_u32, "postMessages"              , "HvPostMessage hypercall"                     },
				{ hyper_v                , 0x0000'0020_u32, "signalEvents"              , "HvSignalEvent hypercall"                     },
				{ hyper_v                , 0x0000'0040_u32, "createPort"                , "HvCreatePort hypercall"                      },
				{ hyper_v                , 0x0000'0080_u32, "connectPort"               , "HvConnectPort hypercall"                     },
				{ hyper_v                , 0x0000'0100_u32, "accessStats"               , "HvMapStatsPage/etc. hypercalls"              },
				{ hyper_v                , 0x0000'0800_u32, "debugging"                 , "HvPostDebugData/etc. hypercalls"             },
				{ hyper_v                , 0x0000'1000_u32, "cpuManagement"             , "HvGetLogicalProcessoRunTime/etc. hypercalls" },
				{ hyper_v                , 0x0000'2000_u32, "configureProfiler"         , "Configure Profiler hypercall"                },
				{ hyper_v                , 0x0000'4000_u32, "enableExpandedStackWalking", "Enable expanded stack walking"               },
				{ hyper_v                , 0x0001'0000_u32, "accessVSM"                 , "The partition can use Virtual Secure Mode"   },
				{ hyper_v                , 0x0002'0000_u32, "accessVpRegisters"         , "Access VP Registers"                         },
				{ hyper_v                , 0x0010'0000_u32, "enabledExtendedHypercalls" , "Extended hypercall interface"                },
				{ hyper_v                , 0x0020'0000_u32, "startVirtualProcessor"     , "HvStartVirtualProcessor hypercall"           },
			}},
			{ edx, {
				{ hyper_v                , 0x0000'0001_u32, "MWAIT"               , "MWAIT available"                         },
				{ hyper_v                , 0x0000'0002_u32, "GDBG"                , "Guest debugging"                         },
				{ hyper_v                , 0x0000'0004_u32, "PM"                  , "Performance Monitoring"                  },
				{ hyper_v                , 0x0000'0008_u32, "DynamicPartitioning" , "Physical CPU dynamic partitioning"       },
				{ hyper_v                , 0x0000'0010_u32, "XMMHypercallInput"   , "Hypercall parameters in XMM registers"   },
				{ hyper_v                , 0x0000'0020_u32, "GuestIdle"           , "Virtual guest idle state available"      },
				{ hyper_v                , 0x0000'0040_u32, "HypervisorSleep"     , "Hypervisor sleep state available"        },
				{ hyper_v                , 0x0000'0080_u32, "QueryNUMA"           , "NUMA distances queryable"                },
				{ hyper_v                , 0x0000'0100_u32, "TimerFrequencies"    , "Determining timer frequencies supported" },
				{ hyper_v                , 0x0000'0200_u32, "MCEInject"           , "Support injecting MCEs"                  },
				{ hyper_v                , 0x0000'0400_u32, "CrashMSRs"           , "Guest crash MSRs available"              },
				{ hyper_v                , 0x0000'0800_u32, "DebugMSRs"           , "Debug MSRs available"                    },
				{ hyper_v                , 0x0000'1000_u32, "NPIEP"               , "NPIEP supported"                         },
				{ hyper_v                , 0x0000'2000_u32, "DisableHypervisor"   , "Disable hypervisor supported"            },
				{ hyper_v                , 0x0000'4000_u32, "ExtendedGvaRanges"   , "Extended GVA ranges for flush"           },
				{ hyper_v                , 0x0000'8000_u32, "XMMHypercallOutput"  , "Hypercall results in XMM registers"      },
				{ hyper_v                , 0x0002'0000_u32, "SintPollingMode"     , "Soft interrupt polling mode available"   },
				{ hyper_v                , 0x0004'0000_u32, "HypercallMsrLock"    , "Hypercall MSR lock available"            },
				{ hyper_v                , 0x0008'0000_u32, "SyntheticTimers"     , "Use direct synthetic timers"             },
			}}
		}}
	}},
	{ leaf_type::hyper_v_enlightenment_recs, {
		{ subleaf_type::main, {
			{ eax, {
				{ hyper_v                , 0x0000'0001_u32, "MOV_CR3"             , "Use hypercall instead of MOV CR3"                    },
				{ hyper_v                , 0x0000'0002_u32, "INVLPG"              , "Use hypercall instead of INVLPG or MOV CR3"          },
				{ hyper_v                , 0x0000'0004_u32, "IPI"                 , "Use hypercall instead of inter-processor interrupts" },
				{ hyper_v                , 0x0000'0008_u32, "APIC_MSR"            , "Use MSRs for APIC registers"                         },
				{ hyper_v                , 0x0000'0010_u32, "RESET_MSR"           , "Use MSR for system reset"                            },
				{ hyper_v                , 0x0000'0020_u32, "RelaxTimings"        , "Use relaxed timings/disable watchdogs"               },
				{ hyper_v                , 0x0000'0040_u32, "DMARemapping"        , "Use DMA remapping"                                   },
				{ hyper_v                , 0x0000'0080_u32, "InterruptRemapping"  , "Use interrupt remapping"                             },
				{ hyper_v                , 0x0000'0100_u32, "x2_APIC_MSR"         , "Use x2 APIC MSRs"                                    },
				{ hyper_v                , 0x0000'0200_u32, "DeprecateAutoEOI"    , "Deprecate AutoEOI"                                   },
				{ hyper_v                , 0x0000'0400_u32, "SyntheticClusterIpi" , "Use SyntheticClusterIpi hypercall"                   },
				{ hyper_v                , 0x0000'0800_u32, "ExProcessorMasks"    , "Use ExProcessorMasks interface"                      },
				{ hyper_v                , 0x0000'1000_u32, "Nested"              , "Running in a nested partition"                       },
				{ hyper_v                , 0x0000'2000_u32, "INT_MBEC"            , "Use INT for MBEC system calls"                       },
				{ hyper_v                , 0x0000'4000_u32, "VMCS"                , "Use VMCS for nested hypervisor"                      },
				{ hyper_v                , 0x0000'8000_u32, "syncedTimeline"      , "Use QueryPerformanceCounter bias from root partition"},
				{ hyper_v                , 0x0002'0000_u32, "directLocalFlush"    , "Use CR4.PGE to flush entire TLB"                     },
				{ hyper_v                , 0x0004'0000_u32, "noNonArchCoreSharing", "Virtual machines never share physical cores"         },
			}}
		}}
	}},
	{ leaf_type::hyper_v_implementation_hardware, {
		{ subleaf_type::main, {
			{ eax, {
				{ hyper_v                , 0x0000'0001_u32, "APIC_OVERLAY"    , "APIC overlay assist"                },
				{ hyper_v                , 0x0000'0002_u32, "MSR_BITMAPS"     , "MSR bitmaps"                        },
				{ hyper_v                , 0x0000'0004_u32, "PERF_COUNTERS"   , "Architectural performance counters" },
				{ hyper_v                , 0x0000'0008_u32, "SLAT"            , "Second Level Address Translation"   },
				{ hyper_v                , 0x0000'0010_u32, "DMA_REMAP"       , "DMA remapping"                      },
				{ hyper_v                , 0x0000'0020_u32, "INTERRUPT_REMAP" , "Interrupt remapping"                },
				{ hyper_v                , 0x0000'0040_u32, "MEMORY_SCRUBBER" , "Memory patrol scrubber"             },
				{ hyper_v                , 0x0000'0080_u32, "DMA_PROTECTION"  , "DMA protection"                     },
				{ hyper_v                , 0x0000'0100_u32, "HPET"            , "HPET"                               },
				{ hyper_v                , 0x0000'0200_u32, "SyntheticTimers" , "Volatile synthetic timers"          },
			}}
		}}
	}},
	{ leaf_type::hyper_v_root_cpu_management, {
		{ subleaf_type::main, {
			{ eax, {
				{ hyper_v                , 0x0000'0001_u32, "StartLogicalProcessor" , "Start logical processor"       },
				{ hyper_v                , 0x0000'0002_u32, "CreateRootVirtProc"    , "Create root virtual processor" },
				{ hyper_v                , 0x8000'0000_u32, "ReservedIdentityBit"   , "Reserved identity bit"         },
			}},
			{ ebx, {
				{ hyper_v                , 0x0000'0001_u32, "ProcessorPowerMgmt"     , "Processor power management" },
				{ hyper_v                , 0x0000'0002_u32, "MwaitIdleStates"        , "MWAIT idle states"          },
				{ hyper_v                , 0x0000'0004_u32, "LogicalProcessorIdling" , "Logical processor idling"   },
			}}
		}}
	}},
	{ leaf_type::hyper_v_shared_virtual_memory, {
		{ subleaf_type::main, {
			{ eax, {
				{ hyper_v                , 0x0000'0001_u32, "SvmSupported"          , "Shared virtual memory supported" },
			}},
		}}
	}},
	{ leaf_type::hyper_v_nested_hypervisor, {
		{ subleaf_type::main, {
			{ eax, {
				{ hyper_v                , 0x0000'0004_u32, "AccessSynIcRegs"       , "SynIC MSRs"               },
				{ hyper_v                , 0x0000'0010_u32, "AccessIntrCtrlRegs"    , "Interrupt Control MSRs"   },
				{ hyper_v                , 0x0000'0020_u32, "AccessHypercallMsrs"   , "Hypercall MSRs"           },
				{ hyper_v                , 0x0000'0040_u32, "AccessVpIndex"         , "VP Index MSRs"            },
				{ hyper_v                , 0x0000'1000_u32, "AccessReenlightenment" , "Reenlightenment controls" },
			}},
			{ edx, {
				{ hyper_v                , 0x0000'0010_u32, "XMMHypercallInput"    , "Hypercall parameters in XMM registers" },
				{ hyper_v                , 0x0000'8000_u32, "XMMHypercallOutput"   , "Hypercall results in XMM registers"    },
				{ hyper_v                , 0x0002'0000_u32, "SintPollingAvailable" , "Soft interrupt polling mode available" },
			}},
		}}
	}},
	{ leaf_type::hyper_v_nested_features, {
		{ subleaf_type::main, {
			{ eax, {
				{ hyper_v                , 0x0002'0000_u32, "DirectVirtualFlush"   , "Direct virtual flush hypercalls"           },
				{ hyper_v                , 0x0004'0010_u32, "FlushGuestPhysical"   , "HvFlushGuestPhysicalAddressXxx hypercalls" },
				{ hyper_v                , 0x0008'0020_u32, "EnlightenedMSRBitmap" , "Enlightened MSR bitmap"                    },
			}},
		}}
	}},
	{ leaf_type::xen_time, {
		{ subleaf_type::xen_time_main, {
			{ eax, {
				{ xen_hvm                , 0x0000'0001_u32, "VTSC"      , "Virtual RDTSC"       },
				{ xen_hvm                , 0x0000'0002_u32, "SafeRDTSC" , "Host has safe RDTSC" },
				{ xen_hvm                , 0x0000'0004_u32, "RDTSCP"    , "Host has RDTSCP"     },
			}}
		}}
	}},
	{ leaf_type::xen_time_offset, {
		{ subleaf_type::xen_time_main, {
			{ eax, {
				{ xen_hvm                , 0x0000'0001_u32, "VTSC"      , "Virtual RDTSC"       },
				{ xen_hvm                , 0x0000'0002_u32, "SafeRDTSC" , "Host has safe RDTSC" },
				{ xen_hvm                , 0x0000'0004_u32, "RDTSCP"    , "Host has RDTSCP"     },
			}}
		}}
	}},
	{ leaf_type::xen_hvm_features, {
		{ subleaf_type::xen_time_main, {
			{ eax, {
				{ xen_hvm                , 0x0000'0001_u32, "VAPIC"   , "Virtualized APIC registers"              },
				{ xen_hvm                , 0x0000'0002_u32, "Vx2APIC" , "Virtualized x2APIC registers"            },
				{ xen_hvm                , 0x0000'0004_u32, "IOMMU"   , "IOMMU mappings from other domains exist" },
				{ xen_hvm                , 0x0000'0008_u32, "VCPU"    , "VCPU ID is present"                      },
			}}
		}}
	}},
	{ leaf_type::xen_hvm_features_offset, {
		{ subleaf_type::xen_time_main, {
			{ eax, {
				{ xen_hvm                , 0x0000'0001_u32, "VAPIC"   , "Virtualized APIC registers"              },
				{ xen_hvm                , 0x0000'0002_u32, "Vx2APIC" , "Virtualized x2APIC registers"            },
				{ xen_hvm                , 0x0000'0004_u32, "IOMMU"   , "IOMMU mappings from other domains exist" },
				{ xen_hvm                , 0x0000'0008_u32, "VCPU"    , "VCPU ID is present"                      },
			}}
		}}
	}},
	{ leaf_type::kvm_features, { // https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/tree/Documentation/virtual/kvm/cpuid.txt
		{ subleaf_type::main, {
			{ eax, {
				{ kvm                    , 0x0000'0001_u32, "CLOCKSOURCE"        , "kvmclock MSRs available"               },
				{ kvm                    , 0x0000'0002_u32, "NOP_IO_DELAY"       , "Don't delay PIO operations"            },
				{ kvm                    , 0x0000'0004_u32, "MMU_OP"             , "Deprecated"                            },
				{ kvm                    , 0x0000'0008_u32, "CLOCKSOURCE_2"      , "More kvmclock MSRs available"          },
				{ kvm                    , 0x0000'0010_u32, "ASYNC_PF"           , "Async PF supported"                    },
				{ kvm                    , 0x0000'0020_u32, "STEAL_TIME"         , "Steal time MSR available"              },
				{ kvm                    , 0x0000'0040_u32, "EOI"                , "End of interrupt MSR available"        },
				{ kvm                    , 0x0000'0080_u32, "UNHALT"             , "Paravirtualized spinlocks supported"   },
				{ kvm                    , 0x0000'0200_u32, "TLB_FLUSH"          , "Paravirtualized TLB flush supported"   },
				{ kvm                    , 0x0000'0400_u32, "ASYNC_PF_VMEXIT"    , "Async PF vmexit supported"             },
				{ kvm                    , 0x0000'0800_u32, "SEND_IPI"           , "Paravirtualized IPI supported"         },
				{ kvm                    , 0x0100'0000_u32, "CLOCKSOURCE_STABLE" , "Guest-side clock should not be warped" },
			}},
			{ edx, {
				{ kvm                    , 0x0000'0001_u32, "REALTIME"           , "vCPUs never pre-empted forever"        },
			}}
		}}
	}},
	{ leaf_type::extended_signature_and_features, {
		{ subleaf_type::main, {
			{ ecx, {
				{ intel | amd            , 0x0000'0001_u32, "LAHF-SAHF"               , "LAHF/SAHF supported in 64-bit mode"   , "lahf_lm"       },
				{         amd            , 0x0000'0002_u32, "CmpLegacy"               , "Core multi-processing legacy mode"    , "cmp_legacy"    },
				{         amd            , 0x0000'0004_u32, "SVM"                     , "Secure Virtual Machine Mode"          , "svm"           },
				{         amd            , 0x0000'0008_u32, "ExtApicSpace"            , "Extended APIC register space"         , "extapic"       },
				{         amd            , 0x0000'0010_u32, "AltMovCr8"               , "LOCK MOV CR0 is MOV CR8"              , "cr8_legacy"    },
				{ intel                  , 0x0000'0020_u32, "LZCNT"                   , "LZCNT instruction"                    , "abm"           },
				{         amd            , 0x0000'0020_u32, "ABM"                     , "Advanced Bit Manipulation"            , "abm"           },
				{         amd            , 0x0000'0040_u32, "SSE4A"                   , "SSE4A instructions"                   , "sse4a"         },
				{         amd            , 0x0000'0080_u32, "MisAlignSse"             , "Misaligned SSE Mode"                  , "misalignsse"   },
				{ intel                  , 0x0000'0100_u32, "PREFETCHW"               , "PREFETCHW instruction"                , "3dnowprefetch" },
				{         amd            , 0x0000'0100_u32, "ThreeDNowPrefetch"       , "PREFETCH and PREFETCHW instructions"  , "3dnowprefetch" },
				{         amd            , 0x0000'0200_u32, "OSVW"                    , "OS Visible Work-around"               , "osvw"          },
				{         amd            , 0x0000'0400_u32, "IBS"                     , "Instruction Based Sampling"           , "ibs"           },
				{         amd            , 0x0000'0800_u32, "XOP"                     , "Extended Operation support"           , "xop"           },
				{         amd            , 0x0000'1000_u32, "SKINIT"                  , "SKINIT/STGI instructions"             , "skinit"        },
				{         amd            , 0x0000'2000_u32, "WDT"                     , "Watchdog Timer Support"               , "wdt"           },
				{         amd            , 0x0000'8000_u32, "LWP"                     , "Lightweight Profiling Support"        , "lwp"           },
				{         amd            , 0x0001'0000_u32, "FMA4"                    , "4-operand FMA instruction"            , "fma4"          },
				{         amd            , 0x0002'0000_u32, "TCE"                     , "Translation Cache Extension"          , "tce"           },
				{         amd            , 0x0008'0000_u32, "NODEID_MSR"              , "NodeId MSR"                           , "nodeid_msr"    },
				{         amd            , 0x0020'0000_u32, "TBM"                     , "Trailing Bit Manipulation"            , "tbm"           },
				{         amd            , 0x0040'0000_u32, "TopologyExtensions"      , "Topology Extensions"                  , "topoext"       },
				{         amd            , 0x0080'0000_u32, "PerfCtrExtCore"          , "Core Performance Counter Extensions"  , "perfctr_core"  },
				{         amd            , 0x0100'0000_u32, "PerfCtrExtNB"            , "NB Performance Counter Extensions"    , "perfctr_nb"    },
				{         amd            , 0x0400'0000_u32, "DataBreakpointExtension" , "Data Breakpoint support"              , "bpext"         },
				{         amd            , 0x0800'0000_u32, "PerfTsc"                 , "Performance Time Stamp Counter"       , "ptsc"          },
				{         amd            , 0x1000'0000_u32, "PerfCtrExtL3"            , "L3 performance counter extensions"    , "perfctr_llc"   },
				{         amd            , 0x2000'0000_u32, "MwaitExtended"           , "MWAITX and MONITORX"                  , "mwaitx"        },
			}},
			{ edx, {
				{         amd            , 0x0000'0001_u32, "FPU"           , "x87 FPU on chip"                      , ""         },
				{         amd            , 0x0000'0002_u32, "VME"           , "Virtual Mode Enhancements"            , ""         },
				{         amd            , 0x0000'0004_u32, "DE"            , "Debugging extensions"                 , ""         },
				{         amd            , 0x0000'0008_u32, "PSE"           , "Page-size extensions"                 , ""         },
				{         amd            , 0x0000'0010_u32, "TSC"           , "Time Stamp Counter"                   , ""         },
				{         amd            , 0x0000'0020_u32, "MSR"           , "RDMSR and WRMSR Instructions"         , ""         },
				{         amd            , 0x0000'0040_u32, "PAE"           , "Physical Address Extension"           , ""         },
				{         amd            , 0x0000'0080_u32, "MCE"           , "Machine Check Exception"              , ""         },
				{         amd            , 0x0000'0100_u32, "CX8"           , "CMPXCHG8B Instruction"                , ""         },
				{         amd            , 0x0000'0200_u32, "APIC"          , "APIC On-Chip"                         , ""         },
				{ intel | amd            , 0x0000'0800_u32, "SysCallSysRet" , "SYSCALL and SYSRET Instructions"      , "syscall"  },
				{         amd            , 0x0000'1000_u32, "MTRR"          , "Memory Type Range Registers"          , ""         },
				{         amd            , 0x0000'2000_u32, "PGE"           , "Page Global Bit"                      , ""         },
				{         amd            , 0x0000'4000_u32, "MCA"           , "Machine Check Architecture"           , ""         },
				{         amd            , 0x0000'8000_u32, "CMOV"          , "Conditional Move Instructions"        , ""         },
				{         amd            , 0x0001'0000_u32, "PAT"           , "Page Attribute Table"                 , ""         },
				{         amd            , 0x0002'0000_u32, "PSE36"         , "36-bit Page Size Extension"           , ""         },
				{         amd            , 0x0008'0000_u32, "MP"            , "MP Capable"                           , ""         },
				{ intel                  , 0x0010'0000_u32, "XD"            , "Execute Disable Bit available"        , "nx"       },
				{         amd            , 0x0010'0000_u32, "NX"            , "No-Execute page protection"           , "nx"       },
				{         amd            , 0x0040'0000_u32, "MmxExt"        , "AMD extensions to MMX instructions"   , "mmxext"   },
				{         amd            , 0x0080'0000_u32, "MMX"           , "MMX instructions"                     , ""         },
				{         amd            , 0x0100'0000_u32, "FXSR"          , "FXSAVE and FXRSTOR instructions"      , ""         },
				{         amd            , 0x0200'0000_u32, "FFXSR"         , "Fast FXSAVE and FXRSTOR"              , "fxsr_opt" },
				{ intel | amd            , 0x0400'0000_u32, "Page1GB"       , "1 GB large page support"              , "pdpe1gb"  },
				{ intel | amd            , 0x0800'0000_u32, "RDTSCP"        , "RDTSCP instruction"                   , "rdtscp"   },
				{ intel                  , 0x2000'0000_u32, "EM64T"         , "Intel 64 Architecture (Long Mode)"    , "lm"       },
				{         amd            , 0x2000'0000_u32, "LM"            , "Long mode"                            , "lm"       },
				{         amd            , 0x4000'0000_u32, "ThreeDNowExt"  , "AMD extensions to 3DNow! instructions", "3dnowext" },
				{         amd            , 0x8000'0000_u32, "ThreeDNow"     , "3DNow! instructions"                  , "3dnow"    },
			}}
		}}
	}},
	{ leaf_type::ras_advanced_power_management, {
		{ subleaf_type::main, {
			{ ebx, {
				{         amd            , 0x0000'0001_u32, "McaOverflowRecov" , "MCA overflow recovery support"                         , "overflow_recov" },
				{         amd            , 0x0000'0002_u32, "SUCCOR"           , "Software Uncorrectable Error Containment and Recovery" , "succor"         },
				{         amd            , 0x0000'0004_u32, "HWA"              , "Hardware Assert supported"                             , ""               },
				{         amd            , 0x0000'0008_u32, "ScalableMca"      , "Scalable MCA supported"                                , "smca"           },

			}},
			{ edx, {
				{         amd            , 0x0000'0001_u32, "TS"                    , "Temperature Sensor"                      , "ts"            },
				{         amd            , 0x0000'0002_u32, "FID"                   , "Frequency ID control"                    , "fid"           },
				{         amd            , 0x0000'0004_u32, "VID"                   , "Voltage ID control"                      , "vid"           },
				{         amd            , 0x0000'0008_u32, "TTP"                   , "THERMTRIP"                               , "ttp"           },
				{         amd            , 0x0000'0010_u32, "TM"                    , "Hardware thermal control (HTC)"          , "tm"            },
				{         amd            , 0x0000'0020_u32, "STC"                   , "Software thermal control (HTC)"          , "stc"           },
				{         amd            , 0x0000'0040_u32, "100MHzSteps"           , "100 MHz multiplier control"              , "100mhzsteps"   },
				{         amd            , 0x0000'0080_u32, "HwPstate"              , "Hardware P-state control"                , "hw_pstate"     },
				{ intel | amd            , 0x0000'0100_u32, "TscInvariant"          , "TSC invariant"                           , ""              },
				{         amd            , 0x0000'0200_u32, "CPB"                   , "Core performance boost"                  , "cpb"           },
				{         amd            , 0x0000'0400_u32, "EffFreqRO"             , "Read-only effective frequency interface" , "eff_freq_ro"   },
				{         amd            , 0x0000'0800_u32, "ProcFeedbackInterface" , "Processor feedback interface"            , "proc_feedback" },
				{         amd            , 0x0000'1000_u32, "ApmPwrReporting"       , "APM power reporting"                     , "acc_power"     },
				{         amd            , 0x0000'2000_u32, "ConnectedStandby"      , "Connected Standby"                       , " "             },
				{         amd            , 0x0000'4000_u32, "RAPL"                  , "Running average power limit"             , " "             },
			}}
		}}
	}},
	{ leaf_type::address_limits, {
		{ subleaf_type::main, {
			{ ebx, {
				{         amd            , 0x0000'0001_u32, "CLZERO"       , "CLZERO instruction"                      , "clzero"     },
				{         amd            , 0x0000'0002_u32, "IRPERF"       , "Instructions retired count support"      , "irperf"     },
				{         amd            , 0x0000'0004_u32, "ASRFPEP"      , "XSAVE (etc.) saves error pointer"        , "xsaveerptr" },
				{         amd            , 0x0000'0040_u32, "MBA"          , "Memory Bandwidth Allocation"             , "mba"        },
				{ intel                  , 0x0000'0200_u32, "WBNOINVD"     , "WBNOINVD is available"                   , "wbnoinvd"   },
				{         amd            , 0x0000'1000_u32, "IBPB"         , "Indirect Branch Prediction Barrier"      , ""           },
				{         amd            , 0x0000'4000_u32, "IBRS"         , "Indirect Branch Restricted Speculation"  , ""           },
				{         amd            , 0x0000'8000_u32, "STIBP"        , "Single Thread Indirect Branch Predictor" , ""           },
				{         amd            , 0x0100'0000_u32, "SSBD"         , "Speculative Store Bypass Disable"        , ""           },
				{         amd            , 0x0200'0000_u32, "VIRT_SPEC_CTL", "VIRT_SPEC_CTL"                           , "virt_ssbd"  },
			}}
		}}
	}},
	{ leaf_type::secure_virtual_machine, {
		{ subleaf_type::main, {
			{ edx, {
				{         amd            , 0x0000'0001_u32, "NP"                   , "Nested paging"                    , "npt"             },
				{         amd            , 0x0000'0002_u32, "LBRV"                 , "LBR virtualization"               , "lbrv"            },
				{         amd            , 0x0000'0004_u32, "SVML"                 , "SVM lock"                         , "svm_lock"        },
				{         amd            , 0x0000'0008_u32, "NRIPS"                , "NRIP Save"                        , "nrip_save"       },
				{         amd            , 0x0000'0010_u32, "TscRateMsr"           , "MSR-based TSC rate control"       , "tsc_scale"       },
				{         amd            , 0x0000'0020_u32, "VmcbClean"            , "VMCB clean bits"                  , "vmcb_clean"      },
				{         amd            , 0x0000'0040_u32, "FlushByAsid"          , "Flush by ASID"                    , "flushbyasid"     },
				{         amd            , 0x0000'0080_u32, "DecodeAssists"        , "Decode assists"                   , "decodeassists"   },
				{         amd            , 0x0000'0400_u32, "PauseFilter"          , "PAUSE intercept filter"           , "pausefilter"     },
				{         amd            , 0x0000'1000_u32, "PauseFilterThreshold" , "PAUSE filter threshold"           , "pfthreshold"     },
				{         amd            , 0x0000'2000_u32, "AVIC"                 , "AMD virtual interrupt controller" , "avic"            },
				{         amd            , 0x0000'8000_u32, "VLS"                  , "Virtualized VMSAVE/VMLOAD"        , "v_vmsave_vmload" },
				{         amd            , 0x0001'0000_u32, "vGIF"                 , "Virtualized GIF"                  , "vgif"            },

			}}
		}}
	}},
	{ leaf_type::performance_optimization, {
		{ subleaf_type::main, {
			{ eax, {
				{         amd            , 0x0000'0001_u32, "FP128" , "Full-width 128-bit SSE instructions" },
				{         amd            , 0x0000'0002_u32, "MOVU"  , "Prefer MOVU to MOVL/MOVH"            },
				{         amd            , 0x0000'0004_u32, "FP256" , "Full-width AVX256 instructions"      },
			}}
		}}
	}},
	{ leaf_type::instruction_based_sampling, {
		{ subleaf_type::main, {
			{ eax, {
				{         amd            , 0x0000'0001_u32, "IBSFFV"          , "IBS feature flags valid"                    },
				{         amd            , 0x0000'0002_u32, "FetchSam"        , "IBS fetch sampling supported"               },
				{         amd            , 0x0000'0004_u32, "OpSam"           , "IBS execution sampling supported"           },
				{         amd            , 0x0000'0008_u32, "RdWrOpCnt"       , "Read/write of op counter supported"         },
				{         amd            , 0x0000'0010_u32, "OpCnt"           , "Op counting mode supported"                 },
				{         amd            , 0x0000'0020_u32, "BrnTarget"       , "Branch target address reporting supported"  },
				{         amd            , 0x0000'0040_u32, "OpCntExt"        , "Op counters extended by 7 bits"             },
				{         amd            , 0x0000'0080_u32, "RipInvalidChk"   , "Invalid RIP indication supported"           },
				{         amd            , 0x0000'0100_u32, "OpBrnFuse"       , "Fused branch micro-op indication supported" },
				{         amd            , 0x0000'0200_u32, "IbsFetchCtlExtd" , "IBS fetch control extended MSR supported"   },
				{         amd            , 0x0000'0400_u32, "IbsOpData4"      , "IBS op data 4 MSR supported"                },
			}}
		}}
	}},
	{ leaf_type::lightweight_profiling, {
		{ subleaf_type::main, {
			{ eax, {
				{         amd            , 0x0000'0001_u32, "LwpAvail" , "Lightweight profiling supported"  },
				{         amd            , 0x0000'0002_u32, "LwpVAL"   , "LWPVAL instruction supported"     },
				{         amd            , 0x0000'0004_u32, "LwpIRE"   , "Instructions retired event"       },
				{         amd            , 0x0000'0008_u32, "LwpBRE"   , "Branch retired event"             },
				{         amd            , 0x0000'0010_u32, "LwpDME"   , "DC miss event"                    },
				{         amd            , 0x0000'0020_u32, "LwpCNH"   , "Core clocks not halted"           },
				{         amd            , 0x0000'0040_u32, "LwpRNH"   , "Core reference clocks not halted" },
				{         amd            , 0x2000'0000_u32, "LwpCont"  , "Samping in continuous mode"       },
				{         amd            , 0x4000'0000_u32, "LwpPTSC"  , "Performance TSC in event record"  },
				{         amd            , 0x8000'0000_u32, "LwpInt"   , "Interrupt on threshold overflow"  },
			}},
			{ ecx, {
				{         amd            , 0x0000'0020_u32, "LwpDataAddress"      , "Data cache miss address valid"        },
				{         amd            , 0x1000'0000_u32, "LwpBranchPrediction" , "Branch prediction filtering supported"},
				{         amd            , 0x2000'0000_u32, "LwpIpFiltering"      , "IP filtering supported"               },
				{         amd            , 0x4000'0000_u32, "LwpCacheLevels"      , "Cache level filtering supported"      },
				{         amd            , 0x8000'0000_u32, "LwpCacheLatency"     , "Cache latency filtering supported"    },
			}}
		}}
	}},
	{ leaf_type::encrypted_memory, {
		{ subleaf_type::main, {
			{ eax, {
				{         amd            , 0x0000'0001_u32, "SME"          , "Secure Memory Encryption supported"        , ""    },
				{         amd            , 0x0000'0002_u32, "SEV"          , "Secure Encrypted Virtualization supported" , "sev" },
				{         amd            , 0x0000'0004_u32, "PageFlushMsr" , "Page Flush MSR available"                  , ""    },
				{         amd            , 0x0000'0008_u32, "SEV-ES"       , "SEV Encrypted State available"             , ""    },
			}}
		}}
	}}
};

template<typename FoundFn, typename NotFoundFn>
void for_feature_range(const cpu_t& cpu, leaf_type leaf, subleaf_type subleaf, register_type reg, FoundFn&& found, NotFoundFn&& not_found) {
	const bool has_value = cpu.leaves.find(leaf) != cpu.leaves.end()
	                    && cpu.leaves.at(leaf).find(subleaf) != cpu.leaves.at(leaf).end();
	const std::uint32_t value = has_value ? cpu.leaves.at(leaf).at(subleaf).at(reg) : 0_u32;
	const auto range = all_features.equal_range(leaf);
	for(auto it = range.first; it != range.second; ++it) {
		if(it->second.find(subleaf) == it->second.end()) {
			continue;
		}
		auto sl = it->second.at(subleaf);
		if(sl.find(reg) == sl.end()) {
			continue;
		}

		const std::vector<feature_t>& feats = sl.at(reg);
		for(const feature_t& f : feats) {
			if(__popcnt(f.mask) == 1_u32) {
				if(cpu.vendor & f.vendor) {
					if(0 != (value & f.mask)) {
						found(f, value);
					} else {
						not_found(f, value);
					}
				}
			}
		}
	}
}

void print_features(fmt::memory_buffer& out, const cpu_t& cpu, leaf_type leaf, subleaf_type subleaf, register_type reg) {
	for_feature_range(cpu, leaf, subleaf, reg,
		[&] (const feature_t& feature, std::uint32_t) {
			format_to(out, "{: >32s} \x1b[32;1m[+]\x1b[0m {:s}\n", feature.mnemonic, feature.description);
		},
		[&] (const feature_t& feature, std::uint32_t) {
			format_to(out, "{: >32s} \x1b[31;1m[-]\x1b[0m {:s}\n", feature.mnemonic, feature.description);
		}
	);
}

bool has_feature(const cpu_t& cpu, leaf_type leaf, subleaf_type subleaf, register_type reg, std::uint32_t bit) {
	if(cpu.leaves.find(leaf) != cpu.leaves.end()) {
		if(cpu.leaves.at(leaf).find(subleaf) != cpu.leaves.at(leaf).end()) {
			if(cpu.leaves.at(leaf).at(subleaf)[reg] & (1_u32 << bit)) {
				return true;
			}
		}
	}
	return false;
}

std::vector<std::string> get_linux_features(const cpu_t& cpu) {
	std::vector<std::string> features;

	const auto add_name = [&] (const feature_t& feature, std::uint32_t) {
		if(!feature.linux_name.empty()) {
			features.push_back(feature.linux_name);
		}
	};

	const auto skip_name = [&] (const feature_t&, std::uint32_t) {
	};

	const auto add_features_bulk = [&] (leaf_type leaf, subleaf_type subleaf, register_type reg) {
		for_feature_range(cpu, leaf, subleaf, reg, add_name, skip_name);
	};

	const auto add_feature_single_masked = [&] (leaf_type leaf, subleaf_type subleaf, register_type reg, std::uint32_t mask) {
		for_feature_range(cpu, leaf, subleaf, reg,
			[&] (const feature_t& feature, std::uint32_t value) {
				if(feature.mask == mask) {
					add_name(feature, value);
				}
			}
		, skip_name);
	};

	const auto add_feature_single_indexed = [&] (leaf_type leaf, subleaf_type subleaf, register_type reg, std::uint32_t index) {
		return add_feature_single_masked(leaf, subleaf, reg, 1_u32 << index);
	};

	const auto add_feature_single_condition = [&] (bool condition, const char* linux_name) {
		if(condition) {
			features.push_back(linux_name);
		}
	};

	add_features_bulk(leaf_type::version_info,                    subleaf_type::main, edx);
	add_features_bulk(leaf_type::extended_signature_and_features, subleaf_type::main, edx);

	//synthetic features:
	bool constant_tsc = false;
	bool nonstop_tsc = false;
	bool nonstop_tsc_s3 = false;
	bool tsc_reliable = false;
	bool tsc_known_freq = false;
	bool always_running_timer = false;

	const auto get_tsc_info = [&] () {
		if(has_feature(cpu, leaf_type::ras_advanced_power_management, subleaf_type::main, edx, 8_u32)) {
			constant_tsc = true;
			nonstop_tsc = true;
		}
		if(cpu.vendor & intel) {
			if((cpu.model.family == 0xf && cpu.model.model >= 0x3)
			|| (cpu.model.family == 0x6 && cpu.model.model >= 0xe)) {
				constant_tsc = true;
			}
			if(cpu.model.family == 0x6 && (cpu.model.model == 0x27 || cpu.model.model == 0x35 || cpu.model.model == 0x4a)) {
				nonstop_tsc_s3 = true;
			}
			if(cpu.model.family == 0x6 && cpu.model.model == 0x5c) {
				tsc_reliable = true;
			}
			if(cpu.leaves.find(leaf_type::time_stamp_counter) != cpu.leaves.end()) {
				if(cpu.leaves.at(leaf_type::time_stamp_counter).at(subleaf_type::main)[ecx] != 0_u32) {
					tsc_known_freq = true;
				}
				if(cpu.model.model == 0x4e
				|| cpu.model.model == 0x5e
				|| cpu.model.model == 0x8e
				|| cpu.model.model == 0x9e
				|| cpu.model.model == 0x5f
				|| cpu.model.model == 0x5c) {
					tsc_known_freq = true;
				}
				if(!has_feature(cpu, leaf_type::version_info, subleaf_type::main, ecx, 31_u32)
				&& nonstop_tsc
				&& has_feature(cpu, leaf_type::extended_features, subleaf_type::main, ebx, 1_u32)) {
					always_running_timer = true;
				}
			}
		}
	};

	const auto get_rep_good = [&] () {
		switch(cpu.vendor & vendor_type::any_silicon) {
		case intel:
			if(cpu.model.family == 0x6) {
				return true;
			}
			break;
		case amd:
			if(cpu.model.family >= 0x10) {
				return true;
			}
			const std::uint32_t a = cpu.leaves.at(leaf_type::version_info).at(subleaf_type::main)[eax];
			if(a >= 0x0f58 || (0x0f48 <= a && a < 0x0f50)) {
				return true;
			}
			break;
		}
		return false;
	};

	const auto get_amd_dcm = [&] () {
		if(cpu.vendor & vendor_type::amd) {
			if(cpu.leaves.find(leaf_type::extended_apic) != cpu.leaves.end()) {
				const register_set_t& regs = cpu.leaves.at(leaf_type::extended_apic).at(subleaf_type::main);

				const struct
				{
					std::uint32_t node_id             : 8;
					std::uint32_t nodes_per_processor : 3;
					std::uint32_t reserved_1          : 21;
				} c = bit_cast<decltype(c)>(regs[ecx]);

				return c.nodes_per_processor > 1;
			}
		}
		return false;
	};

	const bool arch_perfmon = cpu.leaves.find(leaf_type::performance_monitoring) != cpu.leaves.end()
	                    && (((cpu.leaves.at(leaf_type::performance_monitoring).at(subleaf_type::main)[eax] >> 0_u32) & 0xff_u32) != 0_u32)
	                    && (((cpu.leaves.at(leaf_type::performance_monitoring).at(subleaf_type::main)[eax] >> 8_u32) & 0xff_u32) != 0_u32);
	const bool rep_good = get_rep_good();
	const bool acc_power = has_feature(cpu, leaf_type::ras_advanced_power_management, subleaf_type::main, edx, 12_u32);
	const bool xtopology = cpu.leaves.find(leaf_type::extended_topology) != cpu.leaves.end();
	const bool extd_apicid = (cpu.vendor & vendor_type::any_silicon) == vendor_type::amd;
	const bool amd_dcm = get_amd_dcm();

	get_tsc_info();

	add_feature_single_condition(constant_tsc, "constant_tsc");
	// up // never set
	add_feature_single_condition(always_running_timer, "art");
	add_feature_single_condition(arch_perfmon, "arch_perfmon");
	// pebs // needs MSR
	// bts // needs MSR
	add_feature_single_condition(rep_good, "rep_good");
	add_feature_single_condition(acc_power, "acc_power");
	add_feature_single_condition(true, "nopl"); // always set
	add_feature_single_condition(xtopology, "xtopology");
	add_feature_single_condition(tsc_reliable, "tsc_reliable");
	add_feature_single_condition(nonstop_tsc, "nonstop_tsc");
	add_feature_single_condition(true, "cpuid"); // always set
	add_feature_single_condition(extd_apicid, "extd_apicid");
	add_feature_single_condition(amd_dcm, "amd_dcm");
	add_feature_single_indexed(leaf_type::thermal_and_power, subleaf_type::main, ecx, 0_u32);
	add_feature_single_condition(nonstop_tsc_s3, "nonstop_tsc_s3");
	add_feature_single_condition(tsc_known_freq, "tsc_known_freq");
	
	add_features_bulk(leaf_type::version_info,                    subleaf_type::main, ecx);
	add_features_bulk(leaf_type::extended_signature_and_features, subleaf_type::main, ecx);

	//scattered features:

	const auto get_ring3mwait = [&] () {
		if(cpu.vendor & vendor_type::intel) {
			if(cpu.model.family == 0x6
			&& (cpu.model.model == 0x57 || cpu.model.model == 0x85)) {
				return true;
			}
		}
		return false;
	};

	bool ssbd = false;
	bool ibrs = false;
	bool ibpb = false;
	bool stibp = false;

	const auto get_speculative_controls = [&] () {
		if(has_feature(cpu, leaf_type::extended_features, subleaf_type::main, edx, 26_u32)) {
			ibrs = true;
			ibpb = true;
		}
		if(has_feature(cpu, leaf_type::extended_features, subleaf_type::main, edx, 27_u32)) {
			stibp = true;
		}
		if(has_feature(cpu, leaf_type::extended_features, subleaf_type::main, edx, 31_u32)
		|| has_feature(cpu, leaf_type::address_limits, subleaf_type::main, ebx, 25_u32)) {
			ssbd = true;
		}
		if(has_feature(cpu, leaf_type::address_limits, subleaf_type::main, ebx, 14_u32)) {
			ibrs = true;
		}
		if(has_feature(cpu, leaf_type::address_limits, subleaf_type::main, ebx, 12_u32)) {
			ibpb = true;
		}
		if(has_feature(cpu, leaf_type::address_limits, subleaf_type::main, ebx, 15_u32)) {
			stibp = true;
		}
		if(has_feature(cpu, leaf_type::address_limits, subleaf_type::main, ebx, 24_u32)) {
			ssbd = true;
		}
	};

	get_speculative_controls();
	const bool ring3mwait = get_ring3mwait();

	add_feature_single_condition(ring3mwait, "ring3mwait");
	// cpuid_fault // needs MSR
	add_feature_single_indexed(leaf_type::ras_advanced_power_management, subleaf_type::main      , edx,  9_u32);
	add_feature_single_indexed(leaf_type::thermal_and_power            , subleaf_type::main      , ecx,  3_u32);
	add_feature_single_indexed(leaf_type::rdt_allocation               , subleaf_type::main      , ebx,  1_u32);
	add_feature_single_indexed(leaf_type::rdt_allocation               , subleaf_type::main      , ebx,  2_u32);
	add_feature_single_indexed(leaf_type::rdt_allocation               , subleaf_type::rdt_cat_l3, ecx,  2_u32);
	add_feature_single_indexed(leaf_type::extended_features            , subleaf_type::main      , ebx, 10_u32); // if invpcid is set, invpcid_single is also set
	add_feature_single_indexed(leaf_type::ras_advanced_power_management, subleaf_type::main      , edx,  7_u32);
	add_feature_single_indexed(leaf_type::ras_advanced_power_management, subleaf_type::main      , edx, 11_u32);
	add_feature_single_indexed(leaf_type::encrypted_memory             , subleaf_type::main      , eax,  0_u32);
	// pti // TODO API call to see if shadow pagetables are enabled
	// intel_ppin // needs MSR
	add_feature_single_indexed(leaf_type::rdt_allocation               , subleaf_type::rdt_cat_l2, ecx,  2_u32);
	add_feature_single_condition(ssbd, "ssbd");
	add_feature_single_indexed(leaf_type::rdt_allocation               , subleaf_type::main      , ebx,  3_u32);
	add_feature_single_indexed(leaf_type::address_limits               , subleaf_type::main      , ebx,  6_u32);
	add_feature_single_indexed(leaf_type::encrypted_memory             , subleaf_type::main      , eax,  1_u32);
	add_feature_single_condition(ibrs, "ibrs");
	add_feature_single_condition(ibpb, "ibpb");
	add_feature_single_condition(stibp, "stibp");
	// ibrs_enhanced // needs MSR

	//synthetic virtualization features:
	const bool vmmcall = (cpu.vendor & vendor_type::any_silicon) == vendor_type::amd;

	// tpr_shadow // needs MSR
	// vnmi // needs MSR
	// flexpriority // needs MSR
	// ept // needs MSR
	// vpid // needs MSR
	add_feature_single_condition(vmmcall, "vmmcall");
	// ept_ad // needs MSR

	add_features_bulk(leaf_type::extended_features            , subleaf_type::main              , ebx);
	add_features_bulk(leaf_type::extended_state               , subleaf_type::extended_state_sub, eax);
	add_features_bulk(leaf_type::rdt_monitoring               , subleaf_type::main              , edx);
	add_features_bulk(leaf_type::address_limits               , subleaf_type::main              , ebx);

	// amd ssbd suppresses display of virt_ssbd for some reason
	if(has_feature(cpu, leaf_type::address_limits, subleaf_type::main, ebx, 24_u32)
	&& has_feature(cpu, leaf_type::address_limits, subleaf_type::main, ebx, 25_u32)) {
		auto it = std::find(std::begin(features), std::end(features), "virt_ssbd");
		if(it != features.end()) {
			features.erase(it);
		}
	}

	add_features_bulk(leaf_type::thermal_and_power            , subleaf_type::main, eax);
	add_features_bulk(leaf_type::secure_virtual_machine       , subleaf_type::main, edx);
	add_features_bulk(leaf_type::extended_features            , subleaf_type::main, ecx);
	add_features_bulk(leaf_type::ras_advanced_power_management, subleaf_type::main, ebx);
	add_features_bulk(leaf_type::extended_features            , subleaf_type::main, edx);

	return features;
}

std::vector<std::string> get_linux_bugs(const cpu_t& cpu) {
	std::vector<std::string> bugs;

	const auto add_bug_condition = [&] (bool condition, const char* linux_name) {
		if(condition) {
			bugs.push_back(linux_name);
		}
	};

	const auto get_clflush_monitor = [&] () {
		if((cpu.vendor & vendor_type::any_silicon) == vendor_type::intel) {
			if(cpu.model.family == 0x6 && has_feature(cpu, leaf_type::version_info, subleaf_type::main, edx, 19_u32)) {
				if(cpu.model.model == 29 || cpu.model.model == 46 || cpu.model.model == 47) {
					return true;
				}
			}
		}
		return false;
	};

	const auto get_sysret_ss_attrs = [&] () {
		if((cpu.vendor & vendor_type::any_silicon) == vendor_type::amd) {
			return true;
		}
		return false;
	};

	const auto get_null_seg = [&] () {
		if((cpu.vendor & vendor_type::any_silicon) == vendor_type::amd) {
			return true;
		}
		return false;
	};

	const auto get_monitor = [&] () {
		if((cpu.vendor & vendor_type::any_silicon) == vendor_type::intel) {
			if(cpu.model.family == 0x6 && has_feature(cpu, leaf_type::version_info, subleaf_type::main, ecx, 3_u32)) {
				if(cpu.model.model == 0x5c) {
					return true;
				}
			}
		}
		return false;
	};

	const bool clflush_monitor = get_clflush_monitor();
	const bool sysret_ss_attrs = get_sysret_ss_attrs();
	const bool null_seg = get_null_seg();
	const bool monitor = get_monitor();

	bool cpu_meltdown = false;
	bool spectre_v1 = false;
	bool spectre_v2 = false;
	bool spec_store_bypass = false;
	bool l1tf = false;

	const auto get_speculative_bugs = [&] () {
		// in-order 64-bit Atoms have none of these bugs
		if((cpu.vendor & vendor_type::any_silicon) == vendor_type::intel) {
			if(cpu.model.family == 0x6) {
				if(cpu.model.model == 0x36
				|| cpu.model.model == 0x27
				|| cpu.model.model == 0x35
				|| cpu.model.model == 0x1c
				|| cpu.model.model == 0x26) {
					return;
				}
			}
		}
		spectre_v1 = true;
		spectre_v2 = true;

		spec_store_bypass = true;

		if((cpu.vendor & vendor_type::any_silicon) == vendor_type::intel) {
			if(cpu.model.family == 0x6) {
				if(cpu.model.model == 0x37
				|| cpu.model.model == 0x4c
				|| cpu.model.model == 0x4d
				|| cpu.model.model == 0x4a
				|| cpu.model.model == 0x0e
				|| cpu.model.model == 0x57
				|| cpu.model.model == 0x85) {
					spec_store_bypass = false;
				}
			}
		}
		if(has_feature(cpu, leaf_type::address_limits, subleaf_type::main, ebx, 26_u32)) {
			spec_store_bypass = false;
		}
		if((cpu.vendor & vendor_type::any_silicon) == vendor_type::amd) {
			return;
		}
		cpu_meltdown = true;
		if((cpu.vendor & vendor_type::any_silicon) == vendor_type::intel) {
			if(cpu.model.family == 0x6) {
				if(cpu.model.model == 0x37
				|| cpu.model.model == 0x4d
				|| cpu.model.model == 0x4c
				|| cpu.model.model == 0x4a
				|| cpu.model.model == 0x5a
				|| cpu.model.model == 0x5c
				|| cpu.model.model == 0x5f
				|| cpu.model.model == 0x7a
				|| cpu.model.model == 0x57
				|| cpu.model.model == 0x85) {
					return;
				}
			}
		}
		l1tf = true;
	};

	get_speculative_bugs();

	// f00f // never
	// fdiv // never
	// coma // never
	// amd_tlb_mmatch // needs MSR
	// amd_apic_c1e // needs MSR
	// 11ap // never
	// fxsave_leak // never
	add_bug_condition(clflush_monitor  , "clflush_monitor");
	add_bug_condition(sysret_ss_attrs  , "sysret_ss_attrs");
	add_bug_condition(null_seg         , "null_seg");
	// swapgs_fence // never
	add_bug_condition(monitor          , "monitor");
	// amd_e400 // needs MSR
	add_bug_condition(cpu_meltdown     , "cpu_meltdown");
	add_bug_condition(spectre_v1       , "spectre_v1");
	add_bug_condition(spectre_v2       , "spectre_v2");
	add_bug_condition(spec_store_bypass, "spec_store_bypass");
	add_bug_condition(l1tf             , "l1tf");

	return bugs;
}

std::vector<std::string> get_linux_power_management(const cpu_t& cpu) {
	std::vector<std::string> pm;

	const auto add_name = [&] (const feature_t& feature, std::uint32_t) {
		if(feature.linux_name == " ") {
			unsigned long idx = 0;
			bit_scan_forward(&idx, feature.mask);
			pm.push_back(fmt::format("[{:d}]", idx));
		} else if(!feature.linux_name.empty()) {
			pm.push_back(feature.linux_name);
		}
	};

	const auto skip_name = [&] (const feature_t&, std::uint32_t) {
	};

	const auto add_power_management_bulk = [&] () {
		for_feature_range(cpu, leaf_type::ras_advanced_power_management, subleaf_type::main, edx, add_name, skip_name);
	};

	add_power_management_bulk();

	return pm;
}

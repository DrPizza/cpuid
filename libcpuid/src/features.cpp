#include "stdafx.h"

#include "features.hpp"

#include <cstddef>
#include <map>
#include <vector>

#if !defined(_MSC_VER)
#include <x86intrin.h>
#define __popcnt _popcnt32
#endif

const feature_map_t all_features = {
	{ leaf_type::version_info, {
		{ subleaf_type::main, {
			{ ecx, {
				{ intel | amd            , 0x0000'0001_u32, "SSE3"              , "SSE3 Extensions"                               },
				{ intel | amd            , 0x0000'0002_u32, "PCLMULQDQ"         , "Carryless Multiplication"                      },
				{ intel                  , 0x0000'0004_u32, "DTES64"            , "64-bit DS Area"                                },
				{ intel | amd            , 0x0000'0008_u32, "MONITOR"           , "MONITOR/MWAIT"                                 },
				{ intel                  , 0x0000'0010_u32, "DS-CPL"            , "CPL Qualified Debug Store"                     },
				{ intel                  , 0x0000'0020_u32, "VMX"               , "Virtual Machine Extensions"                    },
				{ intel                  , 0x0000'0040_u32, "SMX"               , "Safer Mode Extensions"                         },
				{ intel                  , 0x0000'0080_u32, "EIST"              , "Enhanced Intel SpeedStep Technology"           },
				{ intel                  , 0x0000'0100_u32, "TM2"               , "Thermal Monitor 2"                             },
				{ intel | amd            , 0x0000'0200_u32, "SSSE3"             , "SSSE3 Extensions"                              },
				{ intel                  , 0x0000'0400_u32, "CNXT-ID"           , "L1 Context ID"                                 },
				{ intel                  , 0x0000'0800_u32, "SDBG"              , "IA32_DEBUG_INTERFACE for silicon debug"        },
				{ intel | amd            , 0x0000'1000_u32, "FMA"               , "Fused Multiply Add"                            },
				{ intel | amd            , 0x0000'2000_u32, "CMPXCHG16B"        , "CMPXCHG16B instruction"                        },
				{ intel                  , 0x0000'4000_u32, "xTPR"              , "xTPR update control"                           },
				{ intel                  , 0x0000'8000_u32, "PDCM"              , "Perfmon and Debug Capability"                  },
				{ intel | amd            , 0x0002'0000_u32, "PCID"              , "Process-context identifiers"                   },
				{ intel                  , 0x0004'0000_u32, "DCA"               , "Direct Cache Access"                           },
				{ intel | amd            , 0x0008'0000_u32, "SSE4.1"            , "SSE4.1 Extensions"                             },
				{ intel | amd            , 0x0010'0000_u32, "SSE4.2"            , "SSE4.2 Extensions"                             },
				{ intel | amd            , 0x0020'0000_u32, "x2APIC"            , "x2APIC"                                        },
				{ intel | amd            , 0x0040'0000_u32, "MOVBE"             , "MOVBE instruction"                             },
				{ intel | amd            , 0x0080'0000_u32, "POPCNT"            , "POPCNT instruction"                            },
				{ intel | amd            , 0x0100'0000_u32, "TSC-Deadline"      , "One-shot APIC timers using a TSC deadline"     },
				{ intel | amd            , 0x0200'0000_u32, "AESNI"             , "AESNI instruction extensions"                  },
				{ intel | amd            , 0x0400'0000_u32, "XSAVE"             , "XSAVE/XRSTOR feature"                          },
				{ intel | amd            , 0x0800'0000_u32, "OSXSAVE"           , "OS has set CR4.OSXSAVE"                        },
				{ intel | amd            , 0x1000'0000_u32, "AVX"               , "AVX instructions"                              },
				{ intel | amd            , 0x2000'0000_u32, "F16C"              , "16-bit floating-point conversion instructions" },
				{ intel | amd            , 0x4000'0000_u32, "RDRAND"            , "RDRAND instruction"                            },
				{ any                    , 0x8000'0000_u32, "RAZ"               , "Hypervisor guest"                              },
			}},
			{ edx, {
				{ intel | amd | transmeta, 0x0000'0001_u32, "FPU"               , "x87 FPU on chip"                                             },
				{ intel | amd | transmeta, 0x0000'0002_u32, "VME"               , "Virtual 8086 Mode Enhancements"                              },
				{ intel | amd | transmeta, 0x0000'0004_u32, "DE"                , "Debugging Extensions"                                        },
				{ intel | amd | transmeta, 0x0000'0008_u32, "PSE"               , "Page Size Extension"                                         },
				{ intel | amd | transmeta, 0x0000'0010_u32, "TSC"               , "Time Stamp Counter"                                          },
				{ intel | amd | transmeta, 0x0000'0020_u32, "MSR"               , "RDMSR and WRMSR Instructions"                                },
				{ intel | amd            , 0x0000'0040_u32, "PAE"               , "Physical Address Extension"                                  },
				{ intel | amd            , 0x0000'0080_u32, "MCE"               , "Machine Check Exception"                                     },
				{ intel | amd | transmeta, 0x0000'0100_u32, "CX8"               , "CMPXCHG8B Instruction"                                       },
				{ intel | amd            , 0x0000'0200_u32, "APIC"              , "APIC On-Chip"                                                },
				{ intel | amd | transmeta, 0x0000'0800_u32, "SEP"               , "SYSENTER and SYSEXIT Instructions"                           },
				{ intel | amd            , 0x0000'1000_u32, "MTRR"              , "Memory Type Range Registers"                                 },
				{ intel | amd            , 0x0000'2000_u32, "PGE"               , "Page Global Bit"                                             },
				{ intel | amd            , 0x0000'4000_u32, "MCA"               , "Machine Check Architecture"                                  },
				{ intel | amd | transmeta, 0x0000'8000_u32, "CMOV"              , "Conditional Move Instructions"                               },
				{ intel | amd            , 0x0001'0000_u32, "PAT"               , "Page Attribute Table"                                        },
				{ intel | amd            , 0x0002'0000_u32, "PSE-36"            , "36-bit Page Size Extension"                                  },
				{ intel       | transmeta, 0x0004'0000_u32, "PSN"               , "Processor Serial Number"                                     },
				{ intel | amd            , 0x0008'0000_u32, "CLFSH"             , "CLFLUSH Instruction"                                         },
				{ intel                  , 0x0020'0000_u32, "DS"                , "Debug Store"                                                 },
				{ intel                  , 0x0040'0000_u32, "ACPI"              , "Thermal Monitoring and Software Controlled Clock Facilities" },
				{ intel | amd | transmeta, 0x0080'0000_u32, "MMX"               , "Intel MMX Technology"                                        },
				{ intel | amd            , 0x0100'0000_u32, "FXSR"              , "FXSAVE and FXRSTOR Instructions"                             },
				{ intel | amd            , 0x0200'0000_u32, "SSE"               , "SSE Extensions"                                              },
				{ intel | amd            , 0x0400'0000_u32, "SSE2"              , "SSE2 Extensions"                                             },
				{ intel                  , 0x0800'0000_u32, "SS"                , "Self Snoop"                                                  },
				{ intel | amd            , 0x1000'0000_u32, "HTT"               , "Hyperthreading/Max APIC IDs reserved field is Valid"         },
				{ intel                  , 0x2000'0000_u32, "TM"                , "Thermal Monitor"                                             },
				{ intel                  , 0x4000'0000_u32, "IA64"              , "IA64 emulating x86"                                          },
				{ intel                  , 0x8000'0000_u32, "PBE"               , "Pending Break Enable"                                        },
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
				{ intel                  , 0x0000'0001_u32, "DTS"               , "Digital temperature sensor"                                                     },
				{ intel                  , 0x0000'0002_u32, "TBT"               , "Intel Turbo Boost Technology"                                                   },
				{ intel | amd            , 0x0000'0004_u32, "ARAT"              , "APIC-Timer-always-running"                                                      },
				{ intel                  , 0x0000'0010_u32, "PLN"               , "Power limit notification controls"                                              },
				{ intel                  , 0x0000'0020_u32, "ECMD"              , "Clock modulation duty cycle extension"                                          },
				{ intel                  , 0x0000'0040_u32, "PTM"               , "Package thermal management"                                                     },
				{ intel                  , 0x0000'0080_u32, "HWP"               , "Hardware Managed Performance States: HWP_CAPABILITIES, HWP_REQUEST, HWP_STATUS" },
				{ intel                  , 0x0000'0100_u32, "HWP_N"             , "HWP_Notification                   : HWP_INTERRUPT"                             },
				{ intel                  , 0x0000'0200_u32, "HWP_AW"            , "HWP_Activity_Window                : HWP_REQUEST[41:32]"                        },
				{ intel                  , 0x0000'0400_u32, "HWP_EPP"           , "HWP_Energy_Performance_Preference  : HWP_REQUEST[31:24]"                        },
				{ intel                  , 0x0000'0800_u32, "HWP_PLR"           , "HWP_Package_Level_Request          : HWP_REQUEST_PKG"                           },
				{ intel                  , 0x0000'2000_u32, "HDC"               , "HDC_CTL, HDC_CTL1, THREAD_STALL"                                                },
				{ intel                  , 0x0000'4000_u32, "TBT3"              , "Intel Turbo Boost Max Technology 3.0"                                           },
				{ intel                  , 0x0000'8000_u32, "HWP_Cap"           , "HWP Capabilities. Highest Performance change is supported."                     },
				{ intel                  , 0x0001'0000_u32, "HWP_PECI"          , "HWP PECI Override."                                                             },
				{ intel                  , 0x0002'0000_u32, "Flexible_HWP"      , "Flexible HWP"                                                                   },
				{ intel                  , 0x0004'0000_u32, "Fast_Access"       , "Fast access mode for HWP_REQUEST MSR"                                           },
				{ intel                  , 0x0010'0000_u32, "Ignore_Idle"       , "Ignore Idle Logical Processor HWP request"                                      },
			}},
			{ ecx, {
				{ intel | amd            , 0x0000'0001_u32, "HCF"               , "Hardware Coordination Feedback Capability: MPERF, APERF"     },
				{ intel                  , 0x0000'0008_u32, "PERF_BIAS"         , "Performance-energy bias preference       : ENERGY_PERF_BIAS" },
			}}
		}}
	}},
	{ leaf_type::extended_features, {
		{ subleaf_type::extended_features_main, {
			{ ebx, {
				{ intel | amd            , 0x0000'0001_u32, "FSGSBASE"          , "FSGSBASE instructions"                               },
				{ intel                  , 0x0000'0002_u32, "TSC_ADJUST"        , "TSC_ADJUST MSR"                                      },
				{ intel                  , 0x0000'0004_u32, "SGX"               , "Softward Guard Extensions"                           },
				{ intel | amd            , 0x0000'0008_u32, "BMI1"              , "Bit Manipulation Instructions 1"                     },
				{ intel                  , 0x0000'0010_u32, "HLE"               , "Hardware Lock Elision"                               },
				{ intel | amd            , 0x0000'0020_u32, "AVX2"              , "Advanced Vector Extensions 2.0 instructions"         },
				{ intel                  , 0x0000'0040_u32, "FDP_EXCPTN_ONLY"   , "x87 FPU Data Pointer updated only on x87 exceptions" },
				{ intel | amd            , 0x0000'0080_u32, "SMEP"              , "Supervisor-Mode Execution Prevention"                },
				{ intel | amd            , 0x0000'0100_u32, "BMI2"              , "Bit Manipulation Instructions 2"                     },
				{ intel                  , 0x0000'0200_u32, "EREPMOVSB"         , "Enhanced REP MOVSB/REP STOSB"                        },
				{ intel                  , 0x0000'0400_u32, "INVPCID"           , "INVPCID instruction"                                 },
				{ intel                  , 0x0000'0800_u32, "RTM"               , "Restricted Transactional Memory"                     },
				{ intel                  , 0x0000'1000_u32, "RDT-M"             , "Resource Director Technology Monitoring"             },
				{ intel                  , 0x0000'2000_u32, "FPU-CSDS"          , "x87 FPU CS and DS deprecated"                        },
				{ intel                  , 0x0000'4000_u32, "MPX"               , "Memory Protection Extensions"                        },
				{ intel                  , 0x0000'8000_u32, "RDT-A"             , "Resource Director Technology Allocation"             },
				{ intel                  , 0x0001'0000_u32, "AVX512F"           , "AVX512 Foundation"                                   },
				{ intel                  , 0x0002'0000_u32, "AVX512DQ"          , "AVX512 Double/Quadword Instructions"                 },
				{ intel | amd            , 0x0004'0000_u32, "RDSEED"            , "RDSEED instruction"                                  },
				{ intel | amd            , 0x0008'0000_u32, "ADX"               , "Multi-Precision Add-Carry Instructions"              },
				{ intel | amd            , 0x0010'0000_u32, "SMAP"              , "Supervisor-Mode Access Prevention"                   },
				{ intel                  , 0x0020'0000_u32, "AVX512_IFMA"       , "AVX512 Integer FMA"                                  },
				{         amd            , 0x0040'0000_u32, "PCOMMIT"           , "Persistent Commit"                                   },
				{ intel | amd            , 0x0080'0000_u32, "CLFLUSHOPT"        , "CLFLUSHOPT instruction"                              },
				{ intel                  , 0x0100'0000_u32, "CLWB"              , "CLWB instruction"                                    },
				{ intel                  , 0x0200'0000_u32, "IPT"               , "Intel Processor Trace"                               },
				{ intel                  , 0x0400'0000_u32, "AVX512PF"          , "AVX512 Prefetch"                                     },
				{ intel                  , 0x0800'0000_u32, "AVX512ER"          , "AVX512 Exponential and Reciprocal Instructions"      },
				{ intel                  , 0x1000'0000_u32, "AVX512CD"          , "AVX512 Conflict Detection Instructions"              },
				{ intel | amd            , 0x2000'0000_u32, "SHA"               , "SHA Extensions"                                      },
				{ intel                  , 0x4000'0000_u32, "AVX512BW"          , "AVX512 Byte/Word Instructions"                       },
				{ intel                  , 0x8000'0000_u32, "AVX512VL"          , "AVX512 Vector Length Instructions"                   },
			}},
			{ ecx, {
				{ intel                  , 0x0000'0001_u32, "PREFETCHW1"        , "PREFETCHW1 instruction"                        },
				{ intel                  , 0x0000'0002_u32, "AVX512_VBMI"       , "AVX512 Vector Bit Manipulation Instructions"   },
				{ intel                  , 0x0000'0004_u32, "UMIP"              , "User Mode Instruction Prevention"              },
				{ intel                  , 0x0000'0008_u32, "PKU"               , "Protection Keys for User-mode pages"           },
				{ intel                  , 0x0000'0010_u32, "OSPKE"             , "OS has set CR4.PKE"                            },
				{ intel                  , 0x0000'0020_u32, "WAITPKG"           , "Wait and pause enhancements"                   },
				{ intel                  , 0x0000'0040_u32, "AVX512_VBMI2"      , "AVX512 Vector Bit Manipulation Instructions 2" },
				{ intel                  , 0x0000'0100_u32, "GFNI"              , "Galois Field NI"                               },
				{ intel                  , 0x0000'0200_u32, "VAES"              , "VEX-AES-NI"                                    },
				{ intel                  , 0x0000'0400_u32, "VPCLMULQDQ"        , "VEX-PCLMUL"                                    },
				{ intel                  , 0x0000'0800_u32, "AVX512_VNNI"       , "AVX512 Vector Neural Net Instructions"         },
				{ intel                  , 0x0000'1000_u32, "AVX512_BITALG"     , "AVX512 Bitwise Algorithms"                     },
				{ intel                  , 0x0000'2000_u32, "TME"               , "Intel Total Memory Encryption"                 },
				{ intel                  , 0x0000'4000_u32, "AVX512_VPOPCNTDQ"  , "AVX512 VPOPCNTDQ"                              },
				{ intel                  , 0x0001'0000_u32, "LA57"              , "5-level page tables/57-bit virtual addressing" },
				{ intel                  , 0x003e'0000_u32, "MAWAU"             , "MPX Address Width Adjust for User addresses"   },
				{ intel                  , 0x0040'0000_u32, "RDPID"             , "Read Processor ID"                             },
				{ intel                  , 0x0200'0000_u32, "CLDEMOTE"          , "Cache line demote"                             },
				{ intel                  , 0x0800'0000_u32, "MOVDIRI"           , "32-bit direct stores"                          },
				{ intel                  , 0x1000'0000_u32, "MOVDIRI64B"        , "64-bit direct stores"                          },
				{ intel                  , 0x4000'0000_u32, "SGX_LC"            , "SGX Launch Configuration"                      },
			}},
			{ edx, {
				{ intel                  , 0x0000'0004_u32, "AVX512_4NNIW"      , "AVX512 4-register Neural Network Instructions"                                },
				{ intel                  , 0x0000'0008_u32, "AVX512_4FMAPS"     , "AVX512 4-register Multiply Accumulate Single Precision"                       },
				{ intel                  , 0x0000'0010_u32, "REPMOVS"           , "Fast short REP MOV"                                                           },
				{ intel                  , 0x0004'0000_u32, "PCONFIG"           , "Platform configuration for MKTME"                                             },
				{ intel                  , 0x0400'0000_u32, "IBRS"              , "Indirect Branch Restricted Speculation and Indirect Branch Predictor Barrier" },
				{ intel                  , 0x0800'0000_u32, "STIBP"             , "Single Thread Indirect Branch Predictors"                                     },
				{ intel | amd            , 0x1000'0000_u32, "L1TF"              , "L1 Data Cache flush"                                                          },
				{ intel                  , 0x2000'0000_u32, "ARCH_CAPS"         , "ARCH_CAPABILITIES MSR"                                                        },
				{ intel                  , 0x8000'0000_u32, "SSBD"              , "Speculative Store Bypass Disable"                                             },
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
			}},
		}},
		{ subleaf_type::extended_state_sub, {
			{ eax, {
				{ intel | amd            , 0x0000'0001_u32, "XSAVEOPT"          , "XSAVEOPT available"          },
				{ intel | amd            , 0x0000'0002_u32, "XSAVEC"            , "XSAVEC and compacted XRSTOR" },
				{ intel | amd            , 0x0000'0004_u32, "XG1"               , "XGETBV"                      },
				{ intel | amd            , 0x0000'0008_u32, "XSSS"              , "XSAVES/XRSTORS"              },
			}}
		}}
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
				{ intel | amd            , 0x0000'0001_u32, "LAHF-SAHF"               , "LAHF/SAHF supported in 64-bit mode"   },
				{         amd            , 0x0000'0002_u32, "CmpLegacy"               , "Core multi-processing legacy mode"    },
				{         amd            , 0x0000'0004_u32, "SVM"                     , "Secure Virtual Machine Mode"          },
				{         amd            , 0x0000'0008_u32, "ExtApicSpace"            , "Extended APIC register space"         },
				{         amd            , 0x0000'0010_u32, "AltMovCr8"               , "LOCK MOV CR0 is MOV CR8"              },
				{ intel                  , 0x0000'0020_u32, "LZCNT"                   , "LZCNT instruction"                    },
				{         amd            , 0x0000'0020_u32, "ABM"                     , "Advanced Bit Manipulation"            },
				{         amd            , 0x0000'0040_u32, "SSE4A"                   , "SSE4A instructions"                   },
				{         amd            , 0x0000'0080_u32, "MisAlignSse"             , "Misaligned SSE Mode"                  },
				{ intel                  , 0x0000'0100_u32, "PREFETCHW"               , "PREFETCHW instruction"                },
				{         amd            , 0x0000'0100_u32, "ThreeDNowPrefetch"       , "PREFETCH and PREFETCHW instructions"  },
				{         amd            , 0x0000'0200_u32, "OSVW"                    , "OS Visible Work-around"               },
				{         amd            , 0x0000'0400_u32, "IBS"                     , "Instruction Based Sampling"           },
				{         amd            , 0x0000'0800_u32, "XOP"                     , "Extended Operation support"           },
				{         amd            , 0x0000'1000_u32, "SKINIT"                  , "SKINIT/STGI instructions"             },
				{         amd            , 0x0000'2000_u32, "WDT"                     , "Watchdog Timer Support"               },
				{         amd            , 0x0000'8000_u32, "LWP"                     , "Lightweight Profiling Support"        },
				{         amd            , 0x0001'0000_u32, "FMA4"                    , "4-operand FMA instruction"            },
				{         amd            , 0x0002'0000_u32, "TCE"                     , "Translation Cache Extension"          },
				{         amd            , 0x0008'0000_u32, "NODEID_MSR"              , "NodeId MSR"                           },
				{         amd            , 0x0020'0000_u32, "TBM"                     , "Trailing Bit Manipulation"            },
				{         amd            , 0x0040'0000_u32, "TopologyExtensions"      , "Topology Extensions"                  },
				{         amd            , 0x0080'0000_u32, "PerfCtrExtCore"          , "Core Performance Counter Extensions"  },
				{         amd            , 0x0100'0000_u32, "PerfCtrExtNB"            , "NB Performance Counter Extensions"    },
				{         amd            , 0x0400'0000_u32, "DataBreakpointExtension" , "Data Breakpoint support"              },
				{         amd            , 0x0800'0000_u32, "PerfTsc"                 , "Performance Time Stamp Counter"       },
				{         amd            , 0x1000'0000_u32, "PerfCtrExtL3"            , "L3 performance counter extensions"    },
				{         amd            , 0x2000'0000_u32, "MwaitExtended"           , "MWAITX and MONITORX"                  },
			}},
			{ edx, {
				{         amd            , 0x0000'0001_u32, "FPU"           , "x87 FPU on chip"                      },
				{         amd            , 0x0000'0002_u32, "VME"           , "Virtual Mode Enhancements"            },
				{         amd            , 0x0000'0004_u32, "DE"            , "Debugging extensions"                 },
				{         amd            , 0x0000'0008_u32, "PSE"           , "Page-size extensions"                 },
				{         amd            , 0x0000'0010_u32, "TSC"           , "Time Stamp Counter"                   },
				{         amd            , 0x0000'0020_u32, "MSR"           , "RDMSR and WRMSR Instructions"         },
				{         amd            , 0x0000'0040_u32, "PAE"           , "Physical Address Extension"           },
				{         amd            , 0x0000'0080_u32, "MCE"           , "Machine Check Exception"              },
				{         amd            , 0x0000'0100_u32, "CX8"           , "CMPXCHG8B Instruction"                },
				{         amd            , 0x0000'0200_u32, "APIC"          , "APIC On-Chip"                         },
				{ intel | amd            , 0x0000'0800_u32, "SysCallSysRet" , "SYSCALL and SYSRET Instructions"      },
				{         amd            , 0x0000'1000_u32, "MTRR"          , "Memory Type Range Registers"          },
				{         amd            , 0x0000'2000_u32, "PGE"           , "Page Global Bit"                      },
				{         amd            , 0x0000'4000_u32, "MCA"           , "Machine Check Architecture"           },
				{         amd            , 0x0000'8000_u32, "CMOV"          , "Conditional Move Instructions"        },
				{         amd            , 0x0001'0000_u32, "PAT"           , "Page Attribute Table"                 },
				{         amd            , 0x0002'0000_u32, "PSE36"         , "36-bit Page Size Extension"           },
				{ intel                  , 0x0010'0000_u32, "XD"            , "Execute Disable Bit available"        },
				{         amd            , 0x0010'0000_u32, "NX"            , "No-Execute page protection"           },
				{         amd            , 0x0040'0000_u32, "MmxExt"        , "AMD extensions to MMX instructions"   },
				{         amd            , 0x0080'0000_u32, "MMX"           , "MMX instructions"                     },
				{         amd            , 0x0100'0000_u32, "FXSR"          , "FXSAVE and FXRSTOR instructions"      },
				{         amd            , 0x0200'0000_u32, "FFXSR"         , "Fast FXSAVE and FXRSTOR"              },
				{ intel | amd            , 0x0400'0000_u32, "Page1GB"       , "1 GB large page support"              },
				{ intel | amd            , 0x0800'0000_u32, "RDTSCP"        , "RDTSCP instruction"                   },
				{ intel                  , 0x2000'0000_u32, "EM64T"         , "Intel 64 Architecture (Long Mode)"    },
				{         amd            , 0x2000'0000_u32, "LM"            , "Long mode"                            },
				{         amd            , 0x4000'0000_u32, "ThreeDNowExt"  , "AMD extensions to 3DNow! instructions"},
				{         amd            , 0x8000'0000_u32, "ThreeDNow"     , "3DNow! instructions"                  },
			}}
		}}
	}},
	{ leaf_type::ras_advanced_power_management, {
		{ subleaf_type::main, {
			{ ebx, {
				{         amd            , 0x0000'0001_u32, "McaOverflowRecov" , "MCA overflow recovery support"                         },
				{         amd            , 0x0000'0002_u32, "SUCCOR"           , "Software Uncorrectable Error Containment and Recovery" },
				{         amd            , 0x0000'0004_u32, "HWA"              , "Hardware Assert supported"                             },
				{         amd            , 0x0000'0008_u32, "ScalableMca"      , "Scalable MCA supported"                                },

			}},
			{ edx, {
				{         amd            , 0x0000'0001_u32, "TS"                    , "Temperature Sensor"                      },
				{         amd            , 0x0000'0002_u32, "FID"                   , "Frequency ID control"                    },
				{         amd            , 0x0000'0004_u32, "VID"                   , "Voltage ID control"                      },
				{         amd            , 0x0000'0008_u32, "TTP"                   , "THERMTRIP"                               },
				{         amd            , 0x0000'0010_u32, "TM"                    , "Hardware thermal control (HTC)"          },
				{         amd            , 0x0000'0020_u32, "STC"                   , "Software thermal control (HTC)"          },
				{         amd            , 0x0000'0040_u32, "100MHzSteps"           , "100 MHz multiplier control"              },
				{         amd            , 0x0000'0080_u32, "HwPstate"              , "Hardware P-state control"                },
				{ intel | amd            , 0x0000'0100_u32, "TscInvariant"          , "TSC invariant"                           },
				{         amd            , 0x0000'0200_u32, "CPB"                   , "Core performance boost"                  },
				{         amd            , 0x0000'0400_u32, "EffFreqRO"             , "Read-only effective frequency interface" },
				{         amd            , 0x0000'0800_u32, "ProcFeedbackInterface" , "Processor feedback interface"            },
				{         amd            , 0x0000'1000_u32, "ApmPwrReporting"       , "APM power reporting"                     },
				{         amd            , 0x0000'2000_u32, "ConnectedStandby"      , "Connected Standby"                       },
				{         amd            , 0x0000'4000_u32, "RAPL"                  , "Running average power limit"             },
			}}
		}}
	}},
	{ leaf_type::address_limits, {
		{ subleaf_type::main, {
			{ ebx, {
				{         amd            , 0x0000'0001_u32, "CLZERO"       , "CLZERO instruction"                      },
				{         amd            , 0x0000'0002_u32, "IRPERF"       , "Instructions retired count support"      },
				{         amd            , 0x0000'0004_u32, "ASRFPEP"      , "XSAVE (etc.) saves error pointer"        },
				{ intel                  , 0x0000'0200_u32, "WBNOINVD"     , "WBNOINVD is available"                   },
				{         amd            , 0x0000'1000_u32, "IBPB"         , "Indirect Branch Prediction Barrier"      },
				{         amd            , 0x0000'4000_u32, "IBRS"         , "Indirect Branch Restricted Speculation"  },
				{         amd            , 0x0000'8000_u32, "STIBP"        , "Single Thread Indirect Branch Predictor" },
				{         amd            , 0x0100'0000_u32, "SSBD"         , "Speculative Store Bypass Disable"        },
				{         amd            , 0x0200'0000_u32, "VIRT_SPEC_CTL", "VIRT_SPEC_CTL"                           },
			}}
		}}
	}},
	{ leaf_type::secure_virtual_machine, {
		{ subleaf_type::main, {
			{ edx, {
				{         amd            , 0x0000'0001_u32, "NP"                   , "Nested paging"                    },
				{         amd            , 0x0000'0002_u32, "LBRV"                 , "LBR virtualization"               },
				{         amd            , 0x0000'0004_u32, "SVML"                 , "SVM lock"                         },
				{         amd            , 0x0000'0008_u32, "NRIPS"                , "NRIP Save"                        },
				{         amd            , 0x0000'0010_u32, "TscRateMsr"           , "MSR-based TSC rate control"       },
				{         amd            , 0x0000'0020_u32, "VmcbClean"            , "VMCB clean bits"                  },
				{         amd            , 0x0000'0040_u32, "FlushByAsid"          , "Flush by ASID"                    },
				{         amd            , 0x0000'0080_u32, "DecodeAssists"        , "Decode assists"                   },
				{         amd            , 0x0000'0400_u32, "PauseFilter"          , "PAUSE intercept filter"           },
				{         amd            , 0x0000'1000_u32, "PauseFilterThreshold" , "PAUSE filter threshold"           },
				{         amd            , 0x0000'2000_u32, "AVIC"                 , "AMD virtual interrupt controller" },
				{         amd            , 0x0000'8000_u32, "VLS"                  , "Virtualized VMSAVE/VMLOAD"        },
				{         amd            , 0x0001'0000_u32, "vGIF"                 , "Virtualized GIF"                  },

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
				{         amd            , 0x0000'0001_u32, "SME"          , "Secure Memory Encryption supported"        },
				{         amd            , 0x0000'0002_u32, "SEV"          , "Secure Encrypted Virtualization supported" },
				{         amd            , 0x0000'0004_u32, "PageFlushMsr" , "Page Flush MSR available"                  },
				{         amd            , 0x0000'0008_u32, "SEV-ES"       , "SEV Encrypted State available"             },
			}}
		}}
	}}
};

void print_features(fmt::memory_buffer& out, const cpu_t& cpu, leaf_type leaf, subleaf_type sub, register_type reg) {
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
			if(__popcnt(f.mask) == 1_u32) {
				if(cpu.vendor & f.vendor) {
					if(0 != (value & f.mask)) {
						format_to(out, "{: >32s} \x1b[32;1m[+]\x1b[0m {:s}\n", f.mnemonic, f.description);
					} else {
						format_to(out, "{: >32s} \x1b[31;1m[-]\x1b[0m {:s}\n", f.mnemonic, f.description);
					}
				}
			}
		}
	}
}

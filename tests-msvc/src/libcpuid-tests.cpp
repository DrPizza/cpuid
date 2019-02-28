#include "stdafx.h"

#include <CppUnitTest.h>

#include "cpuid/cpuid.hpp"
#include "utility.hpp"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace Microsoft {
	namespace VisualStudio {
		namespace CppUnitTestFramework {
			template<> std::wstring ToString<flag_spec_t>(const flag_spec_t& spec) {
				std::wstringstream ss;
				ss << std::tie(spec.selector_eax, spec.selector_ecx, spec.flag_register, spec.flag_name, spec.flag_start, spec.flag_end);
				return ss.str();
			};
		}
	}
}

TEST_CLASS(parser_tests) {
public:
	TEST_METHOD(test_flag_spec_cracking) {
		const std::vector<std::pair<std::string, flag_spec_t>> flags = {
			{ "CPUID.01:ECX[SSE4.2]",                         { 0x0000'0001_u32, 0x0000'0000_u32, ecx, "sse4.2"   , 0xffff'ffff_u32, 0xffff'ffff_u32 } },
			{ "CPUID.01:ECX.MOVBE[bit 22]",                   { 0x0000'0001_u32, 0x0000'0000_u32, ecx, "movbe"    ,          22_u32,          22_u32 } },
			{ "CPUID.01H.EDX.SSE[bit 25]",                    { 0x0000'0001_u32, 0x0000'0000_u32, edx, "sse"      ,          25_u32,          25_u32 } },
			{ "CPUID.(EAX=07H, ECX=0H):EBX.BMI1[bit 3]",      { 0x0000'0007_u32, 0x0000'0000_u32, ebx, "bmi1"     ,           3_u32,           3_u32 } },
			{ "CPUID.EAX=80000001H:ECX.LZCNT[bit 5]",         { 0x8000'0001_u32, 0x0000'0000_u32, ecx, "lzcnt"    ,           5_u32,           5_u32 } },
			{ "CPUID.(EAX=07H, ECX=0H):EBX[bit 9]",           { 0x0000'0007_u32, 0x0000'0000_u32, ebx, ""         ,           9_u32,           9_u32 } },
			{ "CPUID.(EAX=0DH,ECX=0):EAX[4:3]",               { 0x0000'000d_u32, 0x0000'0000_u32, eax, ""         ,           3_u32,           4_u32 } },
			{ "CPUID.(EAX=0DH,ECX=0):EAX[9]",                 { 0x0000'000d_u32, 0x0000'0000_u32, eax, ""         ,           9_u32,           9_u32 } },
			{ "CPUID.1:ECX.OSXSAVE[bit 27]",                  { 0x0000'0001_u32, 0x0000'0000_u32, ecx, "osxsave"  ,          27_u32,          27_u32 } },
			{ "CPUID.1:ECX.OSXSAVE",                          { 0x0000'0001_u32, 0x0000'0000_u32, ecx, "osxsave"  , 0xffff'ffff_u32, 0xffff'ffff_u32 } },
			{ "CPUID.(EAX=0DH,ECX=0):EBX",                    { 0x0000'000d_u32, 0x0000'0000_u32, ebx, ""         , 0xffff'ffff_u32, 0xffff'ffff_u32 } },
			{ "CPUID.0x7.0:EBX.AVX512PF[bit 26]",             { 0x0000'0007_u32, 0x0000'0000_u32, ebx, "avx512pf" ,          26_u32,          26_u32 } },
			{ "CPUID.(EAX=0DH, ECX=04H).EBX[31:0]",           { 0x0000'000d_u32, 0x0000'0004_u32, ebx, ""         ,           0_u32,          31_u32 } },
			{ "CPUID.(EAX=07H,ECX=0H):ECX.MAWAU[bits 21:17]", { 0x0000'0007_u32, 0x0000'0000_u32, ecx, "mawau"    ,          17_u32,          21_u32 } },
			{ "CPUID.(EAX=07H, ECX=0H).EBX.MPX ",             { 0x0000'0007_u32, 0x0000'0000_u32, ebx, "mpx"      , 0xffff'ffff_u32, 0xffff'ffff_u32 } },
			{ "CPUID.1.ECX",                                  { 0x0000'0001_u32, 0x0000'0000_u32, ecx, ""         , 0xffff'ffff_u32, 0xffff'ffff_u32 } },
			{ "CPUID.(EAX=07H, ECX=0H):EBX[SGX]",             { 0x0000'0007_u32, 0x0000'0000_u32, ebx, "sgx"      , 0xffff'ffff_u32, 0xffff'ffff_u32 } },
			{ "CPUID.80000008H:EAX[7:0]",                     { 0x8000'0008_u32, 0x0000'0000_u32, eax, ""         ,           0_u32,           7_u32 } },
			{ "CPUID.1.EBX[23:16]",                           { 0x0000'0001_u32, 0x0000'0000_u32, ebx, ""         ,          16_u32,          23_u32 } },
			{ "CPUID.(EAX=07H, ECX=0H):EBX.INVPCID (bit 10)", { 0x0000'0007_u32, 0x0000'0000_u32, ebx, "invpcid"  ,          10_u32,          10_u32 } },
			{ "CPUID.80000001H:ECX.LAHF-SAHF[bit 0]",         { 0x8000'0001_u32, 0x0000'0000_u32, ecx, "lahf-sahf",           0_u32,           0_u32 } },
			{ "CPUID.01H:ECX.POPCNT [Bit 23]",                { 0x0000'0001_u32, 0x0000'0000_u32, ecx, "popcnt"   ,          23_u32,          23_u32 } },
			{ "CPUID.(EAX=0DH,ECX=1):EAX.XSS[bit 3]",         { 0x0000'000d_u32, 0x0000'0001_u32, eax, "xss"      ,           3_u32,           3_u32 } },
			{ "CPUID.80000008H:EAX[bits 7-0]",                { 0x8000'0008_u32, 0x0000'0000_u32, eax, ""         ,           0_u32,           7_u32 } },
		};

		for(const auto& f : flags) {
			const flag_spec_t spec = parse_flag_spec(f.first);
			const std::string message = f.first + " parses to " + to_string(spec);
			Logger::WriteMessage(message.c_str());
			Assert::AreEqual(f.second, spec);
		}
	}
};

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
			{ "CPUID.01:ECX[SSE4.2]",                         { 0x0000'0001ui32, 0x0000'0000ui32, ecx, "sse4.2"   , 0xffff'ffffui32, 0xffff'ffffui32 } },
			{ "CPUID.01:ECX.MOVBE[bit 22]",                   { 0x0000'0001ui32, 0x0000'0000ui32, ecx, "movbe"    ,          22ui32,          22ui32 } },
			{ "CPUID.01H.EDX.SSE[bit 25]",                    { 0x0000'0001ui32, 0x0000'0000ui32, edx, "sse"      ,          25ui32,          25ui32 } },
			{ "CPUID.(EAX=07H, ECX=0H):EBX.BMI1[bit 3]",      { 0x0000'0007ui32, 0x0000'0000ui32, ebx, "bmi1"     ,           3ui32,           3ui32 } },
			{ "CPUID.EAX=80000001H:ECX.LZCNT[bit 5]",         { 0x8000'0001ui32, 0x0000'0000ui32, ecx, "lzcnt"    ,           5ui32,           5ui32 } },
			{ "CPUID.(EAX=07H, ECX=0H):EBX[bit 9]",           { 0x0000'0007ui32, 0x0000'0000ui32, ebx, ""         ,           9ui32,           9ui32 } },
			{ "CPUID.(EAX=0DH,ECX=0):EAX[4:3]",               { 0x0000'000dui32, 0x0000'0000ui32, eax, ""         ,           3ui32,           4ui32 } },
			{ "CPUID.(EAX=0DH,ECX=0):EAX[9]",                 { 0x0000'000dui32, 0x0000'0000ui32, eax, ""         ,           9ui32,           9ui32 } },
			{ "CPUID.1:ECX.OSXSAVE[bit 27]",                  { 0x0000'0001ui32, 0x0000'0000ui32, ecx, "osxsave"  ,          27ui32,          27ui32 } },
			{ "CPUID.1:ECX.OSXSAVE",                          { 0x0000'0001ui32, 0x0000'0000ui32, ecx, "osxsave"  , 0xffff'ffffui32, 0xffff'ffffui32 } },
			{ "CPUID.(EAX=0DH,ECX=0):EBX",                    { 0x0000'000dui32, 0x0000'0000ui32, ebx, ""         , 0xffff'ffffui32, 0xffff'ffffui32 } },
			{ "CPUID.0x7.0:EBX.AVX512PF[bit 26]",             { 0x0000'0007ui32, 0x0000'0000ui32, ebx, "avx512pf" ,          26ui32,          26ui32 } },
			{ "CPUID.(EAX=0DH, ECX=04H).EBX[31:0]",           { 0x0000'000dui32, 0x0000'0004ui32, ebx, ""         ,           0ui32,          31ui32 } },
			{ "CPUID.(EAX=07H,ECX=0H):ECX.MAWAU[bits 21:17]", { 0x0000'0007ui32, 0x0000'0000ui32, ecx, "mawau"    ,          17ui32,          21ui32 } },
			{ "CPUID.(EAX=07H, ECX=0H).EBX.MPX ",             { 0x0000'0007ui32, 0x0000'0000ui32, ebx, "mpx"      , 0xffff'ffffui32, 0xffff'ffffui32 } },
			{ "CPUID.1.ECX",                                  { 0x0000'0001ui32, 0x0000'0000ui32, ecx, ""         , 0xffff'ffffui32, 0xffff'ffffui32 } },
			{ "CPUID.(EAX=07H, ECX=0H):EBX[SGX]",             { 0x0000'0007ui32, 0x0000'0000ui32, ebx, "sgx"      , 0xffff'ffffui32, 0xffff'ffffui32 } },
			{ "CPUID.80000008H:EAX[7:0]",                     { 0x8000'0008ui32, 0x0000'0000ui32, eax, ""         ,           0ui32,           7ui32 } },
			{ "CPUID.1.EBX[23:16]",                           { 0x0000'0001ui32, 0x0000'0000ui32, ebx, ""         ,          16ui32,          23ui32 } },
			{ "CPUID.(EAX=07H, ECX=0H):EBX.INVPCID (bit 10)", { 0x0000'0007ui32, 0x0000'0000ui32, ebx, "invpcid"  ,          10ui32,          10ui32 } },
			{ "CPUID.80000001H:ECX.LAHF-SAHF[bit 0]",         { 0x8000'0001ui32, 0x0000'0000ui32, ecx, "lahf-sahf",           0ui32,           0ui32 } },
			{ "CPUID.01H:ECX.POPCNT [Bit 23]",                { 0x0000'0001ui32, 0x0000'0000ui32, ecx, "popcnt"   ,          23ui32,          23ui32 } },
			{ "CPUID.(EAX=0DH,ECX=1):EAX.XSS[bit 3]",         { 0x0000'000dui32, 0x0000'0001ui32, eax, "xss"      ,           3ui32,           3ui32 } },
			{ "CPUID.80000008H:EAX[bits 7-0]",                { 0x8000'0008ui32, 0x0000'0000ui32, eax, ""         ,           0ui32,           7ui32 } },
		};

		for(const auto& f : flags) {
			const flag_spec_t spec = parse_flag_spec(f.first);
			const std::string message = f.first + " parses to " + to_string(spec);
			Logger::WriteMessage(message.c_str());
			Assert::AreEqual(f.second, spec);
		}
	}
};

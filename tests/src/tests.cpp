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
			{ "CPUID.01:ECX[SSE4.2]",                         { 0x0000'0001u32, 0x0000'0000u32, ecx, "sse4.2"   , 0xffff'ffffu32, 0xffff'ffffu32 } },
			{ "CPUID.01:ECX.MOVBE[bit 22]",                   { 0x0000'0001u32, 0x0000'0000u32, ecx, "movbe"    ,          22u32,          22u32 } },
			{ "CPUID.01H.EDX.SSE[bit 25]",                    { 0x0000'0001u32, 0x0000'0000u32, edx, "sse"      ,          25u32,          25u32 } },
			{ "CPUID.(EAX=07H, ECX=0H):EBX.BMI1[bit 3]",      { 0x0000'0007u32, 0x0000'0000u32, ebx, "bmi1"     ,           3u32,           3u32 } },
			{ "CPUID.EAX=80000001H:ECX.LZCNT[bit 5]",         { 0x8000'0001u32, 0x0000'0000u32, ecx, "lzcnt"    ,           5u32,           5u32 } },
			{ "CPUID.(EAX=07H, ECX=0H):EBX[bit 9]",           { 0x0000'0007u32, 0x0000'0000u32, ebx, ""         ,           9u32,           9u32 } },
			{ "CPUID.(EAX=0DH,ECX=0):EAX[4:3]",               { 0x0000'000du32, 0x0000'0000u32, eax, ""         ,           3u32,           4u32 } },
			{ "CPUID.(EAX=0DH,ECX=0):EAX[9]",                 { 0x0000'000du32, 0x0000'0000u32, eax, ""         ,           9u32,           9u32 } },
			{ "CPUID.1:ECX.OSXSAVE[bit 27]",                  { 0x0000'0001u32, 0x0000'0000u32, ecx, "osxsave"  ,          27u32,          27u32 } },
			{ "CPUID.1:ECX.OSXSAVE",                          { 0x0000'0001u32, 0x0000'0000u32, ecx, "osxsave"  , 0xffff'ffffu32, 0xffff'ffffu32 } },
			{ "CPUID.(EAX=0DH,ECX=0):EBX",                    { 0x0000'000du32, 0x0000'0000u32, ebx, ""         , 0xffff'ffffu32, 0xffff'ffffu32 } },
			{ "CPUID.0x7.0:EBX.AVX512PF[bit 26]",             { 0x0000'0007u32, 0x0000'0000u32, ebx, "avx512pf" ,          26u32,          26u32 } },
			{ "CPUID.(EAX=0DH, ECX=04H).EBX[31:0]",           { 0x0000'000du32, 0x0000'0004u32, ebx, ""         ,           0u32,          31u32 } },
			{ "CPUID.(EAX=07H,ECX=0H):ECX.MAWAU[bits 21:17]", { 0x0000'0007u32, 0x0000'0000u32, ecx, "mawau"    ,          17u32,          21u32 } },
			{ "CPUID.(EAX=07H, ECX=0H).EBX.MPX ",             { 0x0000'0007u32, 0x0000'0000u32, ebx, "mpx"      , 0xffff'ffffu32, 0xffff'ffffu32 } },
			{ "CPUID.1.ECX",                                  { 0x0000'0001u32, 0x0000'0000u32, ecx, ""         , 0xffff'ffffu32, 0xffff'ffffu32 } },
			{ "CPUID.(EAX=07H, ECX=0H):EBX[SGX]",             { 0x0000'0007u32, 0x0000'0000u32, ebx, "sgx"      , 0xffff'ffffu32, 0xffff'ffffu32 } },
			{ "CPUID.80000008H:EAX[7:0]",                     { 0x8000'0008u32, 0x0000'0000u32, eax, ""         ,           0u32,           7u32 } },
			{ "CPUID.1.EBX[23:16]",                           { 0x0000'0001u32, 0x0000'0000u32, ebx, ""         ,          16u32,          23u32 } },
			{ "CPUID.(EAX=07H, ECX=0H):EBX.INVPCID (bit 10)", { 0x0000'0007u32, 0x0000'0000u32, ebx, "invpcid"  ,          10u32,          10u32 } },
			{ "CPUID.80000001H:ECX.LAHF-SAHF[bit 0]",         { 0x8000'0001u32, 0x0000'0000u32, ecx, "lahf-sahf",           0u32,           0u32 } },
			{ "CPUID.01H:ECX.POPCNT [Bit 23]",                { 0x0000'0001u32, 0x0000'0000u32, ecx, "popcnt"   ,          23u32,          23u32 } },
			{ "CPUID.(EAX=0DH,ECX=1):EAX.XSS[bit 3]",         { 0x0000'000du32, 0x0000'0001u32, eax, "xss"      ,           3u32,           3u32 } },
			{ "CPUID.80000008H:EAX[bits 7-0]",                { 0x8000'0008u32, 0x0000'0000u32, eax, ""         ,           0u32,           7u32 } },
		};

		for(const auto& f : flags) {
			const flag_spec_t spec = parse_flag_spec(f.first);
			const std::string message = f.first + " parses to " + to_string(spec);
			Logger::WriteMessage(message.c_str());
			Assert::AreEqual(f.second, spec);
		}
	}
};

#include "stdafx.h"

#include "cpuid/cpuid.hpp"

#include <filesystem>

namespace xp = boost::xpressive;

namespace
{
	std::string sanitize_for_test_name(const std::string& input) {
		return xp::regex_replace(input, xp::sregex::compile("[^[:alnum:]]"), "_");
	}

	const std::vector<std::pair<std::string, cpuid::flag_spec_t>> flag_specs = {
		{ "CPUID.01:ECX[SSE4.2]",                         { 0x0000'0001_u32, 0x0000'0000_u32, cpuid::ecx, "sse4.2"   , 0xffff'ffff_u32, 0xffff'ffff_u32 } },
		{ "CPUID.01:ECX.MOVBE[bit 22]",                   { 0x0000'0001_u32, 0x0000'0000_u32, cpuid::ecx, "movbe"    ,          22_u32,          22_u32 } },
		{ "CPUID.01H.EDX.SSE[bit 25]",                    { 0x0000'0001_u32, 0x0000'0000_u32, cpuid::edx, "sse"      ,          25_u32,          25_u32 } },
		{ "CPUID.(EAX=07H, ECX=0H):EBX.BMI1[bit 3]",      { 0x0000'0007_u32, 0x0000'0000_u32, cpuid::ebx, "bmi1"     ,           3_u32,           3_u32 } },
		{ "CPUID.EAX=80000001H:ECX.LZCNT[bit 5]",         { 0x8000'0001_u32, 0x0000'0000_u32, cpuid::ecx, "lzcnt"    ,           5_u32,           5_u32 } },
		{ "CPUID.(EAX=07H, ECX=0H):EBX[bit 9]",           { 0x0000'0007_u32, 0x0000'0000_u32, cpuid::ebx, ""         ,           9_u32,           9_u32 } },
		{ "CPUID.(EAX=0DH,ECX=0):EAX[4:3]",               { 0x0000'000d_u32, 0x0000'0000_u32, cpuid::eax, ""         ,           3_u32,           4_u32 } },
		{ "CPUID.(EAX=0DH,ECX=0):EAX[9]",                 { 0x0000'000d_u32, 0x0000'0000_u32, cpuid::eax, ""         ,           9_u32,           9_u32 } },
		{ "CPUID.1:ECX.OSXSAVE[bit 27]",                  { 0x0000'0001_u32, 0x0000'0000_u32, cpuid::ecx, "osxsave"  ,          27_u32,          27_u32 } },
		{ "CPUID.1:ECX.OSXSAVE",                          { 0x0000'0001_u32, 0x0000'0000_u32, cpuid::ecx, "osxsave"  , 0xffff'ffff_u32, 0xffff'ffff_u32 } },
		{ "CPUID.(EAX=0DH,ECX=0):EBX",                    { 0x0000'000d_u32, 0x0000'0000_u32, cpuid::ebx, ""         , 0xffff'ffff_u32, 0xffff'ffff_u32 } },
		{ "CPUID.0x7.0:EBX.AVX512PF[bit 26]",             { 0x0000'0007_u32, 0x0000'0000_u32, cpuid::ebx, "avx512pf" ,          26_u32,          26_u32 } },
		{ "CPUID.(EAX=0DH, ECX=04H).EBX[31:0]",           { 0x0000'000d_u32, 0x0000'0004_u32, cpuid::ebx, ""         ,           0_u32,          31_u32 } },
		{ "CPUID.(EAX=07H,ECX=0H):ECX.MAWAU[bits 21:17]", { 0x0000'0007_u32, 0x0000'0000_u32, cpuid::ecx, "mawau"    ,          17_u32,          21_u32 } },
		{ "CPUID.(EAX=07H, ECX=0H).EBX.MPX ",             { 0x0000'0007_u32, 0x0000'0000_u32, cpuid::ebx, "mpx"      , 0xffff'ffff_u32, 0xffff'ffff_u32 } },
		{ "CPUID.1.ECX",                                  { 0x0000'0001_u32, 0x0000'0000_u32, cpuid::ecx, ""         , 0xffff'ffff_u32, 0xffff'ffff_u32 } },
		{ "CPUID.(EAX=07H, ECX=0H):EBX[SGX]",             { 0x0000'0007_u32, 0x0000'0000_u32, cpuid::ebx, "sgx"      , 0xffff'ffff_u32, 0xffff'ffff_u32 } },
		{ "CPUID.80000008H:EAX[7:0]",                     { 0x8000'0008_u32, 0x0000'0000_u32, cpuid::eax, ""         ,           0_u32,           7_u32 } },
		{ "CPUID.1.EBX[23:16]",                           { 0x0000'0001_u32, 0x0000'0000_u32, cpuid::ebx, ""         ,          16_u32,          23_u32 } },
		{ "CPUID.(EAX=07H, ECX=0H):EBX.INVPCID (bit 10)", { 0x0000'0007_u32, 0x0000'0000_u32, cpuid::ebx, "invpcid"  ,          10_u32,          10_u32 } },
		{ "CPUID.80000001H:ECX.LAHF-SAHF[bit 0]",         { 0x8000'0001_u32, 0x0000'0000_u32, cpuid::ecx, "lahf-sahf",           0_u32,           0_u32 } },
		{ "CPUID.01H:ECX.POPCNT [Bit 23]",                { 0x0000'0001_u32, 0x0000'0000_u32, cpuid::ecx, "popcnt"   ,          23_u32,          23_u32 } },
		{ "CPUID.(EAX=0DH,ECX=1):EAX.XSS[bit 3]",         { 0x0000'000d_u32, 0x0000'0001_u32, cpuid::eax, "xss"      ,           3_u32,           3_u32 } },
		{ "CPUID.80000008H:EAX[bits 7-0]",                { 0x8000'0008_u32, 0x0000'0000_u32, cpuid::eax, ""         ,           0_u32,           7_u32 } },
	};
}

struct CpuidFlagCrackingTest : ::testing::TestWithParam<std::pair<std::string, cpuid::flag_spec_t>>
{
};

TEST_P(CpuidFlagCrackingTest, ParserTest) {
	std::pair<std::string, cpuid::flag_spec_t> data = GetParam();
	const cpuid::flag_spec_t spec = cpuid::parse_flag_spec(data.first);
	EXPECT_EQ(data.second, spec);
}

std::string flag_spec_param_printer(testing::TestParamInfo<std::pair<std::string, cpuid::flag_spec_t>> data) {
	std::string index_padding = data.index < 10  ? "00"
	                          : data.index < 100 ? "0"
	                          :                    "";
	return index_padding + std::to_string(data.index) + "_" + sanitize_for_test_name(data.param.first);
}

struct file_parse_data
{
	std::filesystem::path file_name;
	cpuid::file_format format;
	std::string expected;
};

std::vector<file_parse_data> enumerate_test_files() {
	namespace fs = std::filesystem;

	wchar_t dir[260];
	::GetModuleFileNameW(nullptr, dir, 260);
	wchar_t* i = dir + std::wcslen(dir);
	while(*--i != L'\\') {
		*i = '\0';
	}
	::SetCurrentDirectoryW(dir);

	const auto string_to_format = [] (const std::string& str) {
		if(str == "etallen") {
			return cpuid::file_format::etallen;
		} else if(str == "libcpuid") {
			return cpuid::file_format::libcpuid;
		} else if(str == "aida64") {
			return cpuid::file_format::aida64;
		} else {
			return cpuid::file_format::native;
		}
	};

	std::vector<file_parse_data> data;
	for(const auto& d : fs::directory_iterator("../../../libcpuid/tests/data/dumps")) {
		if(d.is_directory()) {
			const fs::path directory = fs::absolute(fs::path(d.path()).make_preferred());
			const cpuid::file_format format = string_to_format((--directory.end())->string());
			
			for(const auto& f : fs::recursive_directory_iterator(d.path())) {
				if(f.is_regular_file() && f.path().filename() != "source.txt") {
					data.push_back({ f.path(), format, "" });
				}
			}
		}
	}
	return data;
}

std::vector<file_parse_data> file_specs = enumerate_test_files();

struct CpuidFileParserTest : ::testing::TestWithParam<file_parse_data>
{
};

TEST_P(CpuidFileParserTest, FileParserTest) {
	file_parse_data data = GetParam();
	std::ifstream fin(data.file_name.string());
	EXPECT_NO_THROW(cpuid::enumerate_file(fin, data.format));
}

std::string file_spec_param_printer(testing::TestParamInfo<file_parse_data> data) {
	namespace fs = std::filesystem;
	const std::string index_padding = data.index < 10  ? "00"
	                                : data.index < 100 ? "0"
	                                :                    "";
	const fs::path p = fs::absolute(data.param.file_name.make_preferred());
	const std::string raw_filename = p.stem().string();
	const std::string safe_filename = sanitize_for_test_name(raw_filename);
	const auto format_to_string = [] (cpuid::file_format format) {
		switch(format) {
		case cpuid::file_format::native:
			return "native";
		case cpuid::file_format::etallen:
			return "etallen";
		case cpuid::file_format::libcpuid:
			return "libcpuid";
		case cpuid::file_format::aida64:
			return "aida64";
		case cpuid::file_format::cpuinfo:
			return "cpuinfo";
		default:
			UNREACHABLE();
		}
	};
	return index_padding + std::to_string(data.index) + "_" + format_to_string(data.param.format) + "_" + safe_filename;
}

INSTANTIATE_TEST_SUITE_P(CpuidFullTests, CpuidFlagCrackingTest, ::testing::ValuesIn(flag_specs), flag_spec_param_printer);
INSTANTIATE_TEST_SUITE_P(CpuidFullTests, CpuidFileParserTest, ::testing::ValuesIn(file_specs), file_spec_param_printer);


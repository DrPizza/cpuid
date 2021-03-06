﻿#include "stdafx.h"

#include "cache-and-topology.hpp"

#include "utility.hpp"

#include <thread>

#include <fmt/format.h>

#if defined(_MSC_VER)
#pragma warning(disable: 26446) // warning c26446: Prefer to use gsl::at() instead of unchecked subscript operator (bounds.4).
#endif

namespace cpuid {

std::string print_size(std::size_t cache_bytes) {
	using namespace fmt::literals;

	double printable_cache_size = cache_bytes / 1'024.0;
	char   cache_scale = 'K';
	if(printable_cache_size > 1'024.0) {
		printable_cache_size /= 1'024.0;
		cache_scale = 'M';
	}
	return "{:<5g} {:c}B"_format(printable_cache_size, cache_scale);
}

enum cache_type_t : std::uint8_t
{
	data_tlb        = 0x01_u8,
	instruction_tlb = 0x02_u8,
	unified_tlb     = 0x04_u8,
	all_tlb         = 0x07_u8,
	data            = 0x10_u8,
	instructions    = 0x20_u8,
	unified         = 0x40_u8,
	all_cache       = 0x70_u8,
	trace           = 0x80_u8,
};

std::string to_string(cache_type_t type) {
	switch(type) {
	case data_tlb:
		return "Data TLB";
	case instruction_tlb:
		return "Instruction TLB";
	case unified_tlb: 
		return "Shared TLB";
	case data:
		return "Data cache";
	case instructions:
		return "Instruction cache";
	case unified:
		return "Unified cache";
	case trace:
		return "Trace cache";
	default:
		UNREACHABLE();
	}
}

enum cache_level_t : std::uint8_t
{
	no_cache,
	level_0,
	level_1,
	level_2,
	level_3,
};

std::string to_string(cache_level_t level) {
	switch(level) {
	case no_cache:
		return "";
	case level_0:
		return "0th-level";
	case level_1:
		return "1st-level";
	case level_2:
		return "2nd-level";
	case level_3:
		return "3rd-level";
	default:
		UNREACHABLE();
	}
}

enum cache_attributes_t : std::uint8_t
{
	no_attributes      = 0x00_u8,
	pages_4k           = 0x01_u8,
	pages_2m           = 0x02_u8,
	pages_4m           = 0x04_u8,
	pages_1g           = 0x08_u8,
	all_page_sizes     = 0x0f_u8,
	sectored_two_lines = 0x10_u8,
};

constexpr cache_attributes_t operator|(const cache_attributes_t& lhs, const cache_attributes_t& rhs) noexcept {
	return static_cast<cache_attributes_t>(static_cast<std::uint32_t>(lhs) | static_cast<std::uint32_t>(rhs));
}

std::string to_string(cache_attributes_t attrs) {
	fmt::memory_buffer out;
	const std::uint8_t page = gsl::narrow_cast<std::uint8_t>(attrs & all_page_sizes);
	std::uint8_t mask = 1_u8;
	bool needs_separator = false;
	do {
		switch(page & mask) {
		case 0:
			format_to(out, "");
			break;
		case pages_4k:
			if(needs_separator) {
				format_to(out, " | ");
			}
			format_to(out, "4 KByte pages");
			needs_separator = true;
			break;
		case pages_2m:
			if(needs_separator) {
				format_to(out, " | ");
			}
			format_to(out, "2 MByte pages");
			needs_separator = true;
			break;
		case pages_4m:
			if(needs_separator) {
				format_to(out, " | ");
			}
			format_to(out, "4 MByte pages");
			needs_separator = true;
			break;
		case pages_1g:
			if(needs_separator) {
				format_to(out, " | ");
			}
			format_to(out, "1 GByte pages");
			needs_separator = true;
			break;
		}
		mask <<= 1_u8;
	} while(mask & all_page_sizes);
	return to_string(out);
}

enum cache_associativity_t : std::uint8_t 
{
	unknown_associativity,
	direct_mapped,
	fully_associative = 0xff_u8
};

std::string to_string(cache_associativity_t assoc) {
	switch(assoc) {
	case 0:
		return "unknown associativity";
	case 1:
		return "direct mapped";
	case 0xff_u8:
		return "fully associative";
	default:
		return fmt::format("{0:d}-way set associative", assoc);
	}
}

struct cache_descriptor_t
{
	cache_type_t type;
	cache_level_t level;
	std::uint32_t size;
	std::uint32_t entries;
	cache_attributes_t attributes;
	cache_associativity_t associativity;
	std::uint32_t line_size;
};

using cache_descriptor_map_t = std::map<std::uint8_t, cache_descriptor_t>;

#define KB * 1024
#define MB KB KB

const cache_descriptor_map_t standard_cache_descriptors = {
	{ 0x01, { instruction_tlb, no_cache, 0      , 32  , pages_4k                      , cache_associativity_t{0x04}, 0  } },
	{ 0x02, { instruction_tlb, no_cache, 0      , 2   , pages_4m                      , fully_associative          , 0  } },
	{ 0x03, { data_tlb       , no_cache, 0      , 64  , pages_4k                      , cache_associativity_t{0x04}, 0  } },
	{ 0x04, { data_tlb       , no_cache, 0      , 8   , pages_4m                      , cache_associativity_t{0x04}, 0  } },
	{ 0x05, { data_tlb       , no_cache, 0      , 32  , pages_4m                      , cache_associativity_t{0x04}, 0  } },
	{ 0x06, { instructions   , level_1 , 8    KB, 0   , no_attributes                 , cache_associativity_t{0x04}, 32 } },
	{ 0x08, { instructions   , level_1 , 16   KB, 0   , no_attributes                 , cache_associativity_t{0x04}, 32 } },
	{ 0x09, { instructions   , level_1 , 32   KB, 0   , no_attributes                 , cache_associativity_t{0x04}, 64 } },
	{ 0x0a, { data           , level_1 , 8    KB, 0   , no_attributes                 , cache_associativity_t{0x02}, 32 } },
	{ 0x0b, { instruction_tlb, level_1 , 0      , 4   , pages_4m                      , cache_associativity_t{0x04}, 0  } },
	{ 0x0c, { data           , level_1 , 16   KB, 0   , no_attributes                 , cache_associativity_t{0x04}, 32 } },
	{ 0x0d, { data           , level_1 , 16   KB, 0   , no_attributes                 , cache_associativity_t{0x04}, 64 } },
	{ 0x0e, { data           , level_1 , 24   KB, 0   , no_attributes                 , cache_associativity_t{0x06}, 64 } },
	{ 0x1d, { unified        , level_2 , 128  KB, 0   , no_attributes                 , cache_associativity_t{0x02}, 64 } },
	{ 0x21, { unified        , level_2 , 256  KB, 0   , no_attributes                 , cache_associativity_t{0x08}, 64 } },
	{ 0x22, { unified        , level_3 , 512  KB, 0   , sectored_two_lines            , cache_associativity_t{0x04}, 64 } },
	{ 0x23, { unified        , level_3 , 1    MB, 0   , sectored_two_lines            , cache_associativity_t{0x08}, 64 } },
	{ 0x24, { unified        , level_2 , 1    MB, 0   , sectored_two_lines            , cache_associativity_t{0x10}, 64 } },
	{ 0x25, { unified        , level_3 , 2    MB, 0   , sectored_two_lines            , cache_associativity_t{0x08}, 64 } },
	{ 0x29, { unified        , level_3 , 4    MB, 0   , sectored_two_lines            , cache_associativity_t{0x08}, 64 } },
	{ 0x2c, { data           , level_1 , 32   KB, 0   , no_attributes                 , cache_associativity_t{0x08}, 64 } },
	{ 0x30, { instructions   , level_1 , 32   KB, 0   , no_attributes                 , cache_associativity_t{0x08}, 64 } },
	{ 0x41, { unified        , level_2 , 128  KB, 0   , no_attributes                 , cache_associativity_t{0x04}, 32 } },
	{ 0x42, { unified        , level_2 , 256  KB, 0   , no_attributes                 , cache_associativity_t{0x04}, 32 } },
	{ 0x43, { unified        , level_2 , 512  KB, 0   , no_attributes                 , cache_associativity_t{0x04}, 32 } },
	{ 0x44, { unified        , level_2 , 1    MB, 0   , no_attributes                 , cache_associativity_t{0x04}, 32 } },
	{ 0x45, { unified        , level_2 , 2    MB, 0   , no_attributes                 , cache_associativity_t{0x04}, 32 } },
	{ 0x46, { unified        , level_3 , 4    MB, 0   , no_attributes                 , cache_associativity_t{0x04}, 64 } },
	{ 0x47, { unified        , level_3 , 8    MB, 0   , no_attributes                 , cache_associativity_t{0x08}, 64 } },
	{ 0x48, { unified        , level_2 , 3    MB, 0   , no_attributes                 , cache_associativity_t{0x0c}, 64 } },
	{ 0x4a, { unified        , level_3 , 6    MB, 0   , no_attributes                 , cache_associativity_t{0x0c}, 64 } },
	{ 0x4b, { unified        , level_3 , 8    MB, 0   , no_attributes                 , cache_associativity_t{0x10}, 64 } },
	{ 0x4c, { unified        , level_3 , 12   MB, 0   , no_attributes                 , cache_associativity_t{0x0c}, 64 } },
	{ 0x4d, { unified        , level_3 , 16   MB, 0   , no_attributes                 , cache_associativity_t{0x10}, 64 } },
	{ 0x4e, { unified        , level_2 , 6    MB, 0   , no_attributes                 , cache_associativity_t{0x18}, 64 } },
	{ 0x4f, { instruction_tlb, no_cache, 0      , 32  , pages_4k                      , unknown_associativity      , 0  } },
	{ 0x50, { instruction_tlb, no_cache, 0      , 64  , pages_4k | pages_2m | pages_4m, unknown_associativity      , 0  } },
	{ 0x51, { instruction_tlb, no_cache, 0      , 128 , pages_4k | pages_2m | pages_4m, unknown_associativity      , 0  } },
	{ 0x52, { instruction_tlb, no_cache, 0      , 256 , pages_4k | pages_2m | pages_4m, unknown_associativity      , 0  } },
	{ 0x55, { instruction_tlb, no_cache, 0      , 7   , pages_2m | pages_4m           , fully_associative          , 0  } },
	{ 0x56, { data_tlb       , level_0 , 0      , 16  , pages_4m                      , cache_associativity_t{0x04}, 0  } },
	{ 0x57, { data_tlb       , level_0 , 0      , 16  , pages_4k                      , cache_associativity_t{0x04}, 0  } },
	{ 0x59, { data_tlb       , level_0 , 0      , 16  , pages_4k                      , fully_associative          , 0  } },
	{ 0x5a, { data_tlb       , no_cache, 0      , 32  , pages_2m | pages_4m           , cache_associativity_t{0x04}, 0  } },
	{ 0x5b, { data_tlb       , no_cache, 0      , 64  , pages_4k | pages_4m           , fully_associative          , 0  } },
	{ 0x5c, { data_tlb       , no_cache, 0      , 128 , pages_4k | pages_4m           , fully_associative          , 0  } },
	{ 0x5d, { data_tlb       , no_cache, 0      , 256 , pages_4k | pages_4m           , fully_associative          , 0  } },
	{ 0x60, { data           , level_1 , 16   KB, 0   , sectored_two_lines            , cache_associativity_t{0x08}, 64 } },
	{ 0x61, { instruction_tlb, no_cache, 0      , 48  , pages_4k                      , fully_associative          , 0  } },
	{ 0x64, { data_tlb       , no_cache, 0      , 512 , pages_4k                      , cache_associativity_t{0x04}, 0  } },
	{ 0x66, { data           , level_1 , 8    KB, 0   , sectored_two_lines            , cache_associativity_t{0x04}, 64 } },
	{ 0x67, { data           , level_1 , 16   KB, 0   , sectored_two_lines            , cache_associativity_t{0x04}, 64 } },
	{ 0x68, { data           , level_1 , 32   KB, 0   , sectored_two_lines            , cache_associativity_t{0x04}, 64 } },
	{ 0x6a, { instruction_tlb, no_cache, 0      , 64  , pages_4k                      , cache_associativity_t{0x08}, 0  } },
	{ 0x6b, { data_tlb       , no_cache, 0      , 256 , pages_4k                      , cache_associativity_t{0x08}, 0  } },
	{ 0x6c, { data_tlb       , no_cache, 0      , 128 , pages_2m | pages_4m           , cache_associativity_t{0x08}, 0  } },
	{ 0x6d, { data_tlb       , no_cache, 0      , 16  , pages_1g                      , fully_associative          , 0  } },
	{ 0x70, { trace          , level_1 , 12   KB, 0   , no_attributes                 , cache_associativity_t{0x08}, 0  } },
	{ 0x71, { trace          , level_1 , 16   KB, 0   , no_attributes                 , cache_associativity_t{0x08}, 0  } },
	{ 0x72, { trace          , level_1 , 32   KB, 0   , no_attributes                 , cache_associativity_t{0x08}, 0  } },
	{ 0x76, { instruction_tlb, no_cache, 0      , 8   , pages_2m | pages_4m           , fully_associative          , 0  } },
	{ 0x78, { unified        , level_2 , 1    MB, 0   , no_attributes                 , cache_associativity_t{0x04}, 64 } },
	{ 0x79, { unified        , level_2 , 128  KB, 0   , sectored_two_lines            , cache_associativity_t{0x08}, 64 } },
	{ 0x7a, { unified        , level_2 , 256  KB, 0   , sectored_two_lines            , cache_associativity_t{0x04}, 64 } },
	{ 0x7b, { unified        , level_2 , 512  KB, 0   , sectored_two_lines            , cache_associativity_t{0x04}, 64 } },
	{ 0x7c, { unified        , level_2 , 1    MB, 0   , sectored_two_lines            , cache_associativity_t{0x04}, 64 } },
	{ 0x7d, { unified        , level_2 , 2    MB, 0   , no_attributes                 , cache_associativity_t{0x08}, 64 } },
	{ 0x7f, { unified        , level_2 , 512  KB, 0   , no_attributes                 , cache_associativity_t{0x02}, 64 } },
	{ 0x80, { unified        , level_2 , 512  KB, 0   , no_attributes                 , cache_associativity_t{0x08}, 64 } },
	{ 0x82, { unified        , level_2 , 256  KB, 0   , no_attributes                 , cache_associativity_t{0x08}, 32 } },
	{ 0x83, { unified        , level_2 , 512  KB, 0   , no_attributes                 , cache_associativity_t{0x08}, 32 } },
	{ 0x84, { unified        , level_2 , 1    MB, 0   , no_attributes                 , cache_associativity_t{0x08}, 32 } },
	{ 0x85, { unified        , level_2 , 2    MB, 0   , no_attributes                 , cache_associativity_t{0x08}, 32 } },
	{ 0x86, { unified        , level_2 , 512  KB, 0   , no_attributes                 , cache_associativity_t{0x04}, 64 } },
	{ 0x87, { unified        , level_2 , 1    MB, 0   , no_attributes                 , cache_associativity_t{0x08}, 64 } },
	{ 0xa0, { data_tlb       , no_cache, 0      , 32  , pages_4k                      , fully_associative          , 0  } },
	{ 0xb0, { instruction_tlb, no_cache, 0      , 128 , pages_4k                      , cache_associativity_t{0x04}, 0  } },
	{ 0xb1, { instruction_tlb, no_cache, 0      , 4   , pages_2m                      , cache_associativity_t{0x04}, 0  } },
	{ 0xb2, { data_tlb       , no_cache, 0      , 64  , pages_4k                      , cache_associativity_t{0x04}, 0  } },
	{ 0xb3, { data_tlb       , no_cache, 0      , 128 , pages_4k                      , cache_associativity_t{0x04}, 0  } },
	{ 0xb4, { data_tlb       , level_1 , 0      , 256 , pages_4k                      , cache_associativity_t{0x04}, 0  } },
	{ 0xb5, { instruction_tlb, no_cache, 0      , 64  , pages_4k                      , cache_associativity_t{0x08}, 0  } },
	{ 0xb6, { instruction_tlb, no_cache, 0      , 128 , pages_4k                      , cache_associativity_t{0x08}, 0  } },
	{ 0xba, { data_tlb       , level_1 , 0      , 64  , pages_4k                      , cache_associativity_t{0x04}, 0  } },
	{ 0xc0, { data_tlb       , no_cache, 0      , 8   , pages_4k | pages_4m           , cache_associativity_t{0x04}, 0  } },
	{ 0xc1, { unified_tlb    , level_2 , 0      , 1024, pages_4k | pages_2m           , cache_associativity_t{0x08}, 0  } },
	{ 0xc2, { data_tlb       , level_1 , 0      , 16  , pages_4k | pages_2m           , cache_associativity_t{0x04}, 0  } },
	{ 0xc2, { data_tlb       , level_1 , 0      , 32  , pages_2m | pages_4m           , cache_associativity_t{0x04}, 0  } },
	{ 0xca, { unified_tlb    , level_2 , 0      , 512 , pages_4k                      , cache_associativity_t{0x04}, 0  } },
	{ 0xd0, { unified        , level_3 , 512  KB, 0   , no_attributes                 , cache_associativity_t{0x04}, 64 } },
	{ 0xd1, { unified        , level_3 , 1    MB, 0   , no_attributes                 , cache_associativity_t{0x04}, 64 } },
	{ 0xd2, { unified        , level_3 , 2    MB, 0   , no_attributes                 , cache_associativity_t{0x04}, 64 } },
	{ 0xd6, { unified        , level_3 , 1    MB, 0   , no_attributes                 , cache_associativity_t{0x08}, 64 } },
	{ 0xd7, { unified        , level_3 , 2    MB, 0   , no_attributes                 , cache_associativity_t{0x08}, 64 } },
	{ 0xd8, { unified        , level_3 , 4    MB, 0   , no_attributes                 , cache_associativity_t{0x08}, 64 } },
	{ 0xdc, { unified        , level_3 , 1536 KB, 0   , no_attributes                 , cache_associativity_t{0x0c}, 64 } },
	{ 0xdd, { unified        , level_3 , 3    MB, 0   , no_attributes                 , cache_associativity_t{0x0c}, 64 } },
	{ 0xde, { unified        , level_3 , 6    MB, 0   , no_attributes                 , cache_associativity_t{0x0c}, 64 } },
	{ 0xe2, { unified        , level_3 , 2    MB, 0   , no_attributes                 , cache_associativity_t{0x10}, 64 } },
	{ 0xe3, { unified        , level_3 , 4    MB, 0   , no_attributes                 , cache_associativity_t{0x10}, 64 } },
	{ 0xe4, { unified        , level_3 , 8    MB, 0   , no_attributes                 , cache_associativity_t{0x10}, 64 } },
	{ 0xea, { unified        , level_3 , 12   MB, 0   , no_attributes                 , cache_associativity_t{0x18}, 64 } },
	{ 0xeb, { unified        , level_3 , 18   MB, 0   , no_attributes                 , cache_associativity_t{0x18}, 64 } },
	{ 0xec, { unified        , level_3 , 24   MB, 0   , no_attributes                 , cache_associativity_t{0x18}, 64 } }
};

using dual_descriptor_t = std::pair<cache_descriptor_t, cache_descriptor_t>;
using dual_descriptors_map_t = std::map<std::uint8_t, dual_descriptor_t>;

const dual_descriptors_map_t dual_cache_descriptors = {
	{ 0x49, {
		{ unified    , level_3 , 4    MB, 0   , no_attributes      , static_cast<cache_associativity_t>(0x10), 64 },
		{ unified    , level_2 , 4    MB, 0   , no_attributes      , static_cast<cache_associativity_t>(0x10), 64 }
	}},
	{ 0x63, {
		{ data_tlb   , no_cache, 0      , 32  , pages_2m | pages_4m, static_cast<cache_associativity_t>(0x04), 0 },
		{ data_tlb   , no_cache, 0      , 4   , pages_1g           , static_cast<cache_associativity_t>(0x04), 0 }
	}},
	{ 0xc3, {
		{ unified_tlb, level_2 , 0      , 1536, pages_4k | pages_2m, static_cast<cache_associativity_t>(0x06), 0 },
		{ unified_tlb, level_2 , 0      , 16  , pages_1g           , static_cast<cache_associativity_t>(0x04), 0 }
	}}
};

#undef MB
#undef KB

std::string to_string(cache_descriptor_t desc) {
	using namespace fmt::literals;
	fmt::memory_buffer out;
	format_to(out, to_string(desc.level));
	if(out.size() > 0) {
		format_to(out, " ");
	}
	format_to(out, to_string(desc.type));
	format_to(out, ": ");
	if(desc.type & all_cache) {
		format_to(out, print_size(desc.size));
	} else if(desc.type & all_tlb) {
		format_to(out, to_string(desc.attributes));
	} else if(desc.type & trace) {
		format_to(out, "{} K-\u00b5op", (desc.size / 1'024));
	}
	format_to(out, ", ");
	format_to(out, to_string(desc.associativity));
	if(desc.type & all_cache) {
		format_to(out, ", {} byte line size", desc.line_size);
		if(desc.attributes & sectored_two_lines) {
			format_to(out, ", 2 lines per sector");
		}
	} else if(desc.type & all_tlb) {
		format_to(out, ", {} entries", desc.entries);
	}
	return to_string(out);
}

struct decomposed_cache_t
{
	std::vector<gsl::not_null<const cache_descriptor_t*>> tlb_descriptors;
	std::vector<gsl::not_null<const cache_descriptor_t*>> cache_descriptors;
	std::vector<gsl::not_null<const cache_descriptor_t*>> other_descriptors;
	std::vector<std::string>                              non_conformant_descriptors;
};

decomposed_cache_t decompose_cache_descriptors(const cpu_t& cpu, const register_set_t& regs) {
	decomposed_cache_t decomposed;
	const auto bytes = gsl::as_bytes(gsl::make_span(regs));
	std::ptrdiff_t idx = 0;
	for(register_type r = eax; r <= edx; ++r, idx += sizeof(std::uint32_t)) {
		if(regs[r] & 0x8000'0000_u32) {
			continue;
		}
		for(std::ptrdiff_t j = 0; j < sizeof(std::uint32_t); ++j) {
			const std::uint8_t value = gsl::to_integer<std::uint8_t>(bytes[idx + j]);
			switch(value) {
			case 0x00_u8:
				break;
			case 0x40_u8:
				decomposed.non_conformant_descriptors.push_back("No 2nd-level cache or, if processor contains a valid 2nd-level cache, no 3rd-level cache");
				break;
			case 0x49_u8:
				{
					const auto it = dual_cache_descriptors.find(value);
					if(cpu.model.family == 0x0f && cpu.model.model == 0x06) {
						decomposed.cache_descriptors.push_back(gsl::not_null(&(it->second.first)));
					} else {
						decomposed.cache_descriptors.push_back(gsl::not_null(&(it->second.second)));
					}
				}
				break;
			case 0xf0_u8:
				decomposed.non_conformant_descriptors.push_back("64-byte prefetching");
				break;
			case 0xf1_u8:
				decomposed.non_conformant_descriptors.push_back("128-byte prefetching");
				break;
			case 0xfe_u8:
			case 0xff_u8:
				break;
			default:
				{
					const auto dual = dual_cache_descriptors.find(value);
					if(dual != dual_cache_descriptors.end()) {
						decomposed.tlb_descriptors.push_back(gsl::not_null(&(dual->second.first)));
						decomposed.tlb_descriptors.push_back(gsl::not_null(&(dual->second.second)));
						break;
					}
					const auto it = standard_cache_descriptors.find(value);
					if(it != standard_cache_descriptors.end()) {
						if(it->second.type & all_tlb) {
							decomposed.tlb_descriptors.push_back(gsl::not_null(&(it->second)));
						} else if(it->second.type & all_cache) {
							decomposed.cache_descriptors.push_back(gsl::not_null(&(it->second)));
						} else {
							decomposed.other_descriptors.push_back(gsl::not_null(&(it->second)));
						}
						break;
					}
					decomposed.non_conformant_descriptors.push_back(fmt::format("Unknown cache type: {:#2x}", value));
				}
				break;
			}
		}
	}

	const auto cmp = [](const cache_descriptor_t* lhs, const cache_descriptor_t* rhs) {
		return lhs->type  != rhs->type  ? lhs->type    < rhs->type
		     : lhs->level != rhs->level ? lhs->level   < rhs->level
		     : lhs->size  != rhs->size  ? lhs->size    > rhs->size     // sic; I want bigger caches at a given level listed first
		     :                            lhs->entries > rhs->entries; // sic
	};

	std::sort(std::begin(decomposed.tlb_descriptors  ), std::end(decomposed.tlb_descriptors  ), cmp);
	std::sort(std::begin(decomposed.cache_descriptors), std::end(decomposed.cache_descriptors), cmp);
	std::sort(std::begin(decomposed.other_descriptors), std::end(decomposed.other_descriptors), cmp);

	return decomposed;
}

void print_cache_tlb_info(fmt::memory_buffer& out, const cpu_t& cpu) {
	const register_set_t& regs = cpu.leaves.at(leaf_type::cache_and_tlb).at(subleaf_type::main);

	if((regs[eax] & 0x0000'00ff_u32) != 0x0000'0001_u32) {
		return;
	}

	decomposed_cache_t decomposed = decompose_cache_descriptors(cpu, regs);

	format_to(out, "Cache and TLB\n");
	for(const auto d : decomposed.tlb_descriptors) {
		format_to(out, "\t{:s}\n", to_string(*d));
	}
	for(const auto d : decomposed.cache_descriptors) {
		format_to(out, "\t{:s}\n", to_string(*d));
	}
	for(const auto d : decomposed.other_descriptors) {
		format_to(out, "\t{:s}\n", to_string(*d));
	}
	for(const std::string& s : decomposed.non_conformant_descriptors) {
		format_to(out, "\t{:s}\n", s);
	}
	format_to(out, "\n");
}

void enumerate_deterministic_cache(cpu_t& cpu) {
	subleaf_type sub = subleaf_type::main;
	while(true) {
		register_set_t regs = cpuid(leaf_type::deterministic_cache, sub);
		if((regs[eax] & 0x0000'001f_u32) == 0) {
			break;
		}
		cpu.leaves[leaf_type::deterministic_cache][subleaf_type{ sub }] = regs;
		++sub;
	}
}

void print_deterministic_cache(fmt::memory_buffer& out, const cpu_t& cpu) {
	format_to(out, "Deterministic cache\n");

	for(const auto& sub : cpu.leaves.at(leaf_type::deterministic_cache)) {
		const register_set_t& regs = sub.second;

		const struct
		{
			std::uint32_t type                           : 5;
			std::uint32_t level                          : 3;
			std::uint32_t self_initializing              : 1;
			std::uint32_t fully_associative              : 1;
			std::uint32_t reserved_1                     : 4;
			std::uint32_t maximum_addressable_thread_ids : 12;
			std::uint32_t maximum_addressable_core_ids   : 6;
		} a = bit_cast<decltype(a)>(regs[eax]);

		const struct
		{
			std::uint32_t coherency_line_size      : 12;
			std::uint32_t physical_line_partitions : 10;
			std::uint32_t associativity_ways       : 10;
		} b = bit_cast<decltype(b)>(regs[ebx]);

		const struct
		{
			std::uint32_t writeback_invalidates : 1;
			std::uint32_t cache_inclusive       : 1;
			std::uint32_t complex_indexing      : 1;
			std::uint32_t reserved_1            : 29;
		} d = bit_cast<decltype(d)>(regs[edx]);
		
		const std::size_t sets = regs[ecx];
		const std::size_t cache_size = (b.associativity_ways       + 1_u64)
		                             * (b.physical_line_partitions + 1_u64)
		                             * (b.coherency_line_size      + 1_u64)
		                             * (sets                       + 1_u64);

		format_to(out, "\t{:s} L{:d} ", print_size(cache_size), a.level);
		switch(a.type) {
		case 1:
			format_to(out, "data");
			break;
		case 2:
			format_to(out, "instruction");
			break;
		case 3:
			format_to(out, "unified");
			break;
		}
		format_to(out, "\n");
		format_to(out, "\t\t{:d} bytes per line \u00d7 {:d} ways \u00d7 {:d} partitions \u00d7 {:d} sets = {:s}.\n", b.coherency_line_size      + 1_u32,
		                                                                                                             b.associativity_ways       + 1_u32,
		                                                                                                             b.physical_line_partitions + 1_u32,
		                                                                                                             sets                       + 1_u32,
		                                                                                                             print_size(cache_size));
		if(a.self_initializing) {
			format_to(out, "\t\tSelf-initializing.\n");
		}
		if(a.fully_associative) {
			format_to(out, "\t\tFully associative.\n");
		} else {
			format_to(out, "\t\t{:d}-way set associative.\n", b.associativity_ways + 1_u32);
		}
		if(d.writeback_invalidates) {
			format_to(out, "\t\tWBINVD/INVD does not invalidate lower level caches for other threads.\n");
		} else {
			format_to(out, "\t\tWBINVD/INVD invalidates lower level caches for all threads.\n");
		}
		format_to(out, "\t\tCache is {:s}inclusive of lower cache levels.\n", d.cache_inclusive != 0 ? "" : "not ");
		format_to(out, "\t\tCache is {:s}complex mapped.\n", d.complex_indexing != 0 ? "" : "not ");
		format_to(out, "\t\tCache is shared by up to {:d} threads, with up to {:d} cores in the package.\n", a.maximum_addressable_thread_ids + 1, a.maximum_addressable_core_ids + 1);
		format_to(out, "\n");
	}
}

void enumerate_extended_topology(cpu_t& cpu) {
	cpu.leaves[leaf_type::extended_topology][subleaf_type::main] = cpuid(leaf_type::extended_topology, subleaf_type::main);
	for(subleaf_type sub = subleaf_type{ 1 }; ; ++sub) {
		register_set_t regs = cpuid(leaf_type::extended_topology, sub);
		if((regs[ecx] & 0x0000'ff00_u32) == 0_u32) {
			break;
		}
		cpu.leaves[leaf_type::extended_topology][sub] = regs;
	}
}

void enumerate_extended_topology_v2(cpu_t& cpu) {
	cpu.leaves[leaf_type::extended_topology_v2][subleaf_type::main] = cpuid(leaf_type::extended_topology_v2, subleaf_type::main);
	for(subleaf_type sub = subleaf_type{ 1 }; ; ++sub) {
		register_set_t regs = cpuid(leaf_type::extended_topology_v2, sub);
		if((regs[ecx] & 0x0000'ff00_u32) == 0_u32) {
			break;
		}
		cpu.leaves[leaf_type::extended_topology_v2][sub] = regs;
	}
}

void print_extended_topology(fmt::memory_buffer& out, const cpu_t& cpu) {
	for(const auto& sub : cpu.leaves.at(leaf_type::extended_topology)) {
		const register_set_t& regs = sub.second;
		switch(sub.first) {
		case subleaf_type::main:
			format_to(out, "Extended topology\n");
			format_to(out, "\tx2 APIC id: {:#010x}\n", regs[edx]);
			[[fallthrough]];
		default:
			const struct
			{
				std::uint32_t shift_distance : 5;
				std::uint32_t reserved_1     : 27;
			} a = bit_cast<decltype(a)>(regs[eax]);

			const struct
			{
				std::uint32_t logical_procesors_at_level_type : 16;
				std::uint32_t reserved_1                      : 16;
			} b = bit_cast<decltype(b)>(regs[ebx]);

			const struct
			{
				std::uint32_t level_number : 8;
				std::uint32_t level_type   : 8;
				std::uint32_t reserved_1   : 16;
			} c = bit_cast<decltype(c)>(regs[ecx]);

			format_to(out, "\t\tbits to shift: {:d}\n", a.shift_distance);
			format_to(out, "\t\tlogical processors at level type: {:d}\n", b.logical_procesors_at_level_type);
			format_to(out, "\t\tlevel number: {:d}\n", c.level_number);
			switch(c.level_type) {
			case 0:
				format_to(out, "\t\tLevel type: invalid\n");
				break;
			case 1:
				format_to(out, "\t\tlevel type: SMT\n");
				break;
			case 2:
				format_to(out, "\t\tlevel type: Core\n");
				break;
			default:
				format_to(out, "\t\tlevel type: reserved {:#04x}\n", c.level_type);
				break;
			}
			format_to(out, "\n");
		}
	}
}

void print_extended_topology_v2(fmt::memory_buffer& out, const cpu_t& cpu) {
	for(const auto& sub : cpu.leaves.at(leaf_type::extended_topology_v2)) {
		const register_set_t& regs = sub.second;
		switch(sub.first) {
		case subleaf_type::main:
			format_to(out, "Extended topology v2\n");
			format_to(out, "\tx2 APIC id: {:#010x}\n", regs[edx]);
			[[fallthrough]];
		default:
			const struct
			{
				std::uint32_t shift_distance : 5;
				std::uint32_t reserved_1     : 27;
			} a = bit_cast<decltype(a)>(regs[eax]);

			const struct
			{
				std::uint32_t logical_procesors_at_level_type : 16;
				std::uint32_t reserved_1                      : 16;
			} b = bit_cast<decltype(b)>(regs[ebx]);

			const struct
			{
				std::uint32_t level_number : 8;
				std::uint32_t level_type   : 8;
				std::uint32_t reserved_1   : 16;
			} c = bit_cast<decltype(c)>(regs[ecx]);

			format_to(out, "\t\tbits to shift: {:d}\n", a.shift_distance);
			format_to(out, "\t\tlogical processors at level type: {:d}\n", b.logical_procesors_at_level_type);
			format_to(out, "\t\tlevel number: {:d}\n", c.level_number);
			switch(c.level_type) {
			case 0:
				format_to(out, "\t\tLevel type: invalid\n");
				break;
			case 1:
				format_to(out, "\t\tlevel type: SMT\n");
				break;
			case 2:
				format_to(out, "\t\tlevel type: Core\n");
				break;
			case 3:
				format_to(out, "\t\tlevel type: Module\n");
				break;
			case 4:
				format_to(out, "\t\tlevel type: Tile\n");
				break;
			case 5:
				format_to(out, "\t\tlevel type: Die\n");
				break;
			default:
				format_to(out, "\t\tlevel type: reserved {:#04x}\n", c.level_type);
				break;
			}
			format_to(out, "\n");
		}
	}
}

void enumerate_deterministic_tlb(cpu_t& cpu) {
	register_set_t regs = cpuid(leaf_type::deterministic_tlb, subleaf_type::main);
	cpu.leaves[leaf_type::deterministic_tlb][subleaf_type::main] = regs;

	const subleaf_type limit = subleaf_type{ regs[eax] };
	for(subleaf_type sub = subleaf_type{ 1 }; sub < limit; ++sub) {
		cpu.leaves[leaf_type::deterministic_tlb][sub] = cpuid(leaf_type::deterministic_tlb, sub);
	}
}

void print_deterministic_tlb(fmt::memory_buffer& out, const cpu_t& cpu) {
	for(const auto& sub : cpu.leaves.at(leaf_type::deterministic_tlb)) {
		const register_set_t& regs = sub.second;
		switch(sub.first) {
		case subleaf_type::main:
			format_to(out, "Deterministic Address Translation\n");
			format_to(out, "\tMaximum deterministic TLB cpuid leaf: {:#010x}\n", regs[eax]);
			[[fallthrough]];
		default:
			{
				const struct
				{
					std::uint32_t page_4k               : 1;
					std::uint32_t page_2m               : 1;
					std::uint32_t page_4m               : 1;
					std::uint32_t page_1g               : 1;
					std::uint32_t reserved_1            : 4;
					std::uint32_t partitioning          : 3;
					std::uint32_t reserved_2            : 5;
					std::uint32_t ways_of_associativity : 16;
				} b = bit_cast<decltype(b)>(regs[ebx]);

				const struct
				{
					std::uint32_t type                           : 5;
					std::uint32_t level                          : 3;
					std::uint32_t fully_associative              : 1;
					std::uint32_t reserved_1                     : 5;
					std::uint32_t maximum_addressable_thread_ids : 12;
					std::uint32_t reserved_2                     : 6;
				} d = bit_cast<decltype(d)>(regs[edx]);

				if(d.type == 0_u32) {
					break;
				}

				const std::uint32_t entries = b.ways_of_associativity * regs[ecx];

				const auto print_associativity = [](std::uint32_t fully_associative, std::uint32_t ways) {
					using namespace fmt::literals;
					return fully_associative ? std::string("fully") : "{:d}-way"_format(ways);
				};

				const auto print_type = [](std::uint32_t type) {
					switch(type) {
					case 0b0001:
						return "data";
					case 0b0010:
						return "instruction";
					case 0b0011:
						return "unified";
					}
					return "";
				};

				const auto print_pages = [](std::uint32_t page_4k, std::uint32_t page_2m, std::uint32_t page_4m, std::uint32_t page_1g) {
					fmt::memory_buffer out;
					if(page_4k) {
						format_to(out, "4K");
					}
					if(page_2m) {
						if(out.size()) {
							format_to(out, " | ");
						}
						format_to(out, "2M");
					}
					if(page_4m) {
						if(out.size()) {
							format_to(out, " | ");
						}
						format_to(out, "4M");
					}
					if(page_1g) {
						if(out.size()) {
							format_to(out, " | ");
						}
						format_to(out, "1G");
					}
					return to_string(out);
				};

				format_to(out, "\t{:d}-entry {:s} associative L{:d} {:s} TLB for {:s}, shared by {:d} threads\n", entries,
				                                                                                                  print_associativity(d.fully_associative, b.ways_of_associativity),
				                                                                                                  d.level,
				                                                                                                  print_type(d.type),
				                                                                                                  print_pages(b.page_4k, b.page_2m, b.page_4m, b.page_1g),
				                                                                                                  d.maximum_addressable_thread_ids + 1_u32);

				format_to(out, "\n");
			}
		}
	}
}

void print_l1_cache_tlb(fmt::memory_buffer& out, const cpu_t & cpu) {
	const register_set_t& regs = cpu.leaves.at(leaf_type::l1_cache_identifiers).at(subleaf_type::main);

	struct tlb_element
	{
		std::uint8_t entries;
		std::uint8_t associativity;
	};

	struct tlb_info
	{
		tlb_element i;
		tlb_element d;
	};

	struct cache_info
	{
		std::uint32_t line_size     : 8;
		std::uint32_t lines_per_tag : 8;
		std::uint32_t associativity : 8;
		std::uint32_t size_kb       : 8;
	};

	const tlb_info a = bit_cast<decltype(a)>(regs[eax]); // 2M page
	const tlb_info b = bit_cast<decltype(b)>(regs[ebx]); // 4K page
	const cache_info c = bit_cast<decltype(c)>(regs[ecx]); // L1d
	const cache_info d = bit_cast<decltype(d)>(regs[edx]); // L1i

	auto print_associativity = [](std::uint8_t assoc) -> std::string {
		switch(assoc) {
		case 0_u8:
			return "unknown assocativity";
		case 1_u8:
			return "direct-mapped";
		case 0xff_u8:
			return "fully associative";
		default:
			return std::to_string(gsl::narrow_cast<std::uint32_t>(assoc)) + "-way associative";
		}
	};

	const auto print_tlb = [&print_associativity](const tlb_element& tlb, const std::string& type, const std::string& page_size) {
		using namespace fmt::literals;
		return "{:d}-entry {:s} L1 {:s} TLB for {:s} pages"_format(tlb.entries, print_associativity(tlb.associativity), type, page_size);
	};

	const auto print_cache = [&print_associativity](const cache_info& cache, const std::string& type) {
		using namespace fmt::literals;
		return "{:d} Kbyte {:s} L1 {:s} cache with {:d} bytes per line and {:d} lines per tag"_format(cache.size_kb,
		                                                                                              print_associativity(cache.associativity),
		                                                                                              type,
		                                                                                              cache.line_size,
		                                                                                              cache.lines_per_tag);
	};

	format_to(out, "Level 1 TLB\n");
	format_to(out, "\t{:s}\n", print_tlb(a.d, "data", "2M"));
	format_to(out, "\t{:s}\n", print_tlb(a.i, "instruction", "2M"));
	format_to(out, "\t{:s}\n", print_tlb(b.d, "data", "4K"));
	format_to(out, "\t{:s}\n", print_tlb(b.i, "instruction", "4K"));
	format_to(out, "\n");

	format_to(out, "Level 1 cache\n");
	format_to(out, "\t{:s}\n", print_cache(c, "data"));
	format_to(out, "\t{:s}\n", print_cache(d, "instruction"));

	format_to(out, "\n");
}

struct tlb_element
{
	std::uint16_t entries       : 12;
	std::uint16_t associativity : 4;
};

struct tlb_info
{
	tlb_element i;
	tlb_element d;
};

std::string print_associativity(std::uint32_t assoc) {
	switch(assoc) {
	case 0x0_u32:
		return "disabled";
	case 0x1_u32:
		return "direct-mapped";
	case 0x2_u32:
		return "2-way associative";
	case 0x3_u32:
		return "3-way associative";
	case 0x4_u32:
		return "4-way associative";
	case 0x5_u32:
		return "6-way associative";
	case 0x6_u32:
		return "8-way associative";
	case 0x8_u32:
		return "16-way associative";
	case 0xa_u32:
		return "32-way associative";
	case 0xb_u32:
		return "48-way associative";
	case 0xc_u32:
		return "64-way associative";
	case 0xd_u32:
		return "96-way associative";
	case 0xe_u32:
		return "128-way associative";
	case 0xf_u32:
		return "fully associative";
	default:
		return std::to_string(assoc) + "-way associative";
	}
};

std::string print_tlb(const tlb_element& tlb, const std::string& type, const std::string& page_size) {
	using namespace fmt::literals;
	return "{:d}-entry {:s} L2 {:s} TLB for {:s} pages"_format(tlb.entries, print_associativity(tlb.associativity), type, page_size);
};

void print_l2_cache_tlb(fmt::memory_buffer& out, const cpu_t & cpu) {
	const register_set_t& regs = cpu.leaves.at(leaf_type::l2_cache_identifiers).at(subleaf_type::main);
	
	struct l2_cache_info
	{
		std::uint32_t line_size     : 8;
		std::uint32_t lines_per_tag : 4;
		std::uint32_t associativity : 4;
		std::uint32_t size          : 16;
	};

	struct l3_cache_info
	{
		std::uint32_t line_size     : 8;
		std::uint32_t lines_per_tag : 4;
		std::uint32_t associativity : 4;
		std::uint32_t reserved_1    : 2;
		std::uint32_t size          : 14;
	};

	const tlb_info a = bit_cast<decltype(a)>(regs[eax]); // 2M page
	const tlb_info b = bit_cast<decltype(b)>(regs[ebx]); // 4K page
	const l2_cache_info c = bit_cast<decltype(c)>(regs[ecx]);
	const l3_cache_info d = bit_cast<decltype(d)>(regs[edx]);

	const auto print_l2_size = [](std::uint32_t cache_size) {
		return print_size(cache_size * 1024_u64);
	};

	const auto print_l3_size = [](std::uint32_t cache_size) {
		return print_size(cache_size * 1024_u64 * 512_u64);
	};
	
	switch(cpu.vendor & vendor_type::any_silicon) {
	case vendor_type::amd:
		format_to(out, "Level 2 TLB\n");
		format_to(out, "\t{:s}\n", print_tlb(a.d, "data", "2M"));
		format_to(out, "\t{:s}\n", print_tlb(a.i, "instruction", "2M"));
		format_to(out, "\t{:s}\n", print_tlb(b.d, "data", "4K"));
		format_to(out, "\t{:s}\n", print_tlb(b.i, "instruction", "4K"));
		format_to(out, "\n");

		format_to(out, "Level 2 cache\n");
		format_to(out, "\t{:s} {:s} L2 cache with {:d} bytes per line and {:d} lines per tag\n", print_l2_size(c.size),
		                                                                                         print_associativity(c.associativity),
		                                                                                         c.line_size,
		                                                                                         c.lines_per_tag);

		format_to(out, "\n");

		format_to(out, "Level 3 cache\n");
		format_to(out, "\t{:s} {:s} L3 cache with {:d} bytes per line and {:d} lines per tag\n", print_l3_size(d.size),
		                                                                                         print_associativity(d.associativity),
		                                                                                         d.line_size,
		                                                                                         d.lines_per_tag);
		break;
	case vendor_type::intel:
		format_to(out, "Level 2 cache\n");
		format_to(out, "\t{:s} {:s} L2 cache with {:d} bytes per line\n", print_l2_size(c.size),
		                                                                  print_associativity(c.associativity),
		                                                                  c.line_size);
		break;
	default:
		print_generic(out, cpu, leaf_type::l2_cache_identifiers, subleaf_type::main);
		break;
	}
	format_to(out, "\n");
}

void print_1g_tlb(fmt::memory_buffer& out, const cpu_t& cpu) {
	const register_set_t& regs = cpu.leaves.at(leaf_type::tlb_1g_identifiers).at(subleaf_type::main);
	
	const tlb_info a = bit_cast<decltype(a)>(regs[eax]); // l1
	const tlb_info b = bit_cast<decltype(b)>(regs[ebx]); // l2

	format_to(out, "1GB page TLB\n");
	format_to(out, "\tLevel 1\n");
	format_to(out, "\t\t{:s}\n", print_tlb(a.d, "data", "1G"));
	format_to(out, "\t\t{:s}\n", print_tlb(a.i, "instruction", "1G"));
	format_to(out, "\tLevel 2\n");
	format_to(out, "\t\t{:s}\n", print_tlb(b.d, "data", "1G"));
	format_to(out, "\t\t{:s}\n", print_tlb(b.i, "instruction", "1G"));
	format_to(out, "\n");
}

void enumerate_cache_properties(cpu_t& cpu) {
	subleaf_type sub = subleaf_type::main;
	while(true) {
		register_set_t regs = cpuid(leaf_type::cache_properties, sub);
		if((regs[eax] & 0xf_u32) == 0) {
			break;
		}
		cpu.leaves[leaf_type::cache_properties][subleaf_type{ sub }] = regs;
		++sub;
	}
}

void print_cache_properties(fmt::memory_buffer& out, const cpu_t& cpu) {
	format_to(out, "Cache properties\n");

	for(const auto& sub : cpu.leaves.at(leaf_type::cache_properties)) {
		const register_set_t& regs = sub.second;

		const struct
		{
			std::uint32_t type                           : 5;
			std::uint32_t level                          : 3;
			std::uint32_t self_initializing              : 1;
			std::uint32_t fully_associative              : 1;
			std::uint32_t reserved_1                     : 4;
			std::uint32_t maximum_addressable_thread_ids : 12;
			std::uint32_t reserved_2                     : 6;
		} a = bit_cast<decltype(a)>(regs[eax]);

		const struct
		{
			std::uint32_t coherency_line_size      : 12;
			std::uint32_t physical_line_partitions : 10;
			std::uint32_t associativity_ways       : 10;
		} b = bit_cast<decltype(b)>(regs[ebx]);

		const struct
		{
			std::uint32_t writeback_invalidates : 1;
			std::uint32_t cache_inclusive       : 1;
			std::uint32_t reserved_1            : 30;
		} d = bit_cast<decltype(d)>(regs[edx]);

		const std::size_t sets = regs[ecx];
		const std::size_t cache_size = (b.associativity_ways       + 1_u64)
		                             * (b.coherency_line_size      + 1_u64)
		                             * (b.physical_line_partitions + 1_u64)
		                             * (sets                       + 1_u64);

		format_to(out, "\t{:s} L{:d} ", print_size(cache_size), a.level);
		switch(a.type) {
		case 1:
			format_to(out, "data");
			break;
		case 2:
			format_to(out, "instruction");
			break;
		case 3:
			format_to(out, "unified");
			break;
		}
		format_to(out, "\n");
		format_to(out, "\t\t{:d} bytes per line \u00d7 {:d} ways \u00d7 {:d} sets = {:s}.\n", b.coherency_line_size + 1_u32,
		                                                                                      b.associativity_ways  + 1_u32,
		                                                                                      sets                        + 1_u32,
		                                                                                      print_size(cache_size));
		if(a.self_initializing) {
			format_to(out, "\t\tSelf-initializing.\n");
		}
		if(a.fully_associative) {
			format_to(out, "\t\tFully associative.\n");
		} else {
			format_to(out, "\t\t{:d}-way set associative.\n", b.associativity_ways + 1_u32);
		}
		if(d.writeback_invalidates) {
			format_to(out, "\t\tWBINVD/INVD does not invalidate lower level caches for other threads.\n");
		} else {
			format_to(out, "\t\tWBINVD/INVD invalidate lower level caches for all threads.\n");
		}
		format_to(out, "\t\tCache is {:s}inclusive of lower cache levels.\n", d.cache_inclusive != 0 ? "" : "not ");
		format_to(out, "\t\tCache is shared by up to {:d} threads in the package.\n", a.maximum_addressable_thread_ids + 1);
		format_to(out, "\n");
	}
}

void print_extended_apic(fmt::memory_buffer& out, const cpu_t& cpu) {
	const register_set_t& regs = cpu.leaves.at(leaf_type::extended_apic).at(subleaf_type::main);

	const struct
	{
		std::uint32_t core_id          : 8;
		std::uint32_t threads_per_core : 8;
		std::uint32_t reserved_1       : 16;
	} b = bit_cast<decltype(b)>(regs[ebx]);

	const struct
	{
		std::uint32_t node_id             : 8;
		std::uint32_t nodes_per_processor : 3;
		std::uint32_t reserved_1          : 21;
	} c = bit_cast<decltype(c)>(regs[ecx]);

	format_to(out, "Extended APIC\n");
	format_to(out, "\tExtended APIC ID: {:#010x}\n", regs[eax]);
	format_to(out, "\tCore ID: {:#04x}\n", b.core_id);
	format_to(out, "\tThreads per core: {:d}\n", (b.threads_per_core + 1_u32));
	format_to(out, "\tNode ID: {:#04x}\n", c.node_id);
	format_to(out, "\tNodes per processor: {:d}\n", (c.nodes_per_processor + 1_u32));
	format_to(out, "\n");
}

std::string to_string(const cache_t& cache) {
	fmt::memory_buffer out;
	format_to(out, "{:s} L{:d} ", print_size(cache.total_size), cache.level);
	switch(cache.type) {
	case 1:
		format_to(out, "data");
		break;
	case 2:
		format_to(out, "instruction");
		break;
	case 3:
		format_to(out, "unified");
		break;
	}
	format_to(out, "\n");
	format_to(out, "\t\t{:d} bytes per line \u00d7 {:d} ways \u00d7 {:d} partitions \u00d7 {:d} sets = {:s}.\n", cache.line_size,
	                                                                                                             cache.ways,
	                                                                                                             cache.line_partitions,
	                                                                                                             cache.sets,
	                                                                                                             print_size(cache.total_size));
	if(cache.self_initializing) {
		format_to(out, "\t\tSelf-initializing.\n");
	}
	if(cache.fully_associative) {
		format_to(out, "\t\tFully associative.\n");
	} else {
		format_to(out, "\t\t{:d}-way set associative.\n", cache.ways);
	}
	if(cache.invalidates_lower_levels) {
		format_to(out, "\t\tWBINVD/INVD does not invalidate lower level caches for other threads.\n");
	} else {
		format_to(out, "\t\tWBINVD/INVD invalidate lower level caches for all threads.\n");
	}
	format_to(out, "\t\tCache is {:s}inclusive of lower cache levels.\n", cache.inclusive ? "" : "not ");
	format_to(out, "\t\tCache is {:s}direct mapped.\n", cache.direct_mapped ? "not " : "");
	format_to(out, "\t\tCache is shared by up to {:d} threads.\n", cache.sharing_mask);
	format_to(out, "\n");
	return to_string(out);
}

std::string to_short_string(const cache_t& cache) {
	fmt::memory_buffer out;
	format_to(out, "{:s} L{:d} ", print_size(cache.total_size), cache.level);
	switch(cache.type) {
	case 1:
		format_to(out, "data       ");
		break;
	case 2:
		format_to(out, "instruction");
		break;
	case 3:
		format_to(out, "unified    ");
		break;
	}
	if(cache.fully_associative) {
		format_to(out, " fully associative     ");
	} else {
		format_to(out, " {:>2d}-way set associative", cache.ways);
	}
	format_to(out, " with {:>5d} sets, {:d} bytes per line", cache.sets, cache.line_size);
	return to_string(out);
}

constexpr full_apic_id_t split_apic_id(std::uint32_t id, std::uint32_t smt_mask_width, std::uint32_t core_mask_width) noexcept {
	//
	//                                                   |<         >| core_mask_width = 7
	//                                                           |< >| smt_mask_width = 3
	//   3                   2                   1                   0
	// 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
	//                                                           |< >| smt_id
	//                                                   |<   >|       core_id
	// |<                                             >|               package_id
	// |<                                                           >| x2_apic_id
	const std::uint32_t smt_select_mask     = ~(0xffff'ffff_u32 << smt_mask_width);
	const std::uint32_t core_select_mask    = ~(0xffff'ffff_u32 << core_mask_width);
	const std::uint32_t package_select_mask =   0xffff'ffff_u32 << core_mask_width;
	const std::uint32_t smt_id     =  id & smt_select_mask;
	const std::uint32_t core_id    = (id & core_select_mask   ) >> smt_mask_width;
	const std::uint32_t package_id = (id & package_select_mask) >> core_mask_width;
	return { smt_id, core_id, 0_u32, 0_u32, 0_u32, package_id };
}

constexpr full_apic_id_t split_apic_id(std::uint32_t id,
                                       std::uint32_t smt_mask_width,
                                       std::uint32_t core_mask_width,
                                       std::uint32_t module_mask_width,
                                       std::uint32_t tile_mask_width,
                                       std::uint32_t die_mask_width) noexcept {
	//
	//
	//                                 |<                           >| die_mask_width = 16
	//                                         |<                   >| tile_mask_width = 12
	//                                             |<               >| module_mask_width = 10
	//                                                         |<   >| core_mask_width = 4
	//                                                               X smt_mask_width = 1
	//   3                   2                   1                   0
	// 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
	//                                                               X smt_id
	//                                                         |< >|   core_id
	// |<                                          |<       >|         module_id
	// |<                                      |X|                     tile_id
	// |<                              |<   >|                         die_id
	// |<                           >|                                 package_id
	// |<                                                           >| x2_apic_id
	const std::uint32_t smt_select_mask     = ~(0xffff'ffff_u32 << smt_mask_width);
	const std::uint32_t core_select_mask    = ~(0xffff'ffff_u32 << core_mask_width);
	const std::uint32_t module_select_mask  = ~(0xffff'ffff_u32 << module_mask_width);
	const std::uint32_t tile_select_mask    = ~(0xffff'ffff_u32 << tile_mask_width);
	const std::uint32_t die_select_mask     = ~(0xffff'ffff_u32 << die_mask_width);
	const std::uint32_t package_select_mask =   0xffff'ffff_u32 << die_mask_width;

	const std::uint32_t smt_id     = (id & smt_select_mask    );
	const std::uint32_t core_id    = (id & core_select_mask   ) >> smt_mask_width;
	const std::uint32_t module_id  = (id & module_select_mask ) >> core_mask_width;
	const std::uint32_t tile_id    = (id & tile_select_mask   ) >> module_mask_width;
	const std::uint32_t die_id     = (id & die_select_mask    ) >> tile_mask_width;
	const std::uint32_t package_id = (id & package_select_mask) >> die_mask_width;

	return { smt_id, core_id, module_id, tile_id, die_id, package_id };
}

std::pair<std::uint32_t, std::uint32_t> generate_mask(std::uint32_t entries) noexcept {
	if(entries > 0x7fff'ffff_u32) {
		return std::make_pair(0xffff'ffff_u32, 32_u32);
	}
	entries *= 2;
	entries -= 1;
	unsigned long idx = 0;
	bit_scan_reverse(&idx, entries);
	idx += 1;
	return std::make_pair((1_u32 << idx) - 1_u32, idx);
}

system_t build_topology(const std::map<std::uint32_t, cpu_t>& logical_cpus) {
	system_t machine = {};
	bool enumerated_caches = false;
	std::for_each(std::begin(logical_cpus), std::end(logical_cpus), [&machine, &enumerated_caches](const std::pair<std::uint32_t, cpu_t>& p) {
		const cpu_t cpu = p.second;
		machine.x2_apic_ids.push_back(cpu.apic_id);
		if(enumerated_caches) {
			return;
		}
		enumerated_caches = true;
		machine.vendor = cpu.vendor;
		switch(cpu.vendor & vendor_type::any_silicon) {
		case vendor_type::intel:
			if(cpu.leaves.find(leaf_type::extended_topology) != cpu.leaves.end()) {
				for(const auto& sub : cpu.leaves.at(leaf_type::extended_topology)) {
					const register_set_t& regs = sub.second;

					const struct
					{
						std::uint32_t shift_distance : 5;
						std::uint32_t reserved_1     : 27;
					} a = bit_cast<decltype(a)>(regs[eax]);

					const struct
					{
						std::uint32_t level_number : 8;
						std::uint32_t level_type   : 8;
						std::uint32_t reserved_1   : 16;
					} c = bit_cast<decltype(c)>(regs[ecx]);

					switch(c.level_type) {
					case 1:
						if(machine.smt_mask_width == 0_u32) {
							machine.smt_mask_width = a.shift_distance;
						}
						break;
					case 2:
						if(machine.core_mask_width == 0_u32) {
							machine.core_mask_width = a.shift_distance;
						}
						break;
					default:
						break;
					}
				}
			}
			if(cpu.leaves.find(leaf_type::deterministic_cache) != cpu.leaves.end()) {
				for(const auto& sub : cpu.leaves.at(leaf_type::deterministic_cache)) {
					const register_set_t& regs = sub.second;

					const struct
					{
						std::uint32_t type                           : 5;
						std::uint32_t level                          : 3;
						std::uint32_t self_initializing              : 1;
						std::uint32_t fully_associative              : 1;
						std::uint32_t reserved_1                     : 4;
						std::uint32_t maximum_addressable_thread_ids : 12;
						std::uint32_t maximum_addressable_core_ids   : 6;
					} a = bit_cast<decltype(a)>(regs[eax]);

					const struct
					{
						std::uint32_t coherency_line_size      : 12;
						std::uint32_t physical_line_partitions : 10;
						std::uint32_t associativity_ways       : 10;
					} b = bit_cast<decltype(b)>(regs[ebx]);

					const struct
					{
						std::uint32_t writeback_invalidates : 1;
						std::uint32_t cache_inclusive       : 1;
						std::uint32_t complex_indexing      : 1;
						std::uint32_t reserved_1            : 29;
					} d = bit_cast<decltype(d)>(regs[edx]);

					switch(sub.first) {
					case subleaf_type::main:
						if(machine.smt_mask_width == 0_u32) {
							const id_info_t leaf_1_b = bit_cast<decltype(leaf_1_b)>(cpu.leaves.at(leaf_type::version_info).at(subleaf_type::main).at(ebx));

							const std::uint32_t total_possible_cores = leaf_1_b.maximum_addressable_ids;
							const std::uint32_t total_cores_in_package = a.maximum_addressable_core_ids + 1_u32;
							const std::uint32_t logical_cores_per_physical_core = total_possible_cores / total_cores_in_package;
							
							const auto logical_mask = generate_mask(logical_cores_per_physical_core);
							machine.smt_mask_width = logical_mask.second;
							const auto physical_mask = generate_mask(total_cores_in_package);
							machine.core_mask_width = physical_mask.second;
						}

						[[fallthrough]];
					default:
						const std::uint32_t sets = regs[ecx];
						const std::uint32_t cache_size = (b.associativity_ways       + 1_u32)
						                               * (b.physical_line_partitions + 1_u32)
						                               * (b.coherency_line_size      + 1_u32)
						                               * (sets                       + 1_u32);

						const cache_t cache = {
							a.level,
							a.type,
							b.associativity_ways + 1_u32,
							sets + 1_u32,
							b.coherency_line_size + 1_u32,
							b.physical_line_partitions + 1_u32,
							cache_size,
							a.fully_associative != 0_u32,
							b.associativity_ways == 0_u32,
							d.complex_indexing == 1_u32,
							a.self_initializing != 0_u32,
							d.writeback_invalidates != 0_u32,
							d.cache_inclusive != 0_u32,
							a.maximum_addressable_thread_ids
						};
						machine.all_caches.push_back(cache);
						break;
					}
				}
			} else if(cpu.leaves.find(leaf_type::cache_and_tlb) != cpu.leaves.end()) {
				machine.smt_mask_width = 0_u32;
				machine.core_mask_width = 0_u32;
				
				decomposed_cache_t decomposed = decompose_cache_descriptors(cpu, cpu.leaves.at(leaf_type::cache_and_tlb).at(subleaf_type::main));
				std::vector<gsl::not_null<const cache_descriptor_t*>> combined;
				combined.reserve(decomposed.cache_descriptors.size() + decomposed.other_descriptors.size());
				combined.insert(combined.end(), decomposed.other_descriptors.begin(), decomposed.other_descriptors.end());
				combined.insert(combined.end(), decomposed.cache_descriptors.begin(), decomposed.cache_descriptors.end());

				for(const auto desc : combined) {
					const std::uint32_t level = desc->level == level_3 ? 3_u32
					                          : desc->level == level_2 ? 2_u32
					                          :                          1_u32;
					
					const std::uint32_t type = desc->type == data         ? 1_u32
					                         : desc->type == instructions ? 2_u32
					                         : desc->type == trace        ? 2_u32
					                         :                              3_u32;
					
					const std::uint32_t ways = desc->associativity == direct_mapped     ? 1_u32
					                         : desc->associativity == fully_associative ? 0xff_u32
					                         :                                            static_cast<std::uint32_t>(desc->associativity);

					const std::uint32_t sets = desc->size / (ways * desc->line_size);

					const cache_t cache = {
						level,
						type,
						ways,
						sets,
						desc->line_size,
						1_u32,
						desc->size,
						desc->associativity == fully_associative,
						desc->associativity == direct_mapped,
						false,
						true,
						true,
						true,
						0_u32
					};
					machine.all_caches.push_back(cache);
				}
			}
			break;
		case vendor_type::amd:
			if(cpu.leaves.find(leaf_type::extended_apic) != cpu.leaves.end()) {
				const register_set_t& regs = cpu.leaves.at(leaf_type::extended_apic).at(subleaf_type::main);
				const struct
				{
					std::uint32_t core_id          : 8;
					std::uint32_t threads_per_core : 8;
					std::uint32_t reserved_1       : 16;
				} b = bit_cast<decltype(b)>(regs[ebx]);

				//// the node_id and nodes_per_processor should surely be mapped somehow...
				//const struct
				//{
				//	std::uint32_t node_id             : 8;
				//	std::uint32_t nodes_per_processor : 3;
				//	std::uint32_t reserved_1          : 21;
				//} c = bit_cast<decltype(c)>(regs[ecx]);

				machine.smt_mask_width = generate_mask(b.threads_per_core).second;

			}
			if(cpu.leaves.find(leaf_type::cache_properties) != cpu.leaves.end()) {
				for(const auto& sub : cpu.leaves.at(leaf_type::cache_properties)) {
					const register_set_t& regs = sub.second;
					const struct
					{
						std::uint32_t type                           : 5;
						std::uint32_t level                          : 3;
						std::uint32_t self_initializing              : 1;
						std::uint32_t fully_associative              : 1;
						std::uint32_t reserved_1                     : 4;
						std::uint32_t maximum_addressable_thread_ids : 12;
						std::uint32_t reserved_2                     : 6;
					} a = bit_cast<decltype(a)>(regs[eax]);

					const struct
					{
						std::uint32_t coherency_line_size      : 12;
						std::uint32_t physical_line_partitions : 10;
						std::uint32_t associativity_ways       : 10;
					} b = bit_cast<decltype(b)>(regs[ebx]);

					const struct
					{
						std::uint32_t writeback_invalidates : 1;
						std::uint32_t cache_inclusive       : 1;
						std::uint32_t reserved_1            : 30;
					} d = bit_cast<decltype(d)>(regs[edx]);

					const std::uint32_t sets = regs[ecx];
					const std::uint32_t cache_size = (b.associativity_ways       + 1_u32)
					                               * (b.physical_line_partitions + 1_u32)
					                               * (b.coherency_line_size      + 1_u32)
					                               * (sets                       + 1_u32);

					const cache_t cache = {
						a.level,
						a.type,
						b.associativity_ways + 1_u32,
						sets + 1_u32,
						b.coherency_line_size + 1_u32,
						b.physical_line_partitions + 1_u32,
						cache_size,
						a.fully_associative != 0,
						b.associativity_ways == 0_u32,
						false,
						a.self_initializing != 0,
						d.writeback_invalidates != 0,
						d.cache_inclusive != 0,
						a.maximum_addressable_thread_ids
					};
					machine.all_caches.push_back(cache);
				}
			}
			if(cpu.leaves.find(leaf_type::address_limits) != cpu.leaves.end()) {
				const register_set_t& regs = cpu.leaves.at(leaf_type::address_limits).at(subleaf_type::main);
				const struct
				{
					std::uint32_t package_threads : 8;
					std::uint32_t reserved_1      : 4;
					std::uint32_t apic_id_size    : 4;
					std::uint32_t perf_tsc_size   : 2;
					std::uint32_t reserved_2      : 14;
				} c = bit_cast<decltype(c)>(regs[ecx]);

				machine.core_mask_width = c.apic_id_size;
			}
			break;
		default:
			break;
		}
	});

	switch(machine.vendor & vendor_type::any_silicon) {
	case vendor_type::intel:
		// per the utterly miserable source code at https://software.intel.com/en-us/articles/intel-64-architecture-processor-topology-enumeration
		for(const std::uint32_t id : machine.x2_apic_ids) {
			const full_apic_id_t split = split_apic_id(id, machine.smt_mask_width, machine.core_mask_width);
		
			logical_core_t core = { id, split.smt_id, split.core_id, split.package_id };

			for(const cache_t& cache : machine.all_caches) {
				core.shared_cache_ids.push_back(id & cache.sharing_mask);
				core.non_shared_cache_ids.push_back(id & ~cache.sharing_mask);
			}

			machine.all_cores.push_back(core);
			machine.packages[split.package_id].physical_cores[split.core_id].logical_cores[split.smt_id] = core;
		}
		for(std::size_t i = 0; i < machine.all_caches.size(); ++i) {
			cache_t& cache = machine.all_caches[i];
			for(const logical_core_t& core : machine.all_cores) {
				cache.instances[core.non_shared_cache_ids[i]].sharing_ids.push_back(core.full_apic_id);
			}
		}
		break;
	case vendor_type::amd:
		// pure guesswork, since AMD does not appear to document its algorithm anywhere
		for(const std::uint32_t id : machine.x2_apic_ids) {
			const full_apic_id_t split = split_apic_id(id, machine.smt_mask_width, machine.core_mask_width);
		
			logical_core_t core = { id, split.smt_id, split.core_id, split.package_id };
			machine.all_cores.push_back(core);
			machine.packages[split.package_id].physical_cores[split.core_id].logical_cores[split.smt_id] = core;
		}
		for(std::size_t i = 0; i < machine.all_caches.size(); ++i) {
			cache_t& cache = machine.all_caches[i];
			for(std::size_t j = 0; j < machine.all_cores.size(); ++j) {
				cache.instances[gsl::narrow<std::uint32_t>(j) / (cache.sharing_mask + 1_u32)].sharing_ids.push_back(machine.all_cores[j].full_apic_id);
			}
		}
		break;
	default:
		break;
	}

	return machine;
}

void print_topology(fmt::memory_buffer& out, const system_t& machine) {
	const std::uint32_t total_addressable_cores = gsl::narrow_cast<std::uint32_t>(machine.all_cores.size());

	std::multimap<std::uint32_t, std::string> cache_output;
	for(const cache_t& cache : machine.all_caches) {
		std::uint32_t cores_covered = 0_u32;
		for(const auto& instance : cache.instances) {
			std::string line;
			for(std::uint32_t i = 0; i < cores_covered; ++i) {
				line += "-";
			}
			for(std::uint32_t i = 0; i < instance.second.sharing_ids.size(); ++i) {
				line += "*";
				++cores_covered;
			}
			for(std::uint32_t i = cores_covered; i < total_addressable_cores; ++i) {
				line += "-";
			}
			line += " ";
			line += to_short_string(cache);
			cache_output.insert(std::make_pair(cores_covered, line));
		}
	}

	for(const auto& p : cache_output) {
		format_to(out, "{:s}\n", p.second);
	}
	format_to(out, "\n");

	std::uint32_t cores_covered_package  = 0_u32;
	std::uint32_t cores_covered_physical = 0_u32;
	std::uint32_t cores_covered_logical  = 0_u32;
	for(const auto& package : machine.packages) {
		std::uint32_t logical_per_package = 0_u32;
		for(const auto& physical : package.second.physical_cores) {
			std::uint32_t logical_per_physical = 0_u32;
			for(const auto& logical : physical.second.logical_cores) {
				for(std::uint32_t i = 0; i < cores_covered_logical; ++i) {
					format_to(out, "-");
				}
				for(std::uint32_t i = 0; i < 1; ++i) {
					format_to(out, "*");
					++cores_covered_logical;
					++logical_per_physical;
					++logical_per_package;
				}
				for(std::uint32_t i = cores_covered_logical; i < total_addressable_cores; ++i) {
					format_to(out, "-");
				}
				format_to(out, " logical  {:d}:{:d}:{:d} apic id: {:#04x}\n", package.first, physical.first, logical.first, logical.second.full_apic_id);
			}
			for(std::uint32_t i = 0; i < cores_covered_physical; ++i) {
				format_to(out, "-");
			}
			for(std::uint32_t i = 0; i < logical_per_physical; ++i) {
				format_to(out, "*");
				++cores_covered_physical;
			}
			for(std::uint32_t i = cores_covered_physical; i < total_addressable_cores; ++i) {
				format_to(out, "-");
			}
			format_to(out, " physical {:d}:{:d}\n", package.first, physical.first);
		}
		for(std::uint32_t i = 0; i < cores_covered_package; ++i) {
			format_to(out, "-");
		}
		for(std::uint32_t i = 0; i < logical_per_package; ++i) {
			format_to(out, "*");
			++cores_covered_package;
		}
		for(std::uint32_t i = cores_covered_package; i < total_addressable_cores; ++i) {
			format_to(out, "-");
		}
		format_to(out, " package  {:d}\n", package.first);
	}
}

}

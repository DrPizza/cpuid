#include "stdafx.h"

#include "cache-and-topology.hpp"

#include "utility.hpp"

#include <iostream>
#include <iomanip>
#include <thread>

#include <fmt/format.h>

std::string print_size(std::size_t cache_bytes) {
	using namespace fmt::literals;

	double printable_cache_size = cache_bytes / 1024.0;
	char   cache_scale = 'K';
	if(printable_cache_size > 1'024.0) {
		printable_cache_size /= 1'024.0;
		cache_scale = 'M';
	}
	return "{:<3g} {:c}B"_format(printable_cache_size, cache_scale);
}

enum cache_type_t : std::uint8_t
{
	data_tlb        = 0x01ui8,
	instruction_tlb = 0x02ui8,
	unified_tlb     = 0x04ui8,
	all_tlb         = 0x07ui8,
	data            = 0x10ui8,
	instructions    = 0x20ui8,
	unified         = 0x40ui8,
	all_cache       = 0x70ui8,
	trace           = 0x80ui8,
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
		__assume(0);
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
		__assume(0);
	}
}

enum cache_attributes_t : std::uint8_t
{
	no_attributes      = 0x00ui8,
	pages_4k           = 0x01ui8,
	pages_2m           = 0x02ui8,
	pages_4m           = 0x04ui8,
	pages_1g           = 0x08ui8,
	all_page_sizes     = 0x0fui8,
	sectored_two_lines = 0x10ui8,
};

constexpr cache_attributes_t operator|(const cache_attributes_t& lhs, const cache_attributes_t& rhs) noexcept {
	return static_cast<cache_attributes_t>(static_cast<std::uint32_t>(lhs) | static_cast<std::uint32_t>(rhs));
}

std::string to_string(cache_attributes_t attrs) {
	fmt::MemoryWriter m;
	const std::uint8_t page = gsl::narrow_cast<std::uint8_t>(attrs & all_page_sizes);
	std::uint8_t mask = 1ui8;
	bool needs_separator = false;
	do {
		switch(page & mask) {
		case 0:
			m <<  "";
			break;
		case pages_4k:
			if(needs_separator) {
				m << " | ";
			}
			m <<  "4 KByte pages";
			needs_separator = true;
			break;
		case pages_2m:
			if(needs_separator) {
				m << " | ";
			}
			m << "2 MByte pages";
			needs_separator = true;
			break;
		case pages_4m:
			if(needs_separator) {
				m << " | ";
			}
			m << "4 MByte pages";
			needs_separator = true;
			break;
		case pages_1g:
			if(needs_separator) {
				m << " | ";
			}
			m << "1 GByte pages";
			needs_separator = true;
			break;
		}
		mask <<= 1ui8;
	} while(mask & all_page_sizes);
	return m.str();
}

enum cache_associativity_t : std::uint8_t 
{
	unknown_associativity,
	direct_mapped,
	fully_associative = 0xffui8
};

std::string to_string(cache_associativity_t assoc) {
	using namespace fmt::literals;

	switch(assoc) {
	case 0:
		return "unknown associativity";
	case 1:
		return "direct mapped";
	case 0xffui8:
		return "fully associative";
	default:
		return "{0:d}-way set associative"_format(assoc);
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
	fmt::MemoryWriter w;
	w << to_string(desc.level);
	if(w.size() > 0) {
		w << " ";
	}
	w << to_string(desc.type) << ": ";
	if(desc.type & all_cache) {
		w << print_size(desc.size);
	} else if(desc.type & all_tlb) {
		w << to_string(desc.attributes);
	} else if(desc.type & trace) {
		w << (desc.size / 1'024) << " K-\u00b5op";
	}
	w << ", ";
	w << to_string(desc.associativity);
	if(desc.type & all_cache) {
		w << ", ";
		w << desc.line_size << " byte line size";
		if(desc.attributes & sectored_two_lines) {
			w << ", 2 lines per sector";
		}
	} else if(desc.type & all_tlb) {
		w << ", ";
		w << desc.entries << " entries";
	}
	return w.str();
}

void print_cache_tlb_info(const cpu_t& cpu) {
	using namespace fmt::literals;

	const register_set_t& regs = cpu.leaves.at(leaf_t::cache_and_tlb).at(subleaf_t::main);

	if((regs[eax] & 0xff) != 0x01) {
		return;
	}
	fmt::MemoryWriter w;

	const auto bytes = gsl::as_bytes(gsl::make_span(regs));

	std::vector<gsl::not_null<const cache_descriptor_t*>> tlb_descriptors;
	std::vector<gsl::not_null<const cache_descriptor_t*>> cache_descriptors;
	std::vector<gsl::not_null<const cache_descriptor_t*>> other_descriptors;
	std::vector<std::string>                              non_conformant_descriptors;

	std::ptrdiff_t idx = 0;
	for(register_t r = eax; r <= edx; ++r, idx += sizeof(std::uint32_t)) {
		if(regs[r] & 0x8000'0000ui32) {
			continue;
		}
		for(std::ptrdiff_t j = 0; j < sizeof(std::uint32_t); ++j) {
			const std::uint8_t value = gsl::to_integer<std::uint8_t>(bytes[idx + j]);
			switch(value) {
			case 0x00ui8:
				break;
			case 0x40ui8:
				non_conformant_descriptors.push_back("No 2nd-level cache or, if processor contains a valid 2nd-level cache, no 3rd-level cache");
				break;
			case 0x49ui8:
				{
					const auto it = dual_cache_descriptors.find(value);
					if(cpu.model.family == 0x0f && cpu.model.model == 0x06) {
						cache_descriptors.push_back(&(it->second.first));
					} else {
						cache_descriptors.push_back(&(it->second.second));
					}
				}
				break;
			case 0xf0ui8:
				non_conformant_descriptors.push_back("64-Byte prefetching");
				break;
			case 0xf1ui8:
				non_conformant_descriptors.push_back("128-Byte prefetching");
				break;
			case 0xfeui8:
			case 0xffui8:
				break;
			default:
				{
					const auto dual = dual_cache_descriptors.find(value);
					if(dual != dual_cache_descriptors.end()) {
						tlb_descriptors.push_back(&(dual->second.first));
						tlb_descriptors.push_back(&(dual->second.second));
						break;
					}
					const auto it = standard_cache_descriptors.find(value);
					if(it != standard_cache_descriptors.end()) {
						if(it->second.type & all_tlb) {
							tlb_descriptors.push_back(&(it->second));
						} else if(it->second.type & all_cache) {
							cache_descriptors.push_back(&(it->second));
						} else {
							other_descriptors.push_back(&(it->second));
						}
						break;
					}
					non_conformant_descriptors.push_back("Unknown cache type: {:#2x}"_format(value));
				}
				break;
			}
		}
	}

	auto cmp = [](const cache_descriptor_t* lhs, const cache_descriptor_t* rhs) {
		return lhs->type  != rhs->type  ? lhs->type    < rhs->type
		     : lhs->level != rhs->level ? lhs->level   < rhs->level
		     : lhs->size  != rhs->size  ? lhs->size    > rhs->size     // sic; I want bigger caches at a given level listed first
		     :                            lhs->entries > rhs->entries; // sic
	};

	std::sort(std::begin(tlb_descriptors  ), std::end(tlb_descriptors  ), cmp);
	std::sort(std::begin(cache_descriptors), std::end(cache_descriptors), cmp);
	std::sort(std::begin(other_descriptors), std::end(other_descriptors), cmp);

	w.write("Cache and TLB\n");
	for(const auto d : tlb_descriptors) {
		w.write("\t{:s}\n", to_string(*d));
	}
	for(const auto d : cache_descriptors) {
		w.write("\t{:s}\n", to_string(*d));
	}
	for(const auto d : other_descriptors) {
		w.write("\t{:s}\n", to_string(*d));
	}
	for(const std::string& s : non_conformant_descriptors) {
		w.write("\t{:s}\n", s);
	}
	w.write("\n");
	std::cout << w.str() << std::flush;
}

void enumerate_deterministic_cache(cpu_t& cpu) {
	register_set_t regs = { 0 };
	subleaf_t sub = subleaf_t::main;
	while(true) {
		cpuid(regs, leaf_t::deterministic_cache, sub);
		if((regs[eax] & 0x1fui32) == 0) {
			break;
		}
		cpu.leaves[leaf_t::deterministic_cache][subleaf_t{ sub }] = regs;
		++sub;
	}
}

void print_deterministic_cache(const cpu_t& cpu) {
	using namespace fmt::literals;
	fmt::MemoryWriter w;

	w.write("Deterministic cache\n");

	for(const auto& sub : cpu.leaves.at(leaf_t::deterministic_cache)) {
		const register_set_t& regs = sub.second;

		const union
		{
			std::uint32_t full;
			struct
			{
				std::uint32_t type                           : 5;
				std::uint32_t level                          : 3;
				std::uint32_t self_initializing              : 1;
				std::uint32_t fully_associative              : 1;
				std::uint32_t reserved_1                     : 4;
				std::uint32_t maximum_addressable_thread_ids : 12;
				std::uint32_t maximum_addressable_core_ids   : 6;
			} split;
		} a = { regs[eax] };

		const union
		{
			std::uint32_t full;
			struct
			{
				std::uint32_t coherency_line_size      : 12;
				std::uint32_t physical_line_partitions : 10;
				std::uint32_t associativity_ways       : 10;
			} split;
		} b = { regs[ebx] };

		const union
		{
			std::uint32_t full;
			struct
			{
				std::uint32_t writeback_invalidates : 1;
				std::uint32_t cache_inclusive       : 1;
				std::uint32_t complex_indexing      : 1;
				std::uint32_t reserved_1            : 29;
			} split;
		} d = { regs[edx] };
		
		const std::size_t sets = regs[ecx];
		const std::size_t cache_size = (b.split.associativity_ways       + 1ui32)
		                             * (b.split.physical_line_partitions + 1ui32)
		                             * (b.split.coherency_line_size      + 1ui32)
		                             * (sets                             + 1ui32);

		w.write("\t{:s} L{:d} ", print_size(cache_size), a.split.level);
		switch(a.split.type) {
		case 1:
			w.write("data");
			break;
		case 2:
			w.write("instruction");
			break;
		case 3:
			w.write("unified");
			break;
		}
		w.write("\n");
		w.write("\t\t{:d} bytes per line \u00d7 {:d} ways \u00d7 {:d} partitions \u00d7 {:d} sets = {:s}.\n", b.split.coherency_line_size      + 1ui32,
		                                                                                                      b.split.associativity_ways       + 1ui32,
		                                                                                                      b.split.physical_line_partitions + 1ui32,
		                                                                                                      sets                             + 1ui32,
		                                                                                                      print_size(cache_size));
		if(a.split.self_initializing) {
			w.write("\t\tSelf-initializing.\n");
		}
		if(a.split.fully_associative) {
			w.write("\t\tFully associative.\n");
		} else {
			w.write("\t\t{:d}-way set associative.\n", b.split.associativity_ways + 1ui32);
		}
		if(d.split.writeback_invalidates) {
			w.write("\t\tWBINVD/INVD does not invalidate lower level caches for other threads.\n");
		} else {
			w.write("\t\tWBINVD/INVD invalidates lower level caches for all threads.\n");
		}
		w.write("\t\tCache is {:s}inclusive of lower cache levels.\n", d.split.cache_inclusive != 0 ? "" : "not ");
		w.write("\t\tCache is {:s}direct mapped.\n", d.split.complex_indexing != 0 ? "not " : "");
		w.write("\t\tCache is shared by up to {:d} threads, with up to {:d} cores in the package.\n", a.split.maximum_addressable_thread_ids + 1, a.split.maximum_addressable_core_ids + 1);
		w.write("\n");
	}
	std::cout << w.str() << std::flush;
}

void enumerate_extended_topology(cpu_t& cpu) {
	register_set_t regs = { 0 };
	cpuid(regs, leaf_t::extended_topology, subleaf_t::main);
	cpu.leaves[leaf_t::extended_topology][subleaf_t::main] = regs;
	for(subleaf_t sub = subleaf_t{ 1 }; ; ++sub) {
		cpuid(regs, leaf_t::extended_topology, sub);
		if((regs[ecx] & 0x0000'ff00ui32) == 0ui32) {
			break;
		}
		cpu.leaves[leaf_t::extended_topology][sub] = regs;
	}
}

void print_extended_topology(const cpu_t& cpu) {
	fmt::MemoryWriter w;
	
	for(const auto& sub : cpu.leaves.at(leaf_t::extended_topology)) {
		const register_set_t& regs = sub.second;
		switch(sub.first) {
		case subleaf_t::main:
			w.write("Extended topology\n");
			w.write("\tx2 APIC id: {:#010x}\n", regs[edx]);
			[[fallthrough]];
		default:
			const union
			{
				std::uint32_t full;
				struct
				{
					std::uint32_t shift_distance : 5;
					std::uint32_t reserved_1     : 27;
				} split;
			} a = { regs[eax] };

			const union
			{
				std::uint32_t full;
				struct
				{
					std::uint32_t logical_procesors_at_level_type : 16;
					std::uint32_t reserved_1                      : 16;
				} split;
			} b = { regs[ebx] };

			const union
			{
				std::uint32_t full;
				struct
				{
					std::uint32_t level_number : 8;
					std::uint32_t level_type   : 8;
					std::uint32_t reserved_1   : 16;
				} split;
			} c = { regs[ecx] };

			w.write("\t\tbits to shift: {:d}\n", a.split.shift_distance);
			w.write("\t\tlogical processors at level type: {:d}\n", b.split.logical_procesors_at_level_type);
			w.write("\t\tlevel number: {:d}\n", c.split.level_number);
			switch(c.split.level_type) {
			case 0:
				w.write("\t\tLevel type: invalid\n");
				break;
			case 1:
				w.write("\t\tlevel type: SMT\n");
				break;
			case 2:
				w.write("\t\tlevel type: Core\n");
				break;
			default:
				w.write("\t\tlevel type: reserved {:#04x}\n", c.split.level_type);
				break;
			}
			w.write("\n");
		}
	}
	std::cout << w.str() << std::flush;
}

void enumerate_deterministic_tlb(cpu_t& cpu) {
	register_set_t regs = { 0 };
	cpuid(regs, leaf_t::deterministic_tlb, subleaf_t::main);
	cpu.leaves[leaf_t::deterministic_tlb][subleaf_t::main] = regs;

	const subleaf_t limit = subleaf_t{ regs[eax] };
	for(subleaf_t sub = subleaf_t{ 1 }; sub < limit; ++sub) {
		cpuid(regs, leaf_t::deterministic_tlb, sub);
		cpu.leaves[leaf_t::deterministic_tlb][sub] = regs;
	}
}

void print_deterministic_tlb(const cpu_t& cpu) {
	using namespace fmt::literals;
	fmt::MemoryWriter w;

	for(const auto& sub : cpu.leaves.at(leaf_t::deterministic_tlb)) {
		const register_set_t& regs = sub.second;
		switch(sub.first) {
		case subleaf_t::main:
			w.write("Deterministic Address Translation\n");
			[[fallthrough]];
		default:
			{
				const union
				{
					std::uint32_t full;
					struct
					{
						std::uint32_t page_4k               : 1;
						std::uint32_t page_2m               : 1;
						std::uint32_t page_4m               : 1;
						std::uint32_t page_1g               : 1;
						std::uint32_t reserved_1            : 4;
						std::uint32_t partitioning          : 3;
						std::uint32_t reserved_2            : 5;
						std::uint32_t ways_of_associativity : 16;
					} split;
				} b = { regs[ebx] };

				const union
				{
					std::uint32_t full;
					struct
					{
						std::uint32_t type                           : 5;
						std::uint32_t level                          : 3;
						std::uint32_t fully_associative              : 1;
						std::uint32_t reserved_1                     : 5;
						std::uint32_t maximum_addressable_thread_ids : 12;
						std::uint32_t reserved_2                     : 6;
					} split;
				} d = { regs[edx] };

				if(d.split.type == 0ui32) {
					break;
				}

				const std::uint32_t entries = b.split.ways_of_associativity * regs[ecx];

				auto print_associativity = [](std::uint32_t fully_associative, std::uint32_t ways) {
					return fully_associative ? std::string("fully") : "{:d}-way"_format(ways);
				};

				auto print_type = [](std::uint32_t type) {
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

				auto print_pages = [](std::uint32_t page_4k, std::uint32_t page_2m, std::uint32_t page_4m, std::uint32_t page_1g) {
					fmt::MemoryWriter w;
					if(page_4k) {
						w << "4K";
					}
					if(page_2m) {
						if(w.size()) {
							w << " | ";
						}
						w << "2M";
					}
					if(page_4m) {
						if(w.size()) {
							w << " | ";
						}
						w << "4M";
					}
					if(page_1g) {
						if(w.size()) {
							w << " | ";
						}
						w << "1G";
					}
					return w.str();
				};

				w.write("\t{:d}-entry {:s} associative L{:d} {:s} TLB for {:s}, shared by {:d} threads\n", entries,
				                                                                                           print_associativity(d.split.fully_associative, b.split.ways_of_associativity),
				                                                                                           d.split.level,
				                                                                                           print_type(d.split.type),
				                                                                                           print_pages(b.split.page_4k, b.split.page_2m, b.split.page_4m, b.split.page_1g),
				                                                                                           d.split.maximum_addressable_thread_ids + 1ui32);

				w.write("\n");
			}
		}
	}
	std::cout << w.str() << std::flush;
}

void print_l1_cache_tlb(const cpu_t & cpu) {
	const register_set_t& regs = cpu.leaves.at(leaf_t::l1_cache_identifiers).at(subleaf_t::main);
	fmt::MemoryWriter w;

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

	const union
	{
		std::uint32_t full;
		tlb_info split; // 2M page
	} a = { regs[eax] };

	const union
	{
		std::uint32_t full;
		tlb_info split; // 4K page

	} b = { regs[ebx] };

	const union
	{
		std::uint32_t full;
		cache_info split; // L1d
	} c = { regs[ecx] };

	const union
	{
		std::uint32_t full;
		cache_info split; // L1i
	} d = { regs[edx] };

	auto print_associativity = [](std::uint8_t assoc) -> std::string {
		switch(assoc) {
		case 0ui8:
			return "unknown assocativity";
		case 1ui8:
			return "direct-mapped";
		case 0xffui8:
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

	w.write("Level 1 TLB\n");
	w.write("\t{:s}\n", print_tlb(a.split.d, "data", "2M"));
	w.write("\t{:s}\n", print_tlb(a.split.i, "instruction", "2M"));
	w.write("\t{:s}\n", print_tlb(b.split.d, "data", "4K"));
	w.write("\t{:s}\n", print_tlb(b.split.i, "instruction", "4K"));
	w.write("\n");

	w.write("Level 1 cache\n");
	w.write("\t{:s}\n", print_cache(c.split, "data"));
	w.write("\t{:s}\n", print_cache(d.split, "instruction"));

	w.write("\n");
	std::cout << w.str() << std::flush;
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
	case 0x0ui32:
		return "disabled";
	case 0x1ui32:
		return "direct-mapped";
	case 0x2ui32:
		return "2-way associative";
	case 0x3ui32:
		return "3-way associative";
	case 0x4ui32:
		return "4-way associative";
	case 0x5ui32:
		return "6-way associative";
	case 0x6ui32:
		return "8-way associative";
	case 0x8ui32:
		return "16-way associative";
	case 0xaui32:
		return "32-way associative";
	case 0xbui32:
		return "48-way associative";
	case 0xcui32:
		return "64-way associative";
	case 0xdui32:
		return "96-way associative";
	case 0xeui32:
		return "128-way associative";
	case 0xfui32:
		return "fully associative";
	default:
		return std::to_string(assoc) + "-way associative";
	}
};

std::string print_tlb(const tlb_element& tlb, const std::string& type, const std::string& page_size) {
	using namespace fmt::literals;
	return "{:d}-entry {:s} L2 {:s} TLB for {:s} pages"_format(tlb.entries, print_associativity(tlb.associativity), type, page_size);
};

void print_l2_cache_tlb(const cpu_t & cpu) {
	const register_set_t& regs = cpu.leaves.at(leaf_t::l2_cache_identifiers).at(subleaf_t::main);
	fmt::MemoryWriter w;
	
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

	const union
	{
		std::uint32_t full;
		tlb_info split; // 2M page
	} a = { regs[eax] };

	const union
	{
		std::uint32_t full;
		tlb_info split; // 4K page

	} b = { regs[ebx] };

	const union
	{
		std::uint32_t full;
		l2_cache_info split;
	} c = { regs[ecx] };

	const union
	{
		std::uint32_t full;
		l3_cache_info split;
	} d = { regs[edx] };

	const auto print_l2_size = [](std::uint32_t cache_size) {
		return print_size(cache_size * 1024);
	};

	const auto print_l3_size = [](std::uint32_t cache_size) {
		return print_size(cache_size * 1024 * 512);
	};
	
	switch(cpu.vendor & any_silicon) {
	case amd:
		w.write("Level 2 TLB\n");
		w.write("\t{:s}\n", print_tlb(a.split.d, "data", "2M"));
		w.write("\t{:s}\n", print_tlb(a.split.i, "instruction", "2M"));
		w.write("\t{:s}\n", print_tlb(b.split.d, "data", "4K"));
		w.write("\t{:s}\n", print_tlb(b.split.i, "instruction", "4K"));
		w.write("\n");

		w.write("Level 2 cache\n");
		w.write("\t{:s} {:s} L2 cache with {:d} bytes per line and {:d} lines per tag\n", print_l2_size(c.split.size),
		                                                                                  print_associativity(c.split.associativity),
		                                                                                  c.split.line_size,
		                                                                                  c.split.lines_per_tag);

		w.write("\n");

		w.write("Level 3 cache\n");
		w.write("\t{:s} {:s} L3 cache with {:d} bytes per line and {:d} lines per tag", print_l3_size(d.split.size),
		                                                                                print_associativity(d.split.associativity),
		                                                                                d.split.line_size,
		                                                                                d.split.lines_per_tag);
		w.write("\n");
		break;
	case intel:
		w.write("Level 2 cache\n");
		w.write("\t{:s} {:s} L2 cache with {:d} bytes per line\n", print_l2_size(c.split.size),
		                                                           print_associativity(c.split.associativity),
		                                                           c.split.line_size);
		w.write("\n");
		break;
	default:
		print_generic(w, cpu, leaf_t::l2_cache_identifiers, subleaf_t::main);
		break;
	}
	w.write("\n");
	std::cout << w.str() << std::flush;
}

void print_1g_tlb(const cpu_t& cpu) {
	fmt::MemoryWriter w;

	const register_set_t& regs = cpu.leaves.at(leaf_t::tlb_1g_identifiers).at(subleaf_t::main);
	
	const union
	{
		std::uint32_t full;
		tlb_info split; // l1
	} a = { regs[eax] };

	const union
	{
		std::uint32_t full;
		tlb_info split; // l2
	} b = { regs[eax] };

	w.write("Level 1 1GB page TLB\n");
	w.write("\t{:s}\n", print_tlb(a.split.d, "data", "1G"));
	w.write("\t{:s}\n", print_tlb(a.split.i, "instruction", "1G"));
	w.write("\t{:s}\n", print_tlb(b.split.d, "data", "1G"));
	w.write("\t{:s}\n", print_tlb(b.split.i, "instruction", "1G"));
	w.write("\n");
	std::cout << w.str() << std::flush;
}

void enumerate_cache_properties(cpu_t& cpu) {
	register_set_t regs = { 0 };
	subleaf_t sub = subleaf_t::main;
	while(true) {
		cpuid(regs, leaf_t::cache_properties, sub);
		if((regs[eax] & 0xfui32) == 0) {
			break;
		}
		cpu.leaves[leaf_t::cache_properties][subleaf_t{ sub }] = regs;
		++sub;
	}
}

void print_cache_properties(const cpu_t& cpu) {
	using namespace fmt::literals;
	fmt::MemoryWriter w;
	
	w.write("Cache properties\n");

	for(const auto& sub : cpu.leaves.at(leaf_t::cache_properties)) {
		const register_set_t& regs = sub.second;

		const union
		{
			std::uint32_t full;
			struct
			{
				std::uint32_t type                           : 5;
				std::uint32_t level                          : 3;
				std::uint32_t self_initializing              : 1;
				std::uint32_t fully_associative              : 1;
				std::uint32_t reserved_1                     : 4;
				std::uint32_t maximum_addressable_thread_ids : 12;
				std::uint32_t reserved_2                     : 6;
			} split;
		} a = { regs[eax] };

		const union
		{
			std::uint32_t full;
			struct
			{
				std::uint32_t coherency_line_size      : 12;
				std::uint32_t physical_line_partitions : 10;
				std::uint32_t associativity_ways       : 10;
			} split;
		} b = { regs[ebx] };

		const union
		{
			std::uint32_t full;
			struct
			{
				std::uint32_t writeback_invalidates : 1;
				std::uint32_t cache_inclusive       : 1;
				std::uint32_t reserved_1            : 30;
			} split;
		} d = { regs[edx] };

		const std::size_t sets = regs[ecx];
		const std::size_t cache_size = (b.split.associativity_ways       + 1ui32)
		                             * (b.split.coherency_line_size      + 1ui32)
		                             * (b.split.physical_line_partitions + 1ui32)
		                             * (sets                             + 1ui32);

		w.write("\t{:s} L{:d} ", print_size(cache_size), a.split.level);
		switch(a.split.type) {
		case 1:
			w.write("data");
			break;
		case 2:
			w.write("instruction");
			break;
		case 3:
			w.write("unified");
			break;
		}
		w.write("\n");
		w.write("\t\t{:d} bytes per line \u00d7 {:d} ways \u00d7 {:d} sets = {:s}.\n", b.split.coherency_line_size + 1ui32,
		                                                                               b.split.associativity_ways  + 1ui32,
		                                                                               sets                        + 1ui32,
		                                                                               print_size(cache_size));
		if(a.split.self_initializing) {
			w.write("\t\tSelf-initializing.\n");
		}
		if(a.split.fully_associative) {
			w.write("\t\tFully associative.\n");
		} else {
			w.write("\t\t{:d}-way set associative.\n", b.split.associativity_ways + 1ui32);
		}
		if(d.split.writeback_invalidates) {
			w.write("\t\tWBINVD/INVD does not invalidate lower level caches for other threads.\n");
		} else {
			w.write("\t\tWBINVD/INVD invalidate lower level caches for all threads.\n");
		}
		w.write("\t\tCache is {:s}inclusive of lower cache levels.\n", d.split.cache_inclusive != 0 ? "" : "not ");
		w.write("\t\tCache is shared by up to {:d} threads in the package.\n", a.split.maximum_addressable_thread_ids + 1);
		w.write("\n");
	}
	std::cout << w.str() << std::flush;
}

void print_extended_apic(const cpu_t& cpu) {
	const register_set_t& regs = cpu.leaves.at(leaf_t::extended_apic).at(subleaf_t::main);
	fmt::MemoryWriter w;

	const union
	{
		std::uint32_t full;
		struct
		{
			std::uint32_t core_id          : 8;
			std::uint32_t threads_per_core : 8;
			std::uint32_t reserved_1       : 16;
		} split;
	} b = { regs[ebx] };

	const union
	{
		std::uint32_t full;
		struct
		{
			std::uint32_t node_id             : 8;
			std::uint32_t nodes_per_processor : 3;
			std::uint32_t reserved_1          : 21;
		} split;
	} c = { regs[ecx] };

	w.write("Extended APIC\n");
	w.write("\tExtended APIC ID: {:#010x}\n", regs[eax]);
	w.write("\tCore ID: {:#04x}\n", b.split.core_id);
	w.write("\tThreads per core: {:d}\n", (b.split.threads_per_core + 1ui32));
	w.write("\tNode ID: {:#04x}\n", c.split.node_id);
	w.write("\tNodes per processor: {:d}\n", (c.split.nodes_per_processor + 1ui32));
	w.write("\n");
	std::cout << w.str() << std::flush;
}

struct cache_instance_t
{
	std::vector<std::uint32_t> sharing_ids;
};

struct cache_t
{
	std::uint32_t level;
	std::uint32_t type;
	std::uint32_t ways;
	std::uint32_t sets;
	std::uint32_t line_size;
	std::uint32_t line_partitions;
	std::uint32_t total_size;
	bool fully_associative;
	bool direct_mapped;
	bool self_initializing;
	bool invalidates_lower_levels;
	bool inclusive;
	std::uint32_t sharing_mask;

	std::map<std::uint32_t, cache_instance_t> instances;
};

std::string to_string(const cache_t& cache) {
	using namespace fmt::literals;
	fmt::MemoryWriter w;
	w.write("{:s} L{:d} ", print_size(cache.total_size), cache.level);
	switch(cache.type) {
	case 1:
		w.write("data");
		break;
	case 2:
		w.write("instruction");
		break;
	case 3:
		w.write("unified");
		break;
	}
	w.write("\n");
	w.write("\t\t{:d} bytes per line \u00d7 {:d} ways \u00d7 {:d} partitions \u00d7 {:d} sets = {:s}.\n", cache.line_size,
	                                                                                                      cache.ways,
	                                                                                                      cache.line_partitions,
	                                                                                                      cache.sets,
	                                                                                                      print_size(cache.total_size));
	if(cache.self_initializing) {
		w.write("\t\tSelf-initializing.\n");
	}
	if(cache.fully_associative) {
		w.write("\t\tFully associative.\n");
	} else {
		w.write("\t\t{:d}-way set associative.\n", cache.ways);
	}
	if(cache.invalidates_lower_levels) {
		w.write("\t\tWBINVD/INVD does not invalidate lower level caches for other threads.\n");
	} else {
		w.write("\t\tWBINVD/INVD invalidate lower level caches for all threads.\n");
	}
	w.write("\t\tCache is {:s}inclusive of lower cache levels.\n", cache.inclusive ? "" : "not ");
	w.write("\t\tCache is {:s}direct mapped.\n", cache.direct_mapped ? "not " : "");
	w.write("\t\tCache is shared by up to {:d} threads.\n", cache.sharing_mask);
	w.write("\n");
	return w.str();
}

std::string to_short_string(const cache_t& cache) {
	using namespace fmt::literals;
	fmt::MemoryWriter w;
	w.write("{:s} L{:d} ", print_size(cache.total_size), cache.level);
	switch(cache.type) {
	case 1:
		w.write("data       ");
		break;
	case 2:
		w.write("instruction");
		break;
	case 3:
		w.write("unified    ");
		break;
	}
	if(cache.fully_associative) {
		w.write(" fully associative     ");
	} else if(cache.direct_mapped) {
		w.write(" direct-mapped         ");
	} else {
		w.write(" {:>2d}-way set associative", cache.ways);
	}
	w.write(" with {:>5d} sets, {:d} bytes per line", cache.sets, cache.line_size);
	return w.str();
}

bool operator<(const cache_t& lhs, const cache_t& rhs) noexcept {
	return lhs.level != rhs.level ? lhs.level      < rhs.level
	     : lhs.type  != rhs.type  ? lhs.type       < rhs.type
	     :                          lhs.total_size < rhs.total_size;
}

struct logical_core_t
{
	std::uint32_t full_apic_id;

	std::uint32_t package_id;
	std::uint32_t physical_core_id;
	std::uint32_t logical_core_id;

	std::vector<std::uint32_t> non_shared_cache_ids;
	std::vector<std::uint32_t> shared_cache_ids;
};

struct physical_core_t
{
	std::map<std::uint32_t, logical_core_t> logical_cores;
};

struct package_t
{
	std::map<std::uint32_t, physical_core_t> physical_cores;
};

struct system_t
{
	std::uint32_t logical_mask_width;
	std::uint32_t physical_mask_width;
	std::vector<std::uint32_t> x2_apic_ids;

	std::vector<cache_t> all_caches;
	std::vector<logical_core_t> all_cores;

	std::map<std::uint32_t, package_t> packages;
};

struct full_apic_id_t
{
	std::uint32_t logical_id;
	std::uint32_t physical_id;
	std::uint32_t package_id;
};

constexpr full_apic_id_t split_apic_id(std::uint32_t id, std::uint32_t logical_mask_width, std::uint32_t physical_mask_width) noexcept {
	const std::uint32_t logical_select_mask  = ~(0xffff'ffffui32 << logical_mask_width);
	const std::uint32_t logical_id = id & logical_select_mask;
	const std::uint32_t core_select_mask = ~(0xffff'ffffui32 << physical_mask_width);
	const std::uint32_t physical_id = (id & core_select_mask) >> logical_mask_width;
	const std::uint32_t package_select_mask = 0xffff'ffffui32 << physical_mask_width;
	const std::uint32_t package_id = (id & package_select_mask) >> physical_mask_width;
	return { logical_id, physical_id, package_id };
}

std::pair<std::uint32_t, std::uint32_t> generate_mask(std::uint32_t entries) noexcept {
	if(entries > 0x7fff'ffffui32) {
		return std::make_pair(0xffff'ffffui32, 32ui32);
	}
	entries *= 2;
	entries -= 1;
	DWORD idx = 0;
	_BitScanReverse(&idx, entries);
	idx += 1;
	return std::make_pair((1ui32 << idx) - 1ui32, idx);
}

void determine_topology(const std::vector<cpu_t>& logical_cpus) {
	system_t machine = {};
	const std::uint32_t total_addressable_cores = gsl::narrow_cast<std::uint32_t>(logical_cpus.size());
	bool enumerated_caches = false;
	std::for_each(std::begin(logical_cpus), std::end(logical_cpus), [&machine, &enumerated_caches](const cpu_t& cpu) {
		machine.x2_apic_ids.push_back(cpu.apic_id);
		if(enumerated_caches) {
			return;
		}
		enumerated_caches = true;
		switch(cpu.vendor & any_silicon) {
		case intel:
			{
				if(cpu.highest_leaf >= leaf_t::extended_topology) {
					for(const auto& sub : cpu.leaves.at(leaf_t::extended_topology)) {
						const register_set_t& regs = sub.second;

						const union
						{
							std::uint32_t full;
							struct
							{
								std::uint32_t shift_distance : 5;
								std::uint32_t reserved_1     : 27;
							} split;
						} a = { regs[eax] };

						const union
						{
							std::uint32_t full;
							struct
							{
								std::uint32_t level_number : 8;
								std::uint32_t level_type   : 8;
								std::uint32_t reserved_1   : 16;
							} split;
						} c = { regs[ecx] };

						switch(c.split.level_type) {
						case 1:
							if(machine.logical_mask_width == 0ui32) {
								machine.logical_mask_width = a.split.shift_distance;
							}
							break;
						case 2:
							if(machine.physical_mask_width == 0ui32) {
								machine.physical_mask_width = a.split.shift_distance;
							}
							break;
						default:
							break;
						}
					}
				}
				if(cpu.highest_leaf >= leaf_t::deterministic_cache) {
					for(const auto& sub : cpu.leaves.at(leaf_t::deterministic_cache)) {
						const register_set_t& regs = sub.second;

						const union
						{
							std::uint32_t full;
							struct
							{
								std::uint32_t type                           : 5;
								std::uint32_t level                          : 3;
								std::uint32_t self_initializing              : 1;
								std::uint32_t fully_associative              : 1;
								std::uint32_t reserved_1                     : 4;
								std::uint32_t maximum_addressable_thread_ids : 12;
								std::uint32_t maximum_addressable_core_ids   : 6;
							} split;
						} a = { regs[eax] };

						const union
						{
							std::uint32_t full;
							struct
							{
								std::uint32_t coherency_line_size      : 12;
								std::uint32_t physical_line_partitions : 10;
								std::uint32_t associativity_ways       : 10;
							} split;
						} b = { regs[ebx] };

						const union
						{
							std::uint32_t full;
							struct
							{
								std::uint32_t writeback_invalidates : 1;
								std::uint32_t cache_inclusive       : 1;
								std::uint32_t complex_indexing      : 1;
								std::uint32_t reserved_1            : 29;
							} split;
						} d = { regs[edx] };

						switch(sub.first) {
						case subleaf_t::main:
							if(machine.logical_mask_width == 0ui32) {
								const union
								{
									std::uint32_t full;
									id_info_t     split;
								} leaf_1_b = { cpu.leaves.at(leaf_t::version_info).at(subleaf_t::main).at(ebx) };

								const std::uint32_t total_possible_cores = leaf_1_b.split.maximum_addressable_ids;
								const std::uint32_t total_cores_in_package = a.split.maximum_addressable_core_ids + 1ui32;
								const std::uint32_t logical_cores_per_physical_core = total_possible_cores / total_cores_in_package;
								
								const auto logical_mask = generate_mask(logical_cores_per_physical_core);
								machine.logical_mask_width = logical_mask.second;
								const auto physical_mask = generate_mask(total_cores_in_package);
								machine.physical_mask_width = physical_mask.second;
							}

							[[fallthrough]];
						default:
							const std::uint32_t sets = regs[ecx];
							const std::uint32_t cache_size = (b.split.associativity_ways       + 1ui32)
							                               * (b.split.physical_line_partitions + 1ui32)
							                               * (b.split.coherency_line_size      + 1ui32)
							                               * (sets                             + 1ui32);

							const cache_t cache = {
								a.split.level,
								a.split.type,
								b.split.associativity_ways + 1ui32,
								regs[ecx] + 1ui32,
								b.split.coherency_line_size + 1ui32,
								b.split.physical_line_partitions + 1ui32,
								cache_size,
								a.split.fully_associative != 0,
								d.split.complex_indexing == 0,
								a.split.self_initializing != 0,
								d.split.writeback_invalidates != 0,
								d.split.cache_inclusive != 0,
								a.split.maximum_addressable_thread_ids
							};
							machine.all_caches.push_back(cache);
						}
					}
				}
			}
			break;
		case amd:
			{
				if(cpu.highest_extended_leaf >= leaf_t::extended_apic) {
					const register_set_t& regs = cpu.leaves.at(leaf_t::extended_apic).at(subleaf_t::main);
					const union
					{
						std::uint32_t full;
						struct
						{
							std::uint32_t core_id          : 8;
							std::uint32_t threads_per_core : 8;
							std::uint32_t reserved_1       : 16;
						} split;
					} b = { regs[ebx] };
					machine.logical_mask_width = generate_mask(b.split.threads_per_core).second;
				}
				if(cpu.highest_extended_leaf >= leaf_t::cache_properties) {
					for(const auto& sub : cpu.leaves.at(leaf_t::cache_properties)) {
						const register_set_t& regs = sub.second;
						const union
						{
							std::uint32_t full;
							struct
							{
								std::uint32_t type                           : 5;
								std::uint32_t level                          : 3;
								std::uint32_t self_initializing              : 1;
								std::uint32_t fully_associative              : 1;
								std::uint32_t reserved_1                     : 4;
								std::uint32_t maximum_addressable_thread_ids : 12;
								std::uint32_t reserved_2                     : 6;
							} split;
						} a = { regs[eax] };

						const union
						{
							std::uint32_t full;
							struct
							{
								std::uint32_t coherency_line_size      : 12;
								std::uint32_t physical_line_partitions : 10;
								std::uint32_t associativity_ways       : 10;
							} split;
						} b = { regs[ebx] };

						const union
						{
							std::uint32_t full;
							struct
							{
								std::uint32_t writeback_invalidates : 1;
								std::uint32_t cache_inclusive       : 1;
								std::uint32_t reserved_1            : 30;
							} split;
						} d = { regs[edx] };

						const std::uint32_t sets = regs[ecx];
						const std::uint32_t cache_size = (b.split.associativity_ways       + 1ui32)
						                               * (b.split.physical_line_partitions + 1ui32)
						                               * (b.split.coherency_line_size      + 1ui32)
						                               * (sets                             + 1ui32);

						const cache_t cache = {
							a.split.level,
							a.split.type,
							b.split.associativity_ways + 1ui32,
							regs[ecx] + 1ui32,
							b.split.coherency_line_size + 1ui32,
							b.split.physical_line_partitions + 1ui32,
							cache_size,
							a.split.fully_associative != 0,
							false,
							a.split.self_initializing != 0,
							d.split.writeback_invalidates != 0,
							d.split.cache_inclusive != 0,
							a.split.maximum_addressable_thread_ids
						};
						machine.all_caches.push_back(cache);
					}
				}
				if(cpu.highest_extended_leaf >= leaf_t::address_limits) {
					const register_set_t& regs = cpu.leaves.at(leaf_t::address_limits).at(subleaf_t::main);
					const union
					{
						std::uint32_t full;
						struct
						{
							std::uint32_t package_threads : 8;
							std::uint32_t reserved_1      : 4;
							std::uint32_t apic_id_size    : 4;
							std::uint32_t perf_tsc_size   : 2;
							std::uint32_t reserved_2      : 24;
						} split;
					} c = { regs[ecx] };

					machine.physical_mask_width = c.split.apic_id_size;
				}
			}
			break;
		}
	});

	for(const std::uint32_t id : machine.x2_apic_ids) {
		const full_apic_id_t split = split_apic_id(id, machine.logical_mask_width, machine.physical_mask_width);
		
		logical_core_t core = { id, split.package_id, split.physical_id, split.logical_id };

		for(const cache_t& cache : machine.all_caches) {
			core.shared_cache_ids.push_back(id & cache.sharing_mask);
			core.non_shared_cache_ids.push_back(id & ~cache.sharing_mask);
		}
		machine.all_cores.push_back(core);

		machine.packages[split.package_id].physical_cores[split.physical_id].logical_cores[split.logical_id] = core;
	}

	for(std::size_t i = 0; i < machine.all_caches.size(); ++i) {
		cache_t& cache = machine.all_caches[i];
		for(const logical_core_t& core : machine.all_cores) {
			cache.instances[core.non_shared_cache_ids[i]].sharing_ids.push_back(core.full_apic_id);
		}
	}

	std::multimap<std::uint32_t, std::string> cache_output;
	for(const cache_t& cache : machine.all_caches) {
		std::uint32_t cores_covered = 0ui32;
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


	fmt::MemoryWriter w;

	for(const auto& p : cache_output) {
		w.write("{:s}\n", p.second);
	}
	w.write("\n");

	std::uint32_t cores_covered_package = 0ui32;
	std::uint32_t cores_covered_physical = 0ui32;
	std::uint32_t cores_covered_logical = 0ui32;
	for(const auto& package : machine.packages) {
		std::uint32_t logical_per_package = 0ui32;
		for(const auto& physical : package.second.physical_cores) {
			std::uint32_t logical_per_physical = 0ui32;
			for(const auto& logical : physical.second.logical_cores) {
				for(std::uint32_t i = 0; i < cores_covered_logical; ++i) {
					w.write("-");
				}
				for(std::uint32_t i = 0; i < 1; ++i) {
					w.write("*");
					++cores_covered_logical;
					++logical_per_physical;
					++logical_per_package;
				}
				for(std::uint32_t i = cores_covered_logical; i < total_addressable_cores; ++i) {
					w.write("-");
				}
				w.write(" logical  {:d}:{:d}:{:d}\n", package.first, physical.first, logical.first);
			}
			for(std::uint32_t i = 0; i < cores_covered_physical; ++i) {
				w.write("-");
			}
			for(std::uint32_t i = 0; i < logical_per_physical; ++i) {
				w.write("*");
				++cores_covered_physical;
			}
			for(std::uint32_t i = cores_covered_physical; i < total_addressable_cores; ++i) {
				w.write("-");
			}
			w.write(" physical {:d}:{:d}\n", package.first, physical.first);
		}
		for(std::uint32_t i = 0; i < cores_covered_package; ++i) {
			w.write("-");
		}
		for(std::uint32_t i = 0; i < logical_per_package; ++i) {
			w.write("*");
			++cores_covered_package;
		}
		for(std::uint32_t i = cores_covered_package; i < total_addressable_cores; ++i) {
			w.write("-");
		}
		w.write(" package  {:d}\n", package.first);
	}
	w.write("\n");
	std::cout << w.str() << std::flush;
}

#include "stdafx.h"

#include "cache.hpp"

#include <fmt/format.h>

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
	const std::uint8_t page = attrs & all_page_sizes;
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
		{ unified    , level_3 , 4    MB, 0   , no_attributes                 , static_cast<cache_associativity_t>(0x10), 64 },
		{ unified    , level_2 , 4    MB, 0   , no_attributes                 , static_cast<cache_associativity_t>(0x10), 64 }
	} },
	{ 0x63, {
		{ data_tlb   , no_cache, 0      , 32  , pages_2m | pages_4m           , static_cast<cache_associativity_t>(0x04), 0 },
		{ data_tlb   , no_cache, 0      , 4   , pages_1g                      , static_cast<cache_associativity_t>(0x04), 0 }
	} },
	{ 0xc3, {
		{ unified_tlb, level_2 , 0      , 1536, pages_4k | pages_2m           , static_cast<cache_associativity_t>(0x06), 0 },
		{ unified_tlb, level_2 , 0      , 16  , pages_1g                      , static_cast<cache_associativity_t>(0x04), 0 }
	} }
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
		double printable_cache_size = desc.size / 1'024.0;
		char   cache_scale = 'K';
		if(printable_cache_size > 1'024.0) {
			printable_cache_size /= 1'024.0;
			cache_scale = 'M';
		}
		w << "{:g} {:c}bytes"_format(printable_cache_size, cache_scale);
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

	if(cpu.vendor != intel && cpu.vendor != cyrix) {
		return;
	}

	const register_set_t& regs = cpu.features.at(leaf_t::cache_and_tlb).at(subleaf_t::main);

	if((regs[eax] & 0xff) != 0x01) {
		return;
	}

	const auto bytes = gsl::as_bytes(gsl::make_span(regs));

	std::vector<std::string> cache_entries;

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
				cache_entries.push_back("No 2nd-level cache or, if processor contains a valid 2nd-level cache, no 3rd-level cache");
				break;
			case 0x49ui8:
				{
					auto it = dual_cache_descriptors.find(value);
					if(cpu.model.family == 0x0f && cpu.model.model == 0x06) {
						cache_entries.push_back(to_string(it->second.first));
					} else {
						cache_entries.push_back(to_string(it->second.second));
					}
				}
				break;
			case 0xf0ui8:
				cache_entries.push_back("64-Byte prefetching");
				break;
			case 0xf1ui8:
				cache_entries.push_back("128-Byte prefetching");
				break;
			case 0xfeui8:
			case 0xffui8:
				break;
			default:
				{
					auto dual = dual_cache_descriptors.find(value);
					if(dual != dual_cache_descriptors.end()) {
						cache_entries.push_back(to_string(dual->second.first) + " and "
						                      + to_string(dual->second.second));
						break;
					}
					auto it = standard_cache_descriptors.find(value);
					if(it != standard_cache_descriptors.end()) {
						cache_entries.push_back(to_string(it->second));
						break;
					}
					cache_entries.push_back("Unknown cache type: {:#2x}"_format(value));
				}
				break;
			}
		}
	}

	std::cout << "Cache and TLB\n";
	for(const std::string& s : cache_entries) {
		std::cout << s << std::endl;
	}
	std::cout << std::endl;
}

void enumerate_deterministic_cache(cpu_t& cpu) {
	if(cpu.vendor != intel) {
		return;
	}

	register_set_t regs = { 0 };
	subleaf_t sub = subleaf_t::main;
	while(true) {
		cpuid(regs, leaf_t::deterministic_cache, sub);
		if((regs[eax] & 0x1fui32) == 0) {
			break;
		}
		cpu.features[leaf_t::deterministic_cache][subleaf_t{ sub }] = regs;
		++sub;
	}
}

void print_deterministic_cache(const cpu_t& cpu) {
	using namespace fmt::literals;

	if(cpu.vendor != intel) {
		return;
	}

	struct cache_a_t
	{
		std::uint32_t type                           : 5;
		std::uint32_t level                          : 3;
		std::uint32_t self_initializing              : 1;
		std::uint32_t fully_associative              : 1;
		std::uint32_t reserved_1                     : 4;
		std::uint32_t maximum_addressable_thread_ids : 12;
		std::uint32_t maximum_addressable_core_ids   : 6;
	};

	struct cache_b_t
	{
		std::uint32_t coherency_line_size      : 12;
		std::uint32_t physical_line_partitions : 10;
		std::uint32_t associativity_ways       : 10;
	};

	struct cache_d_t
	{
		std::uint32_t writeback_invalidates : 1;
		std::uint32_t cache_inclusive       : 1;
		std::uint32_t complex_indexing      : 1;
		std::uint32_t reserved_1            : 29;
	};

	std::cout << "Deterministic cache\n";

	for(const auto& m : cpu.features.at(leaf_t::deterministic_cache)) {
		const register_set_t& regs = m.second;

		union
		{
			std::uint32_t raw;
			cache_a_t a;
		} a;
		a.raw = regs[eax];

		union
		{
			std::uint32_t raw;
			cache_b_t b;
		} b;
		b.raw = regs[ebx];

		union
		{
			std::uint32_t raw;
			cache_d_t d;
		} d;
		d.raw = regs[edx];
		
		const std::size_t sets = regs[ecx];
		const std::size_t cache_size = (b.b.associativity_ways       + 1ui32)
		                             * (b.b.physical_line_partitions + 1ui32)
		                             * (b.b.coherency_line_size      + 1ui32)
		                             * (sets                         + 1ui32);
		
		fmt::MemoryWriter w;
		switch(a.a.type) {
		case 1:
			w << "Data Cache       , ";
			break;
		case 2:
			w << "Instruction Cache, ";
			break;
		case 3:
			w << "Unified Cache    , ";
			break;
		}
		double printable_cache_size = cache_size / 1'024.0;
		char   cache_scale = 'K';
		if(printable_cache_size > 1'024.0) {
			printable_cache_size /= 1'024.0;
			cache_scale = 'M';
		}
		w << "Level " << a.a.level << ", ";
		w << "{:d} bytes per line \u00d7 {:d} ways \u00d7 {:d} partitions \u00d7 {:d} sets = {:g} {:c}bytes. "_format(b.b.coherency_line_size + 1i32,
		                                                                                                              b.b.associativity_ways + 1ui32,
		                                                                                                              b.b.physical_line_partitions + 1ui32,
		                                                                                                              sets + 1ui32,
		                                                                                                              printable_cache_size,
		                                                                                                              cache_scale);
		if(a.a.self_initializing) {
			w << "Self-initializing. ";
		}
		if(a.a.fully_associative) {
			w << "Fully associative. ";
		} else {
			w << "{:d}-way set associative. "_format(b.b.associativity_ways + 1ui32);
		}
		if(d.d.writeback_invalidates) {
			w << "WBINVD/INVD does not invalidate lower level caches for other threads. ";
		} else {
			w << "WBINVD/INVD invalidate lower level caches for all threads. ";
		}
		w << "Cache is {:s} of lower cache levels. "_format(d.d.cache_inclusive != 0 ? "inclusive" : "exclusive");
		w << "Cache is {:s}direct mapped. "_format(d.d.complex_indexing != 0 ? "not " : " ");
		w << "Cache is shared by up to {:d} threads, with up to {:d} cores in the package."_format(a.a.maximum_addressable_thread_ids + 1, a.a.maximum_addressable_core_ids + 1);
		std::cout << w.str() << std::endl;
		std::cout << std::endl;
	}
}

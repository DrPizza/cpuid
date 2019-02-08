#ifndef UTILITY_HPP
#define UTILITY_HPP

#ifdef _WIN32
#include <SDKDDKVer.h>

#define STRICT
#define NOMINMAX
#include <Windows.h>
#else
#if !defined(_GNU_SOURCE)
#define _GNU_SOURCE
#endif
#include <unistd.h>
#include <sched.h>
#include <pthread.h>
#endif

#include <thread>

#include <codecvt>

#include <fmt/format.h>

#include "suffixes.hpp"

#if !defined(_WIN32)

inline cpu_set_t* alloc_cpu_set(std::size_t* size) {
	// the CPU set macros don't handle cases like my Azure VM, where there are 2 cores, but 128 possible cores (why???)
	// hence requiring an oversized 16 byte cpu_set_t rather than the 8 bytes that the macros assume to be sufficient.
	// this is the only way (even documented as such!) to figure out how to make a buffer big enough
	unsigned long* buffer = nullptr;
	int len = 0;
	do {
		++len;
		delete [] buffer;
		buffer = new unsigned long[len];
	} while(pthread_getaffinity_np(pthread_self(), len * sizeof(unsigned long), reinterpret_cast<cpu_set_t*>(buffer)) == EINVAL);

	*size = len * sizeof(unsigned long);
	return reinterpret_cast<cpu_set_t*>(buffer);
}

inline void free_cpu_set(cpu_set_t* s) {
	delete [] reinterpret_cast<unsigned long*>(s);
}

#endif

template<typename Fn>
void run_on_every_core(Fn&& f) {
	std::thread bouncer = std::thread([&]() {
#ifdef _WIN32
		const WORD total_processor_groups = ::GetMaximumProcessorGroupCount();
		for(WORD group_id = 0; group_id < total_processor_groups; ++group_id) {
			const DWORD processors_in_group = ::GetMaximumProcessorCount(group_id);
			for(DWORD proc = 0; proc < processors_in_group; ++proc) {
				const GROUP_AFFINITY aff = { 1_u64 << proc, group_id };
				::SetThreadGroupAffinity(::GetCurrentThread(), &aff, nullptr);
				f();
			}
		}
#else
		long int total_cores = sysconf(_SC_NPROCESSORS_CONF);
		std::size_t cpu_size = 0;
		cpu_set_t* cpus = alloc_cpu_set(&cpu_size);

		CPU_ZERO_S(cpu_size, cpus);
		for(long int i = 0; i < total_cores; ++i) {
			CPU_SET_S(i, cpu_size, cpus);
			pthread_setaffinity_np(pthread_self(), cpu_size, cpus);
			f();
			CPU_CLR_S(i, cpu_size, cpus);
		}
		free_cpu_set(cpus);
#endif
	});
	bouncer.join();
}

#if defined(_MSC_VER)
inline unsigned char bit_scan_reverse(unsigned long* index, unsigned int mask) {
	return _BitScanReverse(index, mask);
}

inline unsigned char bit_scan_forward(unsigned long* index, unsigned int mask) {
	return _BitScanForward(index, mask);
}
#else
inline unsigned char bit_scan_reverse(unsigned long* index, unsigned int mask) {
	if(mask) {
		*index = 31 - __builtin_clz(mask);
		return 1;
	} else {
		return 0;
	}
}

inline unsigned char bit_scan_forward(unsigned long* index, unsigned int mask) {
	if(mask) {
		*index = __builtin_ctz(mask);
		return 1;
	} else {
		return 0;
	}
}
#endif

inline std::uint32_t simple_mask(const std::uint32_t length) noexcept {
	if(length == 32_u32) {
		return 0xffff'ffff_u32;
	} else if(length == 0_u32) {
		return 0_u32;
	} else {
		return (1_u32 << length) - 1_u32;
	}
};

inline std::uint32_t range_mask(const std::uint32_t start, const std::uint32_t end) noexcept {
	return simple_mask(end + 1_u32) ^ simple_mask(start);
};

namespace fmt {
	template<size_t N>
	struct formatter<std::array<char, N> >
	{
		template<typename ParseContext>
		constexpr auto parse(ParseContext& ctx) {
			return ctx.begin();
		}

		template<typename FormatContext>
		auto format(const std::array<char, N>& arr, FormatContext& ctx) {
			for(const char c : arr) {
				if(c != '\0') {
					format_to(ctx.begin(), "{:c}", c);
				}
			}
			return ctx.begin();
		}
	};
}

template <class To, class From>
typename std::enable_if_t<sizeof(To) == sizeof(From)
	&& std::is_trivially_copyable<From>::value
	&& std::is_trivial<To>::value,
	To>
	// constexpr support needs compiler magic
	bit_cast(const From& src) noexcept
{
	std::remove_const_t<To> dst;
	std::memcpy(&dst, &src, sizeof(To));
	return dst;
}

#endif

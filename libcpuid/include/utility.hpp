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
		cpu_set_t* cpus = CPU_ALLOC(total_cores);
		std::size_t cpu_size = CPU_ALLOC_SIZE(total_cores);

		CPU_ZERO_S(cpu_size, cpus);
		for(long int i = 0; i < total_cores; ++i) {
			CPU_SET_S(i, cpu_size, cpus);
			pthread_setaffinity_np(pthread_self(), cpu_size, cpus);
			f();
			CPU_CLR_S(i, cpu_size, cpus);
		}
		CPU_FREE(cpus);
#endif
	});
	bouncer.join();
}

#if !defined(_MSC_VER)

#include <cpuid.h>

unsigned char inline _BitScanReverse(unsigned long* index, unsigned int mask) {
	if(mask) {
		*index = 31 - __builtin_clz(mask);
		return 1;
	} else {
		return 0;
	}
}

unsigned char inline _BitScanForward(unsigned long* index, unsigned int mask) {
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

#endif

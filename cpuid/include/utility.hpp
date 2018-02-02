#ifndef UTILITY_HPP
#define UTILITY_HPP

#ifdef _WIN32
#include <SDKDDKVer.h>

#define STRICT
#define NOMINMAX
#include <Windows.h>
#else
#include <unistd.h>
#include <sched.h>
#include <pthread.h>
#endif

#include <thread>

#include <fmt/format.h>

template<typename Fn>
void run_on_every_core(Fn&& f) {
	std::thread bouncer = std::thread([&]() {
#ifdef _WIN32
		const WORD total_processor_groups = ::GetMaximumProcessorGroupCount();
		for(WORD group_id = 0; group_id < total_processor_groups; ++group_id) {
			const DWORD processors_in_group = ::GetMaximumProcessorCount(group_id);
			for(DWORD proc = 0; proc < processors_in_group; ++proc) {
				const GROUP_AFFINITY aff = { 1ui64 << proc, group_id };
				::SetThreadGroupAffinity(::GetCurrentThread(), &aff, nullptr);
				f();
			}
		}
#else
        long int total_cores = sysconf(_SC_NPROCESSORS_CONF);
		cpu_set_t* cpus = CPU_ALLOC(total_cores);
		std::size_t cpu_size = CPU_SIZE(total_cores);

		CPU_ZERO_S(cpu_size, cpus);
		for(long int i = 0; i < total_cores; ++i) {
			CPU_SET_S(i, cpu_size, cpus);
			pthread_setaffinity_np(pthread_self(), cpu_size, cpus);
			f();
			CPU_CLEAR_S(i, cpu_size, cpus);
		}
#endif
	});
	bouncer.join();
}

inline std::uint32_t simple_mask(const std::uint32_t length) noexcept {
	if(length == 32ui32) {
		return 0xffff'ffffui32;
	} else if(length == 0ui32) {
		return 0ui32;
	} else {
		return (1ui32 << length) - 1ui32;
	}
};

inline std::uint32_t range_mask(const std::uint32_t start, const std::uint32_t end) noexcept {
	return simple_mask(end + 1ui32) ^ simple_mask(start);
};

namespace fmt {
	template<size_t N>
	void format_arg(fmt::BasicFormatter<char>& f, const char*&, const std::array<char, N>& arr) {
		for(const char c : arr) {
			if(c != '\0') {
				f.writer().write("{:c}", c);
			} else {
				return;
			}
		}
	}
}

#endif

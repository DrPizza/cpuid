#include "stdafx.h"

#include "definitions.hpp"

uint64_t tick_rate = 0ui64;
uint64_t measurement_overhead = 0ui64;

uint64_t get_actual_frequency() {
	using namespace std::chrono_literals;

	::SetPriorityClass(::GetCurrentProcess(), REALTIME_PRIORITY_CLASS);
	::SetThreadPriority(::GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
	volatile size_t garbage = 0;
	std::chrono::time_point<std::chrono::high_resolution_clock> start, end;
	start = std::chrono::high_resolution_clock::now();
	auto duration = start - start;
	std::array<int32_t, 4> unused = { 0 };
	__cpuidex(unused.data(), 0, 0);
	uint64_t timestamp_start = __rdtsc();
	do {
		++garbage;
		end = std::chrono::high_resolution_clock::now();
	} while ((duration = end - start) < 1s);
	uint32_t aux = 0ui32;
	uint64_t timestamp_end = __rdtscp(&aux);
	__cpuidex(unused.data(), 0, 0);
	::SetThreadPriority(::GetCurrentThread(), THREAD_PRIORITY_NORMAL);
	::SetPriorityClass(::GetCurrentProcess(), NORMAL_PRIORITY_CLASS);
	return ((timestamp_end - timestamp_start) * 1'000'000'000ui64) / (duration.count());
}

uint64_t get_measurement_overhead() {
	::SetPriorityClass(::GetCurrentProcess(), REALTIME_PRIORITY_CLASS);
	::SetThreadPriority(::GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
	::SetThreadAffinityMask(::GetCurrentThread(), 1ULL);

	alignas(64) volatile std::atomic<uint64_t> dummy = 0ui64;
	std::array<int32_t, 4> unused = { 0 };
	uint32_t aux = 0ui32;
	__cpuidex(unused.data(), 0, 0);
	uint64_t empty_loop_start = __rdtsc();

	for (size_t i = 0; i < iteration_count; ++i) {
		dummy = i;
	}

	uint64_t empty_loop_end = __rdtscp(&aux);
	__cpuidex(unused.data(), 0, 0);
	/////////
	dummy = 0ui64;
	__cpuidex(unused.data(), 0, 0);
	uint64_t rdtscp_loop_start = __rdtsc();

	for (size_t i = 0; i < iteration_count; ++i) {
		dummy.store(0ui64, std::memory_order_release);
		if (0 == dummy.load(std::memory_order_acquire)) {
			__rdtscp(&aux);
		}
	}

	uint64_t rdtscp_loop_end = __rdtscp(&aux);
	__cpuidex(unused.data(), 0, 0);

	::SetThreadAffinityMask(::GetCurrentThread(), ~0ULL);
	::SetThreadPriority(::GetCurrentThread(), THREAD_PRIORITY_NORMAL);
	::SetPriorityClass(::GetCurrentProcess(), NORMAL_PRIORITY_CLASS);

	uint64_t empty_loop_duration = empty_loop_end - empty_loop_start;
	uint64_t rdtscp_duration = rdtscp_loop_end - rdtscp_loop_start;

	uint64_t iteration_overhead = (rdtscp_duration - empty_loop_duration) / iteration_count;
	return iteration_overhead;
}

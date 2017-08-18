#include "stdafx.h"

#include "definitions.hpp"
#include "measurement.hpp"

void cache_ping() {
	std::condition_variable cv;
	std::mutex mtx;
	size_t threads_started = 0ULL;

	SYSTEM_INFO si = { 0 };
	::GetSystemInfo(&si);

	std::vector<std::vector<double> > scores(si.dwNumberOfProcessors, std::vector<double>(si.dwNumberOfProcessors));
	for(DWORD_PTR source_core = 0ULL; source_core < si.dwNumberOfProcessors; ++source_core) {
		for (DWORD_PTR destination_core = 0ULL; destination_core < si.dwNumberOfProcessors; ++destination_core) {
			if(source_core == destination_core) {
				continue;
			}
			alignas(64)             uint64_t  running_sum = { 0ui64 };
			alignas(64) std::atomic<uint64_t> ping = { 0ui64 };
			std::atomic<uint64_t>* ping_ptr = &ping;
			
			::SetPriorityClass(::GetCurrentProcess(), REALTIME_PRIORITY_CLASS);

			std::vector<std::thread> threads;
			static constexpr size_t thread_count = 2;
			for(size_t i = 0; i < thread_count; ++i) {
				threads.emplace_back([&](size_t position) {
					const bool is_source = position == 0;
					const DWORD_PTR mask = 1ULL << (is_source ? source_core : destination_core);
					::SetThreadAffinityMask(::GetCurrentThread(), mask);
					::SetThreadPriority(::GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);

					{
						std::unique_lock<std::mutex> lck(mtx);
						++threads_started;
					}
					cv.notify_all();

					{
						std::unique_lock<std::mutex> lck(mtx);
						cv.wait(lck, [&] { return threads_started == 0; });
					}
					std::atomic<uint64_t>& ping_ref = *ping_ptr;
					// If I capture ping directly by-ref, VC++ generates silly code for the while loop.
					// specifically, it generates a loop that looks like this:
					// 
					// loopstart:
					// mov         rax, qword ptr[rdi + 30h] // rax = &ping;
					// mov         rcx, qword ptr[rax]       // rcx = *rax;
					// test        rcx, rcx
					// jne loopstart
					// And it does this even though qword ptr[rdi + 30h] is invariant (and immutable!)
					// If I capture the address and then form a reference on the stack (rather than as a lambda member)
					// then the loop is tighter:
					// mov         rax, qword ptr[rdi + 30h] // rax = &ping;
					// loopstart:
					// mov         rcx, qword ptr[rax]       // rcx = *rax;
					// test        rcx, rcx
					// jne loopstart
					// Since I want the loop to be as tight as possible, the second form is better.

					int32_t unused[4];
					if(is_source) {
						for(size_t i = 0; i < iteration_count; ++i) {
							while(ping_ref.load(std::memory_order_acquire) != 0) {
								;
							}
							// Intel's preferred mechanism: serialize then rdtsc before, the thing you're timing, rdtscp then serialize after
							// http://www.intel.com/content/www/us/en/embedded/training/ia-32-ia-64-benchmark-code-execution-paper.html
							// cpuid remains the only good cross-platform serializing instrunction, regrettably.
							__cpuidex(unused, 0, 0); // no instructions from above this line can execute after the rdtsc
							uint64_t ping_sent = __rdtsc();
							ping_ref.store(ping_sent, std::memory_order_release);
						}
					} else {
						for(size_t i = 0; i < iteration_count; ++i) {
							uint64_t ping_sent = 0;
							while((ping_sent = ping_ref.load(std::memory_order_acquire)) == 0) {
								;
							}
							uint32_t aux = 0ui32;
							uint64_t ping_received = __rdtscp(&aux);
							__cpuidex(unused, 0, 0); // no instructions from below this line can execute before the rdtscp

							ping_ref.store(0, std::memory_order_release);
							uint64_t raw_duration = ping_received - ping_sent;
							uint64_t duration = raw_duration - measurement_overhead;
							running_sum += duration;
						}
					}
				}, i);
			}

			{
				std::unique_lock<std::mutex> lck(mtx);
				cv.wait(lck, [&] { return threads_started == thread_count; });
			}
			{
				std::unique_lock<std::mutex> lck(mtx);
				threads_started = 0;
			}
			cv.notify_all();
			for(std::thread& t : threads) {
				t.join();
			}

			::SetPriorityClass(::GetCurrentProcess(), NORMAL_PRIORITY_CLASS);

			const double mean_cycles_per_ping = static_cast<double>(running_sum) / static_cast<double>(iteration_count);

			const uint64_t cycles_per_second = tick_rate;
			const double nanoseconds_per_cycle = static_cast<double>(nanoseconds_per_second) / static_cast<double>(cycles_per_second);
			const double nanoseconds_per_ping = mean_cycles_per_ping * nanoseconds_per_cycle;
			scores[source_core][destination_core] = nanoseconds_per_ping;
		}
	}
	char old_fill = std::cout.fill();
	std::cout << "       \\ core-to-core ping time/ns" << std::endl;
	std::cout << "        \\ destination" << std::endl;
	std::cout << " source  \\ ";
	for(DWORD_PTR source_core = 0ULL; source_core < si.dwNumberOfProcessors; ++source_core) {
		std::cout << std::setw(5) << source_core << std::setw(0) << "|";
	}
	std::cout << std::endl;
	std::cout << "__________\\";
	for(DWORD_PTR source_core = 0ULL; source_core < si.dwNumberOfProcessors; ++source_core) {
		std::cout << std::setw(6) << std::setfill('_') << "|";
	}
	std::cout << std::setfill(old_fill) << std::endl;

	for(DWORD_PTR source_core = 0ULL; source_core < si.dwNumberOfProcessors; ++source_core) {
		std::cout << std::setw(9) << std::right << source_core << " |" << std::left << std::setw(0);
		for(DWORD_PTR destination_core = 0ULL; destination_core < si.dwNumberOfProcessors; ++destination_core) {
			std::cout << std::setw(5) << std::right;
			if(source_core != destination_core) {
				std::cout << std::fixed << std::setprecision(0) << scores[source_core][destination_core];
			} else {
				std::cout << "-";
			}
			std::cout << std::setw(0) << std::left << "|";
		}
		std::cout << std::endl;
	}
}

#include "stdafx.h"

typedef struct _PROCESSOR_POWER_INFORMATION {
	ULONG Number;
	ULONG MaxMhz;
	ULONG CurrentMhz;
	ULONG MhzLimit;
	ULONG MaxIdleState;
	ULONG CurrentIdleState;
} PROCESSOR_POWER_INFORMATION, *PPROCESSOR_POWER_INFORMATION;

//static constexpr size_t iteration_count = 1'000'000ULL;
static constexpr size_t iteration_count = 100'000ULL;
static unsigned __int64 tick_rate = 0ui64;
static unsigned __int64 measurement_overhead = 0ui64;

void cache_ping() {
	std::condition_variable cv;
	std::mutex mtx;
	size_t threads_started = 0ULL;

	SYSTEM_INFO si = { 0 };
	::GetSystemInfo(&si);

	std::vector<std::vector<double> > scores(si.dwNumberOfProcessors, std::vector<double>(si.dwNumberOfProcessors));

	for(DWORD_PTR source_core = 0ULL; source_core < si.dwNumberOfProcessors; ++source_core) {
		for(DWORD_PTR destination_core = 0ULL; destination_core < si.dwNumberOfProcessors; ++destination_core) {
			if(source_core == destination_core) {
				continue;
			}
			alignas(64)             unsigned __int64  running_sum = { 0ui64 };
			alignas(64)             unsigned __int64  running_sum_squares = { 0ui64 };
			alignas(64) std::atomic<unsigned __int64> ping = { 0ui64 };
			std::atomic<unsigned __int64>* ping_ptr = &ping;
			
			::SetPriorityClass(::GetCurrentProcess(), REALTIME_PRIORITY_CLASS);

			std::vector<std::thread> threads;
			static constexpr size_t thread_count = 2;
			for(size_t i = 0; i < thread_count; ++i) {
				threads.push_back(std::thread([&](const size_t num) {
					const bool is_source = num == 0;
					DWORD_PTR mask = 1ULL << (is_source ? source_core : destination_core);
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
					std::atomic<unsigned __int64>& ping_ref = *ping_ptr;
					// if I capture ping directly by-ref, VC++ generates lousy code for the while loop.
					// specifically, it generates a loop that looks like this:
					// 
					// loopstart:
					// mov         rax, qword ptr[rdi + 30h] // rax = &ping;
					// mov         rcx, qword ptr[rax]       // rcx = *rax;
					// test        rcx, rcx
					// jne loopstart
					// and it does this even though qword ptr[rdi + 30h] is invariant (and immutable!)
					// if I capture the address and then form a reference
					// on the stack (rather than as a lambda member)
					// then the loop is tighter:
					// mov         rax, qword ptr[rdi + 30h] // rax = &ping;
					// loopstart:
					// mov         rcx, qword ptr[rax]       // rcx = *rax;
					// test        rcx, rcx
					// jne loopstart
					__int32 unused[4];

					if(is_source) {
						for(size_t i = 0; i < iteration_count; ++i) {
							while(ping_ref.load(std::memory_order_acquire) != 0) {
								;
							}
							// Intel's preferred mechanism: serialize then rdtsc before, the thing you're timing, rdtscp then serialize after
							__cpuidex(unused, 0, 0);
							unsigned __int64 ping_sent = __rdtsc();
							ping_ref.store(ping_sent, std::memory_order_release);
						}
					} else {
						for(size_t i = 0; i < iteration_count; ++i) {
							unsigned __int64 ping_sent = 0;
							while((ping_sent = ping_ref.load(std::memory_order_acquire)) == 0) {
								;
							}
							unsigned __int32 aux = 0ui32;
							unsigned __int64 ping_received = __rdtscp(&aux);
							__cpuidex(unused, 0, 0);

							ping_ref.store(0, std::memory_order_release);

							unsigned __int64 duration = (ping_received - ping_sent) - measurement_overhead;
							running_sum += duration;
							running_sum_squares += duration * duration;
						}
					}
				}, i));
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
			const double sum_squared = static_cast<double>(running_sum) * static_cast<double>(running_sum);
			const double variance = (running_sum_squares - (sum_squared / static_cast<double>(iteration_count))) / static_cast<double>(iteration_count);
			const double stddev = std::sqrt(variance);

			const unsigned __int64 cycles_per_second = tick_rate;
			const unsigned __int64 nanoseconds_per_second = 1'000'000'000ui64;
			const double nanoseconds_per_cycle = static_cast<double>(nanoseconds_per_second) / static_cast<double>(cycles_per_second);
			const double nanoseconds_per_ping = mean_cycles_per_ping * nanoseconds_per_cycle;
			//std::cout << mean_cycles_per_ping << " (" << stddev << ") cycles per ping = " << nanoseconds_per_ping << " nanoseconds per ping" << std::endl;
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

void cache_ping_pong() {
	std::condition_variable cv;
	std::mutex mtx;
	size_t threads_started = 0ULL;

	SYSTEM_INFO si = { 0 };
	::GetSystemInfo(&si);

	alignas(64) std::atomic<size_t> shared_value = { 0ULL };
	std::atomic<size_t>* shared_value_ptr = &shared_value;
	for(DWORD_PTR core_base = 0ULL; core_base < si.dwNumberOfProcessors; ++core_base) {
		static constexpr size_t thread_count = 2;
		std::cout << "running on cores";
		for(size_t i = 0; i < thread_count; ++i) {
			std::cout << " " << (core_base + i) % si.dwNumberOfProcessors;
		}
		std::cout << ". ";
		std::atomic<unsigned __int64> total_time = { 0ui64 };
		std::vector<std::thread> threads;
		for(size_t i = 0; i < thread_count; ++i) {
			threads.push_back(std::thread([&](const size_t num) {
				DWORD_PTR mask = 1ULL << ((core_base + num) % si.dwNumberOfProcessors);
				::SetThreadAffinityMask(::GetCurrentThread(), mask);
				::SetThreadPriority(::GetCurrentThread(), THREAD_PRIORITY_HIGHEST);

				{
					std::unique_lock<std::mutex> lck(mtx);
					++threads_started;
				}
				cv.notify_all();

				{
					std::unique_lock<std::mutex> lck(mtx);
					cv.wait(lck, [&] { return threads_started == 0; });
				}
				// see long comment above about VC++ codegen
				std::atomic<size_t>& shared_value_ref = *shared_value_ptr;

				__int32 unused[4];
				__cpuidex(unused, 0, 0);
				unsigned __int64 start = __rdtsc();
				for(size_t i = 0; i < iteration_count; ++i) {
					while(shared_value_ref.load(std::memory_order_acquire) % thread_count != num) {
						;
					}
					shared_value_ref.fetch_add(1, std::memory_order_release);
				}
				unsigned __int32 aux = 0ui32;
				unsigned __int64 end = __rdtscp(&aux);
				__cpuidex(unused, 0, 0);
				total_time += (end - start);
			}, i));
		}

		{
			std::unique_lock<std::mutex> lck(mtx);
			cv.wait(lck, [&] { return threads_started == thread_count; });
		}
		threads_started = 0;
		cv.notify_all();
		for(std::thread& t : threads) {
			t.join();
		}

		// for a single ping-pong
		total_time = total_time / thread_count;
		const double cycles_per_ping_pong = static_cast<double>(total_time) / static_cast<double>(iteration_count);
		const unsigned __int64 cycles_per_second = tick_rate;
		const unsigned __int64 nanoseconds_per_second = 1'000'000'000ui64;
		const double nanoseconds_per_cycle = static_cast<double>(nanoseconds_per_second) / static_cast<double>(cycles_per_second);
		const double nanoseconds_per_ping_pong = cycles_per_ping_pong * nanoseconds_per_cycle;
		std::cout << cycles_per_ping_pong << " cycles per ping pong = " << nanoseconds_per_ping_pong << " nanoseconds per ping pong" << std::endl;
	}
}

void store_buffers() {
	static constexpr size_t items = 1ULL << 24;
	static constexpr size_t mask = items - 1;
	static constexpr size_t iterations = 100'000'000;
	std::vector<std::vector<unsigned char> > arrays{ 12, std::vector<unsigned char>(items, '\0') };
	::SetThreadPriority(::GetCurrentThread(), THREAD_PRIORITY_HIGHEST);

	__int32 unused[4];
	unsigned __int32 also_unused;
	{
		__cpuidex(unused, 0, 0);
		unsigned __int64 start = __rdtsc();

		size_t i = iterations;
		while(--i) {
			size_t slot = i & mask;
			unsigned char value = static_cast<unsigned char>(i & 0xff);
			arrays[0][slot] = value;
			arrays[1][slot] = value;
			arrays[2][slot] = value;
			arrays[3][slot] = value;
			arrays[4][slot] = value;
			arrays[5][slot] = value;
			arrays[6][slot] = value;
			arrays[7][slot] = value;
			arrays[8][slot] = value;
			arrays[9][slot] = value;
			arrays[10][slot] = value;
			arrays[11][slot] = value;
		}

		unsigned __int64 end = __rdtscp(&also_unused);
		__cpuidex(unused, 0, 0);

		std::cout << "combined: " << end - start << std::endl;
	}

	{
		__cpuidex(unused, 0, 0);
		unsigned __int64 start = __rdtsc();

		size_t i = iterations;
		while(--i) {
			size_t slot = i & mask;
			unsigned char value = static_cast<unsigned char>(i & 0xff);
			arrays[0][slot] = value;
			arrays[1][slot] = value;
			arrays[2][slot] = value;
			arrays[3][slot] = value;
			arrays[4][slot] = value;
			arrays[5][slot] = value;
		}

		i = iterations;
		while(--i) {
			size_t slot = i & mask;
			unsigned char value = static_cast<unsigned char>(i & 0xff);
			arrays[6][slot] = value;
			arrays[7][slot] = value;
			arrays[8][slot] = value;
			arrays[9][slot] = value;
			arrays[10][slot] = value;
			arrays[11][slot] = value;
		}

		unsigned __int64 end = __rdtscp(&also_unused);
		__cpuidex(unused, 0, 0);

		std::cout << "split:    " << end - start << std::endl;
	}
	::SetThreadPriority(::GetCurrentThread(), THREAD_PRIORITY_NORMAL);
}

void pointer_chasing() {
	static const size_t page_size = 4096ULL;
	static const size_t cache_line_size = 64ULL;
	const size_t length = (1024ULL * 1024ULL * 1024ULL) / sizeof(void*);
	
	std::unique_ptr<void*[]> storage{ new void*[length] };

	// pattern 1: each pointer simply points to the next pointer.
	// maximal cache hits, maximal TLB hits.
	for(size_t i = 0ULL; i < length; ++i) {
		storage[i] = &storage[i + 1];
	}
	storage[length - 1] = &storage[0];

	// pattern 2: stride one cache line at a time
	size_t stride = cache_line_size;
	for(size_t i = stride; i < length; i += stride) {
		storage[i - stride] = &storage[i];
	}
	storage[length - stride] = &storage[0];
}

unsigned __int64 get_actual_frequency() {
	using namespace std::chrono_literals;

	::SetPriorityClass(::GetCurrentProcess(), REALTIME_PRIORITY_CLASS);
	::SetThreadPriority(::GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
	volatile size_t garbage = 0;
	std::chrono::time_point<std::chrono::high_resolution_clock> start, end;
	start = std::chrono::high_resolution_clock::now();
	auto duration = start - start;
	std::array<__int32, 4> unused = { 0 };
	__cpuidex(unused.data(), 0, 0);
	unsigned __int64 timestamp_start = __rdtsc();
	do {
		++garbage;
		end = std::chrono::high_resolution_clock::now();
	} while((duration = end - start) < 1s);
	unsigned __int32 aux = 0ui32;
	unsigned __int64 timestamp_end = __rdtscp(&aux);
	__cpuidex(unused.data(), 0, 0);
	::SetThreadPriority(::GetCurrentThread(), THREAD_PRIORITY_NORMAL);
	::SetPriorityClass(::GetCurrentProcess(), NORMAL_PRIORITY_CLASS);
	return ((timestamp_end - timestamp_start) * 1'000'000'000ui64) / (duration.count());
}

unsigned __int64 get_measurement_overhead() {
	::SetPriorityClass(::GetCurrentProcess(), REALTIME_PRIORITY_CLASS);
	::SetThreadPriority(::GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);

	std::array<__int32, 4> unused = { 0 };
	unsigned __int32 aux = 0ui32;
	__cpuidex(unused.data(), 0, 0);
	unsigned __int64 timestamp_start = __rdtsc();

	volatile unsigned __int64 garbage = 0ui64;
	for(size_t i = 0; i < iteration_count; ++i) {
		unsigned __int64 discard_start = __rdtsc();
		garbage += discard_start;
		unsigned __int64 discard_end = __rdtscp(&aux);
		garbage += discard_end;
	}

	unsigned __int64 timestamp_end = __rdtscp(&aux);
	__cpuidex(unused.data(), 0, 0);

	::SetThreadPriority(::GetCurrentThread(), THREAD_PRIORITY_NORMAL);
	::SetPriorityClass(::GetCurrentProcess(), NORMAL_PRIORITY_CLASS);

	return (timestamp_end - timestamp_start) / iteration_count;
}

int main(int argc, char*[]) {
	std::array<int, 4> cpu = { 0 };
	__cpuidex(cpu.data(), 0x8000'0000, 0x0);
	if(cpu[0] >= 0x8000'0004) {
		union {
			std::array<char, 48> brand;
			std::array<std::array<__int32, 4>, 3> registers;
		} data;
		__cpuidex(data.registers[0].data(), 0x8000'0002, 0x0);
		__cpuidex(data.registers[1].data(), 0x8000'0003, 0x0);
		__cpuidex(data.registers[2].data(), 0x8000'0004, 0x0);
		std::cout << data.brand.data() << std::endl;
	}
	if(cpu[0] < 0x8000'0007) {
		std::cout << "I can't perform timing on this chip yet" << std::endl;
		return -1;
	}

	std::array<__int32, 4> registers = { 0 };
	__cpuidex(registers.data(), 0x8000'0007, 0x0);
	if(0ui32 == (registers[3] & (1ui32 << 8ui32))) {
		std::cout << "I can't perform timing on this chip yet" << std::endl;
		return -1;
	}
	
	SYSTEM_INFO si = { 0 };
	::GetSystemInfo(&si);

	std::unique_ptr<PROCESSOR_POWER_INFORMATION[]> ppi{ new PROCESSOR_POWER_INFORMATION[si.dwNumberOfProcessors] };

	::CallNtPowerInformation(ProcessorInformation, nullptr, 0, ppi.get(), sizeof(PROCESSOR_POWER_INFORMATION) * si.dwNumberOfProcessors);
	std::cout << "Maximum frequency: " << ppi[0].MaxMhz << " MHz (as reported to/by Windows, which seems in fact to be the base frequency at P0)" << std::endl;
	tick_rate = get_actual_frequency();
	measurement_overhead = get_measurement_overhead();

	cache_ping();
	//cache_ping_pong();
	//store_buffers();
	//pointer_chasing();
}

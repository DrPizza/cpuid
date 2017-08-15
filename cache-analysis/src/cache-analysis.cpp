#include "stdafx.h"

typedef struct _PROCESSOR_POWER_INFORMATION {
	ULONG Number;
	ULONG MaxMhz;
	ULONG CurrentMhz;
	ULONG MhzLimit;
	ULONG MaxIdleState;
	ULONG CurrentIdleState;
} PROCESSOR_POWER_INFORMATION, *PPROCESSOR_POWER_INFORMATION;

//constexpr size_t iteration_count = 1'000'000ULL;
constexpr size_t iteration_count = 100'000ULL;
static uint64_t tick_rate = 0ui64;
static uint64_t measurement_overhead = 0ui64;

constexpr uint64_t nanoseconds_per_second = 1'000'000'000ui64;

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
		std::atomic<uint64_t> total_time = { 0ui64 };
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

				int32_t unused[4];
				__cpuidex(unused, 0, 0);
				uint64_t start = __rdtsc();
				for(size_t i = 0; i < iteration_count; ++i) {
					while(shared_value_ref.load(std::memory_order_acquire) % thread_count != num) {
						;
					}
					shared_value_ref.fetch_add(1, std::memory_order_release);
				}
				uint32_t aux = 0ui32;
				uint64_t end = __rdtscp(&aux);
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
		const uint64_t cycles_per_second = tick_rate;
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

	int32_t unused[4];
	uint32_t also_unused;
	{
		__cpuidex(unused, 0, 0);
		uint64_t start = __rdtsc();

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

		uint64_t end = __rdtscp(&also_unused);
		__cpuidex(unused, 0, 0);

		std::cout << "combined: " << end - start << std::endl;
	}

	{
		__cpuidex(unused, 0, 0);
		uint64_t start = __rdtsc();

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

		uint64_t end = __rdtscp(&also_unused);
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
	} while((duration = end - start) < 1s);
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

	for(size_t i = 0; i < iteration_count; ++i) {
		dummy = i;
	}

	uint64_t empty_loop_end = __rdtscp(&aux);
	__cpuidex(unused.data(), 0, 0);
	/////////
	dummy = 0ui64;
	__cpuidex(unused.data(), 0, 0);
	uint64_t rdtscp_loop_start = __rdtsc();

	for(size_t i = 0; i < iteration_count; ++i) {
		dummy.store(0ui64, std::memory_order_release);
		if(0 == dummy.load(std::memory_order_acquire)) {
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

enum cpuid : int32_t {
	basic_info                   = 0x0000'0000i32,
	extended_limit               = 0x8000'0000i32,
	brand_string_0               = 0x8000'0002i32,
	brand_string_1               = 0x8000'0003i32,
	brand_string_2               = 0x8000'0004i32,
	advanced_power_management    = 0x8000'0007i32,
	amd_secure_memory_encryption = 0x8000'001fi32,
};

enum regs : int8_t {
	eax,
	ebx,
	ecx,
	edx
};

int main(int, char*[]) {
	std::array<int, 4> cpu = { 0 };
	__cpuidex(cpu.data(), cpuid::extended_limit, 0x0);
	if(cpu[regs::eax] >= cpuid::brand_string_2) {
		union {
			std::array<char, 48> brand;
			std::array<std::array<int32_t, 4>, 3> registers;
		} data;
		__cpuidex(data.registers[0].data(), cpuid::brand_string_0, 0x0);
		__cpuidex(data.registers[1].data(), cpuid::brand_string_1, 0x0);
		__cpuidex(data.registers[2].data(), cpuid::brand_string_2, 0x0);
		std::cout << data.brand.data() << std::endl;
	}
	if(cpu[regs::eax] < cpuid::advanced_power_management) {
		std::cout << "I can't perform timing on this chip" << std::endl;
		return -1;
	}

	std::array<int32_t, 4> registers = { 0 };
	__cpuidex(registers.data(), cpuid::advanced_power_management, 0x0);
	if(0ui32 == (registers[regs::edx] & (1ui32 << 8ui32))) {
		std::cout << "I can't perform timing on this chip" << std::endl;
		return -1;
	}
	
	SYSTEM_INFO si = { 0 };
	::GetSystemInfo(&si);

	std::unique_ptr<PROCESSOR_POWER_INFORMATION[]> ppi{ new PROCESSOR_POWER_INFORMATION[si.dwNumberOfProcessors] };

	::CallNtPowerInformation(ProcessorInformation, nullptr, 0, ppi.get(), sizeof(PROCESSOR_POWER_INFORMATION) * si.dwNumberOfProcessors);
	std::cout << "Maximum frequency: " << ppi[0].MaxMhz << " MHz (as reported to/by Windows, which seems in fact to be the base frequency at P0)" << std::endl;
	tick_rate = get_actual_frequency();
	std::cout << "rdtsc ticks at " << tick_rate << " ticks per second" << std::endl;
	measurement_overhead = get_measurement_overhead();

	std::cout << "measurement overhead in ticks: " << measurement_overhead << std::endl;

	cache_ping();
	//cache_ping_pong();
	//store_buffers();
	//pointer_chasing();
}

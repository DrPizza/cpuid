#include "stdafx.h"

typedef struct _PROCESSOR_POWER_INFORMATION {
	ULONG Number;
	ULONG MaxMhz;
	ULONG CurrentMhz;
	ULONG MhzLimit;
	ULONG MaxIdleState;
	ULONG CurrentIdleState;
} PROCESSOR_POWER_INFORMATION, *PPROCESSOR_POWER_INFORMATION;

static const size_t iteration_count = 100'000'000ULL;

void cache_ping() {
	std::condition_variable cv;
	std::mutex mtx;
	size_t threads_started = 0ULL;

	SYSTEM_INFO si = { 0 };
	::GetSystemInfo(&si);

	std::unique_ptr<PROCESSOR_POWER_INFORMATION[]> ppi{ new PROCESSOR_POWER_INFORMATION[si.dwNumberOfProcessors] };

	::CallNtPowerInformation(ProcessorInformation, nullptr, 0, ppi.get(), sizeof(PROCESSOR_POWER_INFORMATION) * si.dwNumberOfProcessors);

	for(DWORD_PTR core_base = 0ULL; core_base < si.dwNumberOfProcessors; ++core_base) {
		static constexpr size_t thread_count = 2;
		std::cout << "running on cores";
		for(size_t i = 0; i < thread_count; ++i) {
			std::cout << " " << (core_base + i) % si.dwNumberOfProcessors;
		}
		std::cout << ". ";

		alignas(64) std::atomic<unsigned __int64> total_time = { 0ui64 };
		alignas(64) std::atomic<unsigned __int64> ping = { 0ULL };
		std::atomic<size_t>* ping_ptr = &ping;

		std::vector<std::thread> threads;
		for(size_t i = 0; i < thread_count; ++i) {
			threads.push_back(std::thread([&, ping_ptr](const size_t num) {
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
				std::atomic<size_t>& ping_ref = *ping_ptr;
				// if I capture ping directly by-ref, VC++ generates lousy code for the while loop.
				// specifically, it generates a loop that looks like this:
				// 
				// loopstart:
				// mov         rax, qword ptr[rdi + 30h] // rax = &ping;
				// mov         rcx, qword ptr[rax]       // rcx = *rax;
				// test        rcx, rcx
				// jne loopstart
				// and it does this even though qword ptr[rdi + 30h] is invariant (and immutable!)
				// if I capture the address by-val and then form a reference
				// on the stack (rather than as a lambda member)
				// then the loop is tighter:
				// mov         rax, qword ptr[rdi + 30h] // rax = &ping;
				// loopstart:
				// mov         rcx, qword ptr[rax]       // rcx = *rax;
				// test        rcx, rcx
				// jne loopstart
				switch(num % 2) {
				case 0:
					{
						for(size_t i = 0; i < iteration_count; ++i) {
							while(ping_ref.load(std::memory_order_acquire) != 0) {
								;
							}
							// Intel's preferred mechanism: serialize then rdtsc before, the thing you're timing, rdtscp then serialize after
							// Intel says that lfence is semi-serializing (no instructions can pass across it), but mfence is not.
							// AMD says that mfence is fully serializing, but lfence is not.
							_mm_mfence();
							_mm_lfence();
							unsigned __int64 ping_sent = __rdtsc();
							ping_ref.store(ping_sent, std::memory_order_release);
						}
					}
					break;
				case 1:
					{
						for(size_t i = 0; i < iteration_count; ++i) {
							unsigned __int64 ping_sent = 0;
							while((ping_sent = ping_ref.load(std::memory_order_acquire)) == 0) {
								;
							}
							unsigned __int32 aux = 0ui32;
							unsigned __int64 ping_received = __rdtscp(&aux);
							_mm_mfence();
							_mm_lfence();

							ping_ref.store(0, std::memory_order_release);

							total_time += (ping_received - ping_sent);
						}
					}
					break;
				}
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
		const double cycles_per_ping = static_cast<double>(total_time) / static_cast<double>(iteration_count);
		const __int64 cycles_per_second = ppi[0].CurrentMhz * 1'000'000;
		const __int64 nanoseconds_per_second = 1'000'000'000;
		const double nanoseconds_per_cycle = static_cast<double>(nanoseconds_per_second) / static_cast<double>(cycles_per_second);
		const double nanoseconds_per_ping = cycles_per_ping * nanoseconds_per_cycle;
		std::cout << cycles_per_ping << " cycles per ping = " << nanoseconds_per_ping << " nanoseconds per ping" << std::endl;
	}
}

void cache_ping_pong() {
	std::condition_variable cv;
	std::mutex mtx;
	size_t threads_started = 0ULL;

	SYSTEM_INFO si = { 0 };
	::GetSystemInfo(&si);

	std::unique_ptr<PROCESSOR_POWER_INFORMATION[]> ppi{ new PROCESSOR_POWER_INFORMATION[si.dwNumberOfProcessors] };

	::CallNtPowerInformation(ProcessorInformation, nullptr, 0, ppi.get(), sizeof(PROCESSOR_POWER_INFORMATION) * si.dwNumberOfProcessors);

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
			threads.push_back(std::thread([&, shared_value_ptr](const size_t num) {
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

				_mm_mfence();
				_mm_lfence();
				unsigned __int64 start = __rdtsc();
				for(size_t i = 0; i < iteration_count; ++i) {
					while(shared_value_ref.load(std::memory_order_acquire) % thread_count != num) {
						;
					}
					shared_value_ref.fetch_add(1, std::memory_order_release);
				}
				unsigned __int32 aux = 0ui32;
				unsigned __int64 end = __rdtscp(&aux);
				_mm_mfence();
				_mm_lfence();
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
		const __int64 cycles_per_second = ppi[0].CurrentMhz * 1'000'000;
		const __int64 nanoseconds_per_second = 1'000'000'000;
		const double nanoseconds_per_cycle = static_cast<double>(nanoseconds_per_second) / static_cast<double>(cycles_per_second);
		const double nanoseconds_per_ping_pong = cycles_per_ping_pong * nanoseconds_per_cycle;
		std::cout << cycles_per_ping_pong << " cycles per ping pong = " << nanoseconds_per_ping_pong << " nanoseconds per ping pong" << std::endl;
	}
}

int main() {
	std::array<int, 4> cpu = { 0 };
	__cpuid(cpu.data(), 0x8000'0000);
	if(cpu[0] >= 0x8000'0004) {
		union {
			std::array<char, 48> brand;
			std::array<std::array<int, 4>, 3> registers;
		} data;
		__cpuid(data.registers[0].data(), 0x8000'0002);
		__cpuid(data.registers[1].data(), 0x8000'0003);
		__cpuid(data.registers[2].data(), 0x8000'0004);
		std::cout << data.brand.data() << std::endl;
	}

	SYSTEM_INFO si = { 0 };
	::GetSystemInfo(&si);

	std::unique_ptr<PROCESSOR_POWER_INFORMATION[]> ppi{ new PROCESSOR_POWER_INFORMATION[si.dwNumberOfProcessors] };

	::CallNtPowerInformation(ProcessorInformation, nullptr, 0, ppi.get(), sizeof(PROCESSOR_POWER_INFORMATION) * si.dwNumberOfProcessors);
	std::cout << "Maximum frequency: " << ppi[0].MaxMhz << " MHz" << std::endl;

	cache_ping();
	cache_ping_pong();
	//scheduler_bounce();
}

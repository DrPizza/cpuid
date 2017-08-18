#include "stdafx.h"

#include "definitions.hpp"
#include "measurement.hpp"
#include "cache-ping.hpp"

typedef struct _PROCESSOR_POWER_INFORMATION {
	ULONG Number;
	ULONG MaxMhz;
	ULONG CurrentMhz;
	ULONG MhzLimit;
	ULONG MaxIdleState;
	ULONG CurrentIdleState;
} PROCESSOR_POWER_INFORMATION, *PPROCESSOR_POWER_INFORMATION;

enum cpuid : int32_t {
	basic_info = 0x0000'0000i32,
	extended_limit = 0x8000'0000i32,
	brand_string_0 = 0x8000'0002i32,
	brand_string_1 = 0x8000'0003i32,
	brand_string_2 = 0x8000'0004i32,
	advanced_power_management = 0x8000'0007i32,
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
	if (cpu[regs::eax] >= cpuid::brand_string_2) {
		union {
			std::array<char, 48> brand;
			std::array<std::array<int32_t, 4>, 3> registers;
		} data;
		__cpuidex(data.registers[0].data(), cpuid::brand_string_0, 0x0);
		__cpuidex(data.registers[1].data(), cpuid::brand_string_1, 0x0);
		__cpuidex(data.registers[2].data(), cpuid::brand_string_2, 0x0);
		std::cout << data.brand.data() << std::endl;
	}
	if (cpu[regs::eax] < cpuid::advanced_power_management) {
		std::cout << "I can't perform timing on this chip" << std::endl;
		return -1;
	}

	std::array<int32_t, 4> registers = { 0 };
	__cpuidex(registers.data(), cpuid::advanced_power_management, 0x0);
	if (0ui32 == (registers[regs::edx] & (1ui32 << 8ui32))) {
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
}

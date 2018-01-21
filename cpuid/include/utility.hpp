#ifndef UTILITY_HPP
#define UTILITY_HPP

#include <SDKDDKVer.h>

#define STRICT
#define NOMINMAX
#include <Windows.h>

#include <thread>

template<typename Fn>
void run_on_every_core(Fn&& f) {
	std::thread bouncer = std::thread([&]() {
		const WORD total_processor_groups = ::GetMaximumProcessorGroupCount();
		for(WORD group_id = 0; group_id < total_processor_groups; ++group_id) {
			const DWORD processors_in_group = ::GetMaximumProcessorCount(group_id);
			for(DWORD proc = 0; proc < processors_in_group; ++proc) {
				const GROUP_AFFINITY aff = { 1ui64 << proc, group_id };
				::SetThreadGroupAffinity(::GetCurrentThread(), &aff, nullptr);
				f();
			}
		}
	});
	bouncer.join();
}

#endif

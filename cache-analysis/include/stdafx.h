#pragma once

#include "targetver.h"

#define _CRT_SECURE_NO_WARNINGS 1
#define _HAS_AUTO_PTR_ETC 1

#pragma warning(disable: 4710) // warning C4710: '%s': function not inlined
#pragma warning(disable: 4668) // warning C4668: '%s' is not defined as a preprocessor macro, replacing with '0' for '#if/#elif'
#pragma warning(disable: 4820) // warning C4820: '%s': '%d' bytes padding added after data member '%s'

#define STRICT
#define NOMINMAX

#include <Windows.h>
#include <PowrProf.h>

#pragma comment(lib, "PowrProf.lib")

#if !defined(_STL_EXTRA_DISABLED_WARNINGS)
#define _STL_EXTRA_DISABLED_WARNINGS 4061 4365 4571 4623 4625 4626 4710 4774 4820 4987 5026 5027
#endif

#include <vector>
#include <thread>
#include <chrono>
#include <atomic>
#include <iostream>
#include <iomanip>
#include <array>
#include <numeric>
#include <condition_variable>
#include <mutex>
#include <numeric>
#include <cstddef>
#include <cstdint>

#include <intrin.h>

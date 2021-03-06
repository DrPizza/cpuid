cmake_minimum_required (VERSION 3.13)

project(libcpuid VERSION 1.0.0 LANGUAGES C CXX)

add_library(libcpuid STATIC src/cpuid/cache-and-topology.cpp src/cpuid/cpuid.cpp src/cpuid/features.cpp src/cpuid/hypervisors.cpp src/cpuid/standard.cpp src/cpuid/utility.cpp)
target_include_directories(libcpuid PUBLIC  include)
target_include_directories(libcpuid PRIVATE src)

set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME ON)
find_package(Boost REQUIRED)
target_include_directories(libcpuid PRIVATE ${Boost_INCLUDE_DIRS})
target_link_libraries(libcpuid PRIVATE ${Boost_LIBRARIES})

find_path(gsl_dir "gsl/gsl")
target_include_directories(libcpuid INTERFACE ${gsl_dir})
target_link_libraries(libcpuid PRIVATE gsl)

find_package(fmt CONFIG REQUIRED)
target_link_libraries(libcpuid PUBLIC fmt::fmt)

find_package(Threads REQUIRED)
target_link_libraries(libcpuid PRIVATE Threads::Threads)

if(MSVC)
	set(LIBCPUID_PRIVATE_DEFINITIONS "UNICODE" "_UNICODE")

	set(LIBCPUID_PRIVATE_CXX_FLAGS         ${LIBCPUID_PRIVATE_CXX_FLAGS} /MP /Zi /W4 /Wall /GF /Gm- /GS /guard:cf /Gy)
	set(LIBCPUID_PRIVATE_CXX_FLAGS         ${LIBCPUID_PRIVATE_CXX_FLAGS} /experimental:external /external:anglebrackets /external:templates- /external:W0)
	set(LIBCPUID_PRIVATE_CXX_FLAGS_DEBUG   ${LIBCPUID_PRIVATE_CXX_FLAGS_DEBUG} /Od /RTC1)
	set(LIBCPUID_PRIVATE_CXX_FLAGS_RELEASE ${LIBCPUID_PRIVATE_CXX_FLAGS_RELEASE} /O2 /Ob2 /Oi /Ot /arch:AVX2 /Qpar)

	set(LIBCPUID_PUBLIC_CXX_FLAGS          ${LIBCPUID_PUBLIC_CXX_FLAGS} /EHsc /permissive- /Zc:wchar_t /Zc:forScope /Zc:inline /Zc:rvalueCast /GR)
	set(LIBCPUID_PUBLIC_CXX_FLAGS_DEBUG    ${LIBCPUID_PUBLIC_CXX_FLAGS_DEBUG})
	set(LIBCPUID_PUBLIC_CXX_FLAGS_RELEASE  ${LIBCPUID_PUBLIC_CXX_FLAGS_RELEASE} /GL)

	set(LIBCPUID_PUBLIC_LINK_FLAGS         ${LIBCPUID_LINK_FLAGS})
	set(LIBCPUID_PUBLIC_LINK_FLAGS_DEBUG   ${LIBCPUID_LINK_FLAGS_DEBUG})
	set(LIBCPUID_PUBLIC_LINK_FLAGS_RELEASE ${LIBCPUID_LINK_FLAGS_RELEASE} /LTCG)

	set(LIBCPUID_STATIC_LIBRARY_FLAGS         ${LIBCPUID_STATIC_LIBRARY_FLAGS})
	set(LIBCPUID_STATIC_LIBRARY_FLAGS_DEBUG   ${LIBCPUID_STATIC_LIBRARY_FLAGS_DEBUG})
	set(LIBCPUID_STATIC_LIBRARY_FLAGS_RELEASE ${LIBCPUID_STATIC_LIBRARY_FLAGS_RELEASE} /LTCG)

	set(MSVC_RUNTIME "static")
	configure_msvc_runtime()
else()
	set(LIBCPUID_DEFINITIONS "")
	set(LIBCPUID_PRIVATE_CXX_FLAGS         ${LIBCPUID_PRIVATE_CXX_FLAGS} -Wall -stdlib=libc++)
	set(LIBCPUID_PRIVATE_CXX_FLAGS_DEBUG   ${LIBCPUID_PRIVATE_CXX_FLAGS_DEBUG} -g)
	set(LIBCPUID_PRIVATE_CXX_FLAGS_RELEASE ${LIBCPUID_PRIVATE_CXX_FLAGS_RELEASE} -O3 -march=native)

	set(LIBCPUID_PUBLIC_CXX_FLAGS          ${LIBCPUID_PUBLIC_CXX_FLAGS})
	set(LIBCPUID_PUBLIC_CXX_FLAGS_DEBUG    ${LIBCPUID_PUBLIC_CXX_FLAGS_DEBUG})
	set(LIBCPUID_PUBLIC_CXX_FLAGS_RELEASE  ${LIBCPUID_PUBLIC_CXX_FLAGS_RELEASE})

	set(LIBCPUID_PUBLIC_LINK_FLAGS         ${LIBCPUID_LINK_FLAGS} -stdlib=libc++ -lc++fs)
	set(LIBCPUID_PUBLIC_LINK_FLAGS_DEBUG   ${LIBCPUID_LINK_FLAGS_DEBUG})
	set(LIBCPUID_PUBLIC_LINK_FLAGS_RELEASE ${LIBCPUID_LINK_FLAGS_RELEASE})

	set(LIBCPUID_STATIC_LIBRARY_FLAGS         ${LIBCPUID_STATIC_LIBRARY_FLAGS})
	set(LIBCPUID_STATIC_LIBRARY_FLAGS_DEBUG   ${LIBCPUID_STATIC_LIBRARY_FLAGS_DEBUG})
	set(LIBCPUID_STATIC_LIBRARY_FLAGS_RELEASE ${LIBCPUID_STATIC_LIBRARY_FLAGS_RELEASE})
endif()

target_compile_features(libcpuid PUBLIC cxx_std_20)

target_compile_options(libcpuid PUBLIC  "${LIBCPUID_PUBLIC_CXX_FLAGS}")
target_compile_options(libcpuid PUBLIC  "$<$<CONFIG:DEBUG>:${LIBCPUID_PUBLIC_CXX_FLAGS_DEBUG}>")
target_compile_options(libcpuid PUBLIC  "$<$<CONFIG:RELEASE>:${LIBCPUID_PUBLIC_CXX_FLAGS_RELEASE}>")

target_compile_definitions(libcpuid PRIVATE ${LIBCPUID_PRIVATE_DEFINITIONS})
target_compile_options(libcpuid PRIVATE "${LIBCPUID_PRIVATE_CXX_FLAGS}")
target_compile_options(libcpuid PRIVATE "$<$<CONFIG:DEBUG>:${LIBCPUID_PRIVATE_CXX_FLAGS_DEBUG}>")
target_compile_options(libcpuid PRIVATE "$<$<CONFIG:RELEASE>:${LIBCPUID_PRIVATE_CXX_FLAGS_RELEASE}>")

target_link_options(libcpuid PUBLIC "${LIBCPUID_PUBLIC_LINK_FLAGS}")
target_link_options(libcpuid PUBLIC "$<$<CONFIG:DEBUG>:${LIBCPUID_PUBLIC_LINK_FLAGS_DEBUG}>")
target_link_options(libcpuid PUBLIC "$<$<CONFIG:RELEASE>:${LIBCPUID_PUBLIC_LINK_FLAGS_RELEASE}>")

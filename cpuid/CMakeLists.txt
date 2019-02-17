cmake_minimum_required (VERSION 3.10)

project(cpuid VERSION 1.0.0 LANGUAGES C CXX)

add_executable(cpuid src/docopt.cpp src/main.cpp)
target_include_directories(cpuid PRIVATE internal)
target_include_directories(cpuid PRIVATE ${Boost_INCLUDE_DIRS})

target_link_libraries(cpuid PRIVATE libcpuid)

find_package(fmt CONFIG REQUIRED)
target_link_libraries(cpuid PRIVATE fmt::fmt)

set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME ON)
find_package(Boost REQUIRED COMPONENTS regex)
target_link_libraries(cpuid PRIVATE ${Boost_LIBRARIES})

find_path(gsl_dir "gsl/gsl")
add_library(gsl INTERFACE)
target_include_directories(cpuid INTERFACE ${gsl_dir})
target_link_libraries(cpuid PRIVATE gsl)

if(MSVC)
	set(CPUID_PRIVATE_CXX_FLAGS         ${CPUID_PRIVATE_CXX_FLAGS} /MP /Zi /W4 /Wall /GF /Gm- /GS /guard:cf /Gy)
	set(CPUID_PRIVATE_CXX_FLAGS         ${CPUID_PRIVATE_CXX_FLAGS} /experimental:external /external:anglebrackets /external:templates- /external:W0)
	set(CPUID_PRIVATE_CXX_FLAGS_DEBUG   ${CPUID_PRIVATE_CXX_FLAGS_DEBUG} /Od /RTC1)
	set(CPUID_PRIVATE_CXX_FLAGS_RELEASE ${CPUID_PRIVATE_CXX_FLAGS_RELEASE} /O2 /Ob2 /Oi /Ot /arch:AVX2 /Qpar)

#	set(CPUID_PUBLIC_CXX_FLAGS          ${CPUID_PRIVATE_CXX_FLAGS} /EHsc /permissive- /Zc:wchar_t /Zc:forScope /Zc:inline /Zc:rvalueCast /GR)
#	set(CPUID_PUBLIC_CXX_FLAGS_RELEASE  ${CPUID_PRIVATE_CXX_FLAGS_RELEASE} /GL)

	set(CPUID_PRIVATE_LINK_FLAGS         ${CPUID_PRIVATE_LINK_FLAGS} /DYNAMICBASE /NXCOMPAT /HIGHENTROPYVA /LARGEADDRESSAWARE)
	set(CPUID_PRIVATE_LINK_FLAGS_DEBUG   ${CPUID_PRIVATE_LINK_FLAGS_DEBUG})
	set(CPUID_PRIVATE_LINK_FLAGS_RELEASE ${CPUID_PRIVATE_LINK_FLAGS_RELEASE} /opt:ref /opt:icf /LTCG)

    set(MSVC_RUNTIME "static")
    configure_msvc_runtime()
else()
endif()

target_compile_options(cpuid PRIVATE "${CPUID_PRIVATE_CXX_FLAGS}")
target_compile_options(cpuid PRIVATE "$<$<CONFIG:DEBUG>:${CPUID_PRIVATE_CXX_FLAGS_DEBUG}>")
target_compile_options(cpuid PRIVATE "$<$<CONFIG:RELEASE>:${CPUID_PRIVATE_CXX_FLAGS_RELEASE}>")

if(${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.13.0") 
	target_link_options(cpuid PRIVATE "${CPUID_PRIVATE_LINK_FLAGS}")
	target_link_options(cpuid PRIVATE "$<$<CONFIG:DEBUG>:${CPUID_PRIVATE_LINK_FLAGS_DEBUG}>")
	target_link_options(cpuid PRIVATE "$<$<CONFIG:RELEASE>:${CPUID_PRIVATE_LINK_FLAGS_RELEASE}>")
else()
	list(JOIN CPUID_PRIVATE_LINK_FLAGS " " CPUID_PRIVATE_LINK_FLAGS)
	list(JOIN CPUID_PRIVATE_LINK_FLAGS_DEBUG " " CPUID_PRIVATE_LINK_FLAGS_DEBUG)
	list(JOIN CPUID_PRIVATE_LINK_FLAGS_RELEASE " " CPUID_PRIVATE_LINK_FLAGS_RELEASE)
	set_target_properties(cpuid PROPERTIES LINK_FLAGS         "${CPUID_PRIVATE_LINK_FLAGS}")
	set_target_properties(cpuid PROPERTIES LINK_FLAGS_DEBUG   "${CPUID_PRIVATE_LINK_FLAGS_DEBUG}")
	set_target_properties(cpuid PROPERTIES LINK_FLAGS_RELEASE "${CPUID_PRIVATE_LINK_FLAGS_RELEASE}")
endif()

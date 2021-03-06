cmake_minimum_required (VERSION 3.13)

project(docopt VERSION 1.0.0 LANGUAGES C CXX)

add_library(docopt STATIC src/docopt/docopt.cpp)
target_include_directories(docopt PUBLIC  include)
target_include_directories(docopt PRIVATE src)

set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME ON)
find_package(Boost REQUIRED)
target_include_directories(docopt PRIVATE ${Boost_INCLUDE_DIRS})
target_link_libraries(docopt PRIVATE ${Boost_LIBRARIES})

find_path(gsl_dir "gsl/gsl")
target_include_directories(docopt INTERFACE ${gsl_dir})
target_link_libraries(docopt PRIVATE gsl)

if(MSVC)
	set(DOCOPT_PRIVATE_DEFINITIONS "UNICODE" "_UNICODE")

	set(DOCOPT_PRIVATE_CXX_FLAGS         ${DOCOPT_PRIVATE_CXX_FLAGS} /MP /Zi /W4 /Wall /GF /Gm- /GS /guard:cf /Gy)
	set(DOCOPT_PRIVATE_CXX_FLAGS         ${DOCOPT_PRIVATE_CXX_FLAGS} /experimental:external /external:anglebrackets /external:templates- /external:W0)
	set(DOCOPT_PRIVATE_CXX_FLAGS_DEBUG   ${DOCOPT_PRIVATE_CXX_FLAGS_DEBUG} /Od /RTC1)
	set(DOCOPT_PRIVATE_CXX_FLAGS_RELEASE ${DOCOPT_PRIVATE_CXX_FLAGS_RELEASE} /O2 /Ob2 /Oi /Ot /arch:AVX2 /Qpar)

	set(DOCOPT_PUBLIC_CXX_FLAGS          ${DOCOPT_PUBLIC_CXX_FLAGS} /EHsc /permissive- /Zc:wchar_t /Zc:forScope /Zc:inline /Zc:rvalueCast /GR)
	set(DOCOPT_PUBLIC_CXX_FLAGS_DEBUG    ${DOCOPT_PUBLIC_CXX_FLAGS_DEBUG})
	set(DOCOPT_PUBLIC_CXX_FLAGS_RELEASE  ${DOCOPT_PUBLIC_CXX_FLAGS_RELEASE} /GL)

	set(DOCOPT_PUBLIC_LINK_FLAGS         ${DOCOPT_LINK_FLAGS})
	set(DOCOPT_PUBLIC_LINK_FLAGS_DEBUG   ${DOCOPT_LINK_FLAGS_DEBUG})
	set(DOCOPT_PUBLIC_LINK_FLAGS_RELEASE ${DOCOPT_LINK_FLAGS_RELEASE} /LTCG)

	set(DOCOPT_STATIC_LIBRARY_FLAGS         ${DOCOPT_STATIC_LIBRARY_FLAGS})
	set(DOCOPT_STATIC_LIBRARY_FLAGS_DEBUG   ${DOCOPT_STATIC_LIBRARY_FLAGS_DEBUG})
	set(DOCOPT_STATIC_LIBRARY_FLAGS_RELEASE ${DOCOPT_STATIC_LIBRARY_FLAGS_RELEASE} /LTCG)

	set(MSVC_RUNTIME "static")
	configure_msvc_runtime()
else()
	set(DOCOPT_DEFINITIONS "")
	set(DOCOPT_PRIVATE_CXX_FLAGS         ${DOCOPT_PRIVATE_CXX_FLAGS} -Wall -stdlib=libc++)
	set(DOCOPT_PRIVATE_CXX_FLAGS_DEBUG   ${DOCOPT_PRIVATE_CXX_FLAGS_DEBUG} -g)
	set(DOCOPT_PRIVATE_CXX_FLAGS_RELEASE ${DOCOPT_PRIVATE_CXX_FLAGS_RELEASE} -O3 -march=native)

	set(DOCOPT_PUBLIC_CXX_FLAGS          ${DOCOPT_PUBLIC_CXX_FLAGS})
	set(DOCOPT_PUBLIC_CXX_FLAGS_DEBUG    ${DOCOPT_PUBLIC_CXX_FLAGS_DEBUG})
	set(DOCOPT_PUBLIC_CXX_FLAGS_RELEASE  ${DOCOPT_PUBLIC_CXX_FLAGS_RELEASE})

	set(DOCOPT_PUBLIC_LINK_FLAGS         ${DOCOPT_LINK_FLAGS} -stdlib=libc++ -lc++fs)
	set(DOCOPT_PUBLIC_LINK_FLAGS_DEBUG   ${DOCOPT_LINK_FLAGS_DEBUG})
	set(DOCOPT_PUBLIC_LINK_FLAGS_RELEASE ${DOCOPT_LINK_FLAGS_RELEASE})

	set(DOCOPT_STATIC_LIBRARY_FLAGS         ${DOCOPT_STATIC_LIBRARY_FLAGS})
	set(DOCOPT_STATIC_LIBRARY_FLAGS_DEBUG   ${DOCOPT_STATIC_LIBRARY_FLAGS_DEBUG})
	set(DOCOPT_STATIC_LIBRARY_FLAGS_RELEASE ${DOCOPT_STATIC_LIBRARY_FLAGS_RELEASE})
endif()

target_compile_features(docopt PUBLIC cxx_std_20)

target_compile_options(docopt PUBLIC  "${DOCOPT_PUBLIC_CXX_FLAGS}")
target_compile_options(docopt PUBLIC  "$<$<CONFIG:DEBUG>:${DOCOPT_PUBLIC_CXX_FLAGS_DEBUG}>")
target_compile_options(docopt PUBLIC  "$<$<CONFIG:RELEASE>:${DOCOPT_PUBLIC_CXX_FLAGS_RELEASE}>")

target_compile_definitions(docopt PRIVATE ${DOCOPT_PRIVATE_DEFINITIONS})
target_compile_options(docopt PRIVATE "${DOCOPT_PRIVATE_CXX_FLAGS}")
target_compile_options(docopt PRIVATE "$<$<CONFIG:DEBUG>:${DOCOPT_PRIVATE_CXX_FLAGS_DEBUG}>")
target_compile_options(docopt PRIVATE "$<$<CONFIG:RELEASE>:${DOCOPT_PRIVATE_CXX_FLAGS_RELEASE}>")

target_link_options(docopt PUBLIC "${DOCOPT_PUBLIC_LINK_FLAGS}")
target_link_options(docopt PUBLIC "$<$<CONFIG:DEBUG>:${DOCOPT_PUBLIC_LINK_FLAGS_DEBUG}>")
target_link_options(docopt PUBLIC "$<$<CONFIG:RELEASE>:${DOCOPT_PUBLIC_LINK_FLAGS_RELEASE}>")

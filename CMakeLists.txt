cmake_minimum_required (VERSION 3.10)

set(VCPKG_TARGET_ARCHITECTURE x64)
if(WIN32)
    set(VCPKG_TARGET_TRIPLET x64-windows-static CACHE STRING "")
    set(VCPKG_CRT_LINKAGE static)
    set(VCPKG_ROOT "C:/Code/Projects/vcpkg")
else()
    set(VCPKG_TARGET_TRIPLET x64-linux CACHE STRING "")
    set(VCPKG_ROOT "~/vcpkg")
endif()
set(CMAKE_TOOLCHAIN_FILE "${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")

set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "" FORCE)

if(MSVC)
    set(CMAKE_GENERATOR_PLATFORM x64)
    set(CMAKE_GENERATOR_TOOLSET v141,host=x64)
    set(CMAKE_VS_PLATFORM_NAME Win64)
else()
endif()

macro(configure_msvc_runtime)
  if(MSVC)

    # Default to statically-linked runtime.
    if("${MSVC_RUNTIME}" STREQUAL "")
      set(MSVC_RUNTIME "static")
    endif()

    # Set compiler options.
    set(variables
      CMAKE_C_FLAGS_DEBUG
      CMAKE_C_FLAGS_MINSIZEREL
      CMAKE_C_FLAGS_RELEASE
      CMAKE_C_FLAGS_RELWITHDEBINFO
      CMAKE_CXX_FLAGS_DEBUG
      CMAKE_CXX_FLAGS_MINSIZEREL
      CMAKE_CXX_FLAGS_RELEASE
      CMAKE_CXX_FLAGS_RELWITHDEBINFO
    )
    if(${MSVC_RUNTIME} STREQUAL "static")
      message(STATUS
        "MSVC -> forcing use of statically-linked runtime."
      )
      foreach(variable ${variables})
        if(${variable} MATCHES "/MD")
          string(REGEX REPLACE "/MD" "/MT" ${variable} "${${variable}}")
        endif()
      endforeach()
    else()
      message(STATUS
        "MSVC -> forcing use of dynamically-linked runtime."
      )
      foreach(variable ${variables})
        if(${variable} MATCHES "/MT")
          string(REGEX REPLACE "/MT" "/MD" ${variable} "${${variable}}")
        endif()
      endforeach()
    endif()
  endif()
endmacro()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(cpuid VERSION 1.0.0 LANGUAGES CXX)

if(MSVC)
    set(MSVC_RUNTIME "static")

    add_definitions(-DUNICODE -D_UNICODE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Zi -W4 -Wall -GF -Gm- -GS -guard:cf -Gy")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -EHsc -permissive- -Zc:wchar_t -Zc:forScope -Zc:inline -Zc:rvalueCast -GR -std:c++latest")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -experimental:external -external:anglebrackets -external:templates- -external:W0 ")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Od -RTC1")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O2 -Ob2 -Oi -Ot -GL -arch:AVX2 -Qpar")
    foreach (i SHARED MODULE EXE)
        set(CMAKE_${i}_LINKER_FLAGS "${CMAKE_${i}_LINKER_FLAGS} -DYNAMICBASE -NXCOMPAT -DEBUG -HIGHENTROPYVA")
        set(CMAKE_${i}_LINKER_FLAGS_RELEASE "${CMAKE_${i}_LINKER_FLAGS_RELEASE} -opt:ref -opt:icf")
    endforeach()
    configure_msvc_runtime()
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native")
endif()

set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME ON)
find_package(Boost REQUIRED COMPONENTS regex)

find_path(gsl_dir "gsl/gsl")
add_library(gsl INTERFACE)
target_include_directories(gsl INTERFACE ${gsl_dir})

find_package(fmt CONFIG REQUIRED)

add_subdirectory("libcpuid")
add_subdirectory("cpuid")

#get_cmake_property(_variableNames VARIABLES)
#list (SORT _variableNames)
#foreach (_variableName ${_variableNames})
#    message(STATUS "${_variableName}=${${_variableName}}")
#endforeach()

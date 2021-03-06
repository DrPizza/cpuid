cmake_minimum_required (VERSION 3.13.0)

if(CMAKE_GENERATOR MATCHES "Visual Studio.*")
    set(CMAKE_GENERATOR_PLATFORM x64)
    set(CMAKE_GENERATOR_TOOLSET v141,host=x64)
    set(CMAKE_VS_PLATFORM_NAME Win64)
endif()

set(VCPKG_TARGET_ARCHITECTURE x64)
if(WIN32)
    set(VCPKG_TARGET_TRIPLET x64-windows-static CACHE STRING "")
    set(VCPKG_CRT_LINKAGE static)
    set(VCPKG_ROOT "C:/Code/Projects/vcpkg")
else()
    set(VCPKG_TARGET_TRIPLET x64-linux CACHE STRING "")
    set(VCPKG_ROOT "~/src/vcpkg")
endif()
set(CMAKE_TOOLCHAIN_FILE "${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")

set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "" FORCE)

macro(configure_msvc_runtime)
  if(MSVC)

    # Default to statically-linked runtime.
    if("${MSVC_RUNTIME}" STREQUAL "")
      set(MSVC_RUNTIME "static")
    endif()

    # Set compiler options.
    set(variables
      CMAKE_C_FLAGS_DEBUG
      CMAKE_C_FLAGS_RELEASE
      CMAKE_CXX_FLAGS_DEBUG
      CMAKE_CXX_FLAGS_RELEASE
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

set(CMAKE_VERBOSE_MAKEFILE TRUE)

project(cpuid-toplevel VERSION 1.0.0 LANGUAGES C CXX)

add_subdirectory("docopt")
add_subdirectory("libcpuid")
add_subdirectory("cpuid")

#get_cmake_property(_variableNames VARIABLES)
#list (SORT _variableNames)
#foreach (_variableName ${_variableNames})
#    message(STATUS "${_variableName}=${${_variableName}}")
#endforeach()

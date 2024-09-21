#
# Copyright 2011-2013 Ettus Research LLC
# Copyright 2018 Ettus Research, a National Instruments Company
# Copyright 2024 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

########################################################################
# This file included, use CMake directory variables
########################################################################
include(CheckIncludeFileCXX)
include(CheckCXXCompilerFlag)
message(STATUS "")

########################################################################
# Check for x86 SIMD compiler support
#
# Note: These checks verify that the COMPILER can generate SIMD code,
# not that the build machine has SIMD support. The actual SIMD converters
# will check CPU support at runtime and only register if the CPU supports
# the instruction set. This allows a single binary to work on machines
# with varying SIMD capabilities.
########################################################################

# Check if we're on an x86 platform
if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64|i[3-6]86")
    set(IS_X86_PLATFORM TRUE)
else()
    set(IS_X86_PLATFORM FALSE)
endif()

if(IS_X86_PLATFORM)
    # Check for SSE2 compiler support
    if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        check_cxx_compiler_flag("-msse2" COMPILER_SUPPORTS_SSE2)
        check_cxx_compiler_flag("-mssse3" COMPILER_SUPPORTS_SSSE3)
        check_cxx_compiler_flag("-mavx2" COMPILER_SUPPORTS_AVX2)
        check_cxx_compiler_flag("-mavx512f" COMPILER_SUPPORTS_AVX512F)
        set(SSE2_FLAGS -msse2)
        set(SSSE3_FLAGS -mssse3)
        set(AVX2_FLAGS -mavx2)
        set(AVX512_FLAGS -mavx512f)
    elseif(MSVC)
        # MSVC doesn't need flags for SSE2 on x64, but we can check
        set(COMPILER_SUPPORTS_SSE2 TRUE)
        set(COMPILER_SUPPORTS_SSSE3 TRUE)
        check_cxx_compiler_flag("/arch:AVX2" COMPILER_SUPPORTS_AVX2)
        check_cxx_compiler_flag("/arch:AVX512" COMPILER_SUPPORTS_AVX512F)
        set(SSE2_FLAGS "")  # Default on x64
        set(SSSE3_FLAGS "") # Default on x64
        set(AVX2_FLAGS /arch:AVX2)
        set(AVX512_FLAGS /arch:AVX512)
    endif()

    if(COMPILER_SUPPORTS_SSE2)
        message(STATUS "Compiler supports SSE2 - will build SSE2 converters")
    endif()
    if(COMPILER_SUPPORTS_SSSE3)
        message(STATUS "Compiler supports SSSE3 - will build SSSE3 converters")
    endif()
    if(COMPILER_SUPPORTS_AVX2)
        message(STATUS "Compiler supports AVX2 - will build AVX2 converters")
    endif()
    if(COMPILER_SUPPORTS_AVX512F)
        message(STATUS "Compiler supports AVX512F - will build AVX512 converters")
    endif()
endif()

########################################################################
# x86 SIMD converter sources
#
# All SIMD variants are compiled unconditionally (if the compiler supports
# the flags). At runtime, each converter checks if the CPU supports the
# required instruction set before registering itself.
########################################################################

if(COMPILER_SUPPORTS_SSE2)
    set(convert_with_sse2_sources
        ${CMAKE_CURRENT_SOURCE_DIR}/sse2_sc16_to_sc16.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/sse2_sc16_to_fc64.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/sse2_sc16_to_fc32.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/sse2_sc8_to_fc64.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/sse2_sc8_to_fc32.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/sse2_fc64_to_sc16.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/sse2_fc32_to_sc16.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/sse2_fc64_to_sc8.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/sse2_fc32_to_sc8.cpp
    )
    set_source_files_properties(
        ${convert_with_sse2_sources}
        PROPERTIES COMPILE_FLAGS "${SSE2_FLAGS}"
    )
    LIBUHD_APPEND_SOURCES(${convert_with_sse2_sources})
endif()

if(COMPILER_SUPPORTS_SSSE3)
    set(convert_with_ssse3_sources
        ${CMAKE_CURRENT_SOURCE_DIR}/ssse3_pack_sc12.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/ssse3_unpack_sc12.cpp
    )
    set_source_files_properties(
        ${convert_with_ssse3_sources}
        PROPERTIES COMPILE_FLAGS "${SSSE3_FLAGS}"
    )
    LIBUHD_APPEND_SOURCES(${convert_with_ssse3_sources})
endif()

if(COMPILER_SUPPORTS_AVX2)
    set(convert_with_avx2_sources
        ${CMAKE_CURRENT_SOURCE_DIR}/avx2_sc16_to_sc16.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/avx2_sc16_to_fc64.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/avx2_sc16_to_fc32.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/avx2_sc8_to_fc32.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/avx2_fc64_to_sc16.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/avx2_fc32_to_sc16.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/avx2_fc64_to_sc8.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/avx2_fc32_to_sc8.cpp
    )
    set_source_files_properties(
        ${convert_with_avx2_sources}
        PROPERTIES COMPILE_FLAGS "${AVX2_FLAGS}"
    )
    LIBUHD_APPEND_SOURCES(${convert_with_avx2_sources})
endif()

########################################################################
# Check for NEON SIMD headers
########################################################################
set(NEON_SIMD_ENABLE ON CACHE BOOL
    "Use NEON SIMD instructions, if applicable")
mark_as_advanced(NEON_SIMD_ENABLE)
if(CMAKE_COMPILER_IS_GNUCXX)
    CHECK_INCLUDE_FILE_CXX(arm_neon.h HAVE_ARM_NEON_H)
endif(CMAKE_COMPILER_IS_GNUCXX)

if(NEON_SIMD_ENABLE AND HAVE_ARM_NEON_H AND
   (${CMAKE_SIZEOF_VOID_P} EQUAL 4))
    enable_language(ASM)

    LIBUHD_APPEND_SOURCES(
        ${CMAKE_CURRENT_SOURCE_DIR}/convert_with_neon.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/convert_neon.S
    )
endif()

########################################################################
# Convert types generation
########################################################################
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_BINARY_DIR})

LIBUHD_PYTHON_GEN_SOURCE(
    ${CMAKE_CURRENT_SOURCE_DIR}/gen_convert_general.py
    ${CMAKE_CURRENT_BINARY_DIR}/convert_general.cpp
)

LIBUHD_APPEND_SOURCES(
    ${CMAKE_CURRENT_SOURCE_DIR}/convert_with_tables.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/convert_impl.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/convert_item32.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/convert_pack_sc12.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/convert_unpack_sc12.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/convert_fc32_item32.cpp
)

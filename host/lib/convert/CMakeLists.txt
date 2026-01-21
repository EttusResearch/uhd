#
# Copyright 2011-2013 Ettus Research LLC
# Copyright 2018 Ettus Research, a National Instruments Company
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
# Check for SIMD headers
########################################################################

# Check for SSE2 support
check_cxx_compiler_flag("-msse2" SSE2_SUPPORTED)
if(SSE2_SUPPORTED)
    message(STATUS "SSE2 is supported")
endif(SSE2_SUPPORTED)

# Check for SSE3 support
check_cxx_compiler_flag("-msse3" SSE3_SUPPORTED)
if(SSE3_SUPPORTED)
    message(STATUS "SSE3 is supported")
    set(SSE2_SUPPORTED OFF)
endif(SSE3_SUPPORTED)

# Check for AVX2 support
check_cxx_compiler_flag("-mavx2" AVX2_SUPPORTED)
# set(AVX2_SUPPORTED OFF)
if(AVX2_SUPPORTED)
    message(STATUS "AVX2 is supported")
    # set(SSE3_SUPPORTED OFF)
endif(AVX2_SUPPORTED)

# Check for AVX2 support
check_cxx_compiler_flag("-mavx512" AVX512_SUPPORTED)
if(AVX512_SUPPORTED)
    message(STATUS "AVX512 is supported")
    set(AVX2_SUPPORTED OFF)
endif(AVX512_SUPPORTED)

if(CMAKE_COMPILER_IS_GNUCXX)
    set(SSE2_FLAGS -msse2)
    set(SSE3_FLAGS -mssse3)
    set(AVX2_FLAGS -mavx2)
    set(AVX512_FLAGS -mavx512)
elseif(MSVC)
    set(SSE2_FLAGS /arch:SSE2)
endif()

if(SSE2_SUPPORTED)
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
endif(SSE2_SUPPORTED)

if(SSE3_SUPPORTED)
    set(convert_with_ssse3_sources
        ${CMAKE_CURRENT_SOURCE_DIR}/ssse3_pack_sc12.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/ssse3_unpack_sc12.cpp
    )
    set_source_files_properties(
        ${convert_with_ssse3_sources}
        PROPERTIES COMPILE_FLAGS "${SSE3_FLAGS}"
    )
    LIBUHD_APPEND_SOURCES(${convert_with_ssse3_sources})
endif(SSE3_SUPPORTED)

if(AVX2_SUPPORTED)
    set(convert_with_avx2_sources
        ${CMAKE_CURRENT_SOURCE_DIR}/avx2_sc16_to_sc16.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/avx2_sc16_to_fc64.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/avx2_sc16_to_fc32.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/sse2_sc8_to_fc64.cpp # AVX2 conversion is not efficient as SSE2 for this case
        ${CMAKE_CURRENT_SOURCE_DIR}/avx2_sc8_to_fc32.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/avx2_fc64_to_sc16.cpp 
        ${CMAKE_CURRENT_SOURCE_DIR}/avx2_fc32_to_sc16.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/avx2_fc64_to_sc8.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/avx2_fc32_to_sc8.cpp
    )
    set_source_files_properties(
        ${convert_with_avx2_sources}
        PROPERTIES COMPILE_FLAGS "${AVX2_FLAGS} ${SSE2_FLAGS}"
    )
    LIBUHD_APPEND_SOURCES(${convert_with_avx2_sources})
endif(AVX2_SUPPORTED)

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

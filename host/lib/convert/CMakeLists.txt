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
message(STATUS "")

########################################################################
# Check for SSE2 SIMD headers
########################################################################
if(CMAKE_COMPILER_IS_GNUCXX)
    set(EMMINTRIN_FLAGS -msse2)
    set(TMMINTRIN_FLAGS -mssse3)
elseif(MSVC)
    set(EMMINTRIN_FLAGS /arch:SSE2)
endif()

set(CMAKE_REQUIRED_FLAGS ${EMMINTRIN_FLAGS})
CHECK_INCLUDE_FILE_CXX(emmintrin.h HAVE_EMMINTRIN_H)
unset(CMAKE_REQUIRED_FLAGS)

if(ENABLE_SSSE3)
set(CMAKE_REQUIRED_FLAGS ${TMMINTRIN_FLAGS})
CHECK_INCLUDE_FILE_CXX(tmmintrin.h HAVE_TMMINTRIN_H)
unset(CMAKE_REQUIRED_FLAGS)
endif(ENABLE_SSSE3)

if(HAVE_EMMINTRIN_H)
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
        PROPERTIES COMPILE_FLAGS "${EMMINTRIN_FLAGS}"
    )
    LIBUHD_APPEND_SOURCES(${convert_with_sse2_sources})
endif(HAVE_EMMINTRIN_H)

if(HAVE_TMMINTRIN_H)
    set(convert_with_ssse3_sources
        ${CMAKE_CURRENT_SOURCE_DIR}/ssse3_pack_sc12.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/ssse3_unpack_sc12.cpp
    )
    set_source_files_properties(
        ${convert_with_ssse3_sources}
        PROPERTIES COMPILE_FLAGS "${TMMINTRIN_FLAGS}"
    )
    LIBUHD_APPEND_SOURCES(${convert_with_ssse3_sources})
endif(HAVE_TMMINTRIN_H)

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

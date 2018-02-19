#
# Copyright 2011-2013 Ettus Research LLC
# Copyright 2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0
#

########################################################################
# This file included, use CMake directory variables
########################################################################
INCLUDE(CheckIncludeFileCXX)
MESSAGE(STATUS "")

########################################################################
# Check for SSE2 SIMD headers
########################################################################
IF(CMAKE_COMPILER_IS_GNUCXX)
    SET(EMMINTRIN_FLAGS -msse2)
    SET(TMMINTRIN_FLAGS -mssse3)
ELSEIF(MSVC)
    SET(EMMINTRIN_FLAGS /arch:SSE2)
ENDIF()

SET(CMAKE_REQUIRED_FLAGS ${EMMINTRIN_FLAGS})
CHECK_INCLUDE_FILE_CXX(emmintrin.h HAVE_EMMINTRIN_H)
SET(CMAKE_REQUIRED_FLAGS)

IF(ENABLE_SSSE3)
SET(CMAKE_REQUIRED_FLAGS ${TMMINTRIN_FLAGS})
CHECK_INCLUDE_FILE_CXX(tmmintrin.h HAVE_TMMINTRIN_H)
SET(CMAKE_REQUIRED_FLAGS)
ENDIF(ENABLE_SSSE3)

IF(HAVE_EMMINTRIN_H)
    SET(convert_with_sse2_sources
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
    SET_SOURCE_FILES_PROPERTIES(
        ${convert_with_sse2_sources}
        PROPERTIES COMPILE_FLAGS "${EMMINTRIN_FLAGS}"
    )
    LIBUHD_APPEND_SOURCES(${convert_with_sse2_sources})
ENDIF(HAVE_EMMINTRIN_H)

IF(HAVE_TMMINTRIN_H)
    SET(convert_with_ssse3_sources
        ${CMAKE_CURRENT_SOURCE_DIR}/ssse3_pack_sc12.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/ssse3_unpack_sc12.cpp
    )
    SET_SOURCE_FILES_PROPERTIES(
        ${convert_with_ssse3_sources}
        PROPERTIES COMPILE_FLAGS "${TMMINTRIN_FLAGS}"
    )
    LIBUHD_APPEND_SOURCES(${convert_with_ssse3_sources})
ENDIF(HAVE_TMMINTRIN_H)

########################################################################
# Check for NEON SIMD headers
########################################################################
IF(CMAKE_COMPILER_IS_GNUCXX)
    CHECK_INCLUDE_FILE_CXX(arm_neon.h HAVE_ARM_NEON_H)
ENDIF(CMAKE_COMPILER_IS_GNUCXX)

IF(HAVE_ARM_NEON_H AND (${CMAKE_SIZEOF_VOID_P} EQUAL 4))
    ENABLE_LANGUAGE(ASM)

    LIBUHD_APPEND_SOURCES(
        ${CMAKE_CURRENT_SOURCE_DIR}/convert_with_neon.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/convert_neon.S
    )
ENDIF()

########################################################################
# Convert types generation
########################################################################
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR})

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

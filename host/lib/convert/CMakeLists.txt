#
# Copyright 2011 Ettus Research LLC
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#

########################################################################
# This file included, use CMake directory variables
########################################################################
INCLUDE(CheckIncludeFileCXX)
MESSAGE(STATUS "")

########################################################################
# Check for SIMD headers
########################################################################
CHECK_INCLUDE_FILE_CXX(emmintrin.h HAVE_EMMINTRIN_H)
IF(HAVE_EMMINTRIN_H)
    LIBUHD_APPEND_SOURCES(
        ${CMAKE_CURRENT_SOURCE_DIR}/convert_with_sse2.cpp
    )
ENDIF(HAVE_EMMINTRIN_H)

CHECK_INCLUDE_FILE_CXX(arm_neon.h HAVE_ARM_NEON_H)
IF(HAVE_ARM_NEON_H)
    LIBUHD_APPEND_SOURCES(
        ${CMAKE_CURRENT_SOURCE_DIR}/convert_with_neon.cpp
    )
ENDIF(HAVE_ARM_NEON_H)

########################################################################
# Convert types generation
########################################################################
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR})

LIBUHD_PYTHON_GEN_SOURCE(
    ${CMAKE_CURRENT_SOURCE_DIR}/gen_convert_pred.py
    ${CMAKE_CURRENT_BINARY_DIR}/convert_pred.hpp
)

INCLUDE(AddFileDependencies)
ADD_FILE_DEPENDENCIES(
    ${CMAKE_CURRENT_SOURCE_DIR}/convert_impl.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/convert_pred.hpp
)

LIBUHD_PYTHON_GEN_SOURCE(
    ${CMAKE_CURRENT_SOURCE_DIR}/gen_convert_general.py
    ${CMAKE_CURRENT_BINARY_DIR}/convert_general.cpp
)

LIBUHD_APPEND_SOURCES(
    ${CMAKE_CURRENT_SOURCE_DIR}/convert_impl.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/convert_general.cpp
)

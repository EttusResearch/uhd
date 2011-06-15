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
# Check for SSE2 SIMD headers
########################################################################
IF(CMAKE_COMPILER_IS_GNUCXX)
    SET(EMMINTRIN_FLAGS -msse2)
    SET(NEON_FLAGS "-mfloat-abi=softfp -mfpu=neon")
ELSEIF(MSVC)
    SET(EMMINTRIN_FLAGS /arch:SSE2)
ENDIF()

SET(CMAKE_REQUIRED_FLAGS ${EMMINTRIN_FLAGS})
CHECK_INCLUDE_FILE_CXX(emmintrin.h HAVE_EMMINTRIN_H)
UNSET(CMAKE_REQUIRED_FLAGS)

IF(HAVE_EMMINTRIN_H)
    SET_SOURCE_FILES_PROPERTIES(
        ${CMAKE_CURRENT_SOURCE_DIR}/convert_with_sse2.cpp
        PROPERTIES COMPILE_FLAGS "${EMMINTRIN_FLAGS}"
    )
    LIBUHD_APPEND_SOURCES(
        ${CMAKE_CURRENT_SOURCE_DIR}/convert_with_sse2.cpp
    )
ENDIF(HAVE_EMMINTRIN_H)

########################################################################
# Check for NEON SIMD headers
########################################################################
SET(CMAKE_REQUIRED_FLAGS ${NEON_FLAGS})
CHECK_INCLUDE_FILE_CXX(arm_neon.h HAVE_ARM_NEON_H)
UNSET(CMAKE_REQUIRED_FLAGS)
if(HAVE_ARM_NEON_H)
    MESSAGE(STATUS "Enabling NEON support")
    SET_SOURCE_FILES_PROPERTIES(
        ${CMAKE_CURRENT_SOURCE_DIR}/convert_with_neon.cpp
        PROPERTIES COMPILE_FLAGS "${NEON_FLAGS}"
    )
    LIBUHD_APPEND_SOURCES(
        ${CMAKE_CURRENT_SOURCE_DIR}/convert_with_neon.cpp
    )
else(HAVE_ARM_NEON_H)
    MESSAGE(STATUS "Disabling NEON support")
endif(HAVE_ARM_NEON_H)
########################################################################
# Look for Orc support
########################################################################
FIND_PACKAGE(PkgConfig)
IF(PKG_CONFIG_FOUND)
PKG_CHECK_MODULES(ORC "orc-0.4")
ENDIF(PKG_CONFIG_FOUND)

FIND_PROGRAM(ORCC_EXECUTABLE orcc)

IF(ORC_FOUND AND ORCC_EXECUTABLE)
    INCLUDE_DIRECTORIES(${ORC_INCLUDE_DIRS})
    LINK_DIRECTORIES(${ORC_LIBRARY_DIRS})
    ENABLE_LANGUAGE(C)

    SET(orcc_src ${CMAKE_CURRENT_SOURCE_DIR}/convert_orc.orc)

    GET_FILENAME_COMPONENT(orc_file_name_we ${orcc_src} NAME_WE)
    SET(orcc_gen ${CMAKE_CURRENT_BINARY_DIR}/${orc_file_name_we}.c)
    MESSAGE(STATUS "Orc found, enabling Orc support")
    ADD_CUSTOM_COMMAND(
        COMMAND ${ORCC_EXECUTABLE} --implementation -o ${orcc_gen} ${orcc_src}
        DEPENDS ${orcc_src} OUTPUT ${orcc_gen}
    )
    LIBUHD_APPEND_SOURCES(${orcc_gen})
    LIBUHD_APPEND_SOURCES(
        ${CMAKE_CURRENT_SOURCE_DIR}/convert_with_orc.cpp
    )
    LIBUHD_APPEND_LIBS(${ORC_LIBRARIES})
ELSE()
    MESSAGE(STATUS "Orc not found, disabling orc support...")
ENDIF(ORC_FOUND AND ORCC_EXECUTABLE)

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
)

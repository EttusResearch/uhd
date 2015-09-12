#
# Copyright 2011-2013,2015 Ettus Research LLC
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
# Setup defines for high resolution timing
########################################################################
MESSAGE(STATUS "")
MESSAGE(STATUS "Configuring high resolution timing...")
INCLUDE(CheckCXXSourceCompiles)

SET(CMAKE_REQUIRED_LIBRARIES -lrt)
CHECK_CXX_SOURCE_COMPILES("
    #include <ctime>
    int main(){
        timespec ts;
        return clock_gettime(CLOCK_MONOTONIC, &ts);
    }
    " HAVE_CLOCK_GETTIME
)
SET(CMAKE_REQUIRED_LIBRARIES)

INCLUDE(CheckCXXSourceCompiles)
CHECK_CXX_SOURCE_COMPILES("
    #include <mach/mach_time.h>
    int main(){
        mach_timebase_info_data_t info;
        mach_timebase_info(&info);
        mach_absolute_time();
        return 0;
    }
    " HAVE_MACH_ABSOLUTE_TIME
)

CHECK_CXX_SOURCE_COMPILES("
    #include <Windows.h>
    int main(){
        LARGE_INTEGER value;
        QueryPerformanceCounter(&value);
        QueryPerformanceFrequency(&value);
        return 0;
    }
    " HAVE_QUERY_PERFORMANCE_COUNTER
)

IF(HAVE_CLOCK_GETTIME)
    MESSAGE(STATUS "  High resolution timing supported through clock_gettime.")
    SET(TIME_SPEC_DEFS HAVE_CLOCK_GETTIME)
    LIBUHD_APPEND_LIBS("-lrt")
ELSEIF(HAVE_MACH_ABSOLUTE_TIME)
    MESSAGE(STATUS "  High resolution timing supported through mach_absolute_time.")
    SET(TIME_SPEC_DEFS HAVE_MACH_ABSOLUTE_TIME)
ELSEIF(HAVE_QUERY_PERFORMANCE_COUNTER)
    MESSAGE(STATUS "  High resolution timing supported through QueryPerformanceCounter.")
    SET(TIME_SPEC_DEFS HAVE_QUERY_PERFORMANCE_COUNTER)
ELSE()
    MESSAGE(STATUS "  High resolution timing supported though microsec_clock.")
    SET(TIME_SPEC_DEFS HAVE_MICROSEC_CLOCK)
ENDIF()

SET_SOURCE_FILES_PROPERTIES(
    ${CMAKE_CURRENT_SOURCE_DIR}/time_spec.cpp
    PROPERTIES COMPILE_DEFINITIONS "${TIME_SPEC_DEFS}"
)

########################################################################
# This file included, use CMake directory variables
########################################################################
LIBUHD_APPEND_SOURCES(
    ${CMAKE_CURRENT_SOURCE_DIR}/device_addr.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/mac_addr.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/metadata.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ranges.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/sensors.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/serial.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/sid.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/time_spec.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tune.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/types.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/wb_iface.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/filters.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/byte_vector.cpp
)

IF(ENABLE_C_API)
    LIBUHD_APPEND_SOURCES(
        ${CMAKE_CURRENT_SOURCE_DIR}/metadata_c.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/ranges_c.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/sensors_c.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/string_vector_c.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tune_c.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/usrp_info_c.cpp
    )
ENDIF()

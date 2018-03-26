#
# Copyright 2010-2011 Ettus Research LLC
# Copyright 2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0
#

########################################################################
# This file included, use CMake directory variables
########################################################################

########################################################################
# Setup defines for process scheduling
########################################################################
MESSAGE(STATUS "")
MESSAGE(STATUS "Configuring priority scheduling...")
INCLUDE(CheckCXXSourceCompiles)

CHECK_CXX_SOURCE_COMPILES("
    #include <pthread.h>
    int main(){
        struct sched_param sp;
        pthread_setschedparam(pthread_self(), SCHED_RR, &sp);
        return 0;
    }
    " HAVE_PTHREAD_SETSCHEDPARAM
)

IF(CYGWIN)
    #SCHED_RR non-operational on cygwin
    SET(HAVE_PTHREAD_SETSCHEDPARAM False)
ENDIF(CYGWIN)

CHECK_CXX_SOURCE_COMPILES("
    #include <windows.h>
    int main(){
        SetThreadPriority(GetCurrentThread(), 0);
        SetPriorityClass(GetCurrentProcess(), 0);
        return 0;
    }
    " HAVE_WIN_SETTHREADPRIORITY
)



IF(HAVE_PTHREAD_SETSCHEDPARAM)
    MESSAGE(STATUS "  Priority scheduling supported through pthread_setschedparam.")
    LIST(APPEND THREAD_PRIO_DEFS HAVE_PTHREAD_SETSCHEDPARAM)
    LIBUHD_APPEND_LIBS(pthread)
ELSEIF(HAVE_WIN_SETTHREADPRIORITY)
    MESSAGE(STATUS "  Priority scheduling supported through windows SetThreadPriority.")
    SET(THREAD_PRIO_DEFS HAVE_WIN_SETTHREADPRIORITY)
ELSE()
    MESSAGE(STATUS "  Priority scheduling not supported.")
    SET(THREAD_PRIO_DEFS HAVE_THREAD_PRIO_DUMMY)
ENDIF()

SET(CMAKE_REQUIRED_LIBRARIES "pthread")

CHECK_CXX_SOURCE_COMPILES("
    #include <pthread.h>
    int main(){
        pthread_t pt;
        const char* pt_name = \"test\";
        pthread_setname_np(pt, pt_name);
        return 0;
    }
    " HAVE_PTHREAD_SETNAME
)

IF(CYGWIN)
    #SCHED_RR non-operational on cygwin
    SET(HAVE_PTHREAD_SETNAME False)
ENDIF(CYGWIN)

IF(HAVE_PTHREAD_SETNAME)
    MESSAGE(STATUS "  Setting thread names is supported through pthread_setname_np.")
    LIST(APPEND THREAD_PRIO_DEFS HAVE_PTHREAD_SETNAME)
    LIBUHD_APPEND_LIBS(pthread)
ELSE()
    MESSAGE(STATUS "  Setting thread names is not supported.")
    LIST(APPEND THREAD_PRIO_DEFS HAVE_THREAD_SETNAME_DUMMY)
ENDIF()


SET_SOURCE_FILES_PROPERTIES(
    ${CMAKE_CURRENT_SOURCE_DIR}/thread.cpp
    PROPERTIES COMPILE_DEFINITIONS "${THREAD_PRIO_DEFS}"
)

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
    SET(SYSTEM_TIME_DEFS HAVE_CLOCK_GETTIME)
    LIBUHD_APPEND_LIBS("-lrt")
ELSEIF(HAVE_MACH_ABSOLUTE_TIME)
    MESSAGE(STATUS "  High resolution timing supported through mach_absolute_time.")
    SET(SYSTEM_TIME_DEFS HAVE_MACH_ABSOLUTE_TIME)
ELSEIF(HAVE_QUERY_PERFORMANCE_COUNTER)
    MESSAGE(STATUS "  High resolution timing supported through QueryPerformanceCounter.")
    SET(SYSTEM_TIME_DEFS HAVE_QUERY_PERFORMANCE_COUNTER)
ELSE()
    MESSAGE(STATUS "  High resolution timing supported though microsec_clock.")
    SET(SYSTEM_TIME_DEFS HAVE_MICROSEC_CLOCK)
ENDIF()

SET_SOURCE_FILES_PROPERTIES(
    ${CMAKE_CURRENT_SOURCE_DIR}/system_time.cpp
    PROPERTIES COMPILE_DEFINITIONS "${SYSTEM_TIME_DEFS}"
)

########################################################################
# Setup defines for module loading
########################################################################
MESSAGE(STATUS "")
MESSAGE(STATUS "Configuring module loading...")
INCLUDE(CheckCXXSourceCompiles)

SET(CMAKE_REQUIRED_LIBRARIES ${CMAKE_DL_LIBS})
CHECK_CXX_SOURCE_COMPILES("
    #include <dlfcn.h>
    int main(){
        dlopen(0, 0);
        return 0;
    }
    " HAVE_DLOPEN
)
SET(CMAKE_REQUIRED_LIBRARIES)

CHECK_CXX_SOURCE_COMPILES("
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
    int main(){
        LoadLibrary(0);
        return 0;
    }
    " HAVE_LOAD_LIBRARY
)

IF(HAVE_DLOPEN)
    MESSAGE(STATUS "  Module loading supported through dlopen.")
    SET(LOAD_MODULES_DEFS HAVE_DLOPEN)
    LIBUHD_APPEND_LIBS(${CMAKE_DL_LIBS})
ELSEIF(HAVE_LOAD_LIBRARY)
    MESSAGE(STATUS "  Module loading supported through LoadLibrary.")
    SET(LOAD_MODULES_DEFS HAVE_LOAD_LIBRARY)
ELSE()
    MESSAGE(STATUS "  Module loading not supported.")
    SET(LOAD_MODULES_DEFS HAVE_LOAD_MODULES_DUMMY)
ENDIF()

SET_SOURCE_FILES_PROPERTIES(
    ${CMAKE_CURRENT_SOURCE_DIR}/load_modules.cpp
    PROPERTIES COMPILE_DEFINITIONS "${LOAD_MODULES_DEFS}"
)

########################################################################
# Define UHD_PKG_DATA_PATH for paths.cpp
########################################################################
FILE(TO_NATIVE_PATH "${CMAKE_INSTALL_PREFIX}" UHD_PKG_PATH)
STRING(REPLACE "\\" "\\\\" UHD_PKG_PATH "${UHD_PKG_PATH}")

STRING(REGEX MATCH ".*/.*" SLASH "${LIB_SUFFIX}")
IF(SLASH STREQUAL "")
    SET(UHD_LIB_DIR "lib${LIB_SUFFIX}")
ELSE()
    SET(UHD_LIB_DIR "lib")
ENDIF(SLASH STREQUAL "")

SET_SOURCE_FILES_PROPERTIES(
    ${CMAKE_CURRENT_SOURCE_DIR}/paths.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/images.cpp
    PROPERTIES COMPILE_DEFINITIONS
    "UHD_PKG_PATH=\"${UHD_PKG_PATH}\";UHD_LIB_DIR=\"${UHD_LIB_DIR}\""
)

########################################################################
# Append sources
########################################################################
LIBUHD_APPEND_SOURCES(
    ${CMAKE_CURRENT_SOURCE_DIR}/csv.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/config_parser.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/compat_check.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/eeprom_utils.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/gain_group.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ihex.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/load_modules.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/log.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/paths.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/pathslib.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/platform.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/prefs.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/static.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/system_time.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tasks.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/thread.cpp
)

IF(ENABLE_C_API)
    LIBUHD_APPEND_SOURCES(
        ${CMAKE_CURRENT_SOURCE_DIR}/log_c.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/thread_priority_c.cpp
    )
ENDIF(ENABLE_C_API)

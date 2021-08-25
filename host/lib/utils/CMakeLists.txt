#
# Copyright 2010-2011 Ettus Research LLC
# Copyright 2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

########################################################################
# This file included, use CMake directory variables
########################################################################

INCLUDE_SUBDIRECTORY(chdr)

########################################################################
# Setup defines for process scheduling
########################################################################
message(STATUS "")
message(STATUS "Configuring priority scheduling...")
include(CheckCXXSourceCompiles)

CHECK_CXX_SOURCE_COMPILES("
    #include <pthread.h>
    int main(){
        struct sched_param sp;
        pthread_setschedparam(pthread_self(), SCHED_RR, &sp);
        return 0;
    }
    " HAVE_PTHREAD_SETSCHEDPARAM
)

if(CYGWIN)
    #SCHED_RR non-operational on cygwin
    set(HAVE_PTHREAD_SETSCHEDPARAM False)
endif(CYGWIN)

CHECK_CXX_SOURCE_COMPILES("
    #include <windows.h>
    int main(){
        SetThreadPriority(GetCurrentThread(), 0);
        SetPriorityClass(GetCurrentProcess(), 0);
        return 0;
    }
    " HAVE_WIN_SETTHREADPRIORITY
)

if(HAVE_PTHREAD_SETSCHEDPARAM)
    message(STATUS "  Priority scheduling supported through pthread_setschedparam.")
    list(APPEND THREAD_PRIO_DEFS HAVE_PTHREAD_SETSCHEDPARAM)
    LIBUHD_APPEND_LIBS(pthread)
elseif(HAVE_WIN_SETTHREADPRIORITY)
    message(STATUS "  Priority scheduling supported through windows SetThreadPriority.")
    set(THREAD_PRIO_DEFS HAVE_WIN_SETTHREADPRIORITY)
else()
    message(STATUS "  Priority scheduling not supported.")
    set(THREAD_PRIO_DEFS HAVE_THREAD_PRIO_DUMMY)
endif()

########################################################################
# Setup defines for thread naming
########################################################################

set(CMAKE_REQUIRED_LIBRARIES "pthread")
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
unset(CMAKE_REQUIRED_LIBRARIES)

if(CYGWIN)
    #SCHED_RR non-operational on cygwin
    set(HAVE_PTHREAD_SETNAME False)
endif(CYGWIN)

if(HAVE_PTHREAD_SETNAME)
    message(STATUS "  Setting thread names is supported through pthread_setname_np.")
    list(APPEND THREAD_PRIO_DEFS HAVE_PTHREAD_SETNAME)
    LIBUHD_APPEND_LIBS(pthread)
else()
    message(STATUS "  Setting thread names is not supported.")
    list(APPEND THREAD_PRIO_DEFS HAVE_THREAD_SETNAME_DUMMY)
endif()

########################################################################
# Setup defines for thread affinitizing
########################################################################

set(CMAKE_REQUIRED_LIBRARIES "pthread")
CHECK_CXX_SOURCE_COMPILES("
    #include <pthread.h>
    int main(){
        pthread_t pt;
        cpu_set_t cs;
        pthread_setaffinity_np(pt, sizeof(cpu_set_t), &cs);
        return 0;
    }
    " HAVE_PTHREAD_SETAFFINITY_NP
)
unset(CMAKE_REQUIRED_LIBRARIES)

if(CYGWIN)
    set(HAVE_PTHREAD_SETAFFINITY_NP False)
endif(CYGWIN)

CHECK_CXX_SOURCE_COMPILES("
    #include <windows.h>
    int main(){
        SetThreadAffinityMask(GetCurrentThread(), 0);
        return 0;
    }
    " HAVE_WIN_SETTHREADAFFINITYMASK
)

if(HAVE_PTHREAD_SETAFFINITY_NP)
    message(STATUS "  Setting thread affinity is supported through pthread_setaffinity_np.")
    list(APPEND THREAD_PRIO_DEFS HAVE_PTHREAD_SETAFFINITY_NP)
    LIBUHD_APPEND_LIBS(pthread)
elseif(HAVE_WIN_SETTHREADPRIORITY)
    message(STATUS "  Setting thread affinity is supported through windows SetThreadAffinityMask.")
    list(APPEND THREAD_PRIO_DEFS HAVE_WIN_SETTHREADAFFINITYMASK)
else()
    message(STATUS "  Setting thread affinity is not supported.")
    list(APPEND THREAD_PRIO_DEFS HAVE_THREAD_SETAFFINITY_DUMMY)
endif()

set_source_files_properties(
    ${CMAKE_CURRENT_SOURCE_DIR}/thread.cpp
    PROPERTIES COMPILE_DEFINITIONS "${THREAD_PRIO_DEFS}"
)

########################################################################
# Setup defines for module loading
########################################################################
message(STATUS "")
message(STATUS "Configuring module loading...")

set(CMAKE_REQUIRED_LIBRARIES ${CMAKE_DL_LIBS})
CHECK_CXX_SOURCE_COMPILES("
    #include <dlfcn.h>
    int main(){
        dlopen(0, 0);
        return 0;
    }
    " HAVE_DLOPEN
)
unset(CMAKE_REQUIRED_LIBRARIES)

CHECK_CXX_SOURCE_COMPILES("
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
    int main(){
        LoadLibrary(0);
        return 0;
    }
    " HAVE_LOAD_LIBRARY
)

if(HAVE_DLOPEN)
    message(STATUS "  Module loading supported through dlopen.")
    set(LOAD_MODULES_DEFS HAVE_DLOPEN)
    LIBUHD_APPEND_LIBS(${CMAKE_DL_LIBS})
elseif(HAVE_LOAD_LIBRARY)
    message(STATUS "  Module loading supported through LoadLibrary.")
    set(LOAD_MODULES_DEFS HAVE_LOAD_LIBRARY)
else()
    message(STATUS "  Module loading not supported.")
    set(LOAD_MODULES_DEFS HAVE_LOAD_MODULES_DUMMY)
endif()

set_source_files_properties(
    ${CMAKE_CURRENT_SOURCE_DIR}/load_modules.cpp
    PROPERTIES COMPILE_DEFINITIONS "${LOAD_MODULES_DEFS}"
)

########################################################################
# Check if we dynamically need to link against libatomic
# "Guerney, deploy the family atomics!"
########################################################################
message(STATUS "")
message(STATUS "Configuring atomics support...")
include(UHDAtomics)
CHECK_ATOMICS_LIB_REQUIRED(NEED_LIBATOMIC)
if(NEED_LIBATOMIC)
    message(STATUS "  Atomics support is provided by separate libatomic.")
    LIBUHD_APPEND_LIBS("atomic")
else()
    message(STATUS "  Atomics support is built-in, no linking required.")
endif()

########################################################################
# Define UHD_PKG_DATA_PATH for paths.cpp
########################################################################
file(TO_NATIVE_PATH "${CMAKE_INSTALL_PREFIX}" UHD_PKG_PATH)
string(REPLACE "\\" "\\\\" UHD_PKG_PATH "${UHD_PKG_PATH}")

string(REGEX MATCH ".*/.*" SLASH "${LIB_SUFFIX}")
if(SLASH STREQUAL "")
    set(UHD_LIB_DIR "lib${LIB_SUFFIX}")
else()
    set(UHD_LIB_DIR "lib")
endif(SLASH STREQUAL "")

set_source_files_properties(
    ${CMAKE_CURRENT_SOURCE_DIR}/paths.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/images.cpp
    PROPERTIES COMPILE_DEFINITIONS
    "UHD_PKG_PATH=\"${UHD_PKG_PATH}\";UHD_LIB_DIR=\"${UHD_LIB_DIR}\""
)

########################################################################
# Append sources
########################################################################
LIBUHD_APPEND_SOURCES(
    ${CMAKE_CURRENT_SOURCE_DIR}/cast.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/csv.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/config_parser.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/compat_check.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/eeprom_utils.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/gain_group.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/graph_utils.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ihex.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/load_modules.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/log.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/paths.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/pathslib.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/platform.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/prefs.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/serial_number.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/static.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/system_time.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tasks.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/thread.cpp
)

if(ENABLE_C_API)
    LIBUHD_APPEND_SOURCES(
        ${CMAKE_CURRENT_SOURCE_DIR}/log_c.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/thread_priority_c.cpp
    )
endif(ENABLE_C_API)

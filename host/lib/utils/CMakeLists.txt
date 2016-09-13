#
# Copyright 2010-2011 Ettus Research LLC
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
    SET(THREAD_PRIO_DEFS HAVE_PTHREAD_SETSCHEDPARAM)
    LIBUHD_APPEND_LIBS(pthread)
ELSEIF(HAVE_WIN_SETTHREADPRIORITY)
    MESSAGE(STATUS "  Priority scheduling supported through windows SetThreadPriority.")
    SET(THREAD_PRIO_DEFS HAVE_WIN_SETTHREADPRIORITY)
ELSE()
    MESSAGE(STATUS "  Priority scheduling not supported.")
    SET(THREAD_PRIO_DEFS HAVE_THREAD_PRIO_DUMMY)
ENDIF()

SET_SOURCE_FILES_PROPERTIES(
    ${CMAKE_CURRENT_SOURCE_DIR}/thread_priority.cpp
    PROPERTIES COMPILE_DEFINITIONS "${THREAD_PRIO_DEFS}"
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
FILE(TO_NATIVE_PATH ${CMAKE_INSTALL_PREFIX} UHD_PKG_PATH)
STRING(REPLACE "\\" "\\\\" UHD_PKG_PATH ${UHD_PKG_PATH})

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
    ${CMAKE_CURRENT_SOURCE_DIR}/gain_group.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ihex.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/load_modules.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/log.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/msg.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/paths.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/platform.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/static.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tasks.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/thread_priority.cpp
)

IF(ENABLE_C_API)
    LIBUHD_APPEND_SOURCES(
        ${CMAKE_CURRENT_SOURCE_DIR}/thread_priority_c.cpp
    )
ENDIF(ENABLE_C_API)

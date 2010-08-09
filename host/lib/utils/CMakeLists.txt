#
# Copyright 2010 Ettus Research LLC
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

#This file will be included by cmake, use absolute paths!

########################################################################
# Setup defines for process scheduling
########################################################################
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
    ADD_DEFINITIONS(-DHAVE_PTHREAD_SETSCHEDPARAM)
ELSEIF(HAVE_WIN_SETTHREADPRIORITY)
    MESSAGE(STATUS "  Priority scheduling supported through windows SetThreadPriority.")
    ADD_DEFINITIONS(-DHAVE_WIN_SETTHREADPRIORITY)
ELSE(HAVE_PTHREAD_SETSCHEDPARAM)
    MESSAGE(STATUS "  Priority scheduling not supported.")
ENDIF(HAVE_PTHREAD_SETSCHEDPARAM)

########################################################################
# Setup defines for module loading
########################################################################
MESSAGE(STATUS "Configuring module loading...")

INCLUDE(CheckIncludeFileCXX)
CHECK_INCLUDE_FILE_CXX(dlfcn.h HAVE_DLFCN_H)
CHECK_INCLUDE_FILE_CXX(windows.h HAVE_WINDOWS_H)

IF(HAVE_DLFCN_H)
    MESSAGE(STATUS "  Module loading supported through dlopen.")
    ADD_DEFINITIONS(-DHAVE_DLFCN_H)
    LIBUHD_APPEND_LIBS(${CMAKE_DL_LIBS})
ELSEIF(HAVE_WINDOWS_H)
    MESSAGE(STATUS "  Module loading supported through LoadLibrary.")
    ADD_DEFINITIONS(-DHAVE_WINDOWS_H)
ELSE(HAVE_DLFCN_H)
    MESSAGE(STATUS "  Module loading not supported.")
ENDIF(HAVE_DLFCN_H)

########################################################################
# Append sources
########################################################################
LIBUHD_APPEND_SOURCES(
    ${CMAKE_SOURCE_DIR}/lib/utils/assert.cpp
    ${CMAKE_SOURCE_DIR}/lib/utils/gain_group.cpp
    ${CMAKE_SOURCE_DIR}/lib/utils/load_modules.cpp
    ${CMAKE_SOURCE_DIR}/lib/utils/paths.cpp
    ${CMAKE_SOURCE_DIR}/lib/utils/props.cpp
    ${CMAKE_SOURCE_DIR}/lib/utils/thread_priority.cpp
    ${CMAKE_SOURCE_DIR}/lib/utils/warning.cpp
)

#
# Copyright 2015-2016 National Instruments Corp.
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

#
# We need this to be macro because GET_DIRECTORY_PROPERTY works with
# the current directory.
#
macro(UHD_LOAD_BUILD_INFO)
    message(STATUS "")
    message(STATUS "Loading build info.")

    # Build date
    if(IGNORE_BUILD_DATE)
        set(UHD_BUILD_DATE "")
    elseif(DEFINED ENV{SOURCE_DATE_EPOCH})
        EXECUTE_PROCESS(COMMAND ${PYTHON_EXECUTABLE} -c
            "import time; print(time.strftime('%a, %d %b %Y %H:%M:%S UTC', time.gmtime($ENV{SOURCE_DATE_EPOCH})))"
            OUTPUT_VARIABLE UHD_BUILD_DATE OUTPUT_STRIP_TRAILING_WHITESPACE
        )
    else()
        execute_process(COMMAND ${PYTHON_EXECUTABLE} -c
            "import time; print(time.strftime('%a, %d %b %Y %H:%M:%S', time.gmtime()))"
            OUTPUT_VARIABLE UHD_BUILD_DATE OUTPUT_STRIP_TRAILING_WHITESPACE
        )
    endif(IGNORE_BUILD_DATE)

    # Compiler name
    if(MSVC)
        if(MSVC10)
            set(UHD_C_COMPILER "MSVC 2010")
            set(UHD_CXX_COMPILER "MSVC 2010")
        elseif(MSVC11)
            set(UHD_C_COMPILER "MSVC 2012")
            set(UHD_CXX_COMPILER "MSVC 2012")
        elseif(MSVC12)
            set(UHD_C_COMPILER "MSVC 2013")
            set(UHD_CXX_COMPILER "MSVC 2013")
        elseif(MSVC14)
            set(UHD_C_COMPILER "MSVC 2015")
            set(UHD_CXX_COMPILER "MSVC 2015")
        else()
            # Go with the ugly string
            set(UHD_C_COMPILER "MSVC ${CMAKE_C_COMPILER_VERSION}")
            set(UHD_CXX_COMPILER "MSVC ${CMAKE_CXX_COMPILER_VERSION}")
        endif(MSVC10)
    else()
        set(UHD_C_COMPILER "${CMAKE_C_COMPILER_ID} ${CMAKE_C_COMPILER_VERSION}")
        set(UHD_CXX_COMPILER "${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
    endif(MSVC)

    # Compiler flags
    get_directory_property(uhd_flags COMPILE_DEFINITIONS)
    set(UHD_C_FLAGS "${uhd_flags}${CMAKE_C_FLAGS}") # CMAKE_C_FLAGS starts with a space
    set(UHD_CXX_FLAGS "${uhd_flags}${CMAKE_CXX_FLAGS}") # CMAKE_CXX_FLAGS starts with a space
    if(DEFINED ENV{SOURCE_DATE_EPOCH})
        # The variable SOURCE_DATE_EPOCH is typically used to ensure reproducible builds.
        # Reproducible builds also require the build directory to be deterministic.
        # The following commands replaces the actual build directory by "BUILD_DIR".
        cmake_path(GET CMAKE_BINARY_DIR PARENT_PATH uhd_topdir)
        string(REPLACE "${uhd_topdir}" "BUILD_DIR" UHD_C_FLAGS "${UHD_C_FLAGS}")
        string(REPLACE "${uhd_topdir}" "BUILD_DIR" UHD_CXX_FLAGS "${UHD_CXX_FLAGS}")
    endif()
endmacro(UHD_LOAD_BUILD_INFO)

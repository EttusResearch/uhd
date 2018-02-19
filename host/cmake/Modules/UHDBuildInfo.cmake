#
# Copyright 2015-2016 National Instruments Corp.
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

#
# We need this to be macro because GET_DIRECTORY_PROPERTY works with
# the current directory.
#
MACRO(UHD_LOAD_BUILD_INFO)
    MESSAGE(STATUS "")
    MESSAGE(STATUS "Loading build info.")

    # Build date
    IF(IGNORE_BUILD_DATE)
        SET(UHD_BUILD_DATE "")
    ELSE()
        EXECUTE_PROCESS(COMMAND ${PYTHON_EXECUTABLE} -c
            "import time; print(time.strftime('%a, %d %b %Y %H:%M:%S', time.gmtime()))"
            OUTPUT_VARIABLE UHD_BUILD_DATE OUTPUT_STRIP_TRAILING_WHITESPACE
        )
    ENDIF(IGNORE_BUILD_DATE)

    # Compiler name
    IF(MSVC)
        IF(MSVC10)
            SET(UHD_C_COMPILER "MSVC 2010")
            SET(UHD_CXX_COMPILER "MSVC 2010")
        ELSEIF(MSVC11)
            SET(UHD_C_COMPILER "MSVC 2012")
            SET(UHD_CXX_COMPILER "MSVC 2012")
        ELSEIF(MSVC12)
            SET(UHD_C_COMPILER "MSVC 2013")
            SET(UHD_CXX_COMPILER "MSVC 2013")
        ELSEIF(MSVC14)
            SET(UHD_C_COMPILER "MSVC 2015")
            SET(UHD_CXX_COMPILER "MSVC 2015")
        ELSE()
            # Go with the ugly string
            SET(UHD_C_COMPILER "MSVC ${CMAKE_C_COMPILER_VERSION}")
            SET(UHD_CXX_COMPILER "MSVC ${CMAKE_CXX_COMPILER_VERSION}")
        ENDIF(MSVC10)
    ELSE()
        SET(UHD_C_COMPILER "${CMAKE_C_COMPILER_ID} ${CMAKE_C_COMPILER_VERSION}")
        SET(UHD_CXX_COMPILER "${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
    ENDIF(MSVC)

    # Compiler flags
    GET_DIRECTORY_PROPERTY(uhd_flags COMPILE_DEFINITIONS)
    SET(UHD_C_FLAGS "${uhd_flags}${CMAKE_C_FLAGS}") # CMAKE_C_FLAGS starts with a space
    SET(UHD_CXX_FLAGS "${uhd_flags}${CMAKE_CXX_FLAGS}") # CMAKE_CXX_FLAGS starts with a space
ENDMACRO(UHD_LOAD_BUILD_INFO)

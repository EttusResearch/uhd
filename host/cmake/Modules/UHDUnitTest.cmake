#
# Copyright 2010-2012,2015 Ettus Research LLC
# Copyright 2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

########################################################################
# Add a unit test and setup the environment for a unit test.
# Takes the same arguments as the ADD_TEST function.
########################################################################
function(UHD_ADD_TEST test_name)

        #Ensure that the build exe also appears in the PATH.
        list(APPEND UHD_TEST_TARGET_DEPS ${ARGN})

        #We need to put the directory with the .so/.dll file in the
        #appropriate environment variable, as well as the test
        #directory itself.
        if(WIN32)
            set(UHD_TEST_LIBRARY_DIRS
                "${CMAKE_BINARY_DIR}/lib/${CMAKE_BUILD_TYPE}"
                "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_BUILD_TYPE}"
                "${Boost_LIBRARY_DIRS}"
            )
        else()
            set(UHD_TEST_LIBRARY_DIRS
                "${CMAKE_BINARY_DIR}/lib"
                "${CMAKE_CURRENT_BINARY_DIR}"
                "${Boost_LIBRARY_DIRS}"
            )
        endif(WIN32)

    file(TO_NATIVE_PATH "${UHD_TEST_LIBRARY_DIRS}" libpath)

    #http://www.cmake.org/pipermail/cmake/2009-May/029464.html
    #Replaced this add test + set environs code with the shell script generation.
    #Its nicer to be able to manually run the shell script to diagnose problems.
    #ADD_TEST(${ARGV})
    #SET_TESTS_PROPERTIES(${test_name} PROPERTIES ENVIRONMENT "${environs}")

    if(UNIX)
        set(LD_PATH_VAR "LD_LIBRARY_PATH")
        if(APPLE)
            set(LD_PATH_VAR "DYLD_LIBRARY_PATH")
        endif()

        set(binpath "${CMAKE_CURRENT_BINARY_DIR}:$PATH")
        list(APPEND libpath "$${LD_PATH_VAR}")

        #replace list separator with the path separator
        string(REPLACE ";" ":" libpath "${libpath}")
        list(APPEND environs "PATH=${binpath}" "${LD_PATH_VAR}=${libpath}" "UHD_RFNOC_DIR=${CMAKE_SOURCE_DIR}/include/uhd/rfnoc")

        #generate a bat file that sets the environment and runs the test
        if (CMAKE_CROSSCOMPILING)
                set(SHELL "/bin/sh")
        else(CMAKE_CROSSCOMPILING)
                find_program(SHELL sh)
        endif(CMAKE_CROSSCOMPILING)
        set(sh_file ${CMAKE_CURRENT_BINARY_DIR}/${test_name}_test.sh)
        file(WRITE ${sh_file} "#!${SHELL}\n")
        #each line sets an environment variable
        foreach(environ ${environs})
            file(APPEND ${sh_file} "export ${environ}\n")
        endforeach(environ)
        #load the command to run with its arguments
        foreach(arg ${ARGN})
            file(APPEND ${sh_file} "${arg} ")
        endforeach(arg)
        file(APPEND ${sh_file} "\n")

        #make the shell file executable
        execute_process(COMMAND chmod +x ${sh_file})

        add_test(${test_name} ${SHELL} ${sh_file})

    endif(UNIX)

    if(WIN32)
        list(APPEND libpath ${DLL_PATHS} "%PATH%")

        #replace list separator with the path separator (escaped)
        string(REPLACE ";" "\\;" libpath "${libpath}")
        list(APPEND environs "PATH=${libpath}" "UHD_RFNOC_DIR=${CMAKE_SOURCE_DIR}/include/uhd/rfnoc")

        #generate a bat file that sets the environment and runs the test
        set(bat_file ${CMAKE_CURRENT_BINARY_DIR}/${test_name}_test.bat)
        file(WRITE ${bat_file} "@echo off\n")
        #each line sets an environment variable
        foreach(environ ${environs})
            file(APPEND ${bat_file} "SET ${environ}\n")
        endforeach(environ)
        #load the command to run with its arguments
        foreach(arg ${ARGN})
            file(APPEND ${bat_file} "${arg} ")
        endforeach(arg)
        file(APPEND ${bat_file} "\n")

        add_test(${test_name} ${bat_file})
    endif(WIN32)

endfunction(UHD_ADD_TEST)

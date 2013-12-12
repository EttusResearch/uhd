#
# Copyright 2010-2012 Ettus Research LLC
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
# Add a unit test and setup the environment for a unit test.
# Takes the same arguments as the ADD_TEST function.
#
# Before calling set the following variables:
# UHD_TEST_TARGET_DEPS  - built targets for the library path
# UHD_TEST_LIBRARY_DIRS - directories for the library path
########################################################################
function(UHD_ADD_TEST test_name)

        #Ensure that the build exe also appears in the PATH.
        list(APPEND UHD_TEST_TARGET_DEPS ${ARGN})

        #In the land of windows, all libraries must be in the PATH.
        #Since the dependent libraries are not yet installed,
        #we must manually set them in the PATH to run tests.
        #The following appends the path of a target dependency.
        foreach(target ${UHD_TEST_TARGET_DEPS})
            get_target_property(location ${target} LOCATION)
            if(location)
                get_filename_component(path ${location} PATH)
                string(REGEX REPLACE "\\$\\(.*\\)" ${CMAKE_BUILD_TYPE} path ${path})
                list(APPEND UHD_TEST_LIBRARY_DIRS ${path})
            endif(location)
        endforeach(target)

    file(TO_NATIVE_PATH ${CMAKE_CURRENT_SOURCE_DIR} srcdir)
    file(TO_NATIVE_PATH "${UHD_TEST_LIBRARY_DIRS}" libpath) #ok to use on dir list?

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
        list(APPEND environs "PATH=${binpath}" "${LD_PATH_VAR}=${libpath}")

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
        list(APPEND environs "PATH=${libpath}")

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

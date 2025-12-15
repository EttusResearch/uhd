#
# Copyright 2010-2012,2015 Ettus Research LLC
# Copyright 2018 Ettus Research, a National Instruments Company
# Copyright 2019 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

########################################################################
# Add a unit test and setup the environment for a unit test.
# Arguments:
#   test_name - Name of the test
#   MODULE_PATH - Optional module path to set UHD_MODULE_PATH environment variable.
#                 Accepts generator expressions.
#   All other arguments are passed to the test executable
########################################################################
function(UHD_ADD_TEST test_name)
    # Parse arguments
    cmake_parse_arguments(UHD_ADD_TEST "" "MODULE_PATH" "" ${ARGN})

    # Remaining unparsed arguments are the test executable and its arguments
    set(test_args ${UHD_ADD_TEST_UNPARSED_ARGUMENTS})

        #Ensure that the build exe also appears in the PATH.
        list(APPEND UHD_TEST_TARGET_DEPS ${test_args})

        #We need to put the directory with the .so/.dll file in the
        #appropriate environment variable, as well as the test
        #directory itself.
        if(WIN32)
            set(UHD_TEST_LIBRARY_DIRS
                "${UHD_BINARY_DIR}/lib/${CMAKE_BUILD_TYPE}"
                "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_BUILD_TYPE}"
                "${Boost_LIBRARY_DIRS}"
            )
        else()
            set(UHD_TEST_LIBRARY_DIRS
                "${UHD_BINARY_DIR}/lib"
                "${CMAKE_CURRENT_BINARY_DIR}"
                "${Boost_LIBRARY_DIRS}"
            )
        endif(WIN32)

    file(TO_NATIVE_PATH "${UHD_TEST_LIBRARY_DIRS}" libpath)

    #http://www.cmake.org/pipermail/cmake/2009-May/029464.html
    #Replaced this add test + set environs code with the shell script generation.
    #Its nicer to be able to manually run the shell script to diagnose problems.
    #add_test(${ARGV})
    #set_tests_properties(${test_name} PROPERTIES ENVIRONMENT "${environs}")

    if(ENABLE_QEMU_UNITTESTS)
        # use QEMU emulator for executing test
        add_test(${test_name} ${QEMU_EXECUTABLE} -L ${QEMU_SYSROOT} ${test_args})
    elseif(UNIX)
        set(LD_PATH_VAR "LD_LIBRARY_PATH")
        if(APPLE)
            set(LD_PATH_VAR "DYLD_LIBRARY_PATH")
        endif()

        set(binpath "${CMAKE_CURRENT_BINARY_DIR}:$PATH")
        list(APPEND libpath "$${LD_PATH_VAR}")

        #replace list separator with the path separator
        string(REPLACE ";" ":" libpath "${libpath}")
        list(APPEND environs "PATH=\"${binpath}\"" "${LD_PATH_VAR}=\"${libpath}\"")

        # Add UHD_MODULE_PATH if MODULE_PATH is specified
        if(UHD_ADD_TEST_MODULE_PATH)
            list(APPEND environs "UHD_MODULE_PATH=\"${UHD_ADD_TEST_MODULE_PATH}\"")
        endif()

        #generate a shell file that sets the environment and runs the test
        if (CMAKE_CROSSCOMPILING)
                set(SHELL "/bin/sh")
        else(CMAKE_CROSSCOMPILING)
                find_program(SHELL sh)
        endif(CMAKE_CROSSCOMPILING)
        set(sh_file ${CMAKE_CURRENT_BINARY_DIR}/${test_name}_test.sh)

        # Use file(GENERATE) to properly handle generator expressions
        set(sh_content "#!${SHELL}\n")
        foreach(environ ${environs})
            set(sh_content "${sh_content}export ${environ}\n")
        endforeach(environ)
        foreach(arg ${test_args})
            set(sh_content "${sh_content}${arg} ")
        endforeach(arg)
        set(sh_content "${sh_content}\n")

        file(GENERATE OUTPUT ${sh_file} CONTENT ${sh_content})

        #make the shell file executable
        execute_process(COMMAND chmod +x ${sh_file})

        add_test(${test_name} ${SHELL} ${sh_file})

    endif(ENABLE_QEMU_UNITTESTS)

    if(WIN32)
        list(APPEND libpath ${DLL_PATHS} "%PATH%")

        #replace list separator with the path separator (escaped)
        string(REPLACE ";" "\\;" libpath "${libpath}")
        list(APPEND environs "PATH=${libpath}")

        # Add UHD_MODULE_PATH if MODULE_PATH is specified
        if(UHD_ADD_TEST_MODULE_PATH)
            list(APPEND environs "UHD_MODULE_PATH=${UHD_ADD_TEST_MODULE_PATH}")
        endif()

        #generate a bat file that sets the environment and runs the test
        set(bat_file ${CMAKE_CURRENT_BINARY_DIR}/${test_name}_test.bat)

        # Use file(GENERATE) to properly handle generator expressions
        set(bat_content "@echo off\n")
        foreach(environ ${environs})
            set(bat_content "${bat_content}SET ${environ}\n")
        endforeach(environ)
        foreach(arg ${test_args})
            set(bat_content "${bat_content}${arg} ")
        endforeach(arg)
        set(bat_content "${bat_content}\n")

        file(GENERATE OUTPUT ${bat_file} CONTENT "${bat_content}")

        add_test(${test_name} ${bat_file})
    endif(WIN32)

endfunction(UHD_ADD_TEST)

########################################################################
# Add a Python unit test
# Arguments:
#   test_name - Name of the test
#   MODULE_PATH - Optional module path to set UHD_MODULE_PATH environment variable
########################################################################
function(UHD_ADD_PYTEST test_name)
    # Parse arguments
    cmake_parse_arguments(UHD_ADD_PYTEST "" "MODULE_PATH" "" ${ARGN})
    if(ENABLE_QEMU_UNITTESTS)
        # use QEMU emulator for executing test
        add_test(NAME ${test_name}
            COMMAND ${QEMU_EXECUTABLE} -L ${QEMU_SYSROOT}
                                       ${QEMU_PYTHON_EXECUTABLE}
                                       -m unittest discover
                                       -s ${CMAKE_CURRENT_SOURCE_DIR}
                                       -p "${test_name}.*"
            WORKING_DIRECTORY "${UHD_BINARY_DIR}/python"
        )
    else()
        add_test(NAME ${test_name}
            COMMAND ${RUNTIME_PYTHON_EXECUTABLE} -m unittest discover
                                                 -s ${CMAKE_CURRENT_SOURCE_DIR}
                                                 -p "${test_name}.*"
            WORKING_DIRECTORY "${UHD_BINARY_DIR}/python"
        )
    endif(ENABLE_QEMU_UNITTESTS)
    # Include ${UHD_BINARY_DIR}/utils/ for testing the python utils
    if(APPLE)
        set(env_vars "DYLD_LIBRARY_PATH=${UHD_BINARY_DIR}/lib/:${Boost_LIBRARY_DIRS};PYTHONPATH=${UHD_BINARY_DIR}/python:${UHD_SOURCE_DIR}/tests/common:${UHD_BINARY_DIR}/utils/")
        if(UHD_ADD_PYTEST_MODULE_PATH)
            set(env_vars "${env_vars};UHD_MODULE_PATH=${UHD_ADD_PYTEST_MODULE_PATH}")
        endif()
        set_tests_properties(${test_name} PROPERTIES
            ENVIRONMENT
            "${env_vars}")
    elseif(MSVC)
        string(REPLACE ";" "\\;" WIN_PATH "$ENV{PATH}")
    # MSVC is a multi-config generator in CMake, so we must specify the config value
        set(env_vars "PATH=${WIN_PATH}\\;${UHD_BINARY_DIR}\\lib\\$<CONFIG>\\;${Boost_LIBRARY_DIRS};PYTHONPATH=${UHD_BINARY_DIR}\\python\\$<CONFIG>\\;${UHD_SOURCE_DIR}\\tests\\common\\;${UHD_BINARY_DIR}\\utils")
        if(UHD_ADD_PYTEST_MODULE_PATH)
            set(env_vars "${env_vars};UHD_MODULE_PATH=${UHD_ADD_PYTEST_MODULE_PATH}")
        endif()
        set_tests_properties(${test_name} PROPERTIES
            ENVIRONMENT
            "${env_vars}"
            )
    else()
        set(env_vars "LD_LIBRARY_PATH=${UHD_BINARY_DIR}/lib/:${Boost_LIBRARY_DIRS};PYTHONPATH=${UHD_BINARY_DIR}/python:${UHD_SOURCE_DIR}/tests/common:${UHD_BINARY_DIR}/utils/")
        if(UHD_ADD_PYTEST_MODULE_PATH)
            set(env_vars "${env_vars};UHD_MODULE_PATH=${UHD_ADD_PYTEST_MODULE_PATH}")
        endif()
        set_tests_properties(${test_name} PROPERTIES
            ENVIRONMENT
            "${env_vars}"
            )
    endif()
endfunction(UHD_ADD_PYTEST)

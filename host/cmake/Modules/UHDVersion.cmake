#
# Copyright 2010-2014,2016 Ettus Research LLC
# Copyright 2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

########################################################################
INCLUDE(UHDPython) #requires python for parsing
FIND_PACKAGE(Git QUIET)

########################################################################
# Setup Version Numbers
#  - Increment major on large-scale library changes
#  - Increment API on API changes
#  - Increment ABI on ABI changes
#  - Increment patch for bugfixes and docs
#  - set UHD_VERSION_DEVEL to true for master and development branches
########################################################################
SET(UHD_VERSION_MAJOR 4)
SET(UHD_VERSION_API   0)
SET(UHD_VERSION_ABI   0)
SET(UHD_VERSION_PATCH rfnoc)
SET(UHD_VERSION_DEVEL TRUE)

########################################################################
# If we're on a development branch, we skip the patch version
########################################################################
IF(DEFINED UHD_VERSION_PATCH_OVERRIDE)
    SET(UHD_VERSION_DEVEL FALSE)
    SET(UHD_VERSION_PATCH ${UHD_VERSION_PATCH_OVERRIDE})
ENDIF(DEFINED UHD_VERSION_PATCH_OVERRIDE)
IF(NOT DEFINED UHD_VERSION_DEVEL)
    SET(UHD_VERSION_DEVEL FALSE)
ENDIF(NOT DEFINED UHD_VERSION_DEVEL)
SET(UHD_GIT_BRANCH "")
IF(GIT_FOUND)
    EXECUTE_PROCESS(
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMAND ${GIT_EXECUTABLE} rev-parse --abbrev-ref HEAD
        OUTPUT_VARIABLE _git_branch OUTPUT_STRIP_TRAILING_WHITESPACE
        RESULT_VARIABLE _git_branch_result
    )
    IF(_git_branch_result EQUAL 0)
        SET(UHD_GIT_BRANCH ${_git_branch})
        IF(UHD_GIT_BRANCH STREQUAL "maint")
            MESSAGE(STATUS "Operating on maint branch (stable).")
	    SET(UHD_VERSION_DEVEL FALSE)
        ELSEIF(UHD_GIT_BRANCH STREQUAL "master")
            MESSAGE(STATUS "Operating on master branch.")
            SET(UHD_VERSION_DEVEL TRUE)
        ELSE()
            MESSAGE(STATUS "Working off of feature or development branch. Updating version number.")
            EXECUTE_PROCESS(
                COMMAND ${PYTHON_EXECUTABLE} -c "print('${_git_branch}'.replace('/', '-'))"
                OUTPUT_VARIABLE _git_safe_branch OUTPUT_STRIP_TRAILING_WHITESPACE
            )
            SET(UHD_VERSION_PATCH ${_git_safe_branch})
            SET(UHD_VERSION_DEVEL TRUE)
        ENDIF()
    ELSE()
        MESSAGE(STATUS "Could not determine git branch. Probably building from tarball.")
    ENDIF()
ELSE(GIT_FOUND)
    MESSAGE(WARNING "Could not detect git executable! Could not determine exact version of UHD!")
ENDIF(GIT_FOUND)

########################################################################
# Version information discovery through git log
########################################################################

#grab the git ref id for the current head
EXECUTE_PROCESS(
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMAND ${GIT_EXECUTABLE} describe --always --abbrev=8 --long
    OUTPUT_VARIABLE _git_describe OUTPUT_STRIP_TRAILING_WHITESPACE
    RESULT_VARIABLE _git_describe_result
)

#only set the build info on success
IF(_git_describe_result EQUAL 0)
    IF(NOT UHD_GIT_COUNT)
        EXECUTE_PROCESS(
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            COMMAND ${PYTHON_EXECUTABLE} -c "
try:
    print('${_git_describe}'.split('-')[-2])
except IndexError:
    print('0')
"
            OUTPUT_VARIABLE UHD_GIT_COUNT OUTPUT_STRIP_TRAILING_WHITESPACE
        )
    ENDIF()
    IF(NOT UHD_GIT_HASH)
        EXECUTE_PROCESS(
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            COMMAND ${PYTHON_EXECUTABLE} -c "
try:
    print('${_git_describe}'.split('-')[-1])
except IndexError:
    print('unknown')
"
             OUTPUT_VARIABLE UHD_GIT_HASH OUTPUT_STRIP_TRAILING_WHITESPACE
        )
    ENDIF()
ENDIF()

## Set default values if all fails. Make sure they're identical to the ones above.
IF(NOT UHD_GIT_COUNT)
    SET(UHD_GIT_COUNT "0")
ENDIF()

IF(NOT UHD_GIT_HASH)
    SET(UHD_GIT_HASH "unknown")
ENDIF()

IF(UHD_RELEASE_MODE)
    SET(UHD_GIT_HASH ${UHD_RELEASE_MODE})

    #Ignore UHD_GIT_COUNT in UHD_VERSION if the string 'release' is in UHD_RELEASE_MODE
    EXECUTE_PROCESS(
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMAND ${PYTHON_EXECUTABLE} -c "print ('release' in '${UHD_RELEASE_MODE}') or ('rc' in '${UHD_RELEASE_MODE}')"
        OUTPUT_VARIABLE TRIM_UHD_VERSION OUTPUT_STRIP_TRAILING_WHITESPACE
    )
ENDIF()


########################################################################
IF(DEFINED UHD_VERSION)
    SET(UHD_VERSION "${UHD_VERSION}" CACHE STRING "Set UHD_VERSION to a custom value")
ELSEIF(TRIM_UHD_VERSION STREQUAL "True")
    SET(UHD_VERSION "${UHD_VERSION_MAJOR}.${UHD_VERSION_API}.${UHD_VERSION_ABI}.${UHD_VERSION_PATCH}-${UHD_GIT_HASH}")
ELSE()
    SET(UHD_VERSION "${UHD_VERSION_MAJOR}.${UHD_VERSION_API}.${UHD_VERSION_ABI}.${UHD_VERSION_PATCH}-${UHD_GIT_COUNT}-${UHD_GIT_HASH}")
ENDIF()

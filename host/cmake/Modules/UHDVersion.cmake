#
# Copyright 2010-2014,2016 Ettus Research LLC
# Copyright 2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

########################################################################
include(UHDPython) #requires python for parsing
find_package(Git QUIET)

########################################################################
# Setup Version Numbers
#  - Increment major on large-scale library changes
#  - Increment API on API changes
#  - Increment ABI on ABI changes
#  - Increment patch for bugfixes and docs
#    (but use 'git' for master to represent 'ahead of the latest stable
#     release)
#  - set UHD_VERSION_DEVEL to true for master and development branches
########################################################################
set(UHD_VERSION_MAJOR 4)
set(UHD_VERSION_API   3)
set(UHD_VERSION_ABI   0)
set(UHD_VERSION_PATCH 0)
set(UHD_VERSION_DEVEL TRUE)

########################################################################
# If we're on a development branch, we skip the patch version
########################################################################
if(DEFINED UHD_VERSION_PATCH_OVERRIDE)
    set(UHD_VERSION_DEVEL FALSE)
    set(UHD_VERSION_PATCH ${UHD_VERSION_PATCH_OVERRIDE})
endif(DEFINED UHD_VERSION_PATCH_OVERRIDE)
if(NOT DEFINED UHD_VERSION_DEVEL)
    set(UHD_VERSION_DEVEL FALSE)
endif(NOT DEFINED UHD_VERSION_DEVEL)
set(UHD_GIT_BRANCH "")
if(GIT_FOUND)
    execute_process(
        WORKING_DIRECTORY ${UHD_SOURCE_DIR}
        COMMAND ${GIT_EXECUTABLE} rev-parse --abbrev-ref HEAD
        OUTPUT_VARIABLE _git_branch OUTPUT_STRIP_TRAILING_WHITESPACE
        RESULT_VARIABLE _git_branch_result
    )
    if(_git_branch_result EQUAL 0)
        # This is a special case for Azure Pipelines where it runs
        # in detached HEAD mode. The branch name is instead stored
        # in the envionement variable BUILD_SOURCEBRANCH with values
        # like refs/heads/master for the branch master and
        # refs/pull/1/merge for pull request 1
        # The regex removes the first two path segments to match
        # the "git rev-parse --abbrev-ref HEAD" output
        if("${_git_branch}" STREQUAL "HEAD" AND DEFINED ENV{BUILD_SOURCEBRANCH})
            string(REGEX REPLACE "^[^\/]*\/[^\/]*\/" "" _git_branch $ENV{BUILD_SOURCEBRANCH})
        endif()
        set(UHD_GIT_BRANCH ${_git_branch})
        if(UHD_GIT_BRANCH MATCHES "^UHD-")
            message(STATUS "Operating on release branch (${UHD_GIT_BRANCH}).")
            set(UHD_VERSION_DEVEL FALSE)
        elseif(UHD_GIT_BRANCH STREQUAL "master")
            message(STATUS "Operating on master branch.")
            set(UHD_VERSION_DEVEL TRUE)
        else()
            message(STATUS "Working off of feature or development branch. Updating version number.")
            execute_process(
                COMMAND ${PYTHON_EXECUTABLE} -c "print('${_git_branch}'.replace('/', '-'))"
                OUTPUT_VARIABLE _git_safe_branch OUTPUT_STRIP_TRAILING_WHITESPACE
            )
            set(UHD_VERSION_PATCH ${_git_safe_branch})
            set(UHD_VERSION_DEVEL TRUE)
        endif()
    else()
        message(STATUS "Could not determine git branch. Probably building from tarball.")
    endif()
else(GIT_FOUND)
    message(WARNING "Could not detect git executable! Could not determine exact version of UHD!")
endif(GIT_FOUND)
if(DEFINED UHD_GIT_BRANCH_OVERRIDE)
    message(STATUS "Overriding auto-detected git branch and setting to: ${UHD_GIT_BRANCH_OVERRIDE}")
    set(UHD_GIT_BRANCH ${UHD_GIT_BRANCH_OVERRIDE})
endif(DEFINED UHD_GIT_BRANCH_OVERRIDE)

########################################################################
# Version information discovery through git log
########################################################################

#grab the git ref id for the current head
execute_process(
    WORKING_DIRECTORY ${UHD_SOURCE_DIR}
    COMMAND ${GIT_EXECUTABLE} describe --always --abbrev=8 --long
    OUTPUT_VARIABLE _git_describe OUTPUT_STRIP_TRAILING_WHITESPACE
    RESULT_VARIABLE _git_describe_result
)

#only set the build info on success
if(_git_describe_result EQUAL 0)
    if(NOT UHD_GIT_COUNT)
        execute_process(
            WORKING_DIRECTORY ${UHD_SOURCE_DIR}
            COMMAND ${PYTHON_EXECUTABLE} -c "
try:
    print('${_git_describe}'.split('-')[-2])
except IndexError:
    print('0')
"
            OUTPUT_VARIABLE UHD_GIT_COUNT OUTPUT_STRIP_TRAILING_WHITESPACE
        )
    endif()
    if(NOT UHD_GIT_HASH)
        execute_process(
            WORKING_DIRECTORY ${UHD_SOURCE_DIR}
            COMMAND ${PYTHON_EXECUTABLE} -c "
try:
    print('${_git_describe}'.split('-')[-1])
except IndexError:
    print('unknown')
"
             OUTPUT_VARIABLE UHD_GIT_HASH OUTPUT_STRIP_TRAILING_WHITESPACE
        )
    endif()
endif()

## Set default values if all fails. Make sure they're identical to the ones above.
if(NOT UHD_GIT_COUNT)
    set(UHD_GIT_COUNT "0")
endif()

if(NOT UHD_GIT_HASH)
    set(UHD_GIT_HASH "unknown")
endif()

if(UHD_RELEASE_MODE)
    set(UHD_GIT_HASH ${UHD_RELEASE_MODE})

    #Ignore UHD_GIT_COUNT in UHD_VERSION if the string 'release' is in UHD_RELEASE_MODE
    execute_process(
        WORKING_DIRECTORY ${UHD_SOURCE_DIR}
        COMMAND ${PYTHON_EXECUTABLE} -c "print ('release' in '${UHD_RELEASE_MODE}') or ('rc' in '${UHD_RELEASE_MODE}')"
        OUTPUT_VARIABLE TRIM_UHD_VERSION OUTPUT_STRIP_TRAILING_WHITESPACE
    )
endif()


########################################################################
# Define the derived version variables:
if(DEFINED UHD_VERSION)
    set(UHD_VERSION "${UHD_VERSION}" CACHE STRING "Set UHD_VERSION to a custom value")
elseif(TRIM_UHD_VERSION STREQUAL "True")
    set(UHD_VERSION "${UHD_VERSION_MAJOR}.${UHD_VERSION_API}.${UHD_VERSION_ABI}.${UHD_VERSION_PATCH}-${UHD_GIT_HASH}" CACHE STRING "")
else()
    set(UHD_VERSION "${UHD_VERSION_MAJOR}.${UHD_VERSION_API}.${UHD_VERSION_ABI}.${UHD_VERSION_PATCH}-${UHD_GIT_COUNT}-${UHD_GIT_HASH}" CACHE STRING "")
endif()
if(DEFINED UHD_ABI_VERSION)
    set(UHD_ABI_VERSION "${UHD_ABI_VERSION}"
        CACHE STRING "Set UHD_ABI_VERSION to a custom value")
else()
    set(UHD_ABI_VERSION "${UHD_VERSION_MAJOR}.${UHD_VERSION_API}.${UHD_VERSION_ABI}")
endif()

if(UNDERSCORE_UHD_VERSION)
    string(REPLACE "-" "_" _uhd_version $CACHE{UHD_VERSION})
    set(UHD_VERSION "${_uhd_version}" CACHE STRING "" FORCE)
endif()

set(UHD_COMPONENT "UHD")

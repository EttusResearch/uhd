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
INCLUDE(UHDPython) #requires python for parsing
FIND_PACKAGE(Git QUIET)

########################################################################
# Setup Version Numbers
#  - increment major on api compatibility changes
#  - increment minor on feature-level changes
#  - increment patch on for bug fixes and docs
########################################################################
SET(UHD_VERSION_MAJOR 003)
SET(UHD_VERSION_MINOR 002)
SET(UHD_VERSION_PATCH 003)

########################################################################
# Version information discovery through git log
########################################################################
IF(UHD_RELEASE_MODE)
    SET(UHD_BUILD_INFO_DISCOVERY FALSE)
    SET(UHD_BUILD_INFO "release")
ELSE()
    SET(UHD_BUILD_INFO_DISCOVERY GIT_FOUND)
    SET(UHD_BUILD_INFO "unknown")
ENDIF()

IF(UHD_BUILD_INFO_DISCOVERY)

    #grab the git ref id for the current head
    EXECUTE_PROCESS(
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
        OUTPUT_VARIABLE _git_rev OUTPUT_STRIP_TRAILING_WHITESPACE
        RESULT_VARIABLE _git_rev_result
    )

    #only set the build info on success
    IF(_git_rev_result EQUAL 0)
        SET(UHD_BUILD_INFO ${_git_rev})
    ENDIF()
ENDIF(UHD_BUILD_INFO_DISCOVERY)

########################################################################
SET(UHD_VERSION "${UHD_VERSION_MAJOR}.${UHD_VERSION_MINOR}.${UHD_VERSION_PATCH}")

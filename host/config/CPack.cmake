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

########################################################################
# Setup Version Numbers
########################################################################
SET(UHD_VERSION_MAJOR 0)
SET(UHD_VERSION_MINOR 0)
SET(UHD_VERSION_PATCH 0)

########################################################################
# Get the current YYYYMMDD timestamp
########################################################################
EXECUTE_PROCESS(
    COMMAND ${PYTHON_EXECUTABLE} -c "import time; print time.strftime('%Y%m%d', time.gmtime())"
    OUTPUT_VARIABLE UHD_DATE OUTPUT_STRIP_TRAILING_WHITESPACE
)
SET(UHD_VERSION_MAJOR ${UHD_DATE})

########################################################################
# Find GIT to get repo information
########################################################################
MESSAGE(STATUS "Checking for git")
FIND_PROGRAM(GIT git)
IF(${GIT} STREQUAL "GIT-NOTFOUND")
    MESSAGE(STATUS "Checking for git - not found")
    SET(UHD_REV "unknown")
ELSE(${GIT} STREQUAL "GIT-NOTFOUND")
    MESSAGE(STATUS "Checking for git - found")
    EXECUTE_PROCESS(
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMAND ${GIT} rev-parse --short HEAD
        OUTPUT_VARIABLE UHD_REV OUTPUT_STRIP_TRAILING_WHITESPACE
    )
ENDIF(${GIT} STREQUAL "GIT-NOTFOUND")
SET(UHD_VERSION_MINOR ${UHD_REV})

########################################################################
# Setup CPack
########################################################################
SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Ettus Research - Universal Hardware Driver")
SET(CPACK_PACKAGE_VENDOR              "Ettus Research LLC")
SET(CPACK_PACKAGE_CONTACT             "support@ettus.com")
SET(CPACK_PACKAGE_VERSION_MAJOR ${UHD_VERSION_MAJOR})
SET(CPACK_PACKAGE_VERSION_MINOR ${UHD_VERSION_MINOR})
SET(CPACK_PACKAGE_VERSION_PATCH ${UHD_VERSION_PATCH})
SET(CPACK_RESOURCE_FILE_README ${CMAKE_CURRENT_SOURCE_DIR}/README)
SET(CPACK_RESOURCE_FILE_LICENSE ${CMAKE_CURRENT_SOURCE_DIR}/LICENSE)
SET(BOOST_MIN_VERSION 1.36) #used in setup for boost
STRING(REPLACE "," ", " CPACK_DEBIAN_PACKAGE_DEPENDS
    "libboost-date-time-dev          (>= ${BOOST_MIN_VERSION}),"
    "libboost-filesystem-dev         (>= ${BOOST_MIN_VERSION}),"
    "libboost-program-options-dev    (>= ${BOOST_MIN_VERSION}),"
    "libboost-regex-dev              (>= ${BOOST_MIN_VERSION}),"
    "libboost-system-dev             (>= ${BOOST_MIN_VERSION}),"
    "libboost-test-dev               (>= ${BOOST_MIN_VERSION}),"
    "libboost-thread-dev             (>= ${BOOST_MIN_VERSION})"
)
SET(CPACK_DEBIAN_PACKAGE_RECOMMENDS "python, python-tk")
SET(CPACK_RPM_PACKAGE_REQUIRES "boost-devel >= ${BOOST_MIN_VERSION}")
INCLUDE(CPack) #include after setting vars

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
INCLUDE(UHDVersion) #sets version information

########################################################################
# Setup package file name
########################################################################
SET(CPACK_PACKAGE_FILE_NAME "UHD-${UHD_VERSION}")
IF(DEFINED UHD_PACKAGE_SUFFIX) #append optional suffix (usually system type)
    SET(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_FILE_NAME}-${UHD_PACKAGE_SUFFIX})
ENDIF(DEFINED UHD_PACKAGE_SUFFIX)

########################################################################
# Setup CPack General
########################################################################
SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Ettus Research - Universal Hardware Driver")
SET(CPACK_PACKAGE_VENDOR              "Ettus Research LLC")
SET(CPACK_PACKAGE_CONTACT             "support@ettus.com")
SET(CPACK_PACKAGE_VERSION_MAJOR ${UHD_VERSION_MAJOR})
SET(CPACK_PACKAGE_VERSION_MINOR ${UHD_VERSION_MINOR})
SET(CPACK_PACKAGE_VERSION_PATCH ${UHD_VERSION_PATCH})
SET(CPACK_RESOURCE_FILE_README ${CMAKE_SOURCE_DIR}/README)
SET(CPACK_RESOURCE_FILE_LICENSE ${CMAKE_SOURCE_DIR}/LICENSE)

########################################################################
# Setup CPack Components
########################################################################
SET(CPACK_COMPONENT_LIBRARIES_GROUP      "Development")
SET(CPACK_COMPONENT_HEADERS_GROUP        "Development")
SET(CPACK_COMPONENT_UTILITIES_GROUP      "Runtime")
SET(CPACK_COMPONENT_EXAMPLES_GROUP       "Runtime")
SET(CPACK_COMPONENT_TESTS_GROUP          "Runtime")
SET(CPACK_COMPONENT_MANUAL_GROUP         "Documentation")
SET(CPACK_COMPONENT_DOXYGEN_GROUP        "Documentation")

SET(CPACK_COMPONENT_LIBRARIES_DISPLAY_NAME      "Libraries")
SET(CPACK_COMPONENT_HEADERS_DISPLAY_NAME        "C++ Headers")
SET(CPACK_COMPONENT_UTILITIES_DISPLAY_NAME      "Utilities")
SET(CPACK_COMPONENT_EXAMPLES_DISPLAY_NAME       "Examples")
SET(CPACK_COMPONENT_TESTS_DISPLAY_NAME          "Unit Tests")
SET(CPACK_COMPONENT_MANUAL_DISPLAY_NAME         "Manual")
SET(CPACK_COMPONENT_DOXYGEN_DISPLAY_NAME        "Doxygen")

SET(CPACK_COMPONENT_UTILITIES_DEPENDS libraries)
SET(CPACK_COMPONENT_EXAMPLES_DEPENDS libraries)
SET(CPACK_COMPONENT_TESTS_DEPENDS libraries)

SET(CPACK_COMPONENTS_ALL libraries headers utilities examples tests manual doxygen)

########################################################################
# Setup CPack Debian
########################################################################
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

########################################################################
# Setup CPack RPM
########################################################################
SET(CPACK_RPM_PACKAGE_REQUIRES "boost-devel >= ${BOOST_MIN_VERSION}")

########################################################################
INCLUDE(CPack) #include after setting vars

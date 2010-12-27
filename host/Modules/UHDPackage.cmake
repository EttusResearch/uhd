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
# Setup CPack
########################################################################
SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Ettus Research - Universal Hardware Driver")
SET(CPACK_PACKAGE_VENDOR              "Ettus Research LLC")
SET(CPACK_PACKAGE_CONTACT             "support@ettus.com")
SET(CPACK_PACKAGE_VERSION_MAJOR ${UHD_VERSION_MAJOR})
SET(CPACK_PACKAGE_VERSION_MINOR ${UHD_VERSION_MINOR})
SET(CPACK_PACKAGE_VERSION_PATCH ${UHD_VERSION_PATCH})
SET(CPACK_RESOURCE_FILE_README ${CMAKE_SOURCE_DIR}/README)
SET(CPACK_RESOURCE_FILE_LICENSE ${CMAKE_SOURCE_DIR}/LICENSE)
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

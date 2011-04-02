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
# Setup Project
########################################################################
CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
PROJECT(UHD-images NONE)
LIST(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/../host/Modules)
INCLUDE(UHDVersion) #sets version variables (used below)

########################################################################
# Setup CPack
########################################################################
SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Ettus Research - Universal Hardware Driver Images")
SET(CPACK_PACKAGE_VENDOR              "Ettus Research LLC")
SET(CPACK_PACKAGE_CONTACT             "support@ettus.com")
SET(CPACK_PACKAGE_VERSION_MAJOR ${UHD_VERSION_MAJOR})
SET(CPACK_PACKAGE_VERSION_MINOR ${UHD_VERSION_MINOR})
SET(CPACK_PACKAGE_VERSION_PATCH ${UHD_VERSION_PATCH})
SET(CPACK_PACKAGE_FILE_NAME "UHD-images-${UHD_VERSION}")
SET(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "all")
INCLUDE(CPack) #include after setting vars
MESSAGE(STATUS "Version: ${CPACK_PACKAGE_VERSION}")

########################################################################
# Install Images
########################################################################
#tag the images with a version number (something identifiable)
FILE(WRITE ${CMAKE_SOURCE_DIR}/images/${CPACK_PACKAGE_VERSION}.tag ${UHD_BUILD_INFO})
INSTALL(DIRECTORY ${CMAKE_SOURCE_DIR}/images DESTINATION share/uhd)

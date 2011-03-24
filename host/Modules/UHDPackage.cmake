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
INCLUDE(UHDVersion) #sets version information

########################################################################
# Setup package file name
########################################################################
IF(UHD_PACKAGE_MODE STREQUAL AUTO)
    FIND_PROGRAM(LSB_RELEASE_EXECUTABLE lsb_release)
    FIND_PROGRAM(UNAME_EXECUTABLE uname)
    IF(LSB_RELEASE_EXECUTABLE AND UNAME_EXECUTABLE)

        #extract system information by executing the commands
        EXECUTE_PROCESS(
            COMMAND ${LSB_RELEASE_EXECUTABLE} --short --id
            OUTPUT_VARIABLE _os_name OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        EXECUTE_PROCESS(
            COMMAND ${LSB_RELEASE_EXECUTABLE} --short --release
            OUTPUT_VARIABLE _os_version OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        EXECUTE_PROCESS(
            COMMAND ${UNAME_EXECUTABLE} --machine
            OUTPUT_VARIABLE _machine OUTPUT_STRIP_TRAILING_WHITESPACE
        )

        #set generator type for recognized systems
        IF(${_os_name} STREQUAL Ubuntu)
            SET(CPACK_GENERATOR DEB)
        ENDIF()
        IF(${_os_name} STREQUAL Fedora)
            SET(CPACK_GENERATOR RPM)
        ENDIF()

        #set a more sensible package name for this system
        SET(CPACK_PACKAGE_FILE_NAME "UHD-${UHD_VERSION}-${_os_name}-${_os_version}-${_machine}")

    ENDIF(LSB_RELEASE_EXECUTABLE AND UNAME_EXECUTABLE)
ENDIF(UHD_PACKAGE_MODE STREQUAL AUTO)

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
SET(CPACK_DEBIAN_PACKAGE_DEPENDS "libusb-1.0-0, libboost-dev")
SET(CPACK_DEBIAN_PACKAGE_RECOMMENDS "python, python-tk")

########################################################################
# Setup CPack RPM
########################################################################
SET(CPACK_RPM_PACKAGE_REQUIRES "boost-devel, libusb1")

########################################################################
INCLUDE(CPack) #include after setting vars

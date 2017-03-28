#
# Copyright 2017 Ettus Research (National Instruments)
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
# This file included, use CMake directory variables
########################################################################

########################################################################
# Setup Boost
########################################################################
MESSAGE(STATUS "")
MESSAGE(STATUS "Configuring Boost C++ Libraries...")
SET(BOOST_REQUIRED_COMPONENTS
  python
  system
)
IF(MINGW)
    LIST(APPEND BOOST_REQUIRED_COMPONENTS thread_win32)
ELSE()
    LIST(APPEND BOOST_REQUIRED_COMPONENTS thread)
ENDIF()

IF(UNIX AND NOT BOOST_ROOT AND EXISTS "/usr/lib64")
    LIST(APPEND BOOST_LIBRARYDIR "/usr/lib64") #fedora 64-bit fix
ENDIF(UNIX AND NOT BOOST_ROOT AND EXISTS "/usr/lib64")

IF(MSVC)
    SET(BOOST_ALL_DYN_LINK "${BOOST_ALL_DYN_LINK}" CACHE BOOL "boost enable dynamic linking")
    IF(BOOST_ALL_DYN_LINK)
        ADD_DEFINITIONS(-DBOOST_ALL_DYN_LINK) #setup boost auto-linking in msvc
    ELSE(BOOST_ALL_DYN_LINK)
        SET(BOOST_REQUIRED_COMPONENTS) #empty components list for static link
    ENDIF(BOOST_ALL_DYN_LINK)
ENDIF(MSVC)

SET(Boost_ADDITIONAL_VERSIONS
    "1.46.0" "1.46" "1.47.0" "1.47" "1.48.0" "1.48" "1.48.0" "1.49" "1.50.0" "1.50"
    "1.51.0" "1.51" "1.52.0" "1.52" "1.53.0" "1.53" "1.54.0" "1.54" "1.55.0" "1.55"
    "1.56.0" "1.56" "1.57" "1.57" "1.58" "1.59" "1.60" "1.61"
)
FIND_PACKAGE(Boost 1.53 COMPONENTS ${BOOST_REQUIRED_COMPONENTS})

MESSAGE(STATUS "Boost include directories: ${Boost_INCLUDE_DIRS}")
MESSAGE(STATUS "Boost library directories: ${Boost_LIBRARY_DIRS}")
MESSAGE(STATUS "Boost libraries: ${Boost_LIBRARIES}")

SET(UHD_HOST_ROOT ${CMAKE_SOURCE_DIR}/../host)

ADD_LIBRARY(pyusrp_periphs SHARED pyusrp_periphs.cpp)
TARGET_INCLUDE_DIRECTORIES(pyusrp_periphs PUBLIC
    ${PYTHON_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/lib/
    ${UHD_HOST_ROOT}/lib/usrp/common
)
TARGET_LINK_LIBRARIES(pyusrp_periphs ${Boost_PYTHON_LIBRARY} ${Boost_LIBRARIES} usrp-periphs)
ADD_CUSTOM_COMMAND(TARGET pyusrp_periphs POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/libpyusrp_periphs.so ${CMAKE_CURRENT_BINARY_DIR}/usrp_mpm/libpyusrp_periphs.so)

SET(USRP_MPM_FILES "")
ADD_SUBDIRECTORY(usrp_mpm)

SET(OUTPUT      "${CMAKE_CURRENT_BINARY_DIR}/build/timestamp")
SET(SETUP_PY_IN "${CMAKE_CURRENT_SOURCE_DIR}/setup.py.in")
SET(SETUP_PY    "${CMAKE_CURRENT_BINARY_DIR}/setup.py")
SET(PERIPH_MGR_INIT_IN "${CMAKE_CURRENT_SOURCE_DIR}/usrp_mpm/periph_manager/__init__.py.in")
SET(PERIPH_MGR_INIT "${CMAKE_CURRENT_BINARY_DIR}/usrp_mpm/periph_manager/__init__.py")

CONFIGURE_FILE(${SETUP_PY_IN} ${SETUP_PY})
CONFIGURE_FILE(${PERIPH_MGR_INIT_IN} ${PERIPH_MGR_INIT})

ADD_CUSTOM_COMMAND(OUTPUT ${OUTPUT}
     COMMAND ${CMAKE_COMMAND}  -DSOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}" -DBINARY_DIR="${CMAKE_CURRENT_BINARY_DIR}"  -P ${CMAKE_CURRENT_SOURCE_DIR}/copy_python_module.cmake
     COMMAND ${PYTHON_EXECUTABLE} ${SETUP_PY} -q build
     COMMAND ${CMAKE_COMMAND} -E touch ${OUTPUT}
     DEPENDS ${USRP_MPM_FILES})
ADD_CUSTOM_TARGET(usrp_mpm ALL DEPENDS ${OUTPUT} pyusrp_periphs)

EXECUTE_PROCESS(COMMAND ${PYTHON_EXECUTABLE} -c
    "from distutils import sysconfig; print sysconfig.get_python_lib(plat_specific=True, prefix='')"
    OUTPUT_VARIABLE USRP_MPM_PYTHON_DIR OUTPUT_STRIP_TRAILING_WHITESPACE
)
INSTALL(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/build/lib/usrp_mpm DESTINATION ${CMAKE_INSTALL_PREFIX}/${USRP_MPM_PYTHON_DIR})
INSTALL(PROGRAMS
  usrp_hwd.py
  DESTINATION ${RUNTIME_DIR})

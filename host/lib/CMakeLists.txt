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
# Helpful Macros
########################################################################
MACRO(LIBUHD_APPEND_SOURCES)
    LIST(APPEND libuhd_sources ${ARGV})
ENDMACRO(LIBUHD_APPEND_SOURCES)

MACRO(LIBUHD_APPEND_LIBS)
    LIST(APPEND libuhd_libs ${ARGV})
ENDMACRO(LIBUHD_APPEND_LIBS)

MACRO(LIBUHD_PYTHON_GEN_SOURCE pyfile outfile)
    #ensure that the directory exists for outfile
    GET_FILENAME_COMPONENT(outfile_dir ${outfile} PATH)
    FILE(MAKE_DIRECTORY ${outfile_dir})

    #make the outfile depend on the python script
    ADD_CUSTOM_COMMAND(
        OUTPUT ${outfile} DEPENDS ${pyfile} ${LIBUHD_PYTHON_GEN_SOURCE_DEPS}
        COMMAND ${PYTHON_EXECUTABLE} -B ${pyfile} ${outfile}
        COMMENT "Generating ${outfile}"
    )

    #make libuhd depend on the outfile
    LIBUHD_APPEND_SOURCES(${outfile})
ENDMACRO(LIBUHD_PYTHON_GEN_SOURCE)

MACRO(INCLUDE_SUBDIRECTORY subdir)
    #insert the current directories on the front of the list
    LIST(INSERT _cmake_source_dirs 0 ${CMAKE_CURRENT_SOURCE_DIR})
    LIST(INSERT _cmake_binary_dirs 0 ${CMAKE_CURRENT_BINARY_DIR})

    #set the current directories to the names of the subdirs
    SET(CMAKE_CURRENT_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/${subdir})
    SET(CMAKE_CURRENT_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/${subdir})

    #include the subdirectory CMakeLists to run it
    FILE(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
    INCLUDE(${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt)

    #reset the value of the current directories
    LIST(GET _cmake_source_dirs 0 CMAKE_CURRENT_SOURCE_DIR)
    LIST(GET _cmake_binary_dirs 0 CMAKE_CURRENT_BINARY_DIR)

    #pop the subdir names of the front of the list
    LIST(REMOVE_AT _cmake_source_dirs 0)
    LIST(REMOVE_AT _cmake_binary_dirs 0)
ENDMACRO(INCLUDE_SUBDIRECTORY)

########################################################################
# Include subdirectories (different than add)
########################################################################
INCLUDE_SUBDIRECTORY(ic_reg_maps)
INCLUDE_SUBDIRECTORY(types)
INCLUDE_SUBDIRECTORY(convert)
INCLUDE_SUBDIRECTORY(transport)
INCLUDE_SUBDIRECTORY(usrp)
INCLUDE_SUBDIRECTORY(utils)

########################################################################
# Setup UHD_VERSION_STRING for version.cpp
########################################################################
CONFIGURE_FILE(
    ${CMAKE_CURRENT_SOURCE_DIR}/version.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/version.cpp
@ONLY)

########################################################################
# Append to the list of sources for lib uhd
########################################################################
LIBUHD_APPEND_SOURCES(
    ${CMAKE_CURRENT_SOURCE_DIR}/deprecated.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/device.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/exception.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/property_tree.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/version.cpp
)

########################################################################
# Add DLL resource file to Windows build
########################################################################
IF(MSVC)
    CONFIGURE_FILE(
        ${CMAKE_CURRENT_SOURCE_DIR}/uhd.rc.in
        ${CMAKE_CURRENT_BINARY_DIR}/uhd.rc
    @ONLY)

    LIST(APPEND libuhd_sources ${CMAKE_CURRENT_BINARY_DIR}/uhd.rc)
ENDIF(MSVC)

########################################################################
# Setup libuhd library
########################################################################
ADD_LIBRARY(uhd SHARED ${libuhd_sources})
TARGET_LINK_LIBRARIES(uhd ${Boost_LIBRARIES} ${libuhd_libs})
SET_TARGET_PROPERTIES(uhd PROPERTIES DEFINE_SYMBOL "UHD_DLL_EXPORTS")
SET_TARGET_PROPERTIES(uhd PROPERTIES SOVERSION "${UHD_VERSION_MAJOR}")
SET_TARGET_PROPERTIES(uhd PROPERTIES VERSION "${UHD_VERSION_MAJOR}.${UHD_VERSION_MINOR}")
IF(DEFINED LIBUHD_OUTPUT_NAME)
    SET_TARGET_PROPERTIES(uhd PROPERTIES OUTPUT_NAME ${LIBUHD_OUTPUT_NAME})
ENDIF(DEFINED LIBUHD_OUTPUT_NAME)

INSTALL(TARGETS uhd
    LIBRARY DESTINATION ${LIBRARY_DIR} COMPONENT libraries # .so file
    ARCHIVE DESTINATION ${LIBRARY_DIR} COMPONENT libraries # .lib file
    RUNTIME DESTINATION ${RUNTIME_DIR} COMPONENT libraries # .dll file
)

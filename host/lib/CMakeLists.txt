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
# Setup Python
########################################################################
INCLUDE(FindPythonInterp)

MACRO(PYTHON_CHECK_MODULE module have)
    MESSAGE(STATUS "Checking for python module ${module}")
    EXECUTE_PROCESS(
        COMMAND ${PYTHON_EXECUTABLE} -c "import ${module}"
        RESULT_VARIABLE ${have}
    )
    IF(${have} EQUAL 0)
        MESSAGE(STATUS "Checking for python module ${module} - found")
        SET(${have} TRUE)
    ELSE(${have} EQUAL 0)
        MESSAGE(STATUS "Checking for python module ${module} - not found")
        SET(${have} FALSE)
    ENDIF(${have} EQUAL 0)
ENDMACRO(PYTHON_CHECK_MODULE)

PYTHON_CHECK_MODULE("Cheetah" HAVE_PYTHON_MODULE_CHEETAH)

IF(NOT HAVE_PYTHON_MODULE_CHEETAH)
    MESSAGE(FATAL_ERROR "Error: Cheetah Templates needed for pre-build generation.")
ENDIF(NOT HAVE_PYTHON_MODULE_CHEETAH)

########################################################################
# Helpful Macros
########################################################################
MACRO(LIBUHD_APPEND_SOURCES)
    LIST(APPEND libuhd_sources ${ARGV})
ENDMACRO(LIBUHD_APPEND_SOURCES)

MACRO(LIBUHD_PYTHON_GEN_SOURCE pyfile outfile)
    #ensure that the directory exists for outfile
    GET_FILENAME_COMPONENT(outfile_dir ${outfile} PATH)
    FILE(MAKE_DIRECTORY ${outfile_dir})

    #make the outfile depend on the python script
    ADD_CUSTOM_COMMAND(
        OUTPUT ${outfile} DEPENDS ${pyfile}
        COMMAND ${PYTHON_EXECUTABLE} ${pyfile} ${outfile}
        COMMENT "Generating ${outfile}"
    )

    #make libuhd depend on the outfile
    LIBUHD_APPEND_SOURCES(${outfile})
ENDMACRO(LIBUHD_PYTHON_GEN_SOURCE)

########################################################################
# Include CMakeLists.txt from subdirectories
########################################################################
INCLUDE(${CMAKE_CURRENT_SOURCE_DIR}/ic_reg_maps/CMakeLists.txt)
INCLUDE(${CMAKE_CURRENT_SOURCE_DIR}/transport/CMakeLists.txt)
INCLUDE(${CMAKE_CURRENT_SOURCE_DIR}/usrp/CMakeLists.txt)
INCLUDE(${CMAKE_CURRENT_SOURCE_DIR}/usrp/dboard/CMakeLists.txt)
INCLUDE(${CMAKE_CURRENT_SOURCE_DIR}/usrp/usrp2/CMakeLists.txt)

########################################################################
# Conditionally add the usrp1e sources
########################################################################
MESSAGE(STATUS "Configuring usrp-e support...")

INCLUDE(CheckIncludeFiles)
SET(usrp_e_required_headers
    linux/ioctl.h
    linux/spi/spidev.h
    linux/usrp_e.h
)
CHECK_INCLUDE_FILES(
    "${usrp_e_required_headers}"
    HAVE_USRP_E_REQUIRED_HEADERS
)

IF(HAVE_USRP_E_REQUIRED_HEADERS)
    MESSAGE(STATUS "  Building usrp-e support.")
    LIST(APPEND libuhd_sources
        usrp/usrp_e/clock_ctrl.cpp
        usrp/usrp_e/codec_ctrl.cpp
        usrp/usrp_e/dboard_impl.cpp
        usrp/usrp_e/dboard_iface.cpp
        usrp/usrp_e/dsp_impl.cpp
        usrp/usrp_e/fpga-downloader.cc
        usrp/usrp_e/mboard_impl.cpp
        usrp/usrp_e/usrp_e_impl.cpp
        usrp/usrp_e/usrp_e_iface.cpp
    )
ELSE(HAVE_USRP_E_REQUIRED_HEADERS)
    MESSAGE(STATUS "  Skipping usrp-e support.")
    LIST(APPEND libuhd_sources
        usrp/usrp_e/usrp_e_none.cpp
    )
ENDIF(HAVE_USRP_E_REQUIRED_HEADERS)

########################################################################
# Setup defines for module loading
########################################################################
MESSAGE(STATUS "Configuring module loading...")

INCLUDE(CheckIncludeFileCXX)
CHECK_INCLUDE_FILE_CXX(dlfcn.h HAVE_DLFCN_H)
CHECK_INCLUDE_FILE_CXX(windows.h HAVE_WINDOWS_H)

IF(HAVE_DLFCN_H)
    MESSAGE(STATUS "  Module loading supported through dlopen.")
    ADD_DEFINITIONS(-DHAVE_DLFCN_H)
ELSEIF(HAVE_WINDOWS_H)
    MESSAGE(STATUS "  Module loading supported through LoadLibrary.")
    ADD_DEFINITIONS(-DHAVE_WINDOWS_H)
ELSE(HAVE_DLFCN_H)
    MESSAGE(STATUS "  Module loading not supported.")
ENDIF(HAVE_DLFCN_H)

########################################################################
# Append to the list of sources for lib uhd
########################################################################
LIBUHD_APPEND_SOURCES(
    ${CMAKE_CURRENT_SOURCE_DIR}/device.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/gain_handler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/load_modules.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/types.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/utils.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/wax.cpp
)

########################################################################
# Setup libuhd library
########################################################################
ADD_LIBRARY(uhd SHARED ${libuhd_sources})

TARGET_LINK_LIBRARIES(uhd ${Boost_LIBRARIES} ${CMAKE_DL_LIBS})

SET_TARGET_PROPERTIES(uhd PROPERTIES DEFINE_SYMBOL "UHD_DLL_EXPORTS")

INSTALL(TARGETS uhd
    LIBRARY DESTINATION ${LIBRARY_DIR} # .so file
    ARCHIVE DESTINATION ${LIBRARY_DIR} # .lib file
    RUNTIME DESTINATION ${LIBRARY_DIR} # .dll file
)

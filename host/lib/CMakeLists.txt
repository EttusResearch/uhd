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
# Create a list of libuhd sources
########################################################################
SET(libuhd_sources
    device.cpp
    gain_handler.cpp
    load_modules.cpp
    types.cpp
    utils.cpp
    wax.cpp
    transport/convert_types.cpp
    transport/if_addrs.cpp
    transport/udp_simple.cpp
    usrp/dboard_base.cpp
    usrp/dboard_eeprom.cpp
    usrp/dboard_id.cpp
    usrp/dboard_manager.cpp
    usrp/simple_usrp.cpp
    usrp/tune_helper.cpp
)

########################################################################
# Generate Files
########################################################################
MACRO(UHD_PYTHON_GEN_SOURCE_FILE pyfile outfile)
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
    LIST(APPEND libuhd_sources ${outfile})
ENDMACRO(UHD_PYTHON_GEN_SOURCE_FILE)

UHD_PYTHON_GEN_SOURCE_FILE(
    ${CMAKE_CURRENT_SOURCE_DIR}/transport/gen_vrt.py
    ${CMAKE_CURRENT_BINARY_DIR}/transport/vrt.cpp
)

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR}/ic_reg_maps)

UHD_PYTHON_GEN_SOURCE_FILE(
    ${CMAKE_CURRENT_SOURCE_DIR}/ic_reg_maps/gen_adf4360_regs.py
    ${CMAKE_CURRENT_BINARY_DIR}/ic_reg_maps/adf4360_regs.hpp
)

UHD_PYTHON_GEN_SOURCE_FILE(
    ${CMAKE_CURRENT_SOURCE_DIR}/ic_reg_maps/gen_ad9510_regs.py
    ${CMAKE_CURRENT_BINARY_DIR}/ic_reg_maps/ad9510_regs.hpp
)

UHD_PYTHON_GEN_SOURCE_FILE(
    ${CMAKE_CURRENT_SOURCE_DIR}/ic_reg_maps/gen_ad9777_regs.py
    ${CMAKE_CURRENT_BINARY_DIR}/ic_reg_maps/ad9777_regs.hpp
)

UHD_PYTHON_GEN_SOURCE_FILE(
    ${CMAKE_CURRENT_SOURCE_DIR}/ic_reg_maps/gen_ad5624_regs.py
    ${CMAKE_CURRENT_BINARY_DIR}/ic_reg_maps/ad5624_regs.hpp
)

UHD_PYTHON_GEN_SOURCE_FILE(
    ${CMAKE_CURRENT_SOURCE_DIR}/ic_reg_maps/gen_ad7922_regs.py
    ${CMAKE_CURRENT_BINARY_DIR}/ic_reg_maps/ad7922_regs.hpp
)

UHD_PYTHON_GEN_SOURCE_FILE(
    ${CMAKE_CURRENT_SOURCE_DIR}/ic_reg_maps/gen_max2829_regs.py
    ${CMAKE_CURRENT_BINARY_DIR}/ic_reg_maps/max2829_regs.hpp
)

########################################################################
# Add dboard sources
########################################################################
LIST(APPEND libuhd_sources
    usrp/dboard/db_basic_and_lf.cpp
    usrp/dboard/db_rfx.cpp
    usrp/dboard/db_xcvr2450.cpp
)

########################################################################
# Add usrp2 sources
########################################################################
LIST(APPEND libuhd_sources
    usrp/usrp2/clock_control.cpp
    usrp/usrp2/dboard_impl.cpp
    usrp/usrp2/dboard_iface.cpp
    usrp/usrp2/dsp_impl.cpp
    usrp/usrp2/io_impl.cpp
    usrp/usrp2/mboard_impl.cpp
    usrp/usrp2/usrp2_iface.cpp
    usrp/usrp2/usrp2_impl.cpp
)

########################################################################
# Conditionally add the udp sources
########################################################################
LIST(APPEND libuhd_sources
    transport/udp_zero_copy_asio.cpp
)

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
# Setup defines for interface address discovery
########################################################################
MESSAGE(STATUS "Configuring interface address discovery...")

INCLUDE(CheckIncludeFileCXX)
CHECK_INCLUDE_FILE_CXX(ifaddrs.h HAVE_IFADDRS_H)
CHECK_INCLUDE_FILE_CXX(winsock2.h HAVE_WINSOCK2_H)

IF(HAVE_IFADDRS_H)
    MESSAGE(STATUS "  Interface address discovery supported through getifaddrs.")
    ADD_DEFINITIONS(-DHAVE_IFADDRS_H)
ELSEIF(HAVE_WINSOCK2_H)
    MESSAGE(STATUS "  Interface address discovery supported through SIO_GET_INTERFACE_LIST.")
    ADD_DEFINITIONS(-DHAVE_WINSOCK2_H)
ELSE(HAVE_IFADDRS_H)
    MESSAGE(STATUS "  Interface address discovery not supported.")
ENDIF(HAVE_IFADDRS_H)

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

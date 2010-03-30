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
# Create a list of libuhd sources
########################################################################
SET(libuhd_sources
    device.cpp
    gain_handler.cpp
    load_modules.cpp
    simple_device.cpp
    tune_helper.cpp
    types.cpp
    wax.cpp
    transport/if_addrs.cpp
    transport/udp_simple.cpp
    transport/vrt.cpp
    usrp/dboard/basic.cpp
    usrp/dboard_base.cpp
    usrp/dboard_interface.cpp
    usrp/dboard_manager.cpp
    usrp/usrp2/dboard_impl.cpp
    usrp/usrp2/dboard_interface.cpp
    usrp/usrp2/dsp_impl.cpp
    usrp/usrp2/io_impl.cpp
    usrp/usrp2/mboard_impl.cpp
    usrp/usrp2/usrp2_impl.cpp
)

########################################################################
# Conditionally add the udp sources
########################################################################
LIST(APPEND libuhd_sources
    transport/udp_zero_copy_asio.cpp
)

########################################################################
# Conditionally add the usrp1e sources
########################################################################
MESSAGE(STATUS "Configuring usrp-e support...")

INCLUDE(CheckIncludeFiles)
SET(usrp1e_required_headers
    linux/ioctl.h
    linux/spi/spidev.h
    linux/usrp1_e.h
)
CHECK_INCLUDE_FILES(
    "${usrp1e_required_headers}"
    HAS_USRP1E_REQUIRED_HEADERS
)

IF(HAS_USRP1E_REQUIRED_HEADERS)
    MESSAGE(STATUS "  Building usrp1e support.")
    LIST(APPEND libuhd_sources
        usrp/usrp_e/dboard_impl.cpp
        usrp/usrp_e/dboard_interface.cpp
        usrp/usrp_e/dsp_impl.cpp
        usrp/usrp_e/fpga-downloader.cc
        usrp/usrp_e/mboard_impl.cpp
        usrp/usrp_e/usrp_e_impl.cpp
    )
ELSE(HAS_USRP1E_REQUIRED_HEADERS)
    MESSAGE(STATUS "  Skipping usrp1e support.")
    LIST(APPEND libuhd_sources
        usrp/usrp_e/usrp_e_none.cpp
    )
ENDIF(HAS_USRP1E_REQUIRED_HEADERS)

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

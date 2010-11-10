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

#This file will be included by cmake, use absolute paths!

########################################################################
# Conditionally configure the USRP-E100 support
########################################################################
MESSAGE(STATUS "Configuring USRP-E100 support...")

INCLUDE(CheckIncludeFileCXX)
CHECK_INCLUDE_FILE_CXX(linux/usrp_e.h HAVE_LINUX_USRP_E_H)

SET(ENABLE_USRP_E TRUE)

IF(DEFINED ENABLE_USRP_E100)
    IF(ENABLE_USRP_E100)
        MESSAGE(STATUS "USRP-E100 support enabled by configure flag")
    ELSE(ENABLE_USRP_E100)
        MESSAGE(STATUS "USRP-E100 support disabled by configure flag")
    ENDIF(ENABLE_USRP_E100)
ELSE(DEFINED ENABLE_USRP_E100) #not defined: automatic enabling of component
    SET(ENABLE_USRP_E100 ${HAVE_LINUX_USRP_E_H})
ENDIF(DEFINED ENABLE_USRP_E100)
SET(ENABLE_USRP_E100 ${ENABLE_USRP_E100} CACHE BOOL "enable USRP-E100 support")

IF(ENABLE_USRP_E100)
    MESSAGE(STATUS "  Building USRP-E100 support.")
    LIBUHD_APPEND_SOURCES(
        ${CMAKE_SOURCE_DIR}/lib/usrp/usrp_e100/clock_ctrl.cpp
        ${CMAKE_SOURCE_DIR}/lib/usrp/usrp_e100/clock_ctrl.hpp
        ${CMAKE_SOURCE_DIR}/lib/usrp/usrp_e100/codec_ctrl.cpp
        ${CMAKE_SOURCE_DIR}/lib/usrp/usrp_e100/codec_ctrl.hpp
        ${CMAKE_SOURCE_DIR}/lib/usrp/usrp_e100/codec_impl.cpp
        ${CMAKE_SOURCE_DIR}/lib/usrp/usrp_e100/dboard_impl.cpp
        ${CMAKE_SOURCE_DIR}/lib/usrp/usrp_e100/dboard_iface.cpp
        ${CMAKE_SOURCE_DIR}/lib/usrp/usrp_e100/dsp_impl.cpp
        ${CMAKE_SOURCE_DIR}/lib/usrp/usrp_e100/fpga-downloader.cc
        ${CMAKE_SOURCE_DIR}/lib/usrp/usrp_e100/io_impl.cpp
        ${CMAKE_SOURCE_DIR}/lib/usrp/usrp_e100/mboard_impl.cpp
        ${CMAKE_SOURCE_DIR}/lib/usrp/usrp_e100/usrp_e_impl.cpp
        ${CMAKE_SOURCE_DIR}/lib/usrp/usrp_e100/usrp_e_impl.hpp
        ${CMAKE_SOURCE_DIR}/lib/usrp/usrp_e100/usrp_e_iface.cpp
        ${CMAKE_SOURCE_DIR}/lib/usrp/usrp_e100/usrp_e_iface.hpp
        ${CMAKE_SOURCE_DIR}/lib/usrp/usrp_e100/usrp_e_mmap_zero_copy.cpp
        ${CMAKE_SOURCE_DIR}/lib/usrp/usrp_e100/usrp_e_regs.hpp
    )
ELSE(ENABLE_USRP_E100)
    MESSAGE(STATUS "  Skipping USRP-E100 support.")
ENDIF(ENABLE_USRP_E100)

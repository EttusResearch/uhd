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
# Conditionally configure the USRP-E support
########################################################################
MESSAGE(STATUS "Configuring usrp-e support...")

INCLUDE(CheckIncludeFileCXX)
CHECK_INCLUDE_FILE_CXX(linux/usrp_e.h HAVE_LINUX_USRP_E_H)

IF(HAVE_LINUX_USRP_E_H)
    MESSAGE(STATUS "  Building usrp-e support.")
    LIBUHD_APPEND_SOURCES(
        ${CMAKE_SOURCE_DIR}/lib/usrp/usrp_e/clock_ctrl.cpp
        ${CMAKE_SOURCE_DIR}/lib/usrp/usrp_e/clock_ctrl.hpp
        ${CMAKE_SOURCE_DIR}/lib/usrp/usrp_e/codec_ctrl.cpp
        ${CMAKE_SOURCE_DIR}/lib/usrp/usrp_e/codec_ctrl.hpp
        ${CMAKE_SOURCE_DIR}/lib/usrp/usrp_e/codec_impl.cpp
        ${CMAKE_SOURCE_DIR}/lib/usrp/usrp_e/dboard_impl.cpp
        ${CMAKE_SOURCE_DIR}/lib/usrp/usrp_e/dboard_iface.cpp
        ${CMAKE_SOURCE_DIR}/lib/usrp/usrp_e/dsp_impl.cpp
        ${CMAKE_SOURCE_DIR}/lib/usrp/usrp_e/fpga-downloader.cc
        ${CMAKE_SOURCE_DIR}/lib/usrp/usrp_e/io_impl.cpp
        ${CMAKE_SOURCE_DIR}/lib/usrp/usrp_e/mboard_impl.cpp
        ${CMAKE_SOURCE_DIR}/lib/usrp/usrp_e/usrp_e_impl.cpp
        ${CMAKE_SOURCE_DIR}/lib/usrp/usrp_e/usrp_e_impl.hpp
        ${CMAKE_SOURCE_DIR}/lib/usrp/usrp_e/usrp_e_iface.cpp
        ${CMAKE_SOURCE_DIR}/lib/usrp/usrp_e/usrp_e_iface.hpp
        ${CMAKE_SOURCE_DIR}/lib/usrp/usrp_e/usrp_e_regs.hpp
    )
ELSE(HAVE_LINUX_USRP_E_H)
    MESSAGE(STATUS "  Skipping usrp-e support.")
ENDIF(HAVE_LINUX_USRP_E_H)

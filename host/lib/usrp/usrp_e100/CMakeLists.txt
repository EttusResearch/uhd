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
LIBUHD_REGISTER_COMPONENT("USRP-E100" ENABLE_USRP_E100 OFF "ENABLE_LIBUHD" ON)

IF(ENABLE_USRP_E100)
    INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/lib/usrp/usrp_e100/include)

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
        ${CMAKE_SOURCE_DIR}/lib/usrp/usrp_e100/usrp_e100_impl.cpp
        ${CMAKE_SOURCE_DIR}/lib/usrp/usrp_e100/usrp_e100_impl.hpp
        ${CMAKE_SOURCE_DIR}/lib/usrp/usrp_e100/usrp_e100_iface.cpp
        ${CMAKE_SOURCE_DIR}/lib/usrp/usrp_e100/usrp_e100_iface.hpp
        ${CMAKE_SOURCE_DIR}/lib/usrp/usrp_e100/usrp_e100_mmap_zero_copy.cpp
        ${CMAKE_SOURCE_DIR}/lib/usrp/usrp_e100/usrp_e100_regs.hpp
    )
ENDIF(ENABLE_USRP_E100)

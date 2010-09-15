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
# Conditionally configure the USRP2 support
########################################################################
MESSAGE(STATUS "Configuring USRP2 support...")

IF(DEFINED ENABLE_USRP2)
    IF(ENABLE_USRP2)
        MESSAGE(STATUS "USRP2 support enabled by configure flag")
    ELSE(ENABLE_USRP2)
        MESSAGE(STATUS "USRP2 support disabled by configure flag")
    ENDIF(ENABLE_USRP2)
ELSE(DEFINED ENABLE_USRP2) #not defined: automatic enabling of component
    SET(ENABLE_USRP2 TRUE)
ENDIF(DEFINED ENABLE_USRP2)
SET(ENABLE_USRP2 ${ENABLE_USRP2} CACHE BOOL "enable USRP2 support")

IF(ENABLE_USRP2)
    MESSAGE(STATUS "  Building USRP2 support.")
    LIBUHD_APPEND_SOURCES(
        ${CMAKE_SOURCE_DIR}/lib/usrp/usrp2/clock_ctrl.cpp
        ${CMAKE_SOURCE_DIR}/lib/usrp/usrp2/clock_ctrl.hpp
        ${CMAKE_SOURCE_DIR}/lib/usrp/usrp2/codec_ctrl.cpp
        ${CMAKE_SOURCE_DIR}/lib/usrp/usrp2/codec_ctrl.hpp
        ${CMAKE_SOURCE_DIR}/lib/usrp/usrp2/codec_impl.cpp
        ${CMAKE_SOURCE_DIR}/lib/usrp/usrp2/dboard_impl.cpp
        ${CMAKE_SOURCE_DIR}/lib/usrp/usrp2/dboard_iface.cpp
        ${CMAKE_SOURCE_DIR}/lib/usrp/usrp2/dsp_impl.cpp
        ${CMAKE_SOURCE_DIR}/lib/usrp/usrp2/gps_ctrl.hpp
        ${CMAKE_SOURCE_DIR}/lib/usrp/usrp2/gps_ctrl.cpp
        ${CMAKE_SOURCE_DIR}/lib/usrp/usrp2/io_impl.cpp
        ${CMAKE_SOURCE_DIR}/lib/usrp/usrp2/mboard_impl.cpp
        ${CMAKE_SOURCE_DIR}/lib/usrp/usrp2/serdes_ctrl.cpp
        ${CMAKE_SOURCE_DIR}/lib/usrp/usrp2/serdes_ctrl.hpp
        ${CMAKE_SOURCE_DIR}/lib/usrp/usrp2/usrp2_iface.cpp
        ${CMAKE_SOURCE_DIR}/lib/usrp/usrp2/usrp2_iface.hpp
        ${CMAKE_SOURCE_DIR}/lib/usrp/usrp2/usrp2_impl.cpp
        ${CMAKE_SOURCE_DIR}/lib/usrp/usrp2/usrp2_impl.hpp
        ${CMAKE_SOURCE_DIR}/lib/usrp/usrp2/usrp2_regs.hpp
        ${CMAKE_SOURCE_DIR}/lib/usrp/usrp2/usrp2_regs.cpp
    )
ELSE(ENABLE_USRP2)
    MESSAGE(STATUS "  Skipping USRP2 support.")
ENDIF(ENABLE_USRP2)

#
# Copyright 2011-2013,2016 Ettus Research LLC
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
IF(ENABLE_USB)
    LIBUHD_APPEND_SOURCES(
        ${CMAKE_CURRENT_SOURCE_DIR}/fx2_ctrl.cpp
    )
ENDIF(ENABLE_USB)

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})
INCLUDE_DIRECTORIES("${CMAKE_CURRENT_SOURCE_DIR}/ad9361_driver")

LIBUHD_APPEND_SOURCES(
    ${CMAKE_CURRENT_SOURCE_DIR}/adf4001_ctrl.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/adf435x.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/adf535x.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ad9361_ctrl.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ad936x_manager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ad9361_driver/ad9361_device.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/apply_corrections.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/validate_subdev_spec.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/recv_packet_demuxer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/fifo_ctrl_excelsior.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/usrp3_fw_ctrl_iface.cpp
)

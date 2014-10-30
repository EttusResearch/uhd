#
# Copyright 2014 Ettus Research LLC
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

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})

LIBUHD_APPEND_SOURCES(
    ${CMAKE_CURRENT_SOURCE_DIR}/block_id.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/block_ctrl_base.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/block_ctrl_base_factory.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/block_ctrl_impl.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/blockdef_xml_impl.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/fir_block_ctrl_impl.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/fft_block_ctrl_impl.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/null_block_ctrl_impl.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/window_block_ctrl_impl.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/addsub_block_ctrl_impl.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/split_stream_block_ctrl_impl.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/packet_resizer_block_ctrl_impl.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/radio_ctrl_impl.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/stream_sig.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/rx_block_ctrl_base.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tx_block_ctrl_base.cpp
)

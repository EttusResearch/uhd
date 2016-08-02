#
# Copyright 2014-2015 Ettus Research LLC
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
    # Infrastructure:
    ${CMAKE_CURRENT_SOURCE_DIR}/block_ctrl_base.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/block_ctrl_base_factory.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/block_ctrl_impl.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/blockdef_xml_impl.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/block_id.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ctrl_iface.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/graph_impl.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/legacy_compat.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/node_ctrl_base.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/rate_node_ctrl.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/rx_stream_terminator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/scalar_node_ctrl.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/sink_block_ctrl_base.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/sink_node_ctrl.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source_block_ctrl_base.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source_node_ctrl.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/stream_sig.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tick_node_ctrl.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tx_stream_terminator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/wb_iface_adapter.cpp
    # Default block control classes:
    ${CMAKE_CURRENT_SOURCE_DIR}/ddc_block_ctrl_impl.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/duc_block_ctrl_impl.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/radio_ctrl_impl.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/dma_fifo_block_ctrl_impl
)

INCLUDE_SUBDIRECTORY(nocscript)

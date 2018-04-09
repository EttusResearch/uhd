#
# Copyright 2014-2015,2017 Ettus Research LLC
# Copyright 2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0
#

########################################################################
# This file included, use CMake directory variables
########################################################################

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
    ${CMAKE_CURRENT_SOURCE_DIR}/fir_block_ctrl_impl.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/null_block_ctrl_impl.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/window_block_ctrl_impl.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/radio_ctrl_impl.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/siggen_block_ctrl_impl.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/dma_fifo_block_ctrl_impl.cpp
)

INCLUDE_SUBDIRECTORY(nocscript)

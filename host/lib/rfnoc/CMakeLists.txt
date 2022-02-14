#
# Copyright 2014-2015,2017 Ettus Research LLC
# Copyright 2018 Ettus Research, a National Instruments Company
# Copyright 2019 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

########################################################################
# This file included, use CMake directory variables
########################################################################

LIBUHD_APPEND_SOURCES(
    # Infrastructure:
    ${CMAKE_CURRENT_SOURCE_DIR}/actions.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/block_container.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/block_id.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/chdr_types.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/chdr_packet_writer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/chdr_ctrl_xport.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/chdr_rx_data_xport.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/chdr_tx_data_xport.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/client_zero.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/device_id.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/epid_allocator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/graph.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/link_stream_manager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/graph_stream_manager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/mb_controller.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/noc_block_base.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/node.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/register_iface_holder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ctrlport_endpoint.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/chdr_ctrl_endpoint.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/registry_factory.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/rfnoc_graph.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/mgmt_portal.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/rfnoc_rx_streamer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/rfnoc_tx_streamer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/rfnoc_tx_streamer_replay_buffered.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tx_async_msg_queue.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/mock_block.cpp
    # Default block control classes:
    ${CMAKE_CURRENT_SOURCE_DIR}/addsub_block_control.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/block_control.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ddc_block_control.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/duc_block_control.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/dmafifo_block_control.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/keep_one_in_n_block_control.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/fft_block_control.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/fir_filter_block_control.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/fosphor_block_control.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/logpwr_block_control.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/moving_average_block_control.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/null_block_control.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/radio_control_impl.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/replay_block_control.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/siggen_block_control.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/split_stream_block_control.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/switchboard_block_control.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/vector_iir_block_control.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/window_block_control.cpp
)

INCLUDE_SUBDIRECTORY(rf_control)

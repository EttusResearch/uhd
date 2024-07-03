#
# Copyright 2014-2016 Ettus Research LLC
# Copyright 2018 Ettus Research, a National Instruments Company
# Copyright 2019 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

UHD_INSTALL(FILES
    # Block controllers
    addsub_block_control.hpp
    block_control.hpp
    ddc_block_control.hpp
    dmafifo_block_control.hpp
    duc_block_control.hpp
    fft_block_control.hpp
    fir_filter_block_control.hpp
    fosphor_block_control.hpp
    keep_one_in_n_block_control.hpp
    lc_block_control.hpp
    logpwr_block_control.hpp
    mock_block.hpp
    moving_average_block_control.hpp
    null_block_control.hpp
    radio_control.hpp
    replay_block_control.hpp
    siggen_block_control.hpp
    split_stream_block_control.hpp
    switchboard_block_control.hpp
    vector_iir_block_control.hpp
    window_block_control.hpp
    # Infrastructure
    actions.hpp
    block_id.hpp
    block_controller_factory_python.hpp
    blockdef.hpp
    chdr_types.hpp
    constants.hpp
    defaults.hpp
    dirtifier.hpp
    filter_node.hpp
    graph_edge.hpp
    mb_controller.hpp
    multichan_register_iface.hpp
    noc_block_base.hpp
    noc_block_make_args.hpp
    node.hpp
    node.ipp
    property.hpp
    property.ipp
    radio_control.hpp
    register_iface.hpp
    register_iface_holder.hpp
    registry.hpp
    res_source_info.hpp
    rfnoc_types.hpp
    traffic_counter.hpp

    DESTINATION ${INCLUDE_DIR}/uhd/rfnoc
    COMPONENT headers
)

add_subdirectory(blocks)
add_subdirectory(core)
add_subdirectory(modules)
add_subdirectory(transport_adapters)
add_subdirectory(rf_control)

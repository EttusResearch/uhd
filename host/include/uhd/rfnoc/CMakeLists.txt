#
# Copyright 2014-2016 Ettus Research LLC
# Copyright 2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0
#

IF(ENABLE_RFNOC)
    UHD_INSTALL(FILES
        # Infrastructure
        block_ctrl_base.hpp
        block_ctrl.hpp
        blockdef.hpp
        block_id.hpp
        constants.hpp
        graph.hpp
        node_ctrl_base.hpp
        node_ctrl_base.ipp
        rate_node_ctrl.hpp
        scalar_node_ctrl.hpp
        sink_block_ctrl_base.hpp
        sink_node_ctrl.hpp
        source_block_ctrl_base.hpp
        source_node_ctrl.hpp
        stream_sig.hpp
        terminator_node_ctrl.hpp
        tick_node_ctrl.hpp
        # Block controllers
        ddc_block_ctrl.hpp
        duc_block_ctrl.hpp
        fir_block_ctrl.hpp
        null_block_ctrl.hpp
        radio_ctrl.hpp
        siggen_block_ctrl.hpp
        window_block_ctrl.hpp
        DESTINATION ${INCLUDE_DIR}/uhd/rfnoc
        COMPONENT headers
    )
ENDIF(ENABLE_RFNOC)

ADD_SUBDIRECTORY(blocks)
#ADD_SUBDIRECTORY(components)


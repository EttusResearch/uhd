#
# Copyright 2019 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
""" @package rfnoc
Python UHD module containing the RFNoC API.
"""

from . import libpyuhd as lib

BlockID = lib.rfnoc.block_id
Edge = lib.rfnoc.edge
GraphEdge = lib.rfnoc.graph_edge
Source = lib.rfnoc.source
ResSourceInfo = lib.rfnoc.res_source_info
RfnocGraph = lib.rfnoc.rfnoc_graph
MBController = lib.rfnoc.mb_controller
Timekeeper = lib.rfnoc.timekeeper
NocBlock = lib.rfnoc.noc_block_base
DdcBlockControl = lib.rfnoc.ddc_block_control
DucBlockControl = lib.rfnoc.duc_block_control
FftBlockControl = lib.rfnoc.fft_block_control
FosphorBlockControl = lib.rfnoc.fosphor_block_control
FirFilterBlockControl = lib.rfnoc.fir_filter_block_control
KeepOneInNBlockControl = lib.rfnoc.keep_one_in_n_block_control
LicenseCheckBlockControl = lib.rfnoc.lc_block_control
MovingAverageBlockControl = lib.rfnoc.moving_average_block_control
NullBlockControl = lib.rfnoc.null_block_control
RadioControl = lib.rfnoc.radio_control
ReplayBlockControl = lib.rfnoc.replay_block_control
SiggenBlockControl = lib.rfnoc.siggen_block_control
SwitchboardBlockControl = lib.rfnoc.switchboard_block_control
VectorIirBlockControl = lib.rfnoc.vector_iir_block_control
WindowBlockControl = lib.rfnoc.window_block_control

connect_through_blocks = lib.rfnoc.connect_through_blocks
get_block_chain = lib.rfnoc.get_block_chain

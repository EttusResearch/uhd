#!/usr/bin/python3
#
# Copyright 2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: LGPL-3.0-or-later
#

create_project tmp_proj -part xc7k410tffg900-3 -in_memory
add_files {chdr_crossbar_nxn_top.v ../chdr_crossbar_nxn.v ../axis_switch.v ../chdr_xb_ingress_buff.v ../chdr_xb_routing_table.v ../../core/chdr_mgmt_pkt_handler.v ../../core/rfnoc_chdr_utils.vh ../../core/rfnoc_chdr_internal_utils.vh}
add_files {../../../fifo/axi_fifo_flop.v ../../../fifo/axi_fifo_flop2.v ../../../fifo/axi_fifo.v ../../../fifo/axi_mux_select.v ../../../fifo/axi_fifo_bram.v ../../../fifo/axi_fifo_cascade.v ../../../fifo/axi_mux.v ../../../fifo/axi_fifo_short.v ../../../fifo/axi_demux.v ../../../fifo/axi_packet_gate.v ../../../control/map/cam_priority_encoder.v ../../../control/map/cam_srl.v ../../../control/map/cam_bram.v ../../../control/map/cam.v ../../../control/map/kv_map.v ../../../control/map/axis_muxed_kv_map.v ../../../control/ram_2port.v}
set_property top chdr_crossbar_nxn_top [current_fileset]
synth_design
create_clock -name clk -period 2.0 [get_ports clk]
report_utilization -no_primitives -file chdr_crossbar_nxn.rpt
report_timing_summary -setup -no_detailed_paths -no_header -datasheet -append -file chdr_crossbar_nxn.rpt
write_checkpoint -force chdr_crossbar_nxn.dcp
close_project
exit
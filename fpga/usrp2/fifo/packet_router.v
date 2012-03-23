//
// Copyright 2011-2012 Ettus Research LLC
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

module packet_router
    #(
        parameter BUF_SIZE = 9,
        parameter UDP_BASE = 0,
        parameter CTRL_BASE = 0
    )
    (
        //wishbone interface for memory mapped CPU frames
        input wb_clk_i,
        input wb_rst_i,
        input wb_we_i,
        input wb_stb_i,
        input [15:0] wb_adr_i,
        input [31:0] wb_dat_i,
        output [31:0] wb_dat_o,
        output wb_ack_o,
        output wb_err_o,
        output wb_rty_o,

        //setting register interface
        input set_stb, input [7:0] set_addr, input [31:0] set_data,

        input stream_clk,
        input stream_rst,
        input stream_clr,

        //output status register
        output [31:0] status,

        output sys_int_o, //want an interrupt?

        output [31:0] debug,

        // Input Interfaces (in to router)
        input [35:0] ser_inp_data, input ser_inp_valid, output ser_inp_ready,
        input [35:0] dsp0_inp_data, input dsp0_inp_valid, output dsp0_inp_ready,
        input [35:0] dsp1_inp_data, input dsp1_inp_valid, output dsp1_inp_ready,
        input [35:0] eth_inp_data, input eth_inp_valid, output eth_inp_ready,
        input [35:0] err_inp_data, input err_inp_valid, output err_inp_ready,
        input [35:0] ctl_inp_data, input ctl_inp_valid, output ctl_inp_ready,

        // Output Interfaces (out of router)
        output [35:0] ser_out_data, output ser_out_valid, input ser_out_ready,
        output [35:0] dsp_out_data, output dsp_out_valid, input dsp_out_ready,
        output [35:0] ctl_out_data, output ctl_out_valid, input ctl_out_ready,
        output [35:0] eth_out_data, output eth_out_valid, input eth_out_ready
    );

    assign wb_err_o = 1'b0;  // Unused for now
    assign wb_rty_o = 1'b0;  // Unused for now

    ////////////////////////////////////////////////////////////////////
    // CPU interface to this packet router
    ////////////////////////////////////////////////////////////////////
    wire [35:0] cpu_inp_data,  cpu_out_data;
    wire        cpu_inp_valid, cpu_out_valid;
    wire        cpu_inp_ready, cpu_out_ready;

    ////////////////////////////////////////////////////////////////////
    // Communication interfaces
    ////////////////////////////////////////////////////////////////////
    wire [35:0] com_inp_data,  com_out_data,  udp_out_data;
    wire        com_inp_valid, com_out_valid, udp_out_valid;
    wire        com_inp_ready, com_out_ready, udp_out_ready;

    ////////////////////////////////////////////////////////////////////
    // Control signals (setting registers and status signals)
    //    - handshake lines for the CPU communication
    //    - setting registers to program the inspector
    ////////////////////////////////////////////////////////////////////

    //setting register for mode control
    wire [31:0] _sreg_mode_ctrl;
    wire        master_mode_flag;
   
    setting_reg #(.my_addr(CTRL_BASE+0), .width(1)) sreg_mode_ctrl(
        .clk(stream_clk),.rst(stream_rst),
        .strobe(set_stb),.addr(set_addr),.in(set_data),
        .out(master_mode_flag),.changed()
    );

    //assign status output signals
    wire [31:0] cpu_iface_status;
    assign status = {
        cpu_iface_status[31:9], master_mode_flag, cpu_iface_status[7:0]
    };

    ////////////////////////////////////////////////////////////////////
    // Communication input source crossbar
    // When in master mode:
    //   - serdes input -> comm output combiner
    //   - ethernet input -> comm input inspector
    // When in slave mode:
    //   - serdes input -> comm input inspector
    //   - ethernet input -> null sink
    ////////////////////////////////////////////////////////////////////

    //streaming signals from the crossbar to the combiner
    wire [35:0] ext_inp_data;
    wire        ext_inp_valid;
    wire        ext_inp_ready;

    //dummy signals for valve/xbar below
    wire [35:0] _eth_inp_data;
    wire        _eth_inp_valid;
    wire        _eth_inp_ready;

    // dummy signals to connect fifo_short
    wire [35:0] _com_inp_data;
    wire        _com_inp_valid;
    wire        _com_inp_ready;

    valve36 eth_inp_valve (
        .clk(stream_clk), .reset(stream_rst), .clear(stream_clr), .shutoff(~master_mode_flag),
        .data_i(eth_inp_data), .src_rdy_i(eth_inp_valid), .dst_rdy_o(eth_inp_ready),
        .data_o(_eth_inp_data), .src_rdy_o(_eth_inp_valid), .dst_rdy_i(_eth_inp_ready)
    );

    crossbar36 com_inp_xbar (
        .clk(stream_clk), .reset(stream_rst), .clear(stream_clr), .cross(~master_mode_flag),
        .data0_i(_eth_inp_data), .src0_rdy_i(_eth_inp_valid), .dst0_rdy_o(_eth_inp_ready),
        .data1_i(ser_inp_data), .src1_rdy_i(ser_inp_valid), .dst1_rdy_o(ser_inp_ready),
        .data0_o(_com_inp_data), .src0_rdy_o(_com_inp_valid), .dst0_rdy_i(_com_inp_ready),
        .data1_o(ext_inp_data), .src1_rdy_o(ext_inp_valid), .dst1_rdy_i(ext_inp_ready)
    );

    //  short fifo in the packet inspection path to help timing
    fifo_short #(.WIDTH(36)) com_inp_fifo
     (.clk(stream_clk), .reset(stream_rst), .clear(stream_clr),
      .datain(_com_inp_data),  .src_rdy_i(_com_inp_valid), .dst_rdy_o(_com_inp_ready),
      .dataout(com_inp_data), .src_rdy_o(com_inp_valid), .dst_rdy_i(com_inp_ready),
      .space(), .occupied() );

    ////////////////////////////////////////////////////////////////////
    // Communication output sink crossbar
    // When in master mode:
    //   - comm output -> ethernet output
    //   - insp output -> serdes output
    // When in slave mode:
    //   - com output -> serdes output
    //   - insp output -> null sink
    ////////////////////////////////////////////////////////////////////

    //streaming signals from the inspector to the crossbar
    wire [35:0] ext_out_data;
    wire        ext_out_valid;
    wire        ext_out_ready;

    //dummy signals for valve/xbar below
    wire [35:0] _eth_out_data;
    wire        _eth_out_valid;
    wire        _eth_out_ready;

    crossbar36 com_out_xbar (
        .clk(stream_clk), .reset(stream_rst), .clear(stream_clr), .cross(~master_mode_flag),
        .data0_i(com_out_data), .src0_rdy_i(com_out_valid), .dst0_rdy_o(com_out_ready),
        .data1_i(ext_out_data), .src1_rdy_i(ext_out_valid), .dst1_rdy_o(ext_out_ready),
        .data0_o(_eth_out_data), .src0_rdy_o(_eth_out_valid), .dst0_rdy_i(_eth_out_ready),
        .data1_o(ser_out_data), .src1_rdy_o(ser_out_valid), .dst1_rdy_i(ser_out_ready)
    );

    valve36 eth_out_valve (
        .clk(stream_clk), .reset(stream_rst), .clear(stream_clr), .shutoff(~master_mode_flag),
        .data_i(_eth_out_data), .src_rdy_i(_eth_out_valid), .dst_rdy_o(_eth_out_ready),
        .data_o(eth_out_data), .src_rdy_o(eth_out_valid), .dst_rdy_i(eth_out_ready)
    );

    ////////////////////////////////////////////////////////////////////
    // Communication output source combiner (feeds UDP proto machine)
    //   - DSP input
    //   - CPU input
    //   - ERR input
    ////////////////////////////////////////////////////////////////////

    //dummy signals to join the the muxes below
    wire [35:0] _combiner0_data, _combiner1_data, _combiner2_data;
    wire        _combiner0_valid, _combiner1_valid, _combiner2_valid;
    wire        _combiner0_ready, _combiner1_ready, _combiner2_ready;

    fifo36_mux #(.prio(0)) // No priority, fair sharing
     _com_output_combiner0(
        .clk(stream_clk), .reset(stream_rst), .clear(stream_clr),
        .data0_i(err_inp_data), .src0_rdy_i(err_inp_valid), .dst0_rdy_o(err_inp_ready),
        .data1_i(cpu_inp_data), .src1_rdy_i(cpu_inp_valid), .dst1_rdy_o(cpu_inp_ready),
        .data_o(_combiner0_data), .src_rdy_o(_combiner0_valid), .dst_rdy_i(_combiner0_ready)
    );

    fifo36_mux #(.prio(0)) // No priority, fair sharing
     _com_output_combiner2(
        .clk(stream_clk), .reset(stream_rst), .clear(stream_clr),
        .data0_i(_combiner0_data), .src0_rdy_i(_combiner0_valid), .dst0_rdy_o(_combiner0_ready),
        .data1_i(ctl_inp_data), .src1_rdy_i(ctl_inp_valid), .dst1_rdy_o(ctl_inp_ready),
        .data_o(_combiner2_data), .src_rdy_o(_combiner2_valid), .dst_rdy_i(_combiner2_ready)
    );

    fifo36_mux #(.prio(0)) // No priority, fair sharing
     _com_output_combiner1(
        .clk(stream_clk), .reset(stream_rst), .clear(stream_clr),
        .data0_i(dsp0_inp_data), .src0_rdy_i(dsp0_inp_valid), .dst0_rdy_o(dsp0_inp_ready),
        .data1_i(dsp1_inp_data), .src1_rdy_i(dsp1_inp_valid), .dst1_rdy_o(dsp1_inp_ready),
        .data_o(_combiner1_data), .src_rdy_o(_combiner1_valid), .dst_rdy_i(_combiner1_ready)
    );

    fifo36_mux #(.prio(1)) // Give priority to err/cpu over dsp
     com_output_source(
        .clk(stream_clk), .reset(stream_rst), .clear(stream_clr),
        .data0_i(_combiner2_data), .src0_rdy_i(_combiner2_valid), .dst0_rdy_o(_combiner2_ready),
        .data1_i(_combiner1_data), .src1_rdy_i(_combiner1_valid), .dst1_rdy_o(_combiner1_ready),
        .data_o(udp_out_data), .src_rdy_o(udp_out_valid), .dst_rdy_i(udp_out_ready)
    );

    ////////////////////////////////////////////////////////////////////
    // Interface CPU to memory mapped wishbone
    //   - Uses 1 setting register
    ////////////////////////////////////////////////////////////////////
    buffer_int2 #(.BASE(CTRL_BASE+3), .BUF_SIZE(BUF_SIZE)) cpu_to_wb(
        .clk(stream_clk), .rst(stream_rst | stream_clr),
        .set_stb(set_stb), .set_addr(set_addr), .set_data(set_data),
        .status(cpu_iface_status),
        // Wishbone interface to RAM
        .wb_clk_i(wb_clk_i), .wb_rst_i(wb_rst_i),
        .wb_we_i(wb_we_i),   .wb_stb_i(wb_stb_i),
        .wb_adr_i(wb_adr_i), .wb_dat_i(wb_dat_i),
        .wb_dat_o(wb_dat_o), .wb_ack_o(wb_ack_o),
        // Write FIFO Interface (from PR and into WB)
        .wr_data_i(cpu_out_data),
        .wr_ready_i(cpu_out_valid),
        .wr_ready_o(cpu_out_ready),
        // Read FIFO Interface (from WB and into PR)
        .rd_data_o(cpu_inp_data),
        .rd_ready_o(cpu_inp_valid),
        .rd_ready_i(cpu_inp_ready)
    );

    ////////////////////////////////////////////////////////////////////
    // Packet Dispatcher
    //   - Uses 2 setting registers
    //   - provide buffering before cpu for random + small packet bursts
    ////////////////////////////////////////////////////////////////////
    wire [35:0] _cpu_out_data;
    wire        _cpu_out_valid;
    wire        _cpu_out_ready;

    packet_dispatcher36_x4 #(.BASE(CTRL_BASE+1)) packet_dispatcher(
        .clk(stream_clk), .rst(stream_rst), .clr(stream_clr),
        .set_stb(set_stb), .set_addr(set_addr), .set_data(set_data),
        .com_inp_data(com_inp_data), .com_inp_valid(com_inp_valid), .com_inp_ready(com_inp_ready),
        .ext_out_data(ext_out_data), .ext_out_valid(ext_out_valid), .ext_out_ready(ext_out_ready),
        .dsp_out_data(dsp_out_data), .dsp_out_valid(dsp_out_valid), .dsp_out_ready(dsp_out_ready),
        .ctl_out_data(ctl_out_data), .ctl_out_valid(ctl_out_valid), .ctl_out_ready(ctl_out_ready),
        .cpu_out_data(_cpu_out_data), .cpu_out_valid(_cpu_out_valid), .cpu_out_ready(_cpu_out_ready)
    );

    fifo_cascade #(.WIDTH(36), .SIZE(9/*512 lines plenty for short pkts*/)) cpu_out_fifo (
        .clk(stream_clk), .reset(stream_rst), .clear(stream_clr),
        .datain(_cpu_out_data), .src_rdy_i(_cpu_out_valid), .dst_rdy_o(_cpu_out_ready),
        .dataout(cpu_out_data), .src_rdy_o(cpu_out_valid),  .dst_rdy_i(cpu_out_ready)
    );

    ////////////////////////////////////////////////////////////////////
    // UDP TX Protocol machine
    ////////////////////////////////////////////////////////////////////

    //dummy signals to connect the components below
    wire [35:0] _com_out_data;
    wire _com_out_valid, _com_out_ready;

    prot_eng_tx #(.BASE(UDP_BASE)) udp_prot_eng_tx
     (.clk(stream_clk), .reset(stream_rst), .clear(stream_clr),
      .set_stb(set_stb), .set_addr(set_addr), .set_data(set_data),
      .datain(udp_out_data),  .src_rdy_i(udp_out_valid), .dst_rdy_o(udp_out_ready),
      .dataout(_com_out_data), .src_rdy_o(_com_out_valid), .dst_rdy_i(_com_out_ready) );

    fifo36_mux com_out_mux(
        .clk(stream_clk), .reset(stream_rst), .clear(stream_clr),
        .data0_i(ext_inp_data),  .src0_rdy_i(ext_inp_valid),  .dst0_rdy_o(ext_inp_ready),
        .data1_i(_com_out_data), .src1_rdy_i(_com_out_valid), .dst1_rdy_o(_com_out_ready),
        .data_o(com_out_data),   .src_rdy_o(com_out_valid),   .dst_rdy_i(com_out_ready)
    );

    ////////////////////////////////////////////////////////////////////
    // Assign debugs
    ////////////////////////////////////////////////////////////////////

    assign debug = {
        //inputs to the router (12)
        dsp0_inp_ready, dsp0_inp_valid,
        dsp1_inp_ready, dsp1_inp_valid,
        err_inp_ready, err_inp_valid,
        ser_inp_ready, ser_inp_valid,
        eth_inp_ready, eth_inp_valid,
        cpu_inp_ready, cpu_inp_valid,

        //outputs from the router (8)
        dsp_out_ready, dsp_out_valid,
        ser_out_ready, ser_out_valid,
        eth_out_ready, eth_out_valid,
        cpu_out_ready, cpu_out_valid,

        //other interfaces (8)
        ext_inp_ready, ext_inp_valid,
        com_out_ready, com_out_valid,
        ext_out_ready, ext_out_valid,
        com_inp_ready, com_inp_valid,

        4'b0
    };

endmodule // packet_router

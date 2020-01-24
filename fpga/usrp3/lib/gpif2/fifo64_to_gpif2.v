//
// Copyright 2012-2013 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//


module fifo64_to_gpif2
#(
    parameter FIFO_SIZE = 9,
    parameter MTU = 12
)
(
    //input fifo interface
    input fifo_clk, input fifo_rst,
    input [63:0] i_tdata,
    input i_tlast,
    input i_tvalid,
    output i_tready,

    //output interface
    input gpif_clk, input gpif_rst,
    output [31:0] o_tdata,
    output o_tlast,
    output o_tvalid,
    input o_tready
);

    wire [31:0] i32_tdata;
    wire i32_tlast;
    wire i32_tvalid, i32_tready;

    axi_fifo64_to_fifo32 fifo64_to_fifo32
    (
        .clk(fifo_clk), .reset(fifo_rst), .clear(1'b0),
        .i_tdata(i_tdata), .i_tuser(3'b0/*done care*/), .i_tlast(i_tlast), .i_tvalid(i_tvalid), .i_tready(i_tready),
        .o_tdata(i32_tdata), .o_tuser(/*ignored cuz vita has len*/), .o_tlast(i32_tlast), .o_tvalid(i32_tvalid), .o_tready(i32_tready)
    );

    wire [31:0] gate_tdata;
    wire gate_tlast;
    wire gate_tvalid, gate_tready;

    axi_fifo_2clk #(.WIDTH(33), .SIZE(FIFO_SIZE)) cross_clock_fifo
    (
        .reset(fifo_rst | gpif_rst),
        .i_aclk(fifo_clk), .i_tdata({i32_tlast, i32_tdata}), .i_tvalid(i32_tvalid), .i_tready(i32_tready),
        .o_aclk(gpif_clk), .o_tdata({gate_tlast, gate_tdata}), .o_tvalid(gate_tvalid), .o_tready(gate_tready)
    );

    wire [31:0] int0_tdata; wire int0_tlast, int0_tvalid, int0_tready;

    axi_packet_gate #(.WIDTH(32), .SIZE(MTU), .USE_AS_BUFF(0)) buffer_whole_pkt
    (
        .clk(gpif_clk), .reset(gpif_rst), .clear(1'b0),
        .i_tdata(gate_tdata), .i_tlast(gate_tlast), .i_terror(1'b0), .i_tvalid(gate_tvalid), .i_tready(gate_tready),
        .o_tdata(int0_tdata), .o_tlast(int0_tlast), .o_tvalid(int0_tvalid), .o_tready(int0_tready)
    );

    axi_fifo #(.WIDTH(33), .SIZE(1)) outgress_timing_fifo
    (
        .clk(gpif_clk), .reset(gpif_rst), .clear(1'b0),
        .i_tdata({int0_tlast, int0_tdata}), .i_tvalid(int0_tvalid), .i_tready(int0_tready), .space(),
        .o_tdata({o_tlast, o_tdata}), .o_tvalid(o_tvalid), .o_tready(o_tready), .occupied()
    );

endmodule //fifo_to_gpmc16


// Copyright 2012 Ettus Research LLC
// axi_mux -- takes 8 64-bit AXI stream, merges them to 1 output channel
// Round-robin if PRIO=0, priority if PRIO=1 (lower number ports get priority)
// Bubble cycles are inserted after each packet in PRIO mode, or on wraparound in Round Robin mode

module axi_mux8 #(
   parameter PRIO=0,
   parameter WIDTH=64,
   parameter BUFFER=0
) (
   input clk, input reset, input clear,
   input [WIDTH-1:0] i0_tdata, input i0_tlast, input i0_tvalid, output i0_tready,
   input [WIDTH-1:0] i1_tdata, input i1_tlast, input i1_tvalid, output i1_tready,
   input [WIDTH-1:0] i2_tdata, input i2_tlast, input i2_tvalid, output i2_tready,
   input [WIDTH-1:0] i3_tdata, input i3_tlast, input i3_tvalid, output i3_tready,
   input [WIDTH-1:0] i4_tdata, input i4_tlast, input i4_tvalid, output i4_tready,
   input [WIDTH-1:0] i5_tdata, input i5_tlast, input i5_tvalid, output i5_tready,
   input [WIDTH-1:0] i6_tdata, input i6_tlast, input i6_tvalid, output i6_tready,
   input [WIDTH-1:0] i7_tdata, input i7_tlast, input i7_tvalid, output i7_tready,
   output [WIDTH-1:0] o_tdata, output o_tlast, output o_tvalid, input o_tready
);

   wire [WIDTH-1:0]  o_tdata_int0, o_tdata_int1;
   wire              o_tlast_int0, o_tlast_int1;
   wire              o_tvalid_int0, o_tvalid_int1;
   wire              o_tready_int0, o_tready_int1;

   axi_mux4 #(.PRIO(PRIO), .WIDTH(WIDTH), .BUFFER(0)) mux4_int0 (
      .clk(clk), .reset(reset), .clear(clear),
      .i0_tdata(i0_tdata), .i0_tlast(i0_tlast), .i0_tvalid(i0_tvalid), .i0_tready(i0_tready),
      .i1_tdata(i1_tdata), .i1_tlast(i1_tlast), .i1_tvalid(i1_tvalid), .i1_tready(i1_tready),
      .i2_tdata(i2_tdata), .i2_tlast(i2_tlast), .i2_tvalid(i2_tvalid), .i2_tready(i2_tready),
      .i3_tdata(i3_tdata), .i3_tlast(i3_tlast), .i3_tvalid(i3_tvalid), .i3_tready(i3_tready),
      .o_tdata(o_tdata_int0), .o_tlast(o_tlast_int0), .o_tvalid(o_tvalid_int0), .o_tready(o_tready_int0)
   );

   axi_mux4 #(.PRIO(PRIO), .WIDTH(WIDTH), .BUFFER(0)) mux4_int1 (
      .clk(clk), .reset(reset), .clear(clear),
      .i0_tdata(i4_tdata), .i0_tlast(i4_tlast), .i0_tvalid(i4_tvalid), .i0_tready(i4_tready),
      .i1_tdata(i5_tdata), .i1_tlast(i5_tlast), .i1_tvalid(i5_tvalid), .i1_tready(i5_tready),
      .i2_tdata(i6_tdata), .i2_tlast(i6_tlast), .i2_tvalid(i6_tvalid), .i2_tready(i6_tready),
      .i3_tdata(i7_tdata), .i3_tlast(i7_tlast), .i3_tvalid(i7_tvalid), .i3_tready(i7_tready),
      .o_tdata(o_tdata_int1), .o_tlast(o_tlast_int1), .o_tvalid(o_tvalid_int1), .o_tready(o_tready_int1)
   );

   axi_mux4 #(.PRIO(PRIO), .WIDTH(WIDTH), .BUFFER(BUFFER)) mux2 (
      .clk(clk), .reset(reset), .clear(clear),
      .i0_tdata(o_tdata_int0), .i0_tlast(o_tlast_int0), .i0_tvalid(o_tvalid_int0), .i0_tready(o_tready_int0),
      .i1_tdata(o_tdata_int1), .i1_tlast(o_tlast_int1), .i1_tvalid(o_tvalid_int1), .i1_tready(o_tready_int1),
      .i2_tdata(0), .i2_tlast(1'b0), .i2_tvalid(1'b0), .i2_tready(),
      .i3_tdata(0), .i3_tlast(1'b0), .i3_tvalid(1'b0), .i3_tready(),
      .o_tdata(o_tdata), .o_tlast(o_tlast), .o_tvalid(o_tvalid), .o_tready(o_tready)
   );

endmodule // axi_mux8

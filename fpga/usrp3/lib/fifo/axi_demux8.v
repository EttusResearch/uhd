
// Copyright 2012 Ettus Research LLC
// axi_demux -- takes one AXI stream, sends to one of 8 output channels
//   Choice of output channel is by external logic based on first line of packet ("header" port)
//   If compressed vita data, this line contains vita header and streamid.

module axi_demux8 #(
   parameter ACTIVE_CHAN = 8'b11111111,  // ACTIVE_CHAN is a map of connected outputs
   parameter WIDTH = 64,
   parameter BUFFER=0
) (
   input clk, input reset, input clear,
   output [WIDTH-1:0] header, input [2:0] dest,
   input [WIDTH-1:0] i_tdata, input i_tlast, input i_tvalid, output i_tready,
   output [WIDTH-1:0] o0_tdata, output o0_tlast, output o0_tvalid, input o0_tready,
   output [WIDTH-1:0] o1_tdata, output o1_tlast, output o1_tvalid, input o1_tready,
   output [WIDTH-1:0] o2_tdata, output o2_tlast, output o2_tvalid, input o2_tready,
   output [WIDTH-1:0] o3_tdata, output o3_tlast, output o3_tvalid, input o3_tready,
   output [WIDTH-1:0] o4_tdata, output o4_tlast, output o4_tvalid, input o4_tready,
   output [WIDTH-1:0] o5_tdata, output o5_tlast, output o5_tvalid, input o5_tready,
   output [WIDTH-1:0] o6_tdata, output o6_tlast, output o6_tvalid, input o6_tready,
   output [WIDTH-1:0] o7_tdata, output o7_tlast, output o7_tvalid, input o7_tready
);

   wire [WIDTH-1:0]  i_tdata_int0, i_tdata_int1;
   wire              i_tlast_int0, i_tlast_int1;
   wire              i_tvalid_int0, i_tvalid_int1;
   wire              i_tready_int0, i_tready_int1;

   axi_demux4 #(.ACTIVE_CHAN({2'b00, (|(ACTIVE_CHAN[7:4])), (|(ACTIVE_CHAN[3:0]))}), .WIDTH(WIDTH), .BUFFER(BUFFER)) demux2 (
      .clk(clk), .reset(reset), .clear(clear),
      .header(header), .dest({1'b0, dest[2]}),
      .i_tdata(i_tdata), .i_tlast(i_tlast), .i_tvalid(i_tvalid), .i_tready(i_tready),
      .o0_tdata(i_tdata_int0), .o0_tlast(i_tlast_int0), .o0_tvalid(i_tvalid_int0), .o0_tready(i_tready_int0),
      .o1_tdata(i_tdata_int1), .o1_tlast(i_tlast_int1), .o1_tvalid(i_tvalid_int1), .o1_tready(i_tready_int1),
      .o2_tdata(), .o2_tlast(), .o2_tvalid(), .o2_tready(1'b0),
      .o3_tdata(), .o3_tlast(), .o3_tvalid(), .o3_tready(1'b0)
   );

   axi_demux4 #(.ACTIVE_CHAN(ACTIVE_CHAN[3:0]), .WIDTH(WIDTH), .BUFFER(0)) demux4_int0 (
      .clk(clk), .reset(reset), .clear(clear),
      .header(), .dest(dest[1:0]),
      .i_tdata(i_tdata_int0), .i_tlast(i_tlast_int0), .i_tvalid(i_tvalid_int0), .i_tready(i_tready_int0),
      .o0_tdata(o0_tdata), .o0_tlast(o0_tlast), .o0_tvalid(o0_tvalid), .o0_tready(o0_tready),
      .o1_tdata(o1_tdata), .o1_tlast(o1_tlast), .o1_tvalid(o1_tvalid), .o1_tready(o1_tready),
      .o2_tdata(o2_tdata), .o2_tlast(o2_tlast), .o2_tvalid(o2_tvalid), .o2_tready(o2_tready),
      .o3_tdata(o3_tdata), .o3_tlast(o3_tlast), .o3_tvalid(o3_tvalid), .o3_tready(o3_tready)
   );

   axi_demux4 #(.ACTIVE_CHAN(ACTIVE_CHAN[7:4]), .WIDTH(WIDTH), .BUFFER(0)) demux4_int1 (
      .clk(clk), .reset(reset), .clear(clear),
      .header(), .dest(dest[1:0]),
      .i_tdata(i_tdata_int1), .i_tlast(i_tlast_int1), .i_tvalid(i_tvalid_int1), .i_tready(i_tready_int1),
      .o0_tdata(o4_tdata), .o0_tlast(o4_tlast), .o0_tvalid(o4_tvalid), .o0_tready(o4_tready),
      .o1_tdata(o5_tdata), .o1_tlast(o5_tlast), .o1_tvalid(o5_tvalid), .o1_tready(o5_tready),
      .o2_tdata(o6_tdata), .o2_tlast(o6_tlast), .o2_tvalid(o6_tvalid), .o2_tready(o6_tready),
      .o3_tdata(o7_tdata), .o3_tlast(o7_tlast), .o3_tvalid(o7_tvalid), .o3_tready(o7_tready)
   );
   
endmodule // axi_demux4

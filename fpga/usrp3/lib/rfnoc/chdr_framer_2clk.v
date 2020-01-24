/////////////////////////////////////////////////////////////////////
//
// Copyright 2018 Ettus Research, A National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: chdr_framer_2clk
// Description:
//  - Takes a sample stream in and uses the tuser input to frame
//    a CHDR packet which is output by the module
//    samples at the output
//  - FIXME Currently only 32 / 64-bit input widths are supported.
//
/////////////////////////////////////////////////////////////////////

module chdr_framer_2clk #(
  parameter SIZE        = 10,
  parameter WIDTH       = 32,  // 32 or 64 only! TODO: Extend to other widths.
  parameter USE_SEQ_NUM = 0   // Use provided seq number in tuser
) (
  input samp_clk, input samp_rst, input pkt_clk, input pkt_rst,
  input [WIDTH-1:0] i_tdata, input [127:0] i_tuser, input i_tlast, input i_tvalid, output i_tready,
  output [63:0] o_tdata, output o_tlast, output o_tvalid, input o_tready
);

  wire          header_i_tvalid, header_i_tready;
  wire [63:0]   body_i_tdata;
  wire          body_i_tlast, body_i_tvalid, body_i_tready;

  wire [127:0]  header_o_tdata;
  wire          header_o_tvalid, header_o_tready;
  wire [63:0]   body_o_tdata;
  wire          body_o_tlast, body_o_tvalid, body_o_tready;
  reg  [15:0]   length;
  reg  [11:0]   seqnum;

  assign i_tready = header_i_tready & body_i_tready;
  assign header_i_tvalid = i_tlast & i_tvalid & i_tready;
  assign body_i_tlast = i_tlast;

  // Handle 32 and 64 widths
  generate
    if (WIDTH == 32) begin
     reg even = 1'b0;
     always @(posedge samp_clk)
       if(samp_rst)
         even <= 1'b0;
       else
         if(i_tvalid & i_tready)
           if(i_tlast)
             even <= 1'b0;
           else
             even <= ~even;

     reg [31:0] held_i_tdata;
     always @(posedge samp_clk) begin
       if (i_tvalid & i_tready) held_i_tdata <= i_tdata;
     end
     assign body_i_tvalid = i_tvalid & i_tready & (i_tlast | even);
     assign body_i_tdata  = even ? { held_i_tdata, i_tdata } : {i_tdata, i_tdata}; // really should be 0 in bottom, but this simplifies mux
    end else begin
     assign body_i_tvalid = i_tvalid & i_tready;
     assign body_i_tdata  = i_tdata;
    end
  endgenerate

  // FIXME handle lengths of partial 32-bit words
  always @(posedge samp_clk)
    if (samp_rst)
      length <= (WIDTH == 32) ? 16'd4 : 16'd8;
    else if(header_i_tready & header_i_tvalid)
      length <= (WIDTH == 32) ? 16'd4 : 16'd8;
    else if(i_tvalid & i_tready)
      length <= (WIDTH == 32) ? length + 16'd4 : length + 16'd8;

  // Extended reset signal to ensure longer reset on axi_fifo_2clk
  // as recommended by Xilinx. It clears all partial packets seen
  // after clearing the fifos.
  // This pulse stretch ratio works in this case and may not work
  // for all clocks.
  wire samp_rst_stretch;
  pulse_stretch #(.SCALE('d10)) samp_reset_i (
    .clk(samp_clk),
    .rst(1'b0),
    .pulse(samp_rst),
    .pulse_stretched(samp_rst_stretch)
  );

  axi_fifo_2clk #(.WIDTH(128), .SIZE(5)) hdr_fifo_i (
    .i_aclk(samp_clk), .o_aclk(pkt_clk), .reset(samp_rst_stretch),
    .i_tdata({i_tuser[127:112],length,i_tuser[95:0]}), .i_tvalid(header_i_tvalid), .i_tready(header_i_tready),
    .o_tdata(header_o_tdata), .o_tvalid(header_o_tvalid), .o_tready(header_o_tready)
  );

  axi_fifo_2clk #(.WIDTH(65), .SIZE(SIZE)) body_fifo_i (
    .i_aclk(samp_clk), .o_aclk(pkt_clk), .reset(samp_rst_stretch),
    .i_tdata({body_i_tlast,body_i_tdata}), .i_tvalid(body_i_tvalid), .i_tready(body_i_tready),
    .o_tdata({body_o_tlast,body_o_tdata}), .o_tvalid(body_o_tvalid), .o_tready(body_o_tready)
  );

  reg [1:0] chdr_state;
  localparam [1:0] ST_IDLE = 0;
  localparam [1:0] ST_HEAD = 1;
  localparam [1:0] ST_TIME = 2;
  localparam [1:0] ST_BODY = 3;

  always @(posedge pkt_clk)
    if(pkt_rst)
      chdr_state <= ST_IDLE;
    else
      case(chdr_state)
      ST_IDLE :
        if(header_o_tvalid & body_o_tvalid)
          chdr_state <= ST_HEAD;
      ST_HEAD :
        if(o_tready)
          if(header_o_tdata[125])  // time
            chdr_state <= ST_TIME;
          else
            chdr_state <= ST_BODY;
      ST_TIME :
        if(o_tready)
          chdr_state <= ST_BODY;
      ST_BODY :
        if(o_tready & body_o_tlast)
          chdr_state <= ST_IDLE;
      endcase

  always @(posedge pkt_clk)
    if(pkt_rst)
      seqnum <= 12'd0;
    else
      if(o_tvalid & o_tready & o_tlast)
        seqnum <= seqnum + 12'd1;

  wire [15:0] out_length = header_o_tdata[111:96] + (header_o_tdata[125] ? 16'd16 : 16'd8);

  assign o_tvalid = (chdr_state == ST_HEAD) | (chdr_state == ST_TIME) | (body_o_tvalid & (chdr_state == ST_BODY));
  assign o_tlast = (chdr_state == ST_BODY) & body_o_tlast;
  assign o_tdata = (chdr_state == ST_HEAD) ? {header_o_tdata[127:124], (USE_SEQ_NUM == 1 ? header_o_tdata[123:112] : seqnum), out_length, header_o_tdata[95:64] } :
                     (chdr_state == ST_TIME) ? header_o_tdata[63:0] :
                        body_o_tdata;
  assign body_o_tready = (chdr_state == ST_BODY) & o_tready;
  assign header_o_tready = ((chdr_state == ST_TIME) | ((chdr_state == ST_HEAD) & ~header_o_tdata[125])) & o_tready;

endmodule

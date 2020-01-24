//
// Copyright 2015 Ettus Research
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Assumes 32-bit elements (such as sc16) carried over AXI-Stream
// SIMPLE_MODE          -- Automatically handle header (s_axis_data_tuser), packets must be consumed / produced 1-to-1
// RESIZE_INPUT_PACKET  -- Resize input packets. m_axis_data_tlast will be based on m_axis_pkt_len_tdata. Otherwise packet length based on actual input packet length (via i_tlast).
// RESIZE_OUTPUT_PACKET -- Resize output packets. s_axis_data_tlast will be ignored and instead use packet length in s_axis_tuser_data. Otherwise use s_axis_data_tlast.
//
// Note: When SIMPLE_MODE = 1 and RESIZE_OUTPUT_PACKET = 1, s_axis_data_tlast is ignored and output packets are sized according to the length
//       of the input packet (via the packet length field in the received header). Useful if the user design wants output packet length to 
//       match the input packet length without having to drive s_axis_data_tlast.
//
// *** Warning: Care should be taken when using RESIZE_INPUT_PACKET and/or RESIZE_OUTPUT_PACKET along with SIMPLE_MODE
//              as issues could arise if packets are not produced / consumed in a 1:1 ratio. For instance, the header
//              FIFO could overflow or underflow.

// _tuser bit definitions
//  [127:64] == CHDR header
//    [127:126] == Packet type -- 00 for data, 01 for flow control, 10 for command, 11 for response
//    [125]     == Has time? (0 for no, 1 for time field on next line)
//    [124]     == EOB (end of burst indicator)
//    [123:112] == 12-bit sequence number
//    [111: 96] == 16-bit length in bytes
//    [ 95: 80] == SRC SID (stream ID)
//    [ 79: 64] == DST SID
//  [ 63: 0] == timestamp

module axi_wrapper
  #(parameter MTU=10,
    parameter SR_AXI_CONFIG_BASE=129,   // AXI configuration bus base, settings bus address range size is 2*NUM_AXI_CONFIG_BUS
    parameter NUM_AXI_CONFIG_BUS=1,     // Number of AXI configuration buses
    parameter CONFIG_BUS_FIFO_DEPTH=1,  // Depth of AXI configuration bus FIFO. Note: AXI configuration bus lacks back pressure.
    parameter SIMPLE_MODE=1,            // 0 = User handles CHDR insertion via tuser signals, 1 = Automatically save / insert CHDR with internal FIFO
    parameter USE_SEQ_NUM=0,            // 0 = Frame will automatically handle sequence number, 1 = Use sequence number provided in s_axis_data_tuser
    parameter RESIZE_INPUT_PACKET=0,    // 0 = Do not resize, packet length determined by i_tlast, 1 = Generate m_axis_data_tlast based on user input m_axis_pkt_len_tdata
    parameter RESIZE_OUTPUT_PACKET=0,   // 0 = Do not resize, packet length determined by s_axis_data_tlast, 1 = Use packet length from user header (s_axis_data_tuser)
    parameter WIDTH=32)                 // Specify the output width for the AXI stream data (can be 32 or 64)
   (input clk, input reset,
    input bus_clk, input bus_rst,

    input clear_tx_seqnum,
    input [15:0] next_dst,              // Used with SIMPLE_MODE=1

    // To NoC Shell
    input set_stb, input [7:0] set_addr, input [31:0] set_data,
    input [63:0] i_tdata, input i_tlast, input i_tvalid, output i_tready,
    output [63:0] o_tdata, output o_tlast, output o_tvalid, input o_tready,

    // To AXI IP
    output [WIDTH-1:0] m_axis_data_tdata, output [127:0] m_axis_data_tuser, output m_axis_data_tlast, output m_axis_data_tvalid, input m_axis_data_tready,
    input [WIDTH-1:0] s_axis_data_tdata, input [127:0] s_axis_data_tuser, input s_axis_data_tlast, input s_axis_data_tvalid, output s_axis_data_tready, 
    input [15:0] m_axis_pkt_len_tdata, input m_axis_pkt_len_tvalid, output m_axis_pkt_len_tready, // Used when RESIZE_INPUT_PACKET=1

    // Variable number of AXI configuration buses
    output [NUM_AXI_CONFIG_BUS*32-1:0] m_axis_config_tdata,
    output [NUM_AXI_CONFIG_BUS-1:0] m_axis_config_tlast,
    output [NUM_AXI_CONFIG_BUS-1:0] m_axis_config_tvalid,
    input [NUM_AXI_CONFIG_BUS-1:0] m_axis_config_tready
    );


   wire clear_tx_seqnum_bclk;
   pulse_synchronizer clear_tx_seqnum_sync_i (
     .clk_a(clk), .rst_a(reset), .pulse_a(clear_tx_seqnum), .busy_a(/*Ignored: Pulses from SW are slow*/),
     .clk_b(bus_clk), .pulse_b(clear_tx_seqnum_bclk)
   );

   // /////////////////////////////////////////////////////////
   // Input side handling, chdr_deframer
   wire [127:0] s_axis_data_tuser_int, m_axis_data_tuser_int;
   wire         s_axis_data_tlast_int, m_axis_data_tlast_int;
   reg [15:0]   m_axis_pkt_len_reg = 16'd8;
   reg          sof_in = 1'b1;
   wire [127:0] header_fifo_i_tdata  = {m_axis_data_tuser[127:96],m_axis_data_tuser[79:64],next_dst,m_axis_data_tuser[63:0]};
   wire         header_fifo_i_tvalid = sof_in & m_axis_data_tvalid & m_axis_data_tready;

   chdr_deframer_2clk #(.WIDTH(WIDTH)) chdr_deframer (
      .samp_clk(clk), .samp_rst(reset | clear_tx_seqnum), .pkt_clk(bus_clk), .pkt_rst(bus_rst | clear_tx_seqnum_bclk),
      .i_tdata(i_tdata), .i_tlast(i_tlast), .i_tvalid(i_tvalid), .i_tready(i_tready),
      .o_tdata(m_axis_data_tdata), .o_tuser(m_axis_data_tuser_int), .o_tlast(m_axis_data_tlast_int), .o_tvalid(m_axis_data_tvalid), .o_tready(m_axis_data_tready)
   );

   assign m_axis_data_tuser[127:80] = m_axis_data_tuser_int[127:80];
   assign m_axis_data_tuser[79:64]  = RESIZE_INPUT_PACKET ? (m_axis_data_tuser_int[125] ? m_axis_pkt_len_reg+16 : m_axis_pkt_len_reg+8) : m_axis_data_tuser_int[79:64];
   assign m_axis_data_tuser[63:0]   = m_axis_data_tuser_int[63:0];

   // Only store header once per packet
   always @(posedge clk)
     if(reset | clear_tx_seqnum)
       sof_in     <= 1'b1;
     else
       if(m_axis_data_tvalid & m_axis_data_tready)
         if(m_axis_data_tlast)
           sof_in <= 1'b1;
         else
           sof_in <= 1'b0;

   // SIMPLE MODE: Store input packet header to reuse as output packet header.
   generate
      if(SIMPLE_MODE)
         begin
            // FIFO 
            axi_fifo #(.WIDTH(128), .SIZE(5)) header_fifo
            (.clk(clk), .reset(reset), .clear(clear_tx_seqnum),
             .i_tdata(header_fifo_i_tdata),
             .i_tvalid(header_fifo_i_tvalid), .i_tready(),
             .o_tdata(s_axis_data_tuser_int), .o_tvalid(), .o_tready(s_axis_data_tlast_int & s_axis_data_tvalid & s_axis_data_tready),
             .occupied(), .space());
      end else begin
        assign s_axis_data_tuser_int = s_axis_data_tuser;
      end
   endgenerate

   // RESIZE INPUT PACKET
   // Size input packets based on m_axis_pkt_len_tdata (RESIZE_INPUT_PACKET=1) or based on i_tdata
   generate
     if (RESIZE_INPUT_PACKET) begin
       reg m_axis_data_tlast_reg;
       reg [15:0] m_axis_pkt_cnt;
       always @(posedge clk) begin
         if (reset | clear_tx_seqnum) begin
           m_axis_data_tlast_reg <= 1'b0;
           m_axis_pkt_cnt        <= (WIDTH/8);   // Number of bytes in packet
           m_axis_pkt_len_reg    <= 2*(WIDTH/8); // Double size by default
         end else begin
           // Only update packet length at the beginning of a new packet
           if (m_axis_pkt_len_tvalid & m_axis_pkt_len_tready) begin
             m_axis_pkt_len_reg <= m_axis_pkt_len_tdata;
           end
           if (m_axis_data_tvalid & m_axis_data_tready) begin
             if (m_axis_pkt_cnt >= m_axis_pkt_len_reg) begin
               m_axis_pkt_cnt        <= (WIDTH/8);
             end else begin
               m_axis_pkt_cnt        <= m_axis_pkt_cnt + (WIDTH/8);
             end
             if (m_axis_pkt_cnt >= m_axis_pkt_len_reg-(WIDTH/8)) begin
               m_axis_data_tlast_reg <= 1'b1;
             end else begin
               m_axis_data_tlast_reg <= 1'b0;
             end
           end
         end
       end
       assign m_axis_data_tlast     = m_axis_data_tlast_reg;
       assign m_axis_pkt_len_tready = sof_in;
     end else begin
       assign m_axis_data_tlast     = m_axis_data_tlast_int;
       assign m_axis_pkt_len_tready = 1'b0;
     end
   endgenerate

   // RESIZE OUTPUT PACKET
   // Size output packets based on either s_axis_data_tlast (RESIZE_OUTPUT_PACKETS=1) or packet length from user header (s_axis_data_tuser)
   // TODO: There could be a race condition on s_axis_data_tuser_int when
   //       receiving very short packets, but latency in chdr_deframer
   //       prevents this from occurring. Need to fix so it cannot
   //       occur by design.
   generate
     if (RESIZE_OUTPUT_PACKET) begin
       reg [15:0] s_axis_pkt_cnt;
       reg [15:0] s_axis_pkt_len;
       always @(posedge clk) begin
         if (reset | clear_tx_seqnum) begin
           s_axis_pkt_cnt        <= (WIDTH/8);
           s_axis_pkt_len        <= 0;
         end else begin
           // Remove header
           s_axis_pkt_len <= s_axis_data_tuser_int[125] ? s_axis_data_tuser_int[111:96]-16 : s_axis_data_tuser_int[111:96]-8;
           if (s_axis_data_tvalid & s_axis_data_tready) begin
             if ((s_axis_pkt_cnt >= s_axis_pkt_len) | s_axis_data_tlast) begin
               s_axis_pkt_cnt        <= (WIDTH/8);
             end else begin
               s_axis_pkt_cnt        <= s_axis_pkt_cnt + (WIDTH/8);
             end
           end
         end
       end
       assign s_axis_data_tlast_int = (s_axis_pkt_cnt >= s_axis_pkt_len) | s_axis_data_tlast;
     end else begin
       // chdr_framer will automatically fill in the packet length based on user provided tlast
       assign s_axis_data_tlast_int = s_axis_data_tlast;
     end
   endgenerate

   // /////////////////////////////////////////////////////////
   // Output side handling, chdr_framer
   chdr_framer_2clk #(.SIZE(MTU), .WIDTH(WIDTH), .USE_SEQ_NUM(USE_SEQ_NUM)) chdr_framer (
      .samp_clk(clk), .samp_rst(reset | clear_tx_seqnum), .pkt_clk(bus_clk), .pkt_rst(bus_rst | clear_tx_seqnum_bclk),
      .i_tdata(s_axis_data_tdata), .i_tuser(s_axis_data_tuser_int), .i_tlast(s_axis_data_tlast_int), .i_tvalid(s_axis_data_tvalid), .i_tready(s_axis_data_tready),
      .o_tdata(o_tdata), .o_tlast(o_tlast), .o_tvalid(o_tvalid), .o_tready(o_tready)
   );

   // /////////////////////////////////////////////////////////
   // Control bus handling
   // FIXME we could put inline control here...
   // Generate additional AXI stream interfaces for configuration. 
   // FIXME need to make sure we don't overrun this if core can backpressure us
   // Write to SR_AXI_CONFIG_BASE+1+2*(CONFIG BUS #) asserts tvalid, SR_AXI_CONFIG_BASE+1+2*(CONFIG BUS #)+1 asserts tvalid & tlast
   genvar k;
   generate
      for (k = 0; k < NUM_AXI_CONFIG_BUS; k = k + 1) begin
         axi_fifo #(.WIDTH(33), .SIZE(CONFIG_BUS_FIFO_DEPTH)) config_stream
           (.clk(clk), .reset(reset), .clear(clear_tx_seqnum),
            .i_tdata({(set_addr == (SR_AXI_CONFIG_BASE+2*k+1)),set_data}),
            .i_tvalid(set_stb & ((set_addr == (SR_AXI_CONFIG_BASE+2*k))|(set_addr == (SR_AXI_CONFIG_BASE+2*k+1)))),
            .i_tready(),
            .o_tdata({m_axis_config_tlast[k],m_axis_config_tdata[32*k+31:32*k]}),
            .o_tvalid(m_axis_config_tvalid[k]),
            .o_tready(m_axis_config_tready[k]),
            .occupied(), .space());
      end
   endgenerate
   
endmodule // axi_wrapper

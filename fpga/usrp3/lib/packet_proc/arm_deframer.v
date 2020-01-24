/////////////////////////////////////////////////////////////////////
//
// Copyright 2017 Ettus Research, A National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: arm_deframer
// Description:
//   Adds 6 bytes of Zeros at the beginning of every packet. It aligns the
//   64-bit words of the ethernet packet to be used later for classifying the
//   packets. The module is based on xge64_to_axi64 and has lesser
//   functionality.
//   Note that the block only works for padding 6 bytes.
//
/////////////////////////////////////////////////////////////////////

module arm_deframer (
  // Clocks and Reset
  input wire clk,
  input wire reset,
  input wire clear,
  // Slave AXI Interface
  input wire [63:0] s_axis_tdata,
  input wire [3:0]  s_axis_tuser, //used as tkeep here
  input wire        s_axis_tlast,
  input wire        s_axis_tvalid,
  output reg        s_axis_tready,
  // Master AXI Interface
  output reg [63:0] m_axis_tdata,
  output reg [3:0]  m_axis_tuser, //used as tkeep here
  output reg        m_axis_tlast,
  output reg        m_axis_tvalid,
  input wire        m_axis_tready
);
  // State Machine States
  localparam START = 2'b00;
  localparam BODY = 2'b01;
  localparam EXTRA = 2'b10;
  localparam PAD_BYTES = 3'b110; //6 bytes
  wire       new_line;
  wire       valid_beat;
  reg [1:0]  state = 2'b00, next_state=2'b00;
  reg [47:0] holding_reg;
  reg [2:0]  holding_user;
  // New line will be created by padding 6 bytes if the valid bytes on the
  // last line are greater than 2 bytes(3 to 7 bytes) or all 8 bytes are valid.
  assign new_line = (s_axis_tuser[2:0] > 3'b010) || (s_axis_tuser[2:0] == 3'b000);
  assign valid_beat = s_axis_tvalid & m_axis_tready;
  always @(posedge clk) begin
    if (reset | clear) begin
      state <= START;
    end else begin
      state <=next_state;
    end
  end
  // holding last 48 bits from input tdata on every valid_beat.
  always @(posedge clk)begin
    if (s_axis_tvalid & s_axis_tready) begin
      // Register the last 6 bytes of data for one cycle
      holding_reg  <= s_axis_tdata[63:16];
      // Register the tuser in case there is a new line
      // tuser should be valid for one extra cycle in that case
      holding_user <= s_axis_tuser[2:0];
    end
  end
  // Outputs
  always @(*) begin
    m_axis_tdata  = 64'b0;
    m_axis_tvalid = 1'b0;
    m_axis_tlast  = 1'b0;
    m_axis_tuser  = 4'b0;
    s_axis_tready = 1'b1;
    case (state)
      START : begin
        // Pad with 6 bytes of Zeros at the beginning of the packet
        // Shift the first 2 bytes to the end
        m_axis_tdata  = {s_axis_tdata[15:0], 48'b0};
        m_axis_tvalid = s_axis_tvalid;
        m_axis_tlast  = s_axis_tlast;
        m_axis_tuser  = 4'b0;
        s_axis_tready = m_axis_tready;
        if(valid_beat) next_state = BODY;
        else next_state = START;
      end
      BODY : begin
        // Shift the remaining packet by 6 bytes.
        // Here we're using register version of data and tvalid.
        m_axis_tdata  = {s_axis_tdata[15:0], holding_reg};
        m_axis_tvalid = s_axis_tvalid;
        m_axis_tlast  = new_line? 1'b0: s_axis_tvalid & s_axis_tlast;
        // Modify the tuser according to the new packet i.e. add 6 to it.
        m_axis_tuser  = (new_line & s_axis_tlast) ? 4'b0: {1'b0, s_axis_tuser[2:0] + PAD_BYTES};
        s_axis_tready = m_axis_tready;
        if (valid_beat & s_axis_tlast) next_state  =  new_line ? EXTRA : START;
        else next_state = BODY;
      end
      EXTRA : begin
        m_axis_tdata  = {16'b0, holding_reg};
        m_axis_tvalid = 1'b1;
        m_axis_tlast  = 1'b1;
        // Modify the tuser according to the new shifted packet i.e. add 6 to it.
        m_axis_tuser  = {1'b0, holding_user + PAD_BYTES};
        // We need to hold off any comming upstream data i.e not ready until
        // downstream done consuming this data
        s_axis_tready = 1'b0;
        if (m_axis_tready) next_state = START;
        else next_state = EXTRA;
      end
      default : begin
        m_axis_tdata  = 64'b0;
        m_axis_tvalid = 1'b0;
        m_axis_tlast  = 1'b0;
        m_axis_tuser  = 4'b0;
        s_axis_tready = 1'b1;
        next_state = START;
      end
    endcase
  end
endmodule // arm_deframer

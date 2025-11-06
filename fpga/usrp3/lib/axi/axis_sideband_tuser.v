//
// Copyright 2023 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: axis_sideband_tuser
//
// Description:
//
//   AXI stream buses often lack a pass-through tuser signal.
//   This block uses a FIFO to implement that functionality.
//   The FIFO can store a tuser entry for every bus transaction or
//   it can operate in packet mode where the tuser signal is assumed
//   to be constant during an entire packet to save resources.
//
//   NOTE: For buses / modules that consume a vector of samples (i.e. FFT block),
//         the FIFO size must be deep enough for the entire vector or a deadlock
//         can occur.
//
// Parameters:
//
//   WIDTH                : Width to tuser signal
//   FIFO_SIZE_LOG2       : Log2 size of FIFO
//   PACKET_MODE          : 0: Store tuser on every bus transaction
//                          1: Replicate tuser from first word in a packet
//                          2: Replicate tuser from last word in a packet, useful when tuser is EOB
//

`default_nettype none

module axis_sideband_tuser
#(
  parameter WIDTH                 = 32,
  parameter USER_WIDTH            = 1,
  parameter FIFO_SIZE_LOG2        = 5,
  parameter PACKET_MODE           = 1
)(
  // Clock/Reset
  input  wire                  clk,
  input  wire                  reset,

  // Toplevel AXI stream buses with tuser
  input  wire [WIDTH-1:0]      s_axis_tdata,
  input  wire [USER_WIDTH-1:0] s_axis_tuser,
  input  wire                  s_axis_tlast,
  input  wire                  s_axis_tvalid,
  output wire                  s_axis_tready,
  output wire [WIDTH-1:0]      m_axis_tdata,
  output wire [USER_WIDTH-1:0] m_axis_tuser,
  output wire                  m_axis_tlast,
  output wire                  m_axis_tvalid,
  input  wire                  m_axis_tready,

  // AXI stream buses to module / bus without tuser
  output wire [WIDTH-1:0]      m_axis_mod_tdata,
  output wire                  m_axis_mod_tlast,
  output wire                  m_axis_mod_tvalid,
  input  wire                  m_axis_mod_tready,
  input  wire [WIDTH-1:0]      s_axis_mod_tdata,
  input  wire                  s_axis_mod_tlast,
  input  wire                  s_axis_mod_tvalid,
  output wire                  s_axis_mod_tready
);

  wire [USER_WIDTH-1:0] fifo_in_tdata;
  wire                  fifo_in_tvalid;
  wire                  fifo_in_tready;
  wire [USER_WIDTH-1:0] fifo_out_tdata;
  wire                  fifo_out_tvalid;
  wire                  fifo_out_tready;

  axi_fifo #(
    .WIDTH    (USER_WIDTH),
    .SIZE     (FIFO_SIZE_LOG2))
  axi_fifo_tuser (
    .clk      (clk),
    .reset    (reset),
    .clear    (1'b0),
    .i_tdata  (fifo_in_tdata),
    .i_tvalid (fifo_in_tvalid),
    .i_tready (fifo_in_tready),
    .o_tdata  (fifo_out_tdata),
    .o_tvalid (fifo_out_tvalid),
    .o_tready (fifo_out_tready),
    .space    (),
    .occupied ()
  );

  reg first_word = 1'b1;
  always @(posedge clk) begin
    if (s_axis_tvalid & s_axis_tready) begin
      first_word <= s_axis_tlast;
    end
    if (reset) begin
      first_word <= 1'b1;
    end
  end

  assign fifo_in_tdata      = s_axis_tuser;
  assign fifo_in_tvalid     = m_axis_mod_tready & s_axis_tvalid &
                              (PACKET_MODE == 0 ? 1'b1         :
                               PACKET_MODE == 1 ? first_word   :
                               PACKET_MODE == 2 ? s_axis_tlast :
                               /* else */         1'b1);
  assign fifo_out_tready    = m_axis_tready & s_axis_mod_tvalid &
                              (PACKET_MODE == 0 ? 1'b1             :
                               PACKET_MODE == 1 ? s_axis_mod_tlast :
                               PACKET_MODE == 2 ? s_axis_mod_tlast :
                               /* else */         1'b1);

  assign s_axis_tready     = fifo_in_tready & m_axis_mod_tready;
  assign m_axis_mod_tdata  = s_axis_tdata;
  assign m_axis_mod_tlast  = s_axis_tlast;
  assign m_axis_mod_tvalid = fifo_in_tready & s_axis_tvalid;

  assign s_axis_mod_tready = m_axis_tready & fifo_out_tvalid;
  assign m_axis_tdata      = s_axis_mod_tdata;
  assign m_axis_tuser      = fifo_out_tdata;
  assign m_axis_tlast      = s_axis_mod_tlast;
  assign m_axis_tvalid     = s_axis_mod_tvalid & fifo_out_tvalid;

endmodule

`default_nettype wire

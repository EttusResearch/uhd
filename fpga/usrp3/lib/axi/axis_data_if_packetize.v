//
// Copyright 2024 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: axis_data_if_packetize
//
// Description:
//
//   ** Note: This block only supports the "axis_data" noc shell interface
//            with "sideband_at_end" enabled! **
//
//   Packetizes AXI stream bus from user based on packet length from
//   noc shell or a provided SPP. Handles timestamp and passes EOB and EOV to
//   the user logic.
//
//   This block is useful when the user logic cannot simply reuse the sideband
//   header data directly from noc shell. For example, this happens when samples
//   are added or removed which requires the user to manually break their stream
//   into packets, i.e. set tlast so their packets sent to noc shell do not
//   exceed samples per packet (SPP).
//
//   Packet length is based on the first packet in a burst unless the user sets
//   SPP to a non-zero value. Packet length is set at the beginning of a burst,
//   i.e. changes to SPP will not take effect until the burst is completed.
//
//   The timestamp on the first packet in a burst is passed (if applicable) and
//   subsequent timestamps are ignored.
//
//   EOB is passed to the user on m_axis_user_teob. The user should mark the last
//   sample in the burst by keeping track of EOB as it propagates through their
//   user logic and then asserting s_axis_user_teob.
//
// Parameters:
//
//   NIPC                        : Number of items/samples per clock cycle
//   ITEM_W                      : Width of each item/sample
//   SIDEBAND_FWD_FIFO_SIZE_LOG2 : Log2 size of FIFO used to forward the length,
//                                 timestamp, and has_time sideband signals.
//                                 The FIFO size may need to be increased when
//                                 receiving many small bursts, otherwise the
//                                 block will throttle the input when the FIFO
//                                 fills up. 0 is not recommended!
//

`default_nettype none


module axis_data_if_packetize #(
  parameter NIPC                        = 1,
  parameter ITEM_W                      = 32,
  parameter SIDEBAND_FWD_FIFO_SIZE_LOG2 = 5,

  localparam DATA_W = NIPC*ITEM_W,
  localparam KEEP_W = NIPC
) (
  // Clock/Reset
  input  wire              clk,
  input  wire              reset,

  // Samples per packet, ignored if set to zero
  input  wire [11:0]       spp,

  // From/to noc shell
  input  wire [DATA_W-1:0] s_axis_tdata,
  input  wire [KEEP_W-1:0] s_axis_tkeep,
  input  wire              s_axis_tlast,
  input  wire              s_axis_tvalid,
  output wire              s_axis_tready,
  input  wire [63:0]       s_axis_ttimestamp,
  input  wire              s_axis_thas_time,
  input  wire [15:0]       s_axis_tlength,
  input  wire              s_axis_teov,
  input  wire              s_axis_teob,
  output wire [DATA_W-1:0] m_axis_tdata,
  output wire [KEEP_W-1:0] m_axis_tkeep,
  output wire              m_axis_tlast,
  output wire              m_axis_tvalid,
  input  wire              m_axis_tready,
  output wire [63:0]       m_axis_ttimestamp,
  output wire              m_axis_thas_time,
  output wire [15:0]       m_axis_tlength,
  output wire              m_axis_teov,
  output wire              m_axis_teob,

  // User facing AXI stream buses
  output wire [DATA_W-1:0] m_axis_user_tdata,
  output wire [KEEP_W-1:0] m_axis_user_tkeep,
  output wire              m_axis_user_teob,
  output wire              m_axis_user_teov,
  output wire              m_axis_user_tlast,
  output wire              m_axis_user_tvalid,
  input  wire              m_axis_user_tready,
  input  wire [DATA_W-1:0] s_axis_user_tdata,
  input  wire [KEEP_W-1:0] s_axis_user_tkeep,
  input  wire              s_axis_user_teob,
  input  wire              s_axis_user_teov,
  input  wire              s_axis_user_tlast,
  input  wire              s_axis_user_tvalid,
  output wire              s_axis_user_tready
);

  //
  // Input State Machine
  //

  wire [64+1+16-1:0] sideband_fwd_fifo_in_tdata;
  wire               sideband_fwd_fifo_in_tvalid;
  wire               sideband_fwd_fifo_in_tready;
  wire [64+1+16-1:0] sideband_fwd_fifo_out_tdata;
  wire               sideband_fwd_fifo_out_tvalid;
  wire               sideband_fwd_fifo_out_tready;

  localparam S_IN_FWD_SIDEBAND = 1'd0;
  localparam S_IN_WAIT_FOR_EOB = 1'd1;
  reg in_state = S_IN_FWD_SIDEBAND;

  always @(posedge clk) begin
    case (in_state)
      // Start of burst. Wait for the first word and forward the
      // sideband signals to the output state machine
      S_IN_FWD_SIDEBAND : begin
        if (s_axis_tvalid & sideband_fwd_fifo_in_tready) begin
          in_state <= S_IN_WAIT_FOR_EOB;
        end
      end
      // Wait for end of burst
      S_IN_WAIT_FOR_EOB : begin
        if (s_axis_tvalid & s_axis_tready & s_axis_tlast & s_axis_teob) begin
          in_state <= S_IN_FWD_SIDEBAND;
        end
      end
      default: in_state <= S_IN_FWD_SIDEBAND;
    endcase
    if (reset) begin
      in_state  <= S_IN_FWD_SIDEBAND;
    end
  end

  assign m_axis_user_tdata     = s_axis_tdata;
  assign m_axis_user_tkeep     = s_axis_tkeep;
  assign m_axis_user_tlast     = s_axis_tlast;
  assign m_axis_user_teob      = s_axis_teob;
  assign m_axis_user_teov      = s_axis_teov;
  assign m_axis_user_tvalid    = s_axis_tvalid &
    ((in_state == S_IN_FWD_SIDEBAND) ? sideband_fwd_fifo_in_tready : 1'b1);
  assign s_axis_tready         = m_axis_user_tready &
    ((in_state == S_IN_FWD_SIDEBAND) ? sideband_fwd_fifo_in_tready : 1'b1);

  assign sideband_fwd_fifo_in_tdata  = {s_axis_thas_time, s_axis_ttimestamp, s_axis_tlength};
  assign sideband_fwd_fifo_in_tvalid = (in_state == S_IN_FWD_SIDEBAND) ? s_axis_tvalid : 1'b0;

  axi_fifo #(
    .WIDTH    (64+1+16),
    .SIZE     (SIDEBAND_FWD_FIFO_SIZE_LOG2))
  axi_fifo_sideband_fwd (
    .clk      (clk),
    .reset    (reset),
    .clear    (1'b0),
    .i_tdata  (sideband_fwd_fifo_in_tdata),
    .i_tvalid (sideband_fwd_fifo_in_tvalid),
    .i_tready (sideband_fwd_fifo_in_tready),
    .o_tdata  (sideband_fwd_fifo_out_tdata),
    .o_tvalid (sideband_fwd_fifo_out_tvalid),
    .o_tready (sideband_fwd_fifo_out_tready),
    .space    (),
    .occupied ()
  );

  //
  // Output State Machine
  //

  localparam S_OUT_WAIT_FOR_FWD_FIFO = 1'd0;
  localparam S_OUT_PACKETIZE         = 1'd1;
  reg out_state = S_OUT_WAIT_FOR_FWD_FIFO;

  wire        fifo_out_thas_time;
  wire [63:0] fifo_out_ttimestamp;
  wire [15:0] fifo_out_tlength;
  reg  [15:0] out_pkt_cnt          = 'd0;
  reg  [15:0] out_pkt_size         = 'd0;
  reg         set_has_time         = 1'b0;

  assign {fifo_out_thas_time, fifo_out_ttimestamp, fifo_out_tlength} = sideband_fwd_fifo_out_tdata;
  assign sideband_fwd_fifo_out_tready = m_axis_tvalid & m_axis_tready & m_axis_tlast & m_axis_teob;

  always @(posedge clk) begin
    case (out_state)
      // Wait for the input state machine to provide the sideband data for
      // this burst.
      S_OUT_WAIT_FOR_FWD_FIFO : begin
        set_has_time <= fifo_out_thas_time;
        out_pkt_size <= (spp > 0) ? (DATA_W/8)*spp : fifo_out_tlength;
        out_pkt_cnt  <= (DATA_W/8);
        if (sideband_fwd_fifo_out_tvalid) begin
          out_state <= S_OUT_PACKETIZE;
        end
      end
      S_OUT_PACKETIZE : begin
        if (s_axis_user_tvalid & s_axis_user_tready) begin
          out_pkt_cnt <= out_pkt_cnt + (DATA_W/8);
          if ((out_pkt_cnt >= out_pkt_size) || s_axis_user_tlast) begin
            set_has_time <= 1'b0;
            out_pkt_cnt  <= (DATA_W/8);
            if (s_axis_user_teob && s_axis_user_tlast) begin
              out_state  <= S_OUT_WAIT_FOR_FWD_FIFO;
            end
          end
        end
      end
      default: out_state <= S_OUT_WAIT_FOR_FWD_FIFO;
    endcase
    if (reset) begin
      out_state <= S_OUT_WAIT_FOR_FWD_FIFO;
    end
  end

  assign m_axis_tdata       = s_axis_user_tdata;
  assign m_axis_tkeep       = s_axis_user_tkeep;
  assign m_axis_tlast       = (out_pkt_cnt >= out_pkt_size) || s_axis_user_tlast;
  assign m_axis_teob        = s_axis_user_teob & s_axis_user_tlast;
  assign m_axis_teov        = s_axis_user_teov & s_axis_user_tlast;
  assign m_axis_thas_time   = set_has_time;
  assign m_axis_ttimestamp  = fifo_out_ttimestamp;
  assign m_axis_tlength     = out_pkt_cnt; // Only valid on tlast
  assign m_axis_tvalid      = (out_state == S_OUT_PACKETIZE) ? s_axis_user_tvalid : 1'b0;
  assign s_axis_user_tready = (out_state == S_OUT_PACKETIZE) ? m_axis_tready      : 1'b0;

endmodule


`default_nettype wire

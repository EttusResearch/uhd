//
// Copyright 2021 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: timekeeper
//
// Description:
//
//   Timekeeper for RFNoC blocks. This block contains a 64-bit counter to
//   represent the current time in terms of sample clock cycles. The counter
//   can be updated and synchronized using the pps input.
//
//   WARNING: All register larger than a single 32-bit word should be read and
//            written least significant word first to guarantee coherency.
//
// Parameters:
//
//   BASE_ADDR      : Base address for the internal CtrlPort registers.
//   TIME_INCREMENT : Amount by which to increment tb_timestamp for each radio
//                    strobe. When 0, the time_increment input is used instead.
//
// Signals:
//
//   tb_clk                : Time-base clock
//   tb_rst                : Time-base reset in tb_clk domain
//   s_ctrlport_clk        : Clock for CtrlPort bus
//   s_ctrlport_*          : CtrlPort bus for register access
//   time_increment        : Amount by which to increment timestamp. This is
//                           only used if TIME_INCREMENT parameter is 0.
//   sample_rx_stb         : Sample Rx strobe (data valid indicator).
//   pps                   : Pulse-per-second input
//   tb_timestamp          : 64-bit global timestamp synchronous to tb_clk
//   tb_timestamp_last_pps : 64-bit timestamp of the last PPS edge
//   tb_period_ns_q32      : Time Period of time-base in nanoseconds
//

module timekeeper #(
  parameter BASE_ADDR      = 'h00,
  parameter TIME_INCREMENT = 1
) (
  input wire         tb_clk,
  input wire         tb_rst,

  //---------------------------------------------------------------------------
  // Control Interface
  //---------------------------------------------------------------------------

  input  wire        s_ctrlport_clk,
  input  wire        s_ctrlport_req_wr,
  input  wire        s_ctrlport_req_rd,
  input  wire [19:0] s_ctrlport_req_addr,
  input  wire [31:0] s_ctrlport_req_data,
  output wire        s_ctrlport_resp_ack,
  output wire [31:0] s_ctrlport_resp_data,

  //---------------------------------------------------------------------------
  // Time (tb_clk domain)
  //---------------------------------------------------------------------------

  input wire [ 7:0]  time_increment,
  input wire         sample_rx_stb,
  input wire         pps,
  output reg [63:0]  tb_timestamp,
  output reg [63:0]  tb_timestamp_last_pps,
  output reg [63:0]  tb_period_ns_q32
);

  //---------------------------------------------------------------------------
  // Register Logic
  //---------------------------------------------------------------------------

  reg        set_time_pps;
  reg        set_time_now;
  reg        new_time_ctrl;
  reg [63:0] time_at_next_event;       // Time to load at next timed event

  reg [31:0] tb_timestamp_hi;          // Holding register for reading tb_timestamp
  reg [31:0] time_at_next_event_lo;    // Holding register for writing time_at_next_event
  reg [31:0] time_at_next_event_hi;    // Holding register for reading time_at_next_event
  reg [31:0] tb_timestamp_last_pps_hi; // Holding register for reading tb_timestamp_last_pps

  wire        s_ctrlport_req_wr_tb;
  wire        s_ctrlport_req_rd_tb;
  wire [19:0] s_ctrlport_req_addr_tb;
  wire [31:0] s_ctrlport_req_data_tb;
  reg         s_ctrlport_resp_ack_tb;
  reg  [31:0] s_ctrlport_resp_data_tb;

  // Clock crossing from ctrlport_clk to tb_clk domain

  ctrlport_clk_cross ctrlport_clk_cross_tb_i (
    .rst                       (tb_rst),
    .s_ctrlport_clk            (s_ctrlport_clk),
    .s_ctrlport_req_wr         (s_ctrlport_req_wr),
    .s_ctrlport_req_rd         (s_ctrlport_req_rd),
    .s_ctrlport_req_addr       (s_ctrlport_req_addr),
    .s_ctrlport_req_portid     (),
    .s_ctrlport_req_rem_epid   (),
    .s_ctrlport_req_rem_portid (),
    .s_ctrlport_req_data       (s_ctrlport_req_data),
    .s_ctrlport_req_byte_en    (),
    .s_ctrlport_req_has_time   (),
    .s_ctrlport_req_time       (),
    .s_ctrlport_resp_ack       (s_ctrlport_resp_ack),
    .s_ctrlport_resp_status    (),
    .s_ctrlport_resp_data      (s_ctrlport_resp_data),
    .m_ctrlport_clk            (tb_clk),
    .m_ctrlport_req_wr         (s_ctrlport_req_wr_tb),
    .m_ctrlport_req_rd         (s_ctrlport_req_rd_tb),
    .m_ctrlport_req_addr       (s_ctrlport_req_addr_tb),
    .m_ctrlport_req_portid     (),
    .m_ctrlport_req_rem_epid   (),
    .m_ctrlport_req_rem_portid (),
    .m_ctrlport_req_data       (s_ctrlport_req_data_tb),
    .m_ctrlport_req_byte_en    (),
    .m_ctrlport_req_has_time   (),
    .m_ctrlport_req_time       (),
    .m_ctrlport_resp_ack       (s_ctrlport_resp_ack_tb),
    .m_ctrlport_resp_status    (),
    .m_ctrlport_resp_data      (s_ctrlport_resp_data_tb)
  );

  //---------------------------------------------------------------------------
  // Timekeeper Register Offsets
  //---------------------------------------------------------------------------

  localparam REG_TIME_NOW_LO         = 'h00;  // Current time count (low word)
  localparam REG_TIME_NOW_HI         = 'h04;  // Current time count (high word)
  localparam REG_TIME_EVENT_LO       = 'h08;  // Time for next event (low word)
  localparam REG_TIME_EVENT_HI       = 'h0C;  // Time for next event (high word)
  localparam REG_TIME_CTRL           = 'h10;  // Time control word
  localparam REG_TIME_LAST_PPS_LO    = 'h14;  // Time of last PPS pulse edge (low word)
  localparam REG_TIME_LAST_PPS_HI    = 'h18;  // Time of last PPS pulse edge (high word)
  localparam REG_TIME_BASE_PERIOD_LO = 'h1C;  // Time Period in nanoseconds (low word)
  localparam REG_TIME_BASE_PERIOD_HI = 'h20;  // Time Period in nanoseconds (high word)

  // REG_TIME_CTRL bit fields
  localparam TIME_NOW_POS  = 0;
  localparam TIME_PPS_POS  = 1;

  always @(posedge tb_clk) begin
    if (tb_rst) begin
      s_ctrlport_resp_ack_tb  <= 0;
      s_ctrlport_resp_data_tb <= 0;
      new_time_ctrl           <= 0;
      set_time_pps            <= 0;
      set_time_now            <= 0;
    end else begin
      // Default assignments
      s_ctrlport_resp_ack_tb  <= 0;
      s_ctrlport_resp_data_tb <= 0;
      new_time_ctrl           <= 0;

      // Handle register writes
      if (s_ctrlport_req_wr_tb) begin
        case (s_ctrlport_req_addr_tb)
          BASE_ADDR + REG_TIME_EVENT_LO: begin
            time_at_next_event_lo    <= s_ctrlport_req_data_tb;
            s_ctrlport_resp_ack_tb   <= 1;
          end
          BASE_ADDR + REG_TIME_EVENT_HI: begin
            time_at_next_event[31: 0] <= time_at_next_event_lo;
            time_at_next_event[63:32] <= s_ctrlport_req_data_tb;
            s_ctrlport_resp_ack_tb    <= 1;
          end
          BASE_ADDR + REG_TIME_CTRL: begin
            set_time_pps            <= s_ctrlport_req_data_tb[TIME_PPS_POS];
            set_time_now            <= s_ctrlport_req_data_tb[TIME_NOW_POS];
            new_time_ctrl           <= 1;
            s_ctrlport_resp_ack_tb  <= 1;
          end
          BASE_ADDR + REG_TIME_BASE_PERIOD_LO: begin
            tb_period_ns_q32[31:0] <= s_ctrlport_req_data_tb;
            s_ctrlport_resp_ack_tb <= 1;
          end
          BASE_ADDR + REG_TIME_BASE_PERIOD_HI: begin
            tb_period_ns_q32[63:32] <= s_ctrlport_req_data_tb;
            s_ctrlport_resp_ack_tb  <= 1;
          end
        endcase
      end

      // Handle register reads
      if (s_ctrlport_req_rd_tb) begin
        case (s_ctrlport_req_addr_tb)
          BASE_ADDR + REG_TIME_NOW_LO: begin
            s_ctrlport_resp_data_tb <= tb_timestamp[31:0];
            tb_timestamp_hi         <= tb_timestamp[63:32];
            s_ctrlport_resp_ack_tb  <= 1;
          end
          BASE_ADDR + REG_TIME_NOW_HI: begin
            s_ctrlport_resp_data_tb <= tb_timestamp_hi;
            s_ctrlport_resp_ack_tb  <= 1;
          end
          BASE_ADDR + REG_TIME_EVENT_LO: begin
            s_ctrlport_resp_data_tb  <= time_at_next_event[31:0];
            time_at_next_event_hi    <= time_at_next_event[63:32];
            s_ctrlport_resp_ack_tb   <= 1;
          end
          BASE_ADDR + REG_TIME_EVENT_HI: begin
            s_ctrlport_resp_data_tb <= time_at_next_event_hi;
            s_ctrlport_resp_ack_tb  <= 1;
          end
          BASE_ADDR + REG_TIME_CTRL: begin
            s_ctrlport_resp_data_tb                <= 0;
            s_ctrlport_resp_data_tb[TIME_PPS_POS]  <= set_time_pps;
            s_ctrlport_resp_data_tb[TIME_NOW_POS]  <= set_time_now;
            s_ctrlport_resp_ack_tb                 <= 1;
          end
          BASE_ADDR + REG_TIME_LAST_PPS_LO: begin
            s_ctrlport_resp_data_tb     <= tb_timestamp_last_pps[31:0];
            tb_timestamp_last_pps_hi    <= tb_timestamp_last_pps[63:32];
            s_ctrlport_resp_ack_tb      <= 1;
          end
          BASE_ADDR + REG_TIME_LAST_PPS_HI: begin
            s_ctrlport_resp_data_tb <= tb_timestamp_last_pps_hi;
            s_ctrlport_resp_ack_tb  <= 1;
          end
          BASE_ADDR + REG_TIME_BASE_PERIOD_LO: begin
            s_ctrlport_resp_data_tb <= tb_period_ns_q32[31:0];
            s_ctrlport_resp_ack_tb  <= 1;
          end
          BASE_ADDR + REG_TIME_BASE_PERIOD_HI: begin
            s_ctrlport_resp_data_tb <= tb_period_ns_q32[63:32];
            s_ctrlport_resp_ack_tb  <= 1;
          end
        endcase
      end
    end
  end


  //---------------------------------------------------------------------------
  // Pulse Per Second
  //---------------------------------------------------------------------------

  reg pps_del;
  reg pps_edge;

  always @(posedge tb_clk) begin
    if (tb_rst) begin
      pps_del  <= 0;
      pps_edge <= 0;
    end else begin
      pps_del <= pps;
      pps_edge<= pps & ~pps_del;
    end
  end


  //---------------------------------------------------------------------------
  // Time Tracker
  //---------------------------------------------------------------------------

  // Amount by which to increment the timekeeper each clock cycle
  wire [31:0] increment = TIME_INCREMENT ? TIME_INCREMENT : time_increment;

  reg time_event_armed; // Boolean to indicate if we're expecting a timed event

  wire time_event =
    time_event_armed && (
      set_time_now || (set_time_pps && pps_edge)
    );

  always @(posedge tb_clk) begin
    if (tb_rst) begin
      tb_timestamp     <= 0;
      time_event_armed <= 0;
    end else begin
      if (time_event) begin
        // Load the timing info configured prior to the event
        time_event_armed <= 0;
        tb_timestamp     <= time_at_next_event;
      end else if (sample_rx_stb) begin
        // Update time for each sample word received
        tb_timestamp <= tb_timestamp + increment;
      end

      if (new_time_ctrl) begin
        // Indicate that we're expecting a timed event because the time control
        // register was updated.
        time_event_armed <= 1;
      end
    end
  end


  //---------------------------------------------------------------------------
  // PPS Tracker
  //---------------------------------------------------------------------------

  always @(posedge tb_clk) begin
    if (tb_rst) begin
      tb_timestamp_last_pps <= 64'h0;
    end else if (pps_edge) begin
      if (time_event) begin
        tb_timestamp_last_pps <= time_at_next_event;
      end else begin
        tb_timestamp_last_pps <= tb_timestamp + increment;
      end
    end
  end

endmodule

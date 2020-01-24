//
// Copyright 2018 Ettus Research, A National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: settings_bus_timed_2clk
// Description:
// - Stores settings bus transaction in a FIFO and 
//   releases them based on VITA time input
// - Also moves the settings bus to the timebase
//   clock domain
//

module settings_bus_timed_2clk #(
  parameter SR_AWIDTH     = 8,
  parameter SR_DWIDTH     = 32,
  parameter RB_AWIDTH     = 8,
  parameter RB_DWIDTH     = 64,
  parameter TIMED_CMDS_EN = 0
) (
  input                  sb_clk,          // Settings bus clock
  input                  sb_rst,          // Reset (sb_clk)
  input                  tb_clk,          // Timebase clock
  input                  tb_rst,          // Reset (tb_clk)

  input  [63:0]          vita_time,       // Current timebase time
                         
  input                  s_set_stb,       // Settings bus strobe
  input  [SR_AWIDTH-1:0] s_set_addr,      // Settings address
  input  [SR_DWIDTH-1:0] s_set_data,      // Settings data
  input                  s_set_has_time,  // Is this a timed command?
  input  [63:0]          s_set_time,      // Command time
  output                 s_set_pending,   // Is settings transaction pending?
  input  [RB_AWIDTH-1:0] s_rb_addr,       // Readback address
  output                 s_rb_stb,        // Readback data strobe
  output [RB_DWIDTH-1:0] s_rb_data,       // Readback data value

  output                 m_set_stb,       // Settings bus strobe
  output [SR_AWIDTH-1:0] m_set_addr,      // Settings address
  output [SR_DWIDTH-1:0] m_set_data,      // Settings data
  output                 m_set_has_time,  // Is this a timed command?
  output [63:0]          m_set_time,      // Command time
  input                  m_set_pending,   // Is settings transaction pending?
  output [RB_AWIDTH-1:0] m_rb_addr,       // Readback address
  input                  m_rb_stb,        // Readback data strobe
  input  [RB_DWIDTH-1:0] m_rb_data        // Readback data value
);

  // States for input and output state machines
  localparam [2:0] ST_IDLE        = 3'd0; // Nothing is happening on the bus
  localparam [2:0] ST_SET_ISSUED  = 3'd1; // A settings transaction has been issued
  localparam [2:0] ST_SET_PENDING = 3'd2; // A settings transaction is pending
  localparam [2:0] ST_RB_PENDING  = 3'd3; // Waiting for readback data
  localparam [2:0] ST_RB_DONE     = 3'd4; // Readback data is valid
  
  wire rb_valid;

  // Input state machine
  reg [2:0] in_state = ST_IDLE;
  always @(posedge sb_clk) begin
    if (sb_rst) begin
      in_state <= ST_IDLE;
    end else begin
      case (in_state)
        ST_IDLE: begin
          if (s_set_stb) begin
            in_state <= ST_SET_PENDING;
          end
        end
        ST_SET_PENDING: begin
          if (rb_valid) begin
            in_state <= ST_RB_DONE;
          end
        end
        ST_RB_DONE: begin
          in_state <= ST_IDLE;
        end
        default: begin
          in_state <= ST_IDLE;
        end
      endcase
    end
  end
  assign s_set_pending = (in_state == ST_SET_PENDING);
  assign s_rb_stb = (in_state == ST_RB_DONE);

  // Clock crossing FIFO (settings)
  // TODO: Look into a more efficient implementation for a single element
  //       clock crossing FIFO.
  wire set_pending, set_finished;
  axi_fifo_2clk #(
    .WIDTH(SR_AWIDTH+SR_DWIDTH+1+64+RB_AWIDTH), .SIZE(0)
  ) sb_2clk_fifo_i (
    .i_aclk(sb_clk), .reset(sb_rst),
    .i_tdata({s_set_addr, s_set_data, s_set_has_time, s_set_time, s_rb_addr}),
    .i_tvalid(s_set_stb), .i_tready(/* Ignored: FIFO may not have an exact size*/),
    .o_aclk(tb_clk),
    .o_tdata({m_set_addr, m_set_data, m_set_has_time, m_set_time, m_rb_addr}),
    .o_tvalid(set_pending), .o_tready(set_finished)
  );

  // Time compare logic
  // If ~has_time then pass the transaction through, otherwise wait for time
  // to tick up to command time
  wire now, late;
  wire go = ((TIMED_CMDS_EN == 1) && m_set_has_time) ? (now | late) : 1'b1;

  // If this is a timed command then vita_time == m_set_time one cycle before
  // strobe is asserted i.e. timed strobe assertion has a one cycle latency
  time_compare time_compare (
    .clk(tb_clk), .reset(tb_rst),
    .time_now(vita_time), .trigger_time(m_set_time),
    .now(now), .early(), .late(late), .too_early()
  );

  // Clock crossing FIFO (readback)
  reg [RB_DWIDTH-1:0] cached_rb_data;
  axi_fifo_2clk #(
    .WIDTH(RB_DWIDTH), .SIZE(0)
  ) rbdata_2clk_fifo_i (
    .reset(tb_rst),
    .i_aclk(tb_clk), .i_tdata(cached_rb_data), .i_tvalid(set_finished), .i_tready(),
    .o_aclk(sb_clk), .o_tdata(s_rb_data), .o_tvalid(rb_valid), .o_tready(s_rb_stb)
  );

  // Output state machine
  reg [2:0] out_state = ST_IDLE;
  always @(posedge tb_clk) begin
    if (tb_rst) begin
      out_state <= ST_IDLE;
    end else begin
      case (out_state)
        ST_IDLE: begin
          if (go & set_pending) begin
            out_state <= ST_SET_ISSUED;
          end
        end
        ST_SET_ISSUED: begin
          out_state <= ST_SET_PENDING;
        end
        ST_SET_PENDING: begin
          if (~m_set_pending) begin
            if (m_rb_stb) begin
              out_state <= ST_RB_DONE;
              cached_rb_data <= m_rb_data;
            end else begin
              out_state <= ST_RB_PENDING;
            end
          end
        end
        ST_RB_PENDING: begin
          if (m_rb_stb) begin
            out_state <= ST_RB_DONE;
            cached_rb_data <= m_rb_data;
          end
        end
        ST_RB_DONE: begin
          out_state <= ST_IDLE;
        end
        default: begin
          out_state <= ST_IDLE;
        end
      endcase
    end
  end

  assign m_set_stb = (out_state == ST_SET_ISSUED);
  assign set_finished = (out_state == ST_RB_DONE);

endmodule

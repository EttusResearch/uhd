//
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//

module noc_traffic_counter #(
  parameter SR_REG_BASE = 128,
  parameter RB_REG_BASE = 0)
(
  input bus_clk, input bus_rst,
  input ce_clk, input ce_rst,

  // Control sink
  input [31:0] set_data, input [7:0] set_addr, input set_stb,
  output rb_stb, input [7:0] rb_addr, output [63:0] rb_data,

  // Traffic signals to count
  input i_tlast, input i_tvalid, input i_tready,
  input o_tlast, input o_tvalid, input  o_tready,
  input str_sink_tlast, input str_sink_tvalid, input str_sink_tready,
  input str_src_tlast, input str_src_tvalid, input str_src_tready
);
  wire        en, counter_enable_changed;

  wire [31:0] set_data_bclk;
  wire [7:0]  set_addr_bclk;
  wire        set_stb_bclk;

  reg  [63:0] rb_data_bclk;
  wire [7:0]  rb_addr_bclk;
  reg         rb_stb_bclk;

  reg  [63:0] tick_cnt_noc_shell;

  wire [63:0] xbar_to_shell_xfer_cnt;
  wire [63:0] xbar_to_shell_pkt_cnt;

  wire [63:0] shell_to_xbar_xfer_cnt;
  wire [63:0] shell_to_xbar_pkt_cnt;

  wire [63:0] shell_to_ce_xfer_cnt;
  wire [63:0] shell_to_ce_pkt_cnt;

  wire [63:0] ce_to_shell_xfer_cnt;
  wire [63:0] ce_to_shell_pkt_cnt;

  localparam SR_COUNTER_ENABLE      = SR_REG_BASE + 0;

  localparam RB_SIGNATURE              = RB_REG_BASE + 0;
  localparam RB_BUS_CLK_TICKS          = RB_REG_BASE + 1;
  localparam RB_XBAR_TO_SHELL_XFER_CNT = RB_REG_BASE + 2;
  localparam RB_XBAR_TO_SHELL_PKT_CNT  = RB_REG_BASE + 3;
  localparam RB_SHELL_TO_XBAR_XFER_CNT = RB_REG_BASE + 4;
  localparam RB_SHELL_TO_XBAR_PKT_CNT  = RB_REG_BASE + 5;
  localparam RB_SHELL_TO_CE_XFER_CNT   = RB_REG_BASE + 6;
  localparam RB_SHELL_TO_CE_PKT_CNT    = RB_REG_BASE + 7;
  localparam RB_CE_TO_SHELL_XFER_CNT   = RB_REG_BASE + 8;
  localparam RB_CE_TO_SHELL_PKT_CNT    = RB_REG_BASE + 9;

  // Registers are implemented on bus clock
  axi_fifo_2clk #(.WIDTH(8+8+32), .SIZE(2)) reg_write_to_bclk (
     .reset(ce_rst), .i_aclk(ce_clk),
     .i_tdata({set_addr, rb_addr, set_data}), .i_tvalid(set_stb), .i_tready(),
     .o_aclk(bus_clk),
     .o_tdata({set_addr_bclk, rb_addr_bclk, set_data_bclk}), .o_tvalid(set_stb_bclk), .o_tready(1'b1));

  axi_fifo_2clk #(.WIDTH(64), .SIZE(2)) reg_rb_from_bclk (
     .reset(bus_rst), .i_aclk(bus_clk),
     .i_tdata(rb_data_bclk), .i_tvalid(rb_stb_bclk), .i_tready(),
     .o_aclk(ce_clk),
     .o_tdata(rb_data), .o_tvalid(rb_stb), .o_tready(1'b1));

  setting_reg #(.my_addr(SR_COUNTER_ENABLE), .width(1)) enable_measurement_reg (
    .clk(bus_clk), .rst(bus_rst), .strobe(set_stb_bclk), .addr(set_addr_bclk),
    .in(set_data_bclk), .out(en), .changed(counter_enable_changed));

  always @(posedge bus_clk)
    if (set_stb_bclk) begin
      case(rb_addr_bclk)
        RB_SIGNATURE             : rb_data_bclk <= 64'h712AFF1C00000000;
        RB_BUS_CLK_TICKS         : rb_data_bclk <= tick_cnt_noc_shell;
        RB_XBAR_TO_SHELL_XFER_CNT : rb_data_bclk <= xbar_to_shell_xfer_cnt;
        RB_XBAR_TO_SHELL_PKT_CNT  : rb_data_bclk <= xbar_to_shell_pkt_cnt;
        RB_SHELL_TO_XBAR_XFER_CNT : rb_data_bclk <= shell_to_xbar_xfer_cnt;
        RB_SHELL_TO_XBAR_PKT_CNT  : rb_data_bclk <= shell_to_xbar_pkt_cnt;
        RB_SHELL_TO_CE_XFER_CNT   : rb_data_bclk <= shell_to_ce_xfer_cnt;
        RB_SHELL_TO_CE_PKT_CNT    : rb_data_bclk <= shell_to_ce_pkt_cnt;
        RB_CE_TO_SHELL_XFER_CNT   : rb_data_bclk <= ce_to_shell_xfer_cnt;
        RB_CE_TO_SHELL_PKT_CNT    : rb_data_bclk <= ce_to_shell_pkt_cnt;
        default                   : rb_data_bclk <= 64'h0BADC0DE0BADC0DE;
      endcase
    end

  always @(posedge bus_clk)
    rb_stb_bclk <= set_stb_bclk;

  assign counter_rst = en & counter_enable_changed;

  axis_strm_monitor #(.COUNT_W(64), .PKT_COUNT_EN(1), .XFER_COUNT_EN(1)) xbar_to_shell (
    .clk(bus_clk), .reset(counter_rst),
    .axis_tdata(), .axis_tlast(i_tlast & en), .axis_tvalid(i_tvalid & en), .axis_tready(i_tready & en),
    .xfer_count(xbar_to_shell_xfer_cnt), .pkt_count(xbar_to_shell_pkt_cnt));

  axis_strm_monitor #(.COUNT_W(64), .PKT_COUNT_EN(1), .XFER_COUNT_EN(1)) shell_to_xbar (
    .clk(bus_clk), .reset(counter_rst),
    .axis_tdata(), .axis_tlast(o_tlast & en), .axis_tvalid(o_tvalid & en), .axis_tready(o_tready & en),
    .xfer_count(shell_to_xbar_xfer_cnt), .pkt_count(shell_to_xbar_pkt_cnt));

  axis_strm_monitor #(.COUNT_W(64), .PKT_COUNT_EN(1), .XFER_COUNT_EN(1)) shell_to_ce (
    .clk(bus_clk), .reset(counter_rst),
    .axis_tdata(), .axis_tlast(str_sink_tlast & en), .axis_tvalid(str_sink_tvalid & en), .axis_tready(str_sink_tready & en),
    .xfer_count(shell_to_ce_xfer_cnt), .pkt_count(shell_to_ce_pkt_cnt));

  axis_strm_monitor #(.COUNT_W(64), .PKT_COUNT_EN(1), .XFER_COUNT_EN(1)) ce_to_shell (
    .clk(bus_clk), .reset(counter_rst),
    .axis_tdata(), .axis_tlast(str_src_tlast & en), .axis_tvalid(str_src_tvalid & en), .axis_tready(str_src_tready & en),
    .xfer_count(ce_to_shell_xfer_cnt), .pkt_count(ce_to_shell_pkt_cnt));

  // Count clock ticks
  always @(posedge bus_clk)
    if (counter_rst)
      tick_cnt_noc_shell <= 0;
    else
      if (en)
        tick_cnt_noc_shell <= tick_cnt_noc_shell + 1;

endmodule

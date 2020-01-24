/////////////////////////////////////////////////////////////////////
//
// Copyright 2017 Ettus Research, A National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: axi_crossbar_regport
// Description:
//   - axi_crossbar with regport interface for register/CAM access
//
//////////////////////////////////////////////////////////////////////

module axi_crossbar_regport #(
  parameter REG_BASE    = 0,  // settings bus base address
  parameter FIFO_WIDTH  = 64, // AXI4-STREAM data bus width
  parameter DST_WIDTH   = 16, // Width of DST field we are routing on.
  parameter NUM_INPUTS  = 2,  // number of input AXI4-STREAM buses
  parameter NUM_OUTPUTS = 2,   // number of output AXI4-STREAM buses
  parameter REG_DWIDTH  = 32, // Width of the AXI4-Lite data bus (must be 32 or 64)
  parameter REG_AWIDTH  = 14  // Width of the address bus
)(
  input                                 clk,
  input                                 reset,
  input                                 clear,

  input                                 reg_wr_req,
  input  [REG_AWIDTH-1:0]               reg_wr_addr,
  input  [REG_DWIDTH-1:0]               reg_wr_data,

  input                                 reg_rd_req,
  input  [REG_AWIDTH-1:0]               reg_rd_addr,
  output [REG_DWIDTH-1:0]               reg_rd_data,
  output                                reg_rd_resp,

  // Inputs
  input [(FIFO_WIDTH*NUM_INPUTS)-1:0]   i_tdata,
  input [NUM_INPUTS-1:0]                i_tvalid,
  input [NUM_INPUTS-1:0]                i_tlast,
  output [NUM_INPUTS-1:0]               i_tready,
  input [NUM_INPUTS-1:0]                pkt_present,

  // Output
  output [(FIFO_WIDTH*NUM_OUTPUTS)-1:0] o_tdata,
  output [NUM_OUTPUTS-1:0]              o_tvalid,
  output [NUM_OUTPUTS-1:0]              o_tlast,
  input [NUM_OUTPUTS-1:0]               o_tready
);

  localparam XBAR_VERSION              = 32'b1;
  localparam XBAR_NUM_PORTS            = NUM_INPUTS; //or NUM_OUTPUTS

  localparam REG_XBAR_VERSION          = REG_BASE + 14'h10;
  localparam REG_XBAR_NUM_PORTS        = REG_BASE + 14'h14;
  localparam REG_XBAR_LOCAL_ADDR       = REG_BASE + 14'h18;
  localparam REG_BASE_XBAR_SETTING_REG = REG_BASE + 14'h20;
  localparam REG_END_ADDR_XBAR_SETTING_REG = REG_BASE + 14'h1000;

  // Settings bus address width
  localparam SR_AWIDTH = 12;

  wire                  xbar_set_stb;
  wire [REG_DWIDTH-1:0] xbar_set_data;
  wire [SR_AWIDTH-1:0]  xbar_set_addr;

  wire                  xbar_rb_stb;
  wire [SR_AWIDTH-1:0]  xbar_rb_addr;
  wire [REG_DWIDTH-1:0] xbar_rb_data;

  reg  [31:0]           local_addr_reg;
  reg                   reg_rd_resp_glob;
  reg  [REG_DWIDTH-1:0] reg_rd_data_glob;

  wire [REG_DWIDTH-1:0] reg_rd_data_xbar;
  wire                  reg_rd_resp_xbar;

  regport_resp_mux #(
    .WIDTH(REG_DWIDTH),
    .NUM_SLAVES(2)
  ) inst_regport_resp_mux_xbar (
    .clk(clk),
    .reset(reset),
    .sla_rd_resp({reg_rd_resp_glob, reg_rd_resp_xbar}),
    .sla_rd_data({reg_rd_data_glob, reg_rd_data_xbar}),
    .mst_rd_resp(reg_rd_resp),
    .mst_rd_data(reg_rd_data)
  );

  // Read Registers
  always @ (posedge clk) begin
    if (reset) begin
      local_addr_reg <= 32'h0;
    end
    else begin
    if (reg_wr_req)
      case (reg_wr_addr)
        REG_XBAR_LOCAL_ADDR:
          local_addr_reg  <= reg_wr_data;
      endcase
    end
  end

  // Write Registers
  always @ (posedge clk) begin
    if (reset)
      reg_rd_resp_glob <= 1'b0;

    else begin
      if (reg_rd_req) begin
        reg_rd_resp_glob <= 1'b1;

        case (reg_rd_addr)
        REG_XBAR_VERSION:
          reg_rd_data_glob <= XBAR_VERSION;

        REG_XBAR_NUM_PORTS:
          reg_rd_data_glob <= XBAR_NUM_PORTS;

        REG_XBAR_LOCAL_ADDR:
          reg_rd_data_glob <= local_addr_reg;

        default:
          reg_rd_resp_glob <= 1'b0;
        endcase
      end
      else if (reg_rd_resp_glob) begin
          reg_rd_resp_glob <= 1'b0;
      end
    end
  end

  regport_to_xbar_settingsbus #(
    .BASE(REG_BASE_XBAR_SETTING_REG),
    .END_ADDR(REG_END_ADDR_XBAR_SETTING_REG),
    .DWIDTH(REG_DWIDTH),
    .AWIDTH(REG_AWIDTH),
    .SR_AWIDTH(SR_AWIDTH),
    .ADDRESSING("WORD")
  ) inst_regport_to_xbar_settingsbus (
    .clk(clk),
    .reset(reset),

    .reg_wr_req(reg_wr_req),
    .reg_wr_addr(reg_wr_addr),
    .reg_wr_data(reg_wr_data),
    .reg_rd_req(reg_rd_req),
    .reg_rd_addr(reg_rd_addr),
    .reg_rd_data(reg_rd_data_xbar),
    .reg_rd_resp(reg_rd_resp_xbar),

    .set_stb(xbar_set_stb),
    .set_addr(xbar_set_addr),
    .set_data(xbar_set_data),
    .rb_stb(xbar_rb_stb),
    .rb_addr(xbar_rb_addr),
    .rb_data(xbar_rb_data)
  );

  axi_crossbar #(
    .BASE(0), // Set to 0 as logic for other values has not been tested
    .FIFO_WIDTH(FIFO_WIDTH),
    .DST_WIDTH(DST_WIDTH),
    .NUM_INPUTS(NUM_INPUTS),
    .NUM_OUTPUTS(NUM_OUTPUTS)
  ) axi_crossbar (
    .clk(clk),
    .reset(reset),
    .clear(1'b0),
    .local_addr(local_addr_reg),

    // settings bus for config
    .set_stb(xbar_set_stb),
    .set_addr({4'b0000,xbar_set_addr}),
    .set_data(xbar_set_data),
    .rb_rd_stb(xbar_rb_stb),
    .rb_addr(xbar_rb_addr[$clog2(NUM_INPUTS)+$clog2(NUM_OUTPUTS)-1:0]),
    .rb_data(xbar_rb_data),

    // inputs, real men flatten busses
    .i_tdata(i_tdata),
    .i_tlast(i_tlast),
    .i_tvalid(i_tvalid),
    .i_tready(i_tready),

    // outputs, real men flatten busses
    .o_tdata(o_tdata),
    .o_tlast(o_tlast),
    .o_tvalid(o_tvalid),
    .o_tready(o_tready),
    .pkt_present(pkt_present)
  );

endmodule // axi_crossbar_regport


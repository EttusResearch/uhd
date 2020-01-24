//
// Copyright 2016 Ettus Research
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// AXI4lite to NI Register Port interface
//

module axil_to_ni_regport #(
  parameter RP_AWIDTH = 16,
  parameter RP_DWIDTH = 32,
  parameter TIMEOUT   = 512
)(
  input                  s_axi_aclk,
  input                  s_axi_areset,

  // AXI4lite interface
  input [31:0]           s_axi_awaddr,
  input                  s_axi_awvalid,
  output                 s_axi_awready,
  input [31:0]           s_axi_wdata,
  input [3:0]            s_axi_wstrb,
  input                  s_axi_wvalid,
  output                 s_axi_wready,
  output [1:0]           s_axi_bresp,
  output                 s_axi_bvalid,
  input                  s_axi_bready,
  input [31:0]           s_axi_araddr,
  input                  s_axi_arvalid,
  output                 s_axi_arready,
  output [31:0]          s_axi_rdata,
  output [1:0]           s_axi_rresp,
  output                 s_axi_rvalid,
  input                  s_axi_rready,

  // RegPort interface, the out vs in
  // is seen from the slave device
  // hooked up to the regport
  output                 reg_port_in_rd,
  output                 reg_port_in_wt,
  output [RP_AWIDTH-1:0] reg_port_in_addr,
  output [RP_DWIDTH-1:0] reg_port_in_data,
  input  [RP_DWIDTH-1:0] reg_port_out_data,
  input                  reg_port_out_ready
);

  localparam IDLE              = 3'd0;
  localparam READ_INIT         = 3'd1;
  localparam WRITE_INIT        = 3'd2;
  localparam READ_IN_PROGRESS  = 3'd3;
  localparam WRITE_IN_PROGRESS = 3'd4;
  localparam WRITE_DONE        = 3'd5;
  localparam READ_DONE         = 3'd6;

  reg [RP_AWIDTH-1:0] addr;
  reg [RP_DWIDTH-1:0] rb_data;
  reg [RP_DWIDTH-1:0] wr_data;
  reg [2:0] state;
  reg [9:0] count;
  reg [1:0] rresp;
  reg [1:0] bresp;

  always @ (posedge s_axi_aclk) begin
    if (s_axi_areset) begin
      state   <= IDLE;
      addr    <= 'd0;
      rb_data <= 'd0;
      wr_data <= 'd0;

      count    <= 10'd0;
      rresp    <= 2'd0;
      bresp    <= 2'd0;
    end
    else case (state)

    IDLE: begin
      if (s_axi_arvalid) begin
        state <= READ_INIT;
        addr  <= s_axi_araddr[RP_AWIDTH-1:0];
      end
      else if (s_axi_awvalid) begin
        state <= WRITE_INIT;
        addr  <= s_axi_awaddr[RP_AWIDTH-1:0];
      end
    end

    READ_INIT: begin
      state <= READ_IN_PROGRESS;
      count <= 10'd0;
      rresp <= 2'b00;
    end

    READ_IN_PROGRESS: begin
      if (reg_port_out_ready) begin
        rb_data <= reg_port_out_data;
        state   <= READ_DONE;
      end
      else if (count >= TIMEOUT) begin
        state   <= READ_DONE;
        rresp   <= 2'b10;
      end
      else begin
        count   <= count + 1'b1;
      end
    end

    READ_DONE: begin
      if (s_axi_rready) begin
        state <= IDLE;
      end
    end

    WRITE_INIT: begin
      if (s_axi_wvalid) begin
        wr_data <= s_axi_wdata[RP_DWIDTH-1:0];
        state   <= WRITE_IN_PROGRESS;
        count <= 10'd0;
        bresp <= 2'b00;
      end
    end

    WRITE_IN_PROGRESS: begin
      if (reg_port_out_ready) begin
        state <= WRITE_DONE;
      end
      else if (count >= TIMEOUT) begin
        state   <= READ_DONE;
        bresp   <= 2'b10;
      end
      else begin
        count   <= count + 1'b1;
      end
    end

    WRITE_DONE: begin
      if (s_axi_bready)
        state <= IDLE;
    end

    default: begin
      state <= IDLE;
    end

    endcase
  end

  assign s_axi_awready = (state == IDLE);
  assign s_axi_wready  = (state == WRITE_INIT);
  assign s_axi_bvalid  = (state == WRITE_DONE);
  assign s_axi_bresp   = bresp;

  assign s_axi_arready = (state == IDLE);
  assign s_axi_rdata   = rb_data;
  assign s_axi_rvalid  = (state == READ_DONE);
  assign s_axi_rresp   = rresp;

  assign reg_port_in_wt   = (state == WRITE_INIT) & s_axi_wvalid;
  assign reg_port_in_data = (state == WRITE_INIT) ? s_axi_wdata : wr_data;
  assign reg_port_in_addr = addr;

  assign reg_port_in_rd   = (state == READ_INIT);

endmodule

//
// Copyright 2015 Ettus Research
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//

module axi_dummy
(
  // sys connect
  input         s_axi_aclk,
  input         s_axi_areset,

  // axi4 lite slave port
  input [31:0]  s_axi_awaddr,
  input         s_axi_awvalid,
  output        s_axi_awready,

  input [31:0]  s_axi_wdata,
  input [3:0]   s_axi_wstrb,
  input         s_axi_wvalid,
  output        s_axi_wready,

  output [1:0]  s_axi_bresp,
  output        s_axi_bvalid,
  input         s_axi_bready,

  input [31:0]  s_axi_araddr,
  input         s_axi_arvalid,
  output        s_axi_arready,

  output [31:0] s_axi_rdata,
  output [1:0]  s_axi_rresp,
  output        s_axi_rvalid,
  input         s_axi_rready
);
  parameter DEC_ERR = 1'b1;

  localparam IDLE              = 3'b001;
  localparam READ_IN_PROGRESS  = 3'b010;
  localparam WRITE_IN_PROGRESS = 3'b100;

  reg [2:0] state;

  always @ (posedge s_axi_aclk) begin
    if (s_axi_areset) begin
      state <= IDLE;
    end
    else case (state)

    IDLE: begin
      if (s_axi_arvalid)
        state <= READ_IN_PROGRESS;
      else if (s_axi_awvalid)
        state <= WRITE_IN_PROGRESS;
    end

    READ_IN_PROGRESS: begin
      if (s_axi_rready)
        state <= IDLE;
    end

    WRITE_IN_PROGRESS: begin
      if (s_axi_bready)
        state <= IDLE;
    end

    default: begin
      state <= IDLE;
    end

    endcase
  end

  assign s_axi_awready = (state == IDLE);
  assign s_axi_wready  = (state == WRITE_IN_PROGRESS);
  assign s_axi_bvalid  = (state == WRITE_IN_PROGRESS);

  assign s_axi_arready = (state == IDLE);
  assign s_axi_rdata = 32'hdead_ba5e;
  assign s_axi_rvalid  = (state == READ_IN_PROGRESS);
  assign s_axi_rresp = DEC_ERR ? 2'b11 : 2'b00;
  assign s_axi_bresp = DEC_ERR ? 2'b11 : 2'b00;

endmodule

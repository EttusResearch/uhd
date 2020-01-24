//
// Copyright 2015 Ettus Research, A National Instruments Company
// SPDX-License-Identifier: LGPL-3.0
//
// Description: AXI PMU
//

module axi_pmu
#(
  parameter DEPTH = 64
)
(
  // sys connect
  input         s_axi_aclk,
  input         s_axi_areset,

  // spi slave port
  input         ss,
  input         mosi,
  input         sck,
  output        miso,

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
  input         s_axi_rready,

  output        s_axi_irq
);

  wire             spi_stb;
  wire [DEPTH-1:0] spi_rx;
  wire [DEPTH-1:0] spi_tx;

  spi_slave inst_spi_slave0
  (
    .clk(s_axi_aclk),
    .rst(s_axi_areset),

    .ss(ss),
    .mosi(mosi),
    .miso(miso),
    .sck(sck),

    .parallel_stb(spi_stb),
    .parallel_din(spi_tx),
    .parallel_dout(spi_rx)
  );

  wire [7:0] rx_type = spi_rx[7:0];

  reg [DEPTH-1:0] spi_rx_r0, spi_rx_r1, spi_rx_r2;
  always @ (posedge s_axi_aclk)
    if (s_axi_areset) begin
      spi_rx_r0 <= 64'h0000_0000_0000_0000;
      spi_rx_r1 <= 64'h0000_0000_0000_0000;
      spi_rx_r2 <= 64'h0000_0000_0000_0000;
    end else begin
      spi_rx_r0 <= spi_stb && (rx_type == 0) ? spi_rx : spi_rx_r0;
      spi_rx_r1 <= spi_stb && (rx_type == 1) ? spi_rx : spi_rx_r1;
      spi_rx_r2 <= spi_stb && (rx_type == 2) ? spi_rx : spi_rx_r2;
    end

  localparam IDLE              = 3'b001;
  localparam READ_IN_PROGRESS  = 3'b010;
  localparam WRITE_IN_PROGRESS = 3'b100;

  reg [2:0] state;
  reg [7:0] addr;

  always @ (posedge s_axi_aclk) begin
    if (s_axi_areset) begin
      state <= IDLE;
    end
    else case (state)

    IDLE: begin
      if (s_axi_arvalid) begin
        state <= READ_IN_PROGRESS;
        addr  <= s_axi_araddr[7:0];
      end
      else if (s_axi_awvalid) begin
        state <= WRITE_IN_PROGRESS;
        addr  <= s_axi_awaddr[7:0];
      end
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

  // write mux
  reg write_shutdown;
  reg write_irq_mask;

  always @(*) begin
    write_shutdown = 1'b0;
    write_irq_mask = 1'b0;

    if (state == WRITE_IN_PROGRESS)
      case (addr)
        8'h00: write_shutdown = 1'b1;
        8'h04: write_irq_mask = 1'b1;
      endcase
  end

  reg [31:0] shutdown = 32'h0000_0000;
  always @ (posedge s_axi_aclk) begin
    if (s_axi_areset)
      shutdown <= 32'h0000_0000;
    else if (write_shutdown)
      shutdown <= s_axi_wdata;
  end

  wire [31:0] spi_tx_tdata;
  wire        spi_tx_tvalid;
  wire [5:0] spi_tx_occupied;
  wire [5:0] spi_tx_space;

  wire [31:0] tmux = write_shutdown ? {s_axi_wdata[23:0], 8'h00}
                                    : {s_axi_wdata[7:0], s_axi_wdata[15:8], addr[7:0], 8'h01};

  wire is_spi_cmd = (addr[7:0] == 8'h00) || (addr[7:0] > 8'h04);

  axi_fifo_bram #(.WIDTH(32), .SIZE(5)) axi_fifo_short_inst
  (
    .clk(s_axi_aclk),
    .reset(s_axi_areset),
    .clear(1'b0),
    .i_tdata(tmux),
    .i_tvalid(state == WRITE_IN_PROGRESS && is_spi_cmd),
    .i_tready(),
    .o_tdata(spi_tx_tdata),
    .o_tvalid(spi_tx_tvalid),
    .o_tready(spi_stb),
    .occupied(spi_tx_occupied),
    .space(spi_tx_space)
  );

  reg [63:0] spi_tx_reg;

  always @ (posedge s_axi_aclk)
    if(s_axi_areset)
      spi_tx_reg <= 64'h0000_0000_0000_0000;
    else if (spi_stb)
      spi_tx_reg <= {spi_tx_tvalid, 31'h00, spi_tx_tdata};

  assign spi_tx = spi_tx_reg;

  /* battery stuff */
  wire [15:0] battery_voltage      = {spi_rx_r0[55:48], spi_rx_r0[63:56]};
  wire [1:0]  battery_temp_alert   =  spi_rx_r0[47:46];
  wire        battery_online       =  spi_rx_r0[45];
  wire [2:0]  battery_health       =  spi_rx_r0[44:42];
  wire [1:0]  battery_status       =  spi_rx_r0[41:40];

  /* charger stuff */
  /* unused [39:38] */
  wire [1:0]  charger_health       = spi_rx_r0[37:36];
  wire        charger_online       = spi_rx_r0[35];
  /* unused bit 34 */
  wire [1:0]  charger_charge_type  = spi_rx_r0[33:32];

  /* gauge stuff */
  wire [7:0]  gauge_status         =  spi_rx_r1[63:56];
  wire [15:0] voltage              = {spi_rx_r1[47:40], spi_rx_r1[55:48]};
  wire [15:0] temp                 = {spi_rx_r1[31:24], spi_rx_r1[39:32]};
  wire [15:0] charge_acc           = {spi_rx_r1[15:8] , spi_rx_r1[23:16]};

  /* charge last full */
  wire [15:0] charge_last_full     = {spi_rx_r2[15:8], spi_rx_r2[23:16]};

  /* settings flags */
  wire [7:0] settings = spi_rx_r2[31:24];

  reg [7:0] irq_enable;
  always @ (posedge s_axi_aclk) begin
    if (s_axi_areset)
      irq_enable <= 8'h00;
    else if (write_irq_mask)
      irq_enable <= s_axi_wdata[15:8];
  end

  wire [7:0] irq_status = gauge_status;
  assign s_axi_irq = |(irq_status & irq_enable);

  wire [3:0] version_maj = spi_rx_r0[15:12];
  wire [3:0] version_min = spi_rx_r0[11:8];

  reg [31:0] rdata;
  // read mux
  always @(*) begin
    rdata = 32'hdead_beef;

    if (state == READ_IN_PROGRESS)
      case (addr)
        8'h00: rdata = shutdown;
        8'h04: rdata = {16'h0000, irq_enable, version_maj, version_min};
        8'h08: rdata = {8'h0, battery_voltage, battery_temp_alert, battery_online, battery_health, battery_status};
        8'h0c: rdata = {27'd0, charger_charge_type, charger_online, charger_health};
        8'h10: rdata = {temp, charge_acc};
        8'h14: rdata = {8'h00, gauge_status, voltage};
        8'h18: rdata = {16'h0000, charge_last_full};
        8'h1c: rdata = {24'd0, settings};
      endcase
  end

  assign s_axi_arready = (state == IDLE);
  assign s_axi_rvalid  = (state == READ_IN_PROGRESS);
  assign s_axi_rresp   = 2'b00;

  assign s_axi_rdata   = rdata;

  assign s_axi_awready = (state == IDLE);
  assign s_axi_wready  = (state == WRITE_IN_PROGRESS);
  assign s_axi_bresp   = 2'b00;
  assign s_axi_bvalid  = (state == WRITE_IN_PROGRESS);

endmodule

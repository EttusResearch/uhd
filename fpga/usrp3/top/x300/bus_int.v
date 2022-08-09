//
// Copyright 2013 Ettus Research LLC
// Copyright 2017 Ettus Research, a National Instruments Company
// Copyright 2019-2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//

module bus_int #(
    parameter NUM_RADIOS = 2,
    parameter NUM_CHANNELS_PER_RADIO = 2,
    parameter NUM_CHANNELS = 4
)(
    input clk, input clk_div2, input reset, input reset_div2,
    output sen, output sclk, output mosi, input miso,
    inout scl0, inout sda0,
    inout scl1, inout sda1,
    inout scl2, inout sda2,
    output gps_txd, input gps_rxd,
    output debug_txd, input debug_rxd,
    output [1:0] leds,
    output [3:0] sw_rst,
    // Timekeeper
    input                   pps,
    // Block connections
    input                   ce_clk,
    input                   ce_rst,
    // Radio Connections
    input                       radio_clk,
    input                       radio_rst,
    input  [   NUM_CHANNELS-1:0] radio_rx_stb,
    input  [32*NUM_CHANNELS-1:0] radio_rx_data,
    output [   NUM_CHANNELS-1:0] radio_rx_running,
    input  [   NUM_CHANNELS-1:0] radio_tx_stb,
    output [32*NUM_CHANNELS-1:0] radio_tx_data,
    output [   NUM_CHANNELS-1:0] radio_tx_running,
    // Daughter Board Settings Buses
    output [   NUM_RADIOS-1:0] db_fe_set_stb,
    output [ 8*NUM_RADIOS-1:0] db_fe_set_addr,
    output [32*NUM_RADIOS-1:0] db_fe_set_data,
    input  [   NUM_RADIOS-1:0] db_fe_rb_stb,
    output [ 8*NUM_RADIOS-1:0] db_fe_rb_addr,
    input  [64*NUM_RADIOS-1:0] db_fe_rb_data,
    // SFP+ 0
    input SFPP0_ModAbs, input SFPP0_TxFault, input SFPP0_RxLOS, inout SFPP0_RS0, inout SFPP0_RS1,
    // SFP+ 1
    input SFPP1_ModAbs, input SFPP1_TxFault, input SFPP1_RxLOS, inout SFPP1_RS0, inout SFPP1_RS1,
    // Front-panel GPIO source
    output [23:0] fp_gpio_src,
    // Clock control and status
    input [7:0] clock_status, output [7:0] clock_control, output [31:0] ref_freq, output ref_freq_changed,
    // SFP+ 0 data stream
    output [63:0] sfp0_tx_tdata, output [3:0] sfp0_tx_tuser, output sfp0_tx_tlast, output sfp0_tx_tvalid, input sfp0_tx_tready,
    input [63:0] sfp0_rx_tdata, input [3:0] sfp0_rx_tuser, input sfp0_rx_tlast, input sfp0_rx_tvalid, output sfp0_rx_tready,
    // SFP+ 1 data stream
    output [63:0] sfp1_tx_tdata, output [3:0] sfp1_tx_tuser, output sfp1_tx_tlast, output sfp1_tx_tvalid, input sfp1_tx_tready,
    input [63:0] sfp1_rx_tdata, input [3:0] sfp1_rx_tuser, input sfp1_rx_tlast, input sfp1_rx_tvalid, output sfp1_rx_tready,
    // PCIe
    output [63:0] pcio_tdata, output [2:0] pcio_tuser, output pcio_tlast, output pcio_tvalid, input pcio_tready,
    input [63:0] pcii_tdata, input [2:0] pcii_tuser, input pcii_tlast, input pcii_tvalid, output pcii_tready,
    //iop2 message fifos
    output [63:0] o_iop2_msg_tdata, output o_iop2_msg_tvalid, output o_iop2_msg_tlast, input o_iop2_msg_tready,
    input [63:0] i_iop2_msg_tdata, input i_iop2_msg_tvalid, input i_iop2_msg_tlast, output i_iop2_msg_tready,
    //------------------------------------------------------------------
    // Wishbone Slave Interface(s)
    //------------------------------------------------------------------
    input [31:0] sfp0_wb_dat_i,
    output [31:0] sfp0_wb_dat_o,
    output [15:0] sfp0_wb_adr,
    output [3:0] sfp0_wb_sel,
    input sfp0_wb_ack,
    output sfp0_wb_stb,
    output sfp0_wb_cyc,
    output sfp0_wb_we,
    input sfp0_wb_int,  // IJB. Nothing to connect this too!! No IRQ controller on x300.

    input [31:0] sfp1_wb_dat_i,
    output [31:0] sfp1_wb_dat_o,
    output [15:0] sfp1_wb_adr,
    output [3:0] sfp1_wb_sel,
    input sfp1_wb_ack,
    output sfp1_wb_stb,
    output sfp1_wb_cyc,
    output sfp1_wb_we,
    input sfp1_wb_int,  // IJB. Nothing to connect this too!! No IRQ controller on x300.

    input [15:0] sfp0_phy_status,
    input [15:0] sfp1_phy_status,
    input [31:0] xadc_readback,

    //
    // AXI4 (128b@250MHz) interface to DDR3 controller
    //
    input           ddr3_axi_clk_x2,
    input           ddr3_axi_rst,
    // Write Address Ports
    output  [1*2-1:0]    ddr3_axi_awid,
    output  [30*2-1:0]   ddr3_axi_awaddr,
    output  [8*2-1:0]    ddr3_axi_awlen,
    output  [3*2-1:0]    ddr3_axi_awsize,
    output  [2*2-1:0]    ddr3_axi_awburst,
    output  [1*2-1:0]    ddr3_axi_awlock,
    output  [4*2-1:0]    ddr3_axi_awcache,
    output  [3*2-1:0]    ddr3_axi_awprot,
    output  [4*2-1:0]    ddr3_axi_awqos,
    output  [1*2-1:0]    ddr3_axi_awvalid,
    input   [1*2-1:0]    ddr3_axi_awready,
    // Write Data Ports
    output  [64*2-1:0]   ddr3_axi_wdata,
    output  [8*2-1:0]    ddr3_axi_wstrb,
    output  [1*2-1:0]    ddr3_axi_wlast,
    output  [1*2-1:0]    ddr3_axi_wvalid,
    input   [1*2-1:0]    ddr3_axi_wready,
    // Write Response Ports
    output  [1*2-1:0]    ddr3_axi_bready,
    input   [1*2-1:0]    ddr3_axi_bid,
    input   [2*2-1:0]    ddr3_axi_bresp,
    input   [1*2-1:0]    ddr3_axi_bvalid,
    // Read Address Ports
    output  [1*2-1:0]    ddr3_axi_arid,
    output  [30*2-1:0]   ddr3_axi_araddr,
    output  [8*2-1:0]    ddr3_axi_arlen,
    output  [3*2-1:0]    ddr3_axi_arsize,
    output  [2*2-1:0]    ddr3_axi_arburst,
    output  [1*2-1:0]    ddr3_axi_arlock,
    output  [4*2-1:0]    ddr3_axi_arcache,
    output  [3*2-1:0]    ddr3_axi_arprot,
    output  [4*2-1:0]    ddr3_axi_arqos,
    output  [1*2-1:0]    ddr3_axi_arvalid,
    input   [1*2-1:0]    ddr3_axi_arready,
    // Read Data Ports
    input   [1*2-1:0]    ddr3_axi_rid,
    input   [64*2-1:0]   ddr3_axi_rdata,
    input   [2*2-1:0]    ddr3_axi_rresp,
    input   [1*2-1:0]    ddr3_axi_rlast,
    input   [1*2-1:0]    ddr3_axi_rvalid,
    output  [1*2-1:0]    ddr3_axi_rready,


   // Debug
    output [31:0] debug0,
    output [31:0] debug1,
    output [127:0] debug2);

   localparam SR_AWIDTH = 8;
   localparam RB_AWIDTH = 8;

   localparam SR_LEDS            = 8'd00;
   localparam SR_SW_RST          = 8'd01;
   localparam SR_CLOCK_CTRL      = 8'd02;
   localparam SR_DEVICE_ID       = 8'd03;
   localparam SR_REF_FREQ        = 8'd04;
   localparam SR_SFPP_CTRL0      = 8'd08;
   localparam SR_SFPP_CTRL1      = 8'd09;
   localparam SR_SPI             = 8'd32;
   localparam SR_ETHINT0         = 8'd40;
   localparam SR_ETHINT1         = 8'd56;
   localparam SR_FP_GPIO_SRC     = 8'd72;
   localparam SR_BASE_TIME       = 8'd100;

   localparam RB_COUNTER         = 8'd00;
   localparam RB_SPI_RDY         = 8'd01;
   localparam RB_SPI_DATA        = 8'd02;
   localparam RB_CLK_STATUS      = 8'd03;
   localparam RB_ETH_TYPE0       = 8'd04;
   localparam RB_ETH_TYPE1       = 8'd05;
   localparam RB_COMPAT_NUM      = 8'd06;
   localparam RB_RFNOC_INFO      = 8'd07;
   localparam RB_SFPP_STATUS0    = 8'd08;
   localparam RB_SFPP_STATUS1    = 8'd09;
   localparam RB_GIT_HASH        = 8'd10;
   localparam RB_XADC_VALS       = 8'd11;
   localparam RB_NUM_TIMEKEEPERS = 8'd12;
   localparam RB_FP_GPIO_SRC     = 8'd13;
   localparam RB_DEVICE_ID       = 8'd14;

   localparam COMPAT_MAJOR       = 16'h0027;
   localparam COMPAT_MINOR       = 16'h0000;
   localparam NUM_TIMEKEEPERS    = 1;

   // Include the RFNoC image core header file
   `ifdef RFNOC_IMAGE_CORE_HDR
     `include `"`RFNOC_IMAGE_CORE_HDR`"
   `else
     ERROR_RFNOC_IMAGE_CORE_HDR_not_defined();
     `define CHDR_WIDTH     64
     `define RFNOC_PROTOVER { 8'd1, 8'd0 }
   `endif
   localparam CHDR_W         = `CHDR_WIDTH;
   localparam RFNOC_PROTOVER = `RFNOC_PROTOVER;

   wire [31:0] 	        set_data;
   wire [7:0] 	        set_addr;
   reg  [31:0] 	        rb_data;
   wire [RB_AWIDTH-1:0] rb_addr;
   wire 		        rb_rd_stb;
   wire                 set_stb;
   wire 	            spi_ready;
   wire [31:0] 	        rb_spi_data;
   wire [15:0]          device_id;

   wire                 m_ctrlport_req_wr_radio0;
   wire                 m_ctrlport_req_rd_radio0;
   wire [19:0]          m_ctrlport_req_addr_radio0;
   wire [31:0]          m_ctrlport_req_data_radio0;
   wire [3:0]           m_ctrlport_req_byte_en_radio0;
   wire                 m_ctrlport_req_has_time_radio0;
   wire [63:0]          m_ctrlport_req_time_radio0;
   wire                 m_ctrlport_resp_ack_radio0;
   wire [1:0]           m_ctrlport_resp_status_radio0;
   wire [31:0]          m_ctrlport_resp_data_radio0;
   wire                 m_ctrlport_req_wr_radio1;
   wire                 m_ctrlport_req_rd_radio1;
   wire [19:0]          m_ctrlport_req_addr_radio1;
   wire [31:0]          m_ctrlport_req_data_radio1;
   wire [3:0]           m_ctrlport_req_byte_en_radio1;
   wire                 m_ctrlport_req_has_time_radio1;
   wire [63:0]          m_ctrlport_req_time_radio1;
   wire                 m_ctrlport_resp_ack_radio1;
   wire [1:0]           m_ctrlport_resp_status_radio1;
   wire [31:0]          m_ctrlport_resp_data_radio1;

   // ZPU in and ZPU out axi streams
   wire [63:0] 	  zpui_tdata, zpuo_tdata;
   wire [3:0] 	  zpui_tuser, zpuo_tuser;
   wire 	  zpui_tlast, zpuo_tlast, zpui_tvalid, zpuo_tvalid, zpui_tready, zpuo_tready;
   wire [63:0] 	  zpui0_tdata, zpuo0_tdata;
   wire [3:0] 	  zpui0_tuser, zpuo0_tuser;
   wire 	  zpui0_tlast, zpuo0_tlast, zpui0_tvalid, zpuo0_tvalid, zpui0_tready, zpuo0_tready;
   wire [63:0] 	  zpui1_tdata, zpuo1_tdata;
   wire [3:0] 	  zpui1_tuser, zpuo1_tuser;
   wire 	  zpui1_tlast, zpuo1_tlast, zpui1_tvalid, zpuo1_tvalid, zpui1_tready, zpuo1_tready;

   // v2e (vita to ethernet) and e2v (eth to vita)
   wire [63:0] 	  v2e0_tdata, v2e1_tdata, e2v0_tdata, e2v1_tdata;
   wire 	  v2e0_tlast, v2e1_tlast, v2e0_tvalid, v2e1_tvalid, v2e0_tready, v2e1_tready;
   wire 	  e2v0_tlast, e2v1_tlast, e2v0_tvalid, e2v1_tvalid, e2v0_tready, e2v1_tready;

   //settings bus for crossbar router
   wire [31:0] 	  set_data_xb;
   wire [8:0] 	  set_addr_xb;
   wire 	  set_stb_xb;

   // SFP+ logic
   wire  SFPP0_ModAbs_sync, SFPP0_TxFault_sync, SFPP0_RxLOS_sync;
   reg   SFPP0_ModAbs_reg,  SFPP0_TxFault_reg,  SFPP0_RxLOS_reg;
   reg   SFPP0_ModAbs_chgd, SFPP0_TxFault_chgd, SFPP0_RxLOS_chgd;

   wire  SFPP1_ModAbs_sync, SFPP1_TxFault_sync, SFPP1_RxLOS_sync;
   reg   SFPP1_ModAbs_reg,  SFPP1_TxFault_reg,  SFPP1_RxLOS_reg;
   reg   SFPP1_ModAbs_chgd, SFPP1_TxFault_chgd, SFPP1_RxLOS_chgd;

   wire [15:0]  sfp0_phy_status_sync, sfp1_phy_status_sync;

   ////////////////////////////////////////////////////////////////////
   // Soft CPU - drives network setup, soft reset, ICMP, ...
   ////////////////////////////////////////////////////////////////////
   soft_ctrl #(.SB_ADDRW(SR_AWIDTH), .RB_ADDRW(RB_AWIDTH)) sc
     (
      .clk(clk), .rst(reset),
      .clk_div2(clk_div2), .rst_div2(reset_div2),
      //------------------------------------------------------------------
      // I2C interfaces
      //------------------------------------------------------------------
      .scl0(scl0), .sda0(sda0),
      .scl1(scl1), .sda1(sda1),
      .scl2(scl2), .sda2(sda2),
      //------------------------------------------------------------------
      // UARTs for CPU comms
      //------------------------------------------------------------------
      .gps_rxd(gps_rxd), .gps_txd(gps_txd),
      .debug_rxd(debug_rxd), .debug_txd(debug_txd),
      //------------------------------------------------------------------
      // settings bus interface
      //------------------------------------------------------------------
      .set_stb(set_stb),
      .set_addr(set_addr),
      .set_data(set_data),
      //------------------------------------------------------------------
      // settings bus interface for crossbar router
      //------------------------------------------------------------------
      .set_stb_xb(set_stb_xb),
      .set_addr_xb(set_addr_xb),
      .set_data_xb(set_data_xb),
      //------------------------------------------------------------------
      // readback bus interface
      //------------------------------------------------------------------
      .rb_addr(rb_addr),
      .rb_data(rb_data),
      .rb_rd_stb(rb_rd_stb),
      //------------------------------------------------------------------
      // packet interface in
      //------------------------------------------------------------------
      .rx_tdata(zpui_tdata),
      .rx_tuser(zpui_tuser),
      .rx_tlast(zpui_tlast),
      .rx_tvalid(zpui_tvalid),
      .rx_tready(zpui_tready),
      //------------------------------------------------------------------
      // packet interface out
      //------------------------------------------------------------------
      .tx_tdata(zpuo_tdata),
      .tx_tuser(zpuo_tuser),
      .tx_tlast(zpuo_tlast),
      .tx_tvalid(zpuo_tvalid),
      .tx_tready(zpuo_tready),
      //------------------------------------------------------------------
      // Wishbone Slave Interface(s)
      //------------------------------------------------------------------
      .s4_dat_i(sfp0_wb_dat_i),
      .s4_dat_o(sfp0_wb_dat_o),
      .s4_adr(sfp0_wb_adr),
      .s4_sel(sfp0_wb_sel),
      .s4_ack(sfp0_wb_ack),
      .s4_stb(sfp0_wb_stb),
      .s4_cyc(sfp0_wb_cyc),
      .s4_we(sfp0_wb_we),
      .s4_int(sfp0_wb_int),
      //------------------------------------------------------------------
      // IoPort2 Msg Interface
      //------------------------------------------------------------------
      .o_iop2_msg_tdata(o_iop2_msg_tdata),
      .o_iop2_msg_tvalid(o_iop2_msg_tvalid),
      .o_iop2_msg_tlast(o_iop2_msg_tlast),
      .o_iop2_msg_tready(o_iop2_msg_tready),
      .i_iop2_msg_tdata(i_iop2_msg_tdata),
      .i_iop2_msg_tvalid(i_iop2_msg_tvalid),
      .i_iop2_msg_tlast(i_iop2_msg_tlast),
      .i_iop2_msg_tready(i_iop2_msg_tready),

      .s5_dat_i(sfp1_wb_dat_i),
      .s5_dat_o(sfp1_wb_dat_o),
      .s5_adr(sfp1_wb_adr),
      .s5_sel(sfp1_wb_sel),
      .s5_ack(sfp1_wb_ack),
      .s5_stb(sfp1_wb_stb),
      .s5_cyc(sfp1_wb_cyc),
      .s5_we(sfp1_wb_we),
      .s5_int(sfp1_wb_int),

      //------------------------------------------------------------------
      // Debug
      //------------------------------------------------------------------
      .debug0(debug2),
      .debug1()
   );

   setting_reg #(.my_addr(SR_LEDS), .awidth(SR_AWIDTH), .width(2)) set_leds
     (.clk(clk), .rst(reset),
      .strobe(set_stb), .addr(set_addr), .in(set_data),
      .out(leds));

   //
   // SW_RST - Bit allocation:
   // [0] - PHY reset
   // [1] - Radio clk domain reset
   // [2] - Radio Clk PLL reset.
   // [3] - ADC IdelayCtrl reset
   //
   setting_reg #(.my_addr(SR_SW_RST), .awidth(SR_AWIDTH), .width(4)) set_sw_rst
     (.clk(clk), .rst(reset),
      .strobe(set_stb), .addr(set_addr), .in(set_data),
      .out(sw_rst));

   setting_reg #(.my_addr(SR_CLOCK_CTRL), .awidth(SR_AWIDTH), .width(8),
        .at_reset(7'b1000000) //bit 6 high means GPSDO on by default
    ) set_clk_ctrl
     (.clk(clk), .rst(reset),
      .strobe(set_stb), .addr(set_addr), .in(set_data),
      .out(clock_control));

   setting_reg #(.my_addr(SR_DEVICE_ID), .awidth(SR_AWIDTH), .width(16),
        .at_reset(32'd0)
    ) set_dev_id
     (.clk(clk), .rst(reset),
      .strobe(set_stb), .addr(set_addr), .in(set_data),
      .out(device_id), .changed());

   setting_reg #(.my_addr(SR_REF_FREQ), .awidth(SR_AWIDTH), .width(32),
        .at_reset(32'd10_000_000) //default to 10 MHz reference clock
    ) set_ref_freq
     (.clk(clk), .rst(reset),
      .strobe(set_stb), .addr(set_addr), .in(set_data),
      .out(ref_freq), .changed(ref_freq_changed));

   simple_spi_core #(.BASE(SR_SPI), .WIDTH(1), .CLK_IDLE(0), .SEN_IDLE(1'b1)) misc_spi
     (.clock(clk), .reset(reset),
      .set_stb(set_stb), .set_addr(set_addr), .set_data(set_data),
      .readback(rb_spi_data), .ready(spi_ready),
      .sen(sen), .sclk(sclk), .mosi(mosi), .miso(miso),
      .debug());

   reg  [31:0] counter;
   wire [31:0] rb_data_crossbar = 32'h0;
   wire [63:0] radio_time_tb;
   wire [63:0] radio_time_last_pps_tb;
   wire [63:0] period_ns_q32_tb;
   wire [63:0] radio_time;
   wire [63:0] radio_time_last_pps;
   reg  [31:0] radio_time_hi;
   reg  [31:0] radio_time_last_pps_hi;
   reg radio_time_hi_ld;
   reg radio_time_last_pps_hi_ld;


   always @(posedge clk) counter <= counter + 1;

   always @* begin
     radio_time_hi_ld          = 1'b0;
     radio_time_last_pps_hi_ld = 1'b0;
     casex (rb_addr)
       RB_RFNOC_INFO: rb_data = {CHDR_W[15:0], RFNOC_PROTOVER[15:0]};
       RB_DEVICE_ID: rb_data = {16'd0, device_id};
       RB_COMPAT_NUM: rb_data = {COMPAT_MAJOR[15:0], COMPAT_MINOR[15:0]};
       RB_COUNTER: rb_data = counter;
       RB_SPI_RDY: rb_data = {31'b0, spi_ready};
       RB_SPI_DATA: rb_data = rb_spi_data;
       RB_CLK_STATUS: rb_data = {24'b0, clock_status};
       // SFPP Interface pins.
       RB_SFPP_STATUS0: rb_data = {
            sfp0_phy_status_sync, 10'b0,
            SFPP0_ModAbs_chgd, SFPP0_TxFault_chgd, SFPP0_RxLOS_chgd,
            SFPP0_ModAbs_sync, SFPP0_TxFault_sync, SFPP0_RxLOS_sync};
       RB_SFPP_STATUS1: rb_data = {
            sfp1_phy_status_sync, 10'b0,
            SFPP1_ModAbs_chgd, SFPP1_TxFault_chgd, SFPP1_RxLOS_chgd,
            SFPP1_ModAbs_sync, SFPP1_TxFault_sync, SFPP1_RxLOS_sync};
       RB_NUM_TIMEKEEPERS: rb_data = NUM_TIMEKEEPERS;
       // Allow readback of configured ethernet interfaces.
`ifdef SFP0_AURORA
       RB_ETH_TYPE0: rb_data = {32'h2};
`else
   `ifdef SFP0_10GBE
       RB_ETH_TYPE0: rb_data = {32'h1};
   `else
       RB_ETH_TYPE0: rb_data = {32'h0};
   `endif
`endif
`ifdef SFP1_AURORA
       RB_ETH_TYPE1: rb_data = {32'h2};
`else
   `ifdef SFP1_10GBE
       RB_ETH_TYPE1: rb_data = {32'h1};
   `else
       RB_ETH_TYPE1: rb_data = {32'h0};
   `endif
`endif
       RB_GIT_HASH:  rb_data = `GIT_HASH;
       RB_XADC_VALS: rb_data = xadc_readback;
       RB_FP_GPIO_SRC: rb_data = fp_gpio_src;
       SR_BASE_TIME: begin
                       rb_data = radio_time[31:0];
                       radio_time_hi_ld = rb_rd_stb;
                     end
       SR_BASE_TIME + 'h04: rb_data = radio_time_hi;
       SR_BASE_TIME + 'h14: begin
                           rb_data = radio_time_last_pps[31:0];
                           radio_time_last_pps_hi_ld = rb_rd_stb;
                         end
       SR_BASE_TIME + 'h18: rb_data = radio_time_last_pps_hi;
       SR_BASE_TIME + 'h1C: rb_data = period_ns_q32_tb[31:0];
       SR_BASE_TIME + 'h20: rb_data = period_ns_q32_tb[63:32];

       default: begin
         rb_data                   = 32'h0;
       end
     endcase // case (rb_addr)
   end

   always @(posedge clk_div2) begin
     if (radio_time_hi_ld)
       radio_time_hi <= radio_time[63:32];
     if (radio_time_last_pps_hi_ld)
       radio_time_last_pps_hi <= radio_time_last_pps[63:32];
   end

   // Timekeeper

   axi_fifo_2clk #(
     .WIDTH (64),
     .SIZE  (3)
   ) radio_time_clk_cross_fifo (
     .reset    (radio_rst),
     .i_aclk   (radio_clk),
     .i_tdata  (radio_time_tb),
     .i_tvalid (1'b1),
     .i_tready (),
     .o_aclk   (clk_div2),
     .o_tdata  (radio_time),
     .o_tready (1'b1),
     .o_tvalid ()
   );

   axi_fifo_2clk #(
     .WIDTH (64),
     .SIZE  (3)
   ) radio_time_last_pps_clk_cross_fifo (
     .reset    (radio_rst),
     .i_aclk   (radio_clk),
     .i_tdata  (radio_time_last_pps_tb),
     .i_tvalid (1'b1),
     .i_tready (),
     .o_aclk   (clk_div2),
     .o_tdata  (radio_time_last_pps),
     .o_tready (1'b1),
     .o_tvalid ()
   );

   timekeeper #(
     .BASE_ADDR      (SR_BASE_TIME),
     .TIME_INCREMENT (1'b1)
   ) timekeeper_i (
     .tb_clk                (radio_clk),
     .tb_rst                (radio_rst),
     .s_ctrlport_clk        (clk_div2),
     .s_ctrlport_req_wr     (set_stb),
     .s_ctrlport_req_rd     (),
     .s_ctrlport_req_addr   (set_addr),
     .s_ctrlport_req_data   (set_data),
     .s_ctrlport_resp_ack   (),
     .s_ctrlport_resp_data  (),
     .sample_rx_stb         (1'b1),
     .pps                   (pps),
     .tb_timestamp          (radio_time_tb),
     .tb_timestamp_last_pps (radio_time_last_pps_tb),
     .tb_period_ns_q32      (period_ns_q32_tb)
   );

   // Latch state changes to SFP0+ pins.
   synchronizer #(.INITIAL_VAL(1'b0)) sfpp0_modabs_sync (
      .clk(clk), .rst(1'b0 /* no reset */), .in(SFPP0_ModAbs), .out(SFPP0_ModAbs_sync));
   synchronizer #(.INITIAL_VAL(1'b0)) sfpp0_txfault_sync (
      .clk(clk), .rst(1'b0 /* no reset */), .in(SFPP0_TxFault), .out(SFPP0_TxFault_sync));
   synchronizer #(.INITIAL_VAL(1'b0)) sfpp0_rxlos_sync (
      .clk(clk), .rst(1'b0 /* no reset */), .in(SFPP0_RxLOS), .out(SFPP0_RxLOS_sync));

   always @(posedge clk) begin
      SFPP0_ModAbs_reg  <= SFPP0_ModAbs_sync;
      SFPP0_TxFault_reg <= SFPP0_TxFault_sync;
      SFPP0_RxLOS_reg   <= SFPP0_RxLOS_sync;
   end

   always @(posedge clk) begin
      if (reset || (rb_rd_stb && (rb_addr == RB_SFPP_STATUS0))) begin
         SFPP0_ModAbs_chgd  <= 1'b0;
         SFPP0_TxFault_chgd <= 1'b0;
         SFPP0_RxLOS_chgd   <= 1'b0;
      end else begin
         if (SFPP0_ModAbs_sync != SFPP0_ModAbs_reg)
            SFPP0_ModAbs_chgd <= 1'b1;
         if (SFPP0_TxFault_sync != SFPP0_TxFault_reg)
            SFPP0_TxFault_chgd <= 1'b1;
         if (SFPP0_RxLOS_sync != SFPP0_RxLOS_reg)
            SFPP0_RxLOS_chgd <= 1'b1;
      end
   end

   // Latch state changes to SFP1+ pins.
   synchronizer #(.INITIAL_VAL(1'b0)) sfpp1_modabs_sync (
      .clk(clk), .rst(1'b0 /* no reset */), .in(SFPP1_ModAbs), .out(SFPP1_ModAbs_sync));
   synchronizer #(.INITIAL_VAL(1'b0)) sfpp1_txfault_sync (
      .clk(clk), .rst(1'b0 /* no reset */), .in(SFPP1_TxFault), .out(SFPP1_TxFault_sync));
   synchronizer #(.INITIAL_VAL(1'b0)) sfpp1_rxlos_sync (
      .clk(clk), .rst(1'b0 /* no reset */), .in(SFPP1_RxLOS), .out(SFPP1_RxLOS_sync));

   always @(posedge clk) begin
      SFPP1_ModAbs_reg  <= SFPP1_ModAbs_sync;
      SFPP1_TxFault_reg <= SFPP1_TxFault_sync;
      SFPP1_RxLOS_reg   <= SFPP1_RxLOS_sync;
   end

   always @(posedge clk) begin
      if (reset || (rb_rd_stb && (rb_addr == RB_SFPP_STATUS1))) begin
         SFPP1_ModAbs_chgd  <= 1'b0;
         SFPP1_TxFault_chgd <= 1'b0;
         SFPP1_RxLOS_chgd   <= 1'b0;
      end else begin
         if (SFPP1_ModAbs_sync != SFPP1_ModAbs_reg)
            SFPP1_ModAbs_chgd <= 1'b1;
         if (SFPP1_TxFault_sync != SFPP1_TxFault_reg)
            SFPP1_TxFault_chgd <= 1'b1;
         if (SFPP1_RxLOS_sync != SFPP1_RxLOS_reg)
            SFPP1_RxLOS_chgd <= 1'b1;
      end
   end

   //Synchronize ethernet PHY status bits to bus_clk
   //All bits in the bus can be treated as asynchronous
   genvar i;
   generate
   for (i=0; i<16; i=i+1) begin: eth_status_synchronizer_gen
      synchronizer #(.INITIAL_VAL(1'b0)) sfp0_status (
         .clk(clk), .rst(1'b0 /* no reset */), .in(sfp0_phy_status[i]), .out(sfp0_phy_status_sync[i]));
      synchronizer #(.INITIAL_VAL(1'b0)) sfp1_status (
         .clk(clk), .rst(1'b0 /* no reset */), .in(sfp1_phy_status[i]), .out(sfp1_phy_status_sync[i]));
   end
   endgenerate


   wire [1:0] 	  sfpp0_ctrl;

   // SFPP_RS0/1 pins are open drain.
   setting_reg #(.my_addr(SR_SFPP_CTRL0), .awidth(SR_AWIDTH), .width(2), .at_reset(2'b00)) set_sfpp0_ctrl
     (.clk(clk), .rst(reset),
      .strobe(set_stb), .addr(set_addr), .in(set_data),
      .out(sfpp0_ctrl));

   assign SFPP0_RS0 = sfpp0_ctrl[0] ? 1'b0 : 1'bz;
   assign SFPP0_RS1 = sfpp0_ctrl[1] ? 1'b0 : 1'bz;

  wire [1:0] 	  sfpp1_ctrl;

   // SFPP1_RS0/1 pins are open drain.
   setting_reg #(.my_addr(SR_SFPP_CTRL1), .awidth(SR_AWIDTH), .width(2), .at_reset(2'b00)) set_sfpp1_ctrl
     (.clk(clk), .rst(reset),
      .strobe(set_stb), .addr(set_addr), .in(set_data),
      .out(sfpp1_ctrl));

   assign SFPP1_RS0 = sfpp1_ctrl[0] ? 1'b0 : 1'bz;
   assign SFPP1_RS1 = sfpp1_ctrl[1] ? 1'b0 : 1'bz;


   // Front-panel GPIO source - Each pin is allocated 2 bits
   setting_reg #(.my_addr(SR_FP_GPIO_SRC), .awidth(SR_AWIDTH), .width(24)) set_fp_gpio_src
     (.clk(clk), .rst(reset),
      .strobe(set_stb), .addr(set_addr), .in(set_data),
      .out(fp_gpio_src));

   // ////////////////////////////////////////////////////////////////
   // ETH interfaces

`ifdef SFP0_AURORA
   // The packet format over Aurora is CHDR so we don't need any special framing/deframing
   assign {e2v0_tdata, e2v0_tlast, e2v0_tvalid, sfp0_rx_tready} = {sfp0_rx_tdata, sfp0_rx_tlast, sfp0_rx_tvalid, e2v0_tready};
   assign {sfp0_tx_tdata, sfp0_tx_tlast, sfp0_tx_tvalid, v2e0_tready} = {v2e0_tdata, v2e0_tlast, v2e0_tvalid, sfp0_tx_tready};
   assign {zpui0_tdata, zpui0_tlast, zpui0_tvalid, zpuo0_tready} = {64'h0, 1'b0, 1'b0, 1'b1};
`else
   x300_eth_interface #(
      .PROTOVER(RFNOC_PROTOVER), .MTU(10), .NODE_INST(0), .BASE(SR_ETHINT0)
   ) eth_interface0 (
      .clk(clk), .reset(reset),
      .device_id(device_id),
      .set_stb(set_stb), .set_addr(set_addr), .set_data(set_data),
      .eth_tx_tdata(sfp0_tx_tdata), .eth_tx_tuser(sfp0_tx_tuser), .eth_tx_tlast(sfp0_tx_tlast),
      .eth_tx_tvalid(sfp0_tx_tvalid), .eth_tx_tready(sfp0_tx_tready),
      .eth_rx_tdata(sfp0_rx_tdata), .eth_rx_tuser(sfp0_rx_tuser), .eth_rx_tlast(sfp0_rx_tlast),
      .eth_rx_tvalid(sfp0_rx_tvalid), .eth_rx_tready(sfp0_rx_tready),
      .e2v_tdata(e2v0_tdata), .e2v_tlast(e2v0_tlast), .e2v_tvalid(e2v0_tvalid), .e2v_tready(e2v0_tready),
      .v2e_tdata(v2e0_tdata), .v2e_tlast(v2e0_tlast), .v2e_tvalid(v2e0_tvalid), .v2e_tready(v2e0_tready),
      .e2z_tdata(zpui0_tdata), .e2z_tuser(zpui0_tuser), .e2z_tlast(zpui0_tlast), .e2z_tvalid(zpui0_tvalid), .e2z_tready(zpui0_tready),
      .z2e_tdata(zpuo0_tdata), .z2e_tuser(zpuo0_tuser), .z2e_tlast(zpuo0_tlast), .z2e_tvalid(zpuo0_tvalid), .z2e_tready(zpuo0_tready)
   );
`endif

`ifdef SFP1_AURORA
   // The packet format over Aurora is CHDR so we don't need any special framing/deframing
   assign {e2v1_tdata, e2v1_tlast, e2v1_tvalid, sfp1_rx_tready} = {sfp1_rx_tdata, sfp1_rx_tlast, sfp1_rx_tvalid, e2v1_tready};
   assign {sfp1_tx_tdata, sfp1_tx_tlast, sfp1_tx_tvalid, v2e1_tready} = {v2e1_tdata, v2e1_tlast, v2e1_tvalid, sfp1_tx_tready};
   assign {zpui1_tdata, zpui1_tlast, zpui1_tvalid, zpuo1_tready} = {64'h0, 1'b0, 1'b0, 1'b1};
`else
   x300_eth_interface #(
      .PROTOVER(RFNOC_PROTOVER), .MTU(10), .NODE_INST(1), .BASE(SR_ETHINT1)
   ) eth_interface1 (
      .clk(clk), .reset(reset),
      .device_id(device_id),
      .set_stb(set_stb), .set_addr(set_addr), .set_data(set_data),
      .eth_tx_tdata(sfp1_tx_tdata), .eth_tx_tuser(sfp1_tx_tuser), .eth_tx_tlast(sfp1_tx_tlast),
      .eth_tx_tvalid(sfp1_tx_tvalid), .eth_tx_tready(sfp1_tx_tready),
      .eth_rx_tdata(sfp1_rx_tdata), .eth_rx_tuser(sfp1_rx_tuser), .eth_rx_tlast(sfp1_rx_tlast),
      .eth_rx_tvalid(sfp1_rx_tvalid), .eth_rx_tready(sfp1_rx_tready),
      .e2v_tdata(e2v1_tdata), .e2v_tlast(e2v1_tlast), .e2v_tvalid(e2v1_tvalid), .e2v_tready(e2v1_tready),
      .v2e_tdata(v2e1_tdata), .v2e_tlast(v2e1_tlast), .v2e_tvalid(v2e1_tvalid), .v2e_tready(v2e1_tready),
      .e2z_tdata(zpui1_tdata), .e2z_tuser(zpui1_tuser), .e2z_tlast(zpui1_tlast), .e2z_tvalid(zpui1_tvalid), .e2z_tready(zpui1_tready),
      .z2e_tdata(zpuo1_tdata), .z2e_tuser(zpuo1_tuser), .z2e_tlast(zpuo1_tlast), .z2e_tvalid(zpuo1_tvalid), .z2e_tready(zpuo1_tready)
   );
`endif

   axi_mux4 #(.PRIO(0), .WIDTH(68)) zpui_mux
     (.clk(clk), .reset(reset), .clear(1'b0),
      .i0_tdata({zpui0_tuser,zpui0_tdata}), .i0_tlast(zpui0_tlast), .i0_tvalid(zpui0_tvalid), .i0_tready(zpui0_tready),
      .i1_tdata({zpui1_tuser,zpui1_tdata}), .i1_tlast(zpui1_tlast), .i1_tvalid(zpui1_tvalid), .i1_tready(zpui1_tready),
      .i2_tdata(68'h0), .i2_tlast(1'b0), .i2_tvalid(1'b0), .i2_tready(),
      .i3_tdata(68'h0), .i3_tlast(1'b0), .i3_tvalid(1'b0), .i3_tready(),
      .o_tdata({zpui_tuser,zpui_tdata}), .o_tlast(zpui_tlast), .o_tvalid(zpui_tvalid), .o_tready(zpui_tready));

   // Demux ZPU to Eth output by the port number in top 8 bits of data on first line
   wire [67:0] 	  zpuo_eth_header;
   wire [1:0] 	  zpuo_eth_dest = (zpuo_eth_header[63:56] == 8'd0) ? 2'b00 : 2'b01;

   axi_demux4 #(.ACTIVE_CHAN(4'b0011), .WIDTH(68)) zpuo_demux
     (.clk(clk), .reset(reset), .clear(1'b0),
      .header(zpuo_eth_header), .dest(zpuo_eth_dest),
      .i_tdata({zpuo_tuser,zpuo_tdata}), .i_tlast(zpuo_tlast), .i_tvalid(zpuo_tvalid), .i_tready(zpuo_tready),
      .o0_tdata({zpuo0_tuser,zpuo0_tdata}), .o0_tlast(zpuo0_tlast), .o0_tvalid(zpuo0_tvalid), .o0_tready(zpuo0_tready),
      .o1_tdata({zpuo1_tuser,zpuo1_tdata}), .o1_tlast(zpuo1_tlast), .o1_tvalid(zpuo1_tvalid), .o1_tready(zpuo1_tready),
      .o2_tdata(), .o2_tlast(), .o2_tvalid(), .o2_tready(1'b1),
      .o3_tdata(), .o3_tlast(), .o3_tvalid(), .o3_tready(1'b1));
   //
   // End ETH interfaces
   ////////////////////////////////////////////////////////////////////////////

    ///////////////////////////////////////////////////////////////////////////
    // PCIe Interface
    //
    localparam DMA_RX_DEST_WIDTH = 3;
    // PCIe to CHDR (p2c) and CHDR to PCIe (c2p)
    wire [63:0] 	  p2c_tdata;
    wire        	  p2c_tlast, p2c_tvalid, p2c_tready;
    wire [63:0] 	  c2p_tdata;
    wire        	  c2p_tlast, c2p_tvalid, c2p_tready;
    // Transport adapter with the management interface
    nirio_chdr64_adapter #(
       .PROTOVER(RFNOC_PROTOVER),
       .DMA_ID_WIDTH(DMA_RX_DEST_WIDTH)
    ) nirio_xport_adapter
    (
        .clk(clk), .rst(reset),
        .device_id(device_id),
        // From x300_pcie_int
        .s_dma_tdata(pcii_tdata),
        .s_dma_tuser(pcii_tuser),
        .s_dma_tlast(pcii_tlast),
        .s_dma_tvalid(pcii_tvalid),
        .s_dma_tready(pcii_tready),
        // To x300_pcie_int
        .m_dma_tdata(pcio_tdata),
        .m_dma_tuser(pcio_tuser),
        .m_dma_tlast(pcio_tlast),
        .m_dma_tvalid(pcio_tvalid),
        .m_dma_tready(pcio_tready),
        // From xport adapter into the image core
        .m_chdr_tdata(p2c_tdata),
        .m_chdr_tlast(p2c_tlast),
        .m_chdr_tvalid(p2c_tvalid),
        .m_chdr_tready(p2c_tready),
        // From image core into the xport adapter
        .s_chdr_tdata(c2p_tdata),
        .s_chdr_tlast(c2p_tlast),
        .s_chdr_tvalid(c2p_tvalid),
        .s_chdr_tready(c2p_tready)
    );
    //
    // End PCIe Interface
    ///////////////////////////////////////////////////////////////////////////

  rfnoc_image_core #(
    .CHDR_W   (CHDR_W),
    .PROTOVER (RFNOC_PROTOVER)
  ) rfnoc_sandbox_i (
    .chdr_aclk               (clk        ),
    .ctrl_aclk               (clk_div2   ),
    .core_arst               (reset      ),
    .device_id               (device_id  ),
    .radio_clk               (radio_clk  ),
    .ce_clk                  (ce_clk     ),
    .dram_clk                (ddr3_axi_clk_x2),
    .m_ctrlport_radio1_req_wr       (m_ctrlport_req_wr_radio1      ),
    .m_ctrlport_radio1_req_rd       (m_ctrlport_req_rd_radio1      ),
    .m_ctrlport_radio1_req_addr     (m_ctrlport_req_addr_radio1    ),
    .m_ctrlport_radio1_req_data     (m_ctrlport_req_data_radio1    ),
    .m_ctrlport_radio1_req_byte_en  (m_ctrlport_req_byte_en_radio1 ),
    .m_ctrlport_radio1_req_has_time (m_ctrlport_req_has_time_radio1),
    .m_ctrlport_radio1_req_time     (m_ctrlport_req_time_radio1    ),
    .m_ctrlport_radio1_resp_ack     (m_ctrlport_resp_ack_radio1    ),
    .m_ctrlport_radio1_resp_status  (m_ctrlport_resp_status_radio1 ),
    .m_ctrlport_radio1_resp_data    (m_ctrlport_resp_data_radio1   ),
    .m_ctrlport_radio0_req_wr       (m_ctrlport_req_wr_radio0      ),
    .m_ctrlport_radio0_req_rd       (m_ctrlport_req_rd_radio0      ),
    .m_ctrlport_radio0_req_addr     (m_ctrlport_req_addr_radio0    ),
    .m_ctrlport_radio0_req_data     (m_ctrlport_req_data_radio0    ),
    .m_ctrlport_radio0_req_byte_en  (m_ctrlport_req_byte_en_radio0 ),
    .m_ctrlport_radio0_req_has_time (m_ctrlport_req_has_time_radio0),
    .m_ctrlport_radio0_req_time     (m_ctrlport_req_time_radio0    ),
    .m_ctrlport_radio0_resp_ack     (m_ctrlport_resp_ack_radio0    ),
    .m_ctrlport_radio0_resp_status  (m_ctrlport_resp_status_radio0 ),
    .m_ctrlport_radio0_resp_data    (m_ctrlport_resp_data_radio0   ),
    .radio_time                     (radio_time_tb    ),
    .radio_rx_stb_radio0            (radio_rx_stb[1:0]     ),
    .radio_rx_data_radio0           (radio_rx_data[63:0]   ),
    .radio_rx_running_radio0        (radio_rx_running[1:0] ),
    .radio_tx_stb_radio0            (radio_tx_stb[1:0]     ),
    .radio_tx_data_radio0           (radio_tx_data[63:0]   ),
    .radio_tx_running_radio0        (radio_tx_running[1:0] ),
    .radio_rx_stb_radio1            (radio_rx_stb[3:2]     ),
    .radio_rx_data_radio1           (radio_rx_data[127:64] ),
    .radio_rx_running_radio1        (radio_rx_running[3:2] ),
    .radio_tx_stb_radio1            (radio_tx_stb[3:2]     ),
    .radio_tx_data_radio1           (radio_tx_data[127:64] ),
    .radio_tx_running_radio1        (radio_tx_running[3:2] ),
    // DRAM interface
    .axi_rst                 (ddr3_axi_rst),
    // Slave Interface Write Address Ports
    .m_axi_awid              (ddr3_axi_awid   ),
    .m_axi_awaddr            (ddr3_axi_awaddr ),
    .m_axi_awlen             (ddr3_axi_awlen  ),
    .m_axi_awsize            (ddr3_axi_awsize ),
    .m_axi_awburst           (ddr3_axi_awburst),
    .m_axi_awlock            (ddr3_axi_awlock ),
    .m_axi_awcache           (ddr3_axi_awcache),
    .m_axi_awprot            (ddr3_axi_awprot ),
    .m_axi_awqos             (ddr3_axi_awqos  ),
    .m_axi_awregion          (),
    .m_axi_awuser            (),
    .m_axi_awvalid           (ddr3_axi_awvalid),
    .m_axi_awready           (ddr3_axi_awready),
    // Slave Interface Write Data Ports
    .m_axi_wdata             (ddr3_axi_wdata ),
    .m_axi_wstrb             (ddr3_axi_wstrb ),
    .m_axi_wlast             (ddr3_axi_wlast ),
    .m_axi_wuser             (),
    .m_axi_wvalid            (ddr3_axi_wvalid),
    .m_axi_wready            (ddr3_axi_wready),
    // Slave Interface Write Response Ports
    .m_axi_bid               (ddr3_axi_bid   ),
    .m_axi_bresp             (ddr3_axi_bresp ),
    .m_axi_buser             (),
    .m_axi_bvalid            (ddr3_axi_bvalid),
    .m_axi_bready            (ddr3_axi_bready),
    // Slave Interface Read Address Ports
    .m_axi_arid              (ddr3_axi_arid   ),
    .m_axi_araddr            (ddr3_axi_araddr ),
    .m_axi_arlen             (ddr3_axi_arlen  ),
    .m_axi_arsize            (ddr3_axi_arsize ),
    .m_axi_arburst           (ddr3_axi_arburst),
    .m_axi_arlock            (ddr3_axi_arlock ),
    .m_axi_arcache           (ddr3_axi_arcache),
    .m_axi_arprot            (ddr3_axi_arprot ),
    .m_axi_arqos             (ddr3_axi_arqos  ),
    .m_axi_arregion          (),
    .m_axi_aruser            (),
    .m_axi_arvalid           (ddr3_axi_arvalid),
    .m_axi_arready           (ddr3_axi_arready),
    // Slave Interface Read Data Ports
    .m_axi_rid               (ddr3_axi_rid   ),
    .m_axi_rdata             (ddr3_axi_rdata ),
    .m_axi_rresp             (ddr3_axi_rresp ),
    .m_axi_rlast             (ddr3_axi_rlast ),
    .m_axi_ruser             (),
    .m_axi_rvalid            (ddr3_axi_rvalid),
    .m_axi_rready            (ddr3_axi_rready),
    .s_eth0_tdata            (e2v0_tdata ),
    .s_eth0_tlast            (e2v0_tlast ),
    .s_eth0_tvalid           (e2v0_tvalid),
    .s_eth0_tready           (e2v0_tready),
    .m_eth0_tdata            (v2e0_tdata ),
    .m_eth0_tlast            (v2e0_tlast ),
    .m_eth0_tvalid           (v2e0_tvalid),
    .m_eth0_tready           (v2e0_tready),
    .s_eth1_tdata            (e2v1_tdata ),
    .s_eth1_tlast            (e2v1_tlast ),
    .s_eth1_tvalid           (e2v1_tvalid),
    .s_eth1_tready           (e2v1_tready),
    .m_eth1_tdata            (v2e1_tdata ),
    .m_eth1_tlast            (v2e1_tlast ),
    .m_eth1_tvalid           (v2e1_tvalid),
    .m_eth1_tready           (v2e1_tready),

    .s_pcie_tdata            (p2c_tdata ),
    .s_pcie_tlast            (p2c_tlast ),
    .s_pcie_tvalid           (p2c_tvalid),
    .s_pcie_tready           (p2c_tready),
    .m_pcie_tdata            (c2p_tdata ),
    .m_pcie_tlast            (c2p_tlast ),
    .m_pcie_tvalid           (c2p_tvalid),
    .m_pcie_tready           (c2p_tready)
  );

  //---------------------------------------------------------------------------
  // Convert Control Port to Settings Bus
  //---------------------------------------------------------------------------

  localparam NUM_CTRLPORTS_PER_DBOARD = 1;

  ctrlport_to_settings_bus # (
    .NUM_PORTS (NUM_CTRLPORTS_PER_DBOARD),
    .USE_TIME  (1)
  ) ctrlport0_to_settings_bus_i (
    .ctrlport_clk             (radio_clk),
    .ctrlport_rst             (radio_rst),
    .s_ctrlport_req_wr        (m_ctrlport_req_wr_radio0),
    .s_ctrlport_req_rd        (m_ctrlport_req_rd_radio0),
    .s_ctrlport_req_addr      (m_ctrlport_req_addr_radio0),
    .s_ctrlport_req_data      (m_ctrlport_req_data_radio0),
    .s_ctrlport_req_has_time  (m_ctrlport_req_has_time_radio0),
    .s_ctrlport_req_time      (m_ctrlport_req_time_radio0),
    .s_ctrlport_resp_ack      (m_ctrlport_resp_ack_radio0),
    .s_ctrlport_resp_data     (m_ctrlport_resp_data_radio0),
    .set_data                 (db_fe_set_data[31:0]),
    .set_addr                 (db_fe_set_addr[7:0]),
    .set_stb                  (db_fe_set_stb[0]),
    .set_time                 (),
    .set_has_time             (),
    .rb_stb                   (db_fe_rb_stb[0]),
    .rb_addr                  (db_fe_rb_addr[7:0]),
    .rb_data                  (db_fe_rb_data[63:0]),
    .timestamp                (radio_time_tb)
  );

  ctrlport_to_settings_bus # (
    .NUM_PORTS (NUM_CTRLPORTS_PER_DBOARD),
    .USE_TIME  (1)
  ) ctrlport1_to_settings_bus_i (
    .ctrlport_clk             (radio_clk),
    .ctrlport_rst             (radio_rst),
    .s_ctrlport_req_wr        (m_ctrlport_req_wr_radio1),
    .s_ctrlport_req_rd        (m_ctrlport_req_rd_radio1),
    .s_ctrlport_req_addr      (m_ctrlport_req_addr_radio1),
    .s_ctrlport_req_data      (m_ctrlport_req_data_radio1),
    .s_ctrlport_req_has_time  (m_ctrlport_req_has_time_radio1),
    .s_ctrlport_req_time      (m_ctrlport_req_time_radio1),
    .s_ctrlport_resp_ack      (m_ctrlport_resp_ack_radio1),
    .s_ctrlport_resp_data     (m_ctrlport_resp_data_radio1),
    .set_data                 (db_fe_set_data[63:32]),
    .set_addr                 (db_fe_set_addr[15:8]),
    .set_stb                  (db_fe_set_stb[1]),
    .set_time                 (),
    .set_has_time             (),
    .rb_stb                   (db_fe_rb_stb[1]),
    .rb_addr                  (db_fe_rb_addr[15:8]),
    .rb_data                  (db_fe_rb_data[127:64]),
    .timestamp                (radio_time_tb)
  );

endmodule // bus_int

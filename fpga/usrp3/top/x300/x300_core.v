//
// Copyright 2014 Ettus Research LLC
// Copyright 2017 Ettus Research, a National Instruments Company
// Copyright 2020 Ettus Research, a National Instruments Brand
// Copyright 2023 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//

module x300_core #(
   parameter BUS_CLK_RATE = 32'd166666666
)(
   //Clocks and resets
   input radio_clk,
   input radio_rst,
   input bus_clk,
   input bus_rst,
   input ce_clk,
   input ce_rst,
   output [3:0] sw_rst,
   input bus_clk_div2,
   input bus_rst_div2,
   // Radio 0
   input [31:0] rx0, output [31:0] tx0,
   input [31:0] db0_gpio_in, output [31:0] db0_gpio_out, output [31:0] db0_gpio_ddr,
   input [31:0] fp_gpio_in, output reg [31:0] fp_gpio_out, output reg [31:0] fp_gpio_ddr,
   output [7:0] sen0, output sclk0, output mosi0, input miso0,
   output [2:0] radio_led0,
   output reg [31:0] radio0_misc_out, input [31:0] radio0_misc_in,
   // Radio 1
   input [31:0] rx1, output [31:0] tx1,
   input [31:0] db1_gpio_in, output [31:0] db1_gpio_out, output [31:0] db1_gpio_ddr,
   output [7:0] sen1, output sclk1, output mosi1, input miso1,
   output [2:0] radio_led1,
   output reg [31:0] radio1_misc_out, input [31:0] radio1_misc_in,
   // Radio shared misc
   inout db_scl,
   inout db_sda,
   // Clock control
   input ext_ref_clk,
   output [1:0] clock_ref_sel,
   output [1:0] clock_misc_opt,
   input [1:0] LMK_Status,
   input LMK_Holdover, input LMK_Lock, output LMK_Sync,
   output LMK_SEN, output LMK_SCLK, output LMK_MOSI,
   input [2:0] misc_clock_status,
   // SFP+ pins
   inout SFPP0_SCL,
   inout SFPP0_SDA,
   inout SFPP0_RS0,
   inout SFPP0_RS1,
   input SFPP0_ModAbs,
   input SFPP0_TxFault,
   input SFPP0_RxLOS,
   output SFPP0_TxDisable,   // Assert low to enable transmitter.

   inout SFPP1_SCL,
   inout SFPP1_SDA,
   inout SFPP1_RS0,
   inout SFPP1_RS1,
   input SFPP1_ModAbs,
   input SFPP1_TxFault,
   input SFPP1_RxLOS,
   output SFPP1_TxDisable,   // Assert low to enable transmitter.

   // SFP+ 0 data stream
   output [63:0] sfp0_tx_tdata,
   output [3:0] sfp0_tx_tuser,
   output sfp0_tx_tlast,
   output sfp0_tx_tvalid,
   input sfp0_tx_tready,

   input [63:0] sfp0_rx_tdata,
   input [3:0] sfp0_rx_tuser,
   input sfp0_rx_tlast,
   input sfp0_rx_tvalid,
   output sfp0_rx_tready,

   input [15:0] sfp0_phy_status,

   // SFP+ 1 data stream
   output [63:0] sfp1_tx_tdata,
   output [3:0] sfp1_tx_tuser,
   output sfp1_tx_tlast,
   output sfp1_tx_tvalid,
   input sfp1_tx_tready,

   input [63:0] sfp1_rx_tdata,
   input [3:0] sfp1_rx_tuser,
   input sfp1_rx_tlast,
   input sfp1_rx_tvalid,
   output sfp1_rx_tready,

   input [15:0] sfp1_phy_status,

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

   input [31:0] xadc_readback,

   // Time
   input pps,
   output [1:0] pps_select,
   output pps_out_enb,
   output [31:0] ref_freq,
   output ref_freq_changed,
   output gps_txd,
   input gps_rxd,
   // Debug UART
   input debug_rxd,
   output debug_txd,

   //
   // AXI4 (128b@250MHz) interface to DDR3 controller
   //
   input           ddr3_axi_clk,
   input           ddr3_axi_clk_x2,
   input           ddr3_axi_rst,
   // Write Address Ports
   output          ddr3_axi_awid,
   output  [31:0]  ddr3_axi_awaddr,
   output  [7:0]   ddr3_axi_awlen,
   output  [2:0]   ddr3_axi_awsize,
   output  [1:0]   ddr3_axi_awburst,
   output  [0:0]   ddr3_axi_awlock,
   output  [3:0]   ddr3_axi_awcache,
   output  [2:0]   ddr3_axi_awprot,
   output  [3:0]   ddr3_axi_awqos,
   output          ddr3_axi_awvalid,
   input           ddr3_axi_awready,
   // Write Data Ports
   output  [255:0] ddr3_axi_wdata,
   output  [31:0]  ddr3_axi_wstrb,
   output          ddr3_axi_wlast,
   output          ddr3_axi_wvalid,
   input           ddr3_axi_wready,
   // Write Response Ports
   output          ddr3_axi_bready,
   input           ddr3_axi_bid,
   input [1:0]     ddr3_axi_bresp,
   input           ddr3_axi_bvalid,
   // Read Address Ports
   output          ddr3_axi_arid,
   output  [31:0]  ddr3_axi_araddr,
   output  [7:0]   ddr3_axi_arlen,
   output  [2:0]   ddr3_axi_arsize,
   output  [1:0]   ddr3_axi_arburst,
   output  [0:0]   ddr3_axi_arlock,
   output  [3:0]   ddr3_axi_arcache,
   output  [2:0]   ddr3_axi_arprot,
   output  [3:0]   ddr3_axi_arqos,
   output          ddr3_axi_arvalid,
   input           ddr3_axi_arready,
   // Read Data Ports
   output          ddr3_axi_rready,
   input           ddr3_axi_rid,
   input [255:0]   ddr3_axi_rdata,
   input [1:0]     ddr3_axi_rresp,
   input           ddr3_axi_rlast,
   input           ddr3_axi_rvalid,

   //iop2 message fifos
   output [63:0] o_iop2_msg_tdata,
   output o_iop2_msg_tvalid,
   output o_iop2_msg_tlast,
   input o_iop2_msg_tready,
   input [63:0] i_iop2_msg_tdata,
   input i_iop2_msg_tvalid,
   input i_iop2_msg_tlast,
   output i_iop2_msg_tready,

   //PCIe
   output [63:0] pcio_tdata,
   output [2:0]  pcio_tuser,
   output        pcio_tlast,
   output        pcio_tvalid,
   input         pcio_tready,
   input [63:0]  pcii_tdata,
   input [2:0]   pcii_tuser,
   input         pcii_tlast,
   input         pcii_tvalid,
   output        pcii_tready,

   // Debug
   output [1:0] led_misc,
   output [31:0] debug0,
   output [31:0] debug1,
   output [127:0] debug2
);

   // Memory Controller AXI4 MM buses
   wire        s00_axi_awready, s01_axi_awready;
   wire [0:0]  s00_axi_awid, s01_axi_awid;
   wire [29:0] s00_axi_awaddr, s01_axi_awaddr;
   wire [7:0]  s00_axi_awlen, s01_axi_awlen;
   wire [2:0]  s00_axi_awsize, s01_axi_awsize;
   wire [1:0]  s00_axi_awburst, s01_axi_awburst;
   wire [0:0]  s00_axi_awlock, s01_axi_awlock;
   wire [3:0]  s00_axi_awcache, s01_axi_awcache;
   wire [2:0]  s00_axi_awprot, s01_axi_awprot;
   wire [3:0]  s00_axi_awqos, s01_axi_awqos;
   wire [3:0]  s00_axi_awregion, s01_axi_awregion;
   wire [0:0]  s00_axi_awuser, s01_axi_awuser;
   wire        s00_axi_wready, s01_axi_wready;
   wire [63:0] s00_axi_wdata, s01_axi_wdata;
   wire [7:0]  s00_axi_wstrb, s01_axi_wstrb;
   wire [0:0]  s00_axi_wuser, s01_axi_wuser;
   wire        s00_axi_bvalid, s01_axi_bvalid;
   wire [0:0]  s00_axi_bid, s01_axi_bid;
   wire [1:0]  s00_axi_bresp, s01_axi_bresp;
   wire [0:0]  s00_axi_buser, s01_axi_buser;
   wire        s00_axi_arready, s01_axi_arready;
   wire [0:0]  s00_axi_arid, s01_axi_arid;
   wire [29:0] s00_axi_araddr, s01_axi_araddr;
   wire [7:0]  s00_axi_arlen, s01_axi_arlen;
   wire [2:0]  s00_axi_arsize, s01_axi_arsize;
   wire [1:0]  s00_axi_arburst, s01_axi_arburst;
   wire [0:0]  s00_axi_arlock, s01_axi_arlock;
   wire [3:0]  s00_axi_arcache, s01_axi_arcache;
   wire [2:0]  s00_axi_arprot, s01_axi_arprot;
   wire [3:0]  s00_axi_arqos, s01_axi_arqos;
   wire [3:0]  s00_axi_arregion, s01_axi_arregion;
   wire [0:0]  s00_axi_aruser, s01_axi_aruser;
   wire        s00_axi_rlast, s01_axi_rlast;
   wire        s00_axi_rvalid, s01_axi_rvalid;
   wire        s00_axi_awvalid, s01_axi_awvalid;
   wire        s00_axi_wlast, s01_axi_wlast;
   wire        s00_axi_wvalid, s01_axi_wvalid;
   wire        s00_axi_bready, s01_axi_bready;
   wire        s00_axi_arvalid, s01_axi_arvalid;
   wire        s00_axi_rready, s01_axi_rready;
   wire [0:0]  s00_axi_rid, s01_axi_rid;
   wire [63:0] s00_axi_rdata, s01_axi_rdata;
   wire [1:0]  s00_axi_rresp, s01_axi_rresp;
   wire [0:0]  s00_axi_ruser, s01_axi_ruser;
   wire        time_sync;


   /////////////////////////////////////////////////////////////////////////////////
   // PPS synchronization logic
   /////////////////////////////////////////////////////////////////////////////////
   wire pps_rclk, pps_detect;
   pps_synchronizer pps_sync_inst (
      .ref_clk(ext_ref_clk), .timebase_clk(radio_clk),
      .pps_in(pps), .pps_out(pps_rclk), .pps_count(pps_detect)
   );

   /////////////////////////////////////////////////////////////////////////////////
   // Bus Int containing soft CPU control, routing fabric
   /////////////////////////////////////////////////////////////////////////////////

   // Number of Radio Cores Instantiated
   localparam NUM_RADIOS = 2;
   localparam NUM_DBOARDS = NUM_RADIOS;
   localparam NUM_CHANNELS_PER_RADIO = 2;
   localparam NUM_CHANNELS = NUM_CHANNELS_PER_RADIO * NUM_RADIOS;
   localparam NUM_CHANNELS_PER_DBOARD = NUM_CHANNELS_PER_RADIO;

   bus_int #(
      .NUM_RADIOS(NUM_RADIOS),
      .NUM_CHANNELS_PER_RADIO(NUM_CHANNELS_PER_RADIO),
      .NUM_CHANNELS(NUM_CHANNELS)
   ) bus_int_i (
      .clk(bus_clk), .clk_div2(bus_clk_div2), .reset(bus_rst), .reset_div2(bus_rst_div2),
      .sen(LMK_SEN), .sclk(LMK_SCLK), .mosi(LMK_MOSI), .miso(1'b0),
      .scl0(SFPP0_SCL), .sda0(SFPP0_SDA),
      .scl1(db_scl), .sda1(db_sda),
      .scl2(SFPP1_SCL), .sda2(SFPP1_SDA),
      .gps_txd(gps_txd), .gps_rxd(gps_rxd),
      .debug_txd(debug_txd), .debug_rxd(debug_rxd),
      .leds(led_misc), .sw_rst(sw_rst),
      // Timekeeper
      .pps(pps_rclk),
      .time_sync(time_sync),
      // Block connections
      .ce_clk     (ce_clk),
      .ce_rst     (ce_rst),

      // Radio connections
      .radio_clk  (radio_clk),
      .radio_rst  (radio_rst),
      .radio_rx_stb     ({     rx_stb[3],     rx_stb[2],     rx_stb[1],     rx_stb[0]}),
      .radio_rx_data    ({    rx_data[3],    rx_data[2],    rx_data[1],    rx_data[0]}),
      .radio_rx_running ({ rx_running[3], rx_running[2], rx_running[1], rx_running[0]}),
      .radio_tx_stb     ({     tx_stb[3],     tx_stb[2],     tx_stb[1],     tx_stb[0]}),
      .radio_tx_data    ({    tx_data[3],    tx_data[2],    tx_data[1],    tx_data[0]}),
      .radio_tx_running ({ tx_running[3], tx_running[2], tx_running[1], tx_running[0]}),
      // Daughter board settings buses
      .db_fe_set_stb ({ db_fe_set_stb[1],  db_fe_set_stb[0]}),
      .db_fe_set_addr({db_fe_set_addr[1], db_fe_set_addr[0]}),
      .db_fe_set_data({db_fe_set_data[1], db_fe_set_data[0]}),
      .db_fe_rb_stb  ({  db_fe_rb_stb[1],   db_fe_rb_stb[0]}),
      .db_fe_rb_addr ({ db_fe_rb_addr[1],  db_fe_rb_addr[0]}),
      .db_fe_rb_data ({ db_fe_rb_data[1],  db_fe_rb_data[0]}),
      // SFP 0
      .SFPP0_ModAbs(SFPP0_ModAbs),.SFPP0_TxFault(SFPP0_TxFault),.SFPP0_RxLOS(SFPP0_RxLOS),
      .SFPP0_RS0(SFPP0_RS0), .SFPP0_RS1(SFPP0_RS1),
      // SFP 1
      .SFPP1_ModAbs(SFPP1_ModAbs),.SFPP1_TxFault(SFPP1_TxFault),.SFPP1_RxLOS(SFPP1_RxLOS),
      .SFPP1_RS0(SFPP1_RS0), .SFPP1_RS1(SFPP1_RS1),
      // Front-panel GPIO source
      .fp_gpio_src(sr_fp_gpio_src),
      //clocky locky misc
      .clock_status({misc_clock_status, pps_detect, LMK_Holdover, LMK_Lock, LMK_Status}),
      .clock_control({1'b0, clock_misc_opt[1:0], pps_out_enb, pps_select[1:0], clock_ref_sel[1:0]}),
      .ref_freq(ref_freq), .ref_freq_changed(ref_freq_changed),
      // Eth0
      .sfp0_tx_tdata(sfp0_tx_tdata), .sfp0_tx_tuser(sfp0_tx_tuser), .sfp0_tx_tlast(sfp0_tx_tlast),
      .sfp0_tx_tvalid(sfp0_tx_tvalid), .sfp0_tx_tready(sfp0_tx_tready),
      .sfp0_rx_tdata(sfp0_rx_tdata), .sfp0_rx_tuser(sfp0_rx_tuser), .sfp0_rx_tlast(sfp0_rx_tlast),
      .sfp0_rx_tvalid(sfp0_rx_tvalid), .sfp0_rx_tready(sfp0_rx_tready),
      // Eth1
      .sfp1_tx_tdata(sfp1_tx_tdata), .sfp1_tx_tuser(sfp1_tx_tuser), .sfp1_tx_tlast(sfp1_tx_tlast),
      .sfp1_tx_tvalid(sfp1_tx_tvalid), .sfp1_tx_tready(sfp1_tx_tready),
      .sfp1_rx_tdata(sfp1_rx_tdata), .sfp1_rx_tuser(sfp1_rx_tuser), .sfp1_rx_tlast(sfp1_rx_tlast),
      .sfp1_rx_tvalid(sfp1_rx_tvalid), .sfp1_rx_tready(sfp1_rx_tready),
      // IoP2 Msgs
      .o_iop2_msg_tdata(o_iop2_msg_tdata), .o_iop2_msg_tvalid(o_iop2_msg_tvalid), .o_iop2_msg_tlast(o_iop2_msg_tlast), .o_iop2_msg_tready(o_iop2_msg_tready),
      .i_iop2_msg_tdata(i_iop2_msg_tdata), .i_iop2_msg_tvalid(i_iop2_msg_tvalid), .i_iop2_msg_tlast(i_iop2_msg_tlast), .i_iop2_msg_tready(i_iop2_msg_tready),
      // PCIe
      .pcio_tdata(pcio_tdata), .pcio_tuser(pcio_tuser), .pcio_tlast(pcio_tlast), .pcio_tvalid(pcio_tvalid), .pcio_tready(pcio_tready),
      .pcii_tdata(pcii_tdata), .pcii_tuser(pcii_tuser), .pcii_tlast(pcii_tlast), .pcii_tvalid(pcii_tvalid), .pcii_tready(pcii_tready),
      // Wishbone Slave Interface(s)
      .sfp0_wb_dat_i(sfp0_wb_dat_i), .sfp0_wb_dat_o(sfp0_wb_dat_o), .sfp0_wb_adr(sfp0_wb_adr),
      .sfp0_wb_sel(sfp0_wb_sel), .sfp0_wb_ack(sfp0_wb_ack), .sfp0_wb_stb(sfp0_wb_stb),
      .sfp0_wb_cyc(sfp0_wb_cyc), .sfp0_wb_we(sfp0_wb_we), .sfp0_wb_int(sfp0_wb_int),
      .sfp1_wb_dat_i(sfp1_wb_dat_i), .sfp1_wb_dat_o(sfp1_wb_dat_o), .sfp1_wb_adr(sfp1_wb_adr),
      .sfp1_wb_sel(sfp1_wb_sel), .sfp1_wb_ack(sfp1_wb_ack), .sfp1_wb_stb(sfp1_wb_stb),
      .sfp1_wb_cyc(sfp1_wb_cyc), .sfp1_wb_we(sfp1_wb_we), .sfp1_wb_int(sfp1_wb_int),
      //Status signals
      .sfp0_phy_status(sfp0_phy_status),
      .sfp1_phy_status(sfp1_phy_status),
      .xadc_readback(xadc_readback),

      // DRAM interface
      .ddr3_axi_clk_x2        (ddr3_axi_clk_x2),
      .ddr3_axi_rst           (ddr3_axi_rst),
      // Slave Interface Write Address Ports
      .ddr3_axi_awid          ({s01_axi_awid, s00_axi_awid}),
      .ddr3_axi_awaddr        ({s01_axi_awaddr, s00_axi_awaddr}),
      .ddr3_axi_awlen         ({s01_axi_awlen, s00_axi_awlen}),
      .ddr3_axi_awsize        ({s01_axi_awsize, s00_axi_awsize}),
      .ddr3_axi_awburst       ({s01_axi_awburst, s00_axi_awburst}),
      .ddr3_axi_awlock        ({s01_axi_awlock, s00_axi_awlock}),
      .ddr3_axi_awcache       ({s01_axi_awcache, s00_axi_awcache}),
      .ddr3_axi_awprot        ({s01_axi_awprot, s00_axi_awprot}),
      .ddr3_axi_awqos         ({s01_axi_awqos, s00_axi_awqos}),
      .ddr3_axi_awvalid       ({s01_axi_awvalid, s00_axi_awvalid}),
      .ddr3_axi_awready       ({s01_axi_awready, s00_axi_awready}),
      // Slave Interface Write Data Ports
      .ddr3_axi_wdata         ({s01_axi_wdata, s00_axi_wdata}),
      .ddr3_axi_wstrb         ({s01_axi_wstrb, s00_axi_wstrb}),
      .ddr3_axi_wlast         ({s01_axi_wlast, s00_axi_wlast}),
      .ddr3_axi_wvalid        ({s01_axi_wvalid, s00_axi_wvalid}),
      .ddr3_axi_wready        ({s01_axi_wready, s00_axi_wready}),
      // Slave Interface Write Response Ports
      .ddr3_axi_bid           ({s01_axi_bid, s00_axi_bid}),
      .ddr3_axi_bresp         ({s01_axi_bresp, s00_axi_bresp}),
      .ddr3_axi_bvalid        ({s01_axi_bvalid, s00_axi_bvalid}),
      .ddr3_axi_bready        ({s01_axi_bready, s00_axi_bready}),
      // Slave Interface Read Address Ports
      .ddr3_axi_arid          ({s01_axi_arid, s00_axi_arid}),
      .ddr3_axi_araddr        ({s01_axi_araddr, s00_axi_araddr}),
      .ddr3_axi_arlen         ({s01_axi_arlen, s00_axi_arlen}),
      .ddr3_axi_arsize        ({s01_axi_arsize, s00_axi_arsize}),
      .ddr3_axi_arburst       ({s01_axi_arburst, s00_axi_arburst}),
      .ddr3_axi_arlock        ({s01_axi_arlock, s00_axi_arlock}),
      .ddr3_axi_arcache       ({s01_axi_arcache, s00_axi_arcache}),
      .ddr3_axi_arprot        ({s01_axi_arprot, s00_axi_arprot}),
      .ddr3_axi_arqos         ({s01_axi_arqos, s00_axi_arqos}),
      .ddr3_axi_arvalid       ({s01_axi_arvalid, s00_axi_arvalid}),
      .ddr3_axi_arready       ({s01_axi_arready, s00_axi_arready}),
      // Slave Interface Read Data Ports
      .ddr3_axi_rid           ({s01_axi_rid, s00_axi_rid}),
      .ddr3_axi_rdata         ({s01_axi_rdata, s00_axi_rdata}),
      .ddr3_axi_rresp         ({s01_axi_rresp, s00_axi_rresp}),
      .ddr3_axi_rlast         ({s01_axi_rlast, s00_axi_rlast}),
      .ddr3_axi_rvalid        ({s01_axi_rvalid, s00_axi_rvalid}),
      .ddr3_axi_rready        ({s01_axi_rready, s00_axi_rready}),
      // Debug
      .debug0(debug0), .debug1(debug1), .debug2(debug2)
   );

   axi_intercon_2x64_128_bd_wrapper axi_intercon_2x64_128_bd_i (
      .S00_AXI_ACLK(ddr3_axi_clk_x2), // input S00_AXI_ACLK
      .S00_AXI_ARESETN(~ddr3_axi_rst), // input S00_AXI_ARESETN
      .S00_AXI_AWID(s00_axi_awid), // input [0 : 0] S00_AXI_AWID
      .S00_AXI_AWADDR({2'b0, s00_axi_awaddr}), // input [31 : 0] S00_AXI_AWADDR
      .S00_AXI_AWLEN(s00_axi_awlen), // input [7 : 0] S00_AXI_AWLEN
      .S00_AXI_AWSIZE(s00_axi_awsize), // input [2 : 0] S00_AXI_AWSIZE
      .S00_AXI_AWBURST(s00_axi_awburst), // input [1 : 0] S00_AXI_AWBURST
      .S00_AXI_AWLOCK(s00_axi_awlock), // input S00_AXI_AWLOCK
      .S00_AXI_AWCACHE(s00_axi_awcache), // input [3 : 0] S00_AXI_AWCACHE
      .S00_AXI_AWPROT(s00_axi_awprot), // input [2 : 0] S00_AXI_AWPROT
      .S00_AXI_AWQOS(s00_axi_awqos), // input [3 : 0] S00_AXI_AWQOS
      .S00_AXI_AWVALID(s00_axi_awvalid), // input S00_AXI_AWVALID
      .S00_AXI_AWREADY(s00_axi_awready), // output S00_AXI_AWREADY
      .S00_AXI_WDATA(s00_axi_wdata), // input [63 : 0] S00_AXI_WDATA
      .S00_AXI_WSTRB(s00_axi_wstrb), // input [7 : 0] S00_AXI_WSTRB
      .S00_AXI_WLAST(s00_axi_wlast), // input S00_AXI_WLAST
      .S00_AXI_WVALID(s00_axi_wvalid), // input S00_AXI_WVALID
      .S00_AXI_WREADY(s00_axi_wready), // output S00_AXI_WREADY
      .S00_AXI_BID(s00_axi_bid), // output [0 : 0] S00_AXI_BID
      .S00_AXI_BRESP(s00_axi_bresp), // output [1 : 0] S00_AXI_BRESP
      .S00_AXI_BVALID(s00_axi_bvalid), // output S00_AXI_BVALID
      .S00_AXI_BREADY(s00_axi_bready), // input S00_AXI_BREADY
      .S00_AXI_ARID(s00_axi_arid), // input [0 : 0] S00_AXI_ARID
      .S00_AXI_ARADDR({2'b0, s00_axi_araddr}), // input [31 : 0] S00_AXI_ARADDR
      .S00_AXI_ARLEN(s00_axi_arlen), // input [7 : 0] S00_AXI_ARLEN
      .S00_AXI_ARSIZE(s00_axi_arsize), // input [2 : 0] S00_AXI_ARSIZE
      .S00_AXI_ARBURST(s00_axi_arburst), // input [1 : 0] S00_AXI_ARBURST
      .S00_AXI_ARLOCK(s00_axi_arlock), // input S00_AXI_ARLOCK
      .S00_AXI_ARCACHE(s00_axi_arcache), // input [3 : 0] S00_AXI_ARCACHE
      .S00_AXI_ARPROT(s00_axi_arprot), // input [2 : 0] S00_AXI_ARPROT
      .S00_AXI_ARQOS(s00_axi_arqos), // input [3 : 0] S00_AXI_ARQOS
      .S00_AXI_ARVALID(s00_axi_arvalid), // input S00_AXI_ARVALID
      .S00_AXI_ARREADY(s00_axi_arready), // output S00_AXI_ARREADY
      .S00_AXI_RID(s00_axi_rid), // output [0 : 0] S00_AXI_RID
      .S00_AXI_RDATA(s00_axi_rdata), // output [63 : 0] S00_AXI_RDATA
      .S00_AXI_RRESP(s00_axi_rresp), // output [1 : 0] S00_AXI_RRESP
      .S00_AXI_RLAST(s00_axi_rlast), // output S00_AXI_RLAST
      .S00_AXI_RVALID(s00_axi_rvalid), // output S00_AXI_RVALID
      .S00_AXI_RREADY(s00_axi_rready), // input S00_AXI_RREADY
      //
      .S01_AXI_ACLK(ddr3_axi_clk_x2), // input S01_AXI_ACLK
      .S01_AXI_ARESETN(~ddr3_axi_rst), // input S00_AXI_ARESETN
      .S01_AXI_AWID(s01_axi_awid), // input [0 : 0] S01_AXI_AWID
      .S01_AXI_AWADDR({2'b0, s01_axi_awaddr}), // input [31 : 0] S01_AXI_AWADDR
      .S01_AXI_AWLEN(s01_axi_awlen), // input [7 : 0] S01_AXI_AWLEN
      .S01_AXI_AWSIZE(s01_axi_awsize), // input [2 : 0] S01_AXI_AWSIZE
      .S01_AXI_AWBURST(s01_axi_awburst), // input [1 : 0] S01_AXI_AWBURST
      .S01_AXI_AWLOCK(s01_axi_awlock), // input S01_AXI_AWLOCK
      .S01_AXI_AWCACHE(s01_axi_awcache), // input [3 : 0] S01_AXI_AWCACHE
      .S01_AXI_AWPROT(s01_axi_awprot), // input [2 : 0] S01_AXI_AWPROT
      .S01_AXI_AWQOS(s01_axi_awqos), // input [3 : 0] S01_AXI_AWQOS
      .S01_AXI_AWVALID(s01_axi_awvalid), // input S01_AXI_AWVALID
      .S01_AXI_AWREADY(s01_axi_awready), // output S01_AXI_AWREADY
      .S01_AXI_WDATA(s01_axi_wdata), // input [63 : 0] S01_AXI_WDATA
      .S01_AXI_WSTRB(s01_axi_wstrb), // input [7 : 0] S01_AXI_WSTRB
      .S01_AXI_WLAST(s01_axi_wlast), // input S01_AXI_WLAST
      .S01_AXI_WVALID(s01_axi_wvalid), // input S01_AXI_WVALID
      .S01_AXI_WREADY(s01_axi_wready), // output S01_AXI_WREADY
      .S01_AXI_BID(s01_axi_bid), // output [0 : 0] S01_AXI_BID
      .S01_AXI_BRESP(s01_axi_bresp), // output [1 : 0] S01_AXI_BRESP
      .S01_AXI_BVALID(s01_axi_bvalid), // output S01_AXI_BVALID
      .S01_AXI_BREADY(s01_axi_bready), // input S01_AXI_BREADY
      .S01_AXI_ARID(s01_axi_arid), // input [0 : 0] S01_AXI_ARID
      .S01_AXI_ARADDR({2'b0, s01_axi_araddr}), // input [31 : 0] S01_AXI_ARADDR
      .S01_AXI_ARLEN(s01_axi_arlen), // input [7 : 0] S01_AXI_ARLEN
      .S01_AXI_ARSIZE(s01_axi_arsize), // input [2 : 0] S01_AXI_ARSIZE
      .S01_AXI_ARBURST(s01_axi_arburst), // input [1 : 0] S01_AXI_ARBURST
      .S01_AXI_ARLOCK(s01_axi_arlock), // input S01_AXI_ARLOCK
      .S01_AXI_ARCACHE(s01_axi_arcache), // input [3 : 0] S01_AXI_ARCACHE
      .S01_AXI_ARPROT(s01_axi_arprot), // input [2 : 0] S01_AXI_ARPROT
      .S01_AXI_ARQOS(s01_axi_arqos), // input [3 : 0] S01_AXI_ARQOS
      .S01_AXI_ARVALID(s01_axi_arvalid), // input S01_AXI_ARVALID
      .S01_AXI_ARREADY(s01_axi_arready), // output S01_AXI_ARREADY
      .S01_AXI_RID(s01_axi_rid), // output [0 : 0] S01_AXI_RID
      .S01_AXI_RDATA(s01_axi_rdata), // output [63 : 0] S01_AXI_RDATA
      .S01_AXI_RRESP(s01_axi_rresp), // output [1 : 0] S01_AXI_RRESP
      .S01_AXI_RLAST(s01_axi_rlast), // output S01_AXI_RLAST
      .S01_AXI_RVALID(s01_axi_rvalid), // output S01_AXI_RVALID
      .S01_AXI_RREADY(s01_axi_rready), // input S01_AXI_RREADY
      //
      .M00_AXI_ACLK(ddr3_axi_clk), // input M00_AXI_ACLK
      .M00_AXI_ARESETN(~ddr3_axi_rst), // input S00_AXI_ARESETN
      .M00_AXI_AWID(ddr3_axi_awid), // output [3 : 0] M00_AXI_AWID
      .M00_AXI_AWADDR(ddr3_axi_awaddr), // output [31 : 0] M00_AXI_AWADDR
      .M00_AXI_AWLEN(ddr3_axi_awlen), // output [7 : 0] M00_AXI_AWLEN
      .M00_AXI_AWSIZE(ddr3_axi_awsize), // output [2 : 0] M00_AXI_AWSIZE
      .M00_AXI_AWBURST(ddr3_axi_awburst), // output [1 : 0] M00_AXI_AWBURST
      .M00_AXI_AWLOCK(ddr3_axi_awlock), // output M00_AXI_AWLOCK
      .M00_AXI_AWCACHE(ddr3_axi_awcache), // output [3 : 0] M00_AXI_AWCACHE
      .M00_AXI_AWPROT(ddr3_axi_awprot), // output [2 : 0] M00_AXI_AWPROT
      .M00_AXI_AWQOS(ddr3_axi_awqos), // output [3 : 0] M00_AXI_AWQOS
      .M00_AXI_AWVALID(ddr3_axi_awvalid), // output M00_AXI_AWVALID
      .M00_AXI_AWREADY(ddr3_axi_awready), // input M00_AXI_AWREADY
      .M00_AXI_WDATA(ddr3_axi_wdata), // output [255 : 0] M00_AXI_WDATA
      .M00_AXI_WSTRB(ddr3_axi_wstrb), // output [15 : 0] M00_AXI_WSTRB
      .M00_AXI_WLAST(ddr3_axi_wlast), // output M00_AXI_WLAST
      .M00_AXI_WVALID(ddr3_axi_wvalid), // output M00_AXI_WVALID
      .M00_AXI_WREADY(ddr3_axi_wready), // input M00_AXI_WREADY
      .M00_AXI_BID(ddr3_axi_bid), // input [3 : 0] M00_AXI_BID
      .M00_AXI_BRESP(ddr3_axi_bresp), // input [1 : 0] M00_AXI_BRESP
      .M00_AXI_BVALID(ddr3_axi_bvalid), // input M00_AXI_BVALID
      .M00_AXI_BREADY(ddr3_axi_bready), // output M00_AXI_BREADY
      .M00_AXI_ARID(ddr3_axi_arid), // output [3 : 0] M00_AXI_ARID
      .M00_AXI_ARADDR(ddr3_axi_araddr), // output [31 : 0] M00_AXI_ARADDR
      .M00_AXI_ARLEN(ddr3_axi_arlen), // output [7 : 0] M00_AXI_ARLEN
      .M00_AXI_ARSIZE(ddr3_axi_arsize), // output [2 : 0] M00_AXI_ARSIZE
      .M00_AXI_ARBURST(ddr3_axi_arburst), // output [1 : 0] M00_AXI_ARBURST
      .M00_AXI_ARLOCK(ddr3_axi_arlock), // output M00_AXI_ARLOCK
      .M00_AXI_ARCACHE(ddr3_axi_arcache), // output [3 : 0] M00_AXI_ARCACHE
      .M00_AXI_ARPROT(ddr3_axi_arprot), // output [2 : 0] M00_AXI_ARPROT
      .M00_AXI_ARQOS(ddr3_axi_arqos), // output [3 : 0] M00_AXI_ARQOS
      .M00_AXI_ARVALID(ddr3_axi_arvalid), // output M00_AXI_ARVALID
      .M00_AXI_ARREADY(ddr3_axi_arready), // input M00_AXI_ARREADY
      .M00_AXI_RID(ddr3_axi_rid), // input [3 : 0] M00_AXI_RID
      .M00_AXI_RDATA(ddr3_axi_rdata), // input [255 : 0] M00_AXI_RDATA
      .M00_AXI_RRESP(ddr3_axi_rresp), // input [1 : 0] M00_AXI_RRESP
      .M00_AXI_RLAST(ddr3_axi_rlast), // input M00_AXI_RLAST
      .M00_AXI_RVALID(ddr3_axi_rvalid), // input M00_AXI_RVALID
      .M00_AXI_RREADY(ddr3_axi_rready) // output M00_AXI_RREADY
   );



   /////////////////////////////////////////////////////////////////////////////////////////////
   //
   // Radios
   //
   /////////////////////////////////////////////////////////////////////////////////////////////

   // Daughter board I/O
   wire [31:0] leds[0:NUM_DBOARDS-1];
   wire [31:0] fp_gpio_r_in[0:NUM_DBOARDS-1], fp_gpio_r_out[0:NUM_DBOARDS-1], fp_gpio_r_ddr[0:NUM_DBOARDS-1];
   wire [31:0] db_gpio_in[0:NUM_DBOARDS-1], db_gpio_out[0:NUM_DBOARDS-1], db_gpio_ddr[0:NUM_DBOARDS-1];
   wire [31:0] misc_outs[0:NUM_DBOARDS-1];
   reg  [31:0] misc_ins[0:NUM_DBOARDS-1];
   wire [24:0] sr_fp_gpio_src, fp_gpio_src;
   wire [7:0]  sen[0:NUM_DBOARDS-1];
   wire        sclk[0:NUM_DBOARDS-1], mosi[0:NUM_DBOARDS-1], miso[0:NUM_DBOARDS-1];
   wire        rx_running[0:NUM_CHANNELS-1], tx_running[0:NUM_CHANNELS-1];
   wire [63:0] rx_data_in_r[0:NUM_DBOARDS-1], rx_data_r[0:NUM_DBOARDS-1];
   wire [63:0] tx_data_r[0:NUM_DBOARDS-1], tx_data_out_r[0:NUM_DBOARDS-1];
   wire [1:0]  rx_stb_r[0:NUM_DBOARDS-1], tx_stb_r[0:NUM_DBOARDS-1];


   // Data
    wire [31:0] rx_data_in[0:NUM_CHANNELS-1], rx_data[0:NUM_CHANNELS-1];
    wire [31:0] tx_data[0:NUM_CHANNELS-1], tx_data_out[0:NUM_CHANNELS-1];
    wire        rx_stb[0:NUM_CHANNELS-1], tx_stb[0:NUM_CHANNELS-1];
    wire        db_fe_set_stb[0:NUM_DBOARDS-1];
    wire [7:0]  db_fe_set_addr[0:NUM_DBOARDS-1];
    wire [31:0] db_fe_set_data[0:NUM_DBOARDS-1];
    wire        db_fe_rb_stb[0:NUM_DBOARDS-1];
    wire [7:0]  db_fe_rb_addr[0:NUM_DBOARDS-1];
    wire [63:0] db_fe_rb_data[0:NUM_DBOARDS-1];


   //------------------------------------
   // Daughterboard Control
   // -----------------------------------
   // Note: We have one db_control per slot, even though we can have up to 2
   // channels. In practice, this only affects TwinRX (and maybe some of the
   // Basic- and LF-boards, depending how they're used).
   // The main disadvantage in these cases is that individual channels cannot
   // individually trigger GPIOs based on ATR state (i.e., we cannot tell if
   // channel 0 or 1 are in a TX/RX state, we can only say that one of them is
   // in such a state).

   localparam [7:0] SR_DB_BASE = 8'd160;
   localparam [7:0] RB_DB_BASE = 8'd16;

   genvar i;
   generate for (i = 0; i < NUM_DBOARDS; i = i + 1)
     begin
       db_control #(
         .USE_SPI_CLK(0),
         .SR_BASE(SR_DB_BASE),
         .RB_BASE(RB_DB_BASE)
       ) db_control_i (
         .clk(radio_clk), .reset(radio_rst),
         .set_stb(db_fe_set_stb[i]), .set_addr(db_fe_set_addr[i]), .set_data(db_fe_set_data[i]),
         .rb_stb(db_fe_rb_stb[i]), .rb_addr(db_fe_rb_addr[i]), .rb_data(db_fe_rb_data[i]),
         // We OR the ATR indicators from both channels
         .run_rx(|{rx_running[i*2], rx_running[i*2+1]}),
         .run_tx(|{tx_running[i*2], tx_running[i*2+1]}),
         .misc_ins(misc_ins[i]), .misc_outs(misc_outs[i]),
         .fp_gpio_in(fp_gpio_r_in[i]), .fp_gpio_out(fp_gpio_r_out[i]), .fp_gpio_ddr(fp_gpio_r_ddr[i]), .fp_gpio_fab(),
         .db_gpio_in(db_gpio_in[i]), .db_gpio_out(db_gpio_out[i]), .db_gpio_ddr(db_gpio_ddr[i]), .db_gpio_fab(),
         .leds(leds[i]),
         .spi_clk(radio_clk), .spi_rst(radio_rst), .sen(sen[i]), .sclk(sclk[i]), .mosi(mosi[i]), .miso(miso[i])
       );
     end
   endgenerate

   //------------------------------------
   // Front End Control
   // -----------------------------------

   localparam [7:0] SR_FE_CHAN_OFFSET  = 8'd16;
   localparam [7:0] SR_TX_FE_BASE      = SR_DB_BASE + 8'd48;
   localparam [7:0] SR_RX_FE_BASE      = SR_DB_BASE + 8'd64;

   generate for (i = 0; i < NUM_DBOARDS; i = i + 1)
     begin
       fe_control #(
         .NUM_CHANNELS(NUM_CHANNELS_PER_DBOARD),
         .SR_FE_CHAN_OFFSET(SR_FE_CHAN_OFFSET),
         .SR_TX_FE_BASE(SR_TX_FE_BASE),
         .SR_RX_FE_BASE(SR_RX_FE_BASE),
         .BYPASS_HETERODYNE(0)
       ) x300_fe_core_i (
         .clk(radio_clk), .reset(radio_rst),
         .set_stb(db_fe_set_stb[i]), .set_addr(db_fe_set_addr[i]), .set_data(db_fe_set_data[i]),
         .time_sync(time_sync),
         .tx_stb(tx_stb_r[i]), .tx_data_in(tx_data_r[i]), .tx_data_out(tx_data_out_r[i]),
         .rx_stb(rx_stb_r[i]), .rx_data_in(rx_data_in_r[i]), .rx_data_out(rx_data_r[i])
       );
     end
   endgenerate

   //------------------------------------
   // Front-Panel GPIO Source Mux
   //------------------------------------
   // Number of FP GPIO Pins
   localparam FP_GPIO_WIDTH = 12;

   // Bring FP GPIO controls into radio clk domain
   synchronizer #(.WIDTH(24)) fp_gpio_sync (
      .clk(radio_clk), .rst(radio_rst), .in(sr_fp_gpio_src), .out(fp_gpio_src)
   );

   // For each bit in the front-panel GPIO, mux the output and the direction
   // control bit based on the fp_gpio_src register. The fp_gpio_src register
   // holds 2 bits per GPIO pin, which selects which source to use for GPIO
   // control. Currently, only daughter board 0 and daughter board 1 are
   // supported.
   for (i=0; i<FP_GPIO_WIDTH; i=i+1) begin : gen_fp_gpio_mux
      always @(posedge radio_clk) begin
         fp_gpio_out[i] <= fp_gpio_r_out[fp_gpio_src[2*i +: 2] == 0 ? 0 : 1][i];
         fp_gpio_ddr[i] <= fp_gpio_r_ddr[fp_gpio_src[2*i +: 2] == 0 ? 0 : 1][i];
      end
   end

   // Front-panel GPIO inputs are routed to all daughter boards
   for (i=0; i<NUM_DBOARDS; i=i+1) begin : gen_fp_gpio_inputs
      assign fp_gpio_r_in[i] = fp_gpio_in;
   end

   //------------------------------------
   // Radio to ADC,DAC and IO Mapping
   //------------------------------------

   // Data
   assign tx_data_r[0][31:0]  = tx_data[0];
   assign tx_data_r[0][63:32] = tx_data[1];
   assign tx_data_r[1][31:0]  = tx_data[2];
   assign tx_data_r[1][63:32] = tx_data[3];

   assign tx_data_out[0] = tx_data_out_r[0][31:0] ;
   assign tx_data_out[1] = tx_data_out_r[0][63:32];
   assign tx_data_out[2] = tx_data_out_r[1][31:0] ;
   assign tx_data_out[3] = tx_data_out_r[1][63:32];

   assign tx_stb_r[0][0] = tx_stb[0];
   assign tx_stb_r[0][1] = tx_stb[1];
   assign tx_stb_r[1][0] = tx_stb[2];
   assign tx_stb_r[1][1] = tx_stb[3];

   assign rx_data_in_r[0][31:0]  = rx_data_in[0];
   assign rx_data_in_r[0][63:32] = rx_data_in[1];
   assign rx_data_in_r[1][31:0]  = rx_data_in[2];
   assign rx_data_in_r[1][63:32] = rx_data_in[3];

   assign rx_data[0] = rx_data_r[0][31:0] ;
   assign rx_data[1] = rx_data_r[0][63:32];
   assign rx_data[2] = rx_data_r[1][31:0] ;
   assign rx_data[3] = rx_data_r[1][63:32];

   assign rx_stb[0] = rx_stb_r[0][0];
   assign rx_stb[1] = rx_stb_r[0][1];
   assign rx_stb[2] = rx_stb_r[1][0];
   assign rx_stb[3] = rx_stb_r[1][1];

   assign {rx_data_in[1], rx_data_in[0]} = {rx0, rx0};
   assign {rx_data_in[3], rx_data_in[2]} = {rx1, rx1};

   assign tx0 = tx_data_out[0];   //tx_data_out[1] unused
   assign tx1 = tx_data_out[2];   //tx_data_out[3] unused
   assign tx_stb[0] = 1'b1;
   assign tx_stb[1] = 1'b0;
   assign tx_stb[2] = 1'b1;
   assign tx_stb[3] = 1'b0;

   //Daughter board GPIO
   assign {db_gpio_in[1], db_gpio_in[0]} = {db1_gpio_in, db0_gpio_in};
   assign db0_gpio_out = db_gpio_out[0];
   assign db1_gpio_out = db_gpio_out[1];
   assign db0_gpio_ddr = db_gpio_ddr[0];
   assign db1_gpio_ddr = db_gpio_ddr[1];

   //SPI
   assign {sen0, sclk0, mosi0} = {sen[0], sclk[0], mosi[0]};
   assign miso[0] = miso0;
   assign {sen1, sclk1, mosi1} = {sen[1], sclk[1], mosi[1]};
   assign miso[1] = miso1;

   //LEDs
   // Reminder: radio_ledX = {RX, TXRX_TX, TXRX_RX}
   assign radio_led0 = leds[0][2:0];
   assign radio_led1 = leds[1][2:0];

   //Misc ins and outs
   always @(posedge radio_clk) begin
      radio0_misc_out   <= misc_outs[0];
      radio1_misc_out   <= misc_outs[1];
      misc_ins[0]       <= radio0_misc_in;
      misc_ins[1]       <= radio1_misc_in;
   end

endmodule // x300_core

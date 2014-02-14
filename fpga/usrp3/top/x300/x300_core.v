
module x300_core
  (
   input radio_clk,
   input radio_rst,
   input bus_clk,
   input bus_rst,
   output [3:0] sw_rst,
   // Radio 0
   inout [31:0] fp_gpio,
   input [31:0] rx0,
   output [31:0] tx0,
   inout [31:0] db_gpio0,
   output [7:0] sen0,
   output sclk0,
   output mosi0,
   input miso0,
   output [2:0] radio_led0,
   output reg [7:0] radio_misc0,
   output sync_dacs_radio0,
   // Radio 1
   input [31:0] rx1,
   output [31:0] tx1,
   inout [31:0] db_gpio1,
   output [7:0] sen1,
   output sclk1,
   output mosi1,
   input miso1,
   output [2:0] radio_led1,
   output reg [7:0] radio_misc1,
   output sync_dacs_radio1,
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
   // MDIO
   output mdc0,
   output mdio_in0,
   input mdio_out0,
   output mdc1,
   output mdio_in1,
   input mdio_out1,

`ifdef ETH10G_PORT0
   // XGMII
   input xgmii_clk0,
   output [63:0] xgmii_txd0,
   output [7:0]  xgmii_txc0,
   input [63:0] xgmii_rxd0,
   input [7:0]  xgmii_rxc0,
   input xge_phy_resetdone0,
`else
   // GMII
   input gmii_clk0,
   output [7:0] gmii_txd0,
   output gmii_tx_en0,
   output gmii_tx_er0,
   input [7:0] gmii_rxd0,
   input gmii_rx_dv0,
   input gmii_rx_er0,
`endif // !`ifdef

`ifdef ETH10G_PORT1
   // XGMII
   input xgmii_clk1,
   output [63:0] xgmii_txd1,
   output [7:0]  xgmii_txc1,
   input [63:0] xgmii_rxd1,
   input [7:0]  xgmii_rxc1,
   input xge_phy_resetdone1,
`else
   // GMII
   input gmii_clk1,
   output [7:0] gmii_txd1,
   output gmii_tx_en1,
   output gmii_tx_er1,
   input [7:0] gmii_rxd1,
   input gmii_rx_dv1,
   input gmii_rx_er1,
`endif // !`ifdef

   // Time
   input gps_pps,
   input ext_pps,
   output reg pps_out,
   output gps_txd,
   input gps_rxd,
   // Debug UART
   input debug_rxd,
   output debug_txd,

   //
   // AXI4 (128b@250MHz) interface to DDR3 controller
   //
   `ifndef NO_DRAM_FIFOS
   input           ddr3_axi_clk,
   input           ddr3_axi_rst,
   input           ddr3_running,
   // Write Address Ports
   output  [3:0]   ddr3_axi_awid,
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
   output  [127:0] ddr3_axi_wdata,
   output  [15:0]  ddr3_axi_wstrb,
   output          ddr3_axi_wlast,
   output          ddr3_axi_wvalid,
   input           ddr3_axi_wready,
   // Write Response Ports
   output          ddr3_axi_bready,
   input [3:0]     ddr3_axi_bid,
   input [1:0]     ddr3_axi_bresp,
   input           ddr3_axi_bvalid,
   // Read Address Ports
   output  [3:0]   ddr3_axi_arid,
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
   input [3:0]     ddr3_axi_rid,
   input [127:0]   ddr3_axi_rdata,
   input [1:0]     ddr3_axi_rresp,
   input           ddr3_axi_rlast,
   input           ddr3_axi_rvalid,
   `endif //  `ifndef NO_DRAM_FIFOS

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
   output pcio_tlast,
   output pcio_tvalid,
   input pcio_tready,
   input [63:0] pcii_tdata,
   input pcii_tlast,
   input pcii_tvalid,
   output pcii_tready,

   // Debug
   output [7:0] led_misc,
   output [31:0] debug0,
   output [31:0] debug1,
   output [127:0] debug2
   );

   wire [63:0] eth0_rx_tdata, eth0_tx_tdata;
   wire [3:0]  eth0_rx_tuser, eth0_tx_tuser;
   wire        eth0_rx_tlast, eth0_tx_tlast, eth0_rx_tvalid, eth0_tx_tvalid, eth0_rx_tready, eth0_tx_tready;

   wire [63:0] eth1_rx_tdata, eth1_tx_tdata;
   wire [3:0]  eth1_rx_tuser, eth1_tx_tuser;
   wire        eth1_rx_tlast, eth1_tx_tlast, eth1_rx_tvalid, eth1_tx_tvalid, eth1_rx_tready, eth1_tx_tready;

   wire [63:0] r0i_tdata, r0o_tdata, r1i_tdata, r1o_tdata;
   wire        r0i_tlast, r0o_tlast, r1i_tlast, r1o_tlast;
   wire        r0i_tvalid, r0o_tvalid, r1i_tvalid, r1o_tvalid;
   wire        r0i_tready, r0o_tready, r1i_tready, r1o_tready;

   wire [63:0] r0_tx_tdata_bi, r0_tx_tdata_bo, r1_tx_tdata_bi, r1_tx_tdata_bo;
   wire        r0_tx_tlast_bi, r0_tx_tlast_bo, r1_tx_tlast_bi, r1_tx_tlast_bo;
   wire        r0_tx_tvalid_bi, r0_tx_tvalid_bo, r1_tx_tvalid_bi, r1_tx_tvalid_bo;
   wire        r0_tx_tready_bi, r0_tx_tready_bo, r1_tx_tready_bi, r1_tx_tready_bo;

   wire [63:0] ce0i_tdata, ce0o_tdata, ce1i_tdata, ce1o_tdata, ce2i_tdata, ce2o_tdata;
   wire        ce0i_tlast, ce0o_tlast, ce1i_tlast, ce1o_tlast, ce2i_tlast, ce2o_tlast;
   wire        ce0i_tvalid, ce0o_tvalid, ce1i_tvalid, ce1o_tvalid, ce2i_tvalid, ce2o_tvalid;
   wire        ce0i_tready, ce0o_tready, ce1i_tready, ce1o_tready, ce2i_tready, ce2o_tready;

   // Radio Misc outputs before pipeline.
   wire [7:0]  misc_outs0, misc_outs1;

   // assign eth1_tx_tready = 1'b1;
   //  assign eth1_rx_tvalid = 1'b0;

`ifndef NO_DRAM_FIFOS
   //////////////////////////////////////////////////////////
   //
   // AXI BUS declarations.
   //
   //////////////////////////////////////////////////////////
   //
   // AXI4 MM bus 0
   wire 	 s00_axi_awready;
   wire 	 s00_axi_wready;
   wire 	 s00_axi_bvalid;
   wire 	 s00_axi_arready;
   wire 	 s00_axi_rlast;
   wire 	 s00_axi_rvalid;
   wire 	 s00_axi_awvalid;
   wire 	 s00_axi_wlast;
   wire 	 s00_axi_wvalid;
   wire 	 s00_axi_bready;
   wire 	 s00_axi_arvalid;
   wire 	 s00_axi_rready;
   wire [0 : 0]  s00_axi_bid;
   wire [1 : 0]  s00_axi_bresp;
   wire [0 : 0]  s00_axi_buser;
   wire [0 : 0]  s00_axi_rid;
   wire [63 : 0] s00_axi_rdata;
   wire [1 : 0]  s00_axi_rresp;
   wire [0 : 0]  s00_axi_ruser;
   wire [0 : 0]  s00_axi_awid;
   wire [31 : 0] s00_axi_awaddr;
   wire [7 : 0]  s00_axi_awlen;
   wire [2 : 0]  s00_axi_awsize;
   wire [1 : 0]  s00_axi_awburst;
   wire [0 : 0]  s00_axi_awlock;
   wire [3 : 0]  s00_axi_awcache;
   wire [2 : 0]  s00_axi_awprot;
   wire [3 : 0]  s00_axi_awqos;
   wire [3 : 0]  s00_axi_awregion;
   wire [0 : 0]  s00_axi_awuser;
   wire [63 : 0] s00_axi_wdata;
   wire [7 : 0]  s00_axi_wstrb;
   wire [0 : 0]  s00_axi_wuser;
   wire [0 : 0]  s00_axi_arid;
   wire [31 : 0] s00_axi_araddr;
   wire [7 : 0]  s00_axi_arlen;
   wire [2 : 0]  s00_axi_arsize;
   wire [1 : 0]  s00_axi_arburst;
   wire [0 : 0]  s00_axi_arlock;
   wire [3 : 0]  s00_axi_arcache;
   wire [2 : 0]  s00_axi_arprot;
   wire [3 : 0]  s00_axi_arqos;
   wire [3 : 0]  s00_axi_arregion;
   wire [0 : 0]  s00_axi_aruser;

   // AXI4 MM bus 1
   wire 	 s01_axi_awready;
   wire 	 s01_axi_wready;
   wire 	 s01_axi_bvalid;
   wire 	 s01_axi_arready;
   wire 	 s01_axi_rlast;
   wire 	 s01_axi_rvalid;
   wire 	 s01_axi_awvalid;
   wire 	 s01_axi_wlast;
   wire 	 s01_axi_wvalid;
   wire 	 s01_axi_bready;
   wire 	 s01_axi_arvalid;
   wire 	 s01_axi_rready;
   wire [0 : 0]  s01_axi_bid;
   wire [1 : 0]  s01_axi_bresp;
   wire [0 : 0]  s01_axi_buser;
   wire [0 : 0]  s01_axi_rid;
   wire [63 : 0] s01_axi_rdata;
   wire [1 : 0]  s01_axi_rresp;
   wire [0 : 0]  s01_axi_ruser;
   wire [0 : 0]  s01_axi_awid;
   wire [31 : 0] s01_axi_awaddr;
   wire [7 : 0]  s01_axi_awlen;
   wire [2 : 0]  s01_axi_awsize;
   wire [1 : 0]  s01_axi_awburst;
   wire [0 : 0]  s01_axi_awlock;
   wire [3 : 0]  s01_axi_awcache;
   wire [2 : 0]  s01_axi_awprot;
   wire [3 : 0]  s01_axi_awqos;
   wire [3 : 0]  s01_axi_awregion;
   wire [0 : 0]  s01_axi_awuser;
   wire [63 : 0] s01_axi_wdata;
   wire [7 : 0]  s01_axi_wstrb;
   wire [0 : 0]  s01_axi_wuser;
   wire [0 : 0]  s01_axi_arid;
   wire [31 : 0] s01_axi_araddr;
   wire [7 : 0]  s01_axi_arlen;
   wire [2 : 0]  s01_axi_arsize;
   wire [1 : 0]  s01_axi_arburst;
   wire [0 : 0]  s01_axi_arlock;
   wire [3 : 0]  s01_axi_arcache;
   wire [2 : 0]  s01_axi_arprot;
   wire [3 : 0]  s01_axi_arqos;
   wire [3 : 0]  s01_axi_arregion;
   wire [0 : 0]  s01_axi_aruser;

`endif //  `ifndef NO_DRAM_FIFOS


   //------------------------------------------------------------------
   // Wishbone Slave Interface(s)
   //------------------------------------------------------------------
   localparam  dw  = 32;  // Data bus width
   localparam  aw  = 16;  // Address bus width, for byte addressibility, 16 = 64K byte memory space
   localparam  sw  = 4;   // Select width -- 32-bit data bus with 8-bit granularity.

   wire [dw-1:0] s4_dat_i;
   wire [dw-1:0] s4_dat_o;
   wire [aw-1:0] s4_adr;
   wire [sw-1:0] s4_sel;
   wire 	 s4_ack;
   wire 	 s4_stb;
   wire 	 s4_cyc;
   wire 	 s4_we;
   wire 	 s4_int;

   wire [dw-1:0] s5_dat_i;
   wire [dw-1:0] s5_dat_o;
   wire [aw-1:0] s5_adr;
   wire [sw-1:0] s5_sel;
   wire 	 s5_ack;
   wire 	 s5_stb;
   wire 	 s5_cyc;
   wire 	 s5_we;
   wire 	 s5_int;

   // AXI_DRAM_FIFO BIST. Not for production code.
   wire [63:0] 	 bist_i_tdata;
   wire 	 bist_i_tvalid;
   wire 	 bist_i_tready;
   wire 	 bist_i_tlast;

   wire [63:0] 	 bist_o_tdata;
   wire 	 bist_o_tvalid;
   wire 	 bist_o_tready;
   wire 	 bist_o_tlast;

   wire 	 bist_done;
   wire 	 bist_start;
   wire 	 bist_fail;
   wire [15:0] 	 bist_control;




    /////////////////////////////////////////////////////////////////////////////////
    // PPS synchronization and generation logic
    /////////////////////////////////////////////////////////////////////////////////

    //PPS input signals will be flopped in via the 10MHz reference clock.
    //Its assumed that the radio clock, which is derived from the 10MHz,
    //will be able to flop this captured PPS signal without metastability.
    //And that the relation of the radio clock to this ref clock will be
    //consistent enough across multiple units to use for this purpose.
    reg [1:0] 	 ext_pps_del, gps_pps_del;
    always @(posedge ext_ref_clk) ext_pps_del[1:0] <= {ext_pps_del[0], ext_pps};
    always @(posedge ext_ref_clk) gps_pps_del[1:0] <= {gps_pps_del[0], gps_pps};

    //Generate a PPS signal disciplined by the input reference PPS.
    //The PPS signal is 25% duty cycle, and generated by the rising edge.
    reg pps_int;
    wire pps_out_enb, pps_select;
    reg [31:0] pps_count;
    wire pps_duty_high = (pps_count < 32'd2500000);
    wire pps_edge = pps_select? (ext_pps_del == 2'b01) : (gps_pps_del == 2'b01);
    always @(posedge ext_ref_clk) begin
        if (pps_edge) pps_count <= 32'd3; //delay = sampling + detection + flopping out
        else if (pps_count == 32'd9999999) pps_count <= 32'b0;
        else pps_count <= pps_count + 1'b1;
        pps_out <= pps_duty_high? pps_out_enb : 1'b0;
        pps_int <= pps_duty_high? 1'b1 : 1'b0;
    end

   /////////////////////////////////////////////////////////////////////////////////
   // Bus Int containing soft CPU control, routing fabric
   /////////////////////////////////////////////////////////////////////////////////
   bus_int bus_int
     (.clk(bus_clk), .reset(bus_rst),
      .sen(LMK_SEN), .sclk(LMK_SCLK), .mosi(LMK_MOSI), .miso(1'b0),
      .scl0(SFPP0_SCL), .sda0(SFPP0_SDA),
      .scl1(db_scl), .sda1(db_sda),
      .scl2(SFPP1_SCL), .sda2(SFPP1_SDA),
      .gps_txd(gps_txd), .gps_rxd(gps_rxd),
      .debug_txd(debug_txd), .debug_rxd(debug_rxd),
      .leds(led_misc), .sw_rst(sw_rst),
      // SFP 0
      .SFPP0_ModAbs(SFPP0_ModAbs),.SFPP0_TxFault(SFPP0_TxFault),.SFPP0_RxLOS(SFPP0_RxLOS),
      .SFPP0_RS0(SFPP0_RS0), .SFPP0_RS1(SFPP0_RS1),
      // SFP 1
      .SFPP1_ModAbs(SFPP1_ModAbs),.SFPP1_TxFault(SFPP1_TxFault),.SFPP1_RxLOS(SFPP1_RxLOS),
      .SFPP1_RS0(SFPP1_RS0), .SFPP1_RS1(SFPP1_RS1),
      //clocky locky misc
      .clock_status({LMK_Holdover, LMK_Lock, LMK_Status}),
      .clock_control({clock_misc_opt[1:0], pps_out_enb, pps_select, clock_ref_sel[1:0]}),
      // Eth0
      .eth0_tx_tdata(eth0_tx_tdata), .eth0_tx_tuser(eth0_tx_tuser), .eth0_tx_tlast(eth0_tx_tlast),
      .eth0_tx_tvalid(eth0_tx_tvalid), .eth0_tx_tready(eth0_tx_tready),
      .eth0_rx_tdata(eth0_rx_tdata), .eth0_rx_tuser(eth0_rx_tuser), .eth0_rx_tlast(eth0_rx_tlast),
      .eth0_rx_tvalid(eth0_rx_tvalid), .eth0_rx_tready(eth0_rx_tready),
      // Eth1
      .eth1_tx_tdata(eth1_tx_tdata), .eth1_tx_tuser(eth1_tx_tuser), .eth1_tx_tlast(eth1_tx_tlast),
      .eth1_tx_tvalid(eth1_tx_tvalid), .eth1_tx_tready(eth1_tx_tready),
      .eth1_rx_tdata(eth1_rx_tdata), .eth1_rx_tuser(eth1_rx_tuser), .eth1_rx_tlast(eth1_rx_tlast),
      .eth1_rx_tvalid(eth1_rx_tvalid), .eth1_rx_tready(eth1_rx_tready),
      // Radio 0
      .r0o_tdata(r0i_tdata), .r0o_tlast(r0i_tlast), .r0o_tready(r0i_tready), .r0o_tvalid(r0i_tvalid),
      .r0i_tdata(r0o_tdata), .r0i_tlast(r0o_tlast), .r0i_tready(r0o_tready), .r0i_tvalid(r0o_tvalid),
      // Radio 1
      .r1o_tdata(r1i_tdata), .r1o_tlast(r1i_tlast), .r1o_tready(r1i_tready), .r1o_tvalid(r1i_tvalid),
      .r1i_tdata(r1o_tdata), .r1i_tlast(r1o_tlast), .r1i_tready(r1o_tready), .r1i_tvalid(r1o_tvalid),
      // CE0
      .ce0o_tdata(ce0o_tdata), .ce0o_tlast(ce0o_tlast), .ce0o_tvalid(ce0o_tvalid), .ce0o_tready(ce0o_tready),
      .ce0i_tdata(ce0i_tdata), .ce0i_tlast(ce0i_tlast), .ce0i_tvalid(ce0i_tvalid), .ce0i_tready(ce0i_tready),
      // CE 1
      .ce1o_tdata(ce1o_tdata), .ce1o_tlast(ce1o_tlast), .ce1o_tvalid(ce1o_tvalid), .ce1o_tready(ce1o_tready),
      .ce1i_tdata(ce1i_tdata), .ce1i_tlast(ce1i_tlast), .ce1i_tvalid(ce1i_tvalid), .ce1i_tready(ce1i_tready),
      // CE 2
      .ce2o_tdata(ce2o_tdata), .ce2o_tlast(ce2o_tlast), .ce2o_tvalid(ce2o_tvalid), .ce2o_tready(ce2o_tready),
      .ce2i_tdata(ce2i_tdata), .ce2i_tlast(ce2i_tlas), .ce2i_tvalid(ce2i_tvalid), .ce2i_tready(ce2i_tready),
      // IoP2 Msgs
      .o_iop2_msg_tdata(o_iop2_msg_tdata), .o_iop2_msg_tvalid(o_iop2_msg_tvalid), .o_iop2_msg_tlast(o_iop2_msg_tlast), .o_iop2_msg_tready(o_iop2_msg_tready),
      .i_iop2_msg_tdata(i_iop2_msg_tdata), .i_iop2_msg_tvalid(i_iop2_msg_tvalid), .i_iop2_msg_tlast(i_iop2_msg_tlast), .i_iop2_msg_tready(i_iop2_msg_tready),
      // PCIe
      .pcio_tdata(pcio_tdata), .pcio_tlast(pcio_tlast), .pcio_tvalid(pcio_tvalid), .pcio_tready(pcio_tready),
      .pcii_tdata(pcii_tdata), .pcii_tlast(pcii_tlast), .pcii_tvalid(pcii_tvalid), .pcii_tready(pcii_tready),
      // Wishbone Slave Interface(s)
      .s4_dat_i(s4_dat_i),
      .s4_dat_o(s4_dat_o),
      .s4_adr(s4_adr),
      .s4_sel(s4_sel),
      .s4_ack(s4_ack),
      .s4_stb(s4_stb),
      .s4_cyc(s4_cyc),
      .s4_we(s4_we),
      .s4_int(s4_int),
      .s5_dat_i(s5_dat_i),
      .s5_dat_o(s5_dat_o),
      .s5_adr(s5_adr),
      .s5_sel(s5_sel),
      .s5_ack(s5_ack),
      .s5_stb(s5_stb),
      .s5_cyc(s5_cyc),
      .s5_we(s5_we),
      .s5_int(s5_int),

      // AXI_DRAM_FIFO BIST. Not for production.
      .bist_start(bist_start),
      .bist_control(bist_control),
      .bist_done(bist_done),
      .bist_fail(bist_fail),
      // Debug
      .fifo_flags({r0_tx_tready_bo,r0_tx_tready_bi,r1_tx_tready_bo,r1_tx_tready_bi}),
      .debug0(debug0), .debug1(debug1), .debug2(debug2));

   //////////////////////////////////////////////////////////////////////////////////////////////
   //
   // AXI Loopbacks as place holders for CE's
   //
   ///////////////////////////////////////////////////////////////////////////////////////////////
   axi_loopback axi_loopback_ce0
     (
      .clk(bus_clk),
      .reset(bus_rst),
      // Input AXIS
      .i_tdata(ce0o_tdata),
      .i_tlast(ce0o_tlast),
      .i_tvalid(ce0o_tvalid),
      .i_tready(ce0o_tready),
      // Output AXIS
      .o_tdata(ce0i_tdata),
      .o_tlast(ce0i_tlast),
      .o_tvalid(ce0i_tvalid),
      .o_tready(ce0i_tready)
      );

   axi_loopback axi_loopback_ce1
     (
      .clk(bus_clk),
      .reset(bus_rst),
      // Input AXIS
      .i_tdata(ce1o_tdata),
      .i_tlast(ce1o_tlast),
      .i_tvalid(ce1o_tvalid),
      .i_tready(ce1o_tready),
      // Output AXIS
      .o_tdata(ce1i_tdata),
      .o_tlast(ce1i_tlast),
      .o_tvalid(ce1i_tvalid),
      .o_tready(ce1i_tready)
      );

   axi_loopback axi_loopback_ce2
     (
      .clk(bus_clk),
      .reset(bus_rst),
      // Input AXIS
      .i_tdata(ce2o_tdata),
      .i_tlast(ce2o_tlast),
      .i_tvalid(ce2o_tvalid),
      .i_tready(ce2o_tready),
      // Output AXIS
      .o_tdata(ce2i_tdata),
      .o_tlast(ce2i_tlast),
      .o_tvalid(ce2i_tvalid),
      .o_tready(ce2i_tready)
      );

   /////////////////////////////////////////////////////////////////////////////////////////////
   //
   // Radio 0
   //
   /////////////////////////////////////////////////////////////////////////////////////////////

`ifndef DELETE_DSP0
 `define DELETE_DSP0 0
`endif

   radio #(
	   .CHIPSCOPE(0),
	   .DELETE_DSP(`DELETE_DSP0)
	   )
   radio0
     (
      .radio_clk(radio_clk), .radio_rst(radio_rst),
      .rx(rx0), .tx(tx0), .db_gpio(db_gpio0), .fp_gpio(fp_gpio),
      .sen(sen0), .sclk(sclk0), .mosi(mosi0), .miso(miso0),
      .misc_outs(misc_outs0), .leds(radio_led0),
      .bus_clk(bus_clk), .bus_rst(bus_rst),
      .in_tdata(r0i_tdata), .in_tlast(r0i_tlast), .in_tvalid(r0i_tvalid), .in_tready(r0i_tready),
      .out_tdata(r0o_tdata), .out_tlast(r0o_tlast), .out_tvalid(r0o_tvalid), .out_tready(r0o_tready),
      .tx_tdata_bo(r0_tx_tdata_bo), .tx_tlast_bo(r0_tx_tlast_bo),
      .tx_tvalid_bo(r0_tx_tvalid_bo), .tx_tready_bo(r0_tx_tready_bo),
      .tx_tdata_bi(r0_tx_tdata_bi), .tx_tlast_bi(r0_tx_tlast_bi),
      .tx_tvalid_bi(r0_tx_tvalid_bi), .tx_tready_bi(r0_tx_tready_bi),
      .pps(pps_int), .sync_dacs(sync_dacs_radio0)
      );

     always @(posedge radio_clk)
       radio_misc0 <= misc_outs0;

   /////////////////////////////////////////////////////////////////////////////////////////////
   //
   // Radio 1
   //
   /////////////////////////////////////////////////////////////////////////////////////////////

`ifndef DELETE_DSP1
 `define DELETE_DSP1 0
`endif

   radio #(
	   .CHIPSCOPE(0),
	   .DELETE_DSP(`DELETE_DSP1)
	   )
   radio1
     (
      .radio_clk(radio_clk), .radio_rst(radio_rst),
      .rx(rx1), .tx(tx1), .db_gpio(db_gpio1),
      .sen(sen1), .sclk(sclk1), .mosi(mosi1), .miso(miso1),
      .misc_outs(misc_outs1), .leds(radio_led1),
      .bus_clk(bus_clk), .bus_rst(bus_rst),
      .in_tdata(r1i_tdata), .in_tlast(r1i_tlast), .in_tvalid(r1i_tvalid), .in_tready(r1i_tready),
      .out_tdata(r1o_tdata), .out_tlast(r1o_tlast), .out_tvalid(r1o_tvalid), .out_tready(r1o_tready),
      .tx_tdata_bo(r1_tx_tdata_bo), .tx_tlast_bo(r1_tx_tlast_bo),
      .tx_tvalid_bo(r1_tx_tvalid_bo), .tx_tready_bo(r1_tx_tready_bo),
      .tx_tdata_bi(r1_tx_tdata_bi), .tx_tlast_bi(r1_tx_tlast_bi),
      .tx_tvalid_bi(r1_tx_tvalid_bi), .tx_tready_bi(r1_tx_tready_bi),
      .pps(pps_int), .sync_dacs(sync_dacs_radio1)
      );

   always @(posedge radio_clk)
     radio_misc1 <= misc_outs1;

   /////////////////////////////////////////////////////////////////////////////////////////////
   //
   // Ethernet
   //
   /////////////////////////////////////////////////////////////////////////////////////////////
`ifdef ETH10G_PORT0
   xge_mac_wrapper
     #(.PORTNUM(8'h0))
   xge_mac_wrapper_port0
     (
      // XGMII
      .xgmii_clk(xgmii_clk0),
      .xgmii_txd(xgmii_txd0),
      .xgmii_txc(xgmii_txc0),
      .xgmii_rxd(xgmii_rxd0),
      .xgmii_rxc(xgmii_rxc0),
      // MDIO
      .mdc(mdc0),
      .mdio_in(mdio_in0),
      .mdio_out(mdio_out0),
      // Wishbone I/F
      .wb_adr_i(s4_adr),               // To wishbone_if0 of wishbone_if.v
      .wb_clk_i(bus_clk),              // To sync_clk_wb0 of sync_clk_wb.v, ...
      .wb_cyc_i(s4_cyc),               // To wishbone_if0 of wishbone_if.v
      .wb_dat_i(s4_dat_o),             // To wishbone_if0 of wishbone_if.v
      .wb_rst_i(bus_rst),              // To sync_clk_wb0 of sync_clk_wb.v, ...
      .wb_stb_i(s4_stb),               // To wishbone_if0 of wishbone_if.v
      .wb_we_i(s4_we),                 // To wishbone_if0 of wishbone_if.v
      .wb_ack_o(s4_ack),               // From wishbone_if0 of wishbone_if.v
      .wb_dat_o(s4_dat_i),             // From wishbone_if0 of wishbone_if.v
      .wb_int_o(s4_int),               // From wishbone_if0 of wishbone_if.v
      // Client FIFO Interfaces
      .sys_clk(bus_clk),
      .reset(bus_rst),          // From sys_clk domain.
      .rx_tdata(eth0_rx_tdata),
      .rx_tuser(eth0_rx_tuser),
      .rx_tlast(eth0_rx_tlast),
      .rx_tvalid(eth0_rx_tvalid),
      .rx_tready(eth0_rx_tready),
      .tx_tdata(eth0_tx_tdata),
      .tx_tuser(eth0_tx_tuser),                // Bit[3] (error) is ignored for now.
      .tx_tlast(eth0_tx_tlast),
      .tx_tvalid(eth0_tx_tvalid),
      .tx_tready(eth0_tx_tready),
      // Other
      .phy_ready(xge_phy_resetdone0),
      // Debug
      .debug_rx(),
      .debug_tx());

`else
   simple_gemac_wrapper #(.RX_FLOW_CTRL(0), .PORTNUM(8'd0)) simple_gemac_wrapper_port0
     (.clk125(gmii_clk0), .reset(sw_rst[0]),
      .GMII_GTX_CLK(), .GMII_TX_EN(gmii_tx_en0), .GMII_TX_ER(gmii_tx_er0), .GMII_TXD(gmii_txd0),
      .GMII_RX_CLK(gmii_clk0), .GMII_RX_DV(gmii_rx_dv0), .GMII_RX_ER(gmii_rx_er0), .GMII_RXD(gmii_rxd0),

      .sys_clk(bus_clk),
      .rx_tdata(eth0_rx_tdata), .rx_tuser(eth0_rx_tuser), .rx_tlast(eth0_rx_tlast), .rx_tvalid(eth0_rx_tvalid), .rx_tready(eth0_rx_tready),
      .tx_tdata(eth0_tx_tdata), .tx_tuser(eth0_tx_tuser), .tx_tlast(eth0_tx_tlast), .tx_tvalid(eth0_tx_tvalid), .tx_tready(eth0_tx_tready),
      // MDIO
      .mdc(mdc0),
      .mdio_in(mdio_in0),
      .mdio_out(mdio_out0),
      // Wishbone I/F
      .wb_adr_i(s4_adr),               // To wishbone_if0 of wishbone_if.v
      .wb_clk_i(bus_clk),              // To sync_clk_wb0 of sync_clk_wb.v, ...
      .wb_cyc_i(s4_cyc),               // To wishbone_if0 of wishbone_if.v
      .wb_dat_i(s4_dat_o),             // To wishbone_if0 of wishbone_if.v
      .wb_rst_i(bus_rst),              // To sync_clk_wb0 of sync_clk_wb.v, ...
      .wb_stb_i(s4_stb),               // To wishbone_if0 of wishbone_if.v
      .wb_we_i(s4_we),                 // To wishbone_if0 of wishbone_if.v
      .wb_ack_o(s4_ack),               // From wishbone_if0 of wishbone_if.v
      .wb_dat_o(s4_dat_i),             // From wishbone_if0 of wishbone_if.v
      .wb_int_o(s4_int),               // From wishbone_if0 of wishbone_if.v
      // Debug
      .debug_tx(), .debug_rx()
      );
`endif // !`ifdef ETH10G_PORT0


 `ifdef ETH10G_PORT1
   xge_mac_wrapper
     #(.PORTNUM(8'h1))
   xge_mac_wrapper_port1
     (
      // XGMII
      .xgmii_clk(xgmii_clk1),
      .xgmii_txd(xgmii_txd1),
      .xgmii_txc(xgmii_txc1),
      .xgmii_rxd(xgmii_rxd1),
      .xgmii_rxc(xgmii_rxc1),
      // MDIO
      .mdc(mdc1),
      .mdio_in(mdio_in1),
      .mdio_out(mdio_out1),
      // Wishbone I/F
      .wb_adr_i(s5_adr),               // To wishbone_if0 of wishbone_if.v
      .wb_clk_i(bus_clk),              // To sync_clk_wb0 of sync_clk_wb.v, ...
      .wb_cyc_i(s5_cyc),               // To wishbone_if0 of wishbone_if.v
      .wb_dat_i(s5_dat_o),             // To wishbone_if0 of wishbone_if.v
      .wb_rst_i(bus_rst),              // To sync_clk_wb0 of sync_clk_wb.v, ...
      .wb_stb_i(s5_stb),               // To wishbone_if0 of wishbone_if.v
      .wb_we_i(s5_we),                 // To wishbone_if0 of wishbone_if.v
      .wb_ack_o(s5_ack),               // From wishbone_if0 of wishbone_if.v
      .wb_dat_o(s5_dat_i),             // From wishbone_if0 of wishbone_if.v
      .wb_int_o(s5_int),               // From wishbone_if0 of wishbone_if.v
      // Client FIFO Interfaces
      .sys_clk(bus_clk),
      .reset(bus_rst),          // From sys_clk domain.
      .rx_tdata(eth1_rx_tdata),
      .rx_tuser(eth1_rx_tuser),
      .rx_tlast(eth1_rx_tlast),
      .rx_tvalid(eth1_rx_tvalid),
      .rx_tready(eth1_rx_tready),
      .tx_tdata(eth1_tx_tdata),
      .tx_tuser(eth1_tx_tuser),                // Bit[3] (error) is ignored for now.
      .tx_tlast(eth1_tx_tlast),
      .tx_tvalid(eth1_tx_tvalid),
      .tx_tready(eth1_tx_tready),
      // Other
      .phy_ready(xge_phy_resetdone1),
      // Debug
      .debug_rx(),
      .debug_tx());

`else
   simple_gemac_wrapper #(.RX_FLOW_CTRL(0), .PORTNUM(8'd1)) simple_gemac_wrapper_port1
     (.clk125(gmii_clk1), .reset(sw_rst[0]),
      .GMII_GTX_CLK(), .GMII_TX_EN(gmii_tx_en1), .GMII_TX_ER(gmii_tx_er1), .GMII_TXD(gmii_txd1),
      .GMII_RX_CLK(gmii_clk1), .GMII_RX_DV(gmii_rx_dv1), .GMII_RX_ER(gmii_rx_er1), .GMII_RXD(gmii_rxd1),

      .sys_clk(bus_clk),
      .rx_tdata(eth1_rx_tdata), .rx_tuser(eth1_rx_tuser), .rx_tlast(eth1_rx_tlast), .rx_tvalid(eth1_rx_tvalid), .rx_tready(eth1_rx_tready),
      .tx_tdata(eth1_tx_tdata), .tx_tuser(eth1_tx_tuser), .tx_tlast(eth1_tx_tlast), .tx_tvalid(eth1_tx_tvalid), .tx_tready(eth1_tx_tready),
      // MDIO
      .mdc(mdc1),
      .mdio_in(mdio_in1),
      .mdio_out(mdio_out1),
      // Wishbone I/F
      .wb_adr_i(s5_adr),               // To wishbone_if0 of wishbone_if.v
      .wb_clk_i(bus_clk),              // To sync_clk_wb0 of sync_clk_wb.v, ...
      .wb_cyc_i(s5_cyc),               // To wishbone_if0 of wishbone_if.v
      .wb_dat_i(s5_dat_o),             // To wishbone_if0 of wishbone_if.v
      .wb_rst_i(bus_rst),              // To sync_clk_wb0 of sync_clk_wb.v, ...
      .wb_stb_i(s5_stb),               // To wishbone_if0 of wishbone_if.v
      .wb_we_i(s5_we),                 // To wishbone_if0 of wishbone_if.v
      .wb_ack_o(s5_ack),               // From wishbone_if0 of wishbone_if.v
      .wb_dat_o(s5_dat_i),             // From wishbone_if0 of wishbone_if.v
      .wb_int_o(s5_int),               // From wishbone_if0 of wishbone_if.v
      // Debug
      .debug_tx(), .debug_rx());
`endif

`ifndef NO_DRAM_FIFOS
   ///////////////////////////////////////////////////////////////////////////////////
   //
   // AXI Crossbar 2:1 (64bit@250MHz Slave-> 128bit@250MHz Master).
   // Uses 512 entry FIFO's internally and packet mode is enabled to minimize bus stalls.
   //
   ///////////////////////////////////////////////////////////////////////////////////
   axi_intercon_2x64_128 axi_intercon_2x64_128_i
     (
      .INTERCONNECT_ACLK(ddr3_axi_clk), // input INTERCONNECT_ACLK
      .INTERCONNECT_ARESETN(~ddr3_axi_rst), // input INTERCONNECT_ARESETN
      //
      .S00_AXI_ARESET_OUT_N(), // output S00_AXI_ARESET_OUT_N
      .S00_AXI_ACLK(ddr3_axi_clk), // input S00_AXI_ACLK
      .S00_AXI_AWID(s00_axi_awid), // input [0 : 0] S00_AXI_AWID
      .S00_AXI_AWADDR(s00_axi_awaddr), // input [31 : 0] S00_AXI_AWADDR
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
      .S00_AXI_ARADDR(s00_axi_araddr), // input [31 : 0] S00_AXI_ARADDR
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
      .S01_AXI_ARESET_OUT_N(), // output S01_AXI_ARESET_OUT_N
      .S01_AXI_ACLK(ddr3_axi_clk), // input S01_AXI_ACLK
      .S01_AXI_AWID(s01_axi_awid), // input [0 : 0] S01_AXI_AWID
      .S01_AXI_AWADDR(s01_axi_awaddr), // input [31 : 0] S01_AXI_AWADDR
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
      .S01_AXI_ARADDR(s01_axi_araddr), // input [31 : 0] S01_AXI_ARADDR
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
      .M00_AXI_ARESET_OUT_N(), // output M00_AXI_ARESET_OUT_N
      .M00_AXI_ACLK(ddr3_axi_clk), // input M00_AXI_ACLK
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
      .M00_AXI_WDATA(ddr3_axi_wdata), // output [127 : 0] M00_AXI_WDATA
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
      .M00_AXI_RDATA(ddr3_axi_rdata), // input [127 : 0] M00_AXI_RDATA
      .M00_AXI_RRESP(ddr3_axi_rresp), // input [1 : 0] M00_AXI_RRESP
      .M00_AXI_RLAST(ddr3_axi_rlast), // input M00_AXI_RLAST
      .M00_AXI_RVALID(ddr3_axi_rvalid), // input M00_AXI_RVALID
      .M00_AXI_RREADY(ddr3_axi_rready) // output M00_AXI_RREADY
      );


   ///////////////////////////////////////////////////////////////////////////////////
   //
   // Dual DRAM FIFO's each providing 16MB of buffer to a 64bit AXI-Stream
   //
   ///////////////////////////////////////////////////////////////////////////////////


   axi_dram_fifo
     #(.BASE('h0),
       .SIZE(24),
       .TIMEOUT(280)
       ) axi_dram_fifo_i0
       (
	//
	// Clocks and reset
	//
	.bus_clk(bus_clk),
	.bus_reset(bus_rst),
	.clear(1'b0),
	.dram_clk(ddr3_axi_clk),
	.dram_reset(ddr3_axi_rst),
	//
	// AXI Write address channel
	//
	.m_axi_awid(s00_axi_awid), // output [0 : 0] m_axi_awid
	.m_axi_awaddr(s00_axi_awaddr), // output [31 : 0] m_axi_awaddr
	.m_axi_awlen(s00_axi_awlen), // output [7 : 0] m_axi_awlen
	.m_axi_awsize(s00_axi_awsize), // output [2 : 0] m_axi_awsize
	.m_axi_awburst(s00_axi_awburst), // output [1 : 0] m_axi_awburst
	.m_axi_awlock(s00_axi_awlock), // output [0 : 0] m_axi_awlock
	.m_axi_awcache(s00_axi_awcache), // output [3 : 0] m_axi_awcache
	.m_axi_awprot(s00_axi_awprot), // output [2 : 0] m_axi_awprot
	.m_axi_awqos(s00_axi_awqos), // output [3 : 0] m_axi_awqos
	.m_axi_awregion(s00_axi_awregion), // output [3 : 0] m_axi_awregion
	.m_axi_awuser(s00_axi_awuser), // output [0 : 0] m_axi_awuser
	.m_axi_awvalid(s00_axi_awvalid), // output m_axi_awvalid
	.m_axi_awready(s00_axi_awready), // input m_axi_awready
	//
	// AXI Write data channel.
	//
	.m_axi_wdata(s00_axi_wdata), // output [63 : 0] m_axi_wdata
	.m_axi_wstrb(s00_axi_wstrb), // output [7 : 0] m_axi_wstrb
	.m_axi_wlast(s00_axi_wlast), // output m_axi_wlast
	.m_axi_wuser(s00_axi_wuser), // output [0 : 0] m_axi_wuser
	.m_axi_wvalid(s00_axi_wvalid), // output m_axi_wvalid
	.m_axi_wready(s00_axi_wready), // input m_axi_wready
	//
	// AXI Write response channel signals
	//
	.m_axi_bid(s00_axi_bid), // input [0 : 0] m_axi_bid
	.m_axi_bresp(s00_axi_bresp), // input [1 : 0] m_axi_bresp
	.m_axi_buser(s00_axi_buser), // input [0 : 0] m_axi_buser
	.m_axi_bvalid(s00_axi_bvalid), // input m_axi_bvalid
	.m_axi_bready(s00_axi_bready), // output m_axi_bready
	//
	// AXI Read address channel
	//
	.m_axi_arid(s00_axi_arid), // output [0 : 0] m_axi_arid
	.m_axi_araddr(s00_axi_araddr), // output [31 : 0] m_axi_araddr
	.m_axi_arlen(s00_axi_arlen), // output [7 : 0] m_axi_arlen
	.m_axi_arsize(s00_axi_arsize), // output [2 : 0] m_axi_arsize
	.m_axi_arburst(s00_axi_arburst), // output [1 : 0] m_axi_arburst
	.m_axi_arlock(s00_axi_arlock), // output [0 : 0] m_axi_arlock
	.m_axi_arcache(s00_axi_arcache), // output [3 : 0] m_axi_arcache
	.m_axi_arprot(s00_axi_arprot), // output [2 : 0] m_axi_arprot
	.m_axi_arqos(s00_axi_arqos), // output [3 : 0] m_axi_arqos
	.m_axi_arregion(s00_axi_arregion), // output [3 : 0] m_axi_arregion
	.m_axi_aruser(s00_axi_aruser), // output [0 : 0] m_axi_aruser
	.m_axi_arvalid(s00_axi_arvalid), // output m_axi_arvalid
	.m_axi_arready(s00_axi_arready), // input m_axi_arready
	//
	// AXI Read data channel
	//
	.m_axi_rid(s00_axi_rid), // input [0 : 0] m_axi_rid
	.m_axi_rdata(s00_axi_rdata), // input [63 : 0] m_axi_rdata
	.m_axi_rresp(s00_axi_rresp), // input [1 : 0] m_axi_rresp
	.m_axi_rlast(s00_axi_rlast), // input m_axi_rlast
	.m_axi_ruser(s00_axi_ruser), // input [0 : 0] m_axi_ruser
	.m_axi_rvalid(s00_axi_rvalid), // input m_axi_rvalid
	.m_axi_rready(s00_axi_rready), // output m_axi_rready
	//
	// CHDR friendly AXI stream input
	//
	.i_tdata(r0_tx_tdata_bo),
	.i_tlast(r0_tx_tlast_bo),
	.i_tvalid(r0_tx_tvalid_bo),
	.i_tready(r0_tx_tready_bo),
	//
	// CHDR friendly AXI Stream output
	//
	.o_tdata(r0_tx_tdata_bi),
	.o_tlast(r0_tx_tlast_bi),
	.o_tvalid(r0_tx_tvalid_bi),
	.o_tready(r0_tx_tready_bi),
	//
	//
	//
	.supress_threshold(bist_control),
	.supress_enable(bist_start),
	//
	// Debug
	//
	.debug()
	);
 
   axi_dram_fifo
     #(.BASE('h2000000),
       .SIZE(24),
       .TIMEOUT(280)
       ) axi_dram_fifo_i1
       (
	//
	// Clocks and reset
	//
	.bus_clk(bus_clk),
	.bus_reset(bus_rst),
	.clear(1'b0),
	.dram_clk(ddr3_axi_clk),
	.dram_reset(ddr3_axi_rst),
	//
	// AXI Write address channel
	//
	.m_axi_awid(s01_axi_awid), // output [0 : 0] m_axi_awid
	.m_axi_awaddr(s01_axi_awaddr), // output [31 : 0] m_axi_awaddr
	.m_axi_awlen(s01_axi_awlen), // output [7 : 0] m_axi_awlen
	.m_axi_awsize(s01_axi_awsize), // output [2 : 0] m_axi_awsize
	.m_axi_awburst(s01_axi_awburst), // output [1 : 0] m_axi_awburst
	.m_axi_awlock(s01_axi_awlock), // output [0 : 0] m_axi_awlock
	.m_axi_awcache(s01_axi_awcache), // output [3 : 0] m_axi_awcache
	.m_axi_awprot(s01_axi_awprot), // output [2 : 0] m_axi_awprot
	.m_axi_awqos(s01_axi_awqos), // output [3 : 0] m_axi_awqos
	.m_axi_awregion(s01_axi_awregion), // output [3 : 0] m_axi_awregion
	.m_axi_awuser(s01_axi_awuser), // output [0 : 0] m_axi_awuser
	.m_axi_awvalid(s01_axi_awvalid), // output m_axi_awvalid
	.m_axi_awready(s01_axi_awready), // input m_axi_awready
	//
	// AXI Write data channel.
	//
	.m_axi_wdata(s01_axi_wdata), // output [63 : 0] m_axi_wdata
	.m_axi_wstrb(s01_axi_wstrb), // output [7 : 0] m_axi_wstrb
	.m_axi_wlast(s01_axi_wlast), // output m_axi_wlast
	.m_axi_wuser(s01_axi_wuser), // output [0 : 0] m_axi_wuser
	.m_axi_wvalid(s01_axi_wvalid), // output m_axi_wvalid
	.m_axi_wready(s01_axi_wready), // input m_axi_wready
	//
	// AXI Write response channel signals
	//
	.m_axi_bid(s01_axi_bid), // input [0 : 0] m_axi_bid
	.m_axi_bresp(s01_axi_bresp), // input [1 : 0] m_axi_bresp
	.m_axi_buser(s01_axi_buser), // input [0 : 0] m_axi_buser
	.m_axi_bvalid(s01_axi_bvalid), // input m_axi_bvalid
	.m_axi_bready(s01_axi_bready), // output m_axi_bready
	//
	// AXI Read address channel
	//
	.m_axi_arid(s01_axi_arid), // output [0 : 0] m_axi_arid
	.m_axi_araddr(s01_axi_araddr), // output [31 : 0] m_axi_araddr
	.m_axi_arlen(s01_axi_arlen), // output [7 : 0] m_axi_arlen
	.m_axi_arsize(s01_axi_arsize), // output [2 : 0] m_axi_arsize
	.m_axi_arburst(s01_axi_arburst), // output [1 : 0] m_axi_arburst
	.m_axi_arlock(s01_axi_arlock), // output [0 : 0] m_axi_arlock
	.m_axi_arcache(s01_axi_arcache), // output [3 : 0] m_axi_arcache
	.m_axi_arprot(s01_axi_arprot), // output [2 : 0] m_axi_arprot
	.m_axi_arqos(s01_axi_arqos), // output [3 : 0] m_axi_arqos
	.m_axi_arregion(s01_axi_arregion), // output [3 : 0] m_axi_arregion
	.m_axi_aruser(s01_axi_aruser), // output [0 : 0] m_axi_aruser
	.m_axi_arvalid(s01_axi_arvalid), // output m_axi_arvalid
	.m_axi_arready(s01_axi_arready), // input m_axi_arready
	//
	// AXI Read data channel
	//
	.m_axi_rid(s01_axi_rid), // input [0 : 0] m_axi_rid
	.m_axi_rdata(s01_axi_rdata), // input [63 : 0] m_axi_rdata
	.m_axi_rresp(s01_axi_rresp), // input [1 : 0] m_axi_rresp
	.m_axi_rlast(s01_axi_rlast), // input m_axi_rlast
	.m_axi_ruser(s01_axi_ruser), // input [0 : 0] m_axi_ruser
	.m_axi_rvalid(s01_axi_rvalid), // input m_axi_rvalid
	.m_axi_rready(s01_axi_rready), // output m_axi_rready
	//
	// CHDR friendly AXI stream input
	//
	.i_tdata(r1_tx_tdata_bo),
	.i_tlast(r1_tx_tlast_bo),
	.i_tvalid(r1_tx_tvalid_bo),
	.i_tready(r1_tx_tready_bo),
	//
	// CHDR friendly AXI Stream output
	//

	.o_tdata(r1_tx_tdata_bi),
	.o_tlast(r1_tx_tlast_bi),
	.o_tvalid(r1_tx_tvalid_bi),
	.o_tready(r1_tx_tready_bi),
	//
	//
	//
	.supress_threshold(bist_control),
	.supress_enable(bist_start),
	//
	// Debug
	//
	.debug()
	);

   //
   // Provide flag in radio_clk domain that indicates DDR3 interface is calibrated
   // and running.
   //

   reg 	       ddr3_running_radio_clk_pre, ddr3_running_radio_clk;

   always @(posedge radio_clk)
     begin
	ddr3_running_radio_clk_pre <= ddr3_running;
	ddr3_running_radio_clk <= ddr3_running_radio_clk_pre;
     end


`else //  `ifndef NO_DRAM_FIFOS

   //
   // Alternate smaller internal SRAM based FIFO's for Tx when DRAM not compiled into FPGA.
   // Short FIFO's added for ease of timing closure since large FIFO's spread all over die.
   //
   wire [63:0] r0_tx_tdata_bos; wire r0_tx_tlast_bos, r0_tx_tvalid_bos, r0_tx_tready_bos;   
   wire [63:0] r0_tx_tdata_0; wire r0_tx_tlast_0, r0_tx_tvalid_0, r0_tx_tready_0;
   wire [63:0] r0_tx_tdata_0s; wire r0_tx_tlast_0s, r0_tx_tvalid_0s, r0_tx_tready_0s;   
   wire [63:0] r0_tx_tdata_1; wire r0_tx_tlast_1, r0_tx_tvalid_1, r0_tx_tready_1;
   wire [63:0] r0_tx_tdata_1s; wire r0_tx_tlast_1s, r0_tx_tvalid_1s, r0_tx_tready_1s;   
   wire [63:0] r0_tx_tdata_2; wire r0_tx_tlast_2, r0_tx_tvalid_2, r0_tx_tready_2;
   wire [63:0] r0_tx_tdata_2s; wire r0_tx_tlast_2s, r0_tx_tvalid_2s, r0_tx_tready_2s;
   wire [63:0] r0_tx_tdata_bis; wire r0_tx_tlast_bis, r0_tx_tvalid_bis, r0_tx_tready_bis;
   
   axi_fifo_short #(.WIDTH(65)) tx_fifo0_bos
     (
      .clk(bus_clk), .reset(bus_rst), .clear(1'b0),
      .i_tdata({r0_tx_tlast_bo,r0_tx_tdata_bo}), .i_tvalid(r0_tx_tvalid_bo), .i_tready(r0_tx_tready_bo),
      .o_tdata({r0_tx_tlast_bos,r0_tx_tdata_bos}), .o_tvalid(r0_tx_tvalid_bos), .o_tready(r0_tx_tready_bos),
      .space(), .occupied()
      );

   axi_fifo #(.WIDTH(65), .SIZE(`SRAM_FIFO_SIZE-2)) tx_fifo0_0
     (.clk(bus_clk), .reset(bus_rst), .clear(1'b0),
      .i_tdata({r0_tx_tlast_bos,r0_tx_tdata_bos}), .i_tvalid(r0_tx_tvalid_bos), .i_tready(r0_tx_tready_bos),
      .o_tdata({r0_tx_tlast_0,r0_tx_tdata_0}), .o_tvalid(r0_tx_tvalid_0), .o_tready(r0_tx_tready_0),
      .space(), .occupied());

   axi_fifo_short #(.WIDTH(65)) tx_fifo0_0s
     (
      .clk(bus_clk), .reset(bus_rst), .clear(1'b0),
      .i_tdata({r0_tx_tlast_0,r0_tx_tdata_0}), .i_tvalid(r0_tx_tvalid_0), .i_tready(r0_tx_tready_0),
      .o_tdata({r0_tx_tlast_0s,r0_tx_tdata_0s}), .o_tvalid(r0_tx_tvalid_0s), .o_tready(r0_tx_tready_0s),
      .space(), .occupied()
      );
   
   axi_fifo #(.WIDTH(65), .SIZE(`SRAM_FIFO_SIZE-2)) tx_fifo0_1
     (.clk(bus_clk), .reset(bus_rst), .clear(1'b0),
      .i_tdata({r0_tx_tlast_0s,r0_tx_tdata_0s}), .i_tvalid(r0_tx_tvalid_0s), .i_tready(r0_tx_tready_0s),
      .o_tdata({r0_tx_tlast_1,r0_tx_tdata_1}), .o_tvalid(r0_tx_tvalid_1), .o_tready(r0_tx_tready_1),
      .space(), .occupied());

   axi_fifo_short #(.WIDTH(65)) tx_fifo0_1s
     (
      .clk(bus_clk), .reset(bus_rst), .clear(1'b0),
      .i_tdata({r0_tx_tlast_1,r0_tx_tdata_1}), .i_tvalid(r0_tx_tvalid_1), .i_tready(r0_tx_tready_1),
      .o_tdata({r0_tx_tlast_1s,r0_tx_tdata_1s}), .o_tvalid(r0_tx_tvalid_1s), .o_tready(r0_tx_tready_1s),
      .space(), .occupied()
      );
      
   axi_fifo #(.WIDTH(65), .SIZE(`SRAM_FIFO_SIZE-2)) tx_fifo0_2
     (.clk(bus_clk), .reset(bus_rst), .clear(1'b0),
      .i_tdata({r0_tx_tlast_1s,r0_tx_tdata_1s}), .i_tvalid(r0_tx_tvalid_1s), .i_tready(r0_tx_tready_1s),
      .o_tdata({r0_tx_tlast_2,r0_tx_tdata_2}), .o_tvalid(r0_tx_tvalid_2), .o_tready(r0_tx_tready_2),
      .space(), .occupied());
   
   axi_fifo_short #(.WIDTH(65)) tx_fifo0_2s
     (
      .clk(bus_clk), .reset(bus_rst), .clear(1'b0),
      .i_tdata({r0_tx_tlast_2,r0_tx_tdata_2}), .i_tvalid(r0_tx_tvalid_2), .i_tready(r0_tx_tready_2),
      .o_tdata({r0_tx_tlast_2s,r0_tx_tdata_2s}), .o_tvalid(r0_tx_tvalid_2s), .o_tready(r0_tx_tready_2s),
      .space(), .occupied()
      );
   
   axi_fifo #(.WIDTH(65), .SIZE(`SRAM_FIFO_SIZE-2)) tx_fifo0_3
     (.clk(bus_clk), .reset(bus_rst), .clear(1'b0),
      .i_tdata({r0_tx_tlast_2s,r0_tx_tdata_2s}), .i_tvalid(r0_tx_tvalid_2s), .i_tready(r0_tx_tready_2s),
      .o_tdata({r0_tx_tlast_bis,r0_tx_tdata_bis}), .o_tvalid(r0_tx_tvalid_bis), .o_tready(r0_tx_tready_bis),
      .space(), .occupied());
   
   axi_fifo_short #(.WIDTH(65)) tx_fifo0_bis
     (
      .clk(bus_clk), .reset(bus_rst), .clear(1'b0),
      .i_tdata({r0_tx_tlast_bis,r0_tx_tdata_bis}), .i_tvalid(r0_tx_tvalid_bis), .i_tready(r0_tx_tready_bis),
      .o_tdata({r0_tx_tlast_bi,r0_tx_tdata_bi}), .o_tvalid(r0_tx_tvalid_bi), .o_tready(r0_tx_tready_bi),
      .space(), .occupied()
      );

   wire [63:0] r1_tx_tdata_bos; wire r1_tx_tlast_bos, r1_tx_tvalid_bos, r1_tx_tready_bos;   
   wire [63:0] r1_tx_tdata_0; wire r1_tx_tlast_0, r1_tx_tvalid_0, r1_tx_tready_0;
   wire [63:0] r1_tx_tdata_0s; wire r1_tx_tlast_0s, r1_tx_tvalid_0s, r1_tx_tready_0s;   
   wire [63:0] r1_tx_tdata_1; wire r1_tx_tlast_1, r1_tx_tvalid_1, r1_tx_tready_1;
   wire [63:0] r1_tx_tdata_1s; wire r1_tx_tlast_1s, r1_tx_tvalid_1s, r1_tx_tready_1s;   
   wire [63:0] r1_tx_tdata_2; wire r1_tx_tlast_2, r1_tx_tvalid_2, r1_tx_tready_2;
   wire [63:0] r1_tx_tdata_2s; wire r1_tx_tlast_2s, r1_tx_tvalid_2s, r1_tx_tready_2s;
   wire [63:0] r1_tx_tdata_bis; wire r1_tx_tlast_bis, r1_tx_tvalid_bis, r1_tx_tready_bis;
   
   axi_fifo_short #(.WIDTH(65)) tx_fifo1_bos
     (
      .clk(bus_clk), .reset(bus_rst), .clear(1'b0),
      .i_tdata({r1_tx_tlast_bo,r1_tx_tdata_bo}), .i_tvalid(r1_tx_tvalid_bo), .i_tready(r1_tx_tready_bo),
      .o_tdata({r1_tx_tlast_bos,r1_tx_tdata_bos}), .o_tvalid(r1_tx_tvalid_bos), .o_tready(r1_tx_tready_bos),
      .space(), .occupied()
      );

   axi_fifo #(.WIDTH(65), .SIZE(`SRAM_FIFO_SIZE-2)) tx_fifo1_0
     (.clk(bus_clk), .reset(bus_rst), .clear(1'b0),
      .i_tdata({r1_tx_tlast_bos,r1_tx_tdata_bos}), .i_tvalid(r1_tx_tvalid_bos), .i_tready(r1_tx_tready_bos),
      .o_tdata({r1_tx_tlast_0,r1_tx_tdata_0}), .o_tvalid(r1_tx_tvalid_0), .o_tready(r1_tx_tready_0),
      .space(), .occupied());

   axi_fifo_short #(.WIDTH(65)) tx_fifo1_0s
     (
      .clk(bus_clk), .reset(bus_rst), .clear(1'b0),
      .i_tdata({r1_tx_tlast_0,r1_tx_tdata_0}), .i_tvalid(r1_tx_tvalid_0), .i_tready(r1_tx_tready_0),
      .o_tdata({r1_tx_tlast_0s,r1_tx_tdata_0s}), .o_tvalid(r1_tx_tvalid_0s), .o_tready(r1_tx_tready_0s),
      .space(), .occupied()
      );
   
   axi_fifo #(.WIDTH(65), .SIZE(`SRAM_FIFO_SIZE-2)) tx_fifo1_1
     (.clk(bus_clk), .reset(bus_rst), .clear(1'b0),
      .i_tdata({r1_tx_tlast_0s,r1_tx_tdata_0s}), .i_tvalid(r1_tx_tvalid_0s), .i_tready(r1_tx_tready_0s),
      .o_tdata({r1_tx_tlast_1,r1_tx_tdata_1}), .o_tvalid(r1_tx_tvalid_1), .o_tready(r1_tx_tready_1),
      .space(), .occupied());

   axi_fifo_short #(.WIDTH(65)) tx_fifo1_1s
     (
      .clk(bus_clk), .reset(bus_rst), .clear(1'b0),
      .i_tdata({r1_tx_tlast_1,r1_tx_tdata_1}), .i_tvalid(r1_tx_tvalid_1), .i_tready(r1_tx_tready_1),
      .o_tdata({r1_tx_tlast_1s,r1_tx_tdata_1s}), .o_tvalid(r1_tx_tvalid_1s), .o_tready(r1_tx_tready_1s),
      .space(), .occupied()
      );
      
   axi_fifo #(.WIDTH(65), .SIZE(`SRAM_FIFO_SIZE-2)) tx_fifo1_2
     (.clk(bus_clk), .reset(bus_rst), .clear(1'b0),
      .i_tdata({r1_tx_tlast_1s,r1_tx_tdata_1s}), .i_tvalid(r1_tx_tvalid_1s), .i_tready(r1_tx_tready_1s),
      .o_tdata({r1_tx_tlast_2,r1_tx_tdata_2}), .o_tvalid(r1_tx_tvalid_2), .o_tready(r1_tx_tready_2),
      .space(), .occupied());
   
   axi_fifo_short #(.WIDTH(65)) tx_fifo1_2s
     (
      .clk(bus_clk), .reset(bus_rst), .clear(1'b0),
      .i_tdata({r1_tx_tlast_2,r1_tx_tdata_2}), .i_tvalid(r1_tx_tvalid_2), .i_tready(r1_tx_tready_2),
      .o_tdata({r1_tx_tlast_2s,r1_tx_tdata_2s}), .o_tvalid(r1_tx_tvalid_2s), .o_tready(r1_tx_tready_2s),
      .space(), .occupied()
      );
   
   axi_fifo #(.WIDTH(65), .SIZE(`SRAM_FIFO_SIZE-2)) tx_fifo1_3
     (.clk(bus_clk), .reset(bus_rst), .clear(1'b0),
      .i_tdata({r1_tx_tlast_2s,r1_tx_tdata_2s}), .i_tvalid(r1_tx_tvalid_2s), .i_tready(r1_tx_tready_2s),
      .o_tdata({r1_tx_tlast_bis,r1_tx_tdata_bis}), .o_tvalid(r1_tx_tvalid_bis), .o_tready(r1_tx_tready_bis),
      .space(), .occupied());
   
   axi_fifo_short #(.WIDTH(65)) tx_fifo1_bis
     (
      .clk(bus_clk), .reset(bus_rst), .clear(1'b0),
      .i_tdata({r1_tx_tlast_bis,r1_tx_tdata_bis}), .i_tvalid(r1_tx_tvalid_bis), .i_tready(r1_tx_tready_bis),
      .o_tdata({r1_tx_tlast_bi,r1_tx_tdata_bi}), .o_tvalid(r1_tx_tvalid_bi), .o_tready(r1_tx_tready_bi),
      .space(), .occupied()
      );


 
`endif //  `ifndef NO_DRAM_FIFOS

   //
   // Chipscope for upto 192 signals
   //
/* -----\/----- EXCLUDED -----\/-----

   reg [255:0] TRIG0;
   reg [63:0] TRIG1;

   wire [35:0] CONTROL0;
   wire [35:0] CONTROL1;
 -----/\----- EXCLUDED -----/\----- */

/* -----\/----- EXCLUDED -----\/-----
   always @(posedge ddr3_axi_clk)
     TRIG0 <= {
		s01_axi_araddr[31:0], // 246-215
 -----/\----- EXCLUDED -----/\----- */
//	      s01_axi_arlen,       // 239-232
//	      s01_axi_rresp,       // 231-230
//	      s01_axi_rlast,       // 229
//	      debug_axi_dram_fifo[13:0], // 228-215
//	      debug_axi_dram_fifo[31:0], // 246-215
//	      bist_fail,           // 214
//	      bist_start,          // 213
//	      bist_done,           // 212
//	      bus_rst,             // 211
/* -----\/----- EXCLUDED -----\/-----
	       4'h0, // 214-211
	      s01_axi_awvalid,	   // 210
	      s01_axi_awready,	   // 209
	      s01_axi_wvalid,	   // 208
	      s01_axi_wready,	   // 207
	      s01_axi_bvalid,	   // 206
	      s01_axi_bready,	   // 205
	      s01_axi_arvalid,	   // 204
	      s01_axi_arready,	   // 203
	      s01_axi_rvalid,	   // 202
	      s01_axi_rready,	   // 201
 -----/\----- EXCLUDED -----/\----- */
/* -----\/----- EXCLUDED -----\/-----
	      r1o_tvalid,          // 200
	      r1o_tready,          // 199
	      r1o_tlast,           // 198
 -----/\----- EXCLUDED -----/\----- */
/* -----\/----- EXCLUDED -----\/-----
	       3'h0, // 200-198
 -----/\----- EXCLUDED -----/\----- */
//	      r1o_tdata[63:0],     // 197-134
/* -----\/----- EXCLUDED -----\/-----
	      debug_axi_dram_fifo[197:0]  // 197-0
 -----/\----- EXCLUDED -----/\----- */

/* -----\/----- EXCLUDED -----\/-----
	      r1_tx_tvalid_bo,     // 133
	      r1_tx_tready_bo,     // 132
	      r1_tx_tlast_bo,      // 131
	      r1_tx_tdata_bo[63:0],// 130-67
	      r1_tx_tvalid_bi,     // 66
	      r1_tx_tready_bi,     // 65
	      r1_tx_tlast_bi,      // 64
	      r1_tx_tdata_bi[63:0] // 63-0
 -----/\----- EXCLUDED -----/\----- */
/* -----\/----- EXCLUDED -----\/-----

	      bist_i_tvalid,     // 133
	      bist_i_tready,     // 132
	      bist_i_tlast,      // 131
	      bist_i_tdata[63:0],// 130-67
	      bist_o_tvalid,     // 66
	      bist_o_tready,     // 65
	      bist_o_tlast,      // 64
	      bist_o_tdata[63:0] // 63-0
 -----/\----- EXCLUDED -----/\----- */
//	       };
/* -----\/----- EXCLUDED -----\/-----
   always @(posedge bus_clk)
     TRIG0 <= {
//	      s01_axi_arlen,       // 239-232
//	      s01_axi_rresp,       // 231-230
//	      s01_axi_rlast,       // 229
//	      debug_axi_dram_fifo[13:0], // 228-215
	      debug_axi_dram_fifo[31:0], // 246-215
	      bist_fail,           // 214
	      bist_start,          // 213
	      bist_done,           // 212
	      bus_rst,             // 211
	      s01_axi_awvalid,	   // 210
	      s01_axi_awready,	   // 209
	      s01_axi_wvalid,	   // 208
	      s01_axi_wready,	   // 207
	      s01_axi_bvalid,	   // 206
	      s01_axi_bready,	   // 205
	      s01_axi_arvalid,	   // 204
	      s01_axi_arready,	   // 203
	      s01_axi_rvalid,	   // 202
	      s01_axi_rready,	   // 201
	      r1o_tvalid,          // 200
	      r1o_tready,          // 199
	      r1o_tlast,           // 198
	      r1o_tdata[63:0],     // 197-134
	      bist_i_tvalid,     // 133
	      bist_i_tready,     // 132
	      bist_i_tlast,      // 131
	      bist_i_tdata[63:0],// 130-67
	      bist_o_tvalid,     // 66
	      bist_o_tready,     // 65
	      bist_o_tlast,      // 64
	      bist_o_tdata[63:0] // 63-0
	       };
 -----/\----- EXCLUDED -----/\----- */
/* -----\/----- EXCLUDED -----\/-----

  always @(posedge ddr3_axi_clk)
     TRIG1 <= {
	       // Write Addr
	       ddr3_axi_awid[3:0],   // 57-54
	       ddr3_axi_awlen[7:0],  // 53-46
	       ddr3_axi_awsize[2:0], // 45-43
	       ddr3_axi_awcache[3:0],// 42-39
               ddr3_axi_awvalid,     // 38
               ddr3_axi_awready,     // 37
	       // Write Data
	       ddr3_axi_wlast,       // 36
               ddr3_axi_wvalid,      // 35
               ddr3_axi_wready,      // 34
	       // Write Resp
	       ddr3_axi_bready,      // 33
	       ddr3_axi_bid[3:0],    // 32-29
	       ddr3_axi_bvalid,      // 28
	       // Read Addr
	       ddr3_axi_arid[3:0],   // 27-24
	       ddr3_axi_arlen[7:0],  // 23-16
	       ddr3_axi_arsize[2:0], // 15-13
	       ddr3_axi_arcache[3:0],// 12-9
	       ddr3_axi_arvalid,     // 8
               ddr3_axi_arready,     // 7
	       // Read Data
	       ddr3_axi_rid[3:0],    // 6-3
	       ddr3_axi_rlast,       // 2
	       ddr3_axi_rvalid,      // 1
	       ddr3_axi_rready       // 0
	       };
 -----/\----- EXCLUDED -----/\----- */

/* -----\/----- EXCLUDED -----\/-----

   chipscope_ila chipscope_ila_i0
     (
      .CONTROL(CONTROL0), // INOUT BUS [35:0]
      .CLK(ddr3_axi_clk), // IN
      .TRIG0(TRIG0 ) // IN BUS [255:0]
      );

   chipscope_ila_64 chipscope_ila_i1
     (
      .CONTROL(CONTROL1), // INOUT BUS [35:0]
      .CLK(ddr3_axi_clk), // IN
      .TRIG0(TRIG1 ) // IN BUS [63:0]
      );

   chipscope_icon_2port chipscope_icon_i0
     (
      .CONTROL0(CONTROL0), // INOUT BUS [35:0]
      .CONTROL1(CONTROL1) // INOUT BUS [35:0]
      );
 -----/\----- EXCLUDED -----/\----- */

endmodule // x300_core

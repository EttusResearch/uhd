//
// Copyright 2013 Ettus Research LLC
//


/***********************************************************
 * B200 Module Declaration
 **********************************************************/
module b200 (
   // SPI Interfaces
   output cat_ce,
   input cat_miso,
   output cat_mosi,
   output cat_sclk,

   input fx3_ce,
   output fx3_miso,
   input fx3_mosi,
   input fx3_sclk,

   output pll_ce,
   output pll_mosi,
   output pll_sclk,

   // UART
   input FPGA_RXD0,
   input FPGA_TXD0,

   // Catalina Controls
   output codec_enable,
   output codec_en_agc,
   output codec_reset,
   output codec_sync,
   output codec_txrx,
   output [3:0] codec_ctrl_in,      // These should be outputs
   input [7:0] codec_ctrl_out,      // MUST BE INPUT

   // Catalina Data
   input codec_data_clk_p,          // Clock from CAT (RX)
   output codec_fb_clk_p,           // Clock to CAT (TX)
   input [11:0] rx_codec_d,
   output [11:0] tx_codec_d,
   input rx_frame_p,
   output tx_frame_p,

   input cat_clkout_fpga,

   //always on 40MHz clock
   input codec_main_clk_p,
   input codec_main_clk_n,

   // Debug Bus
   output [31:0] debug,
   output [1:0] debug_clk,

   // GPIF, FX3 Slave FIFO
   output IFCLK,        // pclk
   input FX3_EXTINT,
   output GPIF_CTL0,    // n_slcs
   output GPIF_CTL1,    // n_slwr
   output GPIF_CTL2,    // n_sloe
   output GPIF_CTL3,    // n_slrd
   output GPIF_CTL7,    // n_pktend
   input GPIF_CTL4,     // slfifo_flags[0]
   input GPIF_CTL5,     // slfifo_flags[1]
   input GPIF_CTL6,     // slfifo_flags[2]
   input GPIF_CTL8,     // slfifo_flags[3]
   output GPIF_CTL11,   // slfifo_addr[1]
   output GPIF_CTL12,   // slfifo_addr[0]
   inout [31:0] GPIF_D,
   input GPIF_CTL9,     // global_reset

   // GPS
   input gps_lock,
   output gps_rxd,
   input gps_txd,
   input gps_txd_nmea,

   // LEDS
   output LED_RX1,
   output LED_RX2,
   output LED_TXRX1_RX,
   output LED_TXRX1_TX,
   output LED_TXRX2_RX,
   output LED_TXRX2_TX,

   // Misc Hardware Control
   output ref_sel,
   input pll_lock,
   input FPGA_CFG_CS,           // Driven by FX3 gpio.
   input AUX_PWR_ON,            // Driven by FX3 gpio.

   // PPS
   input PPS_IN_EXT,
   input PPS_IN_INT,

   // RF Hardware Control
   output SFDX1_RX,
   output SFDX1_TX,
   output SFDX2_RX,
   output SFDX2_TX,
   output SRX1_RX,
   output SRX1_TX,
   output SRX2_RX,
   output SRX2_TX,
   output tx_bandsel_a,
   output tx_bandsel_b,
   output tx_enable1,
   output tx_enable2,
   output rx_bandsel_a,
   output rx_bandsel_b,
   output rx_bandsel_c
   );

    wire reset_global = GPIF_CTL9;

    ///////////////////////////////////////////////////////////////////////
    // generate clocks from always on codec main clk
    ///////////////////////////////////////////////////////////////////////
    wire bus_clk, gpif_clk, radio_clk;
    wire locked;
    b200_clk_gen gen_clks
    (
        .CLK_IN1_40_P(codec_main_clk_p), .CLK_IN1_40_N(codec_main_clk_n),
        .CLK_OUT1_40_int(), .CLK_OUT2_100_gpif(gpif_clk), .CLK_OUT3_100_bus(bus_clk),
        .RESET(reset_global), .LOCKED(locked)
    );

    //hold-off logic for clocks ready
    reg [15:0] clocks_ready_count;
    reg clocks_ready;
    always @(posedge bus_clk or posedge reset_global or negedge locked) begin
        if (reset_global | !locked) begin
            clocks_ready_count <= 16'b0;
            clocks_ready <= 1'b0;
        end
        else if (!clocks_ready) begin
            clocks_ready_count <= clocks_ready_count + 1'b1;
            clocks_ready <= (clocks_ready_count == 16'hffff);
        end
    end

    ///////////////////////////////////////////////////////////////////////
    // drive output clocks
    ///////////////////////////////////////////////////////////////////////
    wire [1:0] debug_clk_int;
    //S6CLK2PIN S6CLK2PIN_dbg0 (.I(debug_clk_int[0]), .O(debug_clk[0]));
    //S6CLK2PIN S6CLK2PIN_dbg1 (.I(debug_clk_int[1]), .O(debug_clk[1]));
    assign      debug_clk[1:0] = 2'b0;
    S6CLK2PIN S6CLK2PIN_gpif (.I(gpif_clk), .O(IFCLK));
   
    ///////////////////////////////////////////////////////////////////////
    // Create sync reset signals
    ///////////////////////////////////////////////////////////////////////
    wire gpif_rst, bus_rst, radio_rst;
    reset_sync gpif_sync(.clk(gpif_clk), .reset_in(!clocks_ready), .reset_out(gpif_rst));
    reset_sync bus_sync(.clk(bus_clk), .reset_in(!clocks_ready), .reset_out(bus_rst));
    reset_sync radio_sync(.clk(radio_clk), .reset_in(!clocks_ready), .reset_out(radio_rst));

   ///////////////////////////////////////////////////////////////////////
   // CODEC capture/gen
   ///////////////////////////////////////////////////////////////////////
   wire [31:0] rx_data1, rx_data2;
   wire [31:0] tx_data1, tx_data2;
   wire mimo, codec_arst;

   catcodec_ddr_cmos catcodec
   (
        .radio_clk(radio_clk), .arst(codec_arst), .mimo(mimo),
        .rx1(rx_data1), .rx2(rx_data2), .tx1(tx_data1), .tx2(tx_data2),
        .rx_clk(codec_data_clk_p), .rx_frame(rx_frame_p), .rx_d(rx_codec_d),
        .tx_clk(codec_fb_clk_p), .tx_frame(tx_frame_p), .tx_d(tx_codec_d)
   );

   ///////////////////////////////////////////////////////////////////////
   // SPI connections
   ///////////////////////////////////////////////////////////////////////
   wire mosi, miso, sclk; wire [7:0]  sen;
   assign cat_ce = sen[0] & fx3_ce;
   assign cat_mosi = (~sen[0] & mosi) | (~fx3_ce & fx3_mosi);
   assign cat_sclk = (~sen[0] & sclk) | (~fx3_ce & fx3_sclk);
   assign miso = cat_miso;
   assign fx3_miso = ~fx3_ce & cat_miso;
   assign pll_ce = sen[1];
   assign pll_mosi = ~sen[1] & mosi;
   assign pll_sclk = ~sen[1] & sclk;

    ///////////////////////////////////////////////////////////////////////
    // bus signals
    ///////////////////////////////////////////////////////////////////////
    wire [63:0] ctrl_tdata, resp_tdata, rx_tdata, tx_tdata;
    wire ctrl_tlast, resp_tlast, rx_tlast, tx_tlast;
    wire ctrl_tvalid, resp_tvalid, rx_tvalid, tx_tvalid;
    wire ctrl_tready, resp_tready, rx_tready, tx_tready;

    ///////////////////////////////////////////////////////////////////////
    // loopback testers
    ///////////////////////////////////////////////////////////////////////
    /*
    axi_fifo #(.WIDTH(65), .SIZE(13)) f0
    (
        .clk(bus_clk), .reset(bus_rst), .clear(1'b0),
        .i_tdata({ctrl_tlast, ctrl_tdata}), .i_tvalid(ctrl_tvalid), .i_tready(ctrl_tready), .space(),
        .o_tdata({resp_tlast, resp_tdata}), .o_tvalid(resp_tvalid), .o_tready(resp_tready), .occupied()
    );
    //*/

    /*
    axi_fifo #(.WIDTH(65), .SIZE(13)) f1
    (
        .clk(bus_clk), .reset(bus_rst), .clear(1'b0),
        .i_tdata({tx_tlast, tx_tdata}), .i_tvalid(tx_tvalid), .i_tready(tx_tready), .space(),
        .o_tdata({rx_tlast, rx_tdata}), .o_tvalid(rx_tvalid), .o_tready(rx_tready), .occupied()
    );
    //*/

    ///////////////////////////////////////////////////////////////////////
    // frontend assignments
    ///////////////////////////////////////////////////////////////////////
    wire [31:0] debug_radio;

    wire [31:0] fe_atr1, fe_atr2;
    assign {tx_enable1, SFDX1_RX, SFDX1_TX, SRX1_RX, SRX1_TX, LED_RX1, LED_TXRX1_RX, LED_TXRX1_TX} = fe_atr1[7:0];
    assign {tx_enable2, SFDX2_RX, SFDX2_TX, SRX2_RX, SRX2_TX, LED_RX2, LED_TXRX2_RX, LED_TXRX2_TX} = fe_atr2[7:0];

    wire [31:0] misc_outs; reg [31:0] misc_outs_r;
    always @(posedge bus_clk) misc_outs_r <= misc_outs; //register misc ios to ease routing to flop
    assign { tx_bandsel_a, tx_bandsel_b, rx_bandsel_a, rx_bandsel_b, rx_bandsel_c, codec_arst, mimo, ref_sel } = misc_outs_r[7:0];

    assign codec_ctrl_in = 4'b1;
    assign codec_en_agc = 1'b1;
    assign codec_txrx = 1'b1;
    assign codec_enable = 1'b1;
    assign codec_reset = !reset_global;   // Codec Reset // RESETB // Operates active-low
    assign codec_sync = 1'b0;

    ///////////////////////////////////////////////////////////////////////
    // b200 core
    ///////////////////////////////////////////////////////////////////////
    b200_core #(.EXTRA_BUFF_SIZE(12)) b200_xfusion_core
    (
        .bus_clk(bus_clk), .bus_rst(bus_rst),
        .tx_tdata(tx_tdata), .tx_tlast(tx_tlast), .tx_tvalid(tx_tvalid), .tx_tready(tx_tready),
        .rx_tdata(rx_tdata), .rx_tlast(rx_tlast),  .rx_tvalid(rx_tvalid), .rx_tready(rx_tready),
        .ctrl_tdata(ctrl_tdata), .ctrl_tlast(ctrl_tlast),  .ctrl_tvalid(ctrl_tvalid), .ctrl_tready(ctrl_tready),
        .resp_tdata(resp_tdata), .resp_tlast(resp_tlast),  .resp_tvalid(resp_tvalid), .resp_tready(resp_tready),

        .radio_clk(radio_clk), .radio_rst(radio_rst),
        .rx0(rx_data2), .rx1(rx_data1),
        .tx0(tx_data2), .tx1(tx_data1),
        .fe_atr0(fe_atr2), .fe_atr1(fe_atr1),
        .pps_int(PPS_IN_INT), .pps_ext(PPS_IN_EXT),

        .rxd(gps_txd), .txd(gps_rxd),
        .sclk(sclk), .sen(sen), .mosi(mosi), .miso(miso),
        .rb_misc({31'b0, pll_lock}), .misc_outs(misc_outs)
    );

    ///////////////////////////////////////////////////////////////////////
    // GPIF2
    ///////////////////////////////////////////////////////////////////////
    wire [31:0] debug_gpif;

    gpif2_slave_fifo32 #(.DATA_RX_FIFO_SIZE(14), .DATA_TX_FIFO_SIZE(14)) slave_fifo32
    (
        .gpif_clk(gpif_clk), .gpif_rst(gpif_rst), .gpif_enb(1'b1),
        .gpif_ctl({GPIF_CTL8, GPIF_CTL6, GPIF_CTL5, GPIF_CTL4}), .fifoadr({GPIF_CTL11,GPIF_CTL12}),
        .slwr(GPIF_CTL1), .sloe(GPIF_CTL2), .slcs(GPIF_CTL0), .slrd(GPIF_CTL3), .pktend(GPIF_CTL7),
        .gpif_d(GPIF_D),

        .fifo_clk(bus_clk), .fifo_rst(bus_rst),
        .tx_tdata(tx_tdata), .tx_tlast(tx_tlast), .tx_tvalid(tx_tvalid), .tx_tready(tx_tready),
        .rx_tdata(rx_tdata), .rx_tlast(rx_tlast),  .rx_tvalid(rx_tvalid), .rx_tready(rx_tready),
        .ctrl_tdata(ctrl_tdata), .ctrl_tlast(ctrl_tlast),  .ctrl_tvalid(ctrl_tvalid), .ctrl_tready(ctrl_tready),
        .resp_tdata(resp_tdata), .resp_tlast(resp_tlast),  .resp_tvalid(resp_tvalid), .resp_tready(resp_tready),

        .debug(debug_gpif)
    );

   ///////////////////////////////////////////////////////////////////////
   // Debug port
   ///////////////////////////////////////////////////////////////////////
   assign debug_clk_int = { 1'b0, 1'b0 };
   assign debug = 32'b0;

endmodule // B200

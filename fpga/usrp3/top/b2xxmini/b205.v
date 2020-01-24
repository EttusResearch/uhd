//
// Copyright 2015 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//

/***********************************************************
 * B205 Module Declaration
 **********************************************************/
module b205 (
   // AD9364 - SPI Interface:
   output 	 CAT_SPI_EN,   // Enable
   input 	 CAT_SPI_DO,   // MISO
   output 	 CAT_SPI_DI,   // MOSI
   output 	 CAT_SPI_CLK,  // SPI Clk

   // AD9364 - Control:
   output 	 CAT_EN,
   output 	 CAT_EN_AGC,
   output 	 CAT_RESETn,
   output 	 CAT_TXnRX,
   output [3:0]  CAT_CTL_IN,  // These should be outputs
   input [7:0] 	 CAT_CTL_OUT, // MUST BE INPUT

   // AD9364 - Data:
   input 	 CAT_DCLK_P,  // Clock from AD9364 (RX)
   output 	 CAT_FBCLK_P, // Clock to AD9364 (TX)
   output    CAT_FBCLK_N,
   input [11:0]  CAT_P0_D,  // RX data is on Port 0
   output [11:0] CAT_P1_D,  // TX data is on Port 1
   input 	 CAT_RX_FR_P,
   output 	 CAT_TX_FR_P,
   output    CAT_TX_FR_N,

   // AD9364 - Always on 40MHz clock:
   input 	 CLK_40MHz_FPGA,

   // GPIF, FX3 Slave FIFO
   output 	 FX3_PCLK,  // pclk
   output 	 FX3_CTL0,  // n_slcs
   output 	 FX3_CTL1,  // n_slwr
   output 	 FX3_CTL2,  // n_sloe
   output 	 FX3_CTL3,  // n_slrd
   output 	 FX3_CTL7,  // n_pktend
   input 	 FX3_CTL4,  // slfifo_flags[0]
   input 	 FX3_CTL5,  // slfifo_flags[1]
   input 	 FX3_CTL6,  // Serial settings bus from FX3. SDA
   input 	 FX3_CTL8,  // Serial settings bus from FX3. SCL
   output 	 FX3_CTL11, // slfifo_addr[1]
   output 	 FX3_CTL12, // slfifo_addr[0]
   inout [31:0]  FX3_DQ,
   input 	 FX3_CTL9, // global_reset

   // LEDs
   output        cLED_TRX_G,
   output        cLED_TRX_B,
   output        cLED_TRX_R,
   output        cLED_RX2_G,
   output        cLED_RX2_B,
   output        cLED_RX2_R,
   output        cLED_S0,
   output        cLED_S1,

   // GPIO
   inout [7:0] 	 fp_gpio,

   // PPS or 10 MHz (need to choose from SW)
   input         PPS_IN,
   input         CLKIN_10MHz,
   output        CLKIN_10MHz_REQ,

   // Clock disciplining / AD5662 controls
   output        CLK_40M_DAC_nSYNC,
   output        CLK_40M_DAC_SCLK,
   output        CLK_40M_DAC_DIN,

   // RF Hardware Control
   output        cFE_SEL_TRX_TX, // Select TX/RX port for Tx
   output        cFE_SEL_TRX_RX, // Select TX/RX port for Rx
   output        cFE_SEL_RX_TRX, // Select TX/RX port for Rx
   output        cFE_SEL_RX_RX2, // Select RX2 port for Rx
   output        cTXDRV_PWEN     // Tx PA enable

   );

    wire reset_global = FX3_CTL9;

    ///////////////////////////////////////////////////////////////////////
    // generate clocks from always on codec main clk
    ///////////////////////////////////////////////////////////////////////
    wire bus_clk, radio_clk;
    wire locked;
    wire int_40mhz;
    wire ref_pll_clk;
    b205_clk_gen gen_clks
    (
        .CLK_IN1_40(CLK_40MHz_FPGA), // No differential input!
        .CLK_OUT1_40_int(int_40mhz), .CLK_OUT2_100_bus(bus_clk), .CLK_OUT3_200_ref_pll(ref_pll_clk),
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
    ODDR2 #(
        .DDR_ALIGNMENT("NONE"), // to "NONE", "C0" or "C1"
        .INIT(1'b0),            // output to 1'b0 or 1'b1
        .SRTYPE("ASYNC")        // set/reset "SYNC" or "ASYNC"
    )
    ODDR2_S6CLK2PIN
    (
        .Q(FX3_PCLK),   // 1-bit DDR output data
        .C0(bus_clk),  // 1-bit clock input
        .C1(~bus_clk), // 1-bit clock input
        .CE(1'b1),      // 1-bit clock enable input
        .D0(1'b1),      // 1-bit data input (associated with C0)
        .D1(1'b0),      // 1-bit data input (associated with C1)
        .R(1'b0),       // 1-bit reset input
        .S(1'b0)        // 1-bit set input
    );

    ///////////////////////////////////////////////////////////////////////
    // Create sync reset signals
    ///////////////////////////////////////////////////////////////////////
    wire bus_rst, ref_pll_rst, radio_rst;
    reset_sync bus_sync(.clk(bus_clk), .reset_in(!clocks_ready), .reset_out(bus_rst));
    reset_sync ref_pll_sync(.clk(ref_pll_clk), .reset_in(!clocks_ready), .reset_out(ref_pll_rst));
    reset_sync radio_sync(.clk(radio_clk), .reset_in(!clocks_ready), .reset_out(radio_rst));

    ///////////////////////////////////////////////////////////////////////
    // reference clock PLL
    ///////////////////////////////////////////////////////////////////////
    wire ref_sel;
    wire ext_ref_is_pps;
    wire ext_ref_locked;
    wire ext_ref = ext_ref_is_pps ? PPS_IN : ref_sel ? CLKIN_10MHz : 1'b0;
    b205_ref_pll ref_pll
    (
        .reset(ref_pll_rst),
        .clk(ref_pll_clk),
        .refclk(int_40mhz),
        .ref(ext_ref),
        .locked(ext_ref_locked),
        .sclk(CLK_40M_DAC_SCLK),
        .mosi(CLK_40M_DAC_DIN),
        .sync_n(CLK_40M_DAC_nSYNC)
    );
    assign CLKIN_10MHz_REQ = ref_sel;

   ///////////////////////////////////////////////////////////////////////
   // AD9364 I/O
   ///////////////////////////////////////////////////////////////////////
   wire [31:0] rx_data;
   wire [31:0] tx_data;

   b205_io b205_io_i0
     (
      .reset(reset_global),
      // Baseband sample interface
      .radio_clk(radio_clk),
      .rx_i0(rx_data[31:20]),
      .rx_q0(rx_data[15:4]),
      .tx_i0(tx_data[31:20]),
      .tx_q0(tx_data[15:4]),
       // Catalina interface
      .rx_clk(CAT_DCLK_P),
      .rx_frame(CAT_RX_FR_P),
      .rx_data(CAT_P0_D),

      .tx_clk(CAT_FBCLK_P),
      .tx_frame(CAT_TX_FR_P),
      .tx_data(CAT_P1_D)
      );

   assign {rx_data[19:16],rx_data[3:0]} = 8'h0;
   assign CAT_FBCLK_N = 1'b0;
   assign CAT_TX_FR_N = 1'b0;

   ///////////////////////////////////////////////////////////////////////
   // SPI connections
   ///////////////////////////////////////////////////////////////////////
   wire mosi,  miso, sclk;
   wire [7:0]  sen;

   // AD9364 Slave (it's the only slave for B205)
   assign CAT_SPI_EN   =  sen[0];
   assign CAT_SPI_DI   = ~sen[0] & mosi;
   assign CAT_SPI_CLK  = ~sen[0] & sclk;
   assign miso         = CAT_SPI_DO;

    ///////////////////////////////////////////////////////////////////////
    // bus signals
    ///////////////////////////////////////////////////////////////////////
    wire [63:0] ctrl_tdata, resp_tdata, rx_tdata, tx_tdata;
    wire ctrl_tlast, resp_tlast, rx_tlast, tx_tlast;
    wire ctrl_tvalid, resp_tvalid, rx_tvalid, tx_tvalid;
    wire ctrl_tready, resp_tready, rx_tready, tx_tready;

    ///////////////////////////////////////////////////////////////////////
    // frontend assignments
    ///////////////////////////////////////////////////////////////////////
    wire [7:0] fe_gpio_out;
    reg [7:0] fe_gpio_reg;
    
    //Register in IOB
    always @(posedge radio_clk)
      fe_gpio_reg <= fe_gpio_out;
    
    assign {cTXDRV_PWEN, cFE_SEL_RX_RX2, cFE_SEL_TRX_TX, cFE_SEL_RX_TRX, cFE_SEL_TRX_RX} = fe_gpio_reg[7:3];
    assign cLED_TRX_R = ~fe_gpio_reg[0];
    assign cLED_TRX_G = ~fe_gpio_reg[1];
    assign cLED_TRX_B = 1'b1;
    assign cLED_RX2_R = 1'b1;
    assign cLED_RX2_G = ~fe_gpio_reg[2];
    assign cLED_RX2_B = 1'b1;
    assign cLED_S0 = ~ext_ref_locked;
    assign cLED_S1 = ~(ext_ref);

    wire [31:0] misc_outs;
    reg [31:0] misc_outs_r;
    always @(posedge bus_clk) misc_outs_r <= misc_outs; //register misc ios to ease routing to flop
    assign ref_sel = misc_outs_r[0];

    wire codec_arst = misc_outs_r[2];

    assign CAT_CTL_IN = 4'b1;
    assign CAT_EN_AGC = 1'b1;
    assign CAT_TXnRX  = 1'b1;
    assign CAT_EN     = 1'b1;
    assign CAT_RESETn = ~codec_arst;   // Codec Reset // RESETB // Operates active-low

    ///////////////////////////////////////////////////////////////////////
    // b205 core
    ///////////////////////////////////////////////////////////////////////
    wire [7:0] fp_gpio_in, fp_gpio_out, fp_gpio_ddr;
    b205_core #(.EXTRA_BUFF_SIZE(12)) b205_core
    (
        .bus_clk(bus_clk), .bus_rst(bus_rst),
        .tx_tdata(tx_tdata), .tx_tlast(tx_tlast), .tx_tvalid(tx_tvalid), .tx_tready(tx_tready),
        .rx_tdata(rx_tdata), .rx_tlast(rx_tlast),  .rx_tvalid(rx_tvalid), .rx_tready(rx_tready),
        .ctrl_tdata(ctrl_tdata), .ctrl_tlast(ctrl_tlast),  .ctrl_tvalid(ctrl_tvalid), .ctrl_tready(ctrl_tready),
        .resp_tdata(resp_tdata), .resp_tlast(resp_tlast),  .resp_tvalid(resp_tvalid), .resp_tready(resp_tready),

        .radio_clk(radio_clk), .radio_rst(radio_rst),
        .rx0(rx_data),
        .tx0(tx_data),
        .fe_gpio_out(fe_gpio_out),
        .fp_gpio_in(fp_gpio_in), .fp_gpio_out(fp_gpio_out), .fp_gpio_ddr(fp_gpio_ddr),
        .ext_ref_is_pps(ext_ref_is_pps),
        .pps_ext(PPS_IN),

        .sclk(sclk), .sen(sen), .mosi(mosi), .miso(miso),
        .rb_misc({31'b0,ext_ref_locked}), .misc_outs(misc_outs),

        .lock_signals(CAT_CTL_OUT[7:6]),

        .debug()
    );

    gpio_atr_io #(.WIDTH(8)) gpio_atr_io_inst (
        .clk(radio_clk), .gpio_pins(fp_gpio),
        .gpio_ddr(fp_gpio_ddr), .gpio_out(fp_gpio_out), .gpio_in(fp_gpio_in)
    );

    ///////////////////////////////////////////////////////////////////////
    // GPIF2
    ///////////////////////////////////////////////////////////////////////
    gpif2_slave_fifo32 #(.DATA_RX_FIFO_SIZE(13), .DATA_TX_FIFO_SIZE(13)) slave_fifo32
    (
        .gpif_clk(bus_clk), .gpif_rst(bus_rst), .gpif_enb(1'b1),
        .gpif_ctl({FX3_CTL8, FX3_CTL6, FX3_CTL5, FX3_CTL4}), .fifoadr({FX3_CTL11, FX3_CTL12}),
        .slwr(FX3_CTL1), .sloe(FX3_CTL2), .slcs(FX3_CTL0), .slrd(FX3_CTL3), .pktend(FX3_CTL7),
        .gpif_d(FX3_DQ),

        .fifo_clk(bus_clk), .fifo_rst(bus_rst),
        .tx_tdata(tx_tdata), .tx_tlast(tx_tlast), .tx_tvalid(tx_tvalid), .tx_tready(tx_tready),
        .rx_tdata(rx_tdata), .rx_tlast(rx_tlast),  .rx_tvalid(rx_tvalid), .rx_tready(rx_tready),
        .ctrl_tdata(ctrl_tdata), .ctrl_tlast(ctrl_tlast),  .ctrl_tvalid(ctrl_tvalid), .ctrl_tready(ctrl_tready),
        .resp_tdata(resp_tdata), .resp_tlast(resp_tlast),  .resp_tvalid(resp_tvalid), .resp_tready(resp_tready),

        .debug()
    );

endmodule // B205

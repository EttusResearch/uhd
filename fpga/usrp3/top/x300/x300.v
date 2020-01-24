///////////////////////////////////
//
// Copyright 2016-2017 Ettus Research LLC
// Copyright 2017 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// NOTE: A set of precompiler directives configure the features in an FPGA build
// and are listed here. These should be set exclusively using the Makefile mechanism provided.
//
// SFP0_10GBE - Ethernet Port0 is configured for 10G (default is 1G)
// SFP1_10GBE - Ethernet Port1 is configured for 10G (default is 1G)
// DEBUG_UART - Adds 115kbaud UART to GPIO pins 10 & 11 for firmware debug
//
///////////////////////////////////

//Defines `LVFPGA_IFACE constants
`include "../../lib/io_port2/LvFpga_Chinch_Interface.vh"

module x300

  (
   ///////////////////////////////////
   //
   // Clock sources for main FPGA clocks
   //
   ///////////////////////////////////
   input FPGA_CLK_p,  input FPGA_CLK_n,
   input FPGA_125MHz_CLK,

   ///////////////////////////////////
   //
   // High Speed SPF+ signals and clocking
   //
   ///////////////////////////////////

`ifdef BUILD_1G
   input ETH_CLK_p, input ETH_CLK_n,
`endif

`ifdef BUILD_10G
   `define BUILD_10G_OR_AURORA
`endif
`ifdef BUILD_AURORA
   `define BUILD_10G_OR_AURORA
`endif
`ifdef BUILD_10G_OR_AURORA
   input XG_CLK_p, input XG_CLK_n,
`endif

   input SFP0_RX_p, input SFP0_RX_n,
   output SFP0_TX_p, output SFP0_TX_n,
   input SFP1_RX_p, input SFP1_RX_n,
   output SFP1_TX_p, output SFP1_TX_n,

   ///////////////////////////////////
   //
   // DRAM Interface
   //
   ///////////////////////////////////
   inout [31:0] ddr3_dq,     // Data pins. Input for Reads, Output for Writes.
   inout [3:0] ddr3_dqs_n,   // Data Strobes. Input for Reads, Output for Writes.
   inout [3:0] ddr3_dqs_p,
   //
   output [14:0] ddr3_addr,  // Address
   output [2:0] ddr3_ba,     // Bank Address
   output ddr3_ras_n,        // Row Address Strobe.
   output ddr3_cas_n,        // Column address select
   output ddr3_we_n,         // Write Enable
   output ddr3_reset_n,      // SDRAM reset pin.
   output [0:0] ddr3_ck_p,         // Differential clock
   output [0:0] ddr3_ck_n,
   output [0:0] ddr3_cke,    // Clock Enable
   output [0:0] ddr3_cs_n,         // Chip Select
   output [3:0] ddr3_dm,     // Data Mask [3] = UDM.U26, [2] = LDM.U26, ...
   output [0:0] ddr3_odt,    // On-Die termination enable.
   //
   input sys_clk_i,          // 100MHz clock source to generate DDR3 clocking.
   ///////////////////////////////////
   //
   // IOPORT2
   //
   ///////////////////////////////////

   //-- The IO_Port2 asynchronous handshaking pins
   input aIoResetIn_n,
   output aIoReadyOut,
   input aIoReadyIn,
   output aIoPort2Restart,
   input aStc3Gpio7,

   //-- The IO_Port2 high speed receiver pins
   input IoRxClock,
   input IoRxClock_n,
   input [15:0] irIoRxData,
   input [15:0] irIoRxData_n,
   input irIoRxHeader,
   input irIoRxHeader_n,

   //-- The IO_Port2 high speed transmitter interface pins
   output IoTxClock,
   output IoTxClock_n,
   output [15:0] itIoTxData,
   output [15:0] itIoTxData_n,
   output itIoTxHeader,
   output itIoTxHeader_n,

   output aIrq,

   ///////////////////////////////////
   //
   // ADC and DAC interfaces
   //
   ///////////////////////////////////

   input DB0_ADC_DCLK_P, input DB0_ADC_DCLK_N,
   input DB0_ADC_DA0_P, input DB0_ADC_DA0_N, input DB0_ADC_DB0_P, input DB0_ADC_DB0_N,
   input DB0_ADC_DA1_P, input DB0_ADC_DA1_N, input DB0_ADC_DB1_P, input DB0_ADC_DB1_N,
   input DB0_ADC_DA2_P, input DB0_ADC_DA2_N, input DB0_ADC_DB2_P, input DB0_ADC_DB2_N,
   input DB0_ADC_DA3_P, input DB0_ADC_DA3_N, input DB0_ADC_DB3_P, input DB0_ADC_DB3_N,
   input DB0_ADC_DA4_P, input DB0_ADC_DA4_N, input DB0_ADC_DB4_P, input DB0_ADC_DB4_N,
   input DB0_ADC_DA5_P, input DB0_ADC_DA5_N, input DB0_ADC_DB5_P, input DB0_ADC_DB5_N,
   input DB0_ADC_DA6_P, input DB0_ADC_DA6_N, input DB0_ADC_DB6_P, input DB0_ADC_DB6_N,

   input DB1_ADC_DCLK_P, input DB1_ADC_DCLK_N,
   input DB1_ADC_DA0_P, input DB1_ADC_DA0_N, input DB1_ADC_DB0_P, input DB1_ADC_DB0_N,
   input DB1_ADC_DA1_P, input DB1_ADC_DA1_N, input DB1_ADC_DB1_P, input DB1_ADC_DB1_N,
   input DB1_ADC_DA2_P, input DB1_ADC_DA2_N, input DB1_ADC_DB2_P, input DB1_ADC_DB2_N,
   input DB1_ADC_DA3_P, input DB1_ADC_DA3_N, input DB1_ADC_DB3_P, input DB1_ADC_DB3_N,
   input DB1_ADC_DA4_P, input DB1_ADC_DA4_N, input DB1_ADC_DB4_P, input DB1_ADC_DB4_N,
   input DB1_ADC_DA5_P, input DB1_ADC_DA5_N, input DB1_ADC_DB5_P, input DB1_ADC_DB5_N,
   input DB1_ADC_DA6_P, input DB1_ADC_DA6_N, input DB1_ADC_DB6_P, input DB1_ADC_DB6_N,

   output DB0_DAC_DCI_P, output DB0_DAC_DCI_N,
   output DB0_DAC_FRAME_P, output DB0_DAC_FRAME_N,
   output DB0_DAC_D0_P, output DB0_DAC_D0_N, output DB0_DAC_D1_P, output DB0_DAC_D1_N,
   output DB0_DAC_D2_P, output DB0_DAC_D2_N, output DB0_DAC_D3_P, output DB0_DAC_D3_N,
   output DB0_DAC_D4_P, output DB0_DAC_D4_N, output DB0_DAC_D5_P, output DB0_DAC_D5_N,
   output DB0_DAC_D6_P, output DB0_DAC_D6_N, output DB0_DAC_D7_P, output DB0_DAC_D7_N,
   output DB0_DAC_ENABLE,

   output DB1_DAC_DCI_P, output DB1_DAC_DCI_N,
   output DB1_DAC_FRAME_P, output DB1_DAC_FRAME_N,
   output DB1_DAC_D0_P, output DB1_DAC_D0_N, output DB1_DAC_D1_P, output DB1_DAC_D1_N,
   output DB1_DAC_D2_P, output DB1_DAC_D2_N, output DB1_DAC_D3_P, output DB1_DAC_D3_N,
   output DB1_DAC_D4_P, output DB1_DAC_D4_N, output DB1_DAC_D5_P, output DB1_DAC_D5_N,
   output DB1_DAC_D6_P, output DB1_DAC_D6_N, output DB1_DAC_D7_P, output DB1_DAC_D7_N,
   output DB1_DAC_ENABLE,

   output DB0_SCLK, output DB0_MOSI,
   output DB0_ADC_SEN, output DB0_DAC_SEN, output DB0_TX_SEN, output DB0_RX_SEN,
   output DB0_RX_LSADC_SEN, output DB0_RX_LSDAC_SEN, output DB0_TX_LSADC_SEN, output DB0_TX_LSDAC_SEN,
   input DB0_RX_LSADC_MISO, input DB0_RX_MISO, input DB0_TX_LSADC_MISO, input DB0_TX_MISO,

   output DB1_SCLK, output DB1_MOSI,
   output DB1_ADC_SEN, output DB1_DAC_SEN, output DB1_TX_SEN, output DB1_RX_SEN,
   output DB1_RX_LSADC_SEN, output DB1_RX_LSDAC_SEN, output DB1_TX_LSADC_SEN, output DB1_TX_LSDAC_SEN,
   input DB1_RX_LSADC_MISO, input DB1_RX_MISO, input DB1_TX_LSADC_MISO, input DB1_TX_MISO,
   output DB_DAC_SCLK, inout DB_DAC_MOSI,

   output DB_ADC_RESET, output DB_DAC_RESET,

   inout DB_SCL, inout DB_SDA,

   ///////////////////////////////////
   //
   // GPIO/LEDS/Etc
   //
   ///////////////////////////////////

   inout [11:0] FrontPanelGpio,

   output LED_ACT1, output LED_ACT2,
   output LED_LINK1, output LED_LINK2,

   output LED_PPS, output LED_REFLOCK, output LED_GPSLOCK,
   output LED_LINKSTAT, output LED_LINKACT,
   output LED_RX1_RX, output LED_RX2_RX,
   output LED_TXRX1_RX, output LED_TXRX1_TX,
   output LED_TXRX2_RX, output LED_TXRX2_TX,
   inout [15:0] DB0_TX_IO,
   inout [15:0] DB0_RX_IO,
   inout [15:0] DB1_TX_IO,
   inout [15:0] DB1_RX_IO,

   ///////////////////////////////////
   //
   // LMK CLock chip
   //
   ///////////////////////////////////

   input [1:0] LMK_Status,
   input LMK_Holdover,
   input LMK_Lock,
   input LMK_Sync, //not used, we do soft sync
   output LMK_SEN, output LMK_MOSI, output LMK_SCLK,

   ///////////////////////////////////
   //
   // GPSDO and Clock Refs
   //
   ///////////////////////////////////

   output [1:0] ClockRefSelect,
   output GPS_SER_IN, input GPS_SER_OUT,
   input GPS_PPS_OUT, input EXT_PPS_IN,
   output EXT_PPS_OUT, input GPS_LOCK_OK,
   output GPSDO_PWR_ENA, output TCXO_ENA,

   output CPRI_CLK_OUT_P, output CPRI_CLK_OUT_N,

   input FPGA_REFCLK_10MHz_p, input FPGA_REFCLK_10MHz_n,

   ///////////////////////////////////
   //
   // Supporting I/O for SPF+ interfaces
   //  (non high speed stuff)
   //
   ///////////////////////////////////

   inout SFPP0_SCL, inout SFPP0_SDA,
   input SFPP0_ModAbs,
   input SFPP0_RxLOS,   // High if module asserts Loss of Signal
   input SFPP0_TxFault, // Current 10G PMA/PCS apparently ignores this signal.
   output SFPP0_RS0,  // These are actually open drain outputs
   output SFPP0_RS1,  // CAUTION! Take great care, this signal shorted to VeeR on SFP module.
   output SFPP0_TxDisable,  // These are actually open drain outputs

   inout SFPP1_SCL, inout SFPP1_SDA,
   input SFPP1_ModAbs,
   input SFPP1_RxLOS,   // High if module asserts Loss of Signal
   input SFPP1_TxFault, // Current 10G PMA/PCS apparently ignores this signal.
   output SFPP1_RS0,  // These are actually open drain outputs
   output SFPP1_RS1,  // CAUTION! Take great care, this signal shorted to VeeR on SFP module.
   output SFPP1_TxDisable, // These are actually open drain outputs

   ///////////////////////////////////
   //
   // Misc.
   //
   ///////////////////////////////////

   input FPGA_PUDC_B
);

   wire         radio_clk, radio_clk_2x, dac_dci_clk;
   wire         global_rst, radio_rst, bus_rst, bus_rst_div2, ce_rst, adc_idlyctrl_rst;
   wire [3:0]   sw_rst;
   wire [2:0]   led0, led1;

   ////////////////////////////////////////////////////////////////////
   //
   // Generate Bus Clock and PCIe Clocks.
   // Source clock comes from U19 which is fixed freq
   // and bufferd to be used by STC3 also (Page17 schematics).
   //
   ////////////////////////////////////////////////////////////////////

   wire     fpga_clk125, bus_clk, bus_clk_div2, ce_clk, ioport2_clk, rio40_clk, ioport2_idelay_ref_clk;
   wire     bus_clk_locked, rio40_clk_locked, rio40_clk_reset;

   IBUFG fpga_125MHz_clk_buf (
     .I(FPGA_125MHz_CLK),
     .O(fpga_clk125));

   //----------------------------------------------------------------------------
   //  Output     Output      Phase    Duty Cycle   Pk-to-Pk     Phase
   //   Clock     Freq (MHz)  (degrees)    (%)     Jitter (ps)  Error (ps)
   //----------------------------------------------------------------------------
   // CLK_OUT1___187.500______0.000______50.0_______85.263_____73.940
   // CLK_OUT2___125.000______0.000______50.0_______91.831_____73.940
   // CLK_OUT3____93.750______0.000______50.0_______96.813_____73.940
   // CLK_OUT4___214.286______0.000______50.0_______83.210_____73.940
   //
   //----------------------------------------------------------------------------
   // Input Clock   Freq (MHz)    Input Jitter (UI)
   //----------------------------------------------------------------------------
   // __primary_________125.000____________0.010

   localparam BUS_CLK_RATE = 32'd187500000;

   wire ioport2_clk_unbuf;

   bus_clk_gen bus_clk_gen (
      .CLK_IN1(fpga_clk125),
      .CLKFB_IN(ioport2_clk),
      .CLK_OUT1(bus_clk),
      .CLK_OUT2_UNBUF(/* unused */),    //This exists to make the IP generate a 125MHz FB clock
      .CLK_OUT3(bus_clk_div2), //bus_clk divided by 2. used by sc/zpu
      .CLK_OUT4(ce_clk),
      .CLKFB_OUT(ioport2_clk_unbuf),
      .LOCKED(bus_clk_locked));

   BUFG ioport2_clk_bufg_i (
      .O(ioport2_clk),
      .I(ioport2_clk_unbuf));

   //----------------------------------------------------------------------------
   //  Output     Output      Phase    Duty Cycle   Pk-to-Pk     Phase
   //   Clock     Freq (MHz)  (degrees)    (%)     Jitter (ps)  Error (ps)
   //----------------------------------------------------------------------------
   // CLK_OUT1____40.000______0.000______50.0______353.417_____96.948
   // CLK_OUT2___200.000______0.000______50.0______192.299_____96.948
   //
   //----------------------------------------------------------------------------
   // Input Clock   Freq (MHz)    Input Jitter (UI)
   //----------------------------------------------------------------------------
   // __primary_________125.000____________0.100

   //rio40_clk and ioport2_idelay_ref_clk cannot share a PLL/MMCM reset with ioport2_clk
   //so they have to come from a different clocking primitive instance
   pcie_clk_gen pcie_clk_gen (
      .CLK_IN1(fpga_clk125),
      .CLK_OUT1(rio40_clk),
      .CLK_OUT2(ioport2_idelay_ref_clk),
      .RESET(rio40_clk_reset),
      .LOCKED(rio40_clk_locked));

   /////////////////////////////////////////////////////////////////////
   //
   // 10MHz Reference clock
   //
   //////////////////////////////////////////////////////////////////////
   wire ref_clk;
   IBUFDS IBUFDS_ref_clk (
        .O(ref_clk),
        .I(FPGA_REFCLK_10MHz_p),
        .IB(FPGA_REFCLK_10MHz_n)
    );

   //////////////////////////////////////////////////////////////////////
   // CPRI Clock output -- this is the dirty recovered clock from the MGT
   // This goes to the LMK04816 which locks to it and cleans it up
   // We get the clean versions back as CPRI_CLK (for the CPRI MGT)
   // and FPGA_CLK (for our main rfclk)
   //////////////////////////////////////////////////////////////////////

   wire cpri_clk_out = 1'b0; // FIXME - connect to CPRI clock recovery when implemented
   OBUFDS OBUFDS_cpri (.I(cpri_clk_out), .O(CPRI_CLK_OUT_P), .OB(CPRI_CLK_OUT_N));

   /////////////////////////////////////////////////////////////////////
   //
   // power-on-reset logic.
   //
   //////////////////////////////////////////////////////////////////////
   por_gen por_gen(.clk(bus_clk), .reset_out(global_rst));

   //////////////////////////////////////////////////////////////////////
   wire [31:0] rx0, rx1;
   wire [31:0] tx0, tx1;
   wire        sclk0, mosi0, miso0, sclk1, mosi1, miso1;
   wire [7:0]  sen0, sen1;

   wire        set_stb;
   wire [7:0]  set_addr;
   wire [31:0] set_data;

   ////////////////////////////////////////////////////////////////////
   //
   // Generate Radio Clocks from LMK04816
   // Radio clock is normally 200MHz, radio_clk_2x 400MHz.
   // In CPRI or LTE mode, radio clock is 184.32 MHz.
   // radio_clk_2x is only to be used for clocking out TX samples to DAC
   //
   //----------------------------------------------------------------------------
   //  Output     Output      Phase    Duty Cycle   Pk-to-Pk     Phase
   //   Clock     Freq (MHz)  (degrees)    (%)     Jitter (ps)  Error (ps)
   //----------------------------------------------------------------------------
   // CLK_OUT1___200.000______0.000______50.0_______92.799_____82.655
   // CLK_OUT2___400.000____-45.000______50.0_______81.254_____82.655
   // CLK_OUT3___400.000_____60.000______50.0_______81.254_____82.655
   //
   //----------------------------------------------------------------------------
   // Input Clock   Freq (MHz)    Input Jitter (UI)
   //----------------------------------------------------------------------------
   // __primary_________200.000____________0.010
   //
   ////////////////////////////////////////////////////////////////////

   wire radio_clk_locked;
   radio_clk_gen radio_clk_gen (
      .clk_in1_p(FPGA_CLK_p), .clk_in1_n(FPGA_CLK_n),
      .CLK_OUT1(radio_clk), .CLK_OUT2(radio_clk_2x), .CLK_OUT3(dac_dci_clk),
      .RESET(sw_rst[2]), .LOCKED(radio_clk_locked));

   ////////////////////////////////////////////////////////////////////
   //
   // IJB. Radio PLL doesn't seem to lock at power up.
   // Probably needs AD9610 to be programmed to 120 or 200MHz to get
   // an input clock thats in the ball park for PLL configuration.
   // Currently use busclk PLL lock signal to control this reset,
   // but we should find a better solution, perhaps a S/W controllable
   // reset like the ETH PHY uses so that we can reset this clock domain
   // after programming the AD9610.
   //
   ////////////////////////////////////////////////////////////////////

   reset_sync radio_reset_sync (
      .clk(radio_clk),
      .reset_in(global_rst || !bus_clk_locked || sw_rst[1]),
      .reset_out(radio_rst)
   );

   reset_sync int_reset_sync (
      .clk(bus_clk),
      .reset_in(global_rst || !bus_clk_locked),
      .reset_out(bus_rst)
   );

   reset_sync int_div2_reset_sync (
      .clk(bus_clk_div2),
      .reset_in(global_rst || !bus_clk_locked),
      .reset_out(bus_rst_div2)
   );

   reset_sync adc_idlyctrl_reset_sync (
      .clk(bus_clk),
      .reset_in(global_rst || !bus_clk_locked || sw_rst[3]),
      .reset_out(adc_idlyctrl_rst)
   );

   reset_sync ce_reset_sync (
      .clk(ce_clk),
      .reset_in(global_rst || !bus_clk_locked),
      .reset_out(ce_rst)
   );

   ////////////////////////////////////////////////////////////////////
   // PPS
   // Support for internal, external, and GPSDO PPS inputs
   // Every attempt to minimize propagation between the external PPS
   // input and outputs to support daisy-chaining the signal.
   ////////////////////////////////////////////////////////////////////

   wire int_pps;
   wire [31:0] ref_freq;
   wire ref_freq_changed;
   wire ref_freq_sync_empty;
   wire [71:0] ref_freq_sync_out;
   reg new_ref_freq = 0;
   reg [31:0] ref_freq_refclk = 10_000_000;   // Default to 10 MHz reference

   // Synchronize ref_freq to ref_clk
   fifo_short_2clk ref_freq_sync
   (
     .rst(bus_rst),
     .wr_clk(bus_clk),
     .rd_clk(ref_clk),
     .din({40'd0,ref_freq}),
     .wr_en(ref_freq_changed),
     .rd_en(new_ref_freq),
     .dout(ref_freq_sync_out),
     .full( /* unused */ ),
     .empty(ref_freq_sync_empty),
     .rd_data_count( /* unused */ ),
     .wr_data_count( /* unused */ )
   );

   // Capture the new reference frequency
   always @(posedge ref_clk) begin
     if (~ref_freq_sync_empty) begin
       ref_freq_refclk <= ref_freq_sync_out[31:0];
       new_ref_freq <= 1'b1;
     end else
       new_ref_freq <= 1'b0;
   end

   // Generate an internal PPS signal with a 25% duty cycle
   pulse_generator #(.WIDTH(32)) pps_gen
   (
      .clk(ref_clk),
      .reset(new_ref_freq),
      .period(ref_freq_refclk),
      //shift frequency by 2 bits (divide by 4) for a 25% duty cycle
      .pulse_width({2'b00,ref_freq_refclk[31:2]}),
      .pulse(int_pps)
   );

   // PPS MUX - selects internal, external, or gpsdo PPS
   reg pps;
   wire [1:0] pps_select;
   wire pps_out_enb;
   always @(*) begin
      case(pps_select)
         2'b00  :   pps = EXT_PPS_IN;
         2'b01  :   pps = 1'b0;
         2'b10  :   pps = int_pps;
         2'b11  :   pps = GPS_PPS_OUT;
         default:   pps = 1'b0;
      endcase
   end

   // PPS out and LED
   assign EXT_PPS_OUT = pps & pps_out_enb;
   assign LED_PPS = ~pps;                                  // active low LED driver

   assign LED_GPSLOCK = ~GPS_LOCK_OK;
   assign LED_REFLOCK = ~LMK_Lock;
   assign {LED_RX1_RX,LED_TXRX1_TX,LED_TXRX1_RX} = ~led0;  // active low LED driver
   assign {LED_RX2_RX,LED_TXRX2_TX,LED_TXRX2_RX} = ~led1;  // active low LED driver
   // Allocate SPI chip selects to various slaves.
   assign {DB1_DAC_SEN, DB1_ADC_SEN, DB1_RX_LSADC_SEN, DB1_RX_LSDAC_SEN, DB1_TX_LSADC_SEN, DB1_TX_LSDAC_SEN, DB1_RX_SEN, DB1_TX_SEN} = sen1;
   assign {DB0_DAC_SEN, DB0_ADC_SEN, DB0_RX_LSADC_SEN, DB0_RX_LSDAC_SEN, DB0_TX_LSADC_SEN, DB0_TX_LSDAC_SEN, DB0_RX_SEN, DB0_TX_SEN} = sen0;

   wire        db_dac_mosi_int, db_dac_miso;
   wire        drive_dac_pin;
   reg         drop_dac_pin;
   reg [5:0]   bitcount;
   reg         sclk_d1;

   // Register copy of outgoing DAC clock to do synchronous edge detect.
   always @(posedge radio_clk) sclk_d1 <= DB_DAC_SCLK;

   always @(posedge radio_clk)
     // If neither DAC is selected keep counter reset
     if(DB0_DAC_SEN & DB1_DAC_SEN)
       begin
      bitcount <= 6'd0;
      drop_dac_pin <= 1'b0;
       end
     else if(~DB_DAC_SCLK & sclk_d1)
       // Falling edge of SCLK detected.
       begin
      bitcount <= bitcount + 6'd1;
       end
     else if(bitcount == 0 & DB_DAC_SCLK & ~sclk_d1)
       // On first rising edge store R/W bit to determine if we tristate after 8bits for a Read.
       drop_dac_pin <= db_dac_mosi_int;

   assign drive_dac_pin = (bitcount < 8) | ~drop_dac_pin;

   // Both DAC's use a single SPI bus on PCB. Select appriate Radio to drive the SPi bus by looking at chip selects.
   assign { DB_DAC_SCLK, db_dac_mosi_int } = ~DB0_DAC_SEN ? {sclk0, mosi0} : ~DB1_DAC_SEN ? {sclk1,mosi1} : 2'b0;
   // Data to/from DAC's is bi-dir so tristate driver when reading.
   assign DB_DAC_MOSI = drive_dac_pin ? db_dac_mosi_int : 1'bz;
   // I/O Input buffer
   assign db_dac_miso = DB_DAC_MOSI;

   // If any SPI Slave is selected (except DAC)  then drive SPI clk and MOSI out onto duaghterboard.
   assign { DB0_SCLK, DB0_MOSI } = (~&sen0[6:0]) ? {sclk0,mosi0} : 2'b0;
   assign { DB1_SCLK, DB1_MOSI } = (~&sen1[6:0]) ? {sclk1,mosi1} : 2'b0;

   // Wired OR Mux together the possible sources of read data from SPI devices.
   assign miso0 = (~DB0_RX_LSADC_SEN & DB0_RX_LSADC_MISO) |
          (~DB0_RX_SEN & DB0_RX_MISO) |
          (~DB0_TX_LSADC_SEN & DB0_TX_LSADC_MISO) |
          (~DB0_TX_SEN & DB0_TX_MISO) |
          (~DB0_DAC_SEN & db_dac_miso);

   assign miso1 = (~DB1_RX_LSADC_SEN & DB1_RX_LSADC_MISO) |
          (~DB1_RX_SEN & DB1_RX_MISO) |
          (~DB1_TX_LSADC_SEN & DB1_TX_LSADC_MISO) |
          (~DB1_TX_SEN & DB1_TX_MISO) |
          (~DB1_DAC_SEN & db_dac_miso);


   wire [31:0] radio0_misc_out, radio1_misc_out;
   wire [31:0] radio0_misc_in, radio1_misc_in;

   /////////////////////////////////////////////////////////////////////
   //
   // ADC Interface for ADS62P48
   //
   /////////////////////////////////////////////////////////////////////
   wire [13:0] rx0_q_inv, rx1_q_inv, rx0_i, rx1_i;
   // Analog diff pairs on I side of ADC are inverted for layout reasons, but data diff pairs are all swapped as well
   //  so I gets a double negative, and is unchanged.  Q must be inverted.

   capture_ddrlvds #(
      .WIDTH(14),
      .PATT_CHECKER("TRUE"),
      .DATA_IDELAY_MODE("DYNAMIC"), .DATA_IDELAY_VAL(16), .DATA_IDELAY_FREF(200.0)
   ) cap_db0 (
      .adc_clk_p(DB0_ADC_DCLK_P), .adc_clk_n(DB0_ADC_DCLK_N),
      .adc_data_p(
        {{DB0_ADC_DA6_P, DB0_ADC_DA5_P, DB0_ADC_DA4_P, DB0_ADC_DA3_P, DB0_ADC_DA2_P, DB0_ADC_DA1_P, DB0_ADC_DA0_P},
         {DB0_ADC_DB6_P, DB0_ADC_DB5_P, DB0_ADC_DB4_P, DB0_ADC_DB3_P, DB0_ADC_DB2_P, DB0_ADC_DB1_P, DB0_ADC_DB0_P}}),
      .adc_data_n(
        {{DB0_ADC_DA6_N, DB0_ADC_DA5_N, DB0_ADC_DA4_N, DB0_ADC_DA3_N, DB0_ADC_DA2_N, DB0_ADC_DA1_N, DB0_ADC_DA0_N},
         {DB0_ADC_DB6_N, DB0_ADC_DB5_N, DB0_ADC_DB4_N, DB0_ADC_DB3_N, DB0_ADC_DB2_N, DB0_ADC_DB1_N, DB0_ADC_DB0_N}}),
      .radio_clk(radio_clk),
      .data_delay_stb(radio0_misc_out[3]), .data_delay_val(radio0_misc_out[8:4]),
      .adc_cap_clk(),
      .data_out({rx0_i,rx0_q_inv}),
      .checker_en(radio0_misc_out[9]), .checker_locked(radio0_misc_in[3:0]), .checker_failed(radio0_misc_in[7:4])
   );
   assign rx0[31:0] = { rx0_i, 2'b00, ~rx0_q_inv, 2'b00 };

   capture_ddrlvds #(
      .WIDTH(14),
      .PATT_CHECKER("TRUE"),
      .DATA_IDELAY_MODE("DYNAMIC"), .DATA_IDELAY_VAL(16), .DATA_IDELAY_FREF(200.0)
   ) cap_db1 (
      .adc_clk_p(DB1_ADC_DCLK_P), .adc_clk_n(DB1_ADC_DCLK_N),
      .adc_data_p(
        {{DB1_ADC_DA6_P, DB1_ADC_DA5_P, DB1_ADC_DA4_P, DB1_ADC_DA3_P, DB1_ADC_DA2_P, DB1_ADC_DA1_P, DB1_ADC_DA0_P},
         {DB1_ADC_DB6_P, DB1_ADC_DB5_P, DB1_ADC_DB4_P, DB1_ADC_DB3_P, DB1_ADC_DB2_P, DB1_ADC_DB1_P, DB1_ADC_DB0_P}}),
      .adc_data_n(
        {{DB1_ADC_DA6_N, DB1_ADC_DA5_N, DB1_ADC_DA4_N, DB1_ADC_DA3_N, DB1_ADC_DA2_N, DB1_ADC_DA1_N, DB1_ADC_DA0_N},
         {DB1_ADC_DB6_N, DB1_ADC_DB5_N, DB1_ADC_DB4_N, DB1_ADC_DB3_N, DB1_ADC_DB2_N, DB1_ADC_DB1_N, DB1_ADC_DB0_N}}),
      .radio_clk(radio_clk),
      .data_delay_stb(radio1_misc_out[3]), .data_delay_val(radio1_misc_out[8:4]),
      .adc_cap_clk(),
      .data_out({rx1_i,rx1_q_inv}),
      .checker_en(radio1_misc_out[9]), .checker_locked(radio1_misc_in[3:0]), .checker_failed(radio1_misc_in[7:4])
   );
   assign rx1[31:0] = { rx1_i, 2'b00, ~rx1_q_inv, 2'b00 };

   // IDELAYCTRL to calibrate all IDELAYE2 instances in capture_ddrlvds for both sides
   wire adc_idlyctrl_rdy;
   IDELAYCTRL adc_cap_idelayctrl_i (.RDY(adc_idlyctrl_rdy), .REFCLK(radio_clk), .RST(adc_idlyctrl_rst));

   /////////////////////////////////////////////////////////////////////
   //
   // DAC Interface for AD9146
   //
   /////////////////////////////////////////////////////////////////////
   gen_ddrlvds gen_db0
     (
      .reset(radio_rst),
      .tx_clk_2x_p(DB0_DAC_DCI_P), .tx_clk_2x_n(DB0_DAC_DCI_N),
      .tx_frame_p(DB0_DAC_FRAME_P), .tx_frame_n(DB0_DAC_FRAME_N),
      .tx_d_p({DB0_DAC_D7_P,DB0_DAC_D6_P,DB0_DAC_D5_P,DB0_DAC_D4_P,DB0_DAC_D3_P,DB0_DAC_D2_P,DB0_DAC_D1_P,DB0_DAC_D0_P}),
      .tx_d_n({DB0_DAC_D7_N,DB0_DAC_D6_N,DB0_DAC_D5_N,DB0_DAC_D4_N,DB0_DAC_D3_N,DB0_DAC_D2_N,DB0_DAC_D1_N,DB0_DAC_D0_N}),
      .tx_clk_2x(radio_clk_2x), .tx_clk_1x(radio_clk), .tx_dci_clk(dac_dci_clk),
      .i(~tx0[31:16]), .q(~tx0[15:0]), // invert b/c Analog diff pairs are swapped for layout
      .sync_dacs(radio0_misc_out[10]|radio1_misc_out[10])
      );


   gen_ddrlvds gen_db1
     (
      .reset(radio_rst),
      .tx_clk_2x_p(DB1_DAC_DCI_P), .tx_clk_2x_n(DB1_DAC_DCI_N),
      .tx_frame_p(DB1_DAC_FRAME_P), .tx_frame_n(DB1_DAC_FRAME_N),
      .tx_d_p({DB1_DAC_D7_P,DB1_DAC_D6_P,DB1_DAC_D5_P,DB1_DAC_D4_P,DB1_DAC_D3_P,DB1_DAC_D2_P,DB1_DAC_D1_P,DB1_DAC_D0_P}),
      .tx_d_n({DB1_DAC_D7_N,DB1_DAC_D6_N,DB1_DAC_D5_N,DB1_DAC_D4_N,DB1_DAC_D3_N,DB1_DAC_D2_N,DB1_DAC_D1_N,DB1_DAC_D0_N}),
      .tx_clk_2x(radio_clk_2x), .tx_clk_1x(radio_clk), .tx_dci_clk(dac_dci_clk),
      .i(~tx1[31:16]), .q(~tx1[15:0]), // invert b/c Analog diff pairs are swapped for layout
      .sync_dacs(radio0_misc_out[10]|radio1_misc_out[10])
      );


   wire [1:0] leds;
   assign {LED_LINKSTAT, LED_LINKACT} = ~leds;

   wire [31:0] debug;

   //////////////////////////////////////////////////////////////////////
   //
   // PCIe Stuff
   //
   //////////////////////////////////////////////////////////////////////

   localparam IOP2_MSG_WIDTH       = 64;
   localparam DMA_STREAM_WIDTH     = `LVFPGA_IFACE_DMA_CHAN_WIDTH;
   localparam DMA_COUNT_WIDTH      = `LVFPGA_IFACE_DMA_SIZE_WIDTH;
   localparam NUM_TX_STREAMS       = `LVFPGA_IFACE_NUM_TX_DMA_CNT;
   localparam NUM_RX_STREAMS       = `LVFPGA_IFACE_NUM_RX_DMA_CNT;
   localparam TX_STREAM_START_IDX  = `LVFPGA_IFACE_TX_DMA_INDEX;
   localparam RX_STREAM_START_IDX  = `LVFPGA_IFACE_RX_DMA_INDEX;
   localparam DMA_DEST_WIDTH       = 3;

   wire [DMA_STREAM_WIDTH-1:0] dmatx_tdata,  dmarx_tdata,  pcii_tdata,  pcio_tdata;
   wire [DMA_DEST_WIDTH-1:0]   dmatx_tuser, dmarx_tuser, pcii_tuser, pcio_tuser;
   wire                        dmatx_tvalid, dmarx_tvalid, pcii_tvalid, pcio_tvalid;
   wire                        dmatx_tlast,  dmarx_tlast,  pcii_tlast,  pcio_tlast;
   wire                        dmatx_tready, dmarx_tready, pcii_tready, pcio_tready;

   wire [IOP2_MSG_WIDTH-1:0]   o_iop2_msg_tdata, i_iop2_msg_tdata;
   wire                        o_iop2_msg_tvalid, o_iop2_msg_tlast, o_iop2_msg_tready;
   wire                        i_iop2_msg_tvalid, i_iop2_msg_tlast, i_iop2_msg_tready;

   wire            pcie_usr_reg_wr, pcie_usr_reg_rd, pcie_usr_reg_rc, pcie_usr_reg_rdy;
   wire [1:0]      pcie_usr_reg_len;
   wire [19:0]     pcie_usr_reg_addr;
   wire [31:0]     pcie_usr_reg_data_in, pcie_usr_reg_data_out;

   wire            chinch_reg_wr, chinch_reg_rd, chinch_reg_rc, chinch_reg_rdy;
   wire [1:0]      chinch_reg_len;
   wire [19:0]     chinch_reg_addr;
   wire [31:0]     chinch_reg_data_out;
   wire [63:0]     chinch_reg_data_in;

   wire [(NUM_TX_STREAMS*DMA_STREAM_WIDTH)-1:0]    dmatx_tdata_iop2;
   wire [NUM_TX_STREAMS-1:0]                       dmatx_tvalid_iop2, dmatx_tready_iop2;

   wire [(NUM_RX_STREAMS*DMA_STREAM_WIDTH)-1:0]    dmarx_tdata_iop2;
   wire [NUM_RX_STREAMS-1:0]                       dmarx_tvalid_iop2, dmarx_tready_iop2;

   //PCIe Express "Physical" DMA and Register logic
   LvFpga_Chinch_Interface lvfpga_chinch_inst
   (
      .aIoResetIn_n(aIoResetIn_n),
      .bBusReset(),   //Output

      // Clocks
      .BusClk(ioport2_clk),
      .Rio40Clk(rio40_clk),
      .IDelayRefClk(ioport2_idelay_ref_clk),
      .aRioClkPllLocked(rio40_clk_locked),
      .aRioClkPllReset(rio40_clk_reset),

      // The IO_Port2 asynchronous handshaking pins
      .aIoReadyOut(aIoReadyOut),
      .aIoReadyIn(aIoReadyIn),
      .aIoPort2Restart(aIoPort2Restart),

      // The IO_Port2 high speed receiver pins
      .IoRxClock(IoRxClock),
      .IoRxClock_n(IoRxClock_n),
      .irIoRxData(irIoRxData),
      .irIoRxData_n(irIoRxData_n),
      .irIoRxHeader(irIoRxHeader),
      .irIoRxHeader_n(irIoRxHeader_n),

      // The IO_Port2 high speed transmitter interface pins
      .IoTxClock(IoTxClock),
      .IoTxClock_n(IoTxClock_n),
      .itIoTxData(itIoTxData),
      .itIoTxData_n(itIoTxData_n),
      .itIoTxHeader(itIoTxHeader),
      .itIoTxHeader_n(itIoTxHeader_n),

      // DMA TX Fifos
      .bDmaTxData(dmatx_tdata_iop2),
      .bDmaTxValid(dmatx_tvalid_iop2),
      .bDmaTxReady(dmatx_tready_iop2),
      .bDmaTxEnabled(),
      .bDmaTxFifoFullCnt(),

      // DMA RX Fifos
      .bDmaRxData(dmarx_tdata_iop2),
      .bDmaRxValid(dmarx_tvalid_iop2),
      .bDmaRxReady(dmarx_tready_iop2),
      .bDmaRxEnabled(),
      .bDmaRxFifoFreeCnt(),

      // User Register Port In
      .bUserRegPortInWt(pcie_usr_reg_wr),
      .bUserRegPortInRd(pcie_usr_reg_rd),
      .bUserRegPortInAddr(pcie_usr_reg_addr),
      .bUserRegPortInData(pcie_usr_reg_data_in),
      .bUserRegPortInSize(pcie_usr_reg_len),

      // User Register Port Out
      .bUserRegPortOutData(pcie_usr_reg_data_out),
      .bUserRegPortOutDataValid(pcie_usr_reg_rc),
      .bUserRegPortOutReady(pcie_usr_reg_rdy),

      // Chinch Register Port Out
      .bChinchRegPortOutWt(chinch_reg_wr),
      .bChinchRegPortOutRd(chinch_reg_rd),
      .bChinchRegPortOutAddr({12'h0, chinch_reg_addr}),
      .bChinchRegPortOutData({32'h0, chinch_reg_data_out}),
      .bChinchRegPortOutSize(chinch_reg_len),

      // User Register Port In
      .bChinchRegPortInData(chinch_reg_data_in),
      .bChinchRegPortInDataValid(chinch_reg_rc),
      .bChinchRegPortInReady(chinch_reg_rdy),

      // Level interrupt
      .aIrq(aIrq)
   );

   //PCIe Express adapter logic to link to the AXI crossbar and the WB bus
   x300_pcie_int #(
      .DMA_STREAM_WIDTH(DMA_STREAM_WIDTH),
      .NUM_TX_STREAMS(NUM_TX_STREAMS),
      .NUM_RX_STREAMS(NUM_RX_STREAMS),
      .REGPORT_ADDR_WIDTH(20),
      .REGPORT_DATA_WIDTH(32),
      .IOP2_MSG_WIDTH(IOP2_MSG_WIDTH),
      .BUS_CLK_RATE(BUS_CLK_RATE)
   ) x300_pcie_int (
      .ioport2_clk(ioport2_clk),
      .bus_clk(bus_clk),
      .bus_rst(bus_rst),

      //DMA TX FIFOs (IoPort2 Clock Domain)
      .dmatx_tdata_iop2(dmatx_tdata_iop2),
      .dmatx_tvalid_iop2(dmatx_tvalid_iop2),
      .dmatx_tready_iop2(dmatx_tready_iop2),

      //DMA TX FIFOs (IoPort2 Clock Domain)
      .dmarx_tdata_iop2(dmarx_tdata_iop2),
      .dmarx_tvalid_iop2(dmarx_tvalid_iop2),
      .dmarx_tready_iop2(dmarx_tready_iop2),

      //PCIe User Regport
      .pcie_usr_reg_wr(pcie_usr_reg_wr),
      .pcie_usr_reg_rd(pcie_usr_reg_rd),
      .pcie_usr_reg_addr(pcie_usr_reg_addr),
      .pcie_usr_reg_data_in(pcie_usr_reg_data_in),
      .pcie_usr_reg_len(pcie_usr_reg_len),
      .pcie_usr_reg_data_out(pcie_usr_reg_data_out),
      .pcie_usr_reg_rc(pcie_usr_reg_rc),
      .pcie_usr_reg_rdy(pcie_usr_reg_rdy),

      //Chinch Regport
      .chinch_reg_wr(chinch_reg_wr),
      .chinch_reg_rd(chinch_reg_rd),
      .chinch_reg_addr(chinch_reg_addr),
      .chinch_reg_data_out(chinch_reg_data_out),
      .chinch_reg_len(chinch_reg_len),
      .chinch_reg_data_in(chinch_reg_data_in[31:0]),
      .chinch_reg_rc(chinch_reg_rc),
      .chinch_reg_rdy(chinch_reg_rdy),

      //DMA TX FIFO (Bus Clock Domain). Note: tuser is used for muxing.
      .dmatx_tdata(dmatx_tdata),
      .dmatx_tuser(dmatx_tuser),
      .dmatx_tlast(dmatx_tlast),
      .dmatx_tvalid(dmatx_tvalid),
      .dmatx_tready(dmatx_tready),

      //DMA RX FIFO (Bus Clock Domain). Note: tuser is used for muxing.
      .dmarx_tdata(dmarx_tdata),
      .dmarx_tuser(dmarx_tuser),
      .dmarx_tlast(dmarx_tlast),
      .dmarx_tvalid(dmarx_tvalid),
      .dmarx_tready(dmarx_tready),

      //Message FIFO Out (Bus Clock Domain)
      .rego_tdata(o_iop2_msg_tdata),
      .rego_tvalid(o_iop2_msg_tvalid),
      .rego_tlast(o_iop2_msg_tlast),
      .rego_tready(o_iop2_msg_tready),

      //Message FIFO In (Bus Clock Domain)
      .regi_tdata(i_iop2_msg_tdata),
      .regi_tvalid(i_iop2_msg_tvalid),
      .regi_tlast(i_iop2_msg_tlast),
      .regi_tready(i_iop2_msg_tready),

      //Misc
      .misc_status({15'h0, aStc3Gpio7}),
      .debug()
   );

   // The PCIe logic will tend to stay close to the physical IoPort2 pins
   // so add an additional stage of pipelining to give the tool more routing
   // slack. This is significantly help timing closure.

   axi_fifo_short #(.WIDTH(DMA_STREAM_WIDTH+1+DMA_DEST_WIDTH)) pcii_pipeline_srl (
      .clk(bus_clk), .reset(bus_rst), .clear(1'b0),
      .i_tdata({dmatx_tuser, dmatx_tlast, dmatx_tdata}), .i_tvalid(dmatx_tvalid), .i_tready(dmatx_tready),
      .o_tdata({pcii_tuser, pcii_tlast, pcii_tdata}), .o_tvalid(pcii_tvalid), .o_tready(pcii_tready),
      .space(), .occupied());

   axi_fifo_short #(.WIDTH(DMA_STREAM_WIDTH+1+DMA_DEST_WIDTH)) pcio_pipeline_srl (
      .clk(bus_clk), .reset(bus_rst), .clear(1'b0),
      .i_tdata({pcio_tuser, pcio_tlast, pcio_tdata}), .i_tvalid(pcio_tvalid), .i_tready(pcio_tready),
      .o_tdata({dmarx_tuser, dmarx_tlast, dmarx_tdata}), .o_tvalid(dmarx_tvalid), .o_tready(dmarx_tready),
      .space(), .occupied());

   //////////////////////////////////////////////////////////////////////
   //
   // Configure SFP+ clocking
   //
   //////////////////////////////////////////////////////////////////////
`ifdef BUILD_1G
   wire  gige_refclk, gige_refclk_bufg;

   one_gige_phy_clk_gen gige_clk_gen_i (
      .refclk_p(ETH_CLK_p),
      .refclk_n(ETH_CLK_n),
      .refclk(gige_refclk),
      .refclk_bufg(gige_refclk_bufg)
   );
`endif
`ifdef BUILD_10G
   wire  xgige_refclk;
   wire  xgige_clk156;
   wire  xgige_dclk;

   ten_gige_phy_clk_gen xgige_clk_gen_i (
      .areset(global_rst | sw_rst[0]),
      .refclk_p(XG_CLK_p),
      .refclk_n(XG_CLK_n),
      .refclk(xgige_refclk),
      .clk156(xgige_clk156),
      .dclk(xgige_dclk)
   );
   `ifdef BUILD_AURORA
      wire  aurora_refclk = xgige_refclk;
      wire  aurora_refclk_bufg = xgige_clk156;
      wire  aurora_init_clk = xgige_dclk;
   `endif
`else
   `ifdef BUILD_AURORA
      wire  aurora_refclk;
      wire  aurora_refclk_bufg;
      wire  aurora_init_clk;

      aurora_phy_clk_gen aurora_clk_gen_i (
         .areset(global_rst | sw_rst[0]),
         .refclk_p(XG_CLK_p),
         .refclk_n(XG_CLK_n),
         .refclk(aurora_refclk),
         .clk156(aurora_refclk_bufg),
         .init_clk(aurora_init_clk)
      );
   `endif
`endif

   wire  sfp0_gt_refclk, sfp1_gt_refclk;
   wire  sfp0_gb_refclk, sfp1_gb_refclk;
   wire  sfp0_misc_clk, sfp1_misc_clk;

`ifdef SFP0_10GBE
   assign sfp0_gt_refclk = xgige_refclk;
   assign sfp0_gb_refclk = xgige_clk156;
   assign sfp0_misc_clk  = xgige_dclk;
`endif
`ifdef SFP0_1GBE
   assign sfp0_gt_refclk = gige_refclk;
   assign sfp0_gb_refclk = gige_refclk_bufg;
   assign sfp0_misc_clk  = gige_refclk_bufg;
`endif
`ifdef SFP0_AURORA
   assign sfp0_gt_refclk = aurora_refclk;
   assign sfp0_gb_refclk = aurora_refclk_bufg;
   assign sfp0_misc_clk  = aurora_init_clk;
`endif

`ifdef SFP1_10GBE
   assign sfp1_gt_refclk = xgige_refclk;
   assign sfp1_gb_refclk = xgige_clk156;
   assign sfp1_misc_clk  = xgige_dclk;
`endif
`ifdef SFP1_1GBE
   assign sfp1_gt_refclk = gige_refclk;
   assign sfp1_gb_refclk = gige_refclk_bufg;
   assign sfp1_misc_clk  = gige_refclk_bufg;
`endif
`ifdef SFP1_AURORA
   assign sfp1_gt_refclk = aurora_refclk;
   assign sfp1_gb_refclk = aurora_refclk_bufg;
   assign sfp1_misc_clk  = aurora_init_clk;
`endif

   //////////////////////////////////////////////////////////////////////
   //
   // SFP+ PORT0
   //
   //////////////////////////////////////////////////////////////////////

   wire [63:0] sfp0_rx_tdata, sfp0_tx_tdata;
   wire [3:0]  sfp0_rx_tuser, sfp0_tx_tuser;
   wire        sfp0_rx_tlast, sfp0_tx_tlast, sfp0_rx_tvalid, sfp0_tx_tvalid, sfp0_rx_tready, sfp0_tx_tready;
   wire [15:0] sfp0_phy_status;

   wire [31:0] sfp0_wb_dat_i;
   wire [31:0] sfp0_wb_dat_o;
   wire [15:0] sfp0_wb_adr;
   wire        sfp0_wb_ack, sfp0_wb_stb, sfp0_wb_cyc, sfp0_wb_we, sfp0_wb_int;

   wire        sfp0_link_up, sfp0_activity;

   x300_sfpp_io_core #(
`ifdef SFP0_10GBE
      .PROTOCOL("10GbE"),
`endif
`ifdef SFP0_1GBE
      .PROTOCOL("1GbE"),
`endif
`ifdef SFP0_AURORA
      .PROTOCOL("Aurora"),
`endif
      .PORTNUM(8'd0)
   ) sfpp_io_i0 (
      .areset(global_rst | sw_rst[0]),
      .gt_refclk(sfp0_gt_refclk),
      .gb_refclk(sfp0_gb_refclk),
      .misc_clk(sfp0_misc_clk),

      .bus_rst(bus_rst),
      .bus_clk(bus_clk),
      .bus_rst_div2(bus_rst_div2),
      .bus_clk_div2(bus_clk_div2),

      .txp(SFP0_TX_p),
      .txn(SFP0_TX_n),
      .rxp(SFP0_RX_p),
      .rxn(SFP0_RX_n),

      .sfpp_rxlos(SFPP0_RxLOS),
      .sfpp_tx_fault(SFPP0_TxFault),
      .sfpp_tx_disable(SFPP0_TxDisable),

      .s_axis_tdata(sfp0_tx_tdata),
      .s_axis_tuser(sfp0_tx_tuser),
      .s_axis_tlast(sfp0_tx_tlast),
      .s_axis_tvalid(sfp0_tx_tvalid),
      .s_axis_tready(sfp0_tx_tready),

      .m_axis_tdata(sfp0_rx_tdata),
      .m_axis_tuser(sfp0_rx_tuser),
      .m_axis_tlast(sfp0_rx_tlast),
      .m_axis_tvalid(sfp0_rx_tvalid),
      .m_axis_tready(sfp0_rx_tready),

      .wb_adr_i(sfp0_wb_adr),
      .wb_cyc_i(sfp0_wb_cyc),
      .wb_dat_i(sfp0_wb_dat_o),
      .wb_stb_i(sfp0_wb_stb),
      .wb_we_i(sfp0_wb_we),
      .wb_ack_o(sfp0_wb_ack),
      .wb_dat_o(sfp0_wb_dat_i),
      .wb_int_o(sfp0_wb_int),

      .phy_status(sfp0_phy_status),
      .link_up(sfp0_link_up),
      .activity(sfp0_activity)
   );

   // LEDs are driven with negative logic.
   assign LED_LINK2 = ~sfp0_link_up;
   assign LED_ACT2  = ~sfp0_activity;


   //////////////////////////////////////////////////////////////////////
   //
   // SFP+ PORT1
   //
   //////////////////////////////////////////////////////////////////////

   wire [63:0] sfp1_rx_tdata, sfp1_tx_tdata;
   wire [3:0]  sfp1_rx_tuser, sfp1_tx_tuser;
   wire        sfp1_rx_tlast, sfp1_tx_tlast, sfp1_rx_tvalid, sfp1_tx_tvalid, sfp1_rx_tready, sfp1_tx_tready;
   wire [15:0] sfp1_phy_status;

   wire [31:0] sfp1_wb_dat_i;
   wire [31:0] sfp1_wb_dat_o;
   wire [15:0] sfp1_wb_adr;
   wire        sfp1_wb_ack, sfp1_wb_stb, sfp1_wb_cyc, sfp1_wb_we, sfp1_wb_int;

   wire        sfp1_link_up, sfp1_activity;

   x300_sfpp_io_core #(
`ifdef SFP1_10GBE
      .PROTOCOL("10GbE"),
`endif
`ifdef SFP1_1GBE
      .PROTOCOL("1GbE"),
`endif
`ifdef SFP1_AURORA
      .PROTOCOL("Aurora"),
`endif
      .PORTNUM(8'd1)
   ) sfpp_io_i1 (
      .areset(global_rst | sw_rst[0]),
      .gt_refclk(sfp1_gt_refclk),
      .gb_refclk(sfp1_gb_refclk),
      .misc_clk(sfp1_misc_clk),

      .bus_rst(bus_rst),
      .bus_clk(bus_clk),
      .bus_rst_div2(bus_rst_div2),
      .bus_clk_div2(bus_clk_div2),

      .txp(SFP1_TX_p),
      .txn(SFP1_TX_n),
      .rxp(SFP1_RX_p),
      .rxn(SFP1_RX_n),

      .sfpp_rxlos(SFPP1_RxLOS),
      .sfpp_tx_fault(SFPP1_TxFault),
      .sfpp_tx_disable(SFPP1_TxDisable),

      .s_axis_tdata(sfp1_tx_tdata),
      .s_axis_tuser(sfp1_tx_tuser),
      .s_axis_tlast(sfp1_tx_tlast),
      .s_axis_tvalid(sfp1_tx_tvalid),
      .s_axis_tready(sfp1_tx_tready),

      .m_axis_tdata(sfp1_rx_tdata),
      .m_axis_tuser(sfp1_rx_tuser),
      .m_axis_tlast(sfp1_rx_tlast),
      .m_axis_tvalid(sfp1_rx_tvalid),
      .m_axis_tready(sfp1_rx_tready),

      .wb_adr_i(sfp1_wb_adr),
      .wb_cyc_i(sfp1_wb_cyc),
      .wb_dat_i(sfp1_wb_dat_o),
      .wb_stb_i(sfp1_wb_stb),
      .wb_we_i(sfp1_wb_we),
      .wb_ack_o(sfp1_wb_ack),
      .wb_dat_o(sfp1_wb_dat_i),
      .wb_int_o(sfp1_wb_int),

      .phy_status(sfp1_phy_status),
      .link_up(sfp1_link_up),
      .activity(sfp1_activity)
   );

   // LEDs are driven with negative logic.
   assign LED_LINK1 = ~sfp1_link_up;
   assign LED_ACT1  = ~sfp1_activity;

   ///////////////////////////////////////////////////////////////////////////////////
   //
   // Synchronize misc asynchronous signals
   //
   ///////////////////////////////////////////////////////////////////////////////////
   wire LMK_Holdover_sync, LMK_Lock_sync, LMK_Sync_sync;
   wire LMK_Status0_sync, LMK_Status1_sync;
   wire radio_clk_locked_sync;
   wire adc_idlyctrl_rdy_sync;

   //Sync all LMK_* signals to bus_clk
   synchronizer #(.INITIAL_VAL(1'b0)) LMK_Holdover_sync_inst (
      .clk(bus_clk), .rst(1'b0 /* no reset */), .in(LMK_Holdover), .out(LMK_Holdover_sync));
   synchronizer #(.INITIAL_VAL(1'b0)) LMK_Lock_sync_inst (
      .clk(bus_clk), .rst(1'b0 /* no reset */), .in(LMK_Lock), .out(LMK_Lock_sync));
   synchronizer #(.INITIAL_VAL(1'b0)) LMK_Sync_sync_inst (
      .clk(bus_clk), .rst(1'b0 /* no reset */), .in(LMK_Sync), .out(LMK_Sync_sync));
   //The status bits (although in a bus) are really independent
   synchronizer #(.INITIAL_VAL(1'b0)) LMK_Status0_sync_inst (
      .clk(bus_clk), .rst(1'b0 /* no reset */), .in(LMK_Status[0]), .out(LMK_Status0_sync));
   synchronizer #(.INITIAL_VAL(1'b0)) LMK_Status1_sync_inst (
      .clk(bus_clk), .rst(1'b0 /* no reset */), .in(LMK_Status[1]), .out(LMK_Status1_sync));

   synchronizer #(.INITIAL_VAL(1'b0)) radio_clk_locked_sync_inst (
      .clk(bus_clk), .rst(1'b0 /* no reset */), .in(radio_clk_locked), .out(radio_clk_locked_sync));
   synchronizer #(.INITIAL_VAL(1'b0)) adc_idlyctrl_rdy_sync_inst (
      .clk(bus_clk), .rst(1'b0 /* no reset */), .in(adc_idlyctrl_rdy), .out(adc_idlyctrl_rdy_sync));

   ///////////////////////////////////////////////////////////////////////////////////
   //
   // Xilinx DDR3 Controller and PHY.
   //
   ///////////////////////////////////////////////////////////////////////////////////


   wire           ddr3_axi_clk;           // 1/4 DDR external clock rate (150MHz)
   wire           ddr3_axi_clk_x2;        // 1/2 DDR external clock rate (300MHz)
   wire           ddr3_axi_rst;           // Synchronized to ddr_sys_clk
   wire           ddr3_running;           // DRAM calibration complete.
   wire [11:0]    device_temp;

   // Slave Interface Write Address Ports
   wire           s_axi_awid;
   wire [31:0]    s_axi_awaddr;
   wire [7:0]     s_axi_awlen;
   wire [2:0]     s_axi_awsize;
   wire [1:0]     s_axi_awburst;
   wire [0:0]     s_axi_awlock;
   wire [3:0]     s_axi_awcache;
   wire [2:0]     s_axi_awprot;
   wire [3:0]     s_axi_awqos;
   wire           s_axi_awvalid;
   wire           s_axi_awready;
   // Slave Interface Write Data Ports
   wire [255:0]   s_axi_wdata;
   wire [31:0]    s_axi_wstrb;
   wire           s_axi_wlast;
   wire           s_axi_wvalid;
   wire           s_axi_wready;
   // Slave Interface Write Response Ports
   wire           s_axi_bready;
   wire           s_axi_bid;
   wire [1:0]     s_axi_bresp;
   wire           s_axi_bvalid;
   // Slave Interface Read Address Ports
   wire           s_axi_arid;
   wire [31:0]    s_axi_araddr;
   wire [7:0]     s_axi_arlen;
   wire [2:0]     s_axi_arsize;
   wire [1:0]     s_axi_arburst;
   wire [0:0]     s_axi_arlock;
   wire [3:0]     s_axi_arcache;
   wire [2:0]     s_axi_arprot;
   wire [3:0]     s_axi_arqos;
   wire           s_axi_arvalid;
   wire           s_axi_arready;
   // Slave Interface Read Data Ports
   wire           s_axi_rready;
   wire           s_axi_rid;
   wire [255:0]   s_axi_rdata;
   wire [1:0]     s_axi_rresp;
   wire           s_axi_rlast;
   wire           s_axi_rvalid;

   wire           ddr3_idelay_refclk;
   reg            ddr3_axi_rst_reg_n;

   // Copied this reset circuit from example design.
   always @(posedge ddr3_axi_clk)
     ddr3_axi_rst_reg_n <= ~ddr3_axi_rst;

   // Instantiate the DDR3 MIG core
   ddr3_32bit u_ddr3_32bit (
      // Memory interface ports
      .ddr3_addr                      (ddr3_addr),
      .ddr3_ba                        (ddr3_ba),
      .ddr3_cas_n                     (ddr3_cas_n),
      .ddr3_ck_n                      (ddr3_ck_n),
      .ddr3_ck_p                      (ddr3_ck_p),
      .ddr3_cke                       (ddr3_cke),
      .ddr3_ras_n                     (ddr3_ras_n),
      .ddr3_reset_n                   (ddr3_reset_n),
      .ddr3_we_n                      (ddr3_we_n),
      .ddr3_dq                        (ddr3_dq),
      .ddr3_dqs_n                     (ddr3_dqs_n),
      .ddr3_dqs_p                     (ddr3_dqs_p),
      .ddr3_cs_n                      (ddr3_cs_n),
      .ddr3_dm                        (ddr3_dm),
      .ddr3_odt                       (ddr3_odt),
      .init_calib_complete            (ddr3_running),
      .device_temp_i                  (device_temp),
      // Application interface ports
      .ui_clk                         (ddr3_axi_clk),    // 150MHz clock out
      .ui_addn_clk_0                  (ddr3_axi_clk_x2), // 300MHz clock out
      .ui_addn_clk_1                  (ddr3_idelay_refclk),
      .ui_addn_clk_2                  (),
      .ui_addn_clk_3                  (),
      .ui_addn_clk_4                  (),
      .clk_ref_i                      (ddr3_idelay_refclk),
      .ui_clk_sync_rst                (ddr3_axi_rst),    // Active high Reset signal synchronised to 150MHz
      .aresetn                        (ddr3_axi_rst_reg_n),
      .app_sr_req                     (1'b0),
      .app_sr_active                  (),
      .app_ref_req                    (1'b0),
      .app_ref_ack                    (),
      .app_zq_req                     (1'b0),
      .app_zq_ack                     (),

      // Slave Interface Write Address Ports
      .s_axi_awid                     (s_axi_awid),
      .s_axi_awaddr                   (s_axi_awaddr),
      .s_axi_awlen                    (s_axi_awlen),
      .s_axi_awsize                   (s_axi_awsize),
      .s_axi_awburst                  (s_axi_awburst),
      .s_axi_awlock                   (s_axi_awlock),
      .s_axi_awcache                  (s_axi_awcache),
      .s_axi_awprot                   (s_axi_awprot),
      .s_axi_awqos                    (s_axi_awqos),
      .s_axi_awvalid                  (s_axi_awvalid),
      .s_axi_awready                  (s_axi_awready),
      // Slave Interface Write Data Ports
      .s_axi_wdata                    (s_axi_wdata),
      .s_axi_wstrb                    (s_axi_wstrb),
      .s_axi_wlast                    (s_axi_wlast),
      .s_axi_wvalid                   (s_axi_wvalid),
      .s_axi_wready                   (s_axi_wready),
      // Slave Interface Write Response Ports
      .s_axi_bid                      (s_axi_bid),
      .s_axi_bresp                    (s_axi_bresp),
      .s_axi_bvalid                   (s_axi_bvalid),
      .s_axi_bready                   (s_axi_bready),
      // Slave Interface Read Address Ports
      .s_axi_arid                     (s_axi_arid),
      .s_axi_araddr                   (s_axi_araddr),
      .s_axi_arlen                    (s_axi_arlen),
      .s_axi_arsize                   (s_axi_arsize),
      .s_axi_arburst                  (s_axi_arburst),
      .s_axi_arlock                   (s_axi_arlock),
      .s_axi_arcache                  (s_axi_arcache),
      .s_axi_arprot                   (s_axi_arprot),
      .s_axi_arqos                    (s_axi_arqos),
      .s_axi_arvalid                  (s_axi_arvalid),
      .s_axi_arready                  (s_axi_arready),
      // Slave Interface Read Data Ports
      .s_axi_rid                      (s_axi_rid),
      .s_axi_rdata                    (s_axi_rdata),
      .s_axi_rresp                    (s_axi_rresp),
      .s_axi_rlast                    (s_axi_rlast),
      .s_axi_rvalid                   (s_axi_rvalid),
      .s_axi_rready                   (s_axi_rready),
      // System Clock Ports
      .sys_clk_i                      (sys_clk_i),  // From external 100MHz source.
      .sys_rst                        (global_rst)
   );

   // Temperature monitor module
   mig_7series_v4_2_tempmon #(
      .TEMP_MON_CONTROL("INTERNAL"), .XADC_CLK_PERIOD(8000 /* 125MHz clock period in ps */)
   ) tempmon_i (
      .clk(bus_clk), .xadc_clk(ioport2_clk), .rst(bus_rst),
      .device_temp_i(12'd0 /* ignored */), .device_temp(device_temp)
   );

   /////////////////////////////////////////////////////////////////////
   //
   // Daughterboard GPIO and Debug UART
   //
   //////////////////////////////////////////////////////////////////////

   wire [31:0] db0_gpio_in, db0_gpio_out, db0_gpio_ddr;
   wire [31:0] db1_gpio_in, db1_gpio_out, db1_gpio_ddr;
   wire [31:0] fp_gpio_in, fp_gpio_out, fp_gpio_ddr;
   wire        debug_txd, debug_rxd;

   gpio_atr_io #(.WIDTH(32)) gpio_atr_db0_inst (
      .clk(radio_clk), .gpio_pins({DB0_TX_IO,DB0_RX_IO}),
      .gpio_ddr(db0_gpio_ddr), .gpio_out(db0_gpio_out), .gpio_in(db0_gpio_in)
   );

   gpio_atr_io #(.WIDTH(32)) gpio_atr_db1_inst (
      .clk(radio_clk), .gpio_pins({DB1_TX_IO,DB1_RX_IO}),
      .gpio_ddr(db1_gpio_ddr), .gpio_out(db1_gpio_out), .gpio_in(db1_gpio_in)
   );

`ifdef DEBUG_UART
   gpio_atr_io #(.WIDTH(10)) fp_gpio_atr_inst (
      .clk(radio_clk), .gpio_pins(FrontPanelGpio[9:0]),
      .gpio_ddr(fp_gpio_ddr[9:0]), .gpio_out(fp_gpio_out[9:0]), .gpio_in(fp_gpio_in[9:0])
   );
   assign FrontPanelGpio[11] = debug_txd;
   assign debug_rxd = FrontPanelGpio[10];
`else
   gpio_atr_io #(.WIDTH(12)) fp_gpio_atr_inst (
      .clk(radio_clk), .gpio_pins(FrontPanelGpio[11:0]),
      .gpio_ddr(fp_gpio_ddr[11:0]), .gpio_out(fp_gpio_out[11:0]), .gpio_in(fp_gpio_in[11:0])
   );
   assign debug_rxd = 1'b0;
`endif
   assign fp_gpio_in[31:12] = 20'h0;

   ///////////////////////////////////////////////////////////////////////////////////
   //
   // X300 Core
   //
   ///////////////////////////////////////////////////////////////////////////////////

   x300_core #( .BUS_CLK_RATE(BUS_CLK_RATE) ) x300_core (
      .radio_clk(radio_clk), .radio_rst(radio_rst),
      .bus_clk(bus_clk), .bus_rst(bus_rst), .sw_rst(sw_rst),
      .bus_clk_div2(bus_clk_div2),
      .ce_clk(ce_clk),
      .ce_rst(ce_rst),
      // Radio0 signals
      .rx0(rx0), .tx0(tx0),
      .db0_gpio_in(db0_gpio_in), .db0_gpio_out(db0_gpio_out), .db0_gpio_ddr(db0_gpio_ddr),
      .fp_gpio_in(fp_gpio_in), .fp_gpio_out(fp_gpio_out), .fp_gpio_ddr(fp_gpio_ddr),
      .sen0(sen0), .sclk0(sclk0), .mosi0(mosi0), .miso0(miso0),
      .radio_led0(led0), .radio0_misc_out(radio0_misc_out), .radio0_misc_in(radio0_misc_in),
      // Radio1 signals
      .rx1(rx1), .tx1(tx1),
      .db1_gpio_in(db1_gpio_in), .db1_gpio_out(db1_gpio_out), .db1_gpio_ddr(db1_gpio_ddr),
      .sen1(sen1), .sclk1(sclk1), .mosi1(mosi1), .miso1(miso1),
      .radio_led1(led1), .radio1_misc_out(radio1_misc_out), .radio1_misc_in(radio1_misc_in),
      // I2C bus
      .db_scl(DB_SCL), .db_sda(DB_SDA),
      // External clock gen
      .ext_ref_clk(ref_clk),
      .clock_ref_sel(ClockRefSelect),
      .clock_misc_opt({GPSDO_PWR_ENA, TCXO_ENA}),
      .LMK_Status({LMK_Status1_sync, LMK_Status0_sync}), .LMK_Holdover(LMK_Holdover_sync), .LMK_Lock(LMK_Lock_sync), .LMK_Sync(LMK_Sync_sync),
      .LMK_SEN(LMK_SEN), .LMK_SCLK(LMK_SCLK), .LMK_MOSI(LMK_MOSI),
      .misc_clock_status({1'b0, adc_idlyctrl_rdy_sync, radio_clk_locked_sync}),
      // SFP+ 0 flags
      .SFPP0_SCL(SFPP0_SCL), .SFPP0_SDA(SFPP0_SDA), .SFPP0_ModAbs(SFPP0_ModAbs), .SFPP0_TxFault(SFPP0_TxFault),
      .SFPP0_RxLOS(SFPP0_RxLOS), .SFPP0_RS1(SFPP0_RS1), .SFPP0_RS0(SFPP0_RS0),
      // SFP+ 1 flags
      .SFPP1_SCL(SFPP1_SCL), .SFPP1_SDA(SFPP1_SDA), .SFPP1_ModAbs(SFPP1_ModAbs), .SFPP1_TxFault(SFPP1_TxFault),
      .SFPP1_RxLOS(SFPP1_RxLOS), .SFPP1_RS1(SFPP1_RS1), .SFPP1_RS0(SFPP1_RS0),
      // SFP+ 0 data stream
      .sfp0_tx_tdata(sfp0_tx_tdata), .sfp0_tx_tuser(sfp0_tx_tuser), .sfp0_tx_tlast(sfp0_tx_tlast),
      .sfp0_tx_tvalid(sfp0_tx_tvalid), .sfp0_tx_tready(sfp0_tx_tready),
      .sfp0_rx_tdata(sfp0_rx_tdata), .sfp0_rx_tuser(sfp0_rx_tuser), .sfp0_rx_tlast(sfp0_rx_tlast),
      .sfp0_rx_tvalid(sfp0_rx_tvalid), .sfp0_rx_tready(sfp0_rx_tready),
      .sfp0_phy_status(sfp0_phy_status),
      // SFP+ 1 data stream
      .sfp1_tx_tdata(sfp1_tx_tdata), .sfp1_tx_tuser(sfp1_tx_tuser), .sfp1_tx_tlast(sfp1_tx_tlast),
      .sfp1_tx_tvalid(sfp1_tx_tvalid), .sfp1_tx_tready(sfp1_tx_tready),
      .sfp1_rx_tdata(sfp1_rx_tdata), .sfp1_rx_tuser(sfp1_rx_tuser), .sfp1_rx_tlast(sfp1_rx_tlast),
      .sfp1_rx_tvalid(sfp1_rx_tvalid), .sfp1_rx_tready(sfp1_rx_tready),
      .sfp1_phy_status(sfp1_phy_status),
      // Wishbone Slave Interface(s)
      .sfp0_wb_dat_i(sfp0_wb_dat_i), .sfp0_wb_dat_o(sfp0_wb_dat_o), .sfp0_wb_adr(sfp0_wb_adr),
      .sfp0_wb_sel(), .sfp0_wb_ack(sfp0_wb_ack), .sfp0_wb_stb(sfp0_wb_stb),
      .sfp0_wb_cyc(sfp0_wb_cyc), .sfp0_wb_we(sfp0_wb_we), .sfp0_wb_int(sfp0_wb_int),
      .sfp1_wb_dat_i(sfp1_wb_dat_i), .sfp1_wb_dat_o(sfp1_wb_dat_o), .sfp1_wb_adr(sfp1_wb_adr),
      .sfp1_wb_sel(), .sfp1_wb_ack(sfp1_wb_ack), .sfp1_wb_stb(sfp1_wb_stb),
      .sfp1_wb_cyc(sfp1_wb_cyc), .sfp1_wb_we(sfp1_wb_we), .sfp1_wb_int(sfp1_wb_int),
      // Time
      .pps(pps),.pps_select(pps_select), .pps_out_enb(pps_out_enb),
      .ref_freq(ref_freq), .ref_freq_changed(ref_freq_changed),
      // GPS Signals
      .gps_txd(GPS_SER_IN), .gps_rxd(GPS_SER_OUT),
      // Debug UART
      .debug_rxd(debug_rxd), .debug_txd(debug_txd),
      // Misc.
      .led_misc(leds),
      .xadc_readback({20'h0, device_temp}),
      .debug0(), .debug1(), .debug2(),
      // DRAM signals.
      .ddr3_axi_clk              (ddr3_axi_clk),
      .ddr3_axi_clk_x2           (ddr3_axi_clk_x2),
      .ddr3_axi_rst              (ddr3_axi_rst),
      // Slave Interface Write Address Ports
      .ddr3_axi_awid             (s_axi_awid),
      .ddr3_axi_awaddr           (s_axi_awaddr),
      .ddr3_axi_awlen            (s_axi_awlen),
      .ddr3_axi_awsize           (s_axi_awsize),
      .ddr3_axi_awburst          (s_axi_awburst),
      .ddr3_axi_awlock           (s_axi_awlock),
      .ddr3_axi_awcache          (s_axi_awcache),
      .ddr3_axi_awprot           (s_axi_awprot),
      .ddr3_axi_awqos            (s_axi_awqos),
      .ddr3_axi_awvalid          (s_axi_awvalid),
      .ddr3_axi_awready          (s_axi_awready),
      // Slave Interface Write Data Ports
      .ddr3_axi_wdata            (s_axi_wdata),
      .ddr3_axi_wstrb            (s_axi_wstrb),
      .ddr3_axi_wlast            (s_axi_wlast),
      .ddr3_axi_wvalid           (s_axi_wvalid),
      .ddr3_axi_wready           (s_axi_wready),
      // Slave Interface Write Response Ports
      .ddr3_axi_bid              (s_axi_bid),
      .ddr3_axi_bresp            (s_axi_bresp),
      .ddr3_axi_bvalid           (s_axi_bvalid),
      .ddr3_axi_bready           (s_axi_bready),
      // Slave Interface Read Address Ports
      .ddr3_axi_arid             (s_axi_arid),
      .ddr3_axi_araddr           (s_axi_araddr),
      .ddr3_axi_arlen            (s_axi_arlen),
      .ddr3_axi_arsize           (s_axi_arsize),
      .ddr3_axi_arburst          (s_axi_arburst),
      .ddr3_axi_arlock           (s_axi_arlock),
      .ddr3_axi_arcache          (s_axi_arcache),
      .ddr3_axi_arprot           (s_axi_arprot),
      .ddr3_axi_arqos            (s_axi_arqos),
      .ddr3_axi_arvalid          (s_axi_arvalid),
      .ddr3_axi_arready          (s_axi_arready),
      // Slave Interface Read Data Ports
      .ddr3_axi_rid              (s_axi_rid),
      .ddr3_axi_rdata            (s_axi_rdata),
      .ddr3_axi_rresp            (s_axi_rresp),
      .ddr3_axi_rlast            (s_axi_rlast),
      .ddr3_axi_rvalid           (s_axi_rvalid),
      .ddr3_axi_rready           (s_axi_rready),
      // IoPort2 Message FIFOs
      .o_iop2_msg_tdata          (o_iop2_msg_tdata),
      .o_iop2_msg_tvalid         (o_iop2_msg_tvalid),
      .o_iop2_msg_tlast          (o_iop2_msg_tlast),
      .o_iop2_msg_tready         (o_iop2_msg_tready),
      .i_iop2_msg_tdata          (i_iop2_msg_tdata),
      .i_iop2_msg_tvalid         (i_iop2_msg_tvalid),
      .i_iop2_msg_tlast          (i_iop2_msg_tlast),
      .i_iop2_msg_tready         (i_iop2_msg_tready),
      // PCIe DMA Data
      .pcio_tdata                (pcio_tdata),
      .pcio_tuser                (pcio_tuser),
      .pcio_tlast                (pcio_tlast),
      .pcio_tvalid               (pcio_tvalid),
      .pcio_tready               (pcio_tready),
      .pcii_tdata                (pcii_tdata),
      .pcii_tuser                (pcii_tuser),
      .pcii_tlast                (pcii_tlast),
      .pcii_tvalid               (pcii_tvalid),
      .pcii_tready               (pcii_tready)
   );

   assign {DB_ADC_RESET, DB_DAC_RESET,DB0_DAC_ENABLE} = radio0_misc_out[2:0];
   assign {DB1_DAC_ENABLE}                            = radio1_misc_out[0];   //[2:1] unused

   /////////////////////////////////////////////////////////////////////
   //
   // PUDC Workaround
   //
   //////////////////////////////////////////////////////////////////////
   // This is a workaround for a silicon bug in Series 7 FPGA where a
   // race condition with the reading of PUDC during the erase of the FPGA
   // image cause glitches on output IO pins. This glitch happens even if
   // you have PUDC correctly pulled high!!  When PUDC is high the pull up
   // resistor should never be enabled on the IO lines, however there is a
   // race condition that causes this to not be the case.
   //
   // Workaround:
   // - Define the PUDC pin in the XDC file with a pullup.
   // - Implements an IBUF on the PUDC input and make sure that it does
   //   not get optimized out.
   (* dont_touch = "true" *) wire fpga_pudc_b_buf;
   IBUF pudc_ibuf_i (
      .I(FPGA_PUDC_B),
      .O(fpga_pudc_b_buf));

endmodule // x300

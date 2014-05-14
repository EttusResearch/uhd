///////////////////////////////////
//
// NOTE: A set of precompiler directives configure the features in an FPGA build
// and are listed here. These should be set exclusively using the Makefile mechanism provided.
//
// ETH10G_PORT0 - Ethernet Port0 is configured for 10G (default is 1G)
// ETH10G_PORT1 - Ethernet Port1 is configured for 10G (default is 1G)
// NO_DRAM_FIFOS - All DRAM I/F logic, supporting AXI4 and VFIFOs' replaced by internal SRAM FIFOs.
// DELETE_DSP0 - Deletes DUC and DDC from Radio0
// DELETE_DSP1 - Deletes DUC and DDC from Radio1
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
`ifndef NO_DRAM_FIFOS
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
`endif
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
   output SFPP1_TxDisable  // These are actually open drain outputs

   );

   //
   // LOG2 function.
   //
   function integer clogb2 (input integer size);
      begin
     size = size - 1;
     for (clogb2=1; size>1; clogb2=clogb2+1)
           size = size >> 1;
      end
   endfunction // clogb2


   wire     radio_clk, radio_clk_2x;
   wire     global_rst, radio_rst, bus_rst;

   wire [3:0]   sw_rst;


   wire [63:0]  vita_time;
   wire [2:0]   led0, led1;

   wire [31:0]  debug0, debug1;

   /////////////////////////////////////////////////////////////////////
   //
   // Debug logic on front panel GPIO pins
   //
   //////////////////////////////////////////////////////////////////////
   wire     debug_txd, debug_rxd;

   `ifdef DEBUG_UART
   assign FrontPanelGpio[11] = debug_txd;

   assign debug_rxd = FrontPanelGpio[10];


   `endif

   ////////////////////////////////////////////////////////////////////
   //
   // Generate Bus Clock and PCIe Clocks.
   // Source clock comes from U19 which is fixed freq
   // and bufferd to be used by STC3 also (Page17 schematics).
   //
   ////////////////////////////////////////////////////////////////////

   wire     fpga_clk125, bus_clk, ioport2_clk, rio40_clk, ioport2_idelay_ref_clk;
   wire     bus_clk_locked, rio40_clk_locked, rio40_clk_reset;

   IBUFG fpga_125MHz_clk_buf (
     .I(FPGA_125MHz_CLK),
     .O(fpga_clk125));

   bus_clk_gen bus_clk_gen (
      .CLK_IN1(fpga_clk125),                //Input Clock: 125MHz Clock from STC3
      .CLK_OUT1(bus_clk),                   //Output Clock 1: 166.666667MHz
      .CLK_OUT2(ioport2_clk),               //Output Clock 2: 125MHz
      .RESET(1'b0),
      .LOCKED(bus_clk_locked));

   //rio40_clk and ioport2_idelay_ref_clk cannot share a PLL/MMCM reset with ioport2_clk
   //so they have to come from a different clocking primitive instance
   pcie_clk_gen pcie_clk_gen (
      .CLK_IN1(fpga_clk125),                //Input Clock: 125MHz Clock from STC3
      .CLK_OUT1(rio40_clk),                 //Output Clock 1: 40MHz
      .CLK_OUT2(ioport2_idelay_ref_clk),    //Output Clock 2: 200MHz
      .RESET(rio40_clk_reset),
      .LOCKED(rio40_clk_locked));

   /////////////////////////////////////////////////////////////////////
   //
   // 10MHz Reference clock
   //
   //////////////////////////////////////////////////////////////////////
   wire ref_clk_10mhz;
   IBUFDS IBUFDS_10_MHz (
        .O(ref_clk_10mhz),
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
   wire sync_dacs_radio0, sync_dacs_radio1;
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
   ////////////////////////////////////////////////////////////////////
   wire        radio_clk_locked;

   radio_clk_gen radio_clk_gen
     (.CLK_IN1_P(FPGA_CLK_p), .CLK_IN1_N(FPGA_CLK_n), .CLK_OUT1(radio_clk), .CLK_OUT2(radio_clk_2x),
      .RESET(sw_rst[2]), .LOCKED(radio_clk_locked));
   defparam radio_clk_gen.clkin1_buf.DIFF_TERM = "TRUE";

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

   reset_sync radio_reset_sync
     (
      .clk(radio_clk),
      .reset_in(global_rst || !bus_clk_locked || sw_rst[1]),
      .reset_out(radio_rst)
      );

   reset_sync int_reset_sync
     (
      .clk(bus_clk),
      .reset_in(global_rst || !bus_clk_locked),
      .reset_out(bus_rst)
      );

   ////////////////////////////////////////////////////////////////////
   // PPS
   // Support for internal, external, and GPSDO PPS inputs
   // Every attempt to minimize propagation between the external PPS
   // input and outputs to support daisy-chaining the signal.
   ////////////////////////////////////////////////////////////////////

   // Generate an internal PPS signal with a 25% duty cycle
   reg [31:0] pps_count;
   wire int_pps = (pps_count < 32'd2500000);
   always @(posedge ref_clk_10mhz) begin
      if (pps_count >= 32'd9999999)
         pps_count <= 32'b0;
      else
         pps_count <= pps_count + 1'b1;
   end

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

   /////////////////////////////////////////////////////////////////////
   //
   // ADC Interface for ADS62P48
   //
   /////////////////////////////////////////////////////////////////////
   wire [13:0] rx0_q_inv, rx1_q_inv, rx0_i, rx1_i;
   // Analog diff pairs on I side of ADC are inverted for layout reasons, but data diff pairs are all swapped as well
   //  so I gets a double negative, and is unchanged.  Q must be inverted.

   capture_ddrlvds #(.WIDTH(14),.X300(1)) cap_db0
     (.clk(radio_clk), .reset(radio_rst), .ssclk_p(DB0_ADC_DCLK_P), .ssclk_n(DB0_ADC_DCLK_N),
      .in_p({{DB0_ADC_DA6_P, DB0_ADC_DA5_P, DB0_ADC_DA4_P, DB0_ADC_DA3_P, DB0_ADC_DA2_P, DB0_ADC_DA1_P, DB0_ADC_DA0_P},
         {DB0_ADC_DB6_P, DB0_ADC_DB5_P, DB0_ADC_DB4_P, DB0_ADC_DB3_P, DB0_ADC_DB2_P, DB0_ADC_DB1_P, DB0_ADC_DB0_P}}),

      .in_n({{DB0_ADC_DA6_N, DB0_ADC_DA5_N, DB0_ADC_DA4_N, DB0_ADC_DA3_N, DB0_ADC_DA2_N, DB0_ADC_DA1_N, DB0_ADC_DA0_N},
         {DB0_ADC_DB6_N, DB0_ADC_DB5_N, DB0_ADC_DB4_N, DB0_ADC_DB3_N, DB0_ADC_DB2_N, DB0_ADC_DB1_N, DB0_ADC_DB0_N}}),
      .out({rx0_i,rx0_q_inv}));
   assign rx0[31:0] = { rx0_i, 2'b00, ~rx0_q_inv, 2'b00 };

   capture_ddrlvds #(.WIDTH(14),.X300(1)) cap_db1
     (.clk(radio_clk),  .reset(radio_rst), .ssclk_p(DB1_ADC_DCLK_P), .ssclk_n(DB1_ADC_DCLK_N),
      .in_p({{DB1_ADC_DA6_P, DB1_ADC_DA5_P, DB1_ADC_DA4_P, DB1_ADC_DA3_P, DB1_ADC_DA2_P, DB1_ADC_DA1_P, DB1_ADC_DA0_P},
         {DB1_ADC_DB6_P, DB1_ADC_DB5_P, DB1_ADC_DB4_P, DB1_ADC_DB3_P, DB1_ADC_DB2_P, DB1_ADC_DB1_P, DB1_ADC_DB0_P}}),

      .in_n({{DB1_ADC_DA6_N, DB1_ADC_DA5_N, DB1_ADC_DA4_N, DB1_ADC_DA3_N, DB1_ADC_DA2_N, DB1_ADC_DA1_N, DB1_ADC_DA0_N},
         {DB1_ADC_DB6_N, DB1_ADC_DB5_N, DB1_ADC_DB4_N, DB1_ADC_DB3_N, DB1_ADC_DB2_N, DB1_ADC_DB1_N, DB1_ADC_DB0_N}}),
      .out({rx1_i,rx1_q_inv}));
   assign rx1[31:0] = { rx1_i, 2'b00, ~rx1_q_inv, 2'b00 };

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
      .tx_clk_2x(radio_clk_2x), .tx_clk_1x(radio_clk),
      .i(~tx0[31:16]), .q(~tx0[15:0]), // invert b/c Analog diff pairs are swapped for layout
      .sync_dacs(sync_dacs_radio0|sync_dacs_radio1)
      );


   gen_ddrlvds gen_db1
     (
      .reset(radio_rst),
      .tx_clk_2x_p(DB1_DAC_DCI_P), .tx_clk_2x_n(DB1_DAC_DCI_N),
      .tx_frame_p(DB1_DAC_FRAME_P), .tx_frame_n(DB1_DAC_FRAME_N),
      .tx_d_p({DB1_DAC_D7_P,DB1_DAC_D6_P,DB1_DAC_D5_P,DB1_DAC_D4_P,DB1_DAC_D3_P,DB1_DAC_D2_P,DB1_DAC_D1_P,DB1_DAC_D0_P}),
      .tx_d_n({DB1_DAC_D7_N,DB1_DAC_D6_N,DB1_DAC_D5_N,DB1_DAC_D4_N,DB1_DAC_D3_N,DB1_DAC_D2_N,DB1_DAC_D1_N,DB1_DAC_D0_N}),
      .tx_clk_2x(radio_clk_2x), .tx_clk_1x(radio_clk),
      .i(~tx1[31:16]), .q(~tx1[15:0]), // invert b/c Analog diff pairs are swapped for layout
      .sync_dacs(sync_dacs_radio0|sync_dacs_radio1)
      );


   wire [7:0] leds;
   assign { LED_ACT1, LED_ACT2,
        LED_LINK1, LED_LINK2,
        LED_LINKSTAT, LED_LINKACT } = ~leds;

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

   wire [DMA_STREAM_WIDTH-1:0] dmatx_tdata,  dmarx_tdata,  pcii_tdata,  pcio_tdata;
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
      .IOP2_MSG_WIDTH(IOP2_MSG_WIDTH)
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

      //DMA TX FIFO (Bus Clock Domain)
      .dmatx_tdata(dmatx_tdata),
      .dmatx_tlast(dmatx_tlast),
      .dmatx_tvalid(dmatx_tvalid),
      .dmatx_tready(dmatx_tready),

      //DMA RX FIFO (Bus Clock Domain)
      .dmarx_tdata(dmarx_tdata),
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
   
   axi_fifo_short #(.WIDTH(DMA_STREAM_WIDTH+1)) pcii_pipeline_srl (
      .clk(bus_clk), .reset(bus_rst), .clear(1'b0),
      .i_tdata({dmatx_tlast, dmatx_tdata}), .i_tvalid(dmatx_tvalid), .i_tready(dmatx_tready),
      .o_tdata({pcii_tlast, pcii_tdata}), .o_tvalid(pcii_tvalid), .o_tready(pcii_tready),
      .space(), .occupied());

   axi_fifo_short #(.WIDTH(DMA_STREAM_WIDTH+1)) pcio_pipeline_srl (
      .clk(bus_clk), .reset(bus_rst), .clear(1'b0),
      .i_tdata({pcio_tlast, pcio_tdata}), .i_tvalid(pcio_tvalid), .i_tready(pcio_tready),
      .o_tdata({dmarx_tlast, dmarx_tdata}), .o_tvalid(dmarx_tvalid), .o_tready(dmarx_tready),
      .space(), .occupied());

   //////////////////////////////////////////////////////////////////////
   //
   // Configure Ethernet PHY clocking
   //
   //////////////////////////////////////////////////////////////////////
`ifdef BUILD_1G
   wire        gige_sfp_clk;

   IBUFDS_GTE2 gige_clk_sfp_pin (.O(gige_sfp_clk),.I(ETH_CLK_p),.IB(ETH_CLK_n), .CEB(1'b0));
`endif
`ifdef BUILD_10G
   wire        xgige_sfp_clk;
   wire        xgige_sfp_clk_bufg;

   IBUFDS_GTE2 clk_sfp_pin (.O(xgige_sfp_clk),.I(XG_CLK_p),.IB(XG_CLK_n), .CEB(1'b0));

   BUFG xgige_bufg_inst
     (
      .I (xgige_sfp_clk),
      .O (xgige_sfp_clk_bufg)
      );
`endif

   //////////////////////////////////////////////////////////////////////
   //
   // Configure Ethernet PHY implemented for PORT0
   //
   //////////////////////////////////////////////////////////////////////
   wire       mdc0, mdio_in0, mdio_out0, mdio_tri0;
   wire [4:0] prtad0 = 5'b00100;  // MDIO address is 4

`ifdef ETH10G_PORT0
   //////////////////////////////////////////////////////////////////////
   //
   // 10GBaseR PHY with XGMII interface to MAC
   //
   //////////////////////////////////////////////////////////////////////
   wire        xgmii_clk0;  // 156.25MHz
   wire [63:0] xgmii_txd0;
   wire [7:0]  xgmii_txc0;
   wire [63:0] xgmii_rxd0;
   wire [7:0]  xgmii_rxc0;

   wire [7:0]  core_status0;
   wire        xge_phy_resetdone0;

   ten_gig_eth_pcs_pma_x300_top ten_gig_eth_pcs_pma_x300_top_port0
     (
      .refclk156(xgige_sfp_clk),
      .refclk156_buf(xgige_sfp_clk_bufg),
      .clk156(xgmii_clk0),
      .reset(global_rst | sw_rst[0]),
      .xgmii_txd(xgmii_txd0),
      .xgmii_txc(xgmii_txc0),
      .xgmii_rxd(xgmii_rxd0),
      .xgmii_rxc(xgmii_rxc0),
      .txp(SFP0_TX_p),
      .txn(SFP0_TX_n),
      .rxp(SFP0_RX_p),
      .rxn(SFP0_RX_n),
      .mdc(mdc0),
      .mdio_in(mdio_in0),
      .mdio_out(mdio_out0),
      .mdio_tri(mdio_tri0),
      .prtad(prtad0),
      .core_status(core_status0),    // Ignore for now. IJB
      .resetdone(xge_phy_resetdone0),
      .signal_detect(~SFPP0_RxLOS),  // Undocumented, but it seems Xilinx expect this to be inverted.
      .tx_fault(SFPP0_TxFault),
      .tx_disable(SFPP0_TxDisable)
      );

`else

   //////////////////////////////////////////////////////////////////////
   //
   // 1000Base-X PHY with GMII interface to MAC
   //
   //////////////////////////////////////////////////////////////////////
   wire [31:0] gige_phy_misc_dbg0;
   wire [15:0] gige_phy_int_data0;
   wire [7:0]  gmii_txd0, gmii_rxd0;
   wire        gmii_tx_en0, gmii_tx_er0, gmii_rx_dv0, gmii_rx_er0;
   wire        gmii_clk0;
   wire [15:0] gmii_status0;

   assign SFPP0_TxDisable = 1'b0; // Always on.

   gige_phy_mdio gige_phy_port0
     (
      .reset(global_rst | sw_rst[0]),
      .independent_clock(bus_clk),
      .sfp_clk(gige_sfp_clk),
      .SFP_TX_p(SFP0_TX_p),
      .SFP_TX_n(SFP0_TX_n),
      .SFP_RX_p(SFP0_RX_p),
      .SFP_RX_n(SFP0_RX_n),
      .gmii_clk(gmii_clk0),
      .gmii_txd(gmii_txd0),
      .gmii_tx_en(gmii_tx_en0),
      .gmii_tx_er(gmii_tx_er0),
      .gmii_rxd(gmii_rxd0),
      .gmii_rx_dv(gmii_rx_dv0),
      .gmii_rx_er(gmii_rx_er0),
      .misc_debug(gige_phy_misc_dbg0),
      .int_data(gige_phy_int_data0),
      .status_vector(gmii_status0),
      // MDIO signals
      .prtad(prtad0),
      .mdc(mdc0),
      .mdio_i(mdio_in0),
      .mdio_o(mdio_out0),
      .mdio_t(mdio_tri0)
      );

      wire [35:0] CONTROL0;


/* -----\/----- EXCLUDED -----\/-----

   chipscope_ila chipscope_ila_i0
     (
      .CONTROL(CONTROL0), // INOUT BUS [35:0]
      .CLK(mdc0), // IN
      .TRIG0(
         {
           gmii_status0, //31:16
           3'b0, prtad0, //15:8
           4'b0, 1'b0, mdio_in0, mdio_out0, mdio_tri0 //7:0
          }
         ) // IN BUS [191:0]
      );

   chipscope_icon chipscope_icon_i0
     (
      .CONTROL0(CONTROL0) // INOUT BUS [35:0]
      );

 -----/\----- EXCLUDED -----/\----- */

`endif // !`ifdef

   //////////////////////////////////////////////////////////////////////
   //
   // Configure Ethernet PHY implemented for PORT1
   //
   //////////////////////////////////////////////////////////////////////
   wire        mdc1, mdio_in1, mdio_out1, mdio_tri1;
   wire [4:0]  prtad1 = 5'b00100;  // MDIO address is 4

`ifdef ETH10G_PORT1
   //////////////////////////////////////////////////////////////////////
   //
   // 10GBaseR PHY with XGMII interface to MAC
   //
   //////////////////////////////////////////////////////////////////////
   wire        xgmii_clk1;  // 156.25MHz
   wire [63:0] xgmii_txd1;
   wire [7:0]  xgmii_txc1;
   wire [63:0] xgmii_rxd1;
   wire [7:0]  xgmii_rxc1;

   wire [7:0]  core_status1;
   wire        xge_phy_resetdone1;

    ten_gig_eth_pcs_pma_x300_top ten_gig_eth_pcs_pma_x300_top_port1
     (
      .refclk156(xgige_sfp_clk),
      .refclk156_buf(xgige_sfp_clk_bufg),
      .clk156(xgmii_clk1),
      .reset(global_rst | sw_rst[0]),
      .xgmii_txd(xgmii_txd1),
      .xgmii_txc(xgmii_txc1),
      .xgmii_rxd(xgmii_rxd1),
      .xgmii_rxc(xgmii_rxc1),
      .txp(SFP1_TX_p),
      .txn(SFP1_TX_n),
      .rxp(SFP1_RX_p),
      .rxn(SFP1_RX_n),
      .mdc(mdc1),
      .mdio_in(mdio_in1),
      .mdio_out(mdio_out1),
      .mdio_tri(mdio_tri1),
      .prtad(prtad0),
      .core_status(core_status1),    // Ignore for now. IJB
      .resetdone(xge_phy_resetdone1),
      .signal_detect(~SFPP1_RxLOS),  // Undocumented, but it seems Xilinx expect this to be inverted.
      .tx_fault(SFPP1_TxFault),
      .tx_disable(SFPP1_TxDisable)
      );

`else

   //////////////////////////////////////////////////////////////////////
   //
   // 1000Base-X PHY with GMII interface to MAC
   //
   //////////////////////////////////////////////////////////////////////
   wire [31:0] gige_phy_misc_dbg1;
   wire [15:0] gige_phy_int_data1;
   wire [7:0]  gmii_txd1, gmii_rxd1;
   wire        gmii_tx_en1, gmii_tx_er1, gmii_rx_dv1, gmii_rx_er1;
   wire        gmii_clk1;
   wire [15:0] gmii_status1;

   assign SFPP1_TxDisable = 1'b0; // Always on.

   gige_phy_mdio gige_phy_port1
     (
      .reset(global_rst | sw_rst[0]),
      .independent_clock(bus_clk),
      .sfp_clk(gige_sfp_clk),
      .SFP_TX_p(SFP1_TX_p),
      .SFP_TX_n(SFP1_TX_n),
      .SFP_RX_p(SFP1_RX_p),
      .SFP_RX_n(SFP1_RX_n),
      .gmii_clk(gmii_clk1),
      .gmii_txd(gmii_txd1),
      .gmii_tx_en(gmii_tx_en1),
      .gmii_tx_er(gmii_tx_er1),
      .gmii_rxd(gmii_rxd1),
      .gmii_rx_dv(gmii_rx_dv1),
      .gmii_rx_er(gmii_rx_er1),
      .misc_debug(gige_phy_misc_dbg1),
      .int_data(gige_phy_int_data1),
      .status_vector(gmii_status1),
      // MDIO signals
      .prtad(prtad0),
      .mdc(mdc1),
      .mdio_i(mdio_in1),
      .mdio_o(mdio_out1),
      .mdio_t(mdio_tri1)
      );
`endif // !`ifdef

   ///////////////////////////////////////////////////////////////////////////////////
   //
   // Debug bus for logic analyzer use.
   //
   ///////////////////////////////////////////////////////////////////////////////////

//   assign {DB0_TX_IO,DB0_RX_IO} = debug0;

//   assign {DB1_TX_IO,DB1_RX_IO} = debug1;

/*
   assign debug = {
        2'b0, SFPP1_ModAbs, SFPP1_RxLOS, SFPP1_TxFault, SFPP1_RS0, SFPP1_RS1, SFPP1_TxDisable, //8
        4'b0, gmii_tx_en, gmii_tx_er, gmii_rx_dv, gmii_rx_er, //8
        gmii_txd, gmii_rxd //16
    };
*/


`ifndef NO_DRAM_FIFOS

   ///////////////////////////////////////////////////////////////////////////////////
   //
   // Xilinx DDR3 Controller and PHY.
   //
   ///////////////////////////////////////////////////////////////////////////////////


   wire        ddr3_axi_clk;           // 1/4 DDR external clock rate (250MHz)
   wire        ddr3_axi_rst;           // Synchronized to ddr_sys_clk
   wire        ddr3_running;           // DRAM calibration complete.

   // Slave Interface Write Address Ports
   wire [3:0]  s_axi_awid;
   wire [31:0] s_axi_awaddr;
   wire [7:0]  s_axi_awlen;
   wire [2:0]  s_axi_awsize;
   wire [1:0]  s_axi_awburst;
   wire [0:0]  s_axi_awlock;
   wire [3:0]  s_axi_awcache;
   wire [2:0]  s_axi_awprot;
   wire [3:0]  s_axi_awqos;
   wire        s_axi_awvalid;
   wire        s_axi_awready;
   // Slave Interface Write Data Ports
   wire [127:0] s_axi_wdata;
   wire [15:0]  s_axi_wstrb;
   wire     s_axi_wlast;
   wire     s_axi_wvalid;
   wire     s_axi_wready;
   // Slave Interface Write Response Ports
   wire     s_axi_bready;
   wire [3:0]   s_axi_bid;
   wire [1:0]   s_axi_bresp;
   wire     s_axi_bvalid;
   // Slave Interface Read Address Ports
   wire [3:0]   s_axi_arid;
   wire [31:0]  s_axi_araddr;
   wire [7:0]   s_axi_arlen;
   wire [2:0]   s_axi_arsize;
   wire [1:0]   s_axi_arburst;
   wire [0:0]   s_axi_arlock;
   wire [3:0]   s_axi_arcache;
   wire [2:0]   s_axi_arprot;
   wire [3:0]   s_axi_arqos;
   wire     s_axi_arvalid;
   wire     s_axi_arready;
   // Slave Interface Read Data Ports
   wire     s_axi_rready;
   wire [3:0]   s_axi_rid;
   wire [127:0] s_axi_rdata;
   wire [1:0]   s_axi_rresp;
   wire     s_axi_rlast;
   wire     s_axi_rvalid;

   reg      ddr3_axi_rst_reg_n;

   // Copied this reset circuit from example design.
   always @(posedge ddr3_axi_clk)
     ddr3_axi_rst_reg_n <= ~ddr3_axi_rst;


/* -----\/----- EXCLUDED -----\/-----
   //-***************************************************************************
   // Traffic Gen related localparams
   //-***************************************************************************
   localparam BL_WIDTH              = 10;
   localparam PORT_MODE             = "BI_MODE";
   localparam DATA_MODE             = 4'b0010;
   localparam ADDR_MODE             = 4'b0011;
   localparam TST_MEM_INSTR_MODE    = "R_W_INSTR_MODE";
   localparam EYE_TEST              = "FALSE";
                                     // set EYE_TEST = "TRUE" to probe memory
                                     // signals. Traffic Generator will only
                                     // write to one single location and no
                                     // read transactions will be generated.
   localparam DATA_PATTERN          = "DGEN_ALL";
                                      // For small devices, choose one only.
                                      // For large device, choose "DGEN_ALL"
                                      // "DGEN_HAMMER", "DGEN_WALKING1",
                                      // "DGEN_WALKING0","DGEN_ADDR","
                                      // "DGEN_NEIGHBOR","DGEN_PRBS","DGEN_ALL"
   localparam CMD_PATTERN           = "CGEN_ALL";
                                      // "CGEN_PRBS","CGEN_FIXED","CGEN_BRAM",
                                      // "CGEN_SEQUENTIAL", "CGEN_ALL"
   localparam BEGIN_ADDRESS         = 32'h00000000;
   localparam END_ADDRESS           = 32'h00ffffff;
   localparam PRBS_EADDR_MASK_POS   = 32'hff000000;
   localparam CMD_WDT               = 'h3FF;
   localparam WR_WDT                = 'h1FFF;
   localparam RD_WDT                = 'h3FF;
   localparam SEL_VICTIM_LINE       = 0;
   localparam ENFORCE_RD_WR         = 0;
   localparam ENFORCE_RD_WR_CMD     = 8'h11;
   localparam ENFORCE_RD_WR_PATTERN = 3'b000;
   localparam C_EN_WRAP_TRANS       = 0;
   localparam C_AXI_NBURST_TEST     = 0;

   //-***************************************************************************
   // The following localparams refer to width of various ports
   //-***************************************************************************
   localparam BANK_WIDTH            = 3;
                                     // # of memory Bank Address bits.
   localparam CK_WIDTH              = 1;
                                     // # of CK/CK# outputs to memory.
   localparam COL_WIDTH             = 10;
                                     // # of memory Column Address bits.
   localparam CS_WIDTH              = 1;
                                     // # of unique CS outputs to memory.
   localparam nCS_PER_RANK          = 1;
                                     // # of unique CS outputs per rank for phy
   localparam CKE_WIDTH             = 1;
                                     // # of CKE outputs to memory.
   localparam DATA_BUF_ADDR_WIDTH   = 5;
   localparam DQ_CNT_WIDTH          = 5;
                                     // = ceil(log2(DQ_WIDTH))
   localparam DQ_PER_DM             = 8;
   localparam DM_WIDTH              = 4;
                                     // # of DM (data mask)
   localparam DQ_WIDTH              = 32;
                                     // # of DQ (data)
   localparam DQS_WIDTH             = 4;
   localparam DQS_CNT_WIDTH         = 2;
                                     // = ceil(log2(DQS_WIDTH))
   localparam DRAM_WIDTH            = 8;
                                     // # of DQ per DQS
   localparam ECC                   = "OFF";
   localparam nBANK_MACHS           = 4;
   localparam RANKS                 = 1;
                                     // # of Ranks.
   localparam ODT_WIDTH             = 1;
                                     // # of ODT outputs to memory.
   localparam ROW_WIDTH             = 15;
                                     // # of memory Row Address bits.
   localparam ADDR_WIDTH            = 29;
                                     // # = RANK_WIDTH + BANK_WIDTH
                                     //     + ROW_WIDTH + COL_WIDTH;
                                     // Chip Select is always tied to low for
                                     // single rank devices
   localparam USE_CS_PORT          = 1;
                                     // # = 1, When Chip Select (CS#) output is enabled
                                     //   = 0, When Chip Select (CS#) output is disabled
                                     // If CS_N disabled, user must connect
                                     // DRAM CS_N input(s) to ground
   localparam USE_DM_PORT           = 1;
                                     // # = 1, When Data Mask option is enabled
                                     //   = 0, When Data Mask option is disbaled
                                     // When Data Mask option is disabled in
                                     // MIG Controller Options page, the logic
                                     // related to Data Mask should not get
                                     // synthesized
   localparam USE_ODT_PORT          = 1;
                                     // # = 1, When ODT output is enabled
                                     //   = 0, When ODT output is disabled
   localparam PHY_CONTROL_MASTER_BANK = 1;
                                     // The bank index where master PHY_CONTROL resides;
                                     // equal to the PLL residing bank
   localparam MEM_DENSITY           = "4Gb";
                                     // Indicates the density of the Memory part
                                     // Added for the sake of Vivado simulations
   localparam MEM_SPEEDGRADE        = "125";
                                     // Indicates the Speed grade of Memory Part
                                     // Added for the sake of Vivado simulations
   localparam MEM_DEVICE_WIDTH      = 16;
                                     // Indicates the device width of the Memory Part
                                     // Added for the sake of Vivado simulations

   //-***************************************************************************
   // The following localparams are mode register settings
   //-***************************************************************************
   localparam AL                    = "0";
                                     // DDR3 SDRAM:
                                     // Additive Latency (Mode Register 1).
                                     // # = "0", "CL-1", "CL-2".
                                     // DDR2 SDRAM:
                                     // Additive Latency (Extended Mode Register).
   localparam nAL                   = 0;
                                     // # Additive Latency in number of clock
                                     // cycles.
   localparam BURST_MODE            = "8";
                                     // DDR3 SDRAM:
                                     // Burst Length (Mode Register 0).
                                     // # = "8", "4", "OTF".
                                     // DDR2 SDRAM:
                                     // Burst Length (Mode Register).
                                     // # = "8", "4".
   localparam BURST_TYPE            = "SEQ";
                                     // DDR3 SDRAM: Burst Type (Mode Register 0).
                                     // DDR2 SDRAM: Burst Type (Mode Register).
                                     // # = "SEQ" - (Sequential),
                                     //   = "INT" - (Interleaved).
   localparam CL                    = 7;
                                     // in number of clock cycles
                                     // DDR3 SDRAM: CAS Latency (Mode Register 0).
                                     // DDR2 SDRAM: CAS Latency (Mode Register).
   localparam CWL                   = 6;
                                     // in number of clock cycles
                                     // DDR3 SDRAM: CAS Write Latency (Mode Register 2).
                                     // DDR2 SDRAM: Can be ignored
   localparam OUTPUT_DRV            = "HIGH";
                                     // Output Driver Impedance Control (Mode Register 1).
                                     // # = "HIGH" - RZQ/7,
                                     //   = "LOW" - RZQ/6.
   localparam RTT_NOM               = "60";
                                     // RTT_NOM (ODT) (Mode Register 1).
                                     //   = "120" - RZQ/2,
                                     //   = "60"  - RZQ/4,
                                     //   = "40"  - RZQ/6.
   localparam RTT_WR                = "OFF";
                                     // RTT_WR (ODT) (Mode Register 2).
                                     // # = "OFF" - Dynamic ODT off,
                                     //   = "120" - RZQ/2,
                                     //   = "60"  - RZQ/4,
   localparam ADDR_CMD_MODE         = "1T" ;
                                     // # = "1T", "2T".
   localparam REG_CTRL              = "OFF";
                                     // # = "ON" - RDIMMs,
                                     //   = "OFF" - Components, SODIMMs, UDIMMs.
   localparam CA_MIRROR             = "OFF";
                                     // C/A mirror opt for DDR3 dual rank

   //-***************************************************************************
   // The following localparams are multiplier and divisor factors for PLLE2.
    // Based on the selected design frequency these localparams vary.
   //-***************************************************************************
   localparam CLKIN_PERIOD          = 10000;
                                     // Input Clock Period
   localparam CLKFBOUT_MULT         = 10;
                                     // write PLL VCO multiplier
   localparam DIVCLK_DIVIDE         = 1;
                                     // write PLL VCO divisor
   localparam CLKOUT0_PHASE         = 337.5;
                                     // Phase for PLL output clock (CLKOUT0)
   localparam CLKOUT0_DIVIDE        = 2;
                                     // VCO output divisor for PLL output clock (CLKOUT0)
   localparam CLKOUT1_DIVIDE        = 2;
                                     // VCO output divisor for PLL output clock (CLKOUT1)
   localparam CLKOUT2_DIVIDE        = 32;
                                     // VCO output divisor for PLL output clock (CLKOUT2)
   localparam CLKOUT3_DIVIDE        = 8;
                                     // VCO output divisor for PLL output clock (CLKOUT3)

   //-***************************************************************************
   // Memory Timing Localparams. These localparams varies based on the selected
   // memory part.
   //-***************************************************************************
   localparam tCKE                  = 5000;
                                     // memory tCKE paramter in pS
   localparam tFAW                  = 30000;
                                     // memory tRAW paramter in pS.
   localparam tRAS                  = 35000;
                                     // memory tRAS paramter in pS.
   localparam tRCD                  = 13750;
                                     // memory tRCD paramter in pS.
   localparam tREFI                 = 7800000;
                                     // memory tREFI paramter in pS.
   localparam tRFC                  = 300000;
                                     // memory tRFC paramter in pS.
   localparam tRP                   = 13750;
                                     // memory tRP paramter in pS.
   localparam tRRD                  = 6000;
                                     // memory tRRD paramter in pS.
   localparam tRTP                  = 7500;
                                     // memory tRTP paramter in pS.
   localparam tWTR                  = 7500;
                                     // memory tWTR paramter in pS.
   localparam tZQI                  = 128_000_000;
                                     // memory tZQI paramter in nS.
   localparam tZQCS                 = 64;
                                     // memory tZQCS paramter in clock cycles.

   //-***************************************************************************
   // Simulation localparams
   //-***************************************************************************
   localparam SIM_BYPASS_INIT_CAL   = "OFF";
                                     // # = "OFF" -  Complete memory init &
                                     //              calibration sequence
                                     // # = "SKIP" - Not supported
                                     // # = "FAST" - Complete memory init & use
                                     //              abbreviated calib sequence

   localparam SIMULATION            = "FALSE";
                                     // Should be TRUE during design simulations and
                                     // FALSE during implementations

   //-***************************************************************************
   // The following localparams varies based on the pin out entered in MIG GUI.
   // Do not change any of these localparams directly by editing the RTL.
   // Any changes required should be done through GUI and the design regenerated.
   //-***************************************************************************
   localparam BYTE_LANES_B0         = 4'b1111;
                                     // Byte lanes used in an IO column.
   localparam BYTE_LANES_B1         = 4'b1110;
                                     // Byte lanes used in an IO column.
   localparam BYTE_LANES_B2         = 4'b0000;
                                     // Byte lanes used in an IO column.
   localparam BYTE_LANES_B3         = 4'b0000;
                                     // Byte lanes used in an IO column.
   localparam BYTE_LANES_B4         = 4'b0000;
                                     // Byte lanes used in an IO column.
   localparam DATA_CTL_B0           = 4'b1111;
                                     // Indicates Byte lane is data byte lane
                                     // or control Byte lane. '1' in a bit
                                     // position indicates a data byte lane and
                                     // a '0' indicates a control byte lane
   localparam DATA_CTL_B1           = 4'b0000;
                                     // Indicates Byte lane is data byte lane
                                     // or control Byte lane. '1' in a bit
                                     // position indicates a data byte lane and
                                     // a '0' indicates a control byte lane
   localparam DATA_CTL_B2           = 4'b0000;
                                     // Indicates Byte lane is data byte lane
                                     // or control Byte lane. '1' in a bit
                                     // position indicates a data byte lane and
                                     // a '0' indicates a control byte lane
   localparam DATA_CTL_B3           = 4'b0000;
                                     // Indicates Byte lane is data byte lane
                                     // or control Byte lane. '1' in a bit
                                     // position indicates a data byte lane and
                                     // a '0' indicates a control byte lane
   localparam DATA_CTL_B4           = 4'b0000;
                                     // Indicates Byte lane is data byte lane
                                     // or control Byte lane. '1' in a bit
                                     // position indicates a data byte lane and
                                     // a '0' indicates a control byte lane
   localparam PHY_0_BITLANES        = 48'h3FE_3FE_3FE_2FF;
   localparam PHY_1_BITLANES        = 48'h3FF_FFF_C00_000;
   localparam PHY_2_BITLANES        = 48'h000_000_000_000;

   // control/address/data pin mapping localparams
   localparam CK_BYTE_MAP
     = 144'h00_00_00_00_00_00_00_00_00_00_00_00_00_00_00_00_00_13;
   localparam ADDR_MAP
     = 192'h000_139_138_137_136_135_134_133_132_131_130_129_128_127_126_12B;
   localparam BANK_MAP   = 36'h12A_125_124;
   localparam CAS_MAP    = 12'h122;
   localparam CKE_ODT_BYTE_MAP = 8'h00;
   localparam CKE_MAP    = 96'h000_000_000_000_000_000_000_11B;
   localparam ODT_MAP    = 96'h000_000_000_000_000_000_000_11A;
   localparam CS_MAP     = 120'h000_000_000_000_000_000_000_000_000_120;
   localparam PARITY_MAP = 12'h000;
   localparam RAS_MAP    = 12'h123;
   localparam WE_MAP     = 12'h121;
   localparam DQS_BYTE_MAP
     = 144'h00_00_00_00_00_00_00_00_00_00_00_00_00_00_00_01_02_03;
   localparam DATA0_MAP  = 96'h031_032_033_034_035_036_037_038;
   localparam DATA1_MAP  = 96'h021_022_023_024_025_026_027_028;
   localparam DATA2_MAP  = 96'h011_012_013_014_015_016_017_018;
   localparam DATA3_MAP  = 96'h000_001_002_003_004_005_006_007;
   localparam DATA4_MAP  = 96'h000_000_000_000_000_000_000_000;
   localparam DATA5_MAP  = 96'h000_000_000_000_000_000_000_000;
   localparam DATA6_MAP  = 96'h000_000_000_000_000_000_000_000;
   localparam DATA7_MAP  = 96'h000_000_000_000_000_000_000_000;
   localparam DATA8_MAP  = 96'h000_000_000_000_000_000_000_000;
   localparam DATA9_MAP  = 96'h000_000_000_000_000_000_000_000;
   localparam DATA10_MAP = 96'h000_000_000_000_000_000_000_000;
   localparam DATA11_MAP = 96'h000_000_000_000_000_000_000_000;
   localparam DATA12_MAP = 96'h000_000_000_000_000_000_000_000;
   localparam DATA13_MAP = 96'h000_000_000_000_000_000_000_000;
   localparam DATA14_MAP = 96'h000_000_000_000_000_000_000_000;
   localparam DATA15_MAP = 96'h000_000_000_000_000_000_000_000;
   localparam DATA16_MAP = 96'h000_000_000_000_000_000_000_000;
   localparam DATA17_MAP = 96'h000_000_000_000_000_000_000_000;
   localparam MASK0_MAP  = 108'h000_000_000_000_000_009_019_029_039;
   localparam MASK1_MAP  = 108'h000_000_000_000_000_000_000_000_000;

   localparam SLOT_0_CONFIG         = 8'b0000_0001;
                                     // Mapping of Ranks.
   localparam SLOT_1_CONFIG         = 8'b0000_0000;
                                     // Mapping of Ranks.
   localparam MEM_ADDR_ORDER
     = "BANK_ROW_COLUMN";

   //-***************************************************************************
   // IODELAY and PHY related localparams
   //-***************************************************************************
   localparam IODELAY_HP_MODE       = "ON";
                                     // to phy_top
   localparam IBUF_LPWR_MODE        = "OFF";
                                     // to phy_top
   localparam DATA_IO_IDLE_PWRDWN   = "ON";
                                     // # = "ON", "OFF"
   localparam BANK_TYPE             = "HP_IO";
                                     // # = "HP_IO", "HPL_IO", "HR_IO", "HRL_IO"
   localparam DATA_IO_PRIM_TYPE     = "HP_LP";
                                     // # = "HP_LP", "HR_LP", "DEFAULT"
   localparam CKE_ODT_AUX           = "FALSE";
   localparam USER_REFRESH          = "OFF";
   localparam WRLVL                 = "ON";
                                     // # = "ON" - DDR3 SDRAM
                                     //   = "OFF" - DDR2 SDRAM.
   localparam ORDERING              = "NORM";
                                     // # = "NORM", "STRICT", "RELAXED".
   localparam CALIB_ROW_ADD         = 16'h0000;
                                     // Calibration row address will be used for
                                     // calibration read and write operations
   localparam CALIB_COL_ADD         = 12'h000;
                                     // Calibration column address will be used for
                                     // calibration read and write operations
   localparam CALIB_BA_ADD          = 3'h0;
                                     // Calibration bank address will be used for
                                     // calibration read and write operations
   localparam TCQ                   = 100;
   localparam IODELAY_GRP           = "IODELAY_MIG";
                                     // It is associated to a set of IODELAYs with
                                     // an IDELAYCTRL that have same IODELAY CONTROLLER
                                     // clock frequency.
   localparam SYSCLK_TYPE           = "SINGLE_ENDED";
                                     // System clock type DIFFERENTIAL, SINGLE_ENDED,
                                     // NO_BUFFER
   localparam REFCLK_TYPE           = "NO_BUFFER";
                                     // Reference clock type DIFFERENTIAL, SINGLE_ENDED,
                                     // NO_BUFFER, USE_SYSTEM_CLOCK

   localparam DRAM_TYPE             = "DDR3";
   localparam CAL_WIDTH             = "HALF";
   localparam STARVE_LIMIT          = 2;
                                     // # = 2,3,4.

   //-***************************************************************************
   // Referece clock frequency localparams
   //-***************************************************************************
   localparam REFCLK_FREQ           = 200.0;
                                     // IODELAYCTRL reference clock frequency
   localparam DIFF_TERM_REFCLK      = "TRUE";
                                     // Differential Termination for idelay
                                     // reference clock input pins
   //-***************************************************************************
   // System clock frequency localparams
   //-***************************************************************************
   localparam tCK                   = 2000;
                                     // memory tCK paramter.
                                     // # = Clock Period in pS.
   localparam nCK_PER_CLK           = 4;
                                     // # of memory CKs per fabric CLK
   localparam DIFF_TERM_SYSCLK      = "TRUE";
                                     // Differential Termination for System
                                     // clock input pins


   //-***************************************************************************
   // AXI4 Shim localparams
   //-***************************************************************************
   localparam C_S_AXI_ID_WIDTH              = 4;
                                             // Width of all master and slave ID signals.
                                             // # = >= 1.
   localparam C_S_AXI_MEM_SIZE              = "2147483648";
                                     // Address Space required for this component
   localparam C_S_AXI_ADDR_WIDTH            = 32;
                                             // Width of S_AXI_AWADDR; S_AXI_ARADDR, M_AXI_AWADDR and
                                             // M_AXI_ARADDR for all SI/MI slots.
                                             // # = 32.
   localparam C_S_AXI_DATA_WIDTH            = 128;
                                             // Width of WDATA and RDATA on SI slot.
                                             // Must be <= APP_DATA_WIDTH.
                                             // # = 32, 64, 128, 256.
   localparam C_MC_nCK_PER_CLK              = 4;
                                             // Indicates whether to instatiate upsizer
                                             // Range: 0, 1
   localparam C_S_AXI_SUPPORTS_NARROW_BURST = 1;
                                             // Indicates whether to instatiate upsizer
                                             // Range: 0, 1
   localparam C_RD_WR_ARB_ALGORITHM          = "ROUND_ROBIN";
                                             // Indicates the Arbitration
                                             // Allowed values - "TDM", "ROUND_ROBIN",
                                             // "RD_PRI_REG", "RD_PRI_REG_STARVE_LIMIT"
                                             // "WRITE_PRIORITY", "WRITE_PRIORITY_REG"
   localparam C_S_AXI_REG_EN0               = 20'h00000;
                                             // C_S_AXI_REG_EN0[00] = Reserved
                                             // C_S_AXI_REG_EN0[04] = AW CHANNEL REGISTER SLICE
                                             // C_S_AXI_REG_EN0[05] =  W CHANNEL REGISTER SLICE
                                             // C_S_AXI_REG_EN0[06] =  B CHANNEL REGISTER SLICE
                                             // C_S_AXI_REG_EN0[07] =  R CHANNEL REGISTER SLICE
                                             // C_S_AXI_REG_EN0[08] = AW CHANNEL UPSIZER REGISTER SLICE
                                             // C_S_AXI_REG_EN0[09] =  W CHANNEL UPSIZER REGISTER SLICE
                                             // C_S_AXI_REG_EN0[10] = AR CHANNEL UPSIZER REGISTER SLICE
                                             // C_S_AXI_REG_EN0[11] =  R CHANNEL UPSIZER REGISTER SLICE
   localparam C_S_AXI_REG_EN1               = 20'h00000;
                                             // Instatiates register slices after the upsizer.
                                             // The type of register is specified for each channel
                                             // in a vector. 4 bits per channel are used.
                                             // C_S_AXI_REG_EN1[03:00] = AW CHANNEL REGISTER SLICE
                                             // C_S_AXI_REG_EN1[07:04] =  W CHANNEL REGISTER SLICE
                                             // C_S_AXI_REG_EN1[11:08] =  B CHANNEL REGISTER SLICE
                                             // C_S_AXI_REG_EN1[15:12] = AR CHANNEL REGISTER SLICE
                                             // C_S_AXI_REG_EN1[20:16] =  R CHANNEL REGISTER SLICE
                                             // Possible values for each channel are:
                                             //
                                             //   0 => BYPASS    = The channel is just wired through the
                                             //                    module.
                                             //   1 => FWD       = The master VALID and payload signals
                                             //                    are registrated.
                                             //   2 => REV       = The slave ready signal is registrated
                                             //   3 => FWD_REV   = Both FWD and REV
                                             //   4 => SLAVE_FWD = All slave side signals and master
                                             //                    VALID and payload are registrated.
                                             //   5 => SLAVE_RDY = All slave side signals and master
                                             //                    READY are registrated.
                                             //   6 => INPUTS    = Slave and Master side inputs are
                                             //                    registrated.
                                             //   7 => ADDRESS   = Optimized for address channel
   localparam C_S_AXI_CTRL_ADDR_WIDTH       = 32;
                                             // Width of AXI-4-Lite address bus
   localparam C_S_AXI_CTRL_DATA_WIDTH       = 32;
                                             // Width of AXI-4-Lite data buses
   localparam C_S_AXI_BASEADDR              = 32'h0000_0000;
                                             // Base address of AXI4 Memory Mapped bus.
   localparam C_ECC_ONOFF_RESET_VALUE       = 1;
                                             // Controls ECC on/off value at startup/reset
   localparam C_ECC_CE_COUNTER_WIDTH        = 8;
                                             // The external memory to controller clock ratio.

   //-***************************************************************************
   // Debug localparams
   //-***************************************************************************
   localparam DEBUG_PORT            = "OFF";
                                     // # = "ON" Enable debug signals/controls.
                                     //   = "OFF" Disable debug signals/controls.

   //-***************************************************************************
   // Temparature monitor localparam
   //-***************************************************************************
   localparam TEMP_MON_CONTROL                          = "INTERNAL";
                                     // # = "INTERNAL", "EXTERNAL"

   localparam RST_ACT_LOW           = 1;
                                     // =1 for active low reset,
                                     // =0 for active high.
   //-*******************

  localparam CMD_PIPE_PLUS1        = "ON";
                                     // add pipeline stage between MC and PHY
  localparam DATA_WIDTH            = 32;
  localparam ECC_WIDTH = (ECC == "OFF")?
                           0 : (DATA_WIDTH <= 4)?
                            4 : (DATA_WIDTH <= 10)?
                             5 : (DATA_WIDTH <= 26)?
                              6 : (DATA_WIDTH <= 57)?
                               7 : (DATA_WIDTH <= 120)?
                                8 : (DATA_WIDTH <= 247)?
                                 9 : 10;
  localparam ECC_TEST              = "OFF";
  localparam RANK_WIDTH = clogb2(RANKS);
  localparam DATA_BUF_OFFSET_WIDTH = 1;
  localparam MC_ERR_ADDR_WIDTH = ((CS_WIDTH == 1) ? 0 : RANK_WIDTH)
                                 + BANK_WIDTH + ROW_WIDTH + COL_WIDTH
                                 + DATA_BUF_OFFSET_WIDTH;
  localparam tPRDI                 = 1_000_000;
                                     // memory tPRDI paramter in pS.
  localparam PAYLOAD_WIDTH         = (ECC_TEST == "OFF") ? DATA_WIDTH : DQ_WIDTH;
  //localparam BURST_LENGTH          = STR_TO_INT(BURST_MODE);
  localparam APP_DATA_WIDTH        = 2 * nCK_PER_CLK * PAYLOAD_WIDTH;
  localparam APP_MASK_WIDTH        = APP_DATA_WIDTH / 8;
 -----/\----- EXCLUDED -----/\----- */


   ddr3_32bit
     /* -----\/----- EXCLUDED -----\/-----
      #(
      .TCQ                              (TCQ),
     .ADDR_CMD_MODE                    (ADDR_CMD_MODE),
     .AL                               (AL),
     .PAYLOAD_WIDTH                    (PAYLOAD_WIDTH),
     .BANK_WIDTH                       (BANK_WIDTH),
     .BURST_MODE                       (BURST_MODE),
     .BURST_TYPE                       (BURST_TYPE),
     .CA_MIRROR                        (CA_MIRROR),
     .CK_WIDTH                         (CK_WIDTH),
     .COL_WIDTH                        (COL_WIDTH),
     .CMD_PIPE_PLUS1                   (CMD_PIPE_PLUS1),
     .CS_WIDTH                         (CS_WIDTH),
     .nCS_PER_RANK                     (nCS_PER_RANK),
     .CKE_WIDTH                        (CKE_WIDTH),
     .DATA_WIDTH                       (DATA_WIDTH),
     .DATA_BUF_ADDR_WIDTH              (DATA_BUF_ADDR_WIDTH),
     .DQ_CNT_WIDTH                     (DQ_CNT_WIDTH),
     .DQ_PER_DM                        (DQ_PER_DM),
     .DQ_WIDTH                         (DQ_WIDTH),
     .DQS_CNT_WIDTH                    (DQS_CNT_WIDTH),
     .DQS_WIDTH                        (DQS_WIDTH),
     .DRAM_WIDTH                       (DRAM_WIDTH),
     .ECC                              (ECC),
     .ECC_WIDTH                        (ECC_WIDTH),
     .ECC_TEST                         (ECC_TEST),
     .MC_ERR_ADDR_WIDTH                (MC_ERR_ADDR_WIDTH),
     .nAL                              (nAL),
     .nBANK_MACHS                      (nBANK_MACHS),
     .CKE_ODT_AUX                      (CKE_ODT_AUX),
     .ORDERING                         (ORDERING),
     .OUTPUT_DRV                       (OUTPUT_DRV),
     .IBUF_LPWR_MODE                   (IBUF_LPWR_MODE),
     .IODELAY_HP_MODE                  (IODELAY_HP_MODE),
     .DATA_IO_IDLE_PWRDWN              (DATA_IO_IDLE_PWRDWN),
     .BANK_TYPE                        (BANK_TYPE),
     .DATA_IO_PRIM_TYPE                (DATA_IO_PRIM_TYPE),
     .REG_CTRL                         (REG_CTRL),
     .RTT_NOM                          (RTT_NOM),
     .RTT_WR                           (RTT_WR),
     .CL                               (CL),
     .CWL                              (CWL),
     .tCKE                             (tCKE),
     .tFAW                             (tFAW),
     .tPRDI                            (tPRDI),
     .tRAS                             (tRAS),
     .tRCD                             (tRCD),
     .tREFI                            (tREFI),
     .tRFC                             (tRFC),
     .tRP                              (tRP),
     .tRRD                             (tRRD),
     .tRTP                             (tRTP),
     .tWTR                             (tWTR),
     .tZQI                             (tZQI),
     .tZQCS                            (tZQCS),
     .USER_REFRESH                     (USER_REFRESH),
     .WRLVL                            (WRLVL),
     .DEBUG_PORT                       (DEBUG_PORT),
     .RANKS                            (RANKS),
     .ODT_WIDTH                        (ODT_WIDTH),
     .ROW_WIDTH                        (ROW_WIDTH),
     .ADDR_WIDTH                       (ADDR_WIDTH),
     .SIM_BYPASS_INIT_CAL              (SIM_BYPASS_INIT_CAL),
     .SIMULATION                       (SIMULATION),
     .BYTE_LANES_B0                    (BYTE_LANES_B0),
     .BYTE_LANES_B1                    (BYTE_LANES_B1),
     .BYTE_LANES_B2                    (BYTE_LANES_B2),
     .BYTE_LANES_B3                    (BYTE_LANES_B3),
     .BYTE_LANES_B4                    (BYTE_LANES_B4),
     .DATA_CTL_B0                      (DATA_CTL_B0),
     .DATA_CTL_B1                      (DATA_CTL_B1),
     .DATA_CTL_B2                      (DATA_CTL_B2),
     .DATA_CTL_B3                      (DATA_CTL_B3),
     .DATA_CTL_B4                      (DATA_CTL_B4),
     .PHY_0_BITLANES                   (PHY_0_BITLANES),
     .PHY_1_BITLANES                   (PHY_1_BITLANES),
     .PHY_2_BITLANES                   (PHY_2_BITLANES),
     .CK_BYTE_MAP                      (CK_BYTE_MAP),
     .ADDR_MAP                         (ADDR_MAP),
     .BANK_MAP                         (BANK_MAP),
     .CAS_MAP                          (CAS_MAP),
     .CKE_ODT_BYTE_MAP                 (CKE_ODT_BYTE_MAP),
     .CKE_MAP                          (CKE_MAP),
     .ODT_MAP                          (ODT_MAP),
     .CS_MAP                           (CS_MAP),
     .PARITY_MAP                       (PARITY_MAP),
     .RAS_MAP                          (RAS_MAP),
     .WE_MAP                           (WE_MAP),
     .DQS_BYTE_MAP                     (DQS_BYTE_MAP),
     .DATA0_MAP                        (DATA0_MAP),
     .DATA1_MAP                        (DATA1_MAP),
     .DATA2_MAP                        (DATA2_MAP),
     .DATA3_MAP                        (DATA3_MAP),
     .DATA4_MAP                        (DATA4_MAP),
     .DATA5_MAP                        (DATA5_MAP),
     .DATA6_MAP                        (DATA6_MAP),
     .DATA7_MAP                        (DATA7_MAP),
     .DATA8_MAP                        (DATA8_MAP),
     .DATA9_MAP                        (DATA9_MAP),
     .DATA10_MAP                       (DATA10_MAP),
     .DATA11_MAP                       (DATA11_MAP),
     .DATA12_MAP                       (DATA12_MAP),
     .DATA13_MAP                       (DATA13_MAP),
     .DATA14_MAP                       (DATA14_MAP),
     .DATA15_MAP                       (DATA15_MAP),
     .DATA16_MAP                       (DATA16_MAP),
     .DATA17_MAP                       (DATA17_MAP),
     .MASK0_MAP                        (MASK0_MAP),
     .MASK1_MAP                        (MASK1_MAP),
     .CALIB_ROW_ADD                    (CALIB_ROW_ADD),
     .CALIB_COL_ADD                    (CALIB_COL_ADD),
     .CALIB_BA_ADD                     (CALIB_BA_ADD),
     .SLOT_0_CONFIG                    (SLOT_0_CONFIG),
     .SLOT_1_CONFIG                    (SLOT_1_CONFIG),
     .MEM_ADDR_ORDER                   (MEM_ADDR_ORDER),
     .USE_CS_PORT                      (USE_CS_PORT),
     .USE_DM_PORT                      (USE_DM_PORT),
     .USE_ODT_PORT                     (USE_ODT_PORT),
     .PHY_CONTROL_MASTER_BANK          (PHY_CONTROL_MASTER_BANK),
     .TEMP_MON_CONTROL                 (TEMP_MON_CONTROL),


     .DM_WIDTH                         (DM_WIDTH),

     .nCK_PER_CLK                      (nCK_PER_CLK),
     .tCK                              (tCK),
     .DIFF_TERM_SYSCLK                 (DIFF_TERM_SYSCLK),
     .CLKIN_PERIOD                     (CLKIN_PERIOD),
     .CLKFBOUT_MULT                    (CLKFBOUT_MULT),
     .DIVCLK_DIVIDE                    (DIVCLK_DIVIDE),
     .CLKOUT0_PHASE                    (CLKOUT0_PHASE),
     .CLKOUT0_DIVIDE                   (CLKOUT0_DIVIDE),
     .CLKOUT1_DIVIDE                   (CLKOUT1_DIVIDE),
     .CLKOUT2_DIVIDE                   (CLKOUT2_DIVIDE),
     .CLKOUT3_DIVIDE                   (CLKOUT3_DIVIDE),
     .C_S_AXI_ID_WIDTH                 (C_S_AXI_ID_WIDTH),
     .C_S_AXI_ADDR_WIDTH               (C_S_AXI_ADDR_WIDTH),
     .C_S_AXI_DATA_WIDTH               (C_S_AXI_DATA_WIDTH),
     .C_MC_nCK_PER_CLK                 (C_MC_nCK_PER_CLK),
     .C_S_AXI_SUPPORTS_NARROW_BURST    (C_S_AXI_SUPPORTS_NARROW_BURST),
     .C_RD_WR_ARB_ALGORITHM            (C_RD_WR_ARB_ALGORITHM),
     .C_S_AXI_REG_EN0                  (C_S_AXI_REG_EN0),
     .C_S_AXI_REG_EN1                  (C_S_AXI_REG_EN1),
     .C_S_AXI_CTRL_ADDR_WIDTH          (C_S_AXI_CTRL_ADDR_WIDTH),
     .C_S_AXI_CTRL_DATA_WIDTH          (C_S_AXI_CTRL_DATA_WIDTH),
     .C_S_AXI_BASEADDR                 (C_S_AXI_BASEADDR),
     .C_ECC_ONOFF_RESET_VALUE          (C_ECC_ONOFF_RESET_VALUE),
     .C_ECC_CE_COUNTER_WIDTH           (C_ECC_CE_COUNTER_WIDTH),


     .SYSCLK_TYPE                      (SYSCLK_TYPE),
     .REFCLK_TYPE                      (REFCLK_TYPE),
     .REFCLK_FREQ                      (REFCLK_FREQ),
     .DIFF_TERM_REFCLK                 (DIFF_TERM_REFCLK),
     .IODELAY_GRP                      (IODELAY_GRP),

     .CAL_WIDTH                        (CAL_WIDTH),
     .STARVE_LIMIT                     (STARVE_LIMIT),
     .DRAM_TYPE                        (DRAM_TYPE),


     .RST_ACT_LOW                      (RST_ACT_LOW)
     )
 -----/\----- EXCLUDED -----/\----- */
     u_ddr3_32bit
       (
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
    .init_calib_complete            (ddr3_running),

    .ddr3_cs_n                      (ddr3_cs_n),
    .ddr3_dm                        (ddr3_dm),
    .ddr3_odt                       (ddr3_odt),
    // Application interface ports
    .ui_clk                         (ddr3_axi_clk),  // 250MHz clock out
    .ui_clk_sync_rst                (ddr3_axi_rst),  // Active high Reset signal synchronised to 250MHz
    .aresetn                        (ddr3_axi_rst_reg_n),
    .app_sr_req                     (1'b0),
    .app_sr_active                  (app_sr_active),
    .app_ref_req                    (1'b0),
    .app_ref_ack                    (app_ref_ack),
    .app_zq_req                     (1'b0),
    .app_zq_ack                     (app_zq_ack),

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
    .sys_rst                        (~global_rst) // IJB. Poorly named active low. Should change RST_ACT_LOW.
    );

`endif //  `ifndef NO_DRAM_FIFOS

   ///////////////////////////////////////////////////////////////////////////////////
   //
   // X300 Core
   //
   ///////////////////////////////////////////////////////////////////////////////////
   x300_core x300_core
     (
      .radio_clk(radio_clk), .radio_rst(radio_rst),
      .bus_clk(bus_clk), .bus_rst(bus_rst), .sw_rst(sw_rst),
`ifdef DEBUG_UART
      .fp_gpio(FrontPanelGpio[9:0]), // Discard upper unsued bits.
`else
      .fp_gpio(FrontPanelGpio[11:0]), // Discard upper unsued bits.
      `endif
      // Radio0 signals
      .rx0(rx0), .tx0(tx0), .db_gpio0({DB0_TX_IO,DB0_RX_IO}),
      .sen0(sen0), .sclk0(sclk0), .mosi0(mosi0), .miso0(miso0),
      .radio_led0(led0), .radio_misc0({DB_ADC_RESET, DB_DAC_RESET,DB0_DAC_ENABLE}),
      .sync_dacs_radio0(sync_dacs_radio0),
      // Radio1 signals
      .rx1(rx1), .tx1(tx1), .db_gpio1({DB1_TX_IO,DB1_RX_IO}),
      .sen1(sen1), .sclk1(sclk1), .mosi1(mosi1), .miso1(miso1),
      .radio_led1(led1), .radio_misc1({DB1_DAC_ENABLE}),
      .sync_dacs_radio1(sync_dacs_radio1),
      // I2C bus
      .db_scl(DB_SCL), .db_sda(DB_SDA),
      // External clock gen
      .ext_ref_clk(ref_clk_10mhz),
      .clock_ref_sel(ClockRefSelect),
      .clock_misc_opt({GPSDO_PWR_ENA, TCXO_ENA}),
      .LMK_Status(LMK_Status[1:0]), .LMK_Holdover(LMK_Holdover), .LMK_Lock(LMK_Lock), .LMK_Sync(LMK_Sync),
      .LMK_SEN(LMK_SEN), .LMK_SCLK(LMK_SCLK), .LMK_MOSI(LMK_MOSI),
      // SFP+ 0 flags
      .SFPP0_SCL(SFPP0_SCL),
      .SFPP0_SDA(SFPP0_SDA),
      .SFPP0_ModAbs(SFPP0_ModAbs),
      .SFPP0_TxFault(SFPP0_TxFault),
      .SFPP0_RxLOS(SFPP0_RxLOS),
      .SFPP0_RS1(SFPP0_RS1),
      .SFPP0_RS0(SFPP0_RS0),
      // SFP+ 1 flags
      .SFPP1_SCL(SFPP1_SCL),
      .SFPP1_SDA(SFPP1_SDA),
      .SFPP1_ModAbs(SFPP1_ModAbs),
      .SFPP1_TxFault(SFPP1_TxFault),
      .SFPP1_RxLOS(SFPP1_RxLOS),
      .SFPP1_RS1(SFPP1_RS1),
      .SFPP1_RS0(SFPP1_RS0),
      // MDIO bus to SFP0
      .mdc0(mdc0),
      .mdio_in0(mdio_in0),
      .mdio_out0(mdio_out0),
      // SFP+ 0 packet interface
`ifdef ETH10G_PORT0
      .xgmii_clk0(xgmii_clk0),
      .xgmii_txd0(xgmii_txd0),
      .xgmii_txc0(xgmii_txc0),
      .xgmii_rxd0(xgmii_rxd0),
      .xgmii_rxc0(xgmii_rxc0),
      .xge_phy_resetdone0(xge_phy_resetdone0),
`else
      .gmii_clk0(gmii_clk0),
      .gmii_txd0(gmii_txd0), .gmii_tx_en0(gmii_tx_en0), .gmii_tx_er0(gmii_tx_er0),
      .gmii_rxd0(gmii_rxd0), .gmii_rx_dv0(gmii_rx_dv0), .gmii_rx_er0(gmii_rx_er0),
`endif // !`ifdef
      // MDIO bus to SFP1
      .mdc1(mdc1),
      .mdio_in1(mdio_in1),
      .mdio_out1(mdio_out1),
      // SFP+ 1 packet interface
`ifdef ETH10G_PORT1
      .xgmii_clk1(xgmii_clk1),
      .xgmii_txd1(xgmii_txd1),
      .xgmii_txc1(xgmii_txc1),
      .xgmii_rxd1(xgmii_rxd1),
      .xgmii_rxc1(xgmii_rxc1),
      .xge_phy_resetdone1(xge_phy_resetdone1),
`else
      .gmii_clk1(gmii_clk1),
      .gmii_txd1(gmii_txd1), .gmii_tx_en1(gmii_tx_en1), .gmii_tx_er1(gmii_tx_er1),
      .gmii_rxd1(gmii_rxd1), .gmii_rx_dv1(gmii_rx_dv1), .gmii_rx_er1(gmii_rx_er1),
`endif // !`ifdef
      // Time
      .pps(pps),.pps_select(pps_select), .pps_out_enb(pps_out_enb),
      // GPS Signals
      .gps_txd(GPS_SER_IN), .gps_rxd(GPS_SER_OUT),
      // Debug UART
      .debug_rxd(debug_rxd), .debug_txd(debug_txd),
      // Misc.
      .led_misc(leds),
      .debug0(), .debug1(), .debug2(),
`ifndef NO_DRAM_FIFOS
      // DRAM signals.
      .ddr3_axi_clk              (ddr3_axi_clk),
      .ddr3_axi_rst              (ddr3_axi_rst),
      .ddr3_running              (ddr3_running),
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
`endif //  `ifndef NO_DRAM_FIFOS

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
      .pcio_tlast                (pcio_tlast),
      .pcio_tvalid               (pcio_tvalid),
      .pcio_tready               (pcio_tready),
      .pcii_tdata                (pcii_tdata),
      .pcii_tlast                (pcii_tlast),
      .pcii_tvalid               (pcii_tvalid),
      .pcii_tready               (pcii_tready)
      );


endmodule // x300

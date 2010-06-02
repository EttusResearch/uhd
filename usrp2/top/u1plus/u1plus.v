
module u1plus
  (input CLK_FPGA_P, input CLK_FPGA_N, // Main Clock
   output FPGA_TXD, input FPGA_RXD,   // UART
   inout SDA_FPGA, inout SCL_FPGA,   // I2C

   // CGEN
   input cgen_st_ld,
   input cgen_st_refmon,
   input cgen_st_status,
   input cgen_ref_sel,
   input cgen_sync_b,
   
   // FPGA Config
   input fpga_cfg_din,
   input fpga_cfg_cclk,
   input fpga_cfg_init_b,
   
   // MISC
   input [2:0] mystery_bus,
   input reset_n,
   input PPS_IN,
   output reset_codec,
   
   // GPIF
   inout [15:0] GPIF_D,   
   input [3:0] GPIF_CTL,   
   output [3:0] GPIF_RDY,
   input FX2_PA7_FLAGD,
   input FX2_PA6_PKTEND,
   input FX2_PA2_SLOE,
   input IFCLK,
   
   output [2:0] debug_led,

   // Debug bus
   output [1:0] debug_clk,
   output [31:0] debug,
   
   input [11:0] adc,
   input RXSYNC,
   
   output TXBLANK,
   output TXSYNC,
   output [13:0] dac,

   // TX DB
   inout [15:0] io_tx,
   inout [15:0] io_rx,

   // SPI
   output SEN_AUX, output SCLK_AUX, input MISO_AUX,
   output SEN_CODEC, output SCLK_CODEC, output MOSI_CODEC, input MISO_CODEC,
   output SEN_RX_DB, output SCLK_RX_DB, output MOSI_RX_DB, input MISO_RX_DB,
   output SEN_TX_DB, output SCLK_TX_DB, output MOSI_TX_DB, input MISO_TX_DB
   );

   wire   clk_fpga, sys_clk, wb_clk, dcm_out, clk_div, dcm_locked;
   
   IBUFGDS clk_fpga_pin (.O(clk_fpga),.I(CLK_FPGA_P),.IB(CLK_FPGA_N));
   defparam 	clk_fpga_pin.IOSTANDARD = "LVPECL_25";
   
   DCM DCM_INST (.CLKFB(sys_clk), 
                 .CLKIN(clk_fpga), 
                 .DSSEN(0), 
                 .PSCLK(0), 
                 .PSEN(0), 
                 .PSINCDEC(0), 
                 .RST(dcm_rst), 
                 .CLKDV(clk_div), 
                 .CLKFX(), 
                 .CLKFX180(), 
                 .CLK0(dcm_out), 
                 .CLK2X(), 
                 .CLK2X180(), 
                 .CLK90(), 
                 .CLK180(), 
                 .CLK270(), 
                 .LOCKED(dcm_locked), 
                 .PSDONE(), 
                 .STATUS());
   defparam DCM_INST.CLK_FEEDBACK = "1X";
   defparam DCM_INST.CLKDV_DIVIDE = 2.0;
   defparam DCM_INST.CLKFX_DIVIDE = 1;
   defparam DCM_INST.CLKFX_MULTIPLY = 4;
   defparam DCM_INST.CLKIN_DIVIDE_BY_2 = "FALSE";
   defparam DCM_INST.CLKIN_PERIOD = 15.625;
   defparam DCM_INST.CLKOUT_PHASE_SHIFT = "NONE";
   defparam DCM_INST.DESKEW_ADJUST = "SYSTEM_SYNCHRONOUS";
   defparam DCM_INST.DFS_FREQUENCY_MODE = "LOW";
   defparam DCM_INST.DLL_FREQUENCY_MODE = "LOW";
   defparam DCM_INST.DUTY_CYCLE_CORRECTION = "TRUE";
   defparam DCM_INST.FACTORY_JF = 16'h8080;
   defparam DCM_INST.PHASE_SHIFT = 0;
   defparam DCM_INST.STARTUP_WAIT = "FALSE";

   BUFG sysclk_BUFG (.I(dcm_out), .O(sys_clk));
   BUFG wbclk_BUFG (.I(clk_div), .O(wb_clk));

   IOBUF scl_pin(.O(scl_pad_i), .IO(SCL), .I(scl_pad_o), .T(scl_pad_oen_o));
   IOBUF sda_pin(.O(sda_pad_i), .IO(SDA), .I(sda_pad_o), .T(sda_pad_oen_o));

   wire   mosi, miso, sclk;
   assign SCLK_AUX = ~SEN_AUX ? sclk : 2'b00;
   assign {SCLK_CODEC,MOSI_CODEC} = ~SEN_CODEC ? {sclk,mosi} : 2'b00;
   assign {SCLK_TX_DB,MOSI_TX_DB} = ~SEN_TX_DB ? {sclk,mosi} : 2'b00;
   assign {SCLK_RX_DB,MOSI_RX_DB} = ~SEN_RX_DB ? {sclk,mosi} : 2'b00;
   assign miso = (~SEN_CODEC & MISO_CODEC) | (~SEN_AUX & MISO_AUX) |
		 (~SEN_RX_DB & MISO_RX_DB) |(~SEN_TX_DB & MISO_TX_DB);
   
   u1_core u1_core
     (.sys_clk(sys_clk), .sys_rst(sys_rst),
      .wb_clk(wb_clk), .wb_rst(wb_rst),
      .uart_tx_o(FPGA_TXD), .uart_rx_i(FPGA_RXD), .uart_baud_o(),
      
      .leds(debug_led), .debug(debug), .debug_clk(debug_clk),
      
      .scl_pad_i(scl_pad_i), .scl_pad_o(scl_pad_o), .scl_pad_oen_o(scl_pad_oen_o),
      .sda_pad_i(sda_pad_i), .sda_pad_o(sda_pad_o), .sda_pad_oen_o(sda_pad_oen_o),
      
      .pps(PPS_IN),
      .reset_codec(reset_codec),
      
      // GPIF
      .gpif_clk(IFCLK), .gpif_d(GPIF_D), .gpif_ctl(GPIF_CTL), .gpif_rdy(GPIF_RDY),
      .gpif_misc({FX2_PA7_FLAGD, FX2_PA6_PKTEND, FX2_PA2_SLOE}),
      
      .adc(adc), .rxsync(RXSYNC),
      
      .txblank(TXBLANK), .txsync(TXSYNC), .dac(dac),
      
      .io_tx(io_tx), .io_rx(io_rx),
      
      // SPI
      .sclk(sclk), .mosi(mosi), .miso(miso), .sen({SEN_AUX, SEN_CODEC, SEN_RX_DB, SEN_TX_DB}),
      .sim_mode(0)
      );

endmodule // u1plus



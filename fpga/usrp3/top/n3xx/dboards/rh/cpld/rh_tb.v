///////////////////////////////////////////////////////////////////
//
// Copyright 2018 Ettus Research, A National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: rh_tb
//   Simple testbench for rhodium_top
//   This creates a rudimentary stimulus only, to allow results to be viewed
//   in the waveform viewer
//////////////////////////////////////////////////////////////////////

`timescale 1ns/1ps


module rh_tb;

reg ADC_A_Over_Range_18, ADC_B_Over_Range_18;

wire [13:0] usrpio_io; // TODO: use one of these as pl_spi_addr[3]

wire CPLD_PS_SPI_LE_25, CPLD_PS_SPI_CLK_25,
      CPLD_PS_ADDR0_25, CPLD_PS_ADDR1_25, CPLD_PS_SPI_SDI_25;
wire CPLD_PS_SPI_SDO_25;

wire CPLD_PL_SPI_SDO_18;
wire CPLD_PL_SPI_LE_18, CPLD_PL_SPI_SCLK_18, 
      CPLD_PL_SPI_SDI_18,
      CPLD_PL_SPI_ADDR0_18, CPLD_PL_SPI_ADDR1_18, 
		CPLD_PL_SPI_ADDR2_18,
      CPLD_ATR_TX_18, CPLD_ATR_RX_18; 
// NOTE: TxRx front-end switches are driven direct from the motherboard, so these ATR 
//       lines have no function at this time.
		
wire ADC_SPI_CS_L_18, ADC_SPI_SCLK_18;
wire ADC_SPI_SDIO_18;

wire DAC_SPI_CS_L_18, DAC_SPI_SCLK_18;
wire DAC_SPI_SDIO_18;
reg DAC_Alarm_18; // TODO: drive to gpio?

wire PHDAC_SPI_CS_L, PHDAC_SPI_SCLK, PHDAC_SPI_SDI;


reg LO_SYNC;

wire CLKDIST_SPI_CS_L,
       CLKDIST_SPI_SCLK;
wire CLKDIST_SPI_SDIO;

wire Tx_DSA_C1,
       Tx_DSA_C2,
       Tx_DSA_C4,
       Tx_DSA_C8,
       Tx_DSA_C16;
 wire       Tx_DSA1_LE,
       Tx_DSA2_LE;
 wire      Tx_Sw1_Ctrl_1,
       Tx_Sw1_Ctrl_2,
       Tx_Sw2_Ctrl_1,
       Tx_Sw2_Ctrl_2,
       Tx_Sw3_Ctrl_1,
       Tx_Sw3_Ctrl_2,
       Tx_Sw3_Ctrl_3,
       Tx_Sw3_Ctrl_4,
       Rx_LO_Input_Select,
       Rx_LO_Filter_Sw_1,
       Rx_LO_Filter_Sw_2,
       Tx_LO_Input_Select,
       Tx_LO_Filter_Sw_1,
       Tx_LO_Filter_Sw_2;
wire  CLKDIST_Status_LD1,
      CLKDIST_Status_LD2;
wire  LOSYNTH_RX_MUXOUT,
      LOSYNTH_TX_MUXOUT;
		
wire LO_SPI_SCLK,
       LO_SPI_SDI,
       LO_TX_CS_L,
       LO_RX_CS_L,
       Rx_Sw1_Ctrl_1,
       Rx_Sw1_Ctrl_2,
       Rx_DSA_C1,
       Rx_DSA_C2,
       Rx_DSA_C4,
       Rx_DSA_C8,
       Rx_DSA_C16;
 wire       Rx_DSA1_LE,
       Rx_DSA2_LE;
 wire      Rx_Sw2_Ctrl,
       Rx_Sw3_Ctrl_1,
       Rx_Sw3_Ctrl_2,
       Rx_Sw4_Ctrl_1,
       Rx_Sw4_Ctrl_2,
       Rx_Sw4_Ctrl_3,
       Rx_Sw4_Ctrl_4,
       Rx_Demod_ADJ_1,
       Rx_Demod_ADJ_2;
wire LO_DSA_C1,
       LO_DSA_C2,
       LO_DSA_C4,
       LO_DSA_C8,
       LO_DSA_C16;
wire       RxLO_DSA_LE,
       TxLO_DSA_LE;
wire       LODIST_Bd_SPI_CS_L,
       LODIST_Bd_SPI_SDI,
       LODIST_Bd_SPI_SCLK,
       Tx_Sw5_Ctrl_1,
       Tx_Sw5_Ctrl_2,
       Rx_Sw6_Ctrl_1,
       Rx_Sw6_Ctrl_2;
wire LODIST_Bd_IO1;
wire Tx_HB_LB_Select,
Rx_HB_LB_Select,
Cal_iso_Sw_Ctrl;


parameter dly = 20;

integer scnt;
integer acnt;
integer ccnt;
integer ccnt_max;

reg ps_sck;
reg ps_mosi;
reg clkdis_cs_b;
reg cpld_ps_cs_b;
reg phdac_cs_b;
reg adc_cs_b;
reg dac_cs_b;

reg pl_sck;
reg pl_mosi;
reg txlo_cs_b;
reg rxlo_cs_b;
reg lodis_cs_b;
reg cpld_pl_cs_b;

task ps_cpld_xfer;
  input [1:0]  tbl;
  input [5:0]  cmd;
  input [15:0] data;
  reg [23:0] shiftreg;
  integer i;
begin
  ps_sck       <= 1'b0;
  clkdis_cs_b  <= 1'b1;
  cpld_ps_cs_b <= 1'b1;
  phdac_cs_b   <= 1'b1;
  adc_cs_b     <= 1'b1;
  dac_cs_b     <= 1'b1;
  txlo_cs_b    <= 1'b1;
  rxlo_cs_b    <= 1'b1;
  lodis_cs_b   <= 1'b1;
  cpld_pl_cs_b <= 1'b1;
  shiftreg     <= {tbl,cmd,data};
  #(dly);
  cpld_ps_cs_b <= 1'b0;
  #(dly);
  for (i = 0; i < 24; i = i + 1) begin
    ps_sck  <= 1'b0;
    ps_mosi <= shiftreg[23-i];
    #(dly);
    ps_sck  <= 1'b1;
    #(dly);
  end
  ps_sck <= 1'b0;
  #(dly);
  cpld_ps_cs_b <= 1'b1;
  #(dly);
end
endtask

task pl_cpld_xfer;
  input [1:0]  tbl;
  input [5:0]  cmd;
  input [15:0] data;
  reg [23:0] shiftreg;
  integer i;
begin
  pl_sck       <= 1'b0;
  clkdis_cs_b  <= 1'b1;
  cpld_ps_cs_b <= 1'b1;
  phdac_cs_b   <= 1'b1;
  adc_cs_b     <= 1'b1;
  dac_cs_b     <= 1'b1;
  txlo_cs_b    <= 1'b1;
  rxlo_cs_b    <= 1'b1;
  lodis_cs_b   <= 1'b1;
  cpld_pl_cs_b <= 1'b1;
  shiftreg     <= {tbl,cmd,data};
  #(dly);
  cpld_pl_cs_b <= 1'b0;
  #(dly);
  for (i = 0; i < 24; i = i + 1) begin
    pl_sck  <= 1'b0;
    pl_mosi <= shiftreg[23-i];
    #(dly);
    pl_sck  <= 1'b1;
    #(dly);
  end
  pl_sck <= 1'b0;
  #(dly);
  cpld_pl_cs_b <= 1'b1;
  #(dly);
end
endtask

assign CPLD_PS_SPI_LE_25 = clkdis_cs_b;
assign CPLD_PS_ADDR0_25  = cpld_ps_cs_b;
assign CPLD_PS_ADDR1_25  = phdac_cs_b;
assign usrpio_io[12] = adc_cs_b;
assign usrpio_io[13] = dac_cs_b;
assign CPLD_PS_SPI_CLK_25 = ps_sck;
assign CPLD_PS_SPI_SDI_25 = ps_mosi;

assign CPLD_PL_SPI_LE_18    = txlo_cs_b;
assign CPLD_PL_SPI_ADDR1_18 = rxlo_cs_b;
assign CPLD_PL_SPI_ADDR2_18 = lodis_cs_b;
assign CPLD_PL_SPI_ADDR0_18 = cpld_pl_cs_b;
assign CPLD_PL_SPI_SCLK_18  = pl_sck;
assign CPLD_PL_SPI_SDI_18   = pl_mosi;

assign CLKDIST_Status_LD1 = 1'b0;
assign LOSYNTH_RX_MUXOUT  = 1'b1;
assign LOSYNTH_TX_MUXOUT  = 1'b1; 

initial
begin
  $dumpfile("rh_cpld.vcd");
  $dumpvars;
  // Check Signature register read-back
  #(dly) ps_cpld_xfer(2'b00, {5'b00000, 1'b1}, 16'h0000);
  // Check Signature register is read-only
  #(dly) ps_cpld_xfer(2'b00, {5'b00000, 1'b0}, 16'h1234);
  #(dly) ps_cpld_xfer(2'b00, {5'b00000, 1'b1}, 16'h0000);

  // Load portions of lower RX gain table with some values
  #(dly) ps_cpld_xfer(2'b00, {5'b00110, 1'b0}, 16'h0000); /* Write GAIN_BAND_SEL for lower table */
  #(dly) ps_cpld_xfer(2'b01, 6'd0, {2'd0, 5'd0, 5'd1, 1'b1, 3'd0});
  #(dly) ps_cpld_xfer(2'b01, 6'd1, {2'd0, 5'd0, 5'd2, 1'b1, 3'd0});
  #(dly) ps_cpld_xfer(2'b01, 6'd2, {2'd0, 5'd1, 5'd2, 1'b1, 3'd0});
  #(dly) ps_cpld_xfer(2'b01, 6'd3, {2'd0, 5'd1, 5'd3, 1'b1, 3'd0});

  // Load portions of upper RX gain table with some values
  #(dly) ps_cpld_xfer(2'b00, {5'b00110, 1'b0}, 16'h0101); /* Write GAIN_BAND_SEL for upper table */
  #(dly) ps_cpld_xfer(2'b01, 6'd4, {2'd0, 5'd2, 5'd3, 1'b1, 3'd0});
  #(dly) ps_cpld_xfer(2'b01, 6'd5, {2'd0, 5'd2, 5'd4, 1'b1, 3'd0});
  #(dly) ps_cpld_xfer(2'b01, 6'd6, {2'd0, 5'd3, 5'd4, 1'b1, 3'd0});
  #(dly) ps_cpld_xfer(2'b01, 6'd7, {2'd0, 5'd3, 5'd5, 1'b1, 3'd0});

  // Check RX gain table readback
  #(dly) ps_cpld_xfer(2'b01, 6'd0, 16'h0);
  #(dly) ps_cpld_xfer(2'b01, 6'd1, 16'h0);
  #(dly) ps_cpld_xfer(2'b01, 6'd2, 16'h0);
  #(dly) ps_cpld_xfer(2'b01, 6'd3, 16'h0);
  #(dly) ps_cpld_xfer(2'b01, 6'd4, 16'h0);
  #(dly) ps_cpld_xfer(2'b01, 6'd5, 16'h0);
  #(dly) ps_cpld_xfer(2'b01, 6'd6, 16'h0);

  // Check can write a couple registers on PL side
  // (Also make sure we're looking at the lower gain tables)
  #(dly) pl_cpld_xfer(2'b00, {5'd6, 1'b0}, 16'h0000);
  #(dly) pl_cpld_xfer(2'b00, {5'd7, 1'b0}, 16'h0000);

  // Check retrieval of gain values for RX table and program DSAs
  #(dly) pl_cpld_xfer(2'b01, 6'd2, {2'b0, 1'b1, 6'b0, 1'b1, 6'b0});
  #(dly) pl_cpld_xfer(2'b01, 6'd3, {2'b0, 1'b0, 6'b0, 1'b1, 6'b0});
  #(dly) pl_cpld_xfer(2'b01, 6'd1, {2'b0, 1'b1, 6'b0, 1'b0, 6'b0});

  // Check writes to RXBS and TXBS registers
  #(dly) pl_cpld_xfer(2'b00, {5'd6, 1'b0}, 16'h1ABC);
  #(dly) pl_cpld_xfer(2'b00, {5'd7, 1'b0}, 16'h1CAB);

  // Check TX DSA programming is independent of RX DSA programming
  #(dly) pl_cpld_xfer(2'b10, 6'd4, {2'b0, 1'b1, 6'b0, 1'b0, 6'b0});

  // Check LO gain programming works
  #(dly) pl_cpld_xfer(2'b11, 6'd5, {2'b0, 1'b1, 6'b0, 1'b0, 6'b0});
  #(dly) pl_cpld_xfer(2'b11, 6'd7, {2'b0, 1'b0, 6'b0, 1'b1, 6'b0});
  #(dly) pl_cpld_xfer(2'b11, 6'd0, {2'b0, 1'b0, 6'b0, 1'b0, 6'b0});

  // More checks for PL register writes
  #(dly) pl_cpld_xfer(2'b00, {5'd6, 1'b0}, 16'h0ABC);
  #(dly) pl_cpld_xfer(2'b00, {5'd7, 1'b0}, 16'h0CAB);
  #(dly) pl_cpld_xfer(2'b00, {5'd8, 1'b0}, 16'hAA5C);
  #(dly) pl_cpld_xfer(2'b00, {5'd8, 1'b0}, 16'h5A5C);
  #(dly) pl_cpld_xfer(2'b00, {5'd6, 1'b0}, 16'h1C42);

  // Check low/high gain tables and independence of RX vs. TX
  #(dly) pl_cpld_xfer(2'b01, 6'd0, 16'h2040);
  #(dly) pl_cpld_xfer(2'b00, {5'd7, 1'b0}, 16'h104C);
  #(dly) pl_cpld_xfer(2'b10, 6'd0, 16'h2040);
  #(dly) pl_cpld_xfer(2'b00, {5'd7, 1'b0}, 16'h0C80);
  #(dly) pl_cpld_xfer(2'b10, 6'd5, 16'h2040);
  $finish;
end

rhodium_top toplevel_inst(usrpio_io, // bank 1A, 1B and 6
ADC_A_Over_Range_18, ADC_B_Over_Range_18, // bank 1A

// bank 6
CPLD_PS_SPI_LE_25,
CPLD_PS_SPI_CLK_25,
CPLD_PS_ADDR0_25,
CPLD_PS_ADDR1_25,
CPLD_PS_SPI_SDI_25,
CPLD_PS_SPI_SDO_25,
PHDAC_SPI_CS_L, PHDAC_SPI_SCLK, PHDAC_SPI_SDI,
LO_SYNC,

// bank 2
CPLD_PL_SPI_SDO_18,
CPLD_PL_SPI_LE_18,
CPLD_PL_SPI_SCLK_18,
CPLD_PL_SPI_SDI_18,
CPLD_PL_SPI_ADDR0_18,
CPLD_PL_SPI_ADDR1_18,
CPLD_PL_SPI_ADDR2_18,
CPLD_ATR_TX_18,
CPLD_ATR_RX_18,
ADC_SPI_CS_L_18,
ADC_SPI_SCLK_18,
ADC_SPI_SDIO_18,
DAC_SPI_CS_L_18,
DAC_SPI_SCLK_18,
DAC_SPI_SDIO_18,
DAC_Alarm_18,

// bank 3

CLKDIST_SPI_CS_L,
CLKDIST_SPI_SCLK,
CLKDIST_SPI_SDIO,
Tx_DSA_C1,
Tx_DSA_C2,
Tx_DSA_C4,
Tx_DSA_C8,
Tx_DSA_C16,
Tx_DSA1_LE,
Tx_DSA2_LE,
Tx_Sw1_Ctrl_1,
Tx_Sw1_Ctrl_2,
Tx_Sw2_Ctrl_1,
Tx_Sw2_Ctrl_2,
Tx_Sw3_Ctrl_1,
Tx_Sw3_Ctrl_2,
Tx_Sw3_Ctrl_3,
Tx_Sw3_Ctrl_4,
Rx_LO_Input_Select,
Rx_LO_Filter_Sw_1,
Rx_LO_Filter_Sw_2,
Tx_LO_Input_Select,
Tx_LO_Filter_Sw_1,
Tx_LO_Filter_Sw_2,
CLKDIST_Status_LD1,
CLKDIST_Status_LD2,
LOSYNTH_RX_MUXOUT,
LOSYNTH_TX_MUXOUT,

// bank 8
LO_SPI_SCLK, // fans out to both rx & tx synths
LO_SPI_SDI,
LO_TX_CS_L,
LO_RX_CS_L,
Rx_Sw1_Ctrl_1,
Rx_Sw1_Ctrl_2,
Rx_DSA_C1,
Rx_DSA_C2,
Rx_DSA_C4,
Rx_DSA_C8,
Rx_DSA_C16,
Rx_DSA1_LE,
Rx_DSA2_LE,
Rx_Sw2_Ctrl,
Rx_Sw3_Ctrl_1,
Rx_Sw3_Ctrl_2,
Rx_Sw4_Ctrl_1,
Rx_Sw4_Ctrl_2,
Rx_Sw4_Ctrl_3,
Rx_Sw4_Ctrl_4,
Rx_Demod_ADJ_1,
Rx_Demod_ADJ_2,

// bank 5
LO_DSA_C1,
LO_DSA_C2,
LO_DSA_C4,
LO_DSA_C8,
LO_DSA_C16,
RxLO_DSA_LE,
TxLO_DSA_LE,
LODIST_Bd_SPI_CS_L,
LODIST_Bd_SPI_SDI,
LODIST_Bd_SPI_SCLK,
LODIST_Bd_IO1,
Tx_Sw5_Ctrl_1,
Tx_Sw5_Ctrl_2,
Rx_Sw6_Ctrl_1,
Rx_Sw6_Ctrl_2,

Tx_HB_LB_Select,
Rx_HB_LB_Select,
Cal_iso_Sw_Ctrl


);
endmodule // rh_tb

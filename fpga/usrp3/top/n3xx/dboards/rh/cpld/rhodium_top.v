///////////////////////////////////////////////////////////////////
//
// Copyright 2018 Ettus Research, A National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: rhodium_top
//////////////////////////////////////////////////////////////////////

`default_nettype none

module rhodium_top(

input [13:0] usrp_io, // bank 1A, 1B and 6

input ADC_A_Over_Range_18, input ADC_B_Over_Range_18, // bank 1A

// bank 6
input      CPLD_PS_SPI_LE_25,
input      CPLD_PS_SPI_CLK_25,
input      CPLD_PS_ADDR0_25,
input      CPLD_PS_ADDR1_25,
input      CPLD_PS_SPI_SDI_25,
output reg CPLD_PS_SPI_SDO_25,
output     PHDAC_SPI_CS_L,
output     PHDAC_SPI_SCLK,
output     PHDAC_SPI_SDI,
input      LO_SYNC,

// bank 2
output reg CPLD_PL_SPI_SDO_18,
input      CPLD_PL_SPI_LE_18,
input      CPLD_PL_SPI_SCLK_18,
input      CPLD_PL_SPI_SDI_18,
input      CPLD_PL_SPI_ADDR0_18,
input      CPLD_PL_SPI_ADDR1_18,
input      CPLD_PL_SPI_ADDR2_18,
// NOTE: TxRx front-end switches are driven direct from the motherboard, so these ATR 
//       lines have no function at this time.
input      CPLD_ATR_TX_18,
input      CPLD_ATR_RX_18,
output     ADC_SPI_CS_L_18,
output     ADC_SPI_SCLK_18,
inout      ADC_SPI_SDIO_18,
output     DAC_SPI_CS_L_18,
output     DAC_SPI_SCLK_18,
inout      DAC_SPI_SDIO_18,
input      DAC_Alarm_18, // TODO: drive to gpio?

// bank 3

output     CLKDIST_SPI_CS_L,
output     CLKDIST_SPI_SCLK,
inout      CLKDIST_SPI_SDIO,
output     Tx_DSA_C1,
output     Tx_DSA_C2,
output     Tx_DSA_C4,
output     Tx_DSA_C8,
output     Tx_DSA_C16,
output     Tx_DSA1_LE,
output     Tx_DSA2_LE,
output     Tx_Sw1_Ctrl_1,
output     Tx_Sw1_Ctrl_2,
output     Tx_Sw2_Ctrl_1,
output     Tx_Sw2_Ctrl_2,
output     Tx_Sw3_Ctrl_1,
output     Tx_Sw3_Ctrl_2,
output     Tx_Sw3_Ctrl_3,
output     Tx_Sw3_Ctrl_4,
output     Rx_LO_Input_Select,
output     Rx_LO_Filter_Sw_1,
output     Rx_LO_Filter_Sw_2,
output     Tx_LO_Input_Select,
output     Tx_LO_Filter_Sw_1,
output     Tx_LO_Filter_Sw_2,
input      CLKDIST_Status_LD1,
input      CLKDIST_Status_LD2,
input      LOSYNTH_RX_MUXOUT,
input      LOSYNTH_TX_MUXOUT,

// bank 8
output     LO_SPI_SCLK, // fans out to both rx & tx synths
output     LO_SPI_SDI,
output     LO_TX_CS_L,
output     LO_RX_CS_L,
output     Rx_Sw1_Ctrl_1,
output     Rx_Sw1_Ctrl_2,
output     Rx_DSA_C1,
output     Rx_DSA_C2,
output     Rx_DSA_C4,
output     Rx_DSA_C8,
output     Rx_DSA_C16,
output     Rx_DSA1_LE,
output     Rx_DSA2_LE,
output     Rx_Sw2_Ctrl,
output     Rx_Sw3_Ctrl_1,
output     Rx_Sw3_Ctrl_2,
output     Rx_Sw4_Ctrl_1,
output     Rx_Sw4_Ctrl_2,
output     Rx_Sw4_Ctrl_3,
output     Rx_Sw4_Ctrl_4,
output     Rx_Demod_ADJ_1,
output     Rx_Demod_ADJ_2,

// bank 5
output     LO_DSA_C1,
output     LO_DSA_C2,
output     LO_DSA_C4,
output     LO_DSA_C8,
output     LO_DSA_C16,
output     RxLO_DSA_LE,
output     TxLO_DSA_LE,
output     LODIST_Bd_SPI_CS_L,
output     LODIST_Bd_SPI_SDI,
output     LODIST_Bd_SPI_SCLK,
inout      LODIST_Bd_IO1,
output     Tx_Sw5_Ctrl_1,
output     Tx_Sw5_Ctrl_2,
output     Rx_Sw6_Ctrl_1,
output     Rx_Sw6_Ctrl_2,

output    Tx_HB_LB_Select,
output    Rx_HB_LB_Select,
output    Cal_iso_Sw_Ctrl


);

/* PS SPI */

localparam GIT_HASH = 36'h`GIT_HASH;
localparam PROD_SIGNATURE   = 16'h0045; // Product signature (Rhodium atomic number in BCD)
localparam REVISION_MINOR   = 16'h0002;
localparam REVISION_MAJOR   = 16'h0004;
localparam CPLD_BUILD_LSB   = GIT_HASH[15:0]; // Build code LSB
localparam CPLD_BUILD_MSB   = GIT_HASH[31:16]; // Build code MSB
localparam PSADDR_SIGNATURE = 3'd0;
localparam PSADDR_REV_MINOR = 3'd1; // Minor version register
localparam PSADDR_REV_MAJOR = 3'd2; // Major version register
localparam PSADDR_BUILD_LSB = 3'd3;
localparam PSADDR_BUILD_MSB = 3'd4;
localparam PSADDR_SCRATCH   = 3'd5; // scratchpad register
localparam PSADDR_GAIN_SEL  = 3'd6; // band select for gain table loader
localparam PSADDR_DAC_ALARM = 3'd7; // DAC alarm pin register

// Sub-device selection for PS SPI
localparam PS_CPLD_REGS   = 2'b00;
localparam GAIN_TABLE_RX  = 2'b01;
localparam GAIN_TABLE_TX  = 2'b10;
localparam GAIN_TABLE_LO  = 2'b11;

// Setting to put TX SW1 in isolation mode
localparam [1:0] TX_SW1_TERM = 2'b11;

wire clkdis_cs_b = CPLD_PS_SPI_LE_25;
wire cpld_ps_cs_b = CPLD_PS_ADDR0_25;
wire phdac_cs_b = CPLD_PS_ADDR1_25;
wire adc_cs_b = usrp_io[12];
wire dac_cs_b = usrp_io[13];

// CPLD PS SPI format (left-most bit first):
// {table_sel[1:0], rsvd, reg_addr[3:0], rnw, data[15:0]}
wire [1:0] cpld_ps_table_sel;
wire [6:0] cpld_ps_spi_addr;
wire cpld_ps_spi_rnw;
reg [7:0] cpld_ps_spi_cmd;
reg [15:0] cpld_ps_spi_rdata;
reg [14:0] cpld_ps_spi_wdata;
reg cpld_ps_spi_sdo;
reg [4:0] cpld_ps_cnt;

assign {cpld_ps_spi_addr, cpld_ps_spi_rnw} = cpld_ps_spi_cmd;

// CPLD registers
reg [15:0] spad;
reg [15:0] gain_load_sel;

// Double sync. the DAC ALARM pin (async).
reg dac_alarm_ms, dac_alarm = 0;
always @(posedge CPLD_PS_SPI_CLK_25) begin
  {dac_alarm, dac_alarm_ms} <= {dac_alarm_ms, DAC_Alarm_18};
end

wire rx_gain_load_tbl_sel;
wire rx_gain_load_miso;
wire rx_gain_ctrl_tbl_sel;
wire rx_gain_ctrl_miso;
wire tx_gain_load_tbl_sel;
wire tx_gain_load_miso;
wire tx_gain_ctrl_tbl_sel;
wire tx_gain_ctrl_miso;
wire lo_gain_ctrl_miso;

assign rx_gain_load_tbl_sel = gain_load_sel[0];
assign tx_gain_load_tbl_sel = gain_load_sel[8];

always @(posedge CPLD_PS_SPI_CLK_25 or posedge cpld_ps_cs_b)
begin
  if (cpld_ps_cs_b) begin
    cpld_ps_cnt <= 5'd0;
  end else if (!cpld_ps_cs_b) begin
    if (cpld_ps_cnt < 8) begin // Address / command
      cpld_ps_spi_cmd <= {cpld_ps_spi_cmd[6:0], CPLD_PS_SPI_SDI_25};
      cpld_ps_cnt <= cpld_ps_cnt + 5'd1;
    end else if (cpld_ps_cnt < 23) begin // Shift in write data
      cpld_ps_spi_wdata <= {cpld_ps_spi_wdata[13:0], CPLD_PS_SPI_SDI_25};
      cpld_ps_cnt <= cpld_ps_cnt + 5'd1;
    end else if (!cpld_ps_spi_rnw && cpld_ps_cnt == 23 && cpld_ps_spi_addr[6:5] == PS_CPLD_REGS) begin // Write
      case (cpld_ps_spi_addr[2:0])
      PSADDR_SIGNATURE: ;
      PSADDR_REV_MINOR: ;
      PSADDR_REV_MAJOR: ;
      PSADDR_BUILD_LSB: ;
      PSADDR_BUILD_MSB: ;
      PSADDR_SCRATCH:   spad <= {cpld_ps_spi_wdata, CPLD_PS_SPI_SDI_25};
      PSADDR_GAIN_SEL:  gain_load_sel <= {cpld_ps_spi_wdata, CPLD_PS_SPI_SDI_25};
      endcase
    end
    if (cpld_ps_cnt == 7) begin // Set up read one cycle earlier
      case (cpld_ps_spi_cmd[2:0])

      PSADDR_SIGNATURE: cpld_ps_spi_rdata <= PROD_SIGNATURE;
      PSADDR_REV_MINOR: cpld_ps_spi_rdata <= REVISION_MINOR;
      PSADDR_REV_MAJOR: cpld_ps_spi_rdata <= REVISION_MAJOR;
      PSADDR_BUILD_LSB: cpld_ps_spi_rdata <= CPLD_BUILD_LSB;
      PSADDR_BUILD_MSB: cpld_ps_spi_rdata <= CPLD_BUILD_MSB;
      PSADDR_SCRATCH:   cpld_ps_spi_rdata <= spad;
      PSADDR_GAIN_SEL:  cpld_ps_spi_rdata <= gain_load_sel;
      PSADDR_DAC_ALARM: cpld_ps_spi_rdata <= {15'b0, dac_alarm};
      endcase
    end else begin
      cpld_ps_spi_rdata <= {cpld_ps_spi_rdata[14:0], 1'b1};
    end
  end
end

always @(negedge CPLD_PS_SPI_CLK_25)
begin
  cpld_ps_spi_sdo <= cpld_ps_spi_rdata[15]; // Shift out on negative edge
end

// CLKDIST 3-wire to 4-wire
reg [4:0] clkdis_cnt;
reg       clkdis_rd_pre, clkdis_rd, clkdis_sdio_t;

always @(posedge CPLD_PS_SPI_CLK_25 or posedge clkdis_cs_b)
begin
  if (clkdis_cs_b) begin
    clkdis_cnt <= 5'd0;
    clkdis_rd <= 1'b0;
    clkdis_rd_pre <= 1'b0;
  end else if (!clkdis_cs_b) begin
    if (clkdis_cnt < 23)
      clkdis_cnt <= clkdis_cnt + 5'd1;

    if (clkdis_cnt == 5'd0) // Check if read
      clkdis_rd_pre <= CPLD_PS_SPI_SDI_25;

    if (clkdis_cnt == 5'd15)
      clkdis_rd <= clkdis_rd_pre;
  end
end

always @(negedge CPLD_PS_SPI_CLK_25 or posedge clkdis_cs_b)
begin
  if (clkdis_cs_b) begin
    clkdis_sdio_t <= 1'b0;
  end else begin
    clkdis_sdio_t <= clkdis_rd;
  end
end

// ADC 3-wire to 4-wire
reg [4:0] adc_cnt;
reg       adc_rd_pre, adc_rd, adc_sdio_t;

always @(posedge CPLD_PS_SPI_CLK_25 or posedge adc_cs_b)
begin
  if (adc_cs_b) begin
    adc_cnt <= 5'd0;
    adc_rd <= 1'b0;
    adc_rd_pre <= 1'b0;
  end else if (!adc_cs_b) begin
    if (adc_cnt < 23)
      adc_cnt <= adc_cnt + 5'd1;

    if (adc_cnt == 5'd0) // Check if read
      adc_rd_pre <= CPLD_PS_SPI_SDI_25;

    if (adc_cnt == 5'd15)
      adc_rd <= adc_rd_pre;
  end
end

always @(negedge CPLD_PS_SPI_CLK_25 or posedge adc_cs_b)
begin
  if (adc_cs_b) begin
    adc_sdio_t <= 1'b0;
  end else begin
    adc_sdio_t <= adc_rd;
  end
end

// DAC 3-wire to 4-wire
reg [4:0] dac_cnt;
reg       dac_rd_pre, dac_rd, dac_sdio_t;

always @(posedge CPLD_PS_SPI_CLK_25 or posedge dac_cs_b)
begin
  if (dac_cs_b) begin
    dac_cnt <= 5'd0;
    dac_rd <= 1'b0;
    dac_rd_pre <= 1'b0;
  end else if (!dac_cs_b) begin
    if (dac_cnt < 23)
      dac_cnt <= dac_cnt + 5'd1;

    if (dac_cnt == 5'd0) // Check if read
      dac_rd_pre <= CPLD_PS_SPI_SDI_25;

    if (dac_cnt == 5'd7)
      dac_rd <= dac_rd_pre;
  end
end

always @(negedge CPLD_PS_SPI_CLK_25 or posedge dac_cs_b)
begin
  if (dac_cs_b) begin
    dac_sdio_t <= 1'b0;
  end else begin
    dac_sdio_t <= dac_rd;
  end
end

// multiplexed slave device SPI ports
wire phdac_sck, phdac_sdi; 
wire clkdis_sck, adc_sck, dac_sck;
assign clkdis_sck = (clkdis_cs_b == 1'b0) ? CPLD_PS_SPI_CLK_25 : 1'b0;

assign CLKDIST_SPI_CS_L = clkdis_cs_b;  
assign CLKDIST_SPI_SCLK = clkdis_sck;

assign adc_sck = !adc_cs_b ? CPLD_PS_SPI_CLK_25 : 1'b0;
assign dac_sck = !dac_cs_b ? CPLD_PS_SPI_CLK_25 : 1'b0;

assign ADC_SPI_CS_L_18 = adc_cs_b;
assign ADC_SPI_SCLK_18 = adc_sck;

assign DAC_SPI_CS_L_18 = dac_cs_b;
assign DAC_SPI_SCLK_18 = dac_sck;

assign CLKDIST_SPI_SDIO = (!clkdis_sdio_t && !clkdis_cs_b) ? CPLD_PS_SPI_SDI_25 : 1'bz ;
assign ADC_SPI_SDIO_18  = (!adc_sdio_t && !adc_cs_b)       ? CPLD_PS_SPI_SDI_25 : 1'bz ;
assign DAC_SPI_SDIO_18  = (!dac_sdio_t && !dac_cs_b)       ? CPLD_PS_SPI_SDI_25 : 1'bz ;

always @(*)
begin
  CPLD_PS_SPI_SDO_25 = 1'b1;
  case ({cpld_ps_cs_b, clkdis_cs_b, adc_cs_b, dac_cs_b})
  4'b0111: begin
    case (cpld_ps_spi_addr[6:5])
    PS_CPLD_REGS : CPLD_PS_SPI_SDO_25 = cpld_ps_spi_sdo;
    GAIN_TABLE_RX: CPLD_PS_SPI_SDO_25 = rx_gain_load_miso;
    GAIN_TABLE_TX: CPLD_PS_SPI_SDO_25 = tx_gain_load_miso;
    GAIN_TABLE_LO: CPLD_PS_SPI_SDO_25 = 1'b1;
    endcase
  end
  4'b1011: CPLD_PS_SPI_SDO_25 = CLKDIST_SPI_SDIO;
  4'b1101: CPLD_PS_SPI_SDO_25 = ADC_SPI_SDIO_18;
  4'b1110: CPLD_PS_SPI_SDO_25 = DAC_SPI_SDIO_18;
  default: ;
  endcase
end  

// note: no readback from PHDAC
assign phdac_sck = (phdac_cs_b == 1'b0) ? CPLD_PS_SPI_CLK_25 : 1'b0;
assign phdac_sdi = (phdac_cs_b == 1'b0) ? CPLD_PS_SPI_SDI_25 : 1'b1;


assign PHDAC_SPI_SCLK = phdac_sck;
assign PHDAC_SPI_CS_L = phdac_cs_b;
assign PHDAC_SPI_SDI = phdac_sdi;


/* PL SPI */ 
// CPLD PL SPI format (left-most bit first):
// {table_sel[1:0], reg_addr[4:0], rnw, data[15:0]}

//TXLO, RXLO, LODIS, CPLD
localparam PLADDR_SCRATCH = 4'b0101; // scratchpad register
localparam PLADDR_RXBS    = 4'b0110;
localparam PLADDR_TXBS    = 4'b0111;
localparam PLADDR_RFCTRL  = 4'b1000;
localparam PL_CPLD_REGS   = 2'b00;

// CPLD PL registers
reg [15:0] rxbs = 'h0;
reg [15:0] txbs = 'h0;
reg [15:0] rfctrl = 'h0;

// register address on the falling edge of chip-select
wire txlo_cs_b    = CPLD_PL_SPI_LE_18;
wire rxlo_cs_b    = CPLD_PL_SPI_ADDR1_18;
wire lodis_cs_b   = CPLD_PL_SPI_ADDR2_18;
wire cpld_pl_cs_b = CPLD_PL_SPI_ADDR0_18;

wire cpld_pl_spi_rnw;
wire [6:0] cpld_pl_spi_addr;
reg [7:0] cpld_pl_spi_cmd;
reg [15:0] cpld_pl_spi_rdata;
reg [14:0] cpld_pl_spi_wdata;
reg cpld_pl_spi_sdo;
reg [4:0] cpld_pl_cnt;

assign {cpld_pl_spi_addr, cpld_pl_spi_rnw} = cpld_pl_spi_cmd;

reg [15:0] pl_spad;

always @(posedge CPLD_PL_SPI_SCLK_18 or posedge cpld_pl_cs_b)
begin
  if (cpld_pl_cs_b) begin
    cpld_pl_cnt <= 5'd0;
  end else if (!cpld_pl_cs_b) begin
    if (cpld_pl_cnt < 8) begin // Address / command
      cpld_pl_spi_cmd <= {cpld_pl_spi_cmd[6:0], CPLD_PL_SPI_SDI_18};
      cpld_pl_cnt <= cpld_pl_cnt + 5'd1;
    end else if (cpld_pl_cnt < 23) begin // Shift in write data
      cpld_pl_spi_wdata <= {cpld_pl_spi_wdata[13:0], CPLD_PL_SPI_SDI_18};
      cpld_pl_cnt <= cpld_pl_cnt + 5'd1;
    end else if (!cpld_pl_spi_rnw && cpld_pl_cnt == 23 && cpld_pl_spi_addr[6:5] == PL_CPLD_REGS) begin // Write
      case (cpld_pl_spi_addr[3:0])
      PLADDR_SCRATCH: pl_spad <= {cpld_pl_spi_wdata, CPLD_PL_SPI_SDI_18};
      PLADDR_RXBS:    rxbs    <= {cpld_pl_spi_wdata, CPLD_PL_SPI_SDI_18};
      PLADDR_TXBS:    txbs    <= {cpld_pl_spi_wdata, CPLD_PL_SPI_SDI_18};
      PLADDR_RFCTRL:  rfctrl  <= {cpld_pl_spi_wdata, CPLD_PL_SPI_SDI_18};
      endcase
    end
    if (cpld_pl_cnt == 7) begin // Set up read one cycle earlier
      case (cpld_pl_spi_cmd[3:0])
      PLADDR_SCRATCH: cpld_pl_spi_rdata <= pl_spad;
      PLADDR_RXBS:    cpld_pl_spi_rdata <= rxbs;
      PLADDR_TXBS:    cpld_pl_spi_rdata <= txbs;
      PLADDR_RFCTRL:  cpld_pl_spi_rdata <= rfctrl;
      endcase
    end else begin
      cpld_pl_spi_rdata <= {cpld_pl_spi_rdata[14:0], 1'b1};
    end
  end
end

always @(negedge CPLD_PL_SPI_SCLK_18)
begin
  cpld_pl_spi_sdo <= cpld_pl_spi_rdata[15]; // Shift out on negative edge
end

// multiplexed slave device SPI ports, names aliased to protect the innocent
wire lo_sck, lodis_sck;
wire lo_sdi, lodis_sdi;
// Note: lo_sck and lo_sdi -> fan out to both rxlo and txlo synths

assign { LO_TX_CS_L, LO_RX_CS_L } = { txlo_cs_b, rxlo_cs_b};
assign LO_SPI_SCLK = lo_sck;
assign LO_SPI_SDI = lo_sdi;


assign LODIST_Bd_SPI_CS_L = lodis_cs_b;
assign LODIST_Bd_SPI_SDI = lodis_sdi;
assign LODIST_Bd_SPI_SCLK = lodis_sck;


assign lodis_sck = !lodis_cs_b ? CPLD_PL_SPI_SCLK_18 : 1'b0;
assign lodis_sdi = !lodis_cs_b ? CPLD_PL_SPI_SDI_18 : 1'b1;

assign { lo_sck, lo_sdi } = (!txlo_cs_b | !rxlo_cs_b) ? {CPLD_PL_SPI_SCLK_18,CPLD_PL_SPI_SDI_18} : 2'b01;

	 
always @(*)
begin
  CPLD_PL_SPI_SDO_18 = 1'bz;
  case ({cpld_pl_cs_b, txlo_cs_b, rxlo_cs_b})
  3'b110: CPLD_PL_SPI_SDO_18 = LOSYNTH_RX_MUXOUT;
  3'b101: CPLD_PL_SPI_SDO_18 = LOSYNTH_TX_MUXOUT; 
  3'b011: begin
    case (cpld_pl_spi_addr[6:5])
    PL_CPLD_REGS : CPLD_PL_SPI_SDO_18 = cpld_pl_spi_sdo;
    GAIN_TABLE_RX: CPLD_PL_SPI_SDO_18 = rx_gain_ctrl_miso;
    GAIN_TABLE_TX: CPLD_PL_SPI_SDO_18 = tx_gain_ctrl_miso;
    GAIN_TABLE_LO: CPLD_PL_SPI_SDO_18 = lo_gain_ctrl_miso;
    endcase
  end
  default: ;
  endcase
end  

assign rx_gain_ctrl_tbl_sel = rxbs[12];
assign { Rx_Sw6_Ctrl_2,
         Rx_Sw6_Ctrl_1,
         Rx_Sw4_Ctrl_4,
         Rx_Sw4_Ctrl_3,
         Rx_Sw4_Ctrl_2,
         Rx_Sw4_Ctrl_1,
         Rx_Sw3_Ctrl_2,
         Rx_Sw3_Ctrl_1,
         Rx_Sw2_Ctrl,
         Rx_Sw1_Ctrl_2,
         Rx_Sw1_Ctrl_1	} = { rxbs[11:1] };

assign tx_gain_ctrl_tbl_sel = txbs[12];
assign { Tx_Sw5_Ctrl_2,
         Tx_Sw5_Ctrl_1,
         Tx_Sw3_Ctrl_4,
         Tx_Sw3_Ctrl_3,
         Tx_Sw3_Ctrl_2,
         Tx_Sw3_Ctrl_1,
         Tx_Sw2_Ctrl_2,
         Tx_Sw2_Ctrl_1} = { txbs[11:4] };

// Terminate TX when idle
assign {Tx_Sw1_Ctrl_2, Tx_Sw1_Ctrl_1} = CPLD_ATR_TX_18 ? txbs[3:2] : TX_SW1_TERM;

assign { Rx_LO_Filter_Sw_2,
  Rx_LO_Filter_Sw_1,
  Tx_LO_Filter_Sw_2,
  Tx_LO_Filter_Sw_1,
  Rx_Demod_ADJ_1,
  Rx_Demod_ADJ_2, 
  Rx_LO_Input_Select } = rfctrl[15:9];

assign { Rx_HB_LB_Select,
  Tx_LO_Input_Select } = rfctrl[7:6];

assign { Tx_HB_LB_Select,
  Cal_iso_Sw_Ctrl } 
  = { rfctrl[4:3] };
  

// RX Gain Table
wire [4:0] rx_dsa;

rhodium_gain_ctrl #(
  .TABLE_NUM(GAIN_TABLE_RX)
) rx_gain_table (
  .load_table_sel(rx_gain_load_tbl_sel),
  .load_sck(CPLD_PS_SPI_CLK_25),
  .load_csb(cpld_ps_cs_b),
  .load_mosi(CPLD_PS_SPI_SDI_25),
  .load_miso(rx_gain_load_miso),
  .ctrl_table_sel(rx_gain_ctrl_tbl_sel),
  .ctrl_sck(CPLD_PL_SPI_SCLK_18),
  .ctrl_csb(cpld_pl_cs_b),
  .ctrl_mosi(CPLD_PL_SPI_SDI_18),
  .ctrl_miso(rx_gain_ctrl_miso),
  .dsa(rx_dsa),
  .dsa1_le(Rx_DSA1_LE),
  .dsa2_le(Rx_DSA2_LE)
);

// TX Gain Table
wire [4:0] tx_dsa;

rhodium_gain_ctrl #(
  .TABLE_NUM(GAIN_TABLE_TX)
) tx_gain_table (
  .load_table_sel(tx_gain_load_tbl_sel),
  .load_sck(CPLD_PS_SPI_CLK_25),
  .load_csb(cpld_ps_cs_b),
  .load_mosi(CPLD_PS_SPI_SDI_25),
  .load_miso(tx_gain_load_miso),
  .ctrl_table_sel(tx_gain_ctrl_tbl_sel),
  .ctrl_sck(CPLD_PL_SPI_SCLK_18),
  .ctrl_csb(cpld_pl_cs_b),
  .ctrl_mosi(CPLD_PL_SPI_SDI_18),
  .ctrl_miso(tx_gain_ctrl_miso),
  .dsa(tx_dsa),
  .dsa1_le(Tx_DSA1_LE),
  .dsa2_le(Tx_DSA2_LE)
);

// LO Gain Table
wire [4:0] lo_dsa;

rhodium_lo_gain #(
  .TABLE_NUM(GAIN_TABLE_LO)
) lo_gain_table (
  .ctrl_sck(CPLD_PL_SPI_SCLK_18),
  .ctrl_csb(cpld_pl_cs_b),
  .ctrl_mosi(CPLD_PL_SPI_SDI_18),
  .ctrl_miso(lo_gain_ctrl_miso),
  .dsa(lo_dsa),
  .dsa1_le(RxLO_DSA_LE),
  .dsa2_le(TxLO_DSA_LE)
);

// Rx data shared by DSA1, DSA2
assign  { Rx_DSA_C16, Rx_DSA_C8, Rx_DSA_C4, Rx_DSA_C2, Rx_DSA_C1 } = rx_dsa;

// Tx data shared by DSA1, DSA2	
assign  { Tx_DSA_C16, Tx_DSA_C8, Tx_DSA_C4, Tx_DSA_C2, Tx_DSA_C1 } = tx_dsa;

// data shared by both tx and rx lo DSAs	
assign  { LO_DSA_C16, LO_DSA_C8, LO_DSA_C4, LO_DSA_C2, LO_DSA_C1 } =  lo_dsa;

endmodule
`default_nettype wire


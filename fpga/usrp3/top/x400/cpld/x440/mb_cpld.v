//
// Copyright 2022 Ettus Research, A National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: mb_cpld
//
// Description:
//
//   Top level file for the X440 motherboard CPLD.
//
// Parameters:
//
//   SIMULATION : Set to 1 to speed up simulation.
//

`default_nettype none


module mb_cpld #(
  parameter SIMULATION = 0
) (
  //---------------------------------------------------------------------------
  // Clocking
  //---------------------------------------------------------------------------

  // CPLD's PLL reference clock (differential input; abbreviation: pclk)
  input  wire PLL_REF_CLK,

  // Reliable clock (100 MHz; differential input)
  input  wire CLK_100,

  //---------------------------------------------------------------------------
  // Power Supplies
  //---------------------------------------------------------------------------

  // Power supply clocks
  output wire PWR_SUPPLY_CLK_CORE,
  output wire PWR_SUPPLY_CLK_DDR4_S,
  output wire PWR_SUPPLY_CLK_DDR4_N,
  output wire PWR_SUPPLY_CLK_0P9V,
  output wire PWR_SUPPLY_CLK_1P8V,
  output wire PWR_SUPPLY_CLK_2P5V,
  output wire PWR_SUPPLY_CLK_3P3V,
  output wire PWR_SUPPLY_CLK_3P6V,

  // Power supply control
  output wire       PWR_EN_5V_OSC_100,
  output wire       PWR_EN_5V_OSC_122_88,
  output wire       IPASS_POWER_DISABLE,
  input  wire [1:0] IPASS_POWER_EN_FAULT,

  //---------------------------------------------------------------------------
  // Interfaces from/to RFSoC
  //---------------------------------------------------------------------------

  // PL SPI slave interface
  input  wire       PL_CPLD_SCLK,
  input  wire       PL_CPLD_MOSI,
  output reg        PL_CPLD_MISO,
  input  wire [1:0] PL_CPLD_CS_N,

  // IRQ to PL
  output wire       PL_CPLD_IRQ,

  // PS SPI slave interface
  // Chip Selects:
  //   PS_CPLD_CS_N(2:0) -> binary encoded chip select
  //   PS_CPLD_CS_N(3)   -> chip select "enable"
  input  wire       PS_CPLD_SCLK,
  input  wire       PS_CPLD_MOSI,
  output wire       PS_CPLD_MISO,
  input  wire [3:0] PS_CPLD_CS_N,

  //---------------------------------------------------------------------------
  // PL Interfaces to/from Motherboard
  //---------------------------------------------------------------------------

  // Clocking AUX board SPI master interface
  output wire CLK_DB_SCLK,
  output wire CLK_DB_MOSI,
  input  wire CLK_DB_MISO,
  output wire CLK_DB_CS_N,

  // QSFP LEDs
  // Port 0
  output wire [3:0] QSFP0_LED_ACTIVE,
  output wire [3:0] QSFP0_LED_LINK,
  // Port 1
  output wire [3:0] QSFP1_LED_ACTIVE,
  output wire [3:0] QSFP1_LED_LINK,

  // Daughterboard LED GPIO
  // 1 -> DB1 / 0 -> DB0
  output wire [1:0] CH0_RX2_LED,
  output wire [1:0] CH0_TX_LED,
  output wire [1:0] CH0_RX_LED,
  output wire [1:0] CH1_RX2_LED,
  output wire [1:0] CH1_TX_LED,
  output wire [1:0] CH1_RX_LED,
  output wire [1:0] CH2_RX2_LED,
  output wire [1:0] CH2_TX_LED,
  output wire [1:0] CH2_RX_LED,
  output wire [1:0] CH3_RX2_LED,
  output wire [1:0] CH3_TX_LED,
  output wire [1:0] CH3_RX_LED,
  //---------------------------------------------------------------------------
  // PS Interfaces to/from Motherboard
  //---------------------------------------------------------------------------

  // LMK04832 SPI master interface
  output wire LMK32_SCLK,
  output wire LMK32_MOSI,
  input  wire LMK32_MISO,
  output wire LMK32_CS_N,

  // TPM 2.0 SPI master interface
  // Note: TPM is not currently supported
  output wire TPM_SCLK,
  output wire TPM_MOSI,
  input  wire TPM_MISO,
  output wire TPM_CS_N,

  // Phase DAC SPI master interface
  output wire PHASE_DAC_SCLK,
  output wire PHASE_DAC_MOSI,
  output wire PHASE_DAC_CS_N,

  // DIO direction control
  output wire [11:0] DIO_DIRECTION_A,
  output wire [11:0] DIO_DIRECTION_B,

  //---------------------------------------------------------------------------
  // Miscellaneous
  //---------------------------------------------------------------------------

  // This signal enables the 1.8 V and 3.3 V power supply clocks.
  output wire PS_CLK_ON_CPLD,

  // iPASS control interface
  input  wire [1:0] IPASS_PRESENT_N,
  inout  wire [1:0] IPASS_SCL,
  inout  wire [1:0] IPASS_SDA,

  // PCIe reset to FPGA
  output wire PCIE_RESET,

  // TPM reset
  output wire TPM_RESET_n
);

  // SPI masters (spi_top) are limited to 64 bit transmission length
  `define SPI_MAX_CHAR_64

  `include "../../../../lib/rfnoc/core/ctrlport.vh"
  `include "../regmap/mb_cpld_ps_regmap_utils.vh"
  `include "../regmap/x440/mb_cpld_pl_regmap_utils.vh"

  //---------------------------------------------------------------------------
  // Clocks and Resets
  //---------------------------------------------------------------------------

  wire clk40, clk50, clk250;
  wire pll_ref_clk_int;

  wire reset_clk50;
  wire reset_clk40;
  wire power_on_reset_clk100;

  wire [0:0] pll_locked_async;
  wire [0:0] pll_locked_clk50;
  wire [0:0] pll_locked_clk40;

  wire pll_ref_clk_en_clk50;
  wire pll_ref_clk_en_pclk;

  reset_generator reliable_reset_gen_inst (
    .clk            (CLK_100),
    .power_on_reset (power_on_reset_clk100)
  );

  // Divide reliable clock by 2 since the design is not capable of running at
  // 100 MHz. Multiple by 2.5 to get a fast clock to handle PS SPI chip select
  // decoding.
  pll pll_inst (
    .inclk0 (CLK_100),
    .c0     (clk50),
    .c1     (clk250),
    .c2     (clk40),
    .locked (pll_locked_async)
  );

  // Bring pll_ref_clk enable signal to the same clock domain.
  synchronizer #(
    .WIDTH            (1),
    .STAGES           (2),
    .INITIAL_VAL      (1'b0),
    .FALSE_PATH_TO_IN (1)
  ) pll_ref_clk_en_sync (
    .clk (PLL_REF_CLK),
    .rst (1'b0),
    .in  (pll_ref_clk_en_clk50),
    .out (pll_ref_clk_en_pclk)
  );

  // Enable clock using ALTCLKCTRL IP.
  clkctrl pll_ref_clk_ctrl_inst (
    .inclk  (PLL_REF_CLK),
    .ena    (pll_ref_clk_en_pclk),
    .outclk (pll_ref_clk_int)
  );

  // Use locked signal as reset for clk50 and clk40 clock domain
  synchronizer #(
    .WIDTH            (1),
    .STAGES           (2),
    .INITIAL_VAL      (1'b0),
    .FALSE_PATH_TO_IN (1)
  ) clk50_reset_sync (
    .clk (clk50),
    .rst (1'b0),
    .in  (pll_locked_async),
    .out (pll_locked_clk50)
  );

  assign reset_clk50 = ~pll_locked_clk50;

  synchronizer #(
    .WIDTH            (1),
    .STAGES           (2),
    .INITIAL_VAL      (1'b0),
    .FALSE_PATH_TO_IN (1)
  ) clk40_reset_sync (
    .clk (clk40),
    .rst (1'b0),
    .in  (pll_locked_async),
    .out (pll_locked_clk40)
  );

  assign reset_clk40 = ~pll_locked_clk40;

  //---------------------------------------------------------------------------
  // Power Supply Clock
  //---------------------------------------------------------------------------

`ifdef MFG_SUPPORT

  assign PWR_SUPPLY_CLK_CORE   = 1'b0;
  assign PWR_SUPPLY_CLK_DDR4_S = 1'b0;
  assign PWR_SUPPLY_CLK_DDR4_N = 1'b0;
  assign PWR_SUPPLY_CLK_0P9V   = 1'b0;
  assign PWR_SUPPLY_CLK_1P8V   = 1'b0;
  assign PWR_SUPPLY_CLK_2P5V   = 1'b0;
  assign PWR_SUPPLY_CLK_3P3V   = 1'b0;
  assign PWR_SUPPLY_CLK_3P6V   = 1'b0;

`else

  // Frequency definitions
  localparam SOUCE_CLOCK_FREQUENCY = 100_000_000;
  localparam TARGET_FREQUENCY_350k =     350_000;
  localparam TARGET_FREQUENCY_450k =     450_000;
  localparam TARGET_FREQUENCY_500k =     500_000;
  localparam TARGET_FREQUENCY_600k =     600_000;
  localparam TARGET_FREQUENCY_800k =     800_000;
  localparam TARGET_FREQUENCY_1M   =   1_000_000;

  pwr_supply_clk_gen #(
    .SOURCE_CLK_FREQ (SOUCE_CLOCK_FREQUENCY),
    .TARGET_CLK_FREQ (TARGET_FREQUENCY_350k)
  ) freq_gen_350k (
    .clk            (CLK_100),
    .rst            (power_on_reset_clk100),
    .pwr_supply_clk (PWR_SUPPLY_CLK_0P9V)
  );

  wire pwr_supply_clk_450k;
  pwr_supply_clk_gen #(
    .SOURCE_CLK_FREQ (SOUCE_CLOCK_FREQUENCY),
    .TARGET_CLK_FREQ (TARGET_FREQUENCY_450k)
  ) freq_gen_450k (
    .clk            (CLK_100),
    .rst            (power_on_reset_clk100),
    .pwr_supply_clk (pwr_supply_clk_450k)
  );

  assign PWR_SUPPLY_CLK_DDR4_S = pwr_supply_clk_450k;
  assign PWR_SUPPLY_CLK_DDR4_N = pwr_supply_clk_450k;

  pwr_supply_clk_gen #(
    .SOURCE_CLK_FREQ (SOUCE_CLOCK_FREQUENCY),
    .TARGET_CLK_FREQ (TARGET_FREQUENCY_500k)
  ) freq_gen_500k (
    .clk            (CLK_100),
    .rst            (power_on_reset_clk100),
    .pwr_supply_clk (PWR_SUPPLY_CLK_CORE)
  );

  pwr_supply_clk_gen #(
    .SOURCE_CLK_FREQ (SOUCE_CLOCK_FREQUENCY),
    .TARGET_CLK_FREQ (TARGET_FREQUENCY_600k)
  ) freq_gen_600k (
    .clk            (CLK_100),
    .rst            (power_on_reset_clk100),
    .pwr_supply_clk (PWR_SUPPLY_CLK_1P8V)
  );

  pwr_supply_clk_gen #(
    .SOURCE_CLK_FREQ (SOUCE_CLOCK_FREQUENCY),
    .TARGET_CLK_FREQ (TARGET_FREQUENCY_800k)
  ) freq_gen_800k (
    .clk            (CLK_100),
    .rst            (power_on_reset_clk100),
    .pwr_supply_clk (PWR_SUPPLY_CLK_2P5V)
  );

  wire pwr_supply_clk_1M;
  pwr_supply_clk_gen #(
    .SOURCE_CLK_FREQ (SOUCE_CLOCK_FREQUENCY),
    .TARGET_CLK_FREQ (TARGET_FREQUENCY_1M)
  ) freq_gen_1M (
    .clk            (CLK_100),
    .rst            (power_on_reset_clk100),
    .pwr_supply_clk (pwr_supply_clk_1M)
  );

  assign PWR_SUPPLY_CLK_3P3V = pwr_supply_clk_1M;
  assign PWR_SUPPLY_CLK_3P6V = pwr_supply_clk_1M;

`endif

  //---------------------------------------------------------------------------
  // PL Interfaces
  //---------------------------------------------------------------------------

  wire [1:0] db_clk_enable;
  wire [1:0] db_reset;
  wire [1:0] ipass_cable_present;

  // PL SPI FPGA -> DB CPLD
  reg  mb_cpld_sclk, mb_cpld_mosi, mb_cpld_cs_n;
  wire mb_cpld_miso;

  // PL SPI chip select decoding
  localparam PL_CS_MB_CPLD = 2'b00;
  localparam PL_CS_DB0     = 2'b10;
  localparam PL_CS_DB1     = 2'b01;
  localparam PL_CS_IDLE    = 2'b11;

  // PL SPI registers do not have a separate reset.
  // SW is expected to properly setup the DBs before issuing SPI transactions.
  always @(posedge pll_ref_clk_int) begin : to_db
    // Default chip selects
    mb_cpld_cs_n    <= 1'b1;

    // MB CPLD
    mb_cpld_sclk <= PL_CPLD_SCLK;
    mb_cpld_mosi <= PL_CPLD_MOSI;
    if (PL_CPLD_CS_N == PL_CS_MB_CPLD) begin
      mb_cpld_cs_n <= 1'b0;
    end
  end

  // SPI DB CPLD -> FPGA
  always @(posedge pll_ref_clk_int) begin : from_db
    case (PL_CPLD_CS_N)
      PL_CS_MB_CPLD : PL_CPLD_MISO <= mb_cpld_miso;    // MB CPLD
      PL_CS_IDLE    : PL_CPLD_MISO <= 1'bz;            // Inactive
    endcase
  end

  // Local PL SPI target
  wire [19:0] pl_ctrlport_req_addr;
  wire [31:0] pl_ctrlport_req_data;
  wire        pl_ctrlport_req_rd;
  wire        pl_ctrlport_req_wr;
  wire        pl_ctrlport_resp_ack;
  wire [31:0] pl_ctrlport_resp_data;
  wire [ 1:0] pl_ctrlport_resp_status;
  spi_slave_to_ctrlport_master #(
    .CLK_FREQUENCY (50_000_000),
    .SPI_FREQUENCY (10_666_667)
  ) pl_spi_endpoint (
    .ctrlport_clk           (clk50),
    .ctrlport_rst           (reset_clk50),
    .m_ctrlport_req_wr      (pl_ctrlport_req_wr),
    .m_ctrlport_req_rd      (pl_ctrlport_req_rd),
    .m_ctrlport_req_addr    (pl_ctrlport_req_addr),
    .m_ctrlport_req_data    (pl_ctrlport_req_data),
    .m_ctrlport_resp_ack    (pl_ctrlport_resp_ack),
    .m_ctrlport_resp_status (pl_ctrlport_resp_status),
    .m_ctrlport_resp_data   (pl_ctrlport_resp_data),
    .sclk                   (mb_cpld_sclk),
    .cs_n                   (mb_cpld_cs_n),
    .mosi                   (mb_cpld_mosi),
    .miso                   (mb_cpld_miso)
  );

  // Split up the PL control port
  wire [19:0] pl_regs_ctrlport_req_addr;
  wire [31:0] pl_regs_ctrlport_req_data;
  wire        pl_regs_ctrlport_req_rd;
  wire        pl_regs_ctrlport_req_wr;
  wire        pl_regs_ctrlport_resp_ack;
  wire [31:0] pl_regs_ctrlport_resp_data;
  wire [ 1:0] pl_regs_ctrlport_resp_status;

  wire [19:0] db0_led_ctrlport_req_addr;
  wire [31:0] db0_led_ctrlport_req_data;
  wire        db0_led_ctrlport_req_rd;
  wire        db0_led_ctrlport_req_wr;
  wire        db0_led_ctrlport_resp_ack;
  wire [31:0] db0_led_ctrlport_resp_data;
  wire [ 1:0] db0_led_ctrlport_resp_status;

  wire [19:0] db1_led_ctrlport_req_addr;
  wire [31:0] db1_led_ctrlport_req_data;
  wire        db1_led_ctrlport_req_rd;
  wire        db1_led_ctrlport_req_wr;
  wire        db1_led_ctrlport_resp_ack;
  wire [31:0] db1_led_ctrlport_resp_data;
  wire [ 1:0] db1_led_ctrlport_resp_status;

  wire [19:0] pl_term_ctrlport_req_addr;
  wire [31:0] pl_term_ctrlport_req_data;
  wire        pl_term_ctrlport_req_rd;
  wire        pl_term_ctrlport_req_wr;
  wire        pl_term_ctrlport_resp_ack;
  wire [31:0] pl_term_ctrlport_resp_data;
  wire [ 1:0] pl_term_ctrlport_resp_status;

  ctrlport_splitter #(
    .NUM_SLAVES (4)
  ) pl_ctrlport_splitter (
    .ctrlport_clk            (clk50),
    .ctrlport_rst            (reset_clk50),
    .s_ctrlport_req_wr       (pl_ctrlport_req_wr),
    .s_ctrlport_req_rd       (pl_ctrlport_req_rd),
    .s_ctrlport_req_addr     (pl_ctrlport_req_addr),
    .s_ctrlport_req_data     (pl_ctrlport_req_data),
    .s_ctrlport_req_byte_en  (),
    .s_ctrlport_req_has_time (),
    .s_ctrlport_req_time     (),
    .s_ctrlport_resp_ack     (pl_ctrlport_resp_ack),
    .s_ctrlport_resp_status  (pl_ctrlport_resp_status),
    .s_ctrlport_resp_data    (pl_ctrlport_resp_data),
    .m_ctrlport_req_wr       ({pl_regs_ctrlport_req_wr, db0_led_ctrlport_req_wr, db1_led_ctrlport_req_wr, pl_term_ctrlport_req_wr}),
    .m_ctrlport_req_rd       ({pl_regs_ctrlport_req_rd, db0_led_ctrlport_req_rd, db1_led_ctrlport_req_rd, pl_term_ctrlport_req_rd}),
    .m_ctrlport_req_addr     ({pl_regs_ctrlport_req_addr, db0_led_ctrlport_req_addr, db1_led_ctrlport_req_addr, pl_term_ctrlport_req_addr}),
    .m_ctrlport_req_data     ({pl_regs_ctrlport_req_data, db0_led_ctrlport_req_data, db1_led_ctrlport_req_data, pl_term_ctrlport_req_data}),
    .m_ctrlport_req_byte_en  (),
    .m_ctrlport_req_has_time (),
    .m_ctrlport_req_time     (),
    .m_ctrlport_resp_ack     ({pl_regs_ctrlport_resp_ack, db0_led_ctrlport_resp_ack, db1_led_ctrlport_resp_ack, pl_term_ctrlport_resp_ack}),
    .m_ctrlport_resp_status  ({pl_regs_ctrlport_resp_status, db0_led_ctrlport_resp_status, db1_led_ctrlport_resp_status, pl_term_ctrlport_resp_status}),
    .m_ctrlport_resp_data    ({pl_regs_ctrlport_resp_data, db0_led_ctrlport_resp_data, db1_led_ctrlport_resp_data, pl_term_ctrlport_resp_data})
  );

  pl_cpld_regs #(
    .BASE_ADDRESS (PL_REGISTERS)
  ) pl_regs (
    .ctrlport_clk           (clk50),
    .ctrlport_rst           (reset_clk50),
    .s_ctrlport_req_wr      (pl_regs_ctrlport_req_wr),
    .s_ctrlport_req_rd      (pl_regs_ctrlport_req_rd),
    .s_ctrlport_req_addr    (pl_regs_ctrlport_req_addr),
    .s_ctrlport_req_data    (pl_regs_ctrlport_req_data),
    .s_ctrlport_resp_ack    (pl_regs_ctrlport_resp_ack),
    .s_ctrlport_resp_status (pl_regs_ctrlport_resp_status),
    .s_ctrlport_resp_data   (pl_regs_ctrlport_resp_data),
    .qsfp0_led_active       (QSFP0_LED_ACTIVE),
    .qsfp0_led_link         (QSFP0_LED_LINK),
    .qsfp1_led_active       (QSFP1_LED_ACTIVE),
    .qsfp1_led_link         (QSFP1_LED_LINK),
    .ipass_cable_present    (ipass_cable_present)
  );

  led_control #(
    .BASE_ADDRESS  (PL_DB0_LED_REGISTERS),
    .REGMAP_SIZE   (PL_DB0_LED_REGISTERS_SIZE)
  ) led_control_db0 (
    .ctrlport_clk            (clk50),
    .ctrlport_rst            (reset_clk50),
    .s_ctrlport_req_wr       (db0_led_ctrlport_req_wr),
    .s_ctrlport_req_rd       (db0_led_ctrlport_req_rd),
    .s_ctrlport_req_addr     (db0_led_ctrlport_req_addr),
    .s_ctrlport_req_data     (db0_led_ctrlport_req_data),
    .s_ctrlport_resp_ack     (db0_led_ctrlport_resp_ack),
    .s_ctrlport_resp_status  (db0_led_ctrlport_resp_status),
    .s_ctrlport_resp_data    (db0_led_ctrlport_resp_data),
    .ch0_rx2_led             (CH0_RX2_LED[0]),
    .ch0_tx_led              (CH0_TX_LED[0]),
    .ch0_rx_led              (CH0_RX_LED[0]),
    .ch1_rx2_led             (CH1_RX2_LED[0]),
    .ch1_tx_led              (CH1_TX_LED[0]),
    .ch1_rx_led              (CH1_RX_LED[0]),
    .ch2_rx2_led             (CH2_RX2_LED[0]),
    .ch2_tx_led              (CH2_TX_LED[0]),
    .ch2_rx_led              (CH2_RX_LED[0]),
    .ch3_rx2_led             (CH3_RX2_LED[0]),
    .ch3_tx_led              (CH3_TX_LED[0]),
    .ch3_rx_led              (CH3_RX_LED[0])
  );

  led_control #(
    .BASE_ADDRESS  (PL_DB1_LED_REGISTERS),
    .REGMAP_SIZE   (PL_DB1_LED_REGISTERS_SIZE)
  ) led_control_db1 (
    .ctrlport_clk            (clk50),
    .ctrlport_rst            (reset_clk50),
    .s_ctrlport_req_wr       (db1_led_ctrlport_req_wr),
    .s_ctrlport_req_rd       (db1_led_ctrlport_req_rd),
    .s_ctrlport_req_addr     (db1_led_ctrlport_req_addr),
    .s_ctrlport_req_data     (db1_led_ctrlport_req_data),
    .s_ctrlport_resp_ack     (db1_led_ctrlport_resp_ack),
    .s_ctrlport_resp_status  (db1_led_ctrlport_resp_status),
    .s_ctrlport_resp_data    (db1_led_ctrlport_resp_data),
    .ch0_rx2_led             (CH0_RX2_LED[1]),
    .ch0_tx_led              (CH0_TX_LED[1]),
    .ch0_rx_led              (CH0_RX_LED[1]),
    .ch1_rx2_led             (CH1_RX2_LED[1]),
    .ch1_tx_led              (CH1_TX_LED[1]),
    .ch1_rx_led              (CH1_RX_LED[1]),
    .ch2_rx2_led             (CH2_RX2_LED[1]),
    .ch2_tx_led              (CH2_TX_LED[1]),
    .ch2_rx_led              (CH2_RX_LED[1]),
    .ch3_rx2_led             (CH3_RX2_LED[1]),
    .ch3_tx_led              (CH3_TX_LED[1]),
    .ch3_rx_led              (CH3_RX_LED[1])
  );

  // Termination of ctrlport request
  ctrlport_terminator #(
    .START_ADDRESS (PL_DB1_LED_REGISTERS + PL_DB1_LED_REGISTERS_SIZE),
    .LAST_ADDRESS  (2**CTRLPORT_ADDR_W-1)
  ) pl_terminator (
    .ctrlport_clk           (clk50),
    .ctrlport_rst           (reset_clk50),
    .s_ctrlport_req_wr      (pl_term_ctrlport_req_wr),
    .s_ctrlport_req_rd      (pl_term_ctrlport_req_rd),
    .s_ctrlport_req_addr    (pl_term_ctrlport_req_addr),
    .s_ctrlport_req_data    (pl_term_ctrlport_req_data),
    .s_ctrlport_resp_ack    (pl_term_ctrlport_resp_ack),
    .s_ctrlport_resp_status (pl_term_ctrlport_resp_status),
    .s_ctrlport_resp_data   (pl_term_ctrlport_resp_data)
  );

  //---------------------------------------------------------------------------
  // PS Interfaces
  //---------------------------------------------------------------------------

  // Local PS SPI target
  wire [19:0] ps_ctrlport_req_addr;
  wire [31:0] ps_ctrlport_req_data;
  wire        ps_ctrlport_req_rd;
  wire        ps_ctrlport_req_wr;
  wire        ps_ctrlport_resp_ack;
  wire [31:0] ps_ctrlport_resp_data;
  wire [ 1:0] ps_ctrlport_resp_status;

  wire ps_spi_endpoint_sclk;
  wire ps_spi_endpoint_mosi;
  wire ps_spi_endpoint_miso;
  wire ps_spi_endpoint_cs_n;
  spi_slave_to_ctrlport_master #(
    .CLK_FREQUENCY (50_000_000),
    .SPI_FREQUENCY (5_000_000)
  ) ps_spi_endpoint (
    .ctrlport_clk           (clk50),
    .ctrlport_rst           (reset_clk50),
    .m_ctrlport_req_wr      (ps_ctrlport_req_wr),
    .m_ctrlport_req_rd      (ps_ctrlport_req_rd),
    .m_ctrlport_req_addr    (ps_ctrlport_req_addr),
    .m_ctrlport_req_data    (ps_ctrlport_req_data),
    .m_ctrlport_resp_ack    (ps_ctrlport_resp_ack),
    .m_ctrlport_resp_status (ps_ctrlport_resp_status),
    .m_ctrlport_resp_data   (ps_ctrlport_resp_data),
    .sclk                   (ps_spi_endpoint_sclk),
    .cs_n                   (ps_spi_endpoint_cs_n),
    .mosi                   (ps_spi_endpoint_mosi),
    .miso                   (ps_spi_endpoint_miso)
  );

  // The PS SPI chip select signals are binary encoded.
  //
  // The internal SPI slaves as well as external slaves like the LMK04832
  // trigger actions or resets based on edges of the chip select signal.
  // Therefore this implementation has to avoid glitches on the chip select
  // signal although the SPI protocol is synchronous.
  //
  // The chip signals are double synchronized to make sure there is no
  // meta-stability. Due to different traces lengths there is no guarantee for
  // the chip select signals to change at the same time. To overcome this issue
  // register stage 2 and 3 are compared. Only in case of matching values the
  // change is propagated to the slaves' chip select lines. Once the IDLE state
  // (all ones) is detected in register stage 2 the slaves' chip select lines
  // will be deasserted.

  // Input sync registers (3 stages)
  wire [3:0] ps_cpld_cs_n_shift2;              // Resolving meta-stability, reset on IDLE
  reg  [3:0] ps_cpld_cs_n_shift3 = {4 {1'b1}}; // Stable state detection
  synchronizer #(
    .WIDTH            (4),
    .STAGES           (2),
    .INITIAL_VAL      (4'b1111),
    .FALSE_PATH_TO_IN (0)
  ) ps_spi_input_sync_inst (
    .clk (clk250),
    .rst (1'b0),
    .in  (PS_CPLD_CS_N),
    .out (ps_cpld_cs_n_shift2)
  );
  always @(posedge clk250) begin
    ps_cpld_cs_n_shift3 <= ps_cpld_cs_n_shift2;
  end

  // SPI binary decoding
  reg [SPI_ENDPOINT_SIZE-2:0] ps_spi_cs_n_decoded = {SPI_ENDPOINT_SIZE-1 {1'b1}};
  always @(posedge clk250) begin
    // reset in case of IDLE state
    if (ps_cpld_cs_n_shift2[2:0] == PS_CS_IDLE) begin
      ps_spi_cs_n_decoded <= {SPI_ENDPOINT_SIZE-1 {1'b1}};
    // only apply changes when stable state is detected
    end else if (ps_cpld_cs_n_shift3[2:0] == ps_cpld_cs_n_shift2[2:0]) begin
      ps_spi_cs_n_decoded[PS_CS_MB_CPLD]        <= ps_cpld_cs_n_shift3[2:0] != PS_CS_MB_CPLD;
      ps_spi_cs_n_decoded[PS_CS_LMK32]          <= ps_cpld_cs_n_shift3[2:0] != PS_CS_LMK32;
      ps_spi_cs_n_decoded[PS_CS_TPM]            <= ps_cpld_cs_n_shift3[2:0] != PS_CS_TPM;
      ps_spi_cs_n_decoded[PS_CS_PHASE_DAC]      <= ps_cpld_cs_n_shift3[2:0] != PS_CS_PHASE_DAC;
      ps_spi_cs_n_decoded[PS_CS_DB0_CAL_EEPROM] <= ps_cpld_cs_n_shift3[2:0] != PS_CS_DB0_CAL_EEPROM;
      ps_spi_cs_n_decoded[PS_CS_DB1_CAL_EEPROM] <= ps_cpld_cs_n_shift3[2:0] != PS_CS_DB1_CAL_EEPROM;
      ps_spi_cs_n_decoded[PS_CS_CLK_AUX_DB]     <= ps_cpld_cs_n_shift3[2:0] != PS_CS_CLK_AUX_DB;
    end
  end

  // Local SPI slave
  assign ps_spi_endpoint_sclk = PS_CPLD_SCLK;
  assign ps_spi_endpoint_mosi = PS_CPLD_MOSI;
  assign ps_spi_endpoint_cs_n = ps_spi_cs_n_decoded[PS_CS_MB_CPLD];

  // LMK04832 SPI signals
  assign LMK32_SCLK = PS_CPLD_SCLK;
  assign LMK32_MOSI = PS_CPLD_MOSI;
  assign LMK32_CS_N = ps_spi_cs_n_decoded[PS_CS_LMK32];

  // TPM SPI signals
  // Note: TPM is not currently supported
  assign TPM_SCLK = PS_CPLD_SCLK;
  assign TPM_MOSI = PS_CPLD_MOSI;
  assign TPM_CS_N = ps_spi_cs_n_decoded[PS_CS_TPM];

  // Phase DAC SPI signals
  assign PHASE_DAC_SCLK = PS_CPLD_SCLK;
  assign PHASE_DAC_MOSI = PS_CPLD_MOSI;
  assign PHASE_DAC_CS_N = ps_spi_cs_n_decoded[PS_CS_PHASE_DAC];

  // CLK AUX DB SPI signals
  assign CLK_DB_SCLK = PS_CPLD_SCLK;
  assign CLK_DB_MOSI = PS_CPLD_MOSI;
  assign CLK_DB_CS_N = ps_spi_cs_n_decoded[PS_CS_CLK_AUX_DB];

  // Combine SPI responses based on inputs only as this path is captured
  // synchronously to PS_CPLD_SCLK by the SPI master.
  assign PS_CPLD_MISO = (PS_CPLD_CS_N[2:0] == PS_CS_MB_CPLD)        ? ps_spi_endpoint_miso :
                        (PS_CPLD_CS_N[2:0] == PS_CS_LMK32)          ? LMK32_MISO           :
                        (PS_CPLD_CS_N[2:0] == PS_CS_TPM)            ? TPM_MISO             :
                        (PS_CPLD_CS_N[2:0] == PS_CS_CLK_AUX_DB)     ? CLK_DB_MISO          :
                        1'bz; // Default case and PHASE_DAC

  // Split up the PS control port
  wire [19:0] ps_regs_ctrlport_req_addr;
  wire [31:0] ps_regs_ctrlport_req_data;
  wire        ps_regs_ctrlport_req_rd;
  wire        ps_regs_ctrlport_req_wr;
  wire        ps_regs_ctrlport_resp_ack;
  wire [31:0] ps_regs_ctrlport_resp_data;
  wire [ 1:0] ps_regs_ctrlport_resp_status;

  wire [19:0] ps_term_ctrlport_req_addr;
  wire [31:0] ps_term_ctrlport_req_data;
  wire        ps_term_ctrlport_req_rd;
  wire        ps_term_ctrlport_req_wr;
  wire        ps_term_ctrlport_resp_ack;
  wire [31:0] ps_term_ctrlport_resp_data;
  wire [ 1:0] ps_term_ctrlport_resp_status;

  wire [19:0] ps_reconfig_ctrlport_req_addr;
  wire [31:0] ps_reconfig_ctrlport_req_data;
  wire        ps_reconfig_ctrlport_req_rd;
  wire        ps_reconfig_ctrlport_req_wr;
  wire        ps_reconfig_ctrlport_resp_ack;
  wire [31:0] ps_reconfig_ctrlport_resp_data;
  wire [ 1:0] ps_reconfig_ctrlport_resp_status;

  wire [19:0] ps_power_ctrlport_req_addr;
  wire [31:0] ps_power_ctrlport_req_data;
  wire        ps_power_ctrlport_req_rd;
  wire        ps_power_ctrlport_req_wr;
  wire        ps_power_ctrlport_resp_ack;
  wire [31:0] ps_power_ctrlport_resp_data;
  wire [ 1:0] ps_power_ctrlport_resp_status;

  ctrlport_splitter #(
    .NUM_SLAVES (4)
  ) ps_ctrlport_splitter (
    .ctrlport_clk            (clk50),
    .ctrlport_rst            (reset_clk50),
    .s_ctrlport_req_wr       (ps_ctrlport_req_wr),
    .s_ctrlport_req_rd       (ps_ctrlport_req_rd),
    .s_ctrlport_req_addr     (ps_ctrlport_req_addr),
    .s_ctrlport_req_data     (ps_ctrlport_req_data),
    .s_ctrlport_req_byte_en  (),
    .s_ctrlport_req_has_time (),
    .s_ctrlport_req_time     (),
    .s_ctrlport_resp_ack     (ps_ctrlport_resp_ack),
    .s_ctrlport_resp_status  (ps_ctrlport_resp_status),
    .s_ctrlport_resp_data    (ps_ctrlport_resp_data),
    .m_ctrlport_req_wr       ({ps_power_ctrlport_req_wr,   ps_regs_ctrlport_req_wr,   ps_term_ctrlport_req_wr,   ps_reconfig_ctrlport_req_wr}),
    .m_ctrlport_req_rd       ({ps_power_ctrlport_req_rd,   ps_regs_ctrlport_req_rd,   ps_term_ctrlport_req_rd,   ps_reconfig_ctrlport_req_rd}),
    .m_ctrlport_req_addr     ({ps_power_ctrlport_req_addr, ps_regs_ctrlport_req_addr, ps_term_ctrlport_req_addr, ps_reconfig_ctrlport_req_addr}),
    .m_ctrlport_req_data     ({ps_power_ctrlport_req_data, ps_regs_ctrlport_req_data, ps_term_ctrlport_req_data, ps_reconfig_ctrlport_req_data}),
    .m_ctrlport_req_byte_en  (),
    .m_ctrlport_req_has_time (),
    .m_ctrlport_req_time     (),
    .m_ctrlport_resp_ack     ({ps_power_ctrlport_resp_ack,    ps_regs_ctrlport_resp_ack,    ps_term_ctrlport_resp_ack,    ps_reconfig_ctrlport_resp_ack}),
    .m_ctrlport_resp_status  ({ps_power_ctrlport_resp_status, ps_regs_ctrlport_resp_status, ps_term_ctrlport_resp_status, ps_reconfig_ctrlport_resp_status}),
    .m_ctrlport_resp_data    ({ps_power_ctrlport_resp_data,   ps_regs_ctrlport_resp_data,   ps_term_ctrlport_resp_data,   ps_reconfig_ctrlport_resp_data})
  );

  wire [39:0] serial_num_clk50;
  wire        cmi_ready_clk50;
  wire        cmi_other_side_detected_clk50;
  ps_cpld_regs #(
    .BASE_ADDRESS (PS_REGISTERS)
  ) ps_regs (
    .ctrlport_clk            (clk50),
    .ctrlport_rst            (reset_clk50),
    .s_ctrlport_req_wr       (ps_regs_ctrlport_req_wr),
    .s_ctrlport_req_rd       (ps_regs_ctrlport_req_rd),
    .s_ctrlport_req_addr     (ps_regs_ctrlport_req_addr),
    .s_ctrlport_req_data     (ps_regs_ctrlport_req_data),
    .s_ctrlport_resp_ack     (ps_regs_ctrlport_resp_ack),
    .s_ctrlport_resp_status  (ps_regs_ctrlport_resp_status),
    .s_ctrlport_resp_data    (ps_regs_ctrlport_resp_data),
    .db_clk_enable           (db_clk_enable),
    .db_reset                (db_reset),
    .pll_ref_clk_enable      (pll_ref_clk_en_clk50),
    .dio_direction_a         (DIO_DIRECTION_A),
    .dio_direction_b         (DIO_DIRECTION_B),
    .serial_num              (serial_num_clk50),
    .cmi_ready               (cmi_ready_clk50),
    .cmi_other_side_detected (cmi_other_side_detected_clk50)
  );

  ps_power_regs #(
    .BASE_ADDRESS  (POWER_REGISTERS),
    .NUM_ADDRESSES (POWER_REGISTERS_SIZE)
  ) ps_power_regs_inst (
    .ctrlport_clk           (clk50),
    .ctrlport_rst           (reset_clk50),
    .s_ctrlport_req_wr      (ps_power_ctrlport_req_wr),
    .s_ctrlport_req_rd      (ps_power_ctrlport_req_rd),
    .s_ctrlport_req_addr    (ps_power_ctrlport_req_addr),
    .s_ctrlport_req_data    (ps_power_ctrlport_req_data),
    .s_ctrlport_resp_ack    (ps_power_ctrlport_resp_ack),
    .s_ctrlport_resp_status (ps_power_ctrlport_resp_status),
    .s_ctrlport_resp_data   (ps_power_ctrlport_resp_data),
    .ipass_power_disable    (IPASS_POWER_DISABLE),
    .ipass_power_fault_n    (IPASS_POWER_EN_FAULT),
    .osc_100_en             (PWR_EN_5V_OSC_100),
    .osc_122_88_en          (PWR_EN_5V_OSC_122_88)
  );

  // Termination of ctrlport request
  ctrlport_terminator #(
    .START_ADDRESS (POWER_REGISTERS + POWER_REGISTERS_SIZE),
    .LAST_ADDRESS  (2**CTRLPORT_ADDR_W-1)
  ) ps_terminator (
    .ctrlport_clk           (clk50),
    .ctrlport_rst           (reset_clk50),
    .s_ctrlport_req_wr      (ps_term_ctrlport_req_wr),
    .s_ctrlport_req_rd      (ps_term_ctrlport_req_rd),
    .s_ctrlport_req_addr    (ps_term_ctrlport_req_addr),
    .s_ctrlport_req_data    (ps_term_ctrlport_req_data),
    .s_ctrlport_resp_ack    (ps_term_ctrlport_resp_ack),
    .s_ctrlport_resp_status (ps_term_ctrlport_resp_status),
    .s_ctrlport_resp_data   (ps_term_ctrlport_resp_data)
  );


  //---------------------------------------------------------------------------
  // Reconfiguration
  //---------------------------------------------------------------------------
  // On-chip flash interface
  //
  // Naming is according to Avalon Memory-Mapped Interfaces:
  // https://www.intel.com/content/dam/www/programmable/us/en/pdfs/literature/manual/mnl_avalon_spec.pdf

  wire        csr_addr;
  wire        csr_read;
  wire [31:0] csr_readdata;
  wire        csr_write;
  wire [31:0] csr_writedata;
  wire [16:0] data_addr;
  wire        data_read;
  wire [31:0] data_readdata;
  wire        data_readdatavalid;
  wire        data_waitrequest;
  wire        data_write;
  wire [31:0] data_writedata;
  wire        reset_clk50_n;

  assign reset_clk50_n = ~reset_clk50;
  on_chip_flash flash_inst (
    .clock                   (clk50),
    .avmm_csr_addr           (csr_addr),
    .avmm_csr_read           (csr_read),
    .avmm_csr_writedata      (csr_writedata),
    .avmm_csr_write          (csr_write),
    .avmm_csr_readdata       (csr_readdata),
    .avmm_data_addr          (data_addr),
    .avmm_data_read          (data_read),
    .avmm_data_writedata     (data_writedata),
    .avmm_data_write         (data_write),
    .avmm_data_readdata      (data_readdata),
    .avmm_data_waitrequest   (data_waitrequest),
    .avmm_data_readdatavalid (data_readdatavalid),
    .avmm_data_burstcount    (4'b0001),
    .reset_n                 (reset_clk50_n)
  );

  reconfig_engine #(
    .BASE_ADDRESS  (RECONFIG),
    .NUM_ADDRESSES (RECONFIG_SIZE),
    .MEM_INIT      (0)
  ) reconfig_engine_inst (
    .ctrlport_clk           (clk50),
    .ctrlport_rst           (reset_clk50),
    .s_ctrlport_req_wr      (ps_reconfig_ctrlport_req_wr),
    .s_ctrlport_req_rd      (ps_reconfig_ctrlport_req_rd),
    .s_ctrlport_req_addr    (ps_reconfig_ctrlport_req_addr),
    .s_ctrlport_req_data    (ps_reconfig_ctrlport_req_data),
    .s_ctrlport_resp_ack    (ps_reconfig_ctrlport_resp_ack),
    .s_ctrlport_resp_status (ps_reconfig_ctrlport_resp_status),
    .s_ctrlport_resp_data   (ps_reconfig_ctrlport_resp_data),
    .csr_addr               (csr_addr),
    .csr_read               (csr_read),
    .csr_writedata          (csr_writedata),
    .csr_write              (csr_write),
    .csr_readdata           (csr_readdata),
    .data_addr              (data_addr),
    .data_read              (data_read),
    .data_writedata         (data_writedata),
    .data_write             (data_write),
    .data_readdata          (data_readdata),
    .data_waitrequest       (data_waitrequest),
    .data_readdatavalid     (data_readdatavalid)
  );

  //---------------------------------------------------------------------------
  // CMI Interface
  //---------------------------------------------------------------------------

  // Control and status information clock transition
  wire [39:0] serial_num_clk40;
  wire        cmi_ready_clk40;
  wire        cmi_other_side_detected_clk40;

  handshake #(
    .WIDTH (41)
  ) cmi_control_hs (
    .clk_a   (clk50),
    .rst_a   (reset_clk50),
    .valid_a (1'b1),
    .data_a  ({cmi_ready_clk50, serial_num_clk50}),
    .busy_a  (),
    .clk_b   (clk40),
    .valid_b (),
    .data_b  ({cmi_ready_clk40, serial_num_clk40})
  );

  synchronizer #(
    .WIDTH            (1),
    .STAGES           (2),
    .INITIAL_VAL      (1'b0),
    .FALSE_PATH_TO_IN (1)
  ) cmi_status_sync (
    .clk (clk50),
    .rst (reset_clk50),
    .in  (cmi_other_side_detected_clk40),
    .out (cmi_other_side_detected_clk50)
  );

  wire       scl_out;
  wire       sda_out;
  wire [1:0] ipass_cable_present_n = ~ipass_cable_present;

  PcieCmiWrapper #(
    .kSimulation (SIMULATION)
  ) pcie_cmi_inst (
    .Clk                (clk40),
    .acReset            (reset_clk40),
    .cSerialNumber      (serial_num_clk40),
    .cBoardIsReady      (cmi_ready_clk40),
    .cCmiReset          (PCIE_RESET),
    .cOtherSideDetected (cmi_other_side_detected_clk40),
    .aCblPrsnt_n        (ipass_cable_present_n[0]),
    .aSdaIn             (IPASS_SDA[0]),
    .aSdaOut            (sda_out),
    .aSclIn             (IPASS_SCL[0]),
    .aSclOut            (scl_out)
  );

  // External pull-ups are used to drive the signal high
  assign IPASS_SDA[0] = sda_out ? 1'bz : 1'b0;
  assign IPASS_SCL[0] = scl_out ? 1'bz : 1'b0;

  // No CMI controller for second interface
  assign IPASS_SCL[1] = 1'bz;
  assign IPASS_SDA[1] = 1'bz;

  //---------------------------------------------------------------------------
  // Miscellaneous
  //---------------------------------------------------------------------------

  // Constants
  assign PS_CLK_ON_CPLD = 1'b0; // Active-low driving of PS clocks
  assign TPM_RESET_n = 1'b1;

  // Currently unused ports
  assign PL_CPLD_IRQ = 1'b0;

endmodule


`default_nettype wire


//XmlParse xml_on
//<top name="X440_MB_CPLD">
//  <regmapcfg readablestrobes="false">
//    <map name="MB_CPLD_PS_REGMAP"/>
//    <map name="MB_CPLD_PL_REGMAP"/>
//  </regmapcfg>
//</top>
//<regmap name="MB_CPLD_PS_REGMAP" readablestrobes="false" markdown="true" generatevhdl="true" ettusguidelines="true">
//  <info>
//    This register map is available using the PS CPLD SPI interface.
//  </info>
//  <group name="MB_CPLD_PS_WINDOWS">
//    <window name="PS_REGISTERS"    offset="0x00" size="0x40" targetregmap="PS_CPLD_BASE_REGMAP"/>
//    <window name="RECONFIG"        offset="0x40" size="0x20" targetregmap="RECONFIG_REGMAP"/>
//    <window name="POWER_REGISTERS" offset="0x60" size="0x20" targetregmap="PS_POWER_REGMAP"/>
//  </group>
//  <group name="PS_SPI_ENDPOINTS">
//    <enumeratedtype name="SPI_ENDPOINT">
//      <value name="PS_CS_MB_CPLD"        integer="0"/>
//      <value name="PS_CS_LMK32"          integer="1"/>
//      <value name="PS_CS_TPM"            integer="2"/>
//      <value name="PS_CS_PHASE_DAC"      integer="3"/>
//      <value name="PS_CS_DB0_CAL_EEPROM" integer="4"/>
//      <value name="PS_CS_DB1_CAL_EEPROM" integer="5"/>
//      <value name="PS_CS_CLK_AUX_DB"     integer="6"/>
//      <value name="PS_CS_IDLE"           integer="7"/>
//    </enumeratedtype>
//  </group>
//</regmap>
//<regmap name="MB_CPLD_PL_REGMAP" readablestrobes="false" markdown="true" generatevhdl="true" ettusguidelines="true">
//  <info>
//    This register map is available using the PL CPLD SPI interface.
//    All protocol masters controller by this register map are running with a clock frequency of 50 MHz.
//  </info>
//  <group name="MB_CPLD_PL_WINDOWS">
//    <window name="PL_REGISTERS"         offset="0x0"  size="0x40" targetregmap="PL_CPLD_BASE_REGMAP"/>
//    <window name="PL_DB0_LED_REGISTERS" offset="0x50" size="0x10" targetregmap="LED_SETUP_REGMAP"/>
//    <window name="PL_DB1_LED_REGISTERS" offset="0x60" size="0x10" targetregmap="LED_SETUP_REGMAP"/>
//  </group>
//</regmap>
//<regmap name="CONSTANTS_REGMAP" readablestrobes="false" generatevhdl="true" ettusguidelines="true">
//  <group name="CONSTANTS_GROUP">
//    <info>
//      Basic registers containing version and capabilities information.
//    </info>

//    <enumeratedtype name="CONSTANTS_ENUM" showhexvalue="true">
//      <info>
//        This enumeration is used to create the constants held in the basic registers.
//      </info>
//      <value name="PS_CPLD_SIGNATURE"     integer="0x0A522D28"/>
//      <value name="PL_CPLD_SIGNATURE"     integer="0x3FDC5C47"/>
//      <value name="CPLD_REVISION"         integer="0x22080414"/>
//      <value name="OLDEST_CPLD_REVISION"  integer="0x22080414"/>
//    </enumeratedtype>
//  </group>
//</regmap>
//XmlParse xml_off

//
// Copyright 2021 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: zbx_top_cpld
//
// Description:
//
//   Top level file for the ZBX CPLD, enabling multi-purpose control around the
//   ZBX board. Main blocks are contained within zbx_cpld_core.
//

`default_nettype none

module zbx_top_cpld (
  //LO Clocking
  input  wire CPLD_REFCLK,
  input  wire MB_SYNTH_SYNC,

  //Clk and reset for control registers
`ifndef VARIANT_XO3
  input  wire CTRL_REG_CLK,
`endif
  input  wire CTRL_REG_ARST,

  //SPI in
  input  wire MB_CTRL_SCK,
  input  wire MB_CTRL_MOSI,
  output wire MB_CTRL_MISO,
  input  wire MB_CTRL_CS,

  //MB FPGA GPIO
  //net name mapping in decreasing order
  // MB_FPGA_GPIO_a14
  // MB_FPGA_GPIO_a16
  // MB_FPGA_GPIO_a17
  // MB_FPGA_GPIO_b8
  // MB_FPGA_GPIO_b9
  // MB_FPGA_GPIO_b11
  // MB_FPGA_GPIO_b12
  // MB_FPGA_GPIO_b14
  // MB_FPGA_GPIO_b15
  // MB_FPGA_GPIO_b17
  // MB_FPGA_GPIO_c15
  // MB_FPGA_GPIO_c16
  // MB_FPGA_GPIO_c18
  // MB_FPGA_GPIO_c19
  inout  wire [13:0] MB_FPGA_GPIO,

  /////////////////////////////////////////////////////////////////////////////
  // Rf Power Controls
  /////////////////////////////////////////////////////////////////////////////

  output wire P7V_ENABLE_A,
  output wire P7V_ENABLE_B,
  output wire P3D3VA_ENABLE,

  input  wire P7V_PG_A,
  input  wire P7V_PG_B,

  /////////////////////////////////////////////////////////////////////////////
  // TX0 Controls
  /////////////////////////////////////////////////////////////////////////////

  //Tx0 LO control for LMX2572
  output wire TX0_LO1_SYNC,
  input  wire TX0_LO1_MUXOUT,
  output wire TX0_LO1_CSB,
  output wire TX0_LO1_SCK,

  output wire TX0_LO2_SYNC,
  input  wire TX0_LO2_MUXOUT,
  output wire TX0_LO2_CSB,
  output wire TX0_LO2_SCK,

  // XO3 variant of this programmable drives all SDI lines from a single pin.
`ifndef VARIANT_XO3
  output wire TX0_LO1_SDI,
  output wire TX0_LO2_SDI,
`else
  output wire RX_TX_LO_SDI,
`endif

  //Tx0 Switch control
  output wire TX0_SW1_SW2_CTRL,
  output wire TX0_SW3_A,
  output wire TX0_SW3_B,
  output wire TX0_SW4_A,
  output wire TX0_SW4_B,
  output wire TX0_SW5_A,
  output wire TX0_SW5_B,
  output wire TX0_SW6_A,
  output wire TX0_SW6_B,
  output wire TX0_SW7_A,
  output wire TX0_SW7_B,
  output wire TX0_SW8_V1,
  output wire TX0_SW8_V2,
  output wire TX0_SW8_V3,
  output wire TX0_SW9_A,
  output wire TX0_SW9_B,
  output wire TX0_SW10_A,
  output wire TX0_SW10_B,
  output wire TX0_SW11_A,
  output wire TX0_SW11_B,
  output wire TX0_SW13_V1,
  output wire TX0_SW14_V1,

  //Tx0 DSA control
  output wire [6:2] TX0_DSA1,
  output wire [6:2] TX0_DSA2,

  /////////////////////////////////////////////////////////////////////////////
  // TX1 Controls
  /////////////////////////////////////////////////////////////////////////////

  //Tx1 LO control for LMX2572
  output wire TX1_LO1_SYNC,
  input  wire TX1_LO1_MUXOUT,
  output wire TX1_LO1_CSB,
  output wire TX1_LO1_SCK,

  output wire TX1_LO2_SYNC,
  input  wire TX1_LO2_MUXOUT,
  output wire TX1_LO2_CSB,
  output wire TX1_LO2_SCK,

`ifndef VARIANT_XO3
  output wire TX1_LO1_SDI,
  output wire TX1_LO2_SDI,
`endif

  //Tx1 Switch control
  output wire TX1_SW1_SW2_CTRL,
  output wire TX1_SW3_A,
  output wire TX1_SW3_B,
  output wire TX1_SW4_A,
  output wire TX1_SW4_B,
  output wire TX1_SW5_A,
  output wire TX1_SW5_B,
  output wire TX1_SW6_A,
  output wire TX1_SW6_B,
  output wire TX1_SW7_A,
  output wire TX1_SW7_B,
  output wire TX1_SW8_V1,
  output wire TX1_SW8_V2,
  output wire TX1_SW8_V3,
  output wire TX1_SW9_A,
  output wire TX1_SW9_B,
  output wire TX1_SW10_A,
  output wire TX1_SW10_B,
  output wire TX1_SW11_A,
  output wire TX1_SW11_B,
  output wire TX1_SW13_V1,
  output wire TX1_SW14_V1,

  //Tx1 DSA control
  output wire [6:2] TX1_DSA1,
  output wire [6:2] TX1_DSA2,

  /////////////////////////////////////////////////////////////////////////////
  // RX0 Controls
  /////////////////////////////////////////////////////////////////////////////

  //Rx0 LO control for LMX2572
  output wire RX0_LO1_SYNC,
  input  wire RX0_LO1_MUXOUT,
  output wire RX0_LO1_CSB,
  output wire RX0_LO1_SCK,

  output wire RX0_LO2_SYNC,
  input  wire RX0_LO2_MUXOUT,
  output wire RX0_LO2_CSB,
  output wire RX0_LO2_SCK,

`ifndef VARIANT_XO3
  output wire RX0_LO1_SDI,
  output wire RX0_LO2_SDI,
`endif

  //Rx0 Switch control
  output wire RX0_SW1_A,
  output wire RX0_SW1_B,
  output wire RX0_SW2_A,
  output wire RX0_SW3_V1,
  output wire RX0_SW3_V2,
  output wire RX0_SW3_V3,
  output wire RX0_SW4_A,
  output wire RX0_SW5_A,
  output wire RX0_SW5_B,
  output wire RX0_SW6_A,
  output wire RX0_SW6_B,
  output wire RX0_SW7_SW8_CTRL,
  output wire RX0_SW9_V1,
  output wire RX0_SW10_V1,
  output wire RX0_SW11_V3,
  output wire RX0_SW11_V2,
  output wire RX0_SW11_V1,

  //Rx0 DSA control
  output wire [1:4] RX0_DSA1_n,
  output wire [1:4] RX0_DSA2_n,
  output wire [1:4] RX0_DSA3_A_n,
  output wire [1:4] RX0_DSA3_B_n,

  /////////////////////////////////////////////////////////////////////////////
  // RX1 Controls
  /////////////////////////////////////////////////////////////////////////////

  //Rx1 LO control for LMX2572
  output wire RX1_LO1_SYNC,
  input  wire RX1_LO1_MUXOUT,
  output wire RX1_LO1_CSB,
  output wire RX1_LO1_SCK,

  output wire RX1_LO2_SYNC,
  input  wire RX1_LO2_MUXOUT,
  output wire RX1_LO2_CSB,
  output wire RX1_LO2_SCK,

`ifndef VARIANT_XO3
  output wire RX1_LO1_SDI,
  output wire RX1_LO2_SDI,
`endif

  //Rx1 Switch Control
  output wire RX1_SW1_A,
  output wire RX1_SW1_B,
  output wire RX1_SW2_A,
  output wire RX1_SW3_V1,
  output wire RX1_SW3_V2,
  output wire RX1_SW3_V3,
  output wire RX1_SW4_A,
  output wire RX1_SW5_A,
  output wire RX1_SW5_B,
  output wire RX1_SW6_A,
  output wire RX1_SW6_B,
  output wire RX1_SW7_SW8_CTRL,
  output wire RX1_SW9_V1,
  output wire RX1_SW10_V1,
  output wire RX1_SW11_V3,
  output wire RX1_SW11_V2,
  output wire RX1_SW11_V1,

  //Rx1 DSA control
  output wire [1:4] RX1_DSA1_n,
  output wire [1:4] RX1_DSA2_n,
  output wire [1:4] RX1_DSA3_A_n,
  output wire [1:4] RX1_DSA3_B_n,

  /////////////////////////////////////////////////////////////////////////////
  // LED Control
  /////////////////////////////////////////////////////////////////////////////

  output wire CH0_RX2_LED,
  output wire CH0_TX_LED,
  output wire CH0_RX_LED,
  output wire CH1_RX2_LED,
  output wire CH1_TX_LED,
  output wire CH1_RX_LED

);

  // SPI masters (LO and power detection) are limited to a maximum 24 bit
  // transmission length
  `define SPI_MAX_CHAR_32

  `include "../../../../../lib/rfnoc/core/ctrlport.vh"
  `include "regmap/db_control_regmap_utils.vh"
  `include "regmap/lo_control_regmap_utils.vh"
  `include "regmap/spi_regmap_utils.vh"
  `include "regmap/gpio_regmap_utils.vh"

  // Bring pll_ref_clk enable signal to the same clock domain.
  wire pll_ref_clk_enable_osc, pll_ref_clk_enable_prc;
  wire pll_ref_clk;

  // Clock used to drive reconfiguration register space.
  wire config_ref_clk;


  // Part-dependent clock generation and gating.
`ifdef VARIANT_XO3

  // Internal oscillator
  wire clk_int_osc;
  defparam OSCH_inst.NOM_FREQ = "66.5";
  OSCH OSCH_inst (
    .STDBY    (1'b0), // 0=Enabled, 1=Disabled
    .OSC      (clk_int_osc),
    .SEDSTDBY ()
  );

  wire pll_ref_clk_freerun;

  DCCA pll_clk_buffer (
    .CLKI (CPLD_REFCLK),
    .CE   (1'b1),
    .CLKO (pll_ref_clk_freerun)
  );

  // Bring pll_ref_clk enable signal to the same clock domain.
  synchronizer #(
    .WIDTH             (1),
    .STAGES            (2),
    .INITIAL_VAL       (1'b1),
    .FALSE_PATH_TO_IN  (1)
  ) pll_ref_clk_enable_sync_i (
    .clk  (pll_ref_clk_freerun),
    .rst  (1'b0),
    .in   (pll_ref_clk_enable_osc),
    .out  (pll_ref_clk_enable_prc)
  );

  // PLL configure to output two clocks, both at the same frequency as the source.
  // One clock is phase-aligned with the reference clock, while the second clock
  // is shifted by 240 degrees. The shifted clock is used to ease timing on
  // output signals with tight setup margins.
  wire pll_ref_clk_adjusted;

  pll pll_ref_clk_pll (
    .CLKI(pll_ref_clk_freerun),
    .ENCLKOP(pll_ref_clk_enable_prc),
    .ENCLKOS(pll_ref_clk_enable_prc),
    .CLKOP(pll_ref_clk),
    .CLKOS(pll_ref_clk_adjusted),
    .LOCK()
  );

  assign config_ref_clk = clk_int_osc;

`else

  // Internal oscillator
  // In the used MAX10 device this oscillator produces a clock anywhere in the
  // range of 55 to 116 MHz. It drives all logic required for identification and
  // to enable the PLL reference clock for the ZBX core registers.
  wire clk_osc;
  osc int_oscillator_i (
    .clkout  (clk_osc),
    .oscena  (1'b1)
  );

  // Pass oscillator clock through buffer to be able to generate a derived clock
  // in the timing constraints
  wire clk_int_osc;
  clkctrl int_osc_clk_ctrl_i (
    .inclk  (clk_osc),
    .ena    (1'b1),
    .outclk (clk_int_osc)
  );

  // Bring pll_ref_clk enable signal to the same clock domain.
  synchronizer #(
    .WIDTH             (1),
    .STAGES            (2),
    .INITIAL_VAL       (1'b1),
    .FALSE_PATH_TO_IN  (1)
  ) pll_ref_clk_enable_sync_i (
    .clk  (CPLD_REFCLK),
    .rst  (1'b0),
    .in   (pll_ref_clk_enable_osc),
    .out  (pll_ref_clk_enable_prc)
  );

  // Add clock buffer with option to enable PLL reference clock during SPLL
  // reconfiguration
  clkctrl pll_ref_clk_ctrl_i (
    .inclk  (CPLD_REFCLK),
    .ena    (pll_ref_clk_enable_prc),
    .outclk (pll_ref_clk)
  );

  assign config_ref_clk = CTRL_REG_CLK;

`endif


  // Generate synchronous resets
  wire ctrlport_rst_osc;
  wire ctrlport_rst_prc;
  wire ctrlport_rst_crc;

  reset_sync reset_sync_osc_i (
    .clk        (clk_int_osc),
    .reset_in   (CTRL_REG_ARST),
    .reset_out  (ctrlport_rst_osc)
  );

  reset_sync reset_sync_prc_i (
    .clk        (pll_ref_clk),
    .reset_in   (CTRL_REG_ARST),
    .reset_out  (ctrlport_rst_prc)
  );

  reset_sync reset_sync_crc_i (
    .clk        (config_ref_clk),
    .reset_in   (CTRL_REG_ARST),
    .reset_out  (ctrlport_rst_crc)
  );

  /////////////////////////////////////////////////////////////////////////////
  // ControlPort sources
  /////////////////////////////////////////////////////////////////////////////
  wire [19:0] spi_ctrlport_req_addr;
  wire [31:0] spi_ctrlport_req_data;
  wire        spi_ctrlport_req_rd;
  wire        spi_ctrlport_req_wr;
  wire        spi_ctrlport_resp_ack;
  wire [31:0] spi_ctrlport_resp_data;
  wire [ 1:0] spi_ctrlport_resp_status;

  // The clock frequencies given as generics are used within the module to
  // determine how many clock cycles are needed to transfer data. The SPI
  // frequency is defined by the PLL ref clock driving this interface and the
  // clock divider, which should be set to at least 10 (see the constraints file
  // for details). Therefore the maximum SPI clock is 6.4MHz. The CtrlPort clock
  // is defined by the internal oscillator. 55 MHz is the lowest rate that
  // should be accounted for (worst case processing cycles for the SPI command).

  spi_slave_to_ctrlport_master #(
    .CLK_FREQUENCY  (55_000_000),
    .SPI_FREQUENCY  (6_400_000)
  ) spi_slave_to_ctrlport_master_i (
    .ctrlport_clk            (clk_int_osc),
    .ctrlport_rst            (ctrlport_rst_osc),
    .m_ctrlport_req_wr       (spi_ctrlport_req_wr),
    .m_ctrlport_req_rd       (spi_ctrlport_req_rd),
    .m_ctrlport_req_addr     (spi_ctrlport_req_addr),
    .m_ctrlport_req_data     (spi_ctrlport_req_data),
    .m_ctrlport_resp_ack     (spi_ctrlport_resp_ack),
    .m_ctrlport_resp_status  (spi_ctrlport_resp_status),
    .m_ctrlport_resp_data    (spi_ctrlport_resp_data),
    .sclk                    (MB_CTRL_SCK),
    .cs_n                    (MB_CTRL_CS),
    .mosi                    (MB_CTRL_MOSI),
    .miso                    (MB_CTRL_MISO)
  );

  wire [19:0] gpio_ctrlport_req_addr;
  wire [31:0] gpio_ctrlport_req_data;
  wire        gpio_ctrlport_req_rd;
  wire        gpio_ctrlport_req_wr;
  wire        gpio_ctrlport_resp_ack;
  wire [31:0] gpio_ctrlport_resp_data;
  wire [ 1:0] gpio_ctrlport_resp_status;

  wire [ 7:0] gpio_data_in;
  wire [ 7:0] gpio_data_out;
  wire        gpio_valid_in;
  wire        gpio_valid_out;
  wire        gpio_output_enable;
  wire        gpio_direction;

  ctrlport_byte_deserializer ctrlport_byte_deserializer_i (
    .ctrlport_clk              (pll_ref_clk),
  `ifdef VARIANT_XO3
    .ctrlport_clk_adjusted   (pll_ref_clk_adjusted),
  `else
    .ctrlport_clk_adjusted   (pll_ref_clk),
  `endif
    .ctrlport_rst              (ctrlport_rst_prc),
    .m_ctrlport_req_wr         (gpio_ctrlport_req_wr),
    .m_ctrlport_req_rd         (gpio_ctrlport_req_rd),
    .m_ctrlport_req_addr       (gpio_ctrlport_req_addr),
    .m_ctrlport_req_data       (gpio_ctrlport_req_data),
    .m_ctrlport_resp_ack       (gpio_ctrlport_resp_ack),
    .m_ctrlport_resp_status    (gpio_ctrlport_resp_status),
    .m_ctrlport_resp_data      (gpio_ctrlport_resp_data),
    .bytestream_data_in        (gpio_data_in),
    .bytestream_valid_in       (gpio_valid_in),
    .bytestream_direction      (gpio_direction),
    .bytestream_data_out       (gpio_data_out),
    .bytestream_valid_out      (gpio_valid_out),
    .bytestream_output_enable  (gpio_output_enable)
  );

  // connect GPIO related signals
  // inputs
  assign gpio_direction = MB_FPGA_GPIO[13];
  assign gpio_valid_in  = MB_FPGA_GPIO[12];
  assign gpio_data_in   = MB_FPGA_GPIO[11:4];
  // outputs
  wire [13:0] gpio_out    = {1'b0, gpio_valid_out, gpio_data_out, 4'b0};
  wire [13:0] gpio_out_en = {1'b0, {9 {gpio_output_enable}},      4'b0};
  // GPIO tristate buffers
  genvar i;
  generate for (i=0; i<14; i=i+1) begin: gpio_tristate_gen
    assign MB_FPGA_GPIO[i] = (gpio_out_en[i]) ? gpio_out[i] : 1'bz;
  end endgenerate

  /////////////////////////////////////////////////////////////////////////////
  // ControlPort distribution network
  /////////////////////////////////////////////////////////////////////////////
  // The network topology is shown below, where S = splitter, C = combiner,
  // X = clock domain crossing and F = address filter (ctrlport_window).
  //
  // The CtrlPort combiner module blocks further requests until a response is
  // received. Therefore there has to be a response for each request. Each
  // combiner input is therefore guarded by an address filter to make sure the
  // request is targeted to the register map behind the combiner. Furthermore
  // each address behind the combiner has to be assigned (no gaps allowed).
  //
  // Requests targeting the reconfig engine are filtered because the reconfig
  // engine clock is disabled when unused.
  //
  //                       X--------> RECONFIG (config_ref_clk)
  //                       |
  //                       F -------> POWER_REGS_REGMAP (clk_int_osc)
  //                       |/
  // SPI (clk_int_osc) >---S-F-C----> BASIC_REGS_REGMAP (clk_int_osc)
  //                       |   |
  //                       F   |
  //                        \ /
  //                         X²
  //                        / \
  //                       F   |
  //                       |   |
  // GPIO (pll_ref_clk) >--S-F-C----> DB_CONTROL (pll_ref_clk)

  wire [19:0] spi_core_ctrlport_req_addr_osc;
  wire [31:0] spi_core_ctrlport_req_data_osc;
  wire        spi_core_ctrlport_req_rd_osc;
  wire        spi_core_ctrlport_req_wr_osc;
  wire        spi_core_ctrlport_resp_ack_osc;
  wire [31:0] spi_core_ctrlport_resp_data_osc;
  wire [1:0]  spi_core_ctrlport_resp_status_osc;

  wire [19:0] spi_base_ctrlport_req_addr;
  wire [31:0] spi_base_ctrlport_req_data;
  wire        spi_base_ctrlport_req_rd;
  wire        spi_base_ctrlport_req_wr;
  wire        spi_base_ctrlport_resp_ack;
  wire [31:0] spi_base_ctrlport_resp_data;
  wire [1:0]  spi_base_ctrlport_resp_status;

  wire [19:0] power_ctrlport_req_addr;
  wire [31:0] power_ctrlport_req_data;
  wire        power_ctrlport_req_rd;
  wire        power_ctrlport_req_wr;
  wire        power_ctrlport_resp_ack;
  wire [31:0] power_ctrlport_resp_data;
  wire [1:0]  power_ctrlport_resp_status;

  wire [19:0] reconfig_ctrlport_req_addr_osc;
  wire [31:0] reconfig_ctrlport_req_data_osc;
  wire        reconfig_ctrlport_req_rd_osc;
  wire        reconfig_ctrlport_req_wr_osc;
  wire        reconfig_ctrlport_resp_ack_osc;
  wire [31:0] reconfig_ctrlport_resp_data_osc;
  wire [1:0]  reconfig_ctrlport_resp_status_osc;

  ctrlport_splitter #(
    .NUM_SLAVES(4)
  ) spi_splitter_i (
    .ctrlport_clk             (clk_int_osc),
    .ctrlport_rst             (ctrlport_rst_osc),
    .s_ctrlport_req_wr        (spi_ctrlport_req_wr),
    .s_ctrlport_req_rd        (spi_ctrlport_req_rd),
    .s_ctrlport_req_addr      (spi_ctrlport_req_addr),
    .s_ctrlport_req_data      (spi_ctrlport_req_data),
    .s_ctrlport_req_byte_en   (),
    .s_ctrlport_req_has_time  (),
    .s_ctrlport_req_time      (),
    .s_ctrlport_resp_ack      (spi_ctrlport_resp_ack),
    .s_ctrlport_resp_status   (spi_ctrlport_resp_status),
    .s_ctrlport_resp_data     (spi_ctrlport_resp_data),
    .m_ctrlport_req_wr        ({spi_core_ctrlport_req_wr_osc, spi_base_ctrlport_req_wr, power_ctrlport_req_wr, reconfig_ctrlport_req_wr_osc}),
    .m_ctrlport_req_rd        ({spi_core_ctrlport_req_rd_osc, spi_base_ctrlport_req_rd, power_ctrlport_req_rd, reconfig_ctrlport_req_rd_osc}),
    .m_ctrlport_req_addr      ({spi_core_ctrlport_req_addr_osc, spi_base_ctrlport_req_addr, power_ctrlport_req_addr, reconfig_ctrlport_req_addr_osc}),
    .m_ctrlport_req_data      ({spi_core_ctrlport_req_data_osc, spi_base_ctrlport_req_data, power_ctrlport_req_data, reconfig_ctrlport_req_data_osc}),
    .m_ctrlport_req_byte_en   (),
    .m_ctrlport_req_has_time  (),
    .m_ctrlport_req_time      (),
    .m_ctrlport_resp_ack      ({spi_core_ctrlport_resp_ack_osc, spi_base_ctrlport_resp_ack, power_ctrlport_resp_ack, reconfig_ctrlport_resp_ack_osc}),
    .m_ctrlport_resp_status   ({spi_core_ctrlport_resp_status_osc, spi_base_ctrlport_resp_status, power_ctrlport_resp_status, reconfig_ctrlport_resp_status_osc}),
    .m_ctrlport_resp_data     ({spi_core_ctrlport_resp_data_osc, spi_base_ctrlport_resp_data, power_ctrlport_resp_data, reconfig_ctrlport_resp_data_osc})
  );

  wire [19:0] spi_core_ctrlport_req_addr_filtered_osc;
  wire [31:0] spi_core_ctrlport_req_data_filtered_osc;
  wire        spi_core_ctrlport_req_rd_filtered_osc;
  wire        spi_core_ctrlport_req_wr_filtered_osc;
  wire        spi_core_ctrlport_resp_ack_filtered_osc;
  wire [31:0] spi_core_ctrlport_resp_data_filtered_osc;
  wire [1:0]  spi_core_ctrlport_resp_status_filtered_osc;

  wire [19:0] spi_base_ctrlport_req_addr_filtered;
  wire [31:0] spi_base_ctrlport_req_data_filtered;
  wire        spi_base_ctrlport_req_rd_filtered;
  wire        spi_base_ctrlport_req_wr_filtered;
  wire        spi_base_ctrlport_resp_ack_filtered;
  wire [31:0] spi_base_ctrlport_resp_data_filtered;
  wire [1:0]  spi_base_ctrlport_resp_status_filtered;

  wire [19:0] reconfig_ctrlport_req_addr_filtered_osc;
  wire [31:0] reconfig_ctrlport_req_data_filtered_osc;
  wire        reconfig_ctrlport_req_rd_filtered_osc;
  wire        reconfig_ctrlport_req_wr_filtered_osc;
  wire        reconfig_ctrlport_resp_ack_filtered_osc;
  wire [31:0] reconfig_ctrlport_resp_data_filtered_osc;
  wire [1:0]  reconfig_ctrlport_resp_status_filtered_osc;

  ctrlport_window #(
    .BASE_ADDRESS  (DB_CONTROL_WINDOW_SPI),
    .WINDOW_SIZE   (DB_CONTROL_WINDOW_SPI_SIZE)
  ) spi_core_window_i (
    .s_ctrlport_req_wr          (spi_core_ctrlport_req_wr_osc),
    .s_ctrlport_req_rd          (spi_core_ctrlport_req_rd_osc),
    .s_ctrlport_req_addr        (spi_core_ctrlport_req_addr_osc),
    .s_ctrlport_req_portid      (),
    .s_ctrlport_req_rem_epid    (),
    .s_ctrlport_req_rem_portid  (),
    .s_ctrlport_req_data        (spi_core_ctrlport_req_data_osc),
    .s_ctrlport_req_byte_en     (),
    .s_ctrlport_req_has_time    (),
    .s_ctrlport_req_time        (),
    .s_ctrlport_resp_ack        (spi_core_ctrlport_resp_ack_osc),
    .s_ctrlport_resp_status     (spi_core_ctrlport_resp_status_osc),
    .s_ctrlport_resp_data       (spi_core_ctrlport_resp_data_osc),
    .m_ctrlport_req_wr          (spi_core_ctrlport_req_wr_filtered_osc),
    .m_ctrlport_req_rd          (spi_core_ctrlport_req_rd_filtered_osc),
    .m_ctrlport_req_addr        (spi_core_ctrlport_req_addr_filtered_osc),
    .m_ctrlport_req_portid      (),
    .m_ctrlport_req_rem_epid    (),
    .m_ctrlport_req_rem_portid  (),
    .m_ctrlport_req_data        (spi_core_ctrlport_req_data_filtered_osc),
    .m_ctrlport_req_byte_en     (),
    .m_ctrlport_req_has_time    (),
    .m_ctrlport_req_time        (),
    .m_ctrlport_resp_ack        (spi_core_ctrlport_resp_ack_filtered_osc),
    .m_ctrlport_resp_status     (spi_core_ctrlport_resp_status_filtered_osc),
    .m_ctrlport_resp_data       (spi_core_ctrlport_resp_data_filtered_osc)
  );

  ctrlport_window #(
    .BASE_ADDRESS  (BASE_WINDOW_SPI),
    .WINDOW_SIZE   (BASE_WINDOW_SPI_SIZE)
  ) spi_base_window_i (
    .s_ctrlport_req_wr          (spi_base_ctrlport_req_wr),
    .s_ctrlport_req_rd          (spi_base_ctrlport_req_rd),
    .s_ctrlport_req_addr        (spi_base_ctrlport_req_addr),
    .s_ctrlport_req_portid      (),
    .s_ctrlport_req_rem_epid    (),
    .s_ctrlport_req_rem_portid  (),
    .s_ctrlport_req_data        (spi_base_ctrlport_req_data),
    .s_ctrlport_req_byte_en     (),
    .s_ctrlport_req_has_time    (),
    .s_ctrlport_req_time        (),
    .s_ctrlport_resp_ack        (spi_base_ctrlport_resp_ack),
    .s_ctrlport_resp_status     (spi_base_ctrlport_resp_status),
    .s_ctrlport_resp_data       (spi_base_ctrlport_resp_data),
    .m_ctrlport_req_wr          (spi_base_ctrlport_req_wr_filtered),
    .m_ctrlport_req_rd          (spi_base_ctrlport_req_rd_filtered),
    .m_ctrlport_req_addr        (spi_base_ctrlport_req_addr_filtered),
    .m_ctrlport_req_portid      (),
    .m_ctrlport_req_rem_epid    (),
    .m_ctrlport_req_rem_portid  (),
    .m_ctrlport_req_data        (spi_base_ctrlport_req_data_filtered),
    .m_ctrlport_req_byte_en     (),
    .m_ctrlport_req_has_time    (),
    .m_ctrlport_req_time        (),
    .m_ctrlport_resp_ack        (spi_base_ctrlport_resp_ack_filtered),
    .m_ctrlport_resp_status     (spi_base_ctrlport_resp_status_filtered),
    .m_ctrlport_resp_data       (spi_base_ctrlport_resp_data_filtered)
  );

  ctrlport_window #(
    .BASE_ADDRESS  (RECONFIG),
    .WINDOW_SIZE   (RECONFIG_SIZE)
  ) spi_reconfig_window_i (
    .s_ctrlport_req_wr          (reconfig_ctrlport_req_wr_osc),
    .s_ctrlport_req_rd          (reconfig_ctrlport_req_rd_osc),
    .s_ctrlport_req_addr        (reconfig_ctrlport_req_addr_osc),
    .s_ctrlport_req_portid      (),
    .s_ctrlport_req_rem_epid    (),
    .s_ctrlport_req_rem_portid  (),
    .s_ctrlport_req_data        (reconfig_ctrlport_req_data_osc),
    .s_ctrlport_req_byte_en     (),
    .s_ctrlport_req_has_time    (),
    .s_ctrlport_req_time        (),
    .s_ctrlport_resp_ack        (reconfig_ctrlport_resp_ack_osc),
    .s_ctrlport_resp_status     (reconfig_ctrlport_resp_status_osc),
    .s_ctrlport_resp_data       (reconfig_ctrlport_resp_data_osc),
    .m_ctrlport_req_wr          (reconfig_ctrlport_req_wr_filtered_osc),
    .m_ctrlport_req_rd          (reconfig_ctrlport_req_rd_filtered_osc),
    .m_ctrlport_req_addr        (reconfig_ctrlport_req_addr_filtered_osc),
    .m_ctrlport_req_portid      (),
    .m_ctrlport_req_rem_epid    (),
    .m_ctrlport_req_rem_portid  (),
    .m_ctrlport_req_data        (reconfig_ctrlport_req_data_filtered_osc),
    .m_ctrlport_req_byte_en     (),
    .m_ctrlport_req_has_time    (),
    .m_ctrlport_req_time        (),
    .m_ctrlport_resp_ack        (reconfig_ctrlport_resp_ack_filtered_osc),
    .m_ctrlport_resp_status     (reconfig_ctrlport_resp_status_filtered_osc),
    .m_ctrlport_resp_data       (reconfig_ctrlport_resp_data_filtered_osc)
  );

  wire [19:0] gpio_core_ctrlport_req_addr;
  wire [31:0] gpio_core_ctrlport_req_data;
  wire        gpio_core_ctrlport_req_rd;
  wire        gpio_core_ctrlport_req_wr;
  wire        gpio_core_ctrlport_resp_ack;
  wire [31:0] gpio_core_ctrlport_resp_data;
  wire [1:0]  gpio_core_ctrlport_resp_status;

  wire [19:0] gpio_base_ctrlport_req_addr_prc;
  wire [31:0] gpio_base_ctrlport_req_data_prc;
  wire        gpio_base_ctrlport_req_rd_prc;
  wire        gpio_base_ctrlport_req_wr_prc;
  wire        gpio_base_ctrlport_resp_ack_prc;
  wire [31:0] gpio_base_ctrlport_resp_data_prc;
  wire [1:0]  gpio_base_ctrlport_resp_status_prc;

  ctrlport_splitter #(
    .NUM_SLAVES(2)
  ) gpio_splitter_i (
    .ctrlport_clk             (pll_ref_clk),
    .ctrlport_rst             (ctrlport_rst_prc),
    .s_ctrlport_req_wr        (gpio_ctrlport_req_wr),
    .s_ctrlport_req_rd        (gpio_ctrlport_req_rd),
    .s_ctrlport_req_addr      (gpio_ctrlport_req_addr),
    .s_ctrlport_req_data      (gpio_ctrlport_req_data),
    .s_ctrlport_req_byte_en   (),
    .s_ctrlport_req_has_time  (),
    .s_ctrlport_req_time      (),
    .s_ctrlport_resp_ack      (gpio_ctrlport_resp_ack),
    .s_ctrlport_resp_status   (gpio_ctrlport_resp_status),
    .s_ctrlport_resp_data     (gpio_ctrlport_resp_data),
    .m_ctrlport_req_wr        ({gpio_core_ctrlport_req_wr, gpio_base_ctrlport_req_wr_prc}),
    .m_ctrlport_req_rd        ({gpio_core_ctrlport_req_rd, gpio_base_ctrlport_req_rd_prc}),
    .m_ctrlport_req_addr      ({gpio_core_ctrlport_req_addr, gpio_base_ctrlport_req_addr_prc}),
    .m_ctrlport_req_data      ({gpio_core_ctrlport_req_data, gpio_base_ctrlport_req_data_prc}),
    .m_ctrlport_req_byte_en   (),
    .m_ctrlport_req_has_time  (),
    .m_ctrlport_req_time      (),
    .m_ctrlport_resp_ack      ({gpio_core_ctrlport_resp_ack, gpio_base_ctrlport_resp_ack_prc}),
    .m_ctrlport_resp_status   ({gpio_core_ctrlport_resp_status, gpio_base_ctrlport_resp_status_prc}),
    .m_ctrlport_resp_data     ({gpio_core_ctrlport_resp_data, gpio_base_ctrlport_resp_data_prc})
  );

  wire [19:0] gpio_core_ctrlport_req_addr_filtered;
  wire [31:0] gpio_core_ctrlport_req_data_filtered;
  wire        gpio_core_ctrlport_req_rd_filtered;
  wire        gpio_core_ctrlport_req_wr_filtered;
  wire        gpio_core_ctrlport_resp_ack_filtered;
  wire [31:0] gpio_core_ctrlport_resp_data_filtered;
  wire [1:0]  gpio_core_ctrlport_resp_status_filtered;

  wire [19:0] gpio_base_ctrlport_req_addr_filtered_prc;
  wire [31:0] gpio_base_ctrlport_req_data_filtered_prc;
  wire        gpio_base_ctrlport_req_rd_filtered_prc;
  wire        gpio_base_ctrlport_req_wr_filtered_prc;
  wire        gpio_base_ctrlport_resp_ack_filtered_prc;
  wire [31:0] gpio_base_ctrlport_resp_data_filtered_prc;
  wire [1:0]  gpio_base_ctrlport_resp_status_filtered_prc;

  ctrlport_window #(
    .BASE_ADDRESS  (DB_CONTROL_WINDOW_GPIO),
    .WINDOW_SIZE   (DB_CONTROL_WINDOW_GPIO_SIZE)
  ) gpio_core_window_i (
    .s_ctrlport_req_wr          (gpio_core_ctrlport_req_wr),
    .s_ctrlport_req_rd          (gpio_core_ctrlport_req_rd),
    .s_ctrlport_req_addr        (gpio_core_ctrlport_req_addr),
    .s_ctrlport_req_portid      (),
    .s_ctrlport_req_rem_epid    (),
    .s_ctrlport_req_rem_portid  (),
    .s_ctrlport_req_data        (gpio_core_ctrlport_req_data),
    .s_ctrlport_req_byte_en     (),
    .s_ctrlport_req_has_time    (),
    .s_ctrlport_req_time        (),
    .s_ctrlport_resp_ack        (gpio_core_ctrlport_resp_ack),
    .s_ctrlport_resp_status     (gpio_core_ctrlport_resp_status),
    .s_ctrlport_resp_data       (gpio_core_ctrlport_resp_data),
    .m_ctrlport_req_wr          (gpio_core_ctrlport_req_wr_filtered),
    .m_ctrlport_req_rd          (gpio_core_ctrlport_req_rd_filtered),
    .m_ctrlport_req_addr        (gpio_core_ctrlport_req_addr_filtered),
    .m_ctrlport_req_portid      (),
    .m_ctrlport_req_rem_epid    (),
    .m_ctrlport_req_rem_portid  (),
    .m_ctrlport_req_data        (gpio_core_ctrlport_req_data_filtered),
    .m_ctrlport_req_byte_en     (),
    .m_ctrlport_req_has_time    (),
    .m_ctrlport_req_time        (),
    .m_ctrlport_resp_ack        (gpio_core_ctrlport_resp_ack_filtered),
    .m_ctrlport_resp_status     (gpio_core_ctrlport_resp_status_filtered),
    .m_ctrlport_resp_data       (gpio_core_ctrlport_resp_data_filtered)
  );

  ctrlport_window #(
    .BASE_ADDRESS  (BASE_WINDOW_GPIO),
    .WINDOW_SIZE   (BASE_WINDOW_GPIO_SIZE)
  ) gpio_base_window_i (
    .s_ctrlport_req_wr          (gpio_base_ctrlport_req_wr_prc),
    .s_ctrlport_req_rd          (gpio_base_ctrlport_req_rd_prc),
    .s_ctrlport_req_addr        (gpio_base_ctrlport_req_addr_prc),
    .s_ctrlport_req_portid      (),
    .s_ctrlport_req_rem_epid    (),
    .s_ctrlport_req_rem_portid  (),
    .s_ctrlport_req_data        (gpio_base_ctrlport_req_data_prc),
    .s_ctrlport_req_byte_en     (),
    .s_ctrlport_req_has_time    (),
    .s_ctrlport_req_time        (),
    .s_ctrlport_resp_ack        (gpio_base_ctrlport_resp_ack_prc),
    .s_ctrlport_resp_status     (gpio_base_ctrlport_resp_status_prc),
    .s_ctrlport_resp_data       (gpio_base_ctrlport_resp_data_prc),
    .m_ctrlport_req_wr          (gpio_base_ctrlport_req_wr_filtered_prc),
    .m_ctrlport_req_rd          (gpio_base_ctrlport_req_rd_filtered_prc),
    .m_ctrlport_req_addr        (gpio_base_ctrlport_req_addr_filtered_prc),
    .m_ctrlport_req_portid      (),
    .m_ctrlport_req_rem_epid    (),
    .m_ctrlport_req_rem_portid  (),
    .m_ctrlport_req_data        (gpio_base_ctrlport_req_data_filtered_prc),
    .m_ctrlport_req_byte_en     (),
    .m_ctrlport_req_has_time    (),
    .m_ctrlport_req_time        (),
    .m_ctrlport_resp_ack        (gpio_base_ctrlport_resp_ack_filtered_prc),
    .m_ctrlport_resp_status     (gpio_base_ctrlport_resp_status_filtered_prc),
    .m_ctrlport_resp_data       (gpio_base_ctrlport_resp_data_filtered_prc)
  );

  wire [19:0] spi_core_ctrlport_req_addr_prc;
  wire [31:0] spi_core_ctrlport_req_data_prc;
  wire        spi_core_ctrlport_req_rd_prc;
  wire        spi_core_ctrlport_req_wr_prc;
  wire        spi_core_ctrlport_resp_ack_prc;
  wire [31:0] spi_core_ctrlport_resp_data_prc;
  wire [1:0]  spi_core_ctrlport_resp_status_prc;

  ctrlport_clk_cross core_clk_cross_i (
    .rst                        (ctrlport_rst_prc),
    .s_ctrlport_clk             (clk_int_osc),
    .s_ctrlport_req_wr          (spi_core_ctrlport_req_wr_filtered_osc),
    .s_ctrlport_req_rd          (spi_core_ctrlport_req_rd_filtered_osc),
    .s_ctrlport_req_addr        (spi_core_ctrlport_req_addr_filtered_osc),
    .s_ctrlport_req_portid      (),
    .s_ctrlport_req_rem_epid    (),
    .s_ctrlport_req_rem_portid  (),
    .s_ctrlport_req_data        (spi_core_ctrlport_req_data_filtered_osc),
    .s_ctrlport_req_byte_en     (),
    .s_ctrlport_req_has_time    (),
    .s_ctrlport_req_time        (),
    .s_ctrlport_resp_ack        (spi_core_ctrlport_resp_ack_filtered_osc),
    .s_ctrlport_resp_status     (spi_core_ctrlport_resp_status_filtered_osc),
    .s_ctrlport_resp_data       (spi_core_ctrlport_resp_data_filtered_osc),
    .m_ctrlport_clk             (pll_ref_clk),
    .m_ctrlport_req_wr          (spi_core_ctrlport_req_wr_prc),
    .m_ctrlport_req_rd          (spi_core_ctrlport_req_rd_prc),
    .m_ctrlport_req_addr        (spi_core_ctrlport_req_addr_prc),
    .m_ctrlport_req_portid      (),
    .m_ctrlport_req_rem_epid    (),
    .m_ctrlport_req_rem_portid  (),
    .m_ctrlport_req_data        (spi_core_ctrlport_req_data_prc),
    .m_ctrlport_req_byte_en     (),
    .m_ctrlport_req_has_time    (),
    .m_ctrlport_req_time        (),
    .m_ctrlport_resp_ack        (spi_core_ctrlport_resp_ack_prc),
    .m_ctrlport_resp_status     (spi_core_ctrlport_resp_status_prc),
    .m_ctrlport_resp_data       (spi_core_ctrlport_resp_data_prc)
  );

  wire [19:0] gpio_base_ctrlport_req_addr_osc;
  wire [31:0] gpio_base_ctrlport_req_data_osc;
  wire        gpio_base_ctrlport_req_rd_osc;
  wire        gpio_base_ctrlport_req_wr_osc;
  wire        gpio_base_ctrlport_resp_ack_osc;
  wire [31:0] gpio_base_ctrlport_resp_data_osc;
  wire [1:0]  gpio_base_ctrlport_resp_status_osc;

  ctrlport_clk_cross base_clk_cross_i (
    .rst                        (ctrlport_rst_osc),
    .s_ctrlport_clk             (pll_ref_clk),
    .s_ctrlport_req_wr          (gpio_base_ctrlport_req_wr_filtered_prc),
    .s_ctrlport_req_rd          (gpio_base_ctrlport_req_rd_filtered_prc),
    .s_ctrlport_req_addr        (gpio_base_ctrlport_req_addr_filtered_prc),
    .s_ctrlport_req_portid      (),
    .s_ctrlport_req_rem_epid    (),
    .s_ctrlport_req_rem_portid  (),
    .s_ctrlport_req_data        (gpio_base_ctrlport_req_data_filtered_prc),
    .s_ctrlport_req_byte_en     (),
    .s_ctrlport_req_has_time    (),
    .s_ctrlport_req_time        (),
    .s_ctrlport_resp_ack        (gpio_base_ctrlport_resp_ack_filtered_prc),
    .s_ctrlport_resp_status     (gpio_base_ctrlport_resp_status_filtered_prc),
    .s_ctrlport_resp_data       (gpio_base_ctrlport_resp_data_filtered_prc),
    .m_ctrlport_clk             (clk_int_osc),
    .m_ctrlport_req_wr          (gpio_base_ctrlport_req_wr_osc),
    .m_ctrlport_req_rd          (gpio_base_ctrlport_req_rd_osc),
    .m_ctrlport_req_addr        (gpio_base_ctrlport_req_addr_osc),
    .m_ctrlport_req_portid      (),
    .m_ctrlport_req_rem_epid    (),
    .m_ctrlport_req_rem_portid  (),
    .m_ctrlport_req_data        (gpio_base_ctrlport_req_data_osc),
    .m_ctrlport_req_byte_en     (),
    .m_ctrlport_req_has_time    (),
    .m_ctrlport_req_time        (),
    .m_ctrlport_resp_ack        (gpio_base_ctrlport_resp_ack_osc),
    .m_ctrlport_resp_status     (gpio_base_ctrlport_resp_status_osc),
    .m_ctrlport_resp_data       (gpio_base_ctrlport_resp_data_osc)
  );

  wire [19:0] reconfig_ctrlport_req_addr_crc;
  wire [31:0] reconfig_ctrlport_req_data_crc;
  wire        reconfig_ctrlport_req_rd_crc;
  wire        reconfig_ctrlport_req_wr_crc;
  wire        reconfig_ctrlport_resp_ack_crc;
  wire [31:0] reconfig_ctrlport_resp_data_crc;
  wire [1:0]  reconfig_ctrlport_resp_status_crc;

  ctrlport_clk_cross reconfig_cross_i (
    .rst                        (ctrlport_rst_crc),
    .s_ctrlport_clk             (clk_int_osc),
    .s_ctrlport_req_wr          (reconfig_ctrlport_req_wr_filtered_osc),
    .s_ctrlport_req_rd          (reconfig_ctrlport_req_rd_filtered_osc),
    .s_ctrlport_req_addr        (reconfig_ctrlport_req_addr_filtered_osc),
    .s_ctrlport_req_portid      (),
    .s_ctrlport_req_rem_epid    (),
    .s_ctrlport_req_rem_portid  (),
    .s_ctrlport_req_data        (reconfig_ctrlport_req_data_filtered_osc),
    .s_ctrlport_req_byte_en     (),
    .s_ctrlport_req_has_time    (),
    .s_ctrlport_req_time        (),
    .s_ctrlport_resp_ack        (reconfig_ctrlport_resp_ack_filtered_osc),
    .s_ctrlport_resp_status     (reconfig_ctrlport_resp_status_filtered_osc),
    .s_ctrlport_resp_data       (reconfig_ctrlport_resp_data_filtered_osc),
    .m_ctrlport_clk             (config_ref_clk),
    .m_ctrlport_req_wr          (reconfig_ctrlport_req_wr_crc),
    .m_ctrlport_req_rd          (reconfig_ctrlport_req_rd_crc),
    .m_ctrlport_req_addr        (reconfig_ctrlport_req_addr_crc),
    .m_ctrlport_req_portid      (),
    .m_ctrlport_req_rem_epid    (),
    .m_ctrlport_req_rem_portid  (),
    .m_ctrlport_req_data        (reconfig_ctrlport_req_data_crc),
    .m_ctrlport_req_byte_en     (),
    .m_ctrlport_req_has_time    (),
    .m_ctrlport_req_time        (),
    .m_ctrlport_resp_ack        (reconfig_ctrlport_resp_ack_crc),
    .m_ctrlport_resp_status     (reconfig_ctrlport_resp_status_crc),
    .m_ctrlport_resp_data       (reconfig_ctrlport_resp_data_crc)
  );

  wire [19:0] core_ctrlport_req_addr;
  wire [31:0] core_ctrlport_req_data;
  wire        core_ctrlport_req_rd;
  wire        core_ctrlport_req_wr;
  wire        core_ctrlport_resp_ack;
  wire [31:0] core_ctrlport_resp_data;
  wire [1:0]  core_ctrlport_resp_status;

  ctrlport_combiner #(
    .NUM_MASTERS  (2),
    .PRIORITY     (1)
  ) core_combiner_i (
    .ctrlport_clk               (pll_ref_clk),
    .ctrlport_rst               (ctrlport_rst_prc),
    .s_ctrlport_req_wr          ({spi_core_ctrlport_req_wr_prc, gpio_core_ctrlport_req_wr_filtered}),
    .s_ctrlport_req_rd          ({spi_core_ctrlport_req_rd_prc, gpio_core_ctrlport_req_rd_filtered}),
    .s_ctrlport_req_addr        ({spi_core_ctrlport_req_addr_prc, gpio_core_ctrlport_req_addr_filtered}),
    .s_ctrlport_req_portid      (),
    .s_ctrlport_req_rem_epid    (),
    .s_ctrlport_req_rem_portid  (),
    .s_ctrlport_req_data        ({spi_core_ctrlport_req_data_prc, gpio_core_ctrlport_req_data_filtered}),
    .s_ctrlport_req_byte_en     (),
    .s_ctrlport_req_has_time    (),
    .s_ctrlport_req_time        (),
    .s_ctrlport_resp_ack        ({spi_core_ctrlport_resp_ack_prc, gpio_core_ctrlport_resp_ack_filtered}),
    .s_ctrlport_resp_status     ({spi_core_ctrlport_resp_status_prc, gpio_core_ctrlport_resp_status_filtered}),
    .s_ctrlport_resp_data       ({spi_core_ctrlport_resp_data_prc, gpio_core_ctrlport_resp_data_filtered}),
    .m_ctrlport_req_wr          (core_ctrlport_req_wr),
    .m_ctrlport_req_rd          (core_ctrlport_req_rd),
    .m_ctrlport_req_addr        (core_ctrlport_req_addr),
    .m_ctrlport_req_portid      (),
    .m_ctrlport_req_rem_epid    (),
    .m_ctrlport_req_rem_portid  (),
    .m_ctrlport_req_data        (core_ctrlport_req_data),
    .m_ctrlport_req_byte_en     (),
    .m_ctrlport_req_has_time    (),
    .m_ctrlport_req_time        (),
    .m_ctrlport_resp_ack        (core_ctrlport_resp_ack),
    .m_ctrlport_resp_status     (core_ctrlport_resp_status),
    .m_ctrlport_resp_data       (core_ctrlport_resp_data)
  );

  wire [19:0] base_ctrlport_req_addr;
  wire [31:0] base_ctrlport_req_data;
  wire        base_ctrlport_req_rd;
  wire        base_ctrlport_req_wr;
  wire        base_ctrlport_resp_ack;
  wire [31:0] base_ctrlport_resp_data;
  wire [1:0]  base_ctrlport_resp_status;

  ctrlport_combiner #(
    .NUM_MASTERS  (2),
    .PRIORITY     (1)
  ) base_combiner_i (
    .ctrlport_clk               (clk_int_osc),
    .ctrlport_rst               (ctrlport_rst_osc),
    .s_ctrlport_req_wr          ({spi_base_ctrlport_req_wr_filtered, gpio_base_ctrlport_req_wr_osc}),
    .s_ctrlport_req_rd          ({spi_base_ctrlport_req_rd_filtered, gpio_base_ctrlport_req_rd_osc}),
    .s_ctrlport_req_addr        ({spi_base_ctrlport_req_addr_filtered, gpio_base_ctrlport_req_addr_osc}),
    .s_ctrlport_req_portid      (),
    .s_ctrlport_req_rem_epid    (),
    .s_ctrlport_req_rem_portid  (),
    .s_ctrlport_req_data        ({spi_base_ctrlport_req_data_filtered, gpio_base_ctrlport_req_data_osc}),
    .s_ctrlport_req_byte_en     (),
    .s_ctrlport_req_has_time    (),
    .s_ctrlport_req_time        (),
    .s_ctrlport_resp_ack        ({spi_base_ctrlport_resp_ack_filtered, gpio_base_ctrlport_resp_ack_osc}),
    .s_ctrlport_resp_status     ({spi_base_ctrlport_resp_status_filtered, gpio_base_ctrlport_resp_status_osc}),
    .s_ctrlport_resp_data       ({spi_base_ctrlport_resp_data_filtered, gpio_base_ctrlport_resp_data_osc}),
    .m_ctrlport_req_wr          (base_ctrlport_req_wr),
    .m_ctrlport_req_rd          (base_ctrlport_req_rd),
    .m_ctrlport_req_addr        (base_ctrlport_req_addr),
    .m_ctrlport_req_portid      (),
    .m_ctrlport_req_rem_epid    (),
    .m_ctrlport_req_rem_portid  (),
    .m_ctrlport_req_data        (base_ctrlport_req_data),
    .m_ctrlport_req_byte_en     (),
    .m_ctrlport_req_has_time    (),
    .m_ctrlport_req_time        (),
    .m_ctrlport_resp_ack        (base_ctrlport_resp_ack),
    .m_ctrlport_resp_status     (base_ctrlport_resp_status),
    .m_ctrlport_resp_data       (base_ctrlport_resp_data)
  );

  /////////////////////////////////////////////////////////////////////////////
  // ControlPort targets
  /////////////////////////////////////////////////////////////////////////////
  // ATR FPGA status input register
  reg [3:0] atr_fpga_state = 4'b0;
  always @(posedge pll_ref_clk) begin
    atr_fpga_state <= MB_FPGA_GPIO[3:0];
  end

  // Local signals to be fanned-out to all SPI slaves.
  wire        lo_sclk;
  wire        lo_mosi;

  wire [7:0]  lo_miso;
  wire [7:0]  lo_csb;

  // Check for same DB_CONTROL_WINDOW offset for SPI and GPIO
  // assert for MAX_BITS
  generate
  if (DB_CONTROL_WINDOW_GPIO != DB_CONTROL_WINDOW_SPI) begin
    DB_CONTROL_WINDOW_offsets_must_be_equal check_db_control_window();
  end
  if (BASE_WINDOW_GPIO != BASE_WINDOW_SPI) begin
    BASE_WINDOW_offsets_must_be_equal check_base_window();
  end
  endgenerate

  zbx_cpld_core #(
    .BASE_ADDRESS(DB_CONTROL_WINDOW_GPIO)
  ) zbx_cpld_core_i (
    .s_ctrlport_req_wr       (core_ctrlport_req_wr),
    .s_ctrlport_req_rd       (core_ctrlport_req_rd),
    .s_ctrlport_req_addr     (core_ctrlport_req_addr),
    .s_ctrlport_req_data     (core_ctrlport_req_data),
    .s_ctrlport_resp_ack     (core_ctrlport_resp_ack),
    .s_ctrlport_resp_status  (core_ctrlport_resp_status),
    .s_ctrlport_resp_data    (core_ctrlport_resp_data),
    .ctrlport_clk            (pll_ref_clk),
    .ctrlport_rst            (ctrlport_rst_prc),
    .atr_fpga_state          (atr_fpga_state),
    .lo_miso                 (lo_miso),
    .lo_csb                  (lo_csb),
    .lo_sclk                 (lo_sclk),
    .lo_mosi                 (lo_mosi),
    .mb_synth_sync           (MB_SYNTH_SYNC),
    .tx0_lo1_sync            (TX0_LO1_SYNC),
    .tx0_lo2_sync            (TX0_LO2_SYNC),
    .tx1_lo1_sync            (TX1_LO1_SYNC),
    .tx1_lo2_sync            (TX1_LO2_SYNC),
    .rx0_lo1_sync            (RX0_LO1_SYNC),
    .rx0_lo2_sync            (RX0_LO2_SYNC),
    .rx1_lo1_sync            (RX1_LO1_SYNC),
    .rx1_lo2_sync            (RX1_LO2_SYNC),
    .tx0_sw1_sw2_ctrl        (TX0_SW1_SW2_CTRL),
    .tx0_sw3_a               (TX0_SW3_A),
    .tx0_sw3_b               (TX0_SW3_B),
    .tx0_sw4_a               (TX0_SW4_A),
    .tx0_sw4_b               (TX0_SW4_B),
    .tx0_sw5_a               (TX0_SW5_A),
    .tx0_sw5_b               (TX0_SW5_B),
    .tx0_sw6_a               (TX0_SW6_A),
    .tx0_sw6_b               (TX0_SW6_B),
    .tx0_sw7_a               (TX0_SW7_A),
    .tx0_sw7_b               (TX0_SW7_B),
    .tx0_sw8_v1              (TX0_SW8_V1),
    .tx0_sw8_v2              (TX0_SW8_V2),
    .tx0_sw8_v3              (TX0_SW8_V3),
    .tx0_sw9_a               (TX0_SW9_A),
    .tx0_sw9_b               (TX0_SW9_B),
    .tx0_sw10_a              (TX0_SW10_A),
    .tx0_sw10_b              (TX0_SW10_B),
    .tx0_sw11_a              (TX0_SW11_A),
    .tx0_sw11_b              (TX0_SW11_B),
    .tx0_sw13_v1             (TX0_SW13_V1),
    .tx0_sw14_v1             (TX0_SW14_V1),
    .tx0_dsa1                (TX0_DSA1),
    .tx0_dsa2                (TX0_DSA2),
    .tx1_sw1_sw2_ctrl        (TX1_SW1_SW2_CTRL),
    .tx1_sw3_a               (TX1_SW3_A),
    .tx1_sw3_b               (TX1_SW3_B),
    .tx1_sw4_a               (TX1_SW4_A),
    .tx1_sw4_b               (TX1_SW4_B),
    .tx1_sw5_a               (TX1_SW5_A),
    .tx1_sw5_b               (TX1_SW5_B),
    .tx1_sw6_a               (TX1_SW6_A),
    .tx1_sw6_b               (TX1_SW6_B),
    .tx1_sw7_a               (TX1_SW7_A),
    .tx1_sw7_b               (TX1_SW7_B),
    .tx1_sw8_v1              (TX1_SW8_V1),
    .tx1_sw8_v2              (TX1_SW8_V2),
    .tx1_sw8_v3              (TX1_SW8_V3),
    .tx1_sw9_a               (TX1_SW9_A),
    .tx1_sw9_b               (TX1_SW9_B),
    .tx1_sw10_a              (TX1_SW10_A),
    .tx1_sw10_b              (TX1_SW10_B),
    .tx1_sw11_a              (TX1_SW11_A),
    .tx1_sw11_b              (TX1_SW11_B),
    .tx1_sw13_v1             (TX1_SW13_V1),
    .tx1_sw14_v1             (TX1_SW14_V1),
    .tx1_dsa1                (TX1_DSA1),
    .tx1_dsa2                (TX1_DSA2),
    .rx0_sw1_a               (RX0_SW1_A),
    .rx0_sw1_b               (RX0_SW1_B),
    .rx0_sw2_a               (RX0_SW2_A),
    .rx0_sw3_v1              (RX0_SW3_V1),
    .rx0_sw3_v2              (RX0_SW3_V2),
    .rx0_sw3_v3              (RX0_SW3_V3),
    .rx0_sw4_a               (RX0_SW4_A),
    .rx0_sw5_a               (RX0_SW5_A),
    .rx0_sw5_b               (RX0_SW5_B),
    .rx0_sw6_a               (RX0_SW6_A),
    .rx0_sw6_b               (RX0_SW6_B),
    .rx0_sw7_sw8_ctrl        (RX0_SW7_SW8_CTRL),
    .rx0_sw9_v1              (RX0_SW9_V1),
    .rx0_sw10_v1             (RX0_SW10_V1),
    .rx0_sw11_v3             (RX0_SW11_V3),
    .rx0_sw11_v2             (RX0_SW11_V2),
    .rx0_sw11_v1             (RX0_SW11_V1),
    .rx0_dsa1_n              (RX0_DSA1_n),
    .rx0_dsa2_n              (RX0_DSA2_n),
    .rx0_dsa3_a_n            (RX0_DSA3_A_n),
    .rx0_dsa3_b_n            (RX0_DSA3_B_n),
    .rx1_sw1_a               (RX1_SW1_A),
    .rx1_sw1_b               (RX1_SW1_B),
    .rx1_sw2_a               (RX1_SW2_A),
    .rx1_sw3_v1              (RX1_SW3_V1),
    .rx1_sw3_v2              (RX1_SW3_V2),
    .rx1_sw3_v3              (RX1_SW3_V3),
    .rx1_sw4_a               (RX1_SW4_A),
    .rx1_sw5_a               (RX1_SW5_A),
    .rx1_sw5_b               (RX1_SW5_B),
    .rx1_sw6_a               (RX1_SW6_A),
    .rx1_sw6_b               (RX1_SW6_B),
    .rx1_sw7_sw8_ctrl        (RX1_SW7_SW8_CTRL),
    .rx1_sw9_v1              (RX1_SW9_V1),
    .rx1_sw10_v1             (RX1_SW10_V1),
    .rx1_sw11_v3             (RX1_SW11_V3),
    .rx1_sw11_v2             (RX1_SW11_V2),
    .rx1_sw11_v1             (RX1_SW11_V1),
    .rx1_dsa1_n              (RX1_DSA1_n),
    .rx1_dsa2_n              (RX1_DSA2_n),
    .rx1_dsa3_a_n            (RX1_DSA3_A_n),
    .rx1_dsa3_b_n            (RX1_DSA3_B_n),
    .ch0_rx2_led             (CH0_RX2_LED),
    .ch0_tx_led              (CH0_TX_LED),
    .ch0_rx_led              (CH0_RX_LED),
    .ch1_rx2_led             (CH1_RX2_LED),
    .ch1_tx_led              (CH1_TX_LED),
    .ch1_rx_led              (CH1_RX_LED)
  );

  /////////////////////////////////////////////////////////////////////////////
  // Base registers
  /////////////////////////////////////////////////////////////////////////////

  basic_regs #(
    .BASE_ADDRESS  (BASE_WINDOW_SPI),
    .SIZE_ADDRESS  (BASE_WINDOW_SPI_SIZE)
  ) basic_regs_i (
    .s_ctrlport_req_wr       (base_ctrlport_req_wr),
    .s_ctrlport_req_rd       (base_ctrlport_req_rd),
    .s_ctrlport_req_addr     (base_ctrlport_req_addr),
    .s_ctrlport_req_data     (base_ctrlport_req_data),
    .s_ctrlport_resp_ack     (base_ctrlport_resp_ack),
    .s_ctrlport_resp_status  (base_ctrlport_resp_status),
    .s_ctrlport_resp_data    (base_ctrlport_resp_data),
    .ctrlport_clk            (clk_int_osc),
    .ctrlport_rst            (ctrlport_rst_osc)
  );

  /////////////////////////////////////////////////////////////////////////////
  // Power registers
  /////////////////////////////////////////////////////////////////////////////

  power_regs #(
    .BASE_ADDRESS  (POWER_REGS),
    .SIZE_ADDRESS  (POWER_REGS_SIZE)
  ) power_regs_i (
    .s_ctrlport_req_wr       (power_ctrlport_req_wr),
    .s_ctrlport_req_rd       (power_ctrlport_req_rd),
    .s_ctrlport_req_addr     (power_ctrlport_req_addr),
    .s_ctrlport_req_data     (power_ctrlport_req_data),
    .s_ctrlport_resp_ack     (power_ctrlport_resp_ack),
    .s_ctrlport_resp_status  (power_ctrlport_resp_status),
    .s_ctrlport_resp_data    (power_ctrlport_resp_data),
    .ctrlport_clk            (clk_int_osc),
    .ctrlport_rst            (ctrlport_rst_osc),
    .enable_tx_7v0           (P7V_ENABLE_A),
    .enable_rx_7v0           (P7V_ENABLE_B),
    .enable_3v3              (P3D3VA_ENABLE),
    .p7v_pg_b                (P7V_PG_B),
    .p7v_pg_a                (P7V_PG_A),
    .pll_ref_clk_enable      (pll_ref_clk_enable_osc)
  );

  /////////////////////////////////////////////////////////////////////////////
  // Reconfiguration
  /////////////////////////////////////////////////////////////////////////////
  // on chip flash interface
  // (naming according to Avalon Memory-Mapped Interfaces -
  // https://www.intel.com/content/dam/www/programmable/us/en/pdfs/literature/manual/mnl_avalon_spec.pdf)
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
  // reset
  wire        ctrlport_rst_crc_n;

  assign ctrlport_rst_crc_n = ~ctrlport_rst_crc;

`ifndef VARIANT_XO3
  on_chip_flash flash_i (
    .clock                    (config_ref_clk),
    .avmm_csr_addr            (csr_addr),
    .avmm_csr_read            (csr_read),
    .avmm_csr_writedata       (csr_writedata),
    .avmm_csr_write           (csr_write),
    .avmm_csr_readdata        (csr_readdata),
    .avmm_data_addr           (data_addr),
    .avmm_data_read           (data_read),
    .avmm_data_writedata      (data_writedata),
    .avmm_data_write          (data_write),
    .avmm_data_readdata       (data_readdata),
    .avmm_data_waitrequest    (data_waitrequest),
    .avmm_data_readdatavalid  (data_readdatavalid),
    .avmm_data_burstcount     (4'b0001),
    .reset_n                  (ctrlport_rst_crc_n)
  );
`endif

  reconfig_engine #(
    .BASE_ADDRESS   (RECONFIG),
    .NUM_ADDRESSES  (RECONFIG_SIZE),
    .MEM_INIT       (1)
  ) reconfig_engine_i (
    .ctrlport_clk            (config_ref_clk),
    .ctrlport_rst            (ctrlport_rst_crc),
    .s_ctrlport_req_wr       (reconfig_ctrlport_req_wr_crc),
    .s_ctrlport_req_rd       (reconfig_ctrlport_req_rd_crc),
    .s_ctrlport_req_addr     (reconfig_ctrlport_req_addr_crc),
    .s_ctrlport_req_data     (reconfig_ctrlport_req_data_crc),
    .s_ctrlport_resp_ack     (reconfig_ctrlport_resp_ack_crc),
    .s_ctrlport_resp_status  (reconfig_ctrlport_resp_status_crc),
    .s_ctrlport_resp_data    (reconfig_ctrlport_resp_data_crc),
    .csr_addr                (csr_addr),
    .csr_read                (csr_read),
    .csr_writedata           (csr_writedata),
    .csr_write               (csr_write),
    .csr_readdata            (csr_readdata),
    .data_addr               (data_addr),
    .data_read               (data_read),
    .data_writedata          (data_writedata),
    .data_write              (data_write),
    .data_readdata           (data_readdata),
    .data_waitrequest        (data_waitrequest),
    .data_readdatavalid      (data_readdatavalid)
  );

  /////////////////////////////////////////////////////////////////////////////
  // LO SPI Break-Out
  /////////////////////////////////////////////////////////////////////////////

  // SDI breakout
`ifdef VARIANT_XO3
  assign RX_TX_LO_SDI     = lo_mosi;
`else
  assign TX0_LO1_SDI      = lo_mosi;
  assign TX0_LO2_SDI      = lo_mosi;
  assign TX1_LO1_SDI      = lo_mosi;
  assign TX1_LO2_SDI      = lo_mosi;
  assign RX0_LO1_SDI      = lo_mosi;
  assign RX0_LO2_SDI      = lo_mosi;
  assign RX1_LO1_SDI      = lo_mosi;
  assign RX1_LO2_SDI      = lo_mosi;
`endif

  assign TX0_LO1_SCK      = lo_sclk;
  assign TX0_LO1_CSB      = lo_csb[TX0_LO1];
  assign lo_miso[TX0_LO1] = TX0_LO1_MUXOUT;

  assign TX0_LO2_SCK      = lo_sclk;
  assign TX0_LO2_CSB      = lo_csb[TX0_LO2];
  assign lo_miso[TX0_LO2] = TX0_LO2_MUXOUT;

  assign TX1_LO1_SCK      = lo_sclk;
  assign TX1_LO1_CSB      = lo_csb[TX1_LO1];
  assign lo_miso[TX1_LO1] = TX1_LO1_MUXOUT;

  assign TX1_LO2_SCK      = lo_sclk;
  assign TX1_LO2_CSB      = lo_csb[TX1_LO2];
  assign lo_miso[TX1_LO2] = TX1_LO2_MUXOUT;

  assign RX0_LO1_SCK      = lo_sclk;
  assign RX0_LO1_CSB      = lo_csb[RX0_LO1];
  assign lo_miso[RX0_LO1] = RX0_LO1_MUXOUT;

  assign RX0_LO2_SCK      = lo_sclk;
  assign RX0_LO2_CSB      = lo_csb[RX0_LO2];
  assign lo_miso[RX0_LO2] = RX0_LO2_MUXOUT;

  assign RX1_LO1_SCK      = lo_sclk;
  assign RX1_LO1_CSB      = lo_csb[RX1_LO1];
  assign lo_miso[RX1_LO1] = RX1_LO1_MUXOUT;

  assign RX1_LO2_SCK      = lo_sclk;
  assign RX1_LO2_CSB      = lo_csb[RX1_LO2];
  assign lo_miso[RX1_LO2] = RX1_LO2_MUXOUT;


endmodule

`default_nettype wire

//XmlParse xml_on
// <top name="ZBX_CPLD">
//  <ports>
//    <info>
//      This section lists all common communication interfaces of the ZBX CPLD.
//      Each input port will point to a regmap. The SPI port can reach out to
//      each register. The GPIO port can access a subset of all register, where
//      the use case is mainly RF configuration to enable fast changes.
//    </info>
//    <port name="SPI" targetregmap="SPI_REGMAP">
//      <info>
//        Controlport requests from this SPI interface are driven by the PL part
//        of the RFSoC via the MB CPLD.
//      </info>
//    </port>
//    <port name="GPIO" targetregmap="GPIO_REGMAP">
//      <info>
//        Controlport requests from the FPGA GPIO lines.
//      </info>
//    </port>
//  </ports>
// </top>
//<regmap name="SPI_REGMAP" readablestrobes="false" generatevhdl="true" ettusguidelines="true">
//  <group name="SPI_REGMAP_WINDOWS">
//    <window name="BASE_WINDOW_SPI"       offset="0x0000" size="0x0020" targetregmap="BASIC_REGS_REGMAP"/>
//    <window name="RECONFIG"              offset="0x0020" size="0x0020" targetregmap="RECONFIG_REGMAP"/>
//    <window name="POWER_REGS"            offset="0x0040" size="0x0020" targetregmap="POWER_REGS_REGMAP"/>
//    <window name="DB_CONTROL_WINDOW_SPI" offset="0x1000" size="0x5000" targetregmap="DB_CONTROL_REGMAP"/>
//  </group>
//</regmap>
//<regmap name="GPIO_REGMAP" readablestrobes="false" generatevhdl="true" ettusguidelines="true">
//  <group name="GPIO_REGMAP_WINDOWS">
//    <window name="BASE_WINDOW_GPIO"       offset="0x0000" size="0x0020" targetregmap="BASIC_REGS_REGMAP"/>
//    <window name="DB_CONTROL_WINDOW_GPIO" offset="0x1000" size="0x5000" targetregmap="DB_CONTROL_REGMAP"/>
//  </group>
//</regmap>
//<regmap name="DB_CONTROL_REGMAP" readablestrobes="false" generatevhdl="true" ettusguidelines="true">
//  <group name="DB_CONTROL_WINDOWS">
//    <info>Windows need to be without gaps to guarantee response to combiners.</info>
//    <window name="ATR_CONTROLLER_REGS"  offset="0x0000" size="0x0020" targetregmap="ATR_REGMAP"/>
//    <window name="LO_CONTROL_REGS"      offset="0x0020" size="0x03E0" targetregmap="LO_CONTROL_REGMAP">
//      <info>Extended original size of 0x20 to fill gap to next window.</info>
//    </window>
//    <window name="LED_SETUP_REGS"       offset="0x0400" size="0x0C00" targetregmap="LED_SETUP_REGMAP">
//      <info>Extended original size of 0x400 to fill gap to next window.</info>
//    </window>
//    <window name="SWITCH_SETUP_REGS"    offset="0x1000" size="0x1000" targetregmap="SWITCH_SETUP_REGMAP"/>
//    <window name="DSA_SETUP_REGS"       offset="0x2000" size="0x3000" targetregmap="DSA_SETUP_REGMAP"/>
//  </group>
//</regmap>
//XmlParse xml_off

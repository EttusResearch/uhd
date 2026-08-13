//
// Copyright 2025 Ettus Research, A National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: b310_core
//
// Description:
//   Encompasses the core logic of the B310. This includes:
//   - Core registers
//   - CHDR encoding
//   - RFNoC image core instantiation
//   - DDR3 data streams infrastructures
//

`default_nettype none

module b310_core # (
  integer PCIE_W      =     128,      // Width of the PCIe interface
  integer RADIO_NIPC  =       1,      // Number of items/samples per cycle
  integer NUM_CH_PER_RADIO =  2,      // Number of channels per radio
  integer BUS_CLK_RATE = 125_000_000  // Bus clock rate in MHz
)
(
  //clock and reset
  input wire          bus_clk,
  input wire          radio_clk,
  input wire          radio_clk_shifted,
  input wire          ce_clk,
  input wire          clk_40mhz,
  input wire          local_ref_clk,  // 122.88/125 MHz
  input wire          ext_ref_clk,    // 10/25/30.72/122.88/125 MHz
  input wire          bus_rst,
  input wire          radio_rst,

  //Core ctrlport (BAR0 accessible, bus_clk domain)
  ctrlport_if.slave   s_core_ctrlport,

  //JESD ctrlport (RFNoC accessible, radio_clk domain)
  ctrlport_if.master  m_radio_jesd_ctrlport,

  input wire [32*NUM_CH_PER_RADIO-1:0]  radio_rx_data,  // Radio RX data
  input wire                            radio_rx_data_valid, // Radio RX strobe
  output wire [32*NUM_CH_PER_RADIO-1:0] radio_tx_data,  // Radio TX data
  input wire                            radio_tx_ready, // Radio TX

  // Radio LED Controls
  output logic [1:0]  radio_led_tx_green,
  output logic [1:0]  radio_led_tx_red,
  output logic [1:0]  radio_led_rx_green,

  // Radio-based ADRV9032 GPIO Controls
  output logic [3:0]  radio_ch0_adrv_gpio_out,
  output logic [3:0]  radio_ch1_adrv_gpio_out,
  input  wire  [3:0]  radio_ch0_adrv_gpio_in,
  input  wire  [3:0]  radio_ch1_adrv_gpio_in,
  output logic [3:0]  radio_ch0_adrv_gpio_dir,
  output logic [3:0]  radio_ch1_adrv_gpio_dir,
  output logic [3:0]  radio_ch0_adrv_trx_out,
  output logic [3:0]  radio_ch1_adrv_trx_out,
  input  wire         radio_ch0_adrv_int,
  input  wire         radio_ch1_adrv_int,

  // Radio path control signals
  output logic [1:0] radio_enable_tdr,
  output logic [1:0] radio_rx_bypass,

  // Radio Misc. Control/Status
  output logic        radio_adrv_reset,
  output logic        radio_test_enable,

  // Radio SPI - For use with ADRV9032 IC
  output logic       radio_spi_mosi,
  output logic       radio_spi_sclk,
  output logic       radio_spi_sen_n,
  input  wire        radio_spi_miso,

  //DDR3 AXI interface
  input wire          ddr3_axi_clk,
  input wire          ddr3_axi_clk_x2,
  input wire          ddr3_axi_rst,
  // Write Address Ports
  output wire [1:0]   ddr3_axi_awid,
  output wire [31:0]  ddr3_axi_awaddr,
  output wire [7:0]   ddr3_axi_awlen,
  output wire [2:0]   ddr3_axi_awsize,
  output wire [1:0]   ddr3_axi_awburst,
  output wire [0:0]   ddr3_axi_awlock,
  output wire [3:0]   ddr3_axi_awcache,
  output wire [2:0]   ddr3_axi_awprot,
  output wire [3:0]   ddr3_axi_awqos,
  output wire         ddr3_axi_awvalid,
  input  wire         ddr3_axi_awready,
  // Write Data Ports
  output wire [255:0] ddr3_axi_wdata,
  output wire [31:0]  ddr3_axi_wstrb,
  output wire         ddr3_axi_wlast,
  output wire         ddr3_axi_wvalid,
  input  wire         ddr3_axi_wready,
  // Write Response Ports
  output wire         ddr3_axi_bready,
  input  wire [1:0]   ddr3_axi_bid,
  input  wire [1:0]   ddr3_axi_bresp,
  input  wire         ddr3_axi_bvalid,
  // Read Address Ports
  output wire [1:0]   ddr3_axi_arid,
  output wire [31:0]  ddr3_axi_araddr,
  output wire [7:0]   ddr3_axi_arlen,
  output wire [2:0]   ddr3_axi_arsize,
  output wire [1:0]   ddr3_axi_arburst,
  output wire [0:0]   ddr3_axi_arlock,
  output wire [3:0]   ddr3_axi_arcache,
  output wire [2:0]   ddr3_axi_arprot,
  output wire [3:0]   ddr3_axi_arqos,
  output wire         ddr3_axi_arvalid,
  input  wire         ddr3_axi_arready,
  // Read Data Ports
  output wire          ddr3_axi_rready,
  input  wire [1:0]    ddr3_axi_rid,
  input  wire [255:0]  ddr3_axi_rdata,
  input  wire [1:0]    ddr3_axi_rresp,
  input  wire          ddr3_axi_rlast,
  input  wire          ddr3_axi_rvalid,

  //PCIe
  output logic [PCIE_W-1:0]  dma_rx_tdata,
  output logic [2:0]         dma_rx_tuser,
  output logic               dma_rx_tlast,
  output logic               dma_rx_tvalid,
  input  wire                dma_rx_tready,
  input  wire  [PCIE_W-1:0]  dma_tx_tdata,
  input  wire  [2:0]         dma_tx_tuser,
  input  wire                dma_tx_tlast,
  input  wire                dma_tx_tvalid,
  output logic               dma_tx_tready,

  // FPGA GPIO
  output logic [9:0] fp_gpio_out,
  input  wire  [9:0] fp_gpio_in,
  output logic [9:0] fp_gpio_dir,

  // Core Control/Status
  output  wire radio_clk_gen_rst,
  output  wire ref_clk_source,
  output  wire lmk_source_select,
  output  wire tcxo_en,
  output  wire lmk_reset,
  input   wire lmk_lock_status,
  output  wire lmk_sync_reva,
  output  wire lmk_sync,

  // PPS from B310 front panel
  input   wire pps_in,

  // PPS from LMK05318 (only used if LMK is configured to output PPS)
  input   wire       gps_lmk_pps_in,
  input   wire [1:0] gps_lmk_status,
  output  logic      gps_lmk_gpio,
  input   wire       gps_pw_fault,
  input   wire       gps_pps_out,

  // Add DONT_TOUCH so that vivado places these on separate registers instead
  // of a single one, to enable it to go into IOBs rather than fanned out.
  (* DONT_TOUCH = "TRUE" *) output  logic [3:0] pps_out,
  output  wire lmk05318_pd_n,

  // Motherboard I2C control - ID EEPROM, power monitor, temp sensors
  inout wire       mb_i2c_scl,
  inout wire       mb_i2c_sda,

  // Thunderbolt I2C control - Thunderbolt controller
  inout wire       tb_i2c_scl,
  inout wire       tb_i2c_sda,

  // Clocking SPI: controls LMKs and TCXO DAC
  output logic       clocking_spi_mosi,
  output logic       clocking_spi_sclk,
  output logic [2:0] clocking_spi_sen_n,
  input  wire        clocking_spi_miso,

  // gps uart
  input  wire uart_rx,
  output wire uart_tx,
  output wire gps_reset_n,
  output wire gps_ant_pwr_en,

  // Temperature monitor
  input wire [11:0] device_temp,

  // cpld jtag
  output wire jtag_cpld_tck,
  output wire jtag_cpld_tms,
  output wire jtag_cpld_tdi,
  input  wire jtag_cpld_tdo,

  // Thunderbolt signal control
  output logic tbolt_pd_ctrl_reset,
  // Power control and status
  input  wire  pwr_1v2_pg,
  input  wire  pwr_25w_src,
  input  wire  pwr_typec_negotiated,
  input  wire  pwr_monitor_alert,
  output logic pwr_led_orange
);

  import ctrlport_pkg::*;
  `include "../regmap/b310_bar0_regmap_utils.vh"
  `include "../regmap/b310_radio_regmap_utils.vh"
  `include "../regmap/core_regs_regmap_utils.vh"
  `include "../regmap/radio_control_regmap_utils.vh"

  //vhook_sigstart
  //vhook_sigend

  // Include the RFNoC image core header file
  `ifdef RFNOC_IMAGE_CORE_HDR
    `include `"`RFNOC_IMAGE_CORE_HDR`"
  `else
    ERROR_RFNOC_IMAGE_CORE_HDR_not_defined();
    `define CHDR_WIDTH     64
    `define RFNOC_PROTOVER { 8'd1, 8'd0 }
  `endif

  localparam CHDR_W   = `CHDR_WIDTH;
  localparam PROTOVER = `RFNOC_PROTOVER;

  if (CHDR_W != PCIE_W) begin : gen_width_check
    $error("CHDR_W does not match PCIE_W");
  end

  localparam NUM_TIMEKEEPERS = 1;

  //---------------------------------------------------------
  // ControlPort Interface Distribution
  //---------------------------------------------------------

  localparam int NUM_CORE_BLOCKS = 8;

  ctrlport_if m_core_ctrlport_array [NUM_CORE_BLOCKS] (.clk(bus_clk), .rst(bus_rst));

  //vhook_e ctrlport_if_splitter
  //vhook_a NUM_SLAVES   NUM_CORE_BLOCKS
  //vhook_a s_ctrlport   s_core_ctrlport
  //vhook_a m_ctrlport   m_core_ctrlport_array
  ctrlport_if_splitter #(
    .NUM_SLAVES(NUM_CORE_BLOCKS)  //int:=2
  ) ctrlport_if_splitterx (
    .s_ctrlport(s_core_ctrlport),       // ctrlport_if.slave
    .m_ctrlport(m_core_ctrlport_array)  // ctrlport_if.master[(NUM_SLAVES-1):0]
  );

  //vhook_e basic_regs
  //vhook_a BASE_ADDRESS BASIC_REGS_WINDOW
  //vhook_a SIZE_ADDRESS BASIC_REGS_WINDOW_SIZE
  //vhook_a s_ctrlport   m_core_ctrlport_array[0]
  basic_regs #(
    .BASE_ADDRESS(BASIC_REGS_WINDOW),      //wire[19:0]:=0
    .SIZE_ADDRESS(BASIC_REGS_WINDOW_SIZE)  //wire[19:0]:=0
  ) basic_regsx (
    .s_ctrlport(m_core_ctrlport_array[0])  // ctrlport_if.slave
  );


  // ID signals
  logic [15:0] device_id;
  logic [63:0] dev_dna;
  logic [63:0] radio_time;
  logic [63:0] bclk_device_dna;
  logic [63:0] bclk_radio_timestamp;

  // dev_dna shouldn't change, but we use a synchronizer
  // anyway to avoid timing issues.

  //vhook_e synchronizer sync_dev_dna
  //vhook_a WIDTH             64
  //vhook_a STAGES            2
  //vhook_a INITIAL_VAL       0
  //vhook_a FALSE_PATH_TO_IN  1
  //vhook_a clk               bus_clk
  //vhook_a rst               1'b0
  //vhook_a in                dev_dna
  //vhook_a out               bclk_device_dna
  synchronizer #(
    .WIDTH           (64),  //integer:=1
    .STAGES          (2),   //integer:=2
    .INITIAL_VAL     (0),   //integer:=0
    .FALSE_PATH_TO_IN(1)    //integer:=1
  ) sync_dev_dna (
    .clk(bus_clk),         //input wire
    .rst(1'b0),            //input wire
    .in (dev_dna),         //input wire[(WIDTH-1):0]
    .out(bclk_device_dna)  //output wire[(WIDTH-1):0]
  );

  // Radio timestamp synchronizer
  logic bclk_radio_time_valid;
  logic [63:0] bclk_radio_time_hs;

  //vhook_e handshake_latch radio_time_hs
  //vhook_a WIDTH       64
  //vhook_a clk_a       radio_clk
  //vhook_a rst_a       radio_rst
  //vhook_a data_a      radio_time
  //vhook_a valid_a     1'b1
  //vhook_a busy_a      {}
  //vhook_a clk_b       bus_clk
  //vhook_a data_b      bclk_radio_timestamp
  //vhook_a valid_b     {}
  handshake_latch #(
    .WIDTH(64)  //int:=32
  ) radio_time_hs (
    .clk_a  (radio_clk),            //input wire
    .rst_a  (radio_rst),            //input wire
    .valid_a(1'b1),                 //input wire
    .data_a (radio_time),           //input wire[(WIDTH-1):0]
    .busy_a (),                     //output logic
    .clk_b  (bus_clk),              //input wire
    .valid_b(),                     //output logic
    .data_b (bclk_radio_timestamp)  //output logic[(WIDTH-1):0]
  );

  logic [19:0] fp_gpio_src;
  logic [31:0] int_pps_div;
  logic use_external_pps;
  logic gps_lmk_pps_count;
  logic gps_bypass_lmk;
  logic gpsdo_pps_in;

  // b3xx_pps_sync signals
  logic [9:0] pps_in_to_rclk_delay;
  logic [26:0] lmk_sync_delay;
  logic lmk_sync_trigger;
  logic lmk_sync_done;
  logic lmk_sync_clk_sel;
  logic lmk_clkin0_sync_sel;
  logic tbolt_pd_ctrl_rst_strobe;

  //vhook_e core_regs
  //vhook_a BASE_ADDRESS              CORE_REGS_WINDOW
  //vhook_a SIZE_ADDRESS              CORE_REGS_WINDOW_SIZE
  //vhook_a CHDR_WIDTH                CHDR_W
  //vhook_a s_ctrlport                m_core_ctrlport_array[1]
  //vhook_a dev_dna                   bclk_device_dna
  //vhook_a radio_time                bclk_radio_timestamp
  //vhook_a gps_lmk_pps_monitor       gps_lmk_pps_count
  //vhook_a tbolt_pd_ctrl_pulse_rst   tbolt_pd_ctrl_rst_strobe
  core_regs #(
    .BASE_ADDRESS   (CORE_REGS_WINDOW),       //logic[19:0]:=0
    .SIZE_ADDRESS   (CORE_REGS_WINDOW_SIZE),  //logic[19:0]:=0
    .PROTOVER       (PROTOVER),               //logic[15:0]:={8'b01,8'b0}
    .CHDR_WIDTH     (CHDR_W),                 //int:=64
    .NUM_TIMEKEEPERS(NUM_TIMEKEEPERS)         //int:=1
  ) core_regsx (
    .s_ctrlport             (m_core_ctrlport_array[1]),  // ctrlport_if.slave
    .device_id              (device_id),                 //output logic[15:0]
    .radio_clk_gen_rst      (radio_clk_gen_rst),         //output logic
    .ref_clk_source         (ref_clk_source),            //output logic
    .lmk_source_select      (lmk_source_select),         //output logic
    .tcxo_en                (tcxo_en),                   //output logic
    .lmk_reset              (lmk_reset),                 //output logic
    .use_external_pps       (use_external_pps),          //output logic
    .lmk_lock_status        (lmk_lock_status),           //input wire
    .dev_dna                (bclk_device_dna),           //input wire[63:0]
    .radio_time             (bclk_radio_timestamp),      //input wire[63:0]
    .int_pps_div            (int_pps_div),               //output logic[31:0]
    .pps_in_to_rclk_delay   (pps_in_to_rclk_delay),      //output logic[9:0]
    .lmk_sync_delay         (lmk_sync_delay),            //output logic[26:0]
    .lmk_sync_clk_sel       (lmk_sync_clk_sel),          //output logic
    .lmk_clkin0_sync_sel    (lmk_clkin0_sync_sel),       //output logic
    .lmk_sync_trigger       (lmk_sync_trigger),          //output logic
    .lmk_sync_done          (lmk_sync_done),             //input wire
    .lmk05318_pd_n          (lmk05318_pd_n),             //output logic
    .fp_gpio_src            (fp_gpio_src),               //output logic[19:0]
    .gps_reset_n            (gps_reset_n),               //output logic
    .gps_ant_pwr_en         (gps_ant_pwr_en),            //output logic
    .gps_lmk_status         (gps_lmk_status),            //input wire[1:0]
    .gps_pw_fault           (gps_pw_fault),              //input wire
    .gps_lmk_gpio           (gps_lmk_gpio),              //output logic
    .gps_lmk_pps_monitor    (gps_lmk_pps_count),         //input wire
    .gps_bypass_lmk         (gps_bypass_lmk),            //output logic
    .tbolt_pd_ctrl_pulse_rst(tbolt_pd_ctrl_rst_strobe),  //output logic
    .pwr_1v2_pg             (pwr_1v2_pg),                //input wire
    .pwr_25w_src            (pwr_25w_src),               //input wire
    .pwr_typec_negotiated   (pwr_typec_negotiated),      //input wire
    .pwr_monitor_alert      (pwr_monitor_alert),         //input wire
    .pwr_led_orange         (pwr_led_orange),            //output logic
    .device_temp            (device_temp)                //input wire[11:0]
  );

  // Motherboard I2C control

  logic mb_i2c_scl_pad_i, mb_i2c_scl_pad_o, mb_i2c_scl_en_n;
  logic mb_i2c_sda_pad_i, mb_i2c_sda_pad_o, mb_i2c_sda_en_n;

  //vhook_e ctrlport_to_wb_i2c mb_i2c_ctrl
  //vhook_a ctrlport_clk                    bus_clk
  //vhook_a ctrlport_rst                    bus_rst
  //vhook_a BASE_ADDRESS                    MBOARD_I2C_WINDOW
  //vhook_a REG_STRIDE_SIZE                 4
  //vhook_a {^s_ctrlport_(req|resp)_(.*)}   m_core_ctrlport_array[2].$1.$2
  //vhook_a {^s(.*)_pad_en_o}               mb_i2c_s$1_en_n
  //vhook_a {^s(.*)_pad_(.*)}               mb_i2c_s$1_pad_$2
  ctrlport_to_wb_i2c #(
    .BASE_ADDRESS   (MBOARD_I2C_WINDOW),  //integer:=0
    .REG_STRIDE_SIZE(4)                   //integer:=1
  ) mb_i2c_ctrl (
    .ctrlport_clk          (bus_clk),                               //input wire
    .ctrlport_rst          (bus_rst),                               //input wire
    .s_ctrlport_req_wr     (m_core_ctrlport_array[2].req.wr),       //input wire
    .s_ctrlport_req_rd     (m_core_ctrlport_array[2].req.rd),       //input wire
    .s_ctrlport_req_addr   (m_core_ctrlport_array[2].req.addr),     //input wire[19:0]
    .s_ctrlport_req_data   (m_core_ctrlport_array[2].req.data),     //input wire[31:0]
    .s_ctrlport_resp_ack   (m_core_ctrlport_array[2].resp.ack),     //output reg
    .s_ctrlport_resp_status(m_core_ctrlport_array[2].resp.status),  //output reg[1:0]
    .s_ctrlport_resp_data  (m_core_ctrlport_array[2].resp.data),    //output reg[31:0]
    .scl_pad_i             (mb_i2c_scl_pad_i),                      //input wire
    .scl_pad_o             (mb_i2c_scl_pad_o),                      //output wire
    .scl_pad_en_o          (mb_i2c_scl_en_n),                       //output wire
    .sda_pad_i             (mb_i2c_sda_pad_i),                      //input wire
    .sda_pad_o             (mb_i2c_sda_pad_o),                      //output wire
    .sda_pad_en_o          (mb_i2c_sda_en_n)                        //output wire
  );

  assign mb_i2c_scl_pad_i = mb_i2c_scl;
  assign mb_i2c_sda_pad_i = mb_i2c_sda;
  // tri-state enabled when high
  assign mb_i2c_scl = ~mb_i2c_scl_en_n ? mb_i2c_scl_pad_o : 1'bz;
  assign mb_i2c_sda = ~mb_i2c_sda_en_n ? mb_i2c_sda_pad_o : 1'bz;

  // Thunderbolt I2C control

  logic tb_i2c_scl_pad_i, tb_i2c_scl_pad_o, tb_i2c_scl_en_n;
  logic tb_i2c_sda_pad_i, tb_i2c_sda_pad_o, tb_i2c_sda_en_n;

  //vhook_e ctrlport_to_wb_i2c tb_i2c_ctrl
  //vhook_a ctrlport_clk                    bus_clk
  //vhook_a ctrlport_rst                    bus_rst
  //vhook_a BASE_ADDRESS                    THUNDERBOLT_I2C_WINDOW
  //vhook_a REG_STRIDE_SIZE                 4
  //vhook_a {^s_ctrlport_(req|resp)_(.*)}   m_core_ctrlport_array[3].$1.$2
  //vhook_a {^s(.*)_pad_en_o}               tb_i2c_s$1_en_n
  //vhook_a {^s(.*)_pad_(.*)}               tb_i2c_s$1_pad_$2
  ctrlport_to_wb_i2c #(
    .BASE_ADDRESS   (THUNDERBOLT_I2C_WINDOW),  //integer:=0
    .REG_STRIDE_SIZE(4)                        //integer:=1
  ) tb_i2c_ctrl (
    .ctrlport_clk          (bus_clk),                               //input wire
    .ctrlport_rst          (bus_rst),                               //input wire
    .s_ctrlport_req_wr     (m_core_ctrlport_array[3].req.wr),       //input wire
    .s_ctrlport_req_rd     (m_core_ctrlport_array[3].req.rd),       //input wire
    .s_ctrlport_req_addr   (m_core_ctrlport_array[3].req.addr),     //input wire[19:0]
    .s_ctrlport_req_data   (m_core_ctrlport_array[3].req.data),     //input wire[31:0]
    .s_ctrlport_resp_ack   (m_core_ctrlport_array[3].resp.ack),     //output reg
    .s_ctrlport_resp_status(m_core_ctrlport_array[3].resp.status),  //output reg[1:0]
    .s_ctrlport_resp_data  (m_core_ctrlport_array[3].resp.data),    //output reg[31:0]
    .scl_pad_i             (tb_i2c_scl_pad_i),                      //input wire
    .scl_pad_o             (tb_i2c_scl_pad_o),                      //output wire
    .scl_pad_en_o          (tb_i2c_scl_en_n),                       //output wire
    .sda_pad_i             (tb_i2c_sda_pad_i),                      //input wire
    .sda_pad_o             (tb_i2c_sda_pad_o),                      //output wire
    .sda_pad_en_o          (tb_i2c_sda_en_n)                        //output wire
  );

  assign tb_i2c_scl_pad_i = tb_i2c_scl;
  assign tb_i2c_sda_pad_i = tb_i2c_sda;
  // tri-state enabled when high
  assign tb_i2c_scl = ~tb_i2c_scl_en_n ? tb_i2c_scl_pad_o : 1'bz;
  assign tb_i2c_sda = ~tb_i2c_sda_en_n ? tb_i2c_sda_pad_o : 1'bz;

  //Clocking SPI

  //vhook_e ctrlport_to_simple_spi clocking_spi_engine
  //vhook_a ctrlport_clk                    bus_clk
  //vhook_a ctrlport_rst                    bus_rst
  //vhook_a BASE_ADDRESS                    CLOCKING_SPI_WINDOW
  //vhook_a SEN_WIDTH                       CLOCKING_SPI_SLAVES_SIZE
  //vhook_a CLK_IDLE                        '0
  //vhook_a SEN_IDLE                        '1
  //vhook_a {^s_ctrlport_(req|resp)_(.*)}   m_core_ctrlport_array[4].$1.$2
  //vhook_a mosi                            clocking_spi_mosi
  //vhook_a sclk                            clocking_spi_sclk
  //vhook_a sen                             clocking_spi_sen_n
  //vhook_a miso                            clocking_spi_miso
  //vhook_a debug                           {}
  ctrlport_to_simple_spi #(
    .BASE_ADDRESS(CLOCKING_SPI_WINDOW),       //int:=0
    .SEN_WIDTH   (CLOCKING_SPI_SLAVES_SIZE),  //int:=8
    .CLK_IDLE    ('0),                        //bit:=0
    .SEN_IDLE    ('1)                         //logic[23:0]:='1
  ) clocking_spi_engine (
    .ctrlport_clk          (bus_clk),                               //input wire
    .ctrlport_rst          (bus_rst),                               //input wire
    .s_ctrlport_req_wr     (m_core_ctrlport_array[4].req.wr),       //input wire
    .s_ctrlport_req_rd     (m_core_ctrlport_array[4].req.rd),       //input wire
    .s_ctrlport_req_addr   (m_core_ctrlport_array[4].req.addr),     //input wire[19:0]
    .s_ctrlport_req_data   (m_core_ctrlport_array[4].req.data),     //input wire[31:0]
    .s_ctrlport_resp_ack   (m_core_ctrlport_array[4].resp.ack),     //output logic
    .s_ctrlport_resp_status(m_core_ctrlport_array[4].resp.status),  //output logic[1:0]
    .s_ctrlport_resp_data  (m_core_ctrlport_array[4].resp.data),    //output logic[31:0]
    .sen                   (clocking_spi_sen_n),                    //output logic[(SEN_WIDTH-1):0]
    .sclk                  (clocking_spi_sclk),                     //output logic
    .mosi                  (clocking_spi_mosi),                     //output logic
    .miso                  (clocking_spi_miso),                     //input wire
    .debug                 ()                                       //output logic[31:0]
  );

  // Select either local refclk if using a single B310 or
  // external refclock if using multi device sync or a custom external reference.
  // TODO:  Think about needing to reset logic when switching this over?
  // vhook_warn think about needing to reset logic when switching this over?


  logic [31:0] int_pps_div_delayed = 32'd122_880_000;
  logic        pps_reset = '1;
  logic        pps_reset_refclk;


  // Reset the internal PPS generator whenever the reference clock source
  // or the PPS period changes.
  always_ff @(posedge bus_clk) begin
    if (bus_rst) begin
      int_pps_div_delayed     <= 32'd122_880_000;
      pps_reset               <= '1;
    end else begin
      int_pps_div_delayed     <= int_pps_div;
      // reset whenever the reference clock source or pps period changes
      pps_reset               <=  (int_pps_div_delayed != int_pps_div);
    end
  end

  // Synchronize pps_reset to local_ref_clk domain

  //vhook_e pulse_synchronizer pps_reset_sync
  //vhook_a MODE        "PULSE"
  //vhook_a STAGES      2
  //vhook_a clk_a       bus_clk
  //vhook_a rst_a       bus_rst
  //vhook_a pulse_a     pps_reset
  //vhook_a busy_a      {}
  //vhook_a clk_b       local_ref_clk
  //vhook_a pulse_b     pps_reset_refclk
  pulse_synchronizer #(
    .MODE  ("PULSE"),  //string:="PULSE"
    .STAGES(2)         //integer:=2
  ) pps_reset_sync (
    .clk_a  (bus_clk),          //input wire
    .rst_a  (bus_rst),          //input wire
    .pulse_a(pps_reset),        //input wire
    .busy_a (),                 //output wire
    .clk_b  (local_ref_clk),    //input wire
    .pulse_b(pps_reset_refclk)  //output wire
  );

  logic pps_reset_extended_refclk;

  //vhook_e pulse_stretch   pp_reset_stretch
  //vhook_a SCALE           8
  //vhook_a clk             local_ref_clk
  //vhook_a rst             1'b0
  //vhook_a pulse           pps_reset_refclk
  //vhook_a pulse_stretched pps_reset_extended_refclk
  pulse_stretch #(
    .SCALE(8)  //integer:=64'b0101111101011110000100000
  ) pp_reset_stretch (
    .clk            (local_ref_clk),             //input wire
    .rst            (1'b0),                      //input wire
    .pulse          (pps_reset_refclk),          //input wire
    .pulse_stretched(pps_reset_extended_refclk)  //output wire
  );


  logic        int_pps_out;
  logic [31:0] int_pps_div_refclk;

  //vhook_e handshake_latch pps_div_hs
  //vhook_a WIDTH             32
  //vhook_a clk_a             bus_clk
  //vhook_a rst_a             bus_rst
  //vhook_a data_a            int_pps_div
  //vhook_a valid_a           1'b1
  //vhook_a busy_a            {}
  //vhook_a clk_b             local_ref_clk
  //vhook_a data_b            int_pps_div_refclk
  //vhook_a valid_b           {}
  handshake_latch #(
    .WIDTH(32)  //int:=32
  ) pps_div_hs (
    .clk_a  (bus_clk),            //input wire
    .rst_a  (bus_rst),            //input wire
    .valid_a(1'b1),               //input wire
    .data_a (int_pps_div),        //input wire[(WIDTH-1):0]
    .busy_a (),                   //output logic
    .clk_b  (local_ref_clk),      //input wire
    .valid_b(),                   //output logic
    .data_b (int_pps_div_refclk)  //output logic[(WIDTH-1):0]
  );

  // Generate an internal PPS signal with a 25% duty cycle
  pulse_generator #(.WIDTH(32)) int_pps_gen
  (
    .clk(local_ref_clk),
    .reset(pps_reset_extended_refclk),
    .period(int_pps_div_refclk),
    //shift frequency by 2 bits (divide by 4) for a 25% duty cycle
    .pulse_width({2'b00,int_pps_div_refclk[31:2]}),
    .pulse(int_pps_out)
  );

  // Export internal PPS to the PPS outputs
  // add 4 registers to enable it to go into the IOB
  always_ff @(posedge local_ref_clk) begin
      pps_out[0] <= int_pps_out;
      pps_out[1] <= int_pps_out;
      pps_out[2] <= int_pps_out;
      pps_out[3] <= int_pps_out;
  end

  logic int_pps_rclk, ext_pps_rclk, pps_rclk;

  //vhook_e synchronizer int_pps_sync
  //vhook_a WIDTH             1
  //vhook_a STAGES            2
  //vhook_a INITIAL_VAL       '0
  //vhook_a FALSE_PATH_TO_IN  1
  //vhook_a clk               radio_clk
  //vhook_a rst               radio_rst
  //vhook_a in                int_pps_out
  //vhook_a out               int_pps_rclk
  synchronizer #(
    .WIDTH           (1),   //integer:=1
    .STAGES          (2),   //integer:=2
    .INITIAL_VAL     ('0),  //integer:=0
    .FALSE_PATH_TO_IN(1)    //integer:=1
  ) int_pps_sync (
    .clk(radio_clk),    //input wire
    .rst(radio_rst),    //input wire
    .in (int_pps_out),  //input wire[(WIDTH-1):0]
    .out(int_pps_rclk)  //output wire[(WIDTH-1):0]
  );

  // sync pps_in to ext_ref_clk first as it is synchronous to that
  // Double sync in case it is not for non B310 PPS INs
  // Then double sync to radio_clk_shifted domain, as meeting timing
  // to that is easier as we phase adjust it with the PLL
  // Finally it goes to radio_clk domain for normal use
  // The entire path is synchronous for case of 4 b310s synced to each other
  // via exported ref_clk and pps.
  logic pps_in_ext_ref_clk;
  //vhook_e synchronizer pps_in_sync_ref_clk_i
  //vhook_a WIDTH             1
  //vhook_a STAGES            2
  //vhook_a INITIAL_VAL       '0
  //vhook_a FALSE_PATH_TO_IN  0
  //vhook_a clk               ext_ref_clk
  //vhook_a rst               '0
  //vhook_a in                pps_in
  //vhook_a out               pps_in_ext_ref_clk
  synchronizer #(
    .WIDTH           (1),   //integer:=1
    .STAGES          (2),   //integer:=2
    .INITIAL_VAL     ('0),  //integer:=0
    .FALSE_PATH_TO_IN(0)    //integer:=1
  ) pps_in_sync_ref_clk_i (
    .clk(ext_ref_clk),        //input wire
    .rst('0),                 //input wire
    .in (pps_in),             //input wire[(WIDTH-1):0]
    .out(pps_in_ext_ref_clk)  //output wire[(WIDTH-1):0]
  );

  logic ext_ref_clk_rst;
  //vhook_e reset_sync  ext_ref_clk_sync
  //vhook_a clk         ext_ref_clk
  //vhook_a reset_in    bus_rst
  //vhook_a reset_out   ext_ref_clk_rst
  reset_sync ext_ref_clk_sync (
    .clk      (ext_ref_clk),     //input wire
    .reset_in (bus_rst),         //input wire
    .reset_out(ext_ref_clk_rst)  //output reg
  );

  logic pps_in_rclk;
  //vhook_e b3xx_pps_sync b3xx_pps_sync_i
  //vhook_a base_ref_clk  ext_ref_clk
  //vhook_a ctrl_clk      bus_clk
  //vhook_a brc_rst       ext_ref_clk_rst
  //vhook_a pps_in_brc    pps_in_ext_ref_clk
  //vhook_a debug         {}
  b3xx_pps_sync b3xx_pps_sync_i (
    .base_ref_clk        (ext_ref_clk),           //input wire
    .radio_clk_shifted   (radio_clk_shifted),     //input wire
    .radio_clk           (radio_clk),             //input wire
    .ctrl_clk            (bus_clk),               //input wire
    .brc_rst             (ext_ref_clk_rst),       //input wire
    .pps_in_brc          (pps_in_ext_ref_clk),    //input wire
    .pps_in_rclk         (pps_in_rclk),           //output logic
    .lmk_sync_reva       (lmk_sync_reva),         //output logic
    .lmk_sync            (lmk_sync),              //output logic
    .lmk_sync_clk_sel    (lmk_sync_clk_sel),      //input wire
    .lmk_clkin0_sync_sel (lmk_clkin0_sync_sel),   //input wire
    .lmk_sync_delay      (lmk_sync_delay),        //input wire[26:0]
    .lmk_sync_trigger    (lmk_sync_trigger),      //input wire
    .lmk_sync_done       (lmk_sync_done),         //output logic
    .pps_in_to_rclk_delay(pps_in_to_rclk_delay),  //input wire[9:0]
    .debug               ()                       //output logic[3:0]
  );

  logic use_external_pps_rclk;
  //vhook_e synchronizer use_external_pps_sync
  //vhook_a WIDTH             1
  //vhook_a STAGES            2
  //vhook_a INITIAL_VAL       '0
  //vhook_a FALSE_PATH_TO_IN  0
  //vhook_a clk               radio_clk
  //vhook_a rst               radio_rst
  //vhook_a in                use_external_pps
  //vhook_a out               use_external_pps_rclk
  synchronizer #(
    .WIDTH           (1),   //integer:=1
    .STAGES          (2),   //integer:=2
    .INITIAL_VAL     ('0),  //integer:=0
    .FALSE_PATH_TO_IN(0)    //integer:=1
  ) use_external_pps_sync (
    .clk(radio_clk),             //input wire
    .rst(radio_rst),             //input wire
    .in (use_external_pps),      //input wire[(WIDTH-1):0]
    .out(use_external_pps_rclk)  //output wire[(WIDTH-1):0]
  );

  // GPS PPS synchronizer to radio_clk domain for use in timekeeper
  // The incoming PPS signal presents jitter from both the GPS module (+- 16ns)
  // and from the LMK05318's tracking of the GPS PPS (up to 100s of ns).
  // We only synchronize the signal to avoid driving metastability in the timekeeper
  // but do not do any additional timing compensation for this jitter,
  // as phase correlation when using the GPS reference will be expected
  // to match the order of this jitter.

  assign gpsdo_pps_in = gps_bypass_lmk ? gps_pps_out : gps_lmk_pps_in;

  logic gpsdo_pps_in_rclk;
  //vhook_e pps_synchronizer gps_lmk_pps_rclk_sync
  //vhook_a ref_clk       radio_clk
  //vhook_a timebase_clk  radio_clk
  //vhook_a pps_in        gpsdo_pps_in
  //vhook_a pps_out       gpsdo_pps_in_rclk
  //vhook_a pps_count     {}
  pps_synchronizer gps_lmk_pps_rclk_sync (
    .ref_clk     (radio_clk),          //input wire
    .timebase_clk(radio_clk),          //input wire
    .pps_in      (gpsdo_pps_in),       //input wire
    .pps_out     (gpsdo_pps_in_rclk),  //output wire
    .pps_count   ()                    //output reg
  );

  // GPS PPS Monitor - Used only for register-based monitoring
  // of the GPS PPS signal
  //vhook_e pps_synchronizer gps_lmk_pps_bclk_sync
  //vhook_a ref_clk       radio_clk
  //vhook_a timebase_clk  bus_clk
  //vhook_a pps_in        gpsdo_pps_in
  //vhook_a pps_out       {}
  //vhook_a pps_count     gps_lmk_pps_count
  pps_synchronizer gps_lmk_pps_bclk_sync (
    .ref_clk     (radio_clk),         //input wire
    .timebase_clk(bus_clk),           //input wire
    .pps_in      (gpsdo_pps_in),      //input wire
    .pps_out     (),                  //output wire
    .pps_count   (gps_lmk_pps_count)  //output reg
  );

  // Select external PPS source
  assign ext_pps_rclk = ref_clk_source ? pps_in_rclk : gpsdo_pps_in_rclk;

  // Select internal/external PPS source
  assign pps_rclk = use_external_pps_rclk ? ext_pps_rclk : int_pps_rclk;

  //vhook_e timekeeper timekeeper_i
  //vhook_a BASE_ADDR                       TIMEKEEPER_WINDOW
  //vhook_a TIME_INCREMENT                  RADIO_NIPC
  //vhook_a tb_clk                          radio_clk
  //vhook_a tb_rst                          radio_rst
  //vhook_a s_ctrlport_clk                  bus_clk
  //vhook_a time_increment                  '0
  //vhook_a pps                             pps_rclk
  //vhook_a sample_rx_stb                   radio_rx_data_valid
  //vhook_a tb_timestamp                    radio_time
  //vhook_a {^tb_(.*)}                      {}
  //vhook_a {^s_ctrlport_(req|resp)_(.*)}   m_core_ctrlport_array[5].$1.$2
  timekeeper #(
    .BASE_ADDR     (TIMEKEEPER_WINDOW),  //integer:='b0
    .TIME_INCREMENT(RADIO_NIPC)          //integer:=1
  ) timekeeper_i (
    .tb_clk               (radio_clk),                           //input wire
    .tb_rst               (radio_rst),                           //input wire
    .s_ctrlport_clk       (bus_clk),                             //input wire
    .s_ctrlport_req_wr    (m_core_ctrlport_array[5].req.wr),     //input wire
    .s_ctrlport_req_rd    (m_core_ctrlport_array[5].req.rd),     //input wire
    .s_ctrlport_req_addr  (m_core_ctrlport_array[5].req.addr),   //input wire[19:0]
    .s_ctrlport_req_data  (m_core_ctrlport_array[5].req.data),   //input wire[31:0]
    .s_ctrlport_resp_ack  (m_core_ctrlport_array[5].resp.ack),   //output wire
    .s_ctrlport_resp_data (m_core_ctrlport_array[5].resp.data),  //output wire[31:0]
    .time_increment       ('0),                                  //input wire[7:0]
    .sample_rx_stb        (radio_rx_data_valid),                 //input wire
    .pps                  (pps_rclk),                            //input wire
    .tb_timestamp         (radio_time),                          //output reg[63:0]
    .tb_timestamp_last_pps(),                                    //output reg[63:0]
    .tb_period_ns_q32     (),                                    //output reg[63:0]
    .tb_changed           ()                                     //output reg
  );

  //RX_Size such that it fits in 1 BRAM
  ctrlport_to_wb_uart #(
    .BASE_ADDRESS   (GPS_UART_WINDOW),  //integer:=0
    .REG_STRIDE_SIZE(4),               //integer:=1
    .CLKDIV_DEFAULT(3906),
    .RX_SIZE(9)
  ) gps_uart_ctrl (
    .ctrlport_clk          (bus_clk),
    .ctrlport_rst          (bus_rst),
    .s_ctrlport_req_wr     (m_core_ctrlport_array[6].req.wr),
    .s_ctrlport_req_rd     (m_core_ctrlport_array[6].req.rd),
    .s_ctrlport_req_addr   (m_core_ctrlport_array[6].req.addr),
    .s_ctrlport_req_data   (m_core_ctrlport_array[6].req.data),
    .s_ctrlport_resp_ack   (m_core_ctrlport_array[6].resp.ack),
    .s_ctrlport_resp_status(m_core_ctrlport_array[6].resp.status),
    .s_ctrlport_resp_data  (m_core_ctrlport_array[6].resp.data),
    .uart_rx               (uart_rx),
    .uart_tx               (uart_tx)
  );

  ctrlport_to_jtag #(
    .BASE_ADDRESS       (CPLD_JTAG_WINDOW),
    .DEFAULT_PRESCALAR  (12)
  ) cpld_jtag_ctrl (
    .ctrlport_clk          (bus_clk),
    .ctrlport_rst          (bus_rst),
    .s_ctrlport_req_wr     (m_core_ctrlport_array[7].req.wr),
    .s_ctrlport_req_rd     (m_core_ctrlport_array[7].req.rd),
    .s_ctrlport_req_addr   (m_core_ctrlport_array[7].req.addr),
    .s_ctrlport_req_data   (m_core_ctrlport_array[7].req.data),
    .s_ctrlport_resp_ack   (m_core_ctrlport_array[7].resp.ack),
    .s_ctrlport_resp_status(m_core_ctrlport_array[7].resp.status),
    .s_ctrlport_resp_data  (m_core_ctrlport_array[7].resp.data),
    .tck   (jtag_cpld_tck),
    .tdi   (jtag_cpld_tdi),
    .tdo   (jtag_cpld_tdo),
    .tms   (jtag_cpld_tms)
  );

  // Thunderbolt PD Controller reset

  logic [31:0] tbolt_pd_reset_strobe_counter  = '0;
  logic        tbolt_pd_reset_cnt_done        = '1;
  logic        tbolt_pd_reset_cnt_done_dlyd   = '1;
  logic        pulse_pd_ctrl_reset            = '0;

  // Delay the Thunderbolt PD Controller reset strobe to ensure the register
  // transaction that triggers it is acknowledged.
  localparam int TB_RESET_DELAY = 75_000_000; // 500ms at 150MHz

  always_ff @(posedge bus_clk) begin
    if (bus_rst) begin
      tbolt_pd_reset_strobe_counter <= '0;
      tbolt_pd_reset_cnt_done       <= '1;
      tbolt_pd_reset_cnt_done_dlyd  <= '1;
      pulse_pd_ctrl_reset           <= '0;
    end else begin
      if (tbolt_pd_ctrl_rst_strobe)  begin
        tbolt_pd_reset_strobe_counter <= TB_RESET_DELAY - 1;
      end else if (!tbolt_pd_reset_cnt_done) begin
        tbolt_pd_reset_strobe_counter <= tbolt_pd_reset_strobe_counter - 1;
      end
      tbolt_pd_reset_cnt_done <= (tbolt_pd_reset_strobe_counter == 0);
      tbolt_pd_reset_cnt_done_dlyd <= tbolt_pd_reset_cnt_done;
      pulse_pd_ctrl_reset <= (tbolt_pd_reset_cnt_done && !tbolt_pd_reset_cnt_done_dlyd);
    end
  end

  // Stretch the Thunderbolt PD Controller reset pulse
  localparam int RST_TICK_LENGTH = 500_000; // ~3ms at 150 MHz

  //vhook_e pulse_stretch tbolt_pd_ctrl_pulse_rst_stretch
  //vhook_a SCALE             RST_TICK_LENGTH
  //vhook_a clk               bus_clk
  //vhook_a rst               bus_rst
  //vhook_a pulse             pulse_pd_ctrl_reset
  //vhook_a pulse_stretched   tbolt_pd_ctrl_reset
  pulse_stretch #(
    .SCALE(RST_TICK_LENGTH)  //integer:=64'b0101111101011110000100000
  ) tbolt_pd_ctrl_pulse_rst_stretch (
    .clk            (bus_clk),              //input wire
    .rst            (bus_rst),              //input wire
    .pulse          (pulse_pd_ctrl_reset),  //input wire
    .pulse_stretched(tbolt_pd_ctrl_reset)   //output wire
  );


  //---------------------------------------------------------
  // CHDR Encoding
  //---------------------------------------------------------

  localparam MTU      = 10;
  localparam RT_TBL_SIZE  = 6;        // Number of noc instances
  localparam NODE_INST    = 0;        // Node instance number
  localparam DMA_ID_WIDTH = 3;        // Width of the DMA ID in CHDR

  wire [CHDR_W-1:0] m_chdr_tdata;
  wire              m_chdr_tlast;
  wire              m_chdr_tready;
  wire              m_chdr_tvalid;
  wire [CHDR_W-1:0] s_chdr_tdata;
  wire              s_chdr_tlast;
  wire              s_chdr_tready;
  wire              s_chdr_tvalid;

  //vhook_e nirio_chdr_adapter nirio_chdr_adapter_i
  //vhook_a  DATA_W             CHDR_W
  //vhook_a  clk                bus_clk
  //vhook_a  rst                bus_rst
  //vhook_a {^s_dma_t(.*)}      dma_tx_t$1
  //vhook_a {^m_dma_t(.*)}      dma_rx_t$1
  nirio_chdr_adapter #(
    .PROTOVER    (PROTOVER),     //wire[15:0]:={8'b01,8'b0}
    .DATA_W      (CHDR_W),       //integer:=64
    .MTU         (MTU),          //integer:=10
    .RT_TBL_SIZE (RT_TBL_SIZE),  //integer:=6
    .NODE_INST   (NODE_INST),    //integer:=0
    .DMA_ID_WIDTH(DMA_ID_WIDTH)  //integer:=3
  ) nirio_chdr_adapter_i (
    .clk          (bus_clk),        //input wire
    .rst          (bus_rst),        //input wire
    .device_id    (device_id),      //input wire[15:0]
    .s_dma_tdata  (dma_tx_tdata),   //input wire[(DATA_W-1):0]
    .s_dma_tuser  (dma_tx_tuser),   //input wire[(DMA_ID_WIDTH-1):0]
    .s_dma_tlast  (dma_tx_tlast),   //input wire
    .s_dma_tvalid (dma_tx_tvalid),  //input wire
    .s_dma_tready (dma_tx_tready),  //output wire
    .m_dma_tdata  (dma_rx_tdata),   //output wire[(DATA_W-1):0]
    .m_dma_tuser  (dma_rx_tuser),   //output wire[(DMA_ID_WIDTH-1):0]
    .m_dma_tlast  (dma_rx_tlast),   //output wire
    .m_dma_tvalid (dma_rx_tvalid),  //output wire
    .m_dma_tready (dma_rx_tready),  //input wire
    .s_chdr_tdata (s_chdr_tdata),   //input wire[(DATA_W-1):0]
    .s_chdr_tlast (s_chdr_tlast),   //input wire
    .s_chdr_tvalid(s_chdr_tvalid),  //input wire
    .s_chdr_tready(s_chdr_tready),  //output wire
    .m_chdr_tdata (m_chdr_tdata),   //output wire[(DATA_W-1):0]
    .m_chdr_tlast (m_chdr_tlast),   //output wire
    .m_chdr_tvalid(m_chdr_tvalid),  //output wire
    .m_chdr_tready(m_chdr_tready)   //input wire
  );


  //---------------------------------------------------------
  // RFNOC
  //---------------------------------------------------------

  ctrlport_if radio_ctrlport_if(.clk(radio_clk), .rst(radio_rst)); // RFNoC ctrlport interface

  wire [19:0] radio_ctrlport_req_addr;
  wire [ 3:0] radio_ctrlport_req_byte_en;
  wire [31:0] radio_ctrlport_req_data;
  wire        radio_ctrlport_req_has_time;
  wire        radio_ctrlport_req_rd;
  wire [63:0] radio_ctrlport_req_time;
  wire        radio_ctrlport_req_wr;

  logic        radio_ctrlport_resp_ack;
  logic [31:0] radio_ctrlport_resp_data;
  logic [ 1:0] radio_ctrlport_resp_status;

  always_comb begin : radio_ctrlport_drive

    radio_ctrlport_if.req.addr      = radio_ctrlport_req_addr;
    radio_ctrlport_if.req.byte_en   = radio_ctrlport_req_byte_en;
    radio_ctrlport_if.req.data      = radio_ctrlport_req_data;
    radio_ctrlport_if.req.has_time  = radio_ctrlport_req_has_time;
    radio_ctrlport_if.req.timestamp = radio_ctrlport_req_time;
    radio_ctrlport_if.req.rd        = radio_ctrlport_req_rd;
    radio_ctrlport_if.req.wr        = radio_ctrlport_req_wr;

    radio_ctrlport_resp_ack     = radio_ctrlport_if.resp.ack;
    radio_ctrlport_resp_status  = radio_ctrlport_if.resp.status;
    radio_ctrlport_resp_data    = radio_ctrlport_if.resp.data;

  end

  wire [ 1:0] radio_rx_running_radio0;
  wire [ 1:0] radio_tx_running_radio0;

  // Memory Controller AXI4 MM buses
  wire [3:0]       dram_ports_awready;
  wire [3:0][0:0]  dram_ports_awid;
  wire [3:0][29:0] dram_ports_awaddr;
  wire [3:0][7:0]  dram_ports_awlen;
  wire [3:0][2:0]  dram_ports_awsize;
  wire [3:0][1:0]  dram_ports_awburst;
  wire [3:0][0:0]  dram_ports_awlock;
  wire [3:0][3:0]  dram_ports_awcache;
  wire [3:0][2:0]  dram_ports_awprot;
  wire [3:0][3:0]  dram_ports_awqos;
  wire [3:0][3:0]  dram_ports_awregion;
  wire [3:0][0:0]  dram_ports_awuser;
  wire [3:0]       dram_ports_wready;
  wire [3:0][63:0] dram_ports_wdata;
  wire [3:0][7:0]  dram_ports_wstrb;
  wire [3:0][0:0]  dram_ports_wuser;
  wire [3:0]       dram_ports_bvalid;
  wire [3:0][0:0]  dram_ports_bid;
  wire [3:0][1:0]  dram_ports_bresp;
  wire [3:0][0:0]  dram_ports_buser;
  wire [3:0]       dram_ports_arready;
  wire [3:0][0:0]  dram_ports_arid;
  wire [3:0][29:0] dram_ports_araddr;
  wire [3:0][7:0]  dram_ports_arlen;
  wire [3:0][2:0]  dram_ports_arsize;
  wire [3:0][1:0]  dram_ports_arburst;
  wire [3:0][0:0]  dram_ports_arlock;
  wire [3:0][3:0]  dram_ports_arcache;
  wire [3:0][2:0]  dram_ports_arprot;
  wire [3:0][3:0]  dram_ports_arqos;
  wire [3:0][3:0]  dram_ports_arregion;
  wire [3:0][0:0]  dram_ports_aruser;
  wire [3:0]       dram_ports_rlast;
  wire [3:0]       dram_ports_rvalid;
  wire [3:0]       dram_ports_awvalid;
  wire [3:0]       dram_ports_wlast;
  wire [3:0]       dram_ports_wvalid;
  wire [3:0]       dram_ports_bready;
  wire [3:0]       dram_ports_arvalid;
  wire [3:0]       dram_ports_rready;
  wire [3:0][0:0]  dram_ports_rid;
  wire [3:0][63:0] dram_ports_rdata;
  wire [3:0][1:0]  dram_ports_rresp;
  wire [3:0][0:0]  dram_ports_ruser;

  //vhook_e rfnoc_image_core rfnoc_image_core_i
  //vhook_a  PORT_W                   CHDR_W
  //vhook_a  chdr_aclk                bus_clk
  //vhook_a  ctrl_aclk                clk_40mhz
  //vhook_a  core_arst                bus_rst
  //vhook_a  ce_clk                   ce_clk
  //vhook_a  dram_clk                 ddr3_axi_clk_x2
  //vhook_a {^s_pcie(.*)}             m_chdr$1
  //vhook_a {^m_pcie(.*)}             s_chdr$1
  //vhook_a {^m_ctrlport_radio0_(.*)} radio_ctrlport_$1
  //vhook_a  axi_rst                  ddr3_axi_rst
  //vhook_a {^m_axi_((awuser)|(wuser)|(buser)|(ruser)|(aruser))} {}
  //vhook_a  {^m_axi(.*)} {s00_ddr_axi$1, s01_ddr_axi$1}
  //vhook_a  radio_rx_data_radio0     radio_rx_data
  //vhook_a  radio_rx_stb_radio0      {NUM_CH_PER_RADIO{radio_rx_data_valid}}
  //vhook_a  radio_tx_data_radio0     radio_tx_data
  //vhook_a  radio_tx_stb_radio0      {NUM_CH_PER_RADIO{radio_tx_ready}}
  //vhook_a  dna                      dev_dna
  rfnoc_image_core #(
    .CHDR_W    (CHDR_W),     //wire[31:0]:=128
    .PORT_W    (CHDR_W),     //wire[31:0]:=128
    .PCIE_W    (PCIE_W),     //wire[31:0]:=128
    .MTU       (MTU),        //integer:=10
    .PROTOVER  (PROTOVER),   //wire[15:0]:={8'b01,8'b0}
    .RADIO_NIPC(RADIO_NIPC)  //integer:=1
  ) rfnoc_image_core_i (
    .chdr_aclk                     (bus_clk),                                  //input wire
    .ctrl_aclk                     (clk_40mhz),                                //input wire
    .core_arst                     (bus_rst),                                  //input wire
    .radio_clk                     (radio_clk),                                //input wire
    .ce_clk                        (ce_clk),                                   //input wire
    .dram_clk                      (ddr3_axi_clk_x2),                          //input wire
    .device_id                     (device_id),                                //input wire[15:0]
    .m_ctrlport_radio0_req_wr      (radio_ctrlport_req_wr),                    //output wire
    .m_ctrlport_radio0_req_rd      (radio_ctrlport_req_rd),                    //output wire
    .m_ctrlport_radio0_req_addr    (radio_ctrlport_req_addr),                  //output wire[19:0]
    .m_ctrlport_radio0_req_data    (radio_ctrlport_req_data),                  //output wire[31:0]
    .m_ctrlport_radio0_req_byte_en (radio_ctrlport_req_byte_en),               //output wire[3:0]
    .m_ctrlport_radio0_req_has_time(radio_ctrlport_req_has_time),              //output wire
    .m_ctrlport_radio0_req_time    (radio_ctrlport_req_time),                  //output wire[63:0]
    .m_ctrlport_radio0_resp_ack    (radio_ctrlport_resp_ack),                  //input wire
    .m_ctrlport_radio0_resp_status (radio_ctrlport_resp_status),               //input wire[1:0]
    .m_ctrlport_radio0_resp_data   (radio_ctrlport_resp_data),                 //input wire[31:0]
    .radio_time                    (radio_time),                               //input wire[63:0]
    .radio_rx_data_radio0          (radio_rx_data),                            //input wire[63:0]
    .radio_rx_stb_radio0           ({NUM_CH_PER_RADIO{radio_rx_data_valid}}),  //input wire[1:0]
    .radio_rx_running_radio0       (radio_rx_running_radio0),                  //output wire[1:0]
    .radio_tx_data_radio0          (radio_tx_data),                            //output wire[63:0]
    .radio_tx_stb_radio0           ({NUM_CH_PER_RADIO{radio_tx_ready}}),       //input wire[1:0]
    .radio_tx_running_radio0       (radio_tx_running_radio0),                  //output wire[1:0]
    .axi_rst                       (ddr3_axi_rst),                             //input wire
    .m_axi_awid                    (dram_ports_awid),                          //output wire[7:0]
    .m_axi_awaddr                  (dram_ports_awaddr),                        //output wire[383:0]
    .m_axi_awlen                   (dram_ports_awlen),                         //output wire[63:0]
    .m_axi_awsize                  (dram_ports_awsize),                        //output wire[23:0]
    .m_axi_awburst                 (dram_ports_awburst),                       //output wire[15:0]
    .m_axi_awlock                  (dram_ports_awlock),                        //output wire[7:0]
    .m_axi_awcache                 (dram_ports_awcache),                       //output wire[31:0]
    .m_axi_awprot                  (dram_ports_awprot),                        //output wire[23:0]
    .m_axi_awqos                   (dram_ports_awqos),                         //output wire[31:0]
    .m_axi_awregion                (dram_ports_awregion),                      //output wire[31:0]
    .m_axi_awuser                  (),                                         //output wire[7:0]
    .m_axi_awvalid                 (dram_ports_awvalid),                       //output wire[7:0]
    .m_axi_awready                 (dram_ports_awready),                       //input wire[7:0]
    .m_axi_wdata                   (dram_ports_wdata),                         //output wire[4191:0]
    .m_axi_wstrb                   (dram_ports_wstrb),                         //output wire[511:0]
    .m_axi_wlast                   (dram_ports_wlast),                         //output wire[7:0]
    .m_axi_wuser                   (),                                         //output wire[7:0]
    .m_axi_wvalid                  (dram_ports_wvalid),                        //output wire[7:0]
    .m_axi_wready                  (dram_ports_wready),                        //input wire[7:0]
    .m_axi_bid                     (dram_ports_bid),                           //input wire[7:0]
    .m_axi_bresp                   (dram_ports_bresp),                         //input wire[15:0]
    .m_axi_buser                   (),                                         //input wire[7:0]
    .m_axi_bvalid                  (dram_ports_bvalid),                        //input wire[7:0]
    .m_axi_bready                  (dram_ports_bready),                        //output wire[7:0]
    .m_axi_arid                    (dram_ports_arid),                          //output wire[7:0]
    .m_axi_araddr                  (dram_ports_araddr),                        //output wire[383:0]
    .m_axi_arlen                   (dram_ports_arlen),                         //output wire[63:0]
    .m_axi_arsize                  (dram_ports_arsize),                        //output wire[23:0]
    .m_axi_arburst                 (dram_ports_arburst),                       //output wire[15:0]
    .m_axi_arlock                  (dram_ports_arlock),                        //output wire[7:0]
    .m_axi_arcache                 (dram_ports_arcache),                       //output wire[31:0]
    .m_axi_arprot                  (dram_ports_arprot),                        //output wire[31:0]
    .m_axi_arqos                   (dram_ports_arqos),                         //output wire[31:0]
    .m_axi_arregion                (dram_ports_arregion),                      //output wire[31:0]
    .m_axi_aruser                  (),                                         //output wire[7:0]
    .m_axi_arvalid                 (dram_ports_arvalid),                       //output wire[7:0]
    .m_axi_arready                 (dram_ports_arready),                       //input wire[7:0]
    .m_axi_rid                     (dram_ports_rid),                           //input wire[7:0]
    .m_axi_rdata                   (dram_ports_rdata),                         //input wire[4191:0]
    .m_axi_rresp                   (dram_ports_rresp),                         //input wire[15:0]
    .m_axi_rlast                   (dram_ports_rlast),                         //input wire[7:0]
    .m_axi_ruser                   (),                                         //input wire[7:0]
    .m_axi_rvalid                  (dram_ports_rvalid),                        //input wire[7:0]
    .m_axi_rready                  (dram_ports_rready),                        //output wire[7:0]
    .dna                           (dev_dna),                                  //output wire[63:0]
    .s_pcie_tdata                  (m_chdr_tdata),                             //input wire[(PORT_W-1):0]
    .s_pcie_tlast                  (m_chdr_tlast),                             //input wire
    .s_pcie_tvalid                 (m_chdr_tvalid),                            //input wire
    .s_pcie_tready                 (m_chdr_tready),                            //output wire
    .m_pcie_tdata                  (s_chdr_tdata),                             //output wire[(PORT_W-1):0]
    .m_pcie_tlast                  (s_chdr_tlast),                             //output wire
    .m_pcie_tvalid                 (s_chdr_tvalid),                            //output wire
    .m_pcie_tready                 (s_chdr_tready)                             //input wire
  );

  `ifndef DRAM_CH
    `define DRAM_CH 0
  `endif

  if (`DRAM_CH >= 1 && `DRAM_CH <= 2) begin : gen_2_port_interconnect
  axi_intercon_2x64_256_bd axi_intercon_2x64_256_bd_i (
    .M00_AXI_ACLK     (ddr3_axi_clk          ),
    .M00_AXI_ARESETN  (~ddr3_axi_rst         ),
    .M00_AXI_araddr   (ddr3_axi_araddr       ),
    .M00_AXI_arburst  (ddr3_axi_arburst      ),
    .M00_AXI_arcache  (ddr3_axi_arcache      ),
    .M00_AXI_arid     (ddr3_axi_arid         ),
    .M00_AXI_arlen    (ddr3_axi_arlen        ),
    .M00_AXI_arlock   (ddr3_axi_arlock       ),
    .M00_AXI_arprot   (ddr3_axi_arprot       ),
    .M00_AXI_arqos    (ddr3_axi_arqos        ),
    .M00_AXI_arready  (ddr3_axi_arready      ),
    .M00_AXI_arregion (                      ),
    .M00_AXI_arsize   (ddr3_axi_arsize       ),
    .M00_AXI_arvalid  (ddr3_axi_arvalid      ),
    .M00_AXI_awaddr   (ddr3_axi_awaddr       ),
    .M00_AXI_awburst  (ddr3_axi_awburst      ),
    .M00_AXI_awcache  (ddr3_axi_awcache      ),
    .M00_AXI_awid     (ddr3_axi_awid         ),
    .M00_AXI_awlen    (ddr3_axi_awlen        ),
    .M00_AXI_awlock   (ddr3_axi_awlock       ),
    .M00_AXI_awprot   (ddr3_axi_awprot       ),
    .M00_AXI_awqos    (ddr3_axi_awqos        ),
    .M00_AXI_awready  (ddr3_axi_awready      ),
    .M00_AXI_awregion (                      ),
    .M00_AXI_awsize   (ddr3_axi_awsize       ),
    .M00_AXI_awvalid  (ddr3_axi_awvalid      ),
    .M00_AXI_bid      (ddr3_axi_bid          ),
    .M00_AXI_bready   (ddr3_axi_bready       ),
    .M00_AXI_bresp    (ddr3_axi_bresp        ),
    .M00_AXI_bvalid   (ddr3_axi_bvalid       ),
    .M00_AXI_rdata    (ddr3_axi_rdata        ),
    .M00_AXI_rid      (ddr3_axi_rid          ),
    .M00_AXI_rlast    (ddr3_axi_rlast        ),
    .M00_AXI_rready   (ddr3_axi_rready       ),
    .M00_AXI_rresp    (ddr3_axi_rresp        ),
    .M00_AXI_rvalid   (ddr3_axi_rvalid       ),
    .M00_AXI_wdata    (ddr3_axi_wdata        ),
    .M00_AXI_wlast    (ddr3_axi_wlast        ),
    .M00_AXI_wready   (ddr3_axi_wready       ),
    .M00_AXI_wstrb    (ddr3_axi_wstrb        ),
    .M00_AXI_wvalid   (ddr3_axi_wvalid       ),
    // Slave 0 AXI Ports
    .S00_AXI_ACLK     (ddr3_axi_clk_x2       ),
    .S00_AXI_ARESETN  (~ddr3_axi_rst         ),
    .S00_AXI_araddr   (dram_ports_araddr  [0]),
    .S00_AXI_arburst  (dram_ports_arburst [0]),
    .S00_AXI_arcache  (dram_ports_arcache [0]),
    .S00_AXI_arid     (dram_ports_arid    [0]),
    .S00_AXI_arlen    (dram_ports_arlen   [0]),
    .S00_AXI_arlock   (dram_ports_arlock  [0]),
    .S00_AXI_arprot   (dram_ports_arprot  [0]),
    .S00_AXI_arqos    (dram_ports_arqos   [0]),
    .S00_AXI_arready  (dram_ports_arready [0]),
    .S00_AXI_arregion (dram_ports_arregion[0]),
    .S00_AXI_arsize   (dram_ports_arsize  [0]),
    .S00_AXI_arvalid  (dram_ports_arvalid [0]),
    .S00_AXI_awaddr   (dram_ports_awaddr  [0]),
    .S00_AXI_awburst  (dram_ports_awburst [0]),
    .S00_AXI_awcache  (dram_ports_awcache [0]),
    .S00_AXI_awid     (dram_ports_awid    [0]),
    .S00_AXI_awlen    (dram_ports_awlen   [0]),
    .S00_AXI_awlock   (dram_ports_awlock  [0]),
    .S00_AXI_awprot   (dram_ports_awprot  [0]),
    .S00_AXI_awqos    (dram_ports_awqos   [0]),
    .S00_AXI_awready  (dram_ports_awready [0]),
    .S00_AXI_awregion (dram_ports_awregion[0]),
    .S00_AXI_awsize   (dram_ports_awsize  [0]),
    .S00_AXI_awvalid  (dram_ports_awvalid [0]),
    .S00_AXI_bid      (dram_ports_bid     [0]),
    .S00_AXI_bready   (dram_ports_bready  [0]),
    .S00_AXI_bresp    (dram_ports_bresp   [0]),
    .S00_AXI_bvalid   (dram_ports_bvalid  [0]),
    .S00_AXI_rdata    (dram_ports_rdata   [0]),
    .S00_AXI_rid      (dram_ports_rid     [0]),
    .S00_AXI_rlast    (dram_ports_rlast   [0]),
    .S00_AXI_rready   (dram_ports_rready  [0]),
    .S00_AXI_rresp    (dram_ports_rresp   [0]),
    .S00_AXI_rvalid   (dram_ports_rvalid  [0]),
    .S00_AXI_wdata    (dram_ports_wdata   [0]),
    .S00_AXI_wlast    (dram_ports_wlast   [0]),
    .S00_AXI_wready   (dram_ports_wready  [0]),
    .S00_AXI_wstrb    (dram_ports_wstrb   [0]),
    .S00_AXI_wvalid   (dram_ports_wvalid  [0]),
    // Slave 1 AXI Ports
    .S01_AXI_ACLK     (ddr3_axi_clk_x2       ),
    .S01_AXI_ARESETN  (~ddr3_axi_rst         ),
    .S01_AXI_araddr   (dram_ports_araddr  [1]),
    .S01_AXI_arburst  (dram_ports_arburst [1]),
    .S01_AXI_arcache  (dram_ports_arcache [1]),
    .S01_AXI_arid     (dram_ports_arid    [1]),
    .S01_AXI_arlen    (dram_ports_arlen   [1]),
    .S01_AXI_arlock   (dram_ports_arlock  [1]),
    .S01_AXI_arprot   (dram_ports_arprot  [1]),
    .S01_AXI_arqos    (dram_ports_arqos   [1]),
    .S01_AXI_arready  (dram_ports_arready [1]),
    .S01_AXI_arregion (dram_ports_arregion[1]),
    .S01_AXI_arsize   (dram_ports_arsize  [1]),
    .S01_AXI_arvalid  (dram_ports_arvalid [1]),
    .S01_AXI_awaddr   (dram_ports_awaddr  [1]),
    .S01_AXI_awburst  (dram_ports_awburst [1]),
    .S01_AXI_awcache  (dram_ports_awcache [1]),
    .S01_AXI_awid     (dram_ports_awid    [1]),
    .S01_AXI_awlen    (dram_ports_awlen   [1]),
    .S01_AXI_awlock   (dram_ports_awlock  [1]),
    .S01_AXI_awprot   (dram_ports_awprot  [1]),
    .S01_AXI_awqos    (dram_ports_awqos   [1]),
    .S01_AXI_awready  (dram_ports_awready [1]),
    .S01_AXI_awregion (dram_ports_awregion[1]),
    .S01_AXI_awsize   (dram_ports_awsize  [1]),
    .S01_AXI_awvalid  (dram_ports_awvalid [1]),
    .S01_AXI_bid      (dram_ports_bid     [1]),
    .S01_AXI_bready   (dram_ports_bready  [1]),
    .S01_AXI_bresp    (dram_ports_bresp   [1]),
    .S01_AXI_bvalid   (dram_ports_bvalid  [1]),
    .S01_AXI_rdata    (dram_ports_rdata   [1]),
    .S01_AXI_rid      (dram_ports_rid     [1]),
    .S01_AXI_rlast    (dram_ports_rlast   [1]),
    .S01_AXI_rready   (dram_ports_rready  [1]),
    .S01_AXI_rresp    (dram_ports_rresp   [1]),
    .S01_AXI_rvalid   (dram_ports_rvalid  [1]),
    .S01_AXI_wdata    (dram_ports_wdata   [1]),
    .S01_AXI_wlast    (dram_ports_wlast   [1]),
    .S01_AXI_wready   (dram_ports_wready  [1]),
    .S01_AXI_wstrb    (dram_ports_wstrb   [1]),
    .S01_AXI_wvalid   (dram_ports_wvalid  [1]) 
  );
  end : gen_2_port_interconnect
  else if (`DRAM_CH >= 3 && `DRAM_CH <= 4) begin : gen_4_port_interconnect
  axi_intercon_4x64_256_bd axi_intercon_4x64_256_bd_i (
    .M00_AXI_ACLK     (ddr3_axi_clk          ),
    .M00_AXI_ARESETN  (~ddr3_axi_rst         ),
    .M00_AXI_araddr   (ddr3_axi_araddr       ),
    .M00_AXI_arburst  (ddr3_axi_arburst      ),
    .M00_AXI_arcache  (ddr3_axi_arcache      ),
    .M00_AXI_arid     (ddr3_axi_arid         ),
    .M00_AXI_arlen    (ddr3_axi_arlen        ),
    .M00_AXI_arlock   (ddr3_axi_arlock       ),
    .M00_AXI_arprot   (ddr3_axi_arprot       ),
    .M00_AXI_arqos    (ddr3_axi_arqos        ),
    .M00_AXI_arready  (ddr3_axi_arready      ),
    .M00_AXI_arregion (                      ),
    .M00_AXI_arsize   (ddr3_axi_arsize       ),
    .M00_AXI_arvalid  (ddr3_axi_arvalid      ),
    .M00_AXI_awaddr   (ddr3_axi_awaddr       ),
    .M00_AXI_awburst  (ddr3_axi_awburst      ),
    .M00_AXI_awcache  (ddr3_axi_awcache      ),
    .M00_AXI_awid     (ddr3_axi_awid         ),
    .M00_AXI_awlen    (ddr3_axi_awlen        ),
    .M00_AXI_awlock   (ddr3_axi_awlock       ),
    .M00_AXI_awprot   (ddr3_axi_awprot       ),
    .M00_AXI_awqos    (ddr3_axi_awqos        ),
    .M00_AXI_awready  (ddr3_axi_awready      ),
    .M00_AXI_awregion (                      ),
    .M00_AXI_awsize   (ddr3_axi_awsize       ),
    .M00_AXI_awvalid  (ddr3_axi_awvalid      ),
    .M00_AXI_bid      (ddr3_axi_bid          ),
    .M00_AXI_bready   (ddr3_axi_bready       ),
    .M00_AXI_bresp    (ddr3_axi_bresp        ),
    .M00_AXI_bvalid   (ddr3_axi_bvalid       ),
    .M00_AXI_rdata    (ddr3_axi_rdata        ),
    .M00_AXI_rid      (ddr3_axi_rid          ),
    .M00_AXI_rlast    (ddr3_axi_rlast        ),
    .M00_AXI_rready   (ddr3_axi_rready       ),
    .M00_AXI_rresp    (ddr3_axi_rresp        ),
    .M00_AXI_rvalid   (ddr3_axi_rvalid       ),
    .M00_AXI_wdata    (ddr3_axi_wdata        ),
    .M00_AXI_wlast    (ddr3_axi_wlast        ),
    .M00_AXI_wready   (ddr3_axi_wready       ),
    .M00_AXI_wstrb    (ddr3_axi_wstrb        ),
    .M00_AXI_wvalid   (ddr3_axi_wvalid       ),
    // Slave 0 AXI Ports
    .S00_AXI_ACLK     (ddr3_axi_clk_x2       ),
    .S00_AXI_ARESETN  (~ddr3_axi_rst         ),
    .S00_AXI_araddr   (dram_ports_araddr  [0]),
    .S00_AXI_arburst  (dram_ports_arburst [0]),
    .S00_AXI_arcache  (dram_ports_arcache [0]),
    .S00_AXI_arid     (dram_ports_arid    [0]),
    .S00_AXI_arlen    (dram_ports_arlen   [0]),
    .S00_AXI_arlock   (dram_ports_arlock  [0]),
    .S00_AXI_arprot   (dram_ports_arprot  [0]),
    .S00_AXI_arqos    (dram_ports_arqos   [0]),
    .S00_AXI_arready  (dram_ports_arready [0]),
    .S00_AXI_arregion (dram_ports_arregion[0]),
    .S00_AXI_arsize   (dram_ports_arsize  [0]),
    .S00_AXI_arvalid  (dram_ports_arvalid [0]),
    .S00_AXI_awaddr   (dram_ports_awaddr  [0]),
    .S00_AXI_awburst  (dram_ports_awburst [0]),
    .S00_AXI_awcache  (dram_ports_awcache [0]),
    .S00_AXI_awid     (dram_ports_awid    [0]),
    .S00_AXI_awlen    (dram_ports_awlen   [0]),
    .S00_AXI_awlock   (dram_ports_awlock  [0]),
    .S00_AXI_awprot   (dram_ports_awprot  [0]),
    .S00_AXI_awqos    (dram_ports_awqos   [0]),
    .S00_AXI_awready  (dram_ports_awready [0]),
    .S00_AXI_awregion (dram_ports_awregion[0]),
    .S00_AXI_awsize   (dram_ports_awsize  [0]),
    .S00_AXI_awvalid  (dram_ports_awvalid [0]),
    .S00_AXI_bid      (dram_ports_bid     [0]),
    .S00_AXI_bready   (dram_ports_bready  [0]),
    .S00_AXI_bresp    (dram_ports_bresp   [0]),
    .S00_AXI_bvalid   (dram_ports_bvalid  [0]),
    .S00_AXI_rdata    (dram_ports_rdata   [0]),
    .S00_AXI_rid      (dram_ports_rid     [0]),
    .S00_AXI_rlast    (dram_ports_rlast   [0]),
    .S00_AXI_rready   (dram_ports_rready  [0]),
    .S00_AXI_rresp    (dram_ports_rresp   [0]),
    .S00_AXI_rvalid   (dram_ports_rvalid  [0]),
    .S00_AXI_wdata    (dram_ports_wdata   [0]),
    .S00_AXI_wlast    (dram_ports_wlast   [0]),
    .S00_AXI_wready   (dram_ports_wready  [0]),
    .S00_AXI_wstrb    (dram_ports_wstrb   [0]),
    .S00_AXI_wvalid   (dram_ports_wvalid  [0]),
    // Slave 1 AXI Ports
    .S01_AXI_ACLK     (ddr3_axi_clk_x2       ),
    .S01_AXI_ARESETN  (~ddr3_axi_rst         ),
    .S01_AXI_araddr   (dram_ports_araddr  [1]),
    .S01_AXI_arburst  (dram_ports_arburst [1]),
    .S01_AXI_arcache  (dram_ports_arcache [1]),
    .S01_AXI_arid     (dram_ports_arid    [1]),
    .S01_AXI_arlen    (dram_ports_arlen   [1]),
    .S01_AXI_arlock   (dram_ports_arlock  [1]),
    .S01_AXI_arprot   (dram_ports_arprot  [1]),
    .S01_AXI_arqos    (dram_ports_arqos   [1]),
    .S01_AXI_arready  (dram_ports_arready [1]),
    .S01_AXI_arregion (dram_ports_arregion[1]),
    .S01_AXI_arsize   (dram_ports_arsize  [1]),
    .S01_AXI_arvalid  (dram_ports_arvalid [1]),
    .S01_AXI_awaddr   (dram_ports_awaddr  [1]),
    .S01_AXI_awburst  (dram_ports_awburst [1]),
    .S01_AXI_awcache  (dram_ports_awcache [1]),
    .S01_AXI_awid     (dram_ports_awid    [1]),
    .S01_AXI_awlen    (dram_ports_awlen   [1]),
    .S01_AXI_awlock   (dram_ports_awlock  [1]),
    .S01_AXI_awprot   (dram_ports_awprot  [1]),
    .S01_AXI_awqos    (dram_ports_awqos   [1]),
    .S01_AXI_awready  (dram_ports_awready [1]),
    .S01_AXI_awregion (dram_ports_awregion[1]),
    .S01_AXI_awsize   (dram_ports_awsize  [1]),
    .S01_AXI_awvalid  (dram_ports_awvalid [1]),
    .S01_AXI_bid      (dram_ports_bid     [1]),
    .S01_AXI_bready   (dram_ports_bready  [1]),
    .S01_AXI_bresp    (dram_ports_bresp   [1]),
    .S01_AXI_bvalid   (dram_ports_bvalid  [1]),
    .S01_AXI_rdata    (dram_ports_rdata   [1]),
    .S01_AXI_rid      (dram_ports_rid     [1]),
    .S01_AXI_rlast    (dram_ports_rlast   [1]),
    .S01_AXI_rready   (dram_ports_rready  [1]),
    .S01_AXI_rresp    (dram_ports_rresp   [1]),
    .S01_AXI_rvalid   (dram_ports_rvalid  [1]),
    .S01_AXI_wdata    (dram_ports_wdata   [1]),
    .S01_AXI_wlast    (dram_ports_wlast   [1]),
    .S01_AXI_wready   (dram_ports_wready  [1]),
    .S01_AXI_wstrb    (dram_ports_wstrb   [1]),
    .S01_AXI_wvalid   (dram_ports_wvalid  [1]),
    // Slave 2 AXI Ports
    .S02_AXI_ACLK     (ddr3_axi_clk_x2       ),
    .S02_AXI_ARESETN  (~ddr3_axi_rst         ),
    .S02_AXI_araddr   (dram_ports_araddr  [2]),
    .S02_AXI_arburst  (dram_ports_arburst [2]),
    .S02_AXI_arcache  (dram_ports_arcache [2]),
    .S02_AXI_arid     (dram_ports_arid    [2]),
    .S02_AXI_arlen    (dram_ports_arlen   [2]),
    .S02_AXI_arlock   (dram_ports_arlock  [2]),
    .S02_AXI_arprot   (dram_ports_arprot  [2]),
    .S02_AXI_arqos    (dram_ports_arqos   [2]),
    .S02_AXI_arready  (dram_ports_arready [2]),
    .S02_AXI_arregion (dram_ports_arregion[2]),
    .S02_AXI_arsize   (dram_ports_arsize  [2]),
    .S02_AXI_arvalid  (dram_ports_arvalid [2]),
    .S02_AXI_awaddr   (dram_ports_awaddr  [2]),
    .S02_AXI_awburst  (dram_ports_awburst [2]),
    .S02_AXI_awcache  (dram_ports_awcache [2]),
    .S02_AXI_awid     (dram_ports_awid    [2]),
    .S02_AXI_awlen    (dram_ports_awlen   [2]),
    .S02_AXI_awlock   (dram_ports_awlock  [2]),
    .S02_AXI_awprot   (dram_ports_awprot  [2]),
    .S02_AXI_awqos    (dram_ports_awqos   [2]),
    .S02_AXI_awready  (dram_ports_awready [2]),
    .S02_AXI_awregion (dram_ports_awregion[2]),
    .S02_AXI_awsize   (dram_ports_awsize  [2]),
    .S02_AXI_awvalid  (dram_ports_awvalid [2]),
    .S02_AXI_bid      (dram_ports_bid     [2]),
    .S02_AXI_bready   (dram_ports_bready  [2]),
    .S02_AXI_bresp    (dram_ports_bresp   [2]),
    .S02_AXI_bvalid   (dram_ports_bvalid  [2]),
    .S02_AXI_rdata    (dram_ports_rdata   [2]),
    .S02_AXI_rid      (dram_ports_rid     [2]),
    .S02_AXI_rlast    (dram_ports_rlast   [2]),
    .S02_AXI_rready   (dram_ports_rready  [2]),
    .S02_AXI_rresp    (dram_ports_rresp   [2]),
    .S02_AXI_rvalid   (dram_ports_rvalid  [2]),
    .S02_AXI_wdata    (dram_ports_wdata   [2]),
    .S02_AXI_wlast    (dram_ports_wlast   [2]),
    .S02_AXI_wready   (dram_ports_wready  [2]),
    .S02_AXI_wstrb    (dram_ports_wstrb   [2]),
    .S02_AXI_wvalid   (dram_ports_wvalid  [2]),
    // Slave 3 AXI Ports
    .S03_AXI_ACLK     (ddr3_axi_clk_x2       ),
    .S03_AXI_ARESETN  (~ddr3_axi_rst         ),
    .S03_AXI_araddr   (dram_ports_araddr  [3]),
    .S03_AXI_arburst  (dram_ports_arburst [3]),
    .S03_AXI_arcache  (dram_ports_arcache [3]),
    .S03_AXI_arid     (dram_ports_arid    [3]),
    .S03_AXI_arlen    (dram_ports_arlen   [3]),
    .S03_AXI_arlock   (dram_ports_arlock  [3]),
    .S03_AXI_arprot   (dram_ports_arprot  [3]),
    .S03_AXI_arqos    (dram_ports_arqos   [3]),
    .S03_AXI_arready  (dram_ports_arready [3]),
    .S03_AXI_arregion (dram_ports_arregion[3]),
    .S03_AXI_arsize   (dram_ports_arsize  [3]),
    .S03_AXI_arvalid  (dram_ports_arvalid [3]),
    .S03_AXI_awaddr   (dram_ports_awaddr  [3]),
    .S03_AXI_awburst  (dram_ports_awburst [3]),
    .S03_AXI_awcache  (dram_ports_awcache [3]),
    .S03_AXI_awid     (dram_ports_awid    [3]),
    .S03_AXI_awlen    (dram_ports_awlen   [3]),
    .S03_AXI_awlock   (dram_ports_awlock  [3]),
    .S03_AXI_awprot   (dram_ports_awprot  [3]),
    .S03_AXI_awqos    (dram_ports_awqos   [3]),
    .S03_AXI_awready  (dram_ports_awready [3]),
    .S03_AXI_awregion (dram_ports_awregion[3]),
    .S03_AXI_awsize   (dram_ports_awsize  [3]),
    .S03_AXI_awvalid  (dram_ports_awvalid [3]),
    .S03_AXI_bid      (dram_ports_bid     [3]),
    .S03_AXI_bready   (dram_ports_bready  [3]),
    .S03_AXI_bresp    (dram_ports_bresp   [3]),
    .S03_AXI_bvalid   (dram_ports_bvalid  [3]),
    .S03_AXI_rdata    (dram_ports_rdata   [3]),
    .S03_AXI_rid      (dram_ports_rid     [3]),
    .S03_AXI_rlast    (dram_ports_rlast   [3]),
    .S03_AXI_rready   (dram_ports_rready  [3]),
    .S03_AXI_rresp    (dram_ports_rresp   [3]),
    .S03_AXI_rvalid   (dram_ports_rvalid  [3]),
    .S03_AXI_wdata    (dram_ports_wdata   [3]),
    .S03_AXI_wlast    (dram_ports_wlast   [3]),
    .S03_AXI_wready   (dram_ports_wready  [3]),
    .S03_AXI_wstrb    (dram_ports_wstrb   [3]),
    .S03_AXI_wvalid   (dram_ports_wvalid  [3]) 
  );
  end : gen_4_port_interconnect
  else if (`DRAM_CH < 0 || `DRAM_CH > 4) begin : gen_port_interconnect_error
    $error({"Invalid number of DRAM ports: %0d. ",
      "Number of DRAM ports must from 0 to 4."}, `DRAM_CH);
  end

  //---------------------------------------------------------
  // Radio Ctrlport Breakout
  //---------------------------------------------------------

  ctrlport_if radio_ctrlport_timed (.clk(radio_clk), .rst(radio_rst));

  logic [3:0] time_ignore_bits = $clog2(RADIO_NIPC);

  //vhook_e ctrlport_if_timer radio_ctrlport_timer
  //vhook_a EXEC_LATE_CMDS  1
  //vhook_a clk             radio_clk
  //vhook_a rst             radio_rst
  //vhook_a time_now        radio_time
  //vhook_a time_now_stb    radio_rx_data_valid
  //vhook_a s_ctrlport      radio_ctrlport_if.slave
  //vhook_a m_ctrlport      radio_ctrlport_timed.master
  ctrlport_if_timer #(
    .EXEC_LATE_CMDS(1)  //bit:=1
  ) radio_ctrlport_timer (
    .clk             (radio_clk),                   //input logic
    .rst             (radio_rst),                   //input logic
    .time_now        (radio_time),                  //input logic[63:0]
    .time_now_stb    (radio_rx_data_valid),         //input logic
    .time_ignore_bits(time_ignore_bits),            //input logic[3:0]
    .s_ctrlport      (radio_ctrlport_if.slave),     // ctrlport_if.slave
    .m_ctrlport      (radio_ctrlport_timed.master)  // ctrlport_if.master
  );


  localparam int NUM_RADIO_CTRLPORT_S = 3;

  ctrlport_if m_radio_ctrlport_array [NUM_RADIO_CTRLPORT_S] (.clk(radio_clk), .rst(radio_rst));

  //vhook_e ctrlport_if_decoder radio_ctrlport_if_decoder
  //vhook_a NUM_SLAVES   NUM_RADIO_CTRLPORT_S
  //vhook_a PORT_BASE    '{RADIO0_CONTROL_WINDOW, RADIO_SPI_WINDOW, JESD_CTRL_WINDOW}
  //vhook_a PORT_SIZE    '{RADIO0_CONTROL_WINDOW_SIZE, RADIO_SPI_WINDOW_SIZE, JESD_CTRL_WINDOW_SIZE}
  //vhook_a s_ctrlport   radio_ctrlport_timed.slave
  //vhook_a m_ctrlport   m_radio_ctrlport_array
  ctrlport_if_decoder #(
    .NUM_SLAVES(NUM_RADIO_CTRLPORT_S),                                                        //int:=2
    .PORT_BASE ('{RADIO0_CONTROL_WINDOW, RADIO_SPI_WINDOW, JESD_CTRL_WINDOW}),                //int:='{'b0,'b0100000000}
    .PORT_SIZE ('{RADIO0_CONTROL_WINDOW_SIZE, RADIO_SPI_WINDOW_SIZE, JESD_CTRL_WINDOW_SIZE})  //int:='{'b0100000000,'b0100000000}
  ) radio_ctrlport_if_decoder (
    .s_ctrlport(radio_ctrlport_timed.slave),  // ctrlport_if.slave
    .m_ctrlport(m_radio_ctrlport_array)       // ctrlport_if.master[NUM_SLAVES]
  );

  ctrlport_if m_radio_control_ctrlport (.clk(radio_clk), .rst(radio_rst));
  ctrlport_if m_radio_spi_ctrlport     (.clk(radio_clk), .rst(radio_rst));

  assign m_radio_control_ctrlport.req = m_radio_ctrlport_array[0].req;
  assign m_radio_spi_ctrlport.req     = m_radio_ctrlport_array[1].req;
  assign m_radio_jesd_ctrlport.req    = m_radio_ctrlport_array[2].req;
  assign m_radio_ctrlport_array[0].resp = m_radio_control_ctrlport.resp;
  assign m_radio_ctrlport_array[1].resp = m_radio_spi_ctrlport.resp;
  assign m_radio_ctrlport_array[2].resp = m_radio_jesd_ctrlport.resp;


  localparam int FP_GPIO_WIDTH      = 10;

  logic [FP_GPIO_WIDTH-1:0] radio_ch0_fp_gpio_out;
  logic [FP_GPIO_WIDTH-1:0] radio_ch1_fp_gpio_out;
  logic [FP_GPIO_WIDTH-1:0] radio_ch0_fp_gpio_dir;
  logic [FP_GPIO_WIDTH-1:0] radio_ch1_fp_gpio_dir;

  //vhook_e radio_control_regs radio_control_regs_i
  //vhook_a BASE_ADDRESS  0
  //vhook_a SIZE_ADDRESS  RADIO0_CONTROL_WINDOW_SIZE
  //vhook_a s_ctrlport    m_radio_control_ctrlport
  //vhook_a rx_running    radio_rx_running_radio0
  //vhook_a tx_running    radio_tx_running_radio0
  //vhook_a radio_fp_gpio_in fp_gpio_in
  radio_control_regs #(
    .BASE_ADDRESS(0),                          //logic[19:0]:=0
    .SIZE_ADDRESS(RADIO0_CONTROL_WINDOW_SIZE)  //logic[19:0]:=0
  ) radio_control_regs_i (
    .s_ctrlport             (m_radio_control_ctrlport),  // ctrlport_if.slave
    .radio_adrv_reset       (radio_adrv_reset),          //output logic
    .radio_test_enable      (radio_test_enable),         //output logic
    .radio_led_tx_green     (radio_led_tx_green),        //output logic[1:0]
    .radio_led_tx_red       (radio_led_tx_red),          //output logic[1:0]
    .radio_led_rx_green     (radio_led_rx_green),        //output logic[1:0]
    .radio_ch0_fp_gpio_out  (radio_ch0_fp_gpio_out),     //output logic[9:0]
    .radio_ch1_fp_gpio_out  (radio_ch1_fp_gpio_out),     //output logic[9:0]
    .radio_fp_gpio_in       (fp_gpio_in),                //input wire[9:0]
    .radio_ch0_fp_gpio_dir  (radio_ch0_fp_gpio_dir),     //output logic[9:0]
    .radio_ch1_fp_gpio_dir  (radio_ch1_fp_gpio_dir),     //output logic[9:0]
    .radio_ch0_adrv_gpio_out(radio_ch0_adrv_gpio_out),   //output logic[3:0]
    .radio_ch1_adrv_gpio_out(radio_ch1_adrv_gpio_out),   //output logic[3:0]
    .radio_ch0_adrv_gpio_in (radio_ch0_adrv_gpio_in),    //input wire[3:0]
    .radio_ch1_adrv_gpio_in (radio_ch1_adrv_gpio_in),    //input wire[3:0]
    .radio_ch0_adrv_gpio_dir(radio_ch0_adrv_gpio_dir),   //output logic[3:0]
    .radio_ch1_adrv_gpio_dir(radio_ch1_adrv_gpio_dir),   //output logic[3:0]
    .radio_ch0_adrv_trx_out (radio_ch0_adrv_trx_out),    //output logic[3:0]
    .radio_ch1_adrv_trx_out (radio_ch1_adrv_trx_out),    //output logic[3:0]
    .radio_ch0_adrv_int     (radio_ch0_adrv_int),        //input wire
    .radio_ch1_adrv_int     (radio_ch1_adrv_int),        //input wire
    .radio_enable_tdr       (radio_enable_tdr),          //output logic[1:0]
    .radio_rx_bypass        (radio_rx_bypass),           //output logic[1:0]
    .rx_running             (radio_rx_running_radio0),   //input wire[1:0]
    .tx_running             (radio_tx_running_radio0)    //input wire[1:0]
  );

  //vhook_e ctrlport_spi_adrv ctrlport_spi_adrv_i
  //vhook_g BASE_ADDR                     0
  //vhook_g NUM_BYTES_W                   8
  //vhook_g HALF_PER                      14
  //vhook_g HALF_PER_EN                   0
  //vhook_g HALF_PER_W                    8
  //vhook_g CS_HOLD                       1
  //vhook_g CS_GUARD                      2
  //vhook_gh RX_FIFO_SIZE
  //vhook_a clk                           radio_clk
  //vhook_a rst                           radio_rst
  //vhook_a {^s_ctrlport_(req|resp)_(.*)} m_radio_spi_ctrlport.$1.$2
  //vhook_a mosi                          radio_spi_mosi
  //vhook_a sclk                          radio_spi_sclk
  //vhook_a cs_n                          radio_spi_sen_n
  //vhook_a miso                          radio_spi_miso
  ctrlport_spi_adrv #(
    .BASE_ADDR  (0),   //int:=0
    .NUM_BYTES_W(8),   //int:=8
    .HALF_PER   (14),  //int:=9
    .HALF_PER_W (8),   //int:=8
    .HALF_PER_EN(0),   //bit:=1
    .CS_HOLD    (1),   //bit:=1
    .CS_GUARD   (2)    //int:=2
  ) ctrlport_spi_adrv_i (
    .clk                   (radio_clk),                         //input wire
    .rst                   (radio_rst),                         //input wire
    .s_ctrlport_req_wr     (m_radio_spi_ctrlport.req.wr),       //input wire
    .s_ctrlport_req_rd     (m_radio_spi_ctrlport.req.rd),       //input wire
    .s_ctrlport_req_addr   (m_radio_spi_ctrlport.req.addr),     //input wire[(CTRLPORT_ADDR_W-1):0]
    .s_ctrlport_req_data   (m_radio_spi_ctrlport.req.data),     //input wire[(CTRLPORT_DATA_W-1):0]
    .s_ctrlport_resp_ack   (m_radio_spi_ctrlport.resp.ack),     //output logic
    .s_ctrlport_resp_status(m_radio_spi_ctrlport.resp.status),  //output ctrlport_status_t
    .s_ctrlport_resp_data  (m_radio_spi_ctrlport.resp.data),    //output logic[(CTRLPORT_DATA_W-1):0]
    .sclk                  (radio_spi_sclk),                    //output logic
    .cs_n                  (radio_spi_sen_n),                   //output logic
    .mosi                  (radio_spi_mosi),                    //output logic
    .miso                  (radio_spi_miso)                     //input wire
  );

  //------------------------------------
  // Front-Panel GPIO Source Mux
  //------------------------------------
  localparam int FP_GPIO_SRC_WIDTH  = FP_GPIO_WIDTH*2; // each source-select signal is 2 bits wide

  logic [FP_GPIO_SRC_WIDTH-1:0] rclk_fp_gpio_src;

  // Synchronize the fp_gpio_src signal to the radio_clk domain.
  // This signal should not change during operation, the synchronizer
  // is used to avoid timing issues.

  //vhook_e synchronizer fp_gpio_src_sync
  //vhook_a WIDTH             FP_GPIO_SRC_WIDTH
  //vhook_a STAGES            2
  //vhook_a INITIAL_VAL       '0
  //vhook_a FALSE_PATH_TO_IN  1
  //vhook_a clk               radio_clk
  //vhook_a rst               radio_rst
  //vhook_a in                fp_gpio_src
  //vhook_a out               rclk_fp_gpio_src
  synchronizer #(
    .WIDTH           (FP_GPIO_SRC_WIDTH),  //integer:=1
    .STAGES          (2),                  //integer:=2
    .INITIAL_VAL     ('0),                 //integer:=0
    .FALSE_PATH_TO_IN(1)                   //integer:=1
  ) fp_gpio_src_sync (
    .clk(radio_clk),        //input wire
    .rst(radio_rst),        //input wire
    .in (fp_gpio_src),      //input wire[(WIDTH-1):0]
    .out(rclk_fp_gpio_src)  //output wire[(WIDTH-1):0]
  );


  // For each bit in the front-panel GPIO, mux the output and the direction
  // control bit based on the fp_gpio_src register. The fp_gpio_src register
  // holds 2 bits per GPIO pin, which selects which source to use for GPIO
  // control. Currently, only CH0 and CH1 are supported.
  localparam logic [1:0] FP_GPIO_SRC_CH0 = 2'b00;
  localparam logic [1:0] FP_GPIO_SRC_CH1 = 2'b01;
  genvar fp_gpio_i;
  for (fp_gpio_i = 0; fp_gpio_i < FP_GPIO_WIDTH; fp_gpio_i++) begin : gen_fp_gpio_mux
    always_ff @(posedge radio_clk) begin
      if (radio_rst) begin
        fp_gpio_dir[fp_gpio_i] <= 1'b0;
        fp_gpio_out[fp_gpio_i] <= 1'b0;
      end else begin
        case (rclk_fp_gpio_src[fp_gpio_i*2 +: 2])
          FP_GPIO_SRC_CH0: begin
            fp_gpio_dir[fp_gpio_i] <= radio_ch0_fp_gpio_dir[fp_gpio_i];
            fp_gpio_out[fp_gpio_i] <= radio_ch0_fp_gpio_out[fp_gpio_i];
          end
          FP_GPIO_SRC_CH1: begin
            fp_gpio_dir[fp_gpio_i] <= radio_ch1_fp_gpio_dir[fp_gpio_i];
            fp_gpio_out[fp_gpio_i] <= radio_ch1_fp_gpio_out[fp_gpio_i];
          end
          default: begin
            fp_gpio_dir[fp_gpio_i] <= 1'b0;
            fp_gpio_out[fp_gpio_i] <= 1'b0;
          end
        endcase
      end
    end
  end


endmodule : b310_core

`default_nettype wire

//XmlParse xml_on
//<top name="B310_FPGA" removeunreferenced="true">
//  <regmapcfg readablestrobes="false">
//    <map name="B310_BAR0_REGMAP"/>
//    <map name="B310_RADIO_REGMAP"/>
//  </regmapcfg>
//</top>
//<regmap name="B310_BAR0_REGMAP" readablestrobes="false" generateverilog="true" generatesv="false" ettusguidelines="true">
//  <group name="CORE_CONTROL_REGS">
//    <window name="BASIC_REGS_WINDOW" offset="0x0" targetregmap="BASIC_REGS_REGMAP" size="0x20"/>
//    <window name="CORE_REGS_WINDOW" offset="0x20" targetregmap="CORE_REGS_REGMAP" size="0x50"/>
//    <window name="MBOARD_I2C_WINDOW" offset="0x70" size="0x20"/>
//    <window name="CLOCKING_SPI_WINDOW" offset="0xA0" targetregmap="CTRLPORT_TO_SIMPLE_SPI_REGMAP" size="0x20"/>
//    <window name="THUNDERBOLT_I2C_WINDOW" offset="0xC0" size="0x20"/>
//    <window name="TIMEKEEPER_WINDOW" offset="0x100" size="0x30"/>
//    <window name="GPS_UART_WINDOW" offset="0x130" size="0x14"/>
//    <window name="CPLD_JTAG_WINDOW" offset="0x150" size="0x20"/>
//  </group>
//</regmap>
//<regmap name="B310_RADIO_REGMAP" readablestrobes="false" generateverilog="true" generatesv="false" ettusguidelines="true">
//  <group name="RADIO_CONTROL_REGS">
//    <window name="RADIO0_CONTROL_WINDOW" offset="0x0" targetregmap="RADIO_CONTROL_REGMAP" size="0x100"/>
//    <window name="RADIO_SPI_WINDOW" offset="0x100" targetregmap="CTRLPORT_SPI_ADRV_REGMAP" size="0x100"/>
//    <window name="JESD_CTRL_WINDOW" offset="0x1000" size="0x1000"/>
//  </group>
//</regmap>
//<regmap name="CTRLPORT_TO_SIMPLE_SPI_REGMAP" readablestrobes="false" generateverilog="true" generatesv="false" ettusguidelines="true">
//  <group name="CTRLPORT_TO_SIMPLE_SPI_REGMAP">
//    <info>
//      This regmap contains the settings registers for the SPI engine.
//    </info>
//    <register name="SCLK_DIVIDER" offset="0x0" size="32" attributes="Readable|Writable">
//      <info>
//         Controls clock rate for subsequent SPI transactions.
//      </info>
//     <bitfield name="SPI_CLK_DIV" range="15..0">
//       <info>
//         Clock Divider. f_sclk = f_radio_clk / (2 * (SPI_CLK_DIV + 1))
//       </info>
//     </bitfield>
//   <bitfield name="FORCE_SS" range="16">
//       <info>
//         Force the slave select line to be active regardless of the
//         state of the SPI transaction. This allows for sending longer
//         transactions without having to wait for the slave select line
//         to be deasserted.
//       </info>
//     </bitfield>
//    </register>
//    <register name="SPI_PERSONALITY" offset="0x4" size="32" attributes="Readable|Writable">
//      <info>
//         Controls the personality of the SPI transaction.
//      </info>
//     <bitfield name="SPI_MOSI_EDGE" range="31">
//       <info>
//          Controls the edge in which the MOSI line is updated.</br>
//            0 =  falling edge of SCLK.</br>
//            1 =  rising edge of SCLK.
//       </info>
//     </bitfield>
//     <bitfield name="SPI_MISO_EDGE" range="30">
//       <info>
//          Controls the edge in which the MISO line is sampled.</br>
//            0 =  falling edge of SCLK.</br>
//            1 =  rising edge of SCLK.
//       </info>
//     </bitfield>
//     <bitfield name="SPI_NUM_BITS" range="29..24">
//       <info>
//          Number of bits to transfer in the SPI transaction.</br>
//          The actual length is SPI_NUM_BITS+1.</br>
//       </info>
//     </bitfield>
//     <bitfield name="SPI_SLAVE_SELECT" range="23..0">
//       <info>
//          Controls which slaves are selected for the SPI transaction.
//          Each bit corresponds to a slave, starting with slave 0.
//       </info>
//     </bitfield>
//    </register>
//    <register name="SPI_MOSI_DATA" offset="0x8" size="32" attributes="Writable">
//      <info>
//         Controls the data to be sent on the MOSI line.
//      </info>
//      <bitfield name="SPI_DATA_OUT" range="31..0">
//        <info>
//          The data to be sent on the MOSI line. This data is shifted out
//          starting from bit 31, so it is MSB aligned. Writing this register
//          will trigger a SPI transaction.
//       </info>
//      </bitfield>
//    </register>
//    <register name="SPI_STATUS" offset="0xC" size="32" attributes="Readable">
//      <info>
//         Contains the status for the SPI ENGINE.
//      </info>
//      <bitfield name="SPI_READY" range="0">
//        <info>
//          Indicates if the SPI engine is ready for a new transaction.
//        </info>
//      </bitfield>
//    </register>
//    <register name="SPI_READBACK" offset="0x10" size="32" attributes="Readable">
//      <info>
//         Contains the readback data from the last SPI transaction.
//      </info>
//      <bitfield name="SPI_READBACK_DATA" range="31..0">
//        <info>
//          Contains the readback data from the last SPI transaction.
//          The data is shifted in starting from bit 0, so it is in-order.
//          This register is updated when the SPI transaction is done.
//        </info>
//      </bitfield>
//    </register>
//  </group>
//</regmap>
//<regmap name="CTRLPORT_SPI_ADRV_REGMAP" readablestrobes="false" generateverilog="true" generatesv="false" ettusguidelines="true">
//  <group name="CTRLPORT_SPI_ADRV_REGMAP">
//    <info>
//      Settings registers for the ADRV SPI engine. The SCLK rate is
//      fixed by the HALF_PER parameter; no software-programmable clock
//      divider register is present in this configuration.
//    </info>
//    <register name="REG_SI" offset="0x4" size="32" attributes="Writable">
//      <info>
//        Single-instruction register. Writing this register immediately
//        triggers a 3-byte SPI transaction ([addr_hi, addr_lo,
//        data_byte]) without needing to write REG_CONTROL or REG_DATA.
//        Direction is set by bit 7 of SI_ADDR_HI (ADI ADRV R/W# bit):
//        0 = write, 1 = read. For reads, the 3 received bytes are
//        placed in the RX FIFO and can be read back via REG_DATA.
//      </info>
//      <bitfield name="SI_ADDR_HI" range="7..0">
//        <info>
//          First byte on the wire (high byte of SPI address).
//          Bit 7 is the R/W# direction bit.
//        </info>
//      </bitfield>
//      <bitfield name="SI_ADDR_LO" range="15..8">
//        <info>
//          Second byte on the wire (low byte of the SPI address).
//        </info>
//      </bitfield>
//      <bitfield name="SI_DATA_BYTE" range="23..16">
//        <info>
//          Third byte on the wire (data byte).
//        </info>
//      </bitfield>
//    </register>
//    <register name="REG_CONTROL" offset="0x3C" size="32" attributes="Writable">
//      <info>
//        Transaction control register. Writing this register starts a
//        new SPI transaction.
//      </info>
//      <bitfield name="SPI_ADDR" range="15..0">
//        <info>
//          16-bit SPI register address. Used in SI_SEQ, SI_REP, and
//          STREAMING modes.
//        </info>
//      </bitfield>
//      <bitfield name="SPI_NUM_BYTES" range="27..16">
//        <info>
//          Number of data bytes to transfer.
//        </info>
//      </bitfield>
//      <bitfield name="SPI_MODE" range="29..28">
//        <info>
//          Transaction mode: 0 = RAW, 1 = SI_SEQ, 2 = SI_REP,
//          3 = STREAMING.
//        </info>
//      </bitfield>
//      <bitfield name="SPI_DIR" range="31">
//        <info>
//          Transfer direction: 0 = write, 1 = read.
//        </info>
//      </bitfield>
//    </register>
//    <register name="REG_DATA" offset="0x40" size="32" attributes="Readable|Writable">
//      <info>
//        Data window (16 words, 64 bytes). Any address in 0x40..0x7F maps to
//        this virtual register. When transmitted over SPI, the first byte on
//        the wire is taken from [7:0], the second byte from [15:8], and so on.
//
//        Write: Load the next 32-bit word into the TX FIFO. The CtrlPort ACK
//        is not asserted until the word is accepted.
//
//        Read: Return the next 32-bit word from the RX FIFO. The CtrlPort does
//        not respond to the read until the data is available.
//      </info>
//      <bitfield name="SPI_DATA" range="31..0">
//        <info>
//          32-bit data word (first byte in least-significant position).
//        </info>
//      </bitfield>
//    </register>
//  </group>
//</regmap>
//XmlParse xml_off

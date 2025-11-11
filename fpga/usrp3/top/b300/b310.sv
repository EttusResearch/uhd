//
// Copyright 2025 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: b310
//
// Description: Top-level module for B310 devices.
//

`default_nettype none

module b310
(

  //-------------------------------------------------------------------------
  // Clocks and resets.
  //-------------------------------------------------------------------------

  input  wire   DEVCLK_P,
  input  wire   DEVCLK_N,
  input  wire   FPGA_REFCLK_N,
  input  wire   FPGA_REFCLK_P,
  input  wire   EXT_REFCLK_N,
  input  wire   EXT_REFCLK_P,
  input  wire   JESD_CLK_N,
  input  wire   JESD_CLK_P,
  input  wire   SYSREF_N,
  input  wire   SYSREF_P,
  input  wire   PCIE_REF_CLK_P,
  input  wire   PCIE_REF_CLK_N,

  input  wire   PCIE_PRESENT_N,
  input  wire   PCIE_RESET_N,

  // PCIe lanes
  //vhook_nodgv Pcie[RT]x_[pn]
  input  wire [3:0] PCIE_RX_P,
  input  wire [3:0] PCIE_RX_N,
  output wire [3:0] PCIE_TX_P,
  output wire [3:0] PCIE_TX_N,


  ///////////////////////////////////
  //
  // DRAM Interface
  //
  ///////////////////////////////////

`ifdef ENABLE_DRAM
  inout wire [31:0] ddr3_dq,     // Data pins. Input for Reads, Output for Writes.
  inout wire [ 3:0] ddr3_dqs_n,   // Data Strobes. Input for Reads, Output for Writes.
  inout wire [ 3:0] ddr3_dqs_p,
  //
  output wire [14:0] ddr3_addr,       // Address
  output wire [ 2:0] ddr3_ba,         // Bank Address
  output wire        ddr3_ras_n,      // Row Address Strobe.
  output wire        ddr3_cas_n,      // Column address select
  output wire        ddr3_we_n,       // Write Enable
  output wire        ddr3_reset_n,    // SDRAM reset pin.
  output wire [ 0:0] ddr3_ck_p,       // Differential clock
  output wire [ 0:0] ddr3_ck_n,
  output wire [ 0:0] ddr3_cke,        // Clock Enable
  output wire [ 0:0] ddr3_cs_n,       // Chip Select
  output wire [ 3:0] ddr3_dm,         // Data Mask [3] = UDM.U26, [2] = LDM.U26, ...
  output wire [ 0:0] ddr3_odt,        // On-Die termination enable.
  //
  input wire sys_clk_i,          // 100MHz clock source to generate DDR3 clocking.
`endif

  // Power status
  input  wire         PG_1V2,
  input  wire         TYPEC_PWR_NEGOTIATED,
  input  wire         VBUS_ALERT_N,
  input  wire         GT_25W_PWR_SRC,

  // ADRV GTX pairs
  input  wire [1:0]   JESD_RX_P,                // JESD204 RX differential clock inputs;
  input  wire [1:0]   JESD_RX_N,                // JESD204 RX differential clock inputs;
  output wire [1:0]   JESD_TX_P,                // JESD204 TX differential clock outputs;
  output wire [1:0]   JESD_TX_N,                // JESD204 TX differential clock outputs;

  // ADRV control and GPIO
  inout  wire [1:0]   ADRV_GPINT,
  inout  wire [7:0]   ADRV_GPIO,
  output wire         ADRV_ORXA_CTRL,
  output wire         ADRV_ORXB_CTRL,
  output wire         ADRV_RESET_N,
  output wire         ADRV_SPI_CLK,
  inout  wire         ADRV_SPI_DIO,
  input  wire         ADRV_SPI_DO,
  output wire         ADRV_SPI_EN_N,
  output wire [2:0]   ADRV_SYNCIN_N,
  output wire [2:0]   ADRV_SYNCIN_P,
  input  wire [1:0]   ADRV_SYNCOUT_N,
  input  wire [1:0]   ADRV_SYNCOUT_P,
  output wire         ADRV_TEST_EN,
  output wire         ADRV_TRXA_CTRL,
  output wire         ADRV_TRXB_CTRL,
  output wire         ADRV_TRXC_CTRL,
  output wire         ADRV_TRXD_CTRL,
  output wire         ADRV_TRXE_CTRL,
  output wire         ADRV_TRXF_CTRL,
  output wire         ADRV_TRXG_CTRL,
  output wire         ADRV_TRXH_CTRL,

  // Clock/Time source control and status signals
  output wire         LMK32_CS_N,
  input  wire         LMK32_MISO,
  output wire         LMK32_MOSI,
  output wire         LMK32_RESET,
  output wire         LMK32_SCLK,
  input  wire         LMK32_STATUS,
  output wire         LMK32_VCXO_SEL_122M88,
  output wire         LMK053_CS_N,
  input  wire         LMK053_MISO,
  output wire         LMK053_MOSI,
  output wire         LMK053_SCLK,
  output wire         LMK_SYNC_REVA,
  output wire         LMK_SYNC,

  input  wire         GPS_PPS_OUT,
  input  wire         GPS_PWR_FAULT_N,
  input  wire [1:0]   GPS_REF,
  output wire         GPS_REFSEL,
  output wire         GPS_RESET_N,
  output wire         GPS_SECREF_N,
  output wire         GPS_SECREF_P,
  output wire         GPS_EXTINT,
  input  wire         GPS_UART_TOFPGA,
  output wire         GPS_UART_TOGPS,

  output wire         DAC_CS_N,
  output wire         DAC_MOSI,
  output wire         DAC_SCLK,
  output wire         DAC_CLR_N,

  output wire         NSYNC_GPIO0,
  output wire         NSYNC_PDN,
  input  wire [1:0]   NSYNC_STATUS,

  input  wire         EXT_PPS_IN,
  output wire [3:0]   PPS_OUT,

  output wire         REF_CLK_SEL,
  output wire         TCXO_EN_N,

  // CPLD communication interface
  output wire         CPLD_EXT_RST_R_N,
  output wire         CPLD_OSC_EN,
  output wire         CPLD_RX_CLK,
  output wire         CPLD_RX_CLKEN,
  output wire [7:0]   CPLD_RX_DATA,
  input  wire         CPLD_TX_CLK,
  input  wire         CPLD_TX_CLKEN,
  input  wire [7:0]   CPLD_TX_DATA,
  output wire         PCIE_RESET_N_TO_CPLD,

  // FE control signals
  output wire         ANT_PWR_EN,
  output wire [1:0]   RX_SW_CTRL,
  output wire [1:0]   ENABLE_TXRX_TDR,

  // Thunderbolt control
  inout  wire         TBOLT_SCL,
  inout  wire         TBOLT_SDA,
  output wire         TBOLT_RIDGE_RESET_N,
  output wire         TBOLT_PD_CTRL_RESET,

  // Miscellaneous GPIO
  inout  wire [9:0]   FP_GPIO,

  // Status LEDs
  output wire         LED_CLK_STS_GRN,
  output wire         LED_PWR_STS_BLUE,
  output wire         LED_PWR_STS_ORANGE,
  output wire [1:0]   LED_TX_GRN,
  output wire [1:0]   LED_TX_RED,
  output wire [1:0]   LED_RX_GRN,
  output wire         LED_FPGA_CONFIG_GRN,

  // JTAG
  output wire         JTAG_CPLD_TCK,
  output wire         JTAG_CPLD_TDI,
  input  wire         JTAG_CPLD_TDO,
  output wire         JTAG_CPLD_TMS,
  output wire         JTAG_TBOLT_TCK,
  output wire         JTAG_TBOLT_TDI,
  input  wire         JTAG_TBOLT_TDO,
  output wire         JTAG_TBOLT_TMS,

  // I2C - Temperature and power monitoring
  inout  wire         MISC_I2C_SCL,
  inout  wire         MISC_I2C_SDA,

  // Authentication Interface
  inout  wire         AUTH_SDA

);

  logic pcie_arst;

  //vhook_sigstart
  logic bus_arst;
  logic dma_clk;
  logic lmk_lock_status;
  logic lmk_reset;
  logic lmk_source_select;
  logic radio_clk;
  logic radio_clk_2x;
  logic radio_clk_gen_rst;
  logic ref_clk_source;
  logic tcxo_en;
  //vhook_sigend

  //---------------------------------------------------------------------------
  // Clocking and resets
  //---------------------------------------------------------------------------

  localparam int BUS_CLK_RATE = 150_000_000;

  // Generate internal clocks from available free-running oscillator.
  // This is a 200 MHz clock used as a DDR3 sys clk for KC705.
  // The generated clocks are:
  // - bus_clk: 150 MHz
  // - clk_40mhz: 40 MHz
  // - ce_clk: 200 MHz

  logic bus_clk_in;

  BUFG bufg_bus_clk (
    .I (dma_clk),          // Input clock
    .O (bus_clk_in)       // Output clock
  );

  logic bus_clk;
  logic clk_40mhz;
  logic ce_clk;

  //vhook_e bus_clk_gen bus_clk_gen_i
  //vhook_a clk_in1   bus_clk_in
  //vhook_a clk_out1  bus_clk
  //vhook_a clk_out2  clk_40mhz
  //vhook_a clk_out3  ce_clk
  //vhook_a reset     1'b0
  //vhook_a locked    {}
  bus_clk_gen bus_clk_gen_i (
    .clk_out1(bus_clk),    //output wire
    .clk_out2(clk_40mhz),  //output wire
    .clk_out3(ce_clk),     //output wire
    .reset   (1'b0),       //input wire
    .locked  (),           //output wire
    .clk_in1 (bus_clk_in)  //input wire
  );

  logic radio_clk_gen_locked;
  logic radio_clk_shifted;

  //vhook_e radio_clk_gen radio_clk_gen_i
  //vhook_a clk_in1_p DEVCLK_P
  //vhook_a clk_in1_n DEVCLK_N
  //vhook_a clk_out1  radio_clk
  //vhook_a clk_out2  radio_clk_2x
  //vhook_a clk_out3  {}
  //vhook_a clk_out4  radio_clk_shifted
  //vhook_a reset     radio_clk_gen_rst
  //vhook_a locked    radio_clk_gen_locked
  radio_clk_gen radio_clk_gen_i (
    .clk_out1 (radio_clk),             //output wire
    .clk_out2 (radio_clk_2x),          //output wire
    .clk_out3 (),                      //output wire
    .clk_out4 (radio_clk_shifted),     //output wire
    .reset    (radio_clk_gen_rst),     //input wire
    .locked   (radio_clk_gen_locked),  //output wire
    .clk_in1_p(DEVCLK_P),              //input wire
    .clk_in1_n(DEVCLK_N)               //input wire
  );


  wire bclk_radio_clk_gen_locked;

  //vhook_e synchronizer synchronizer_radio_clk_locked
  //vhook_a WIDTH 1
  //vhook_a STAGES 2
  //vhook_a INITIAL_VAL 0
  //vhook_a FALSE_PATH_TO_IN 1
  //vhook_a clk bus_clk
  //vhook_a rst 1'b0
  //vhook_a in radio_clk_gen_locked
  //vhook_a out bclk_radio_clk_gen_locked
  synchronizer #(
    .WIDTH           (1),  //integer:=1
    .STAGES          (2),  //integer:=2
    .INITIAL_VAL     (0),  //integer:=0
    .FALSE_PATH_TO_IN(1)   //integer:=1
  ) synchronizer_radio_clk_locked (
    .clk(bus_clk),                   //input wire
    .rst(1'b0),                      //input wire
    .in (radio_clk_gen_locked),      //input wire[(WIDTH-1):0]
    .out(bclk_radio_clk_gen_locked)  //output wire[(WIDTH-1):0]
  );

  logic bus_rst;
  logic radio_rst;
  logic clk40_rst;

  //vhook_e reset_sync  bus_rst_sync
  //vhook_a clk         bus_clk
  //vhook_a reset_in    bus_arst
  //vhook_a reset_out   bus_rst
  reset_sync bus_rst_sync (
    .clk      (bus_clk),   //input wire
    .reset_in (bus_arst),  //input wire
    .reset_out(bus_rst)    //output reg
  );

  //vhook_e reset_sync  radio_rst_sync
  //vhook_a clk         radio_clk
  //vhook_a reset_in    bus_arst
  //vhook_a reset_out   radio_rst
  reset_sync radio_rst_sync (
    .clk      (radio_clk),  //input wire
    .reset_in (bus_arst),   //input wire
    .reset_out(radio_rst)   //output reg
  );

  //vhook_e reset_sync  clk40_rst_sync
  //vhook_a clk         clk_40mhz
  //vhook_a reset_in    bus_arst
  //vhook_a reset_out   clk40_rst
  reset_sync clk40_rst_sync (
    .clk      (clk_40mhz),  //input wire
    .reset_in (bus_arst),   //input wire
    .reset_out(clk40_rst)   //output reg
  );

  //---------------------------------------------------------------------------
  // PCI Communication Core
  //---------------------------------------------------------------------------

  localparam int DMA_STREAM_WIDTH = 128; // Width of DMA streams
  localparam int NUM_TX_STREAMS = 5;    // Number of TX streams
  localparam int NUM_RX_STREAMS = 5;    // Number of RX streams

  ctrlport_if host_ctrlport(.clk(bus_clk), .rst(bus_rst));

  // DMA streams for host interface
  wire [NUM_RX_STREAMS-1:0] [DMA_STREAM_WIDTH-1:0]  host_dma_rx_tdata;
  wire [NUM_RX_STREAMS-1:0]                         host_dma_rx_tready;
  wire [NUM_RX_STREAMS-1:0]                         host_dma_rx_tvalid;
  wire [NUM_TX_STREAMS-1:0] [DMA_STREAM_WIDTH-1:0]  host_dma_tx_tdata;
  wire [NUM_TX_STREAMS-1:0]                         host_dma_tx_tready;
  wire [NUM_TX_STREAMS-1:0]                         host_dma_tx_tvalid;

  // PCIe user register interface signals
  logic [19:0]  pcie_usr_ctrlport_req_addr;
  logic [31:0]  pcie_usr_ctrlport_req_data;
  logic         pcie_usr_ctrlport_req_rd;
  logic         pcie_usr_ctrlport_req_wr;
  logic         pcie_usr_ctrlport_resp_ack;
  logic [31:0]  pcie_usr_ctrlport_resp_data;
  logic [ 1:0]  pcie_usr_ctrlport_resp_status;

  // CPLD register interface signals
  wire [50:0] cpld_int_report_in;
  wire [33:0] cpld_int_report_out;

  logic auth_sda_in;
  logic auth_sda_out;

  // OpenDrainIoBuf: IOBUF
  //   port map (
  //     O  => aAuthSdaIn,   --out std_ulogic
  //     IO => aAuthSda,     --inout std_ulogic
  //     I  => '0',          --in  std_ulogic
  //     T  => aAuthSdaOut); --in  std_ulogic
  IOBUF auth_sda_iobuf (
    .O  (auth_sda_in),   //out std_ulogic
    .IO (AUTH_SDA),      //inout std_ulogic
    .I  ('b0),           //in  std_ulogic
    .T  (auth_sda_out)   //in  std_ulogic
  );

  //vhook_e b310_g2x4_host_interface b310_host_interface_i
  //vhook_a pcie_rx_p                           PCIE_RX_P
  //vhook_a pcie_rx_n                           PCIE_RX_N
  //vhook_a pcie_tx_p                           PCIE_TX_P
  //vhook_a pcie_tx_n                           PCIE_TX_N
  //vhook_a pcie_ref_clk_p                      PCIE_REF_CLK_P
  //vhook_a pcie_ref_clk_n                      PCIE_REF_CLK_N
  //vhook_a core_ctrlport_req_time              host_ctrlport.req.timestamp
  //vhook_a {^core_ctrlport_req_rem(.*)}        host_ctrlport.req.remote$1
  //vhook_a {^core_ctrlport_req_(.*)}           host_ctrlport.req.$1
  //vhook_a {^core_ctrlport_resp_(.*)}          host_ctrlport.resp.$1
  //vhook_a {^pcie_usr_ctrlport_req_rem(.*)}    {}
  //vhook_a pcie_usr_ctrlport_req_byte_en       {}
  //vhook_a pcie_usr_ctrlport_req_has_time      {}
  //vhook_a pcie_usr_ctrlport_req_time          {}
  //vhook_a pcie_usr_ctrlport_req_port_id       {}
  b310_g2x4_host_interface b310_host_interface_i (
    .pcie_rx_p                       (PCIE_RX_P),                        //in  std_logic_vector(3:0)
    .pcie_rx_n                       (PCIE_RX_N),                        //in  std_logic_vector(3:0)
    .pcie_tx_p                       (PCIE_TX_P),                        //out std_logic_vector(3:0)
    .pcie_tx_n                       (PCIE_TX_N),                        //out std_logic_vector(3:0)
    .pcie_ref_clk_p                  (PCIE_REF_CLK_P),                   //in  std_logic
    .pcie_ref_clk_n                  (PCIE_REF_CLK_N),                   //in  std_logic
    .clk_40mhz                       (clk_40mhz),                        //in  std_logic
    .bus_clk                         (bus_clk),                          //in  std_logic
    .dma_clk                         (dma_clk),                          //out std_logic
    .pcie_arst                       (pcie_arst),                        //in  std_logic
    .bus_arst                        (bus_arst),                         //out boolean
    .host_dma_rx_tdata               (host_dma_rx_tdata),                //in  std_logic_vector(639:0)
    .host_dma_rx_tready              (host_dma_rx_tready),               //out std_logic_vector(4:0)
    .host_dma_rx_tvalid              (host_dma_rx_tvalid),               //in  std_logic_vector(4:0)
    .host_dma_tx_tdata               (host_dma_tx_tdata),                //out std_logic_vector(639:0)
    .host_dma_tx_tready              (host_dma_tx_tready),               //in  std_logic_vector(4:0)
    .host_dma_tx_tvalid              (host_dma_tx_tvalid),               //out std_logic_vector(4:0)
    .pcie_usr_ctrlport_req_wr        (pcie_usr_ctrlport_req_wr),         //out std_logic
    .pcie_usr_ctrlport_req_rd        (pcie_usr_ctrlport_req_rd),         //out std_logic
    .pcie_usr_ctrlport_req_addr      (pcie_usr_ctrlport_req_addr),       //out std_logic_vector(19:0)
    .pcie_usr_ctrlport_req_port_id   (),                                 //out std_logic_vector(9:0)
    .pcie_usr_ctrlport_req_rem_epid  (),                                 //out std_logic_vector(15:0)
    .pcie_usr_ctrlport_req_rem_portid(),                                 //out std_logic_vector(9:0)
    .pcie_usr_ctrlport_req_data      (pcie_usr_ctrlport_req_data),       //out std_logic_vector(31:0)
    .pcie_usr_ctrlport_req_byte_en   (),                                 //out std_logic_vector(3:0)
    .pcie_usr_ctrlport_req_has_time  (),                                 //out std_logic
    .pcie_usr_ctrlport_req_time      (),                                 //out std_logic_vector(63:0)
    .pcie_usr_ctrlport_resp_ack      (pcie_usr_ctrlport_resp_ack),       //in  std_logic
    .pcie_usr_ctrlport_resp_status   (pcie_usr_ctrlport_resp_status),    //in  std_logic_vector(1:0)
    .pcie_usr_ctrlport_resp_data     (pcie_usr_ctrlport_resp_data),      //in  std_logic_vector(31:0)
    .cpld_int_report_in              (cpld_int_report_in),               //out std_logic_vector(50:0)
    .cpld_int_report_out             (cpld_int_report_out),              //in  std_logic_vector(33:0)
    .core_ctrlport_req_wr            (host_ctrlport.req.wr),             //out std_logic
    .core_ctrlport_req_rd            (host_ctrlport.req.rd),             //out std_logic
    .core_ctrlport_req_addr          (host_ctrlport.req.addr),           //out std_logic_vector(19:0)
    .core_ctrlport_req_port_id       (host_ctrlport.req.port_id),        //out std_logic_vector(9:0)
    .core_ctrlport_req_rem_epid      (host_ctrlport.req.remote_epid),    //out std_logic_vector(15:0)
    .core_ctrlport_req_rem_portid    (host_ctrlport.req.remote_portid),  //out std_logic_vector(9:0)
    .core_ctrlport_req_data          (host_ctrlport.req.data),           //out std_logic_vector(31:0)
    .core_ctrlport_req_byte_en       (host_ctrlport.req.byte_en),        //out std_logic_vector(3:0)
    .core_ctrlport_req_has_time      (host_ctrlport.req.has_time),       //out std_logic
    .core_ctrlport_req_time          (host_ctrlport.req.timestamp),      //out std_logic_vector(63:0)
    .core_ctrlport_resp_ack          (host_ctrlport.resp.ack),           //in  std_logic
    .core_ctrlport_resp_status       (host_ctrlport.resp.status),        //in  std_logic_vector(1:0)
    .core_ctrlport_resp_data         (host_ctrlport.resp.data),          //in  std_logic_vector(31:0)
    .auth_sda_in                     (auth_sda_in),                      //in  std_logic
    .auth_sda_out                    (auth_sda_out)                      //out std_logic
  );

  assign pcie_arst = !PCIE_RESET_N; // Invert the reset signal to match the active low reset of the PCIe interface.
  assign PCIE_RESET_N_TO_CPLD = PCIE_RESET_N;  // Pass PCIe reset to CPLD so that it can re-load the FPGA on a PCIe reset.


  //---------------------------------------------------------
  // UHD PCIe Interface
  //---------------------------------------------------------

  // Consolidate DMA streams for the transport adapter.
  wire [DMA_STREAM_WIDTH-1:0] dma_rx_tdata;
  wire                        dma_rx_tlast;
  wire                        dma_rx_tready;
  wire                  [2:0] dma_rx_tuser;
  wire                        dma_rx_tvalid;
  wire [DMA_STREAM_WIDTH-1:0] dma_tx_tdata;
  wire                        dma_tx_tlast;
  wire                        dma_tx_tready;
  wire                  [2:0] dma_tx_tuser;
  wire                        dma_tx_tvalid;
  //vhook_e b310_pcie_int b310_pcie_int_i
  //vhook_a  dma_clk                dma_clk
  //vhook_a  bus_clk                bus_clk
  //vhook_a  reg_clk                dma_clk
  //vhook_a  bus_rst                bus_arst
  //vhook_a  misc_status           '0
  //vhook_a  debug                  {}
  //vhook_a  {^s_ctrlport_(.*)}     pcie_usr_ctrlport_$1
  b310_pcie_int #(
    .DMA_STREAM_WIDTH(DMA_STREAM_WIDTH),  //int:=128
    .NUM_TX_STREAMS  (NUM_TX_STREAMS),    //int:=5
    .NUM_RX_STREAMS  (NUM_RX_STREAMS),    //int:=5
    .BUS_CLK_RATE    (BUS_CLK_RATE)       //int:=166666666
  ) b310_pcie_int_i (
    .reg_clk               (dma_clk),                        //input wire
    .dma_clk               (dma_clk),                        //input wire
    .bus_clk               (bus_clk),                        //input wire
    .bus_rst               (bus_arst),                       //input wire
    .host_dma_tx_tdata     (host_dma_tx_tdata),              //input wire[(NUM_TX_STREAMS-1):0][(DMA_STREAM_WIDTH-1):0]
    .host_dma_tx_tvalid    (host_dma_tx_tvalid),             //input wire[(NUM_TX_STREAMS-1):0]
    .host_dma_tx_tready    (host_dma_tx_tready),             //output wire[(NUM_TX_STREAMS-1):0]
    .host_dma_rx_tdata     (host_dma_rx_tdata),              //output wire[(NUM_RX_STREAMS-1):0][(DMA_STREAM_WIDTH-1):0]
    .host_dma_rx_tvalid    (host_dma_rx_tvalid),             //output wire[(NUM_RX_STREAMS-1):0]
    .host_dma_rx_tready    (host_dma_rx_tready),             //input wire[(NUM_RX_STREAMS-1):0]
    .dma_tx_tdata          (dma_tx_tdata),                   //output wire[(DMA_STREAM_WIDTH-1):0]
    .dma_tx_tuser          (dma_tx_tuser),                   //output wire[2:0]
    .dma_tx_tvalid         (dma_tx_tvalid),                  //output wire
    .dma_tx_tlast          (dma_tx_tlast),                   //output wire
    .dma_tx_tready         (dma_tx_tready),                  //input wire
    .dma_rx_tdata          (dma_rx_tdata),                   //input wire[(DMA_STREAM_WIDTH-1):0]
    .dma_rx_tuser          (dma_rx_tuser),                   //input wire[2:0]
    .dma_rx_tvalid         (dma_rx_tvalid),                  //input wire
    .dma_rx_tlast          (dma_rx_tlast),                   //input wire
    .dma_rx_tready         (dma_rx_tready),                  //output wire
    .s_ctrlport_req_wr     (pcie_usr_ctrlport_req_wr),       //input wire
    .s_ctrlport_req_rd     (pcie_usr_ctrlport_req_rd),       //input wire
    .s_ctrlport_req_addr   (pcie_usr_ctrlport_req_addr),     //input wire[19:0]
    .s_ctrlport_req_data   (pcie_usr_ctrlport_req_data),     //input wire[31:0]
    .s_ctrlport_resp_ack   (pcie_usr_ctrlport_resp_ack),     //output wire
    .s_ctrlport_resp_status(pcie_usr_ctrlport_resp_status),  //output wire[1:0]
    .s_ctrlport_resp_data  (pcie_usr_ctrlport_resp_data),    //output wire[31:0]
    .misc_status           ('0),                             //input wire[15:0]
    .debug                 ()                                //output wire[127:0]
  );

  //---------------------------------------------------------
  // RF Converter interface
  //---------------------------------------------------------

  // SYNCIN/OUT
  //----------------------------------------
  logic adc_sync_b;
  logic dac_sync_b;

  // Eventhough the ADRV chip has three SYNCIN pins, we only use one of them.
  // This is due to each SYNC line connected to one framer, and only
  // framer 0 is used in the B310.
  OBUFDS adc_sync0_buf(
    .O(ADRV_SYNCIN_P[0]),
    .OB(ADRV_SYNCIN_N[0]),
    .I(adc_sync_b)
  );
  OBUFDS adc_sync1_buf(
    .O(ADRV_SYNCIN_P[1]),
    .OB(ADRV_SYNCIN_N[1]),
    .I(1'b1)
  );
  OBUFDS adc_sync2_buf(
    .O(ADRV_SYNCIN_P[2]),
    .OB(ADRV_SYNCIN_N[2]),
    .I(1'b1)
  );

  // Eventhough the ADRV chip has two SYNCOUT pins, we only use one of them.
  // This is due to each SYNC line connected to one deframer, and only
  // deframer 0 is used in the B310.
  IBUFDS dac_sync0_buf(
    .I(ADRV_SYNCOUT_P[0]),
    .IB(ADRV_SYNCOUT_N[0]),
    .O(dac_sync_b)
  );

  IBUFDS dac_sync1_buf(
    .I(ADRV_SYNCOUT_P[1]),
    .IB(ADRV_SYNCOUT_N[1]),
    .O()
  );

  // CTRLPORT
  //----------------------------------------

  ctrlport_if m_radio_jesd_ctrlport (
    .clk(radio_clk), // Clock for the control port
    .rst(radio_rst) // Reset for the control port
  );


  // RFNOC Radio
  //----------------------------------------
  localparam int NUM_CH_PER_RADIO = 2; // Number of channels per radio

  logic [32*NUM_CH_PER_RADIO-1:0] radio_tx_data; // Data to be transmitted by the radio
  logic [32*NUM_CH_PER_RADIO-1:0] radio_rx_data; // Data received from the radio
  logic radio_tx_ready;
  logic radio_rx_data_valid;
  // JESD IP instance
  //----------------------------------------

  //vhook_e b310_jesd204b b310_jesd204b_i
  //vhook_a rclk_rst                        radio_rst
  //vhook_a reg_clk                         radio_clk
  //vhook_a sample_clk_1x                   radio_clk
  //vhook_a sample_clk_2x                   radio_clk_2x
  //vhook_a rclk_fpga_clocks_stable         bclk_radio_clk_gen_locked
  //vhook_a rclk_jesd_ref_clk_present       {}
  //vhook_a jesd_ref_clk_p                  JESD_CLK_P
  //vhook_a jesd_ref_clk_n                  JESD_CLK_N
  //vhook_a adc_rx_p                        JESD_RX_P
  //vhook_a adc_rx_n                        JESD_RX_N
  //vhook_a dac_tx_p                        JESD_TX_P
  //vhook_a dac_tx_n                        JESD_TX_N
  //vhook_a rclk_ctrlport_req_time          m_radio_jesd_ctrlport.req.timestamp
  //vhook_a {^rclk_ctrlport_req_rem(.*)}    m_radio_jesd_ctrlport.req.remote$1
  //vhook_a {^rclk_ctrlport_req_(.*)}       m_radio_jesd_ctrlport.req.$1
  //vhook_a {^rclk_ctrlport_resp_(.*)}      m_radio_jesd_ctrlport.resp.$1
  //vhook_a sysref_in_p                     SYSREF_P
  //vhook_a sysref_in_n                     SYSREF_N
  //vhook_a capture_sysref_clk              radio_clk
  //vhook_a sclk_sysref_out                 {}
  //vhook_a lmk_sync                        {}
  //vhook_a sclk_adc_data_flatter           radio_rx_data
  //vhook_a sclk_adc_data_valid             radio_rx_data_valid
  //vhook_a sclk_dac_data_flatter           radio_tx_data
  //vhook_a sclk_dac_ready_for_input        radio_tx_ready
  //vhook_a adc_sync_out_n                  adc_sync_b
  //vhook_a dac_sync_in_n                   dac_sync_b
  //vhook_a *c_sync_out                     {}
  b310_jesd204b b310_jesd204b_i (
    .rclk_rst                    (radio_rst),                                //in  std_logic
    .reg_clk                     (radio_clk),                                //in  std_logic
    .clk_40mhz                   (clk_40mhz),                                //in  std_logic
    .sample_clk_1x               (radio_clk),                                //in  std_logic
    .sample_clk_2x               (radio_clk_2x),                             //in  std_logic
    .rclk_fpga_clocks_stable     (bclk_radio_clk_gen_locked),                //in  std_logic
    .jesd_ref_clk_p              (JESD_CLK_P),                               //in  std_logic
    .jesd_ref_clk_n              (JESD_CLK_N),                               //in  std_logic
    .rclk_jesd_ref_clk_present   (),                                         //out std_logic
    .lmk_sync                    (),                                         //out std_logic
    .rclk_ctrlport_req_wr        (m_radio_jesd_ctrlport.req.wr),             //in  std_logic
    .rclk_ctrlport_req_rd        (m_radio_jesd_ctrlport.req.rd),             //in  std_logic
    .rclk_ctrlport_req_addr      (m_radio_jesd_ctrlport.req.addr),           //in  std_logic_vector(19:0)
    .rclk_ctrlport_req_port_id   (m_radio_jesd_ctrlport.req.port_id),        //in  std_logic_vector(9:0)
    .rclk_ctrlport_req_rem_epid  (m_radio_jesd_ctrlport.req.remote_epid),    //in  std_logic_vector(15:0)
    .rclk_ctrlport_req_rem_portid(m_radio_jesd_ctrlport.req.remote_portid),  //in  std_logic_vector(9:0)
    .rclk_ctrlport_req_data      (m_radio_jesd_ctrlport.req.data),           //in  std_logic_vector(31:0)
    .rclk_ctrlport_req_byte_en   (m_radio_jesd_ctrlport.req.byte_en),        //in  std_logic_vector(3:0)
    .rclk_ctrlport_req_has_time  (m_radio_jesd_ctrlport.req.has_time),       //in  std_logic
    .rclk_ctrlport_req_time      (m_radio_jesd_ctrlport.req.timestamp),      //in  std_logic_vector(63:0)
    .rclk_ctrlport_resp_ack      (m_radio_jesd_ctrlport.resp.ack),           //out std_logic
    .rclk_ctrlport_resp_status   (m_radio_jesd_ctrlport.resp.status),        //out std_logic_vector(1:0)
    .rclk_ctrlport_resp_data     (m_radio_jesd_ctrlport.resp.data),          //out std_logic_vector(31:0)
    .capture_sysref_clk          (radio_clk),                                //in  std_logic
    .sysref_in_p                 (SYSREF_P),                                 //in  std_logic
    .sysref_in_n                 (SYSREF_N),                                 //in  std_logic
    .sclk_sysref_out             (),                                         //out std_logic
    .adc_rx_p                    (JESD_RX_P),                                //in  std_logic_vector(1:0)
    .adc_rx_n                    (JESD_RX_N),                                //in  std_logic_vector(1:0)
    .adc_sync_out_n              (adc_sync_b),                               //out std_logic
    .dac_tx_p                    (JESD_TX_P),                                //out std_logic_vector(1:0)
    .dac_tx_n                    (JESD_TX_N),                                //out std_logic_vector(1:0)
    .dac_sync_in_n               (dac_sync_b),                               //in  std_logic
    .sclk_adc_data_flatter       (radio_rx_data),                            //out std_logic_vector(63:0)
    .sclk_dac_data_flatter       (radio_tx_data),                            //in  std_logic_vector(63:0)
    .sclk_adc_data_valid         (radio_rx_data_valid),                      //out std_logic
    .sclk_dac_ready_for_input    (radio_tx_ready),                           //out std_logic
    .dac_sync_out                (),                                         //out std_logic
    .adc_sync_out                ()                                          //out std_logic
  );

  //---------------------------------------------------------
  // Radio/Core Logic
  //---------------------------------------------------------
  //-----------------------------------------------------------------------------
  // DDR3 Interface
  //-----------------------------------------------------------------------------
  wire           ddr3_axi_clk;           // 1/4 DDR external clock rate (125MHz)
  wire           ddr3_axi_clk_x2;        // 1/2 DDR external clock rate (250MHz)
  wire           ddr3_axi_rst;           // Synchronized to ddr_sys_clk
  wire           ddr3_running;           // DRAM calibration complete.

  // Slave Interface Write Address Ports
  wire [1:0]     ddr3_axi_awid;
  wire [31:0]    ddr3_axi_awaddr;
  wire [7:0]     ddr3_axi_awlen;
  wire [2:0]     ddr3_axi_awsize;
  wire [1:0]     ddr3_axi_awburst;
  wire [0:0]     ddr3_axi_awlock;
  wire [3:0]     ddr3_axi_awcache;
  wire [2:0]     ddr3_axi_awprot;
  wire [3:0]     ddr3_axi_awqos;
  wire           ddr3_axi_awvalid;
  wire           ddr3_axi_awready;
  // Slave Interface Write Data Ports
  wire [255:0]   ddr3_axi_wdata;
  wire [31:0]    ddr3_axi_wstrb;
  wire           ddr3_axi_wlast;
  wire [1:0]     ddr3_axi_wvalid;
  wire           ddr3_axi_wready;
  // Slave Interface Write Response Ports
  wire           ddr3_axi_bready;
  wire [1:0]     ddr3_axi_bid;
  wire [1:0]     ddr3_axi_bresp;
  wire           ddr3_axi_bvalid;
  // Slave Interface Read Address Ports
  wire [1:0]     ddr3_axi_arid;
  wire [31:0]    ddr3_axi_araddr;
  wire [7:0]     ddr3_axi_arlen;
  wire [2:0]     ddr3_axi_arsize;
  wire [1:0]     ddr3_axi_arburst;
  wire [0:0]     ddr3_axi_arlock;
  wire [3:0]     ddr3_axi_arcache;
  wire [2:0]     ddr3_axi_arprot;
  wire [3:0]     ddr3_axi_arqos;
  wire           ddr3_axi_arvalid;
  wire           ddr3_axi_arready;
  // Slave Interface Read Data Ports
  wire           ddr3_axi_rready;
  wire [1:0]     ddr3_axi_rid;
  wire [255:0]   ddr3_axi_rdata;
  wire [1:0]     ddr3_axi_rresp;
  wire           ddr3_axi_rlast;
  wire           ddr3_axi_rvalid;

  wire           ddr3_idelay_refclk;
  reg            ddr3_axi_rst_reg_n;

  localparam int RADIO_NIPC = 1;


  // Radio-based control
  logic [3:0] radio_ch0_adrv_gpio_dir;
  wire  [3:0] radio_ch0_adrv_gpio_in;
  logic [3:0] radio_ch0_adrv_gpio_out;
  logic [3:0] radio_ch0_adrv_trx_out;
  logic [3:0] radio_ch1_adrv_gpio_dir;
  wire  [3:0] radio_ch1_adrv_gpio_in;
  logic [3:0] radio_ch1_adrv_gpio_out;
  logic [3:0] radio_ch1_adrv_trx_out;
  logic       radio_ch0_adrv_int;
  logic       radio_ch1_adrv_int;
  logic [1:0] radio_enable_tdr;
  logic [1:0] radio_led_rx_green;
  logic [1:0] radio_led_tx_green;
  logic [1:0] radio_led_tx_red;
  logic [1:0] radio_rx_bypass;
  logic       radio_adrv_reset;
  logic       radio_test_enable;
  wire        radio_spi_miso;
  wire        radio_spi_mosi;
  wire        radio_spi_sclk;
  wire        radio_spi_sen_n;

  // Clocking control signals
  wire        clocking_spi_miso;
  wire        clocking_spi_mosi;
  wire        clocking_spi_sclk;
  wire [2:0]  clocking_spi_sen_n;

  logic local_ref_clk_buf;
  logic local_ref_clk;
  logic ext_ref_clk;

  logic gps_lmk_pps_in;
  logic gps_pw_fault;

  // GPIO control signals
  wire [9:0] fp_gpio_dir;
  wire [9:0] fp_gpio_in;
  wire [9:0] fp_gpio_out;

  // Power LED control
  logic pwr_led_orange;

  // Device monitoring
  logic        pwr_monitor_alert;
  logic [11:0] device_temp;

  //vhook_e b310_core b310_core_i
  //vhook_a PCIE_W                DMA_STREAM_WIDTH
  //vhook_a clk_40mhz             clk_40mhz
  //vhook_a s_core_ctrlport       host_ctrlport.slave
  //vhook_a m_radio_jesd_ctrlport m_radio_jesd_ctrlport.master
  //vhook_a mb_i2c_scl            MISC_I2C_SCL
  //vhook_a mb_i2c_sda            MISC_I2C_SDA
  //vhook_a tb_i2c_scl            TBOLT_SCL
  //vhook_a tb_i2c_sda            TBOLT_SDA
  //vhook_a pps_in                EXT_PPS_IN
  //vhook_a pps_out               PPS_OUT
  //vhook_a uart_rx               GPS_UART_TOFPGA
  //vhook_a uart_tx               GPS_UART_TOGPS
  //vhook_a gps_reset_n           GPS_RESET_N
  //vhook_a gps_ant_pwr_en        ANT_PWR_EN
  //vhook_a jtag_cpld_tck         JTAG_CPLD_TCK
  //vhook_a jtag_cpld_tms         JTAG_CPLD_TMS
  //vhook_a jtag_cpld_tdi         JTAG_CPLD_TDI
  //vhook_a jtag_cpld_tdo         JTAG_CPLD_TDO
  //vhook_a lmk_sync_reva         LMK_SYNC_REVA
  //vhook_a lmk_sync              LMK_SYNC
  //vhook_a lmk05318_pd_n         NSYNC_PDN
  //vhook_a gps_lmk_status        NSYNC_STATUS
  //vhook_a gps_lmk_gpio          NSYNC_GPIO0
  //vhook_a gps_pps_out           GPS_PPS_OUT
  //vhook_a tbolt_pd_ctrl_reset   TBOLT_PD_CTRL_RESET
  //vhook_a pwr_1v2_pg            PG_1V2
  //vhook_a pwr_typec_negotiated  TYPEC_PWR_NEGOTIATED
  //vhook_a pwr_25w_src           GT_25W_PWR_SRC
  b310_core #(
    .PCIE_W          (DMA_STREAM_WIDTH),  //integer:=128
    .RADIO_NIPC      (RADIO_NIPC),        //integer:=1
    .NUM_CH_PER_RADIO(NUM_CH_PER_RADIO),  //integer:=2
    .BUS_CLK_RATE    (BUS_CLK_RATE)       //integer:=125000000
  ) b310_core_i (
    .bus_clk                (bus_clk),                       //input wire
    .radio_clk              (radio_clk),                     //input wire
    .radio_clk_shifted      (radio_clk_shifted),             //input wire
    .ce_clk                 (ce_clk),                        //input wire
    .clk_40mhz              (clk_40mhz),                     //input wire
    .local_ref_clk          (local_ref_clk),                 //input wire
    .ext_ref_clk            (ext_ref_clk),                   //input wire
    .bus_rst                (bus_rst),                       //input wire
    .radio_rst              (radio_rst),                     //input wire
    .s_core_ctrlport        (host_ctrlport.slave),           // ctrlport_if.slave
    .m_radio_jesd_ctrlport  (m_radio_jesd_ctrlport.master),  // ctrlport_if.master
    .radio_rx_data          (radio_rx_data),                 //input wire[((32*NUM_CH_PER_RADIO)-1):0]
    .radio_rx_data_valid    (radio_rx_data_valid),           //input wire
    .radio_tx_data          (radio_tx_data),                 //output wire[((32*NUM_CH_PER_RADIO)-1):0]
    .radio_tx_ready         (radio_tx_ready),                //input wire
    .radio_led_tx_green     (radio_led_tx_green),            //output logic[1:0]
    .radio_led_tx_red       (radio_led_tx_red),              //output logic[1:0]
    .radio_led_rx_green     (radio_led_rx_green),            //output logic[1:0]
    .radio_ch0_adrv_gpio_out(radio_ch0_adrv_gpio_out),       //output logic[3:0]
    .radio_ch1_adrv_gpio_out(radio_ch1_adrv_gpio_out),       //output logic[3:0]
    .radio_ch0_adrv_gpio_in (radio_ch0_adrv_gpio_in),        //input wire[3:0]
    .radio_ch1_adrv_gpio_in (radio_ch1_adrv_gpio_in),        //input wire[3:0]
    .radio_ch0_adrv_gpio_dir(radio_ch0_adrv_gpio_dir),       //output logic[3:0]
    .radio_ch1_adrv_gpio_dir(radio_ch1_adrv_gpio_dir),       //output logic[3:0]
    .radio_ch0_adrv_trx_out (radio_ch0_adrv_trx_out),        //output logic[3:0]
    .radio_ch1_adrv_trx_out (radio_ch1_adrv_trx_out),        //output logic[3:0]
    .radio_ch0_adrv_int     (radio_ch0_adrv_int),            //input wire
    .radio_ch1_adrv_int     (radio_ch1_adrv_int),            //input wire
    .radio_enable_tdr       (radio_enable_tdr),              //output logic[1:0]
    .radio_rx_bypass        (radio_rx_bypass),               //output logic[1:0]
    .radio_adrv_reset       (radio_adrv_reset),              //output logic
    .radio_test_enable      (radio_test_enable),             //output logic
    .radio_spi_mosi         (radio_spi_mosi),                //output logic
    .radio_spi_sclk         (radio_spi_sclk),                //output logic
    .radio_spi_sen_n        (radio_spi_sen_n),               //output logic
    .radio_spi_miso         (radio_spi_miso),                //input wire
    .ddr3_axi_clk           (ddr3_axi_clk),                  //input wire
    .ddr3_axi_clk_x2        (ddr3_axi_clk_x2),               //input wire
    .ddr3_axi_rst           (ddr3_axi_rst),                  //input wire
    .ddr3_axi_awid          (ddr3_axi_awid),                 //output wire
    .ddr3_axi_awaddr        (ddr3_axi_awaddr),               //output wire[31:0]
    .ddr3_axi_awlen         (ddr3_axi_awlen),                //output wire[7:0]
    .ddr3_axi_awsize        (ddr3_axi_awsize),               //output wire[2:0]
    .ddr3_axi_awburst       (ddr3_axi_awburst),              //output wire[1:0]
    .ddr3_axi_awlock        (ddr3_axi_awlock),               //output wire[0:0]
    .ddr3_axi_awcache       (ddr3_axi_awcache),              //output wire[3:0]
    .ddr3_axi_awprot        (ddr3_axi_awprot),               //output wire[2:0]
    .ddr3_axi_awqos         (ddr3_axi_awqos),                //output wire[3:0]
    .ddr3_axi_awvalid       (ddr3_axi_awvalid),              //output wire
    .ddr3_axi_awready       (ddr3_axi_awready),              //input wire
    .ddr3_axi_wdata         (ddr3_axi_wdata),                //output wire[255:0]
    .ddr3_axi_wstrb         (ddr3_axi_wstrb),                //output wire[31:0]
    .ddr3_axi_wlast         (ddr3_axi_wlast),                //output wire
    .ddr3_axi_wvalid        (ddr3_axi_wvalid),               //output wire
    .ddr3_axi_wready        (ddr3_axi_wready),               //input wire
    .ddr3_axi_bready        (ddr3_axi_bready),               //output wire
    .ddr3_axi_bid           (ddr3_axi_bid),                  //input wire
    .ddr3_axi_bresp         (ddr3_axi_bresp),                //input wire[1:0]
    .ddr3_axi_bvalid        (ddr3_axi_bvalid),               //input wire
    .ddr3_axi_arid          (ddr3_axi_arid),                 //output wire
    .ddr3_axi_araddr        (ddr3_axi_araddr),               //output wire[31:0]
    .ddr3_axi_arlen         (ddr3_axi_arlen),                //output wire[7:0]
    .ddr3_axi_arsize        (ddr3_axi_arsize),               //output wire[2:0]
    .ddr3_axi_arburst       (ddr3_axi_arburst),              //output wire[1:0]
    .ddr3_axi_arlock        (ddr3_axi_arlock),               //output wire[0:0]
    .ddr3_axi_arcache       (ddr3_axi_arcache),              //output wire[3:0]
    .ddr3_axi_arprot        (ddr3_axi_arprot),               //output wire[2:0]
    .ddr3_axi_arqos         (ddr3_axi_arqos),                //output wire[3:0]
    .ddr3_axi_arvalid       (ddr3_axi_arvalid),              //output wire
    .ddr3_axi_arready       (ddr3_axi_arready),              //input wire
    .ddr3_axi_rready        (ddr3_axi_rready),               //output wire
    .ddr3_axi_rid           (ddr3_axi_rid),                  //input wire
    .ddr3_axi_rdata         (ddr3_axi_rdata),                //input wire[255:0]
    .ddr3_axi_rresp         (ddr3_axi_rresp),                //input wire[1:0]
    .ddr3_axi_rlast         (ddr3_axi_rlast),                //input wire
    .ddr3_axi_rvalid        (ddr3_axi_rvalid),               //input wire
    .dma_rx_tdata           (dma_rx_tdata),                  //output logic[(PCIE_W-1):0]
    .dma_rx_tuser           (dma_rx_tuser),                  //output logic[2:0]
    .dma_rx_tlast           (dma_rx_tlast),                  //output logic
    .dma_rx_tvalid          (dma_rx_tvalid),                 //output logic
    .dma_rx_tready          (dma_rx_tready),                 //input wire
    .dma_tx_tdata           (dma_tx_tdata),                  //input wire[(PCIE_W-1):0]
    .dma_tx_tuser           (dma_tx_tuser),                  //input wire[2:0]
    .dma_tx_tlast           (dma_tx_tlast),                  //input wire
    .dma_tx_tvalid          (dma_tx_tvalid),                 //input wire
    .dma_tx_tready          (dma_tx_tready),                 //output logic
    .fp_gpio_out            (fp_gpio_out),                   //output logic[9:0]
    .fp_gpio_in             (fp_gpio_in),                    //input wire[9:0]
    .fp_gpio_dir            (fp_gpio_dir),                   //output logic[9:0]
    .radio_clk_gen_rst      (radio_clk_gen_rst),             //output wire
    .ref_clk_source         (ref_clk_source),                //output wire
    .lmk_source_select      (lmk_source_select),             //output wire
    .tcxo_en                (tcxo_en),                       //output wire
    .lmk_reset              (lmk_reset),                     //output wire
    .lmk_lock_status        (lmk_lock_status),               //input wire
    .lmk_sync_reva          (LMK_SYNC_REVA),                 //output wire
    .lmk_sync               (LMK_SYNC),                      //output wire
    .pps_in                 (EXT_PPS_IN),                    //input wire
    .gps_lmk_pps_in         (gps_lmk_pps_in),                //input wire
    .gps_lmk_status         (NSYNC_STATUS),                  //input wire[1:0]
    .gps_lmk_gpio           (NSYNC_GPIO0),                   //output logic
    .gps_pw_fault           (gps_pw_fault),                  //input wire
    .gps_pps_out            (GPS_PPS_OUT),                   //input wire
    .pps_out                (PPS_OUT),                       //output logic[3:0]
    .lmk05318_pd_n          (NSYNC_PDN),                     //output wire
    .mb_i2c_scl             (MISC_I2C_SCL),                  //inout wire
    .mb_i2c_sda             (MISC_I2C_SDA),                  //inout wire
    .tb_i2c_scl             (TBOLT_SCL),                     //inout wire
    .tb_i2c_sda             (TBOLT_SDA),                     //inout wire
    .clocking_spi_mosi      (clocking_spi_mosi),             //output logic
    .clocking_spi_sclk      (clocking_spi_sclk),             //output logic
    .clocking_spi_sen_n     (clocking_spi_sen_n),            //output logic[2:0]
    .clocking_spi_miso      (clocking_spi_miso),             //input wire
    .uart_rx                (GPS_UART_TOFPGA),               //input wire
    .uart_tx                (GPS_UART_TOGPS),                //output wire
    .gps_reset_n            (GPS_RESET_N),                   //output wire
    .gps_ant_pwr_en         (ANT_PWR_EN),                    //output wire
    .device_temp            (device_temp),                   //input wire[11:0]
    .jtag_cpld_tck          (JTAG_CPLD_TCK),                 //output wire
    .jtag_cpld_tms          (JTAG_CPLD_TMS),                 //output wire
    .jtag_cpld_tdi          (JTAG_CPLD_TDI),                 //output wire
    .jtag_cpld_tdo          (JTAG_CPLD_TDO),                 //input wire
    .tbolt_pd_ctrl_reset    (TBOLT_PD_CTRL_RESET),           //output logic
    .pwr_1v2_pg             (PG_1V2),                        //input wire
    .pwr_25w_src            (GT_25W_PWR_SRC),                //input wire
    .pwr_typec_negotiated   (TYPEC_PWR_NEGOTIATED),          //input wire
    .pwr_monitor_alert      (pwr_monitor_alert),             //input wire
    .pwr_led_orange         (pwr_led_orange)                 //output logic
  );

  //---------------------------------------------------------
  // CPLD Interface
  //---------------------------------------------------------

  // CPLD Interface wrapper instantiation
  cpld_interface_wrapper cpld_interface_i (
    .a_reset                (clk40_rst),            // boolean reset input
    .bus_clk                (clk_40mhz),            // bus clock input
    .b_reg_port_in          (cpld_int_report_in),   // register port input [50:0]
    .b_reg_port_out         (cpld_int_report_out),  // register port output [33:0]
    .a_cpld_ext_reset_n     (CPLD_EXT_RST_R_N),     // CPLD external reset output
    .fpga_to_cpld_clk       (CPLD_RX_CLK),          // FPGA to CPLD clock output
    .f_fpga_to_cpld_clk_en  (CPLD_RX_CLKEN),        // FPGA to CPLD clock enable output
    .f_fpga_to_cpld_data    (CPLD_RX_DATA),         // FPGA to CPLD data output [7:0]
    .cpld_to_fpga_clk       (CPLD_TX_CLK),          // CPLD to FPGA clock input
    .c_cpld_to_fpga_data    (CPLD_TX_DATA)          // CPLD to FPGA data input [7:0]
  );

  assign CPLD_OSC_EN = 1'b1; // Let the CPLD oscillator run continuously.

  //---------------------------------------------------------
  // IO assignments
  //---------------------------------------------------------

  // IOs for ADRV9032
  //-----------------------------------------

  // SPI
  assign ADRV_SPI_CLK    = radio_spi_sclk;
  assign ADRV_SPI_DIO    = radio_spi_mosi;
  assign radio_spi_miso  = ADRV_SPI_DO;
  assign ADRV_SPI_EN_N   = radio_spi_sen_n;
  // GPIO - First 4 GPIOs for channel 0, next 4 GPIOs for channel 1
  assign ADRV_GPIO[0] = radio_ch0_adrv_gpio_dir[0] ? radio_ch0_adrv_gpio_out[0] : 1'bz;
  assign ADRV_GPIO[1] = radio_ch0_adrv_gpio_dir[1] ? radio_ch0_adrv_gpio_out[1] : 1'bz;
  assign ADRV_GPIO[2] = radio_ch0_adrv_gpio_dir[2] ? radio_ch0_adrv_gpio_out[2] : 1'bz;
  assign ADRV_GPIO[3] = radio_ch0_adrv_gpio_dir[3] ? radio_ch0_adrv_gpio_out[3] : 1'bz;
  assign ADRV_GPIO[4] = radio_ch1_adrv_gpio_dir[0] ? radio_ch1_adrv_gpio_out[0] : 1'bz;
  assign ADRV_GPIO[5] = radio_ch1_adrv_gpio_dir[1] ? radio_ch1_adrv_gpio_out[1] : 1'bz;
  assign ADRV_GPIO[6] = radio_ch1_adrv_gpio_dir[2] ? radio_ch1_adrv_gpio_out[2] : 1'bz;
  assign ADRV_GPIO[7] = radio_ch1_adrv_gpio_dir[3] ? radio_ch1_adrv_gpio_out[3] : 1'bz;
  assign radio_ch0_adrv_gpio_in = ADRV_GPIO[3:0];
  assign radio_ch1_adrv_gpio_in = ADRV_GPIO[7:4];
  // TRX - First 4 TRX controls for channel 0, next 4 TRX controls for channel 1
  assign ADRV_TRXA_CTRL = radio_ch0_adrv_trx_out[0];
  assign ADRV_TRXB_CTRL = radio_ch0_adrv_trx_out[1];
  assign ADRV_TRXC_CTRL = radio_ch0_adrv_trx_out[2];
  assign ADRV_TRXD_CTRL = radio_ch0_adrv_trx_out[3];
  assign ADRV_TRXE_CTRL = radio_ch1_adrv_trx_out[0];
  assign ADRV_TRXF_CTRL = radio_ch1_adrv_trx_out[1];
  assign ADRV_TRXG_CTRL = radio_ch1_adrv_trx_out[2];
  assign ADRV_TRXH_CTRL = radio_ch1_adrv_trx_out[3];

  assign radio_ch0_adrv_int = ADRV_GPINT[0];
  assign radio_ch1_adrv_int = ADRV_GPINT[1];

  // The observation receiver on the ADRV9032 is not used
  assign ADRV_ORXA_CTRL = 1'b0;
  assign ADRV_ORXB_CTRL = 1'b0;

  // Path controls
  //-----------------------------------------
  // Due to board layout, RX_SW_CTRL 0 and 1 have opposite polarity.
  assign ENABLE_TXRX_TDR = radio_enable_tdr;
  assign RX_SW_CTRL[0] = ~radio_enable_tdr[0];
  assign RX_SW_CTRL[1] = radio_enable_tdr[1];

  assign ADRV_RESET_N = ~radio_adrv_reset; // Active low reset for ADRV9032
  assign ADRV_TEST_EN = radio_test_enable; // Enable test mode for ADRV9032

  // LED indicators
  //-----------------------------------------
  // Radio LED indicators
  assign LED_TX_GRN = radio_led_tx_green;
  assign LED_TX_RED = radio_led_tx_red;
  assign LED_RX_GRN = radio_led_rx_green;
  // Miscellaneous LED indicators
  assign LED_FPGA_CONFIG_GRN = 1'b1; // Always on, indicating FPGA is configured
  assign LED_PWR_STS_ORANGE  = pwr_led_orange; // Force Orange PWR LED
  assign LED_PWR_STS_BLUE    = ~pwr_led_orange; // Indicate blue unless overriden.
  assign LED_CLK_STS_GRN     = LMK32_STATUS; // LMK32 lock status


  `include "../regmap/core_regs_regmap_utils.vh"
  // MISC SPI fanout
  //-----------------------------------------
  assign LMK053_MOSI = clocking_spi_mosi;
  assign LMK32_MOSI  = clocking_spi_mosi;
  assign DAC_MOSI    = clocking_spi_mosi;

  assign LMK053_SCLK = clocking_spi_sclk;
  assign LMK32_SCLK  = clocking_spi_sclk;
  assign DAC_SCLK    = clocking_spi_sclk;

  assign LMK053_CS_N  = clocking_spi_sen_n[CLOCKING_SPI_LMK053_SS];
  assign LMK32_CS_N   = clocking_spi_sen_n[CLOCKING_SPI_LMK04832_SS];
  assign DAC_CS_N     = clocking_spi_sen_n[CLOCKING_SPI_TCXO_DAC_SS];

  assign DAC_CLR_N = 1'b1;

  // The DAC interface is write-only, so it is ommitted from the MISO merge.
  assign clocking_spi_miso =  ~clocking_spi_sen_n[CLOCKING_SPI_LMK053_SS]    ? LMK053_MISO :
                              ~clocking_spi_sen_n[CLOCKING_SPI_LMK04832_SS]  ? LMK32_MISO  : 1'b0;

  // Clocking controls
  //-----------------------------------------
  assign LMK32_RESET = lmk_reset;
  assign TCXO_EN_N = ~tcxo_en;
  assign LMK32_VCXO_SEL_122M88 = ~lmk_source_select; // Select 122.88 MHz when low.
  assign REF_CLK_SEL = ref_clk_source;
  assign lmk_lock_status = LMK32_STATUS;

  // GPS_REF[1] is the PPS signal from LMK05318, which is disciplined by the GPSDO.
  assign gps_lmk_pps_in = GPS_REF[1];
  assign gps_pw_fault   = ~GPS_PWR_FAULT_N; // Active low power fault signal from GPSDO

  assign JTAG_TBOLT_TCK = 1'bZ; // JTAG TCK is not used, set to high-impedance
  assign JTAG_TBOLT_TMS = 1'bZ; // JTAG TMS is not used, set to high-impedance
  assign JTAG_TBOLT_TDI = 1'bZ; // JTAG TDI is not used, set to high-impedance

  // Power monitoring
  assign pwr_monitor_alert = ~VBUS_ALERT_N; // Active low alert signal from power monitor

  // Thunderbolt Resets
  //-----------------------------------------
  // This signal goes into a reset controller.  No intention to use it.  Set to 1.
  assign TBOLT_RIDGE_RESET_N =   1'b1;

  // Internal Reference Clock (122.88/125 MHz)
  IBUFDS fpga_refclk_buf(
    .I(FPGA_REFCLK_P),
    .IB(FPGA_REFCLK_N),
    .O(local_ref_clk_buf)
  );

  // ref_clk_pll for better phase consistency to meet PPS_OUT timing
  ref_clk_pll ref_clk_pll_inst
  (
    .clk_in1(local_ref_clk_buf),
    .clk_out1(local_ref_clk),
    .reset(1'b0),
    .locked()
  );

  // External Reference Clock (10/25/30.72/122.88/125 MHz)
  IBUFDS ext_refclk_buf(
    .I(EXT_REFCLK_P),
    .IB(EXT_REFCLK_N),
    .O(ext_ref_clk)
  );

  // Front Panel GPIO
  //-----------------------------------------

  //vhook_e gpio_atr_io fp_gpio_io
  //vhook_a WIDTH       10
  //vhook_a clk         radio_clk
  //vhook_a gpio_ddr    fp_gpio_dir
  //vhook_a gpio_in     fp_gpio_in
  //vhook_a gpio_out    fp_gpio_out
  //vhook_a gpio_pins   FP_GPIO
  gpio_atr_io #(
    .WIDTH(10)  //integer:=32
  ) fp_gpio_io (
    .clk      (radio_clk),    //input wire
    .gpio_ddr (fp_gpio_dir),  //input wire[(WIDTH-1):0]
    .gpio_out (fp_gpio_out),  //input wire[(WIDTH-1):0]
    .gpio_in  (fp_gpio_in),   //output wire[(WIDTH-1):0]
    .gpio_pins(FP_GPIO)       //inout wire[(WIDTH-1):0]
  );

  // GPS
  OBUFDS #(
   .IOSTANDARD("DEFAULT"),  // Specify the output I/O standard
   .SLEW("SLOW")            // Specify the output slew rate
  ) OBUFDS_inst (
    .O    (GPS_SECREF_P),   // Diff_p output (connect directly to top-level port)
    .OB   (GPS_SECREF_N),   // Diff_n output (connect directly to top-level port)
    .I    ('b0)             // Buffer input
  );

  assign GPS_REFSEL = 1'b0; // Since the secondary reference is not used,
                            // tie the REFSEL pin low to select the primary reference.
  assign GPS_EXTINT = 1'b0; // External interrupt connection is de-populated.

  ///////////////////////////////////////////////////////////////////////////////////
  //
  // Xilinx DDR3 Controller and PHY.
  //
  ///////////////////////////////////////////////////////////////////////////////////

  // Copied this reset circuit from example design.
  always_ff @(posedge ddr3_axi_clk) begin
    ddr3_axi_rst_reg_n <= ~ddr3_axi_rst;
  end

`ifdef ENABLE_DRAM
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
    .s_axi_awid                     (ddr3_axi_awid),
    .s_axi_awaddr                   (ddr3_axi_awaddr),
    .s_axi_awlen                    (ddr3_axi_awlen),
    .s_axi_awsize                   (ddr3_axi_awsize),
    .s_axi_awburst                  (ddr3_axi_awburst),
    .s_axi_awlock                   (ddr3_axi_awlock),
    .s_axi_awcache                  (ddr3_axi_awcache),
    .s_axi_awprot                   (ddr3_axi_awprot),
    .s_axi_awqos                    (ddr3_axi_awqos),
    .s_axi_awvalid                  (ddr3_axi_awvalid),
    .s_axi_awready                  (ddr3_axi_awready),
    // Slave Interface Write Data Ports
    .s_axi_wdata                    (ddr3_axi_wdata),
    .s_axi_wstrb                    (ddr3_axi_wstrb),
    .s_axi_wlast                    (ddr3_axi_wlast),
    .s_axi_wvalid                   (ddr3_axi_wvalid),
    .s_axi_wready                   (ddr3_axi_wready),
    // Slave Interface Write Response Ports
    .s_axi_bid                      (ddr3_axi_bid),
    .s_axi_bresp                    (ddr3_axi_bresp),
    .s_axi_bvalid                   (ddr3_axi_bvalid),
    .s_axi_bready                   (ddr3_axi_bready),
    // Slave Interface Read Address Ports
    .s_axi_arid                     (ddr3_axi_arid),
    .s_axi_araddr                   (ddr3_axi_araddr),
    .s_axi_arlen                    (ddr3_axi_arlen),
    .s_axi_arsize                   (ddr3_axi_arsize),
    .s_axi_arburst                  (ddr3_axi_arburst),
    .s_axi_arlock                   (ddr3_axi_arlock),
    .s_axi_arcache                  (ddr3_axi_arcache),
    .s_axi_arprot                   (ddr3_axi_arprot),
    .s_axi_arqos                    (ddr3_axi_arqos),
    .s_axi_arvalid                  (ddr3_axi_arvalid),
    .s_axi_arready                  (ddr3_axi_arready),
    // Slave Interface Read Data Ports
    .s_axi_rid                      (ddr3_axi_rid),
    .s_axi_rdata                    (ddr3_axi_rdata),
    .s_axi_rresp                    (ddr3_axi_rresp),
    .s_axi_rlast                    (ddr3_axi_rlast),
    .s_axi_rvalid                   (ddr3_axi_rvalid),
    .s_axi_rready                   (ddr3_axi_rready),
    // System Clock Ports
    .sys_clk_i                      (sys_clk_i),  // From external 100MHz source.
    .sys_rst                        (bus_arst)  // Active high Reset signal from external source.
  );
`else // bank disabled

  assign ddr3_axi_clk       = '0;
  assign ddr3_axi_clk_x2    = '0;
  assign ddr3_axi_rst       = '1;
  assign ddr3_running       = '1;
  assign ddr3_idelay_refclk = '0;

  assign ddr3_axi_arready = '1;
  assign ddr3_axi_awready = '1;
  assign ddr3_axi_bid     = '0;
  assign ddr3_axi_bresp   = '0;
  assign ddr3_axi_bvalid  = '0;
  assign ddr3_axi_rdata   = '0;
  assign ddr3_axi_rid     = '0;
  assign ddr3_axi_rlast   = '0;
  assign ddr3_axi_rresp   = '0;
  assign ddr3_axi_rvalid  = '0;
  assign ddr3_axi_wready  = '1;

`endif

  // Temperature monitor module
  localparam int SHUTDOWN_TEMP_C    = 100;  // OT shutdown temperature (degrees C)

  //vhook_e b310_xadc_wrapper xadc_wrapper_i
  //vhook_a clk         bus_clk
  //vhook_a rst         bus_rst
  b310_xadc_wrapper #(
    .SHUTDOWN_TEMP_C(SHUTDOWN_TEMP_C)  //int:=125
  ) xadc_wrapper_i (
    .clk        (bus_clk),     //input wire
    .rst        (bus_rst),     //input wire
    .device_temp(device_temp)  //output logic[11:0]
  );

endmodule : b310

`default_nettype wire

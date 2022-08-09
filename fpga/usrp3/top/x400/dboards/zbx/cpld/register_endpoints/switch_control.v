//
// Copyright 2021 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: switch_control
//
// Description:
//  Implements control over RF switches via CtrlPort. Uses RAM to store multiple
//  ATR configurations.
//

`default_nettype none

module switch_control #(
  parameter [19:0]  BASE_ADDRESS = 0,
  parameter [19:0]  SIZE_ADDRESS = 0
) (
  // Clock and reset
  input  wire        ctrlport_clk,
  input  wire        ctrlport_rst,

  // Request
  input  wire        s_ctrlport_req_wr,
  input  wire        s_ctrlport_req_rd,
  input  wire [19:0] s_ctrlport_req_addr,
  input  wire [31:0] s_ctrlport_req_data,

  // Response
  output reg         s_ctrlport_resp_ack,
  output reg  [ 1:0] s_ctrlport_resp_status = 2'b0,
  output reg  [31:0] s_ctrlport_resp_data = 32'b0,

  // ATR switching
  input  wire [ 7:0] atr_config_rf0,
  input  wire [ 7:0] atr_config_rf1,

  //Tx0 Switch control (domain: ctrl_reg_clk)
  output wire tx0_sw1_sw2_ctrl,
  output wire tx0_sw3_a,
  output wire tx0_sw3_b,
  output wire tx0_sw4_a,
  output wire tx0_sw4_b,
  output wire tx0_sw5_a,
  output wire tx0_sw5_b,
  output wire tx0_sw6_a,
  output wire tx0_sw6_b,
  output wire tx0_sw7_a,
  output wire tx0_sw7_b,
  output wire tx0_sw8_v1,
  output wire tx0_sw8_v2,
  output wire tx0_sw8_v3,
  output wire tx0_sw9_a,
  output wire tx0_sw9_b,
  output wire tx0_sw10_a,
  output wire tx0_sw10_b,
  output wire tx0_sw11_a,
  output wire tx0_sw11_b,
  output wire tx0_sw13_v1,
  output wire tx0_sw14_v1,

  //Tx1 Switch control (domain: ctrl_reg_clk)
  output wire tx1_sw1_sw2_ctrl,
  output wire tx1_sw3_a,
  output wire tx1_sw3_b,
  output wire tx1_sw4_a,
  output wire tx1_sw4_b,
  output wire tx1_sw5_a,
  output wire tx1_sw5_b,
  output wire tx1_sw6_a,
  output wire tx1_sw6_b,
  output wire tx1_sw7_a,
  output wire tx1_sw7_b,
  output wire tx1_sw8_v1,
  output wire tx1_sw8_v2,
  output wire tx1_sw8_v3,
  output wire tx1_sw9_a,
  output wire tx1_sw9_b,
  output wire tx1_sw10_a,
  output wire tx1_sw10_b,
  output wire tx1_sw11_a,
  output wire tx1_sw11_b,
  output wire tx1_sw13_v1,
  output wire tx1_sw14_v1,

  //Rx0 Switch control (domain: ctrl_reg_clk)
  output wire rx0_sw1_a,
  output wire rx0_sw1_b,
  output wire rx0_sw2_a,
  output wire rx0_sw3_v1,
  output wire rx0_sw3_v2,
  output wire rx0_sw3_v3,
  output wire rx0_sw4_a,
  output wire rx0_sw5_a,
  output wire rx0_sw5_b,
  output wire rx0_sw6_a,
  output wire rx0_sw6_b,
  output wire rx0_sw7_sw8_ctrl,
  output wire rx0_sw9_v1,
  output wire rx0_sw10_v1,
  output wire rx0_sw11_v3,
  output wire rx0_sw11_v2,
  output wire rx0_sw11_v1,

  //Rx1 Switch control (domain: ctrl_reg_clk)
  output wire rx1_sw1_a,
  output wire rx1_sw1_b,
  output wire rx1_sw2_a,
  output wire rx1_sw3_v1,
  output wire rx1_sw3_v2,
  output wire rx1_sw3_v3,
  output wire rx1_sw4_a,
  output wire rx1_sw5_a,
  output wire rx1_sw5_b,
  output wire rx1_sw6_a,
  output wire rx1_sw6_b,
  output wire rx1_sw7_sw8_ctrl,
  output wire rx1_sw9_v1,
  output wire rx1_sw10_v1,
  output wire rx1_sw11_v3,
  output wire rx1_sw11_v2,
  output wire rx1_sw11_v1
);

  `include "../regmap/switch_setup_regmap_utils.vh"
  `include "../../../../../../lib/rfnoc/core/ctrlport.vh"

  //---------------------------------------------------------------
  // register bitfields
  //---------------------------------------------------------------
  //Tx0PathControl Register
  reg [ TX_SWITCH_1_2_SIZE-1:0] tx0_switch_1_2_reg;
  reg [   TX_SWITCH_3_SIZE-1:0] tx0_switch_3_reg;
  reg [   TX_SWITCH_4_SIZE-1:0] tx0_switch_4_reg;
  reg [   TX_SWITCH_5_SIZE-1:0] tx0_switch_5_reg;
  reg [   TX_SWITCH_6_SIZE-1:0] tx0_switch_6_reg;
  reg [   TX_SWITCH_7_SIZE-1:0] tx0_switch_7_reg;
  reg [   TX_SWITCH_8_SIZE-1:0] tx0_switch_8_reg;
  reg [   TX_SWITCH_9_SIZE-1:0] tx0_switch_9_reg;
  reg [  TX_SWITCH_10_SIZE-1:0] tx0_switch_10_reg;
  reg [  TX_SWITCH_11_SIZE-1:0] tx0_switch_11_reg;
  reg [  TX_SWITCH_13_SIZE-1:0] tx0_switch_13_reg;
  reg [  TX_SWITCH_14_SIZE-1:0] tx0_switch_14_reg;

  //Tx1PathControl Register
  reg [ TX_SWITCH_1_2_SIZE-1:0] tx1_switch_1_2_reg;
  reg [   TX_SWITCH_3_SIZE-1:0] tx1_switch_3_reg;
  reg [   TX_SWITCH_4_SIZE-1:0] tx1_switch_4_reg;
  reg [   TX_SWITCH_5_SIZE-1:0] tx1_switch_5_reg;
  reg [   TX_SWITCH_6_SIZE-1:0] tx1_switch_6_reg;
  reg [   TX_SWITCH_7_SIZE-1:0] tx1_switch_7_reg;
  reg [   TX_SWITCH_8_SIZE-1:0] tx1_switch_8_reg;
  reg [   TX_SWITCH_9_SIZE-1:0] tx1_switch_9_reg;
  reg [  TX_SWITCH_10_SIZE-1:0] tx1_switch_10_reg;
  reg [  TX_SWITCH_11_SIZE-1:0] tx1_switch_11_reg;
  reg [  TX_SWITCH_13_SIZE-1:0] tx1_switch_13_reg;
  reg [  TX_SWITCH_14_SIZE-1:0] tx1_switch_14_reg;

  //Rx0PathControl Register
  reg [   RX_SWITCH_1_SIZE-1:0] rx0_switch_1_reg;
  reg [   RX_SWITCH_2_SIZE-1:0] rx0_switch_2_reg;
  reg [   RX_SWITCH_3_SIZE-1:0] rx0_switch_3_reg;
  reg [   RX_SWITCH_4_SIZE-1:0] rx0_switch_4_reg;
  reg [   RX_SWITCH_5_SIZE-1:0] rx0_switch_5_reg;
  reg [   RX_SWITCH_6_SIZE-1:0] rx0_switch_6_reg;
  reg [ RX_SWITCH_7_8_SIZE-1:0] rx0_switch_7_8_reg;
  reg [   RX_SWITCH_9_SIZE-1:0] rx0_switch_9_reg;
  reg [  RX_SWITCH_10_SIZE-1:0] rx0_switch_10_reg;
  reg [  RX_SWITCH_11_SIZE-1:0] rx0_switch_11_reg;

  //Rx1PathControl Register
  reg [   RX_SWITCH_1_SIZE-1:0] rx1_switch_1_reg;
  reg [   RX_SWITCH_2_SIZE-1:0] rx1_switch_2_reg;
  reg [   RX_SWITCH_3_SIZE-1:0] rx1_switch_3_reg;
  reg [   RX_SWITCH_4_SIZE-1:0] rx1_switch_4_reg;
  reg [   RX_SWITCH_5_SIZE-1:0] rx1_switch_5_reg;
  reg [   RX_SWITCH_6_SIZE-1:0] rx1_switch_6_reg;
  reg [ RX_SWITCH_7_8_SIZE-1:0] rx1_switch_7_8_reg;
  reg [   RX_SWITCH_9_SIZE-1:0] rx1_switch_9_reg;
  reg [  RX_SWITCH_10_SIZE-1:0] rx1_switch_10_reg;
  reg [  RX_SWITCH_11_SIZE-1:0] rx1_switch_11_reg;

  //---------------------------------------------------------------
  // ATR memory signals
  //---------------------------------------------------------------
  reg         ram_rx0_wea;
  wire [31:0] ram_rx0_doa;
  wire [31:0] ram_rx0_dob;

  reg         ram_rx1_wea;
  wire [31:0] ram_rx1_doa;
  wire [31:0] ram_rx1_dob;

  reg         ram_tx0_wea;
  wire [31:0] ram_tx0_doa;
  wire [31:0] ram_tx0_dob;

  reg         ram_tx1_wea;
  wire [31:0] ram_tx1_doa;
  wire [31:0] ram_tx1_dob;


  //---------------------------------------------------------------
  // Handling of CtrlPort
  //---------------------------------------------------------------
  // Check of request address is targeted for this module.
  wire address_in_range = (s_ctrlport_req_addr >= BASE_ADDRESS) && (s_ctrlport_req_addr < BASE_ADDRESS + SIZE_ADDRESS);
  // Read request shift register to align memory read and response generation.
  reg  [ 1:0] read_req_shift_reg = 2'b0;
  // Mask out 8 bits for ATR configurations to be able to compare all ATR
  // configurations against the same base register address.
  wire [31:0] register_base_address = {s_ctrlport_req_addr[19:10], 8'b0, s_ctrlport_req_addr[1:0]};
  // Extract masked out bits from the address, which represent the register
  // array index = ATR configuration index
  wire [ 7:0] register_index = s_ctrlport_req_addr[9:2];

  always @(posedge ctrlport_clk) begin
    // reset internal registers and responses
    if (ctrlport_rst) begin
      s_ctrlport_resp_ack <= 1'b0;

      read_req_shift_reg <= 2'b0;

      ram_tx0_wea <= 1'b0;
      ram_tx1_wea <= 1'b0;
      ram_rx0_wea <= 1'b0;
      ram_rx1_wea <= 1'b0;

    end else begin
      // default assignments
      read_req_shift_reg <= {read_req_shift_reg[0], s_ctrlport_req_rd};

      ram_tx0_wea <= 1'b0;
      ram_tx1_wea <= 1'b0;
      ram_rx0_wea <= 1'b0;
      ram_rx1_wea <= 1'b0;

      // write requests
      if (s_ctrlport_req_wr) begin
        // always issue an ack and no data
        s_ctrlport_resp_ack     <= 1'b1;
        s_ctrlport_resp_data    <= {32{1'bx}};
        s_ctrlport_resp_status  <= CTRL_STS_OKAY;

        case (register_base_address)
          BASE_ADDRESS + TX0_PATH_CONTROL(0): begin
            ram_tx0_wea <= 1'b1;
          end

          BASE_ADDRESS + TX1_PATH_CONTROL(0): begin
            ram_tx1_wea <= 1'b1;
          end

          BASE_ADDRESS + RX0_PATH_CONTROL(0): begin
            ram_rx0_wea <= 1'b1;
          end

          BASE_ADDRESS + RX1_PATH_CONTROL(0): begin
            ram_rx1_wea <= 1'b1;
          end

          // error on undefined address
          default: begin
            if (address_in_range) begin
              s_ctrlport_resp_status <= CTRL_STS_CMDERR;

            // no response if out of range
            end else begin
              s_ctrlport_resp_ack <= 1'b0;
            end
          end
        endcase

      // Answer read requests delayed by 2 clock cycles. This compensated for
      // register ram_addr and the memory internal address register to make sure
      // ram_ch0_doa is up to date when generating the response.
      end else if (read_req_shift_reg[1]) begin
        // default assumption: valid request
        s_ctrlport_resp_ack     <= 1'b1;
        s_ctrlport_resp_status  <= CTRL_STS_OKAY;
        s_ctrlport_resp_data    <= {32{1'b0}};

        case (register_base_address)
          BASE_ADDRESS + TX0_PATH_CONTROL(0): begin
            s_ctrlport_resp_data <= ram_tx0_doa & TX_PATH_CONTROL_MASK;
          end
          BASE_ADDRESS + TX1_PATH_CONTROL(0): begin
            s_ctrlport_resp_data <= ram_tx1_doa & TX_PATH_CONTROL_MASK;
          end
          BASE_ADDRESS + RX0_PATH_CONTROL(0): begin
            s_ctrlport_resp_data <= ram_rx0_doa & RX_PATH_CONTROL_MASK;
          end
          BASE_ADDRESS + RX1_PATH_CONTROL(0): begin
            s_ctrlport_resp_data <= ram_rx1_doa & RX_PATH_CONTROL_MASK;
          end

          default: begin
            if (address_in_range) begin
              s_ctrlport_resp_status <= CTRL_STS_CMDERR;

            // no response if out of range
            end else begin
              s_ctrlport_resp_ack <= 1'b0;
            end
          end
        endcase

      // no request
      end else begin
        s_ctrlport_resp_ack <= 1'b0;
      end
    end
  end

  // register without reset
  reg [ 7:0] ram_addr   =  8'b0;
  reg [31:0] ram_datain = 32'b0;
  always @(posedge ctrlport_clk) begin
    // memories
    ram_addr   <= register_index;
    ram_datain <= s_ctrlport_req_data;

    // outputs
    tx0_switch_1_2_reg <= ram_tx0_dob[TX_SWITCH_1_2];
    tx0_switch_3_reg   <= ram_tx0_dob[TX_SWITCH_3_MSB  : TX_SWITCH_3];
    tx0_switch_4_reg   <= ram_tx0_dob[TX_SWITCH_4_MSB  : TX_SWITCH_4];
    tx0_switch_5_reg   <= ram_tx0_dob[TX_SWITCH_5_MSB  : TX_SWITCH_5];
    tx0_switch_6_reg   <= ram_tx0_dob[TX_SWITCH_6_MSB  : TX_SWITCH_6];
    tx0_switch_7_reg   <= ram_tx0_dob[TX_SWITCH_7_MSB  : TX_SWITCH_7];
    tx0_switch_8_reg   <= ram_tx0_dob[TX_SWITCH_8_MSB  : TX_SWITCH_8];
    tx0_switch_9_reg   <= ram_tx0_dob[TX_SWITCH_9_MSB  : TX_SWITCH_9];
    tx0_switch_10_reg  <= ram_tx0_dob[TX_SWITCH_10_MSB : TX_SWITCH_10];
    tx0_switch_11_reg  <= ram_tx0_dob[TX_SWITCH_11_MSB : TX_SWITCH_11];
    tx0_switch_13_reg  <= ram_tx0_dob[TX_SWITCH_13];
    tx0_switch_14_reg  <= ram_tx0_dob[TX_SWITCH_14];

    tx1_switch_1_2_reg <= ram_tx1_dob[TX_SWITCH_1_2];
    tx1_switch_3_reg   <= ram_tx1_dob[TX_SWITCH_3_MSB  : TX_SWITCH_3];
    tx1_switch_4_reg   <= ram_tx1_dob[TX_SWITCH_4_MSB  : TX_SWITCH_4];
    tx1_switch_5_reg   <= ram_tx1_dob[TX_SWITCH_5_MSB  : TX_SWITCH_5];
    tx1_switch_6_reg   <= ram_tx1_dob[TX_SWITCH_6_MSB  : TX_SWITCH_6];
    tx1_switch_7_reg   <= ram_tx1_dob[TX_SWITCH_7_MSB  : TX_SWITCH_7];
    tx1_switch_8_reg   <= ram_tx1_dob[TX_SWITCH_8_MSB  : TX_SWITCH_8];
    tx1_switch_9_reg   <= ram_tx1_dob[TX_SWITCH_9_MSB  : TX_SWITCH_9];
    tx1_switch_10_reg  <= ram_tx1_dob[TX_SWITCH_10_MSB : TX_SWITCH_10];
    tx1_switch_11_reg  <= ram_tx1_dob[TX_SWITCH_11_MSB : TX_SWITCH_11];
    tx1_switch_13_reg  <= ram_tx1_dob[TX_SWITCH_13];
    tx1_switch_14_reg  <= ram_tx1_dob[TX_SWITCH_14];

    rx0_switch_1_reg   <= ram_rx0_dob[RX_SWITCH_1_MSB : RX_SWITCH_1];
    rx0_switch_2_reg   <= ram_rx0_dob[RX_SWITCH_2];
    rx0_switch_3_reg   <= ram_rx0_dob[RX_SWITCH_3_MSB : RX_SWITCH_3];
    rx0_switch_4_reg   <= ram_rx0_dob[RX_SWITCH_4];
    rx0_switch_5_reg   <= ram_rx0_dob[RX_SWITCH_5_MSB : RX_SWITCH_5];
    rx0_switch_6_reg   <= ram_rx0_dob[RX_SWITCH_6_MSB : RX_SWITCH_6];
    rx0_switch_7_8_reg <= ram_rx0_dob[RX_SWITCH_7_8];
    rx0_switch_9_reg   <= ram_rx0_dob[RX_SWITCH_9];
    rx0_switch_10_reg  <= ram_rx0_dob[RX_SWITCH_10];
    rx0_switch_11_reg  <= ram_rx0_dob[RX_SWITCH_11_MSB: RX_SWITCH_11];

    rx1_switch_1_reg   <= ram_rx1_dob[RX_SWITCH_1_MSB : RX_SWITCH_1];
    rx1_switch_2_reg   <= ram_rx1_dob[RX_SWITCH_2];
    rx1_switch_3_reg   <= ram_rx1_dob[RX_SWITCH_3_MSB : RX_SWITCH_3];
    rx1_switch_4_reg   <= ram_rx1_dob[RX_SWITCH_4];
    rx1_switch_5_reg   <= ram_rx1_dob[RX_SWITCH_5_MSB : RX_SWITCH_5];
    rx1_switch_6_reg   <= ram_rx1_dob[RX_SWITCH_6_MSB : RX_SWITCH_6];
    rx1_switch_7_8_reg <= ram_rx1_dob[RX_SWITCH_7_8];
    rx1_switch_9_reg   <= ram_rx1_dob[RX_SWITCH_9];
    rx1_switch_10_reg  <= ram_rx1_dob[RX_SWITCH_10];
    rx1_switch_11_reg  <= ram_rx1_dob[RX_SWITCH_11_MSB: RX_SWITCH_11];
  end

`ifdef VARIANT_XO3
  localparam RAM_RW_MODE = "B-READ-ONLY" ;
`else
  localparam RAM_RW_MODE = "READ-FIRST" ;
`endif

  ram_2port #(
    .DWIDTH     (32),
    .AWIDTH     (8),
    .RW_MODE    (RAM_RW_MODE),
    .RAM_TYPE   ("AUTOMATIC"),
    .OUT_REG    (0),
    .INIT_FILE  ("register_endpoints/memory_init_files/tx0_path_defaults.hex")
  ) ram_tx0_i (
    .clka   (ctrlport_clk),
    .ena    (1'b1),
    .wea    (ram_tx0_wea),
    .addra  (ram_addr),
    .dia    (ram_datain),
    .doa    (ram_tx0_doa),
    .clkb   (ctrlport_clk),
    .enb    (1'b1),
    .web    (1'b0),
    .addrb  (atr_config_rf0),
    .dib    (0),
    .dob    (ram_tx0_dob));

  ram_2port #(
    .DWIDTH     (32),
    .AWIDTH     (8),
    .RW_MODE    (RAM_RW_MODE),
    .RAM_TYPE   ("AUTOMATIC"),
    .OUT_REG    (0),
    .INIT_FILE  ("register_endpoints/memory_init_files/tx1_path_defaults.hex")
  ) ram_tx1_i (
    .clka   (ctrlport_clk),
    .ena    (1'b1),
    .wea    (ram_tx1_wea),
    .addra  (ram_addr),
    .dia    (ram_datain),
    .doa    (ram_tx1_doa),
    .clkb   (ctrlport_clk),
    .enb    (1'b1),
    .web    (1'b0),
    .addrb  (atr_config_rf1),
    .dib    (0),
    .dob    (ram_tx1_dob));

  ram_2port #(
    .DWIDTH     (32),
    .AWIDTH     (8),
    .RW_MODE    (RAM_RW_MODE),
    .RAM_TYPE   ("AUTOMATIC"),
    .OUT_REG    (0),
    .INIT_FILE  ("register_endpoints/memory_init_files/rx0_path_defaults.hex")
  ) ram_rx0_i (
    .clka   (ctrlport_clk),
    .ena    (1'b1),
    .wea    (ram_rx0_wea),
    .addra  (ram_addr),
    .dia    (ram_datain),
    .doa    (ram_rx0_doa),
    .clkb   (ctrlport_clk),
    .enb    (1'b1),
    .web    (1'b0),
    .addrb  (atr_config_rf0),
    .dib    (0),
    .dob    (ram_rx0_dob));

  ram_2port #(
    .DWIDTH     (32),
    .AWIDTH     (8),
    .RW_MODE    (RAM_RW_MODE),
    .RAM_TYPE   ("AUTOMATIC"),
    .OUT_REG    (0),
    .INIT_FILE  ("register_endpoints/memory_init_files/rx1_path_defaults.hex")
  ) ram_rx1_i (
    .clka   (ctrlport_clk),
    .ena    (1'b1),
    .wea    (ram_rx1_wea),
    .addra  (ram_addr),
    .dia    (ram_datain),
    .doa    (ram_rx1_doa),
    .clkb   (ctrlport_clk),
    .enb    (1'b1),
    .web    (1'b0),
    .addrb  (atr_config_rf1),
    .dib    (0),
    .dob    (ram_rx1_dob));

  assign tx0_sw1_sw2_ctrl = tx0_switch_1_2_reg;
  assign tx0_sw3_a        = tx0_switch_3_reg[0];
  assign tx0_sw3_b        = tx0_switch_3_reg[1];
  assign tx0_sw4_a        = tx0_switch_4_reg[0];
  assign tx0_sw4_b        = tx0_switch_4_reg[1];
  assign tx0_sw5_a        = tx0_switch_5_reg[0];
  assign tx0_sw5_b        = tx0_switch_5_reg[1];
  assign tx0_sw6_a        = tx0_switch_6_reg[0];
  assign tx0_sw6_b        = tx0_switch_6_reg[1];
  assign tx0_sw7_a        = tx0_switch_7_reg[0];
  assign tx0_sw7_b        = tx0_switch_7_reg[1];
  assign tx0_sw8_v1       = tx0_switch_8_reg[0];
  assign tx0_sw8_v2       = tx0_switch_8_reg[1];
  assign tx0_sw8_v3       = tx0_switch_8_reg[2];
  assign tx0_sw9_a        = tx0_switch_9_reg[0];
  assign tx0_sw9_b        = tx0_switch_9_reg[1];
  assign tx0_sw10_a       = tx0_switch_10_reg[0];
  assign tx0_sw10_b       = tx0_switch_10_reg[1];
  assign tx0_sw11_a       = tx0_switch_11_reg[0];
  assign tx0_sw11_b       = tx0_switch_11_reg[1];
  assign tx0_sw13_v1      = tx0_switch_13_reg;
  assign tx0_sw14_v1      = tx0_switch_14_reg;

  assign tx1_sw1_sw2_ctrl = tx1_switch_1_2_reg;
  assign tx1_sw3_a        = tx1_switch_3_reg[0];
  assign tx1_sw3_b        = tx1_switch_3_reg[1];
  assign tx1_sw4_a        = tx1_switch_4_reg[0];
  assign tx1_sw4_b        = tx1_switch_4_reg[1];
  assign tx1_sw5_a        = tx1_switch_5_reg[0];
  assign tx1_sw5_b        = tx1_switch_5_reg[1];
  assign tx1_sw6_a        = tx1_switch_6_reg[0];
  assign tx1_sw6_b        = tx1_switch_6_reg[1];
  assign tx1_sw7_a        = tx1_switch_7_reg[0];
  assign tx1_sw7_b        = tx1_switch_7_reg[1];
  assign tx1_sw8_v1       = tx1_switch_8_reg[0];
  assign tx1_sw8_v2       = tx1_switch_8_reg[1];
  assign tx1_sw8_v3       = tx1_switch_8_reg[2];
  assign tx1_sw9_a        = tx1_switch_9_reg[0];
  assign tx1_sw9_b        = tx1_switch_9_reg[1];
  assign tx1_sw10_a       = tx1_switch_10_reg[0];
  assign tx1_sw10_b       = tx1_switch_10_reg[1];
  assign tx1_sw11_a       = tx1_switch_11_reg[0];
  assign tx1_sw11_b       = tx1_switch_11_reg[1];
  assign tx1_sw13_v1      = tx1_switch_13_reg;
  assign tx1_sw14_v1      = tx1_switch_14_reg;

  assign rx0_sw1_a        = rx0_switch_1_reg[0];
  assign rx0_sw1_b        = rx0_switch_1_reg[1];
  assign rx0_sw2_a        = rx0_switch_2_reg;
  assign rx0_sw3_v1       = rx0_switch_3_reg[0];
  assign rx0_sw3_v2       = rx0_switch_3_reg[1];
  assign rx0_sw3_v3       = rx0_switch_3_reg[2];
  assign rx0_sw4_a        = rx0_switch_4_reg;
  assign rx0_sw5_a        = rx0_switch_5_reg[0];
  assign rx0_sw5_b        = rx0_switch_5_reg[1];
  assign rx0_sw6_a        = rx0_switch_6_reg[0];
  assign rx0_sw6_b        = rx0_switch_6_reg[1];
  assign rx0_sw7_sw8_ctrl = rx0_switch_7_8_reg;
  assign rx0_sw9_v1       = rx0_switch_9_reg;
  assign rx0_sw10_v1      = rx0_switch_10_reg;
  assign rx0_sw11_v1      = rx0_switch_11_reg[0];
  assign rx0_sw11_v2      = rx0_switch_11_reg[1];
  assign rx0_sw11_v3      = rx0_switch_11_reg[2];

  assign rx1_sw1_a        = rx1_switch_1_reg[0];
  assign rx1_sw1_b        = rx1_switch_1_reg[1];
  assign rx1_sw2_a        = rx1_switch_2_reg;
  assign rx1_sw3_v1       = rx1_switch_3_reg[0];
  assign rx1_sw3_v2       = rx1_switch_3_reg[1];
  assign rx1_sw3_v3       = rx1_switch_3_reg[2];
  assign rx1_sw4_a        = rx1_switch_4_reg;
  assign rx1_sw5_a        = rx1_switch_5_reg[0];
  assign rx1_sw5_b        = rx1_switch_5_reg[1];
  assign rx1_sw6_a        = rx1_switch_6_reg[0];
  assign rx1_sw6_b        = rx1_switch_6_reg[1];
  assign rx1_sw7_sw8_ctrl = rx1_switch_7_8_reg;
  assign rx1_sw9_v1       = rx1_switch_9_reg;
  assign rx1_sw10_v1      = rx1_switch_10_reg;
  assign rx1_sw11_v1      = rx1_switch_11_reg[0];
  assign rx1_sw11_v2      = rx1_switch_11_reg[1];
  assign rx1_sw11_v3      = rx1_switch_11_reg[2];

endmodule

`default_nettype wire

//XmlParse xml_on
//<regmap name="SWITCH_SETUP_REGMAP" readablestrobes="false" generatevhdl="true" ettusguidelines="true">
//  <group name="SWITCH_SETUP_REGISTERS">
//    <info>
//      The following registers are used to control the path that the RF signal
//      takes for both Tx and Rx{BR/}{BR/}
//    </info>
//    <regtype name="TX_PATH_CONTROL" size="32" attributes="Readable|Writable">
//      <info>
//        This Register controls the switches along the Tx path. Note: default
//        values refer to the RX0 path. RX1 has the same defaults, but their
//        bit values may differ.
//      </info>
//      <bitfield name="TX_SWITCH_1_2" range="0" initialvalue="0">
//        <info>
//          Write 0 to select Tx IF2 filter 2, CF = 2050 MHz, BW = 400 MHz{BR/}
//          Write 1 to select Tx IF2 filter 1, CF = 1060 MHz, BW = 400 MHz{BR/}
//        </info>
//      </bitfield>
//      <bitfield name="TX_SWITCH_3" range="3..2" initialvalue="0">
//        <info>
//          {font color="red"}note to digital designer: control A is LSB, and control B is MSB{/font}{BR/}
//          Control for Tx Switch 3. The configuration of this switch changes between TX paths.{BR/}
//          {b}FOR TX0:{/b}{BR/}
//          Write 0 to select Tx If1 Filter 1,2,3, or 50 ohm termination. See @.TX_SWITCH_4 for those controls{BR/}
//          Write 1 to select Tx If1 Filter 4, 5.7 GHz to 6.4 GHz{BR/}
//          Write 2 to select Tx If1 Filter 6, 7.0 GHz to 8.0 GHz{BR/}
//          Write 3 to select Tx If1 Filter 5, 6.4 GHz to 7.0 GHz{BR/}
//          {b}FOR TX1:{/b}{BR/}
//          Write 0 to select Tx If1 Filter 6, 7.0 GHz to 8.0 GHz{BR/}
//          Write 1 to select Tx If1 Filter 5, 6.4 GHz to 7.0 GHz{BR/}
//          Write 2 to select Tx If1 Filter 4, 5.7 GHz to 6.4 GHz{BR/}
//          Write 3 to select Tx If1 Filter 1,2,3, or 50 ohm termination. See @.TX_SWITCH_4 for those controls{BR/}
//        </info>
//      </bitfield>
//      <bitfield name="TX_SWITCH_4" range="5..4" initialvalue="0">
//        <info>
//          {font color="red"}note to digital designer: control A is LSB, and control B is MSB{/font}{BR/}
//          Control for Tx Switch 4. This switch path is only taken if @.TX_SWITCH_4 is set to 0.
//          The configuration of this switch changes between TX paths.{BR/}
//          {b}FOR TX0:{/b}{BR/}
//          Write 0 to select 50 ohm termination{BR/}
//          Write 1 to select Tx If1 Filter 1, 3.1 GHz to 4.3 GHz{BR/}
//          Write 2 to select Tx If1 Filter 2, 4.3 GHz to 5.1 GHz{BR/}
//          Write 3 to select Tx If1 Filter 3, 5.1 GHz to 5.7 GHz{BR/}
//          {b}FOR TX1:{/b}{BR/}
//          Write 0 to select Tx If1 Filter 3, 5.1 GHz to 5.7 GHz{BR/}
//          Write 1 to select Tx If1 Filter 2, 4.3 GHz to 5.1 GHz{BR/}
//          Write 2 to select Tx If1 Filter 1, 3.1 GHz to 4.3 GHz{BR/}
//          Write 3 to select 50 ohm termination{BR/}
//        </info>
//      </bitfield>
//      <bitfield name="TX_SWITCH_5" range="7..6" initialvalue="0">
//        <info>
//          {font color="red"}note to digital designer: control A is LSB, and control B is MSB{/font}{BR/}
//          Control for Tx Switch 5. This switch path is only taken if @.TX_SWITCH_6 is set to 0.
//          The configuration of this switch changes between TX paths.{BR/}
//          {b}FOR TX0:{/b}{BR/}
//          Write 0 to select Tx If1 Filter 3, 5.1 GHz to 5.7 GHz{BR/}
//          Write 1 to select Tx If1 Filter 2, 4.3 GHz to 5.1 GHz{BR/}
//          Write 2 to select Tx If1 Filter 1, 3.1 GHz to 4.3 GHz{BR/}
//          Write 3 to select Tx If1 Filter 50 ohm termination{BR/}
//          {b}FOR TX1:{/b}{BR/}
//          Write 0 to select Tx If1 Filter 50 ohm termination{BR/}
//          Write 1 to select Tx If1 Filter 1, 3.1 GHz to 4.3 GHz{BR/}
//          Write 2 to select Tx If1 Filter 2, 4.3 GHz to 5.1 GHz{BR/}
//          Write 3 to select Tx If1 Filter 3, 5.1 GHz to 5.7 GHz{BR/}
//        </info>
//      </bitfield>
//      <bitfield name="TX_SWITCH_6" range="9..8" initialvalue="0">
//        <info>
//          {font color="red"}note to digital designer: control A is LSB, and control B is MSB{/font}{BR/}
//          Control for Tx Switch 6.
//          The configuration of this switch changes between TX paths.{BR/}
//          {b}FOR TX0:{/b}{BR/}
//          Write 0 to select Tx If1 Filter 6, 7.0 GHz to 8.0 GHz{BR/}
//          Write 1 to select Tx If1 Filter 5, 6.4 GHz to 7.0 GHz{BR/}
//          Write 2 to select Tx If1 Filter 4, 5.7 GHz to 6.4 GHz{BR/}
//          Write 3 to select Tx If1 Filter 1, 2, 3, or 50 ohm termination. See @.TX_SWITCH_5 for those controls{/font}{BR/}
//          {b}FOR TX1:{/b}{BR/}
//          Write 0 to select Tx If1 Filter 1, 2, 3, or 50 ohm termination. See @.TX_SWITCH_5 for those controls{/font}{BR/}
//          Write 1 to select Tx If1 Filter 4, 5.7 GHz to 6.4 GHz{BR/}
//          Write 2 to select Tx If1 Filter 5, 6.4 GHz to 7.0 GHz{BR/}
//          Write 3 to select Tx If1 Filter 6, 7.0 GHz to 8.0 GHz{BR/}
//        </info>
//      </bitfield>
//      <bitfield name="TX_SWITCH_7" range="11..10" initialvalue="0">
//        <info>
//          {font color="red"}note to digital designer: control A is LSB, and control B is MSB{/font}{BR/}
//          Control for Tx Switch 7.
//          The configuration of this switch changes between TX paths.{BR/}
//          {b}FOR TX0:{/b}{BR/}
//          Write 0 to select 50 ohm termination{BR/}
//          Write 1 to select no connect{BR/}
//          Write 2 to select Tx highBand RF4 path, 3.1 GHz to 8 GHz{BR/}
//          Write 3 to select Tx lowbands RF1, RF2, RF3 path. See @.TX_SWITCH_8 for those controls{BR/}
//          {b}FOR TX1:{/b}{BR/}
//          Write 0 to select Tx lowbands RF1, RF2, RF3 path. See @.TX_SWITCH_8 for those controls{BR/}
//          Write 1 to select Tx highBand RF4 path, 3.1 GHz to 8 GHz{BR/}
//          Write 2 to select no connect{BR/}
//          Write 3 to select 50 ohm termination{BR/}
//        </info>
//      </bitfield>
//      <bitfield name="TX_SWITCH_8" range="14..12" initialvalue="0">
//        <info>
//          Control for Tx Switch 8, note this is one hot encoding and not binary.
//          The configuration of this switch changes between TX paths.{BR/}
//          {b}FOR TX0:{/b}{BR/}
//          Write 1 to select Tx RF3 path, 2.3  GHz to 3.1 GHz{BR/}
//          Write 2 to select Tx RF1 path, 1.0  MHz to 1.95 GHz{BR/}
//          Write 4 to select Tx RF2 path, 1.95 GHz to 2.3 GHz{BR/}
//          {b}FOR TX1:{/b}{BR/}
//          Write 1 to select Tx RF2 path, 1.95 GHz to 2.3 GHz{BR/}
//          Write 2 to select Tx RF1 path, 1.0  MHz to 1.95 GHz{BR/}
//          Write 4 to select Tx RF3 path, 2.3  GHz to 3.1 GHz{BR/}
//          {i}*All other values are invalid{/i}{BR/}
//        </info>
//      </bitfield>
//      <bitfield name="TX_SWITCH_9" range="17..16" initialvalue="0">
//        <info>
//          {font color="red"}note to digital designer: control A is LSB, and control B is MSB{/font}{BR/}
//          Control for Tx Switch 9.
//          The configuration of this switch changes between TX paths.{BR/}
//          {b}FOR TX0:{/b}{BR/}
//          Write 0 to select Tx RF3 path, 2.3  GHz to 3.1  GHz{BR/}
//          Write 1 to select Tx RF1 path, 1.0  MHz to 1.95 GHz{BR/}
//          Write 2 to select Tx RF2 path, 1.95 GHz to 2.3  GHz{BR/}
//          Write 3 to select Tx RF4 path, 3.1  GHz to 8.0  GHz{BR/}
//          {b}FOR TX1:{/b}{BR/}
//          Write 0 to select Tx RF4 path, 3.1  GHz to 8.0  GHz{BR/}
//          Write 1 to select Tx RF2 path, 1.95 GHz to 2.3  GHz{BR/}
//          Write 2 to select Tx RF1 path, 1.0  MHz to 1.95 GHz{BR/}
//          Write 3 to select Tx RF3 path, 2.3  GHz to 3.1  GHz{BR/}
//        </info>
//      </bitfield>
//      <bitfield name="TX_SWITCH_10" range="19..18" initialvalue="0">
//        <info>
//          {font color="red"}note to digital designer: control A is LSB, and control B is MSB{/font}{BR/}
//          Control for Tx Switch 10.
//          The configuration of this switch changes between TX paths.{BR/}
//          {b}FOR TX0:{/b}{BR/}
//          Write 0 to select Tx amplifier bypass path{BR/}
//          Write 1 to select Tx calibration loopback path{BR/}
//          Write 2 to select Tx lowband amp path. @.TX_Switch_11 must also match this path.{BR/}
//          Write 3 to select Tx highband amp path. @.TX_Switch_11 must also match this path.{BR/}
//          {b}FOR TX1:{/b}{BR/}
//          Write 0 to select Tx highband amp path. @.TX_Switch_11 must also match this path.{BR/}
//          Write 1 to select Tx lowband amp path. @.TX_Switch_11 must also match this path.{BR/}
//          Write 2 to select Tx amplifier bypass path{BR/}
//          Write 3 to select Tx calibration loopback path{BR/}
//        </info>
//      </bitfield>
//      <bitfield name="TX_SWITCH_11" range="21..20" initialvalue="0">
//        <info>
//          {font color="red"}note to digital designer: control A is LSB, and control B is MSB{/font}{BR/}
//          Control for Tx Switch 11.
//          The configuration of this switch changes between TX paths.{BR/}
//          {b}FOR TX0:{/b}{BR/}
//          Write 0 to select Tx Rx path, @.RX_SWITCH_1 must also select the correct path{BR/}
//          Write 1 to select Tx highband amp path. @.TX_SWITCH_10 must also match this path.{BR/}
//          Write 2 to select Tx lowband amp path. @.TX_SWITCH_10 must also match this path.{BR/}
//          Write 3 to select Tx amplifier bypass path{BR/}
//          {b}FOR TX1:{/b}{BR/}
//          Write 0 to select Tx Rx path, @.RX_SWITCH_1 must also select the correct path{BR/}
//          Write 1 to select Tx amplifier bypass path{BR/}
//          Write 2 to select Tx lowband amp path. @.TX_SWITCH_10 must also match this path.{BR/}
//          Write 3 to select Tx highband amp path. @.TX_SWITCH_10 must also match this path.{BR/}
//        </info>
//      </bitfield>
//      <bitfield name="TX_SWITCH_13" range="24" initialvalue="0">
//        <info>
//          Control for Tx0 Switch 13 LO path.
//          The configuration of this switch changes between TX paths.{BR/}
//          {b}FOR TX0:{/b}{BR/}
//          Write 0 to select Tx0 internal LO path{BR/}
//          Write 1 to select Tx0 external LO path{BR/}
//          {b}FOR TX1:{/b}{BR/}
//          Write 0 to select Tx0 external LO path{BR/}
//          Write 1 to select Tx0 internal LO path{BR/}
//        </info>
//      </bitfield>
//      <bitfield name="TX_SWITCH_14" range="26" initialvalue="0">
//        <info>
//          Control for Tx Switch 13 LO path.
//          The configuration of this switch changes between TX paths.{BR/}
//          {b}FOR TX0:{/b}{BR/}
//          Write 0 to select Tx external LO path{BR/}
//          Write 1 to select Tx internal LO path{BR/}
//          {b}FOR TX1:{/b}{BR/}
//          Write 0 to select Tx internal LO path{BR/}
//          Write 1 to select Tx external LO path{BR/}
//        </info>
//      </bitfield>
//    </regtype>
//
//    <regtype name="RX_PATH_CONTROL" size="32" attributes="Readable|Writable">
//      <info>
//        This Register controls switches along Rx paths. Note: default
//        values refer to the RX0 path. RX1 has the same defaults, but their
//        bit values may differ.
//      </info>
//      <bitfield name="RX_SWITCH_1" range="1..0" initialvalue="0">
//        <info>
//          {font color="red"}note to digital designer: control A is LSB, and control B is MSB{/font}{BR/}
//          Control for Rx Switch 1.
//          The configuration of this switch changes between RX paths.{BR/}
//          {b}FOR RX0:{/b}{BR/}
//          Write 0 to select Rx calibration loopback{BR/}
//          Write 1 to select Rx 50 ohm termination path{BR/}
//          Write 2 to select Tx Rx path, @.TX_SWITCH_11 must also select the correct path{BR/}
//          Write 3 to select Rx input port{BR/}
//          {b}FOR RX1:{/b}{BR/}
//          Write 0 to select Rx calibration loopback{BR/}
//          Write 1 to select Tx Rx path, @.TX_SWITCH_11 must also select the correct path{BR/}
//          Write 2 to select Rx input port{BR/}
//          Write 3 to select Rx 50 ohm termination path{BR/}
//          </info>
//      </bitfield>
//      <bitfield name="RX_SWITCH_2" range="2" initialvalue="0">
//        <info>
//          {font color="red"}note to digital designer: control A is the only control, and control B is pulled high{/font}{BR/}
//          Control for Rx Switch 2. The configuration of this switch changes between RX paths.{BR/}
//          {b}FOR RX0:{/b}{BR/}
//          Write 0 to select Rx RF3 highband path{BR/}
//          Write 1 to select Rx RF1/2 lowband path{BR/}
//          {b}FOR RX1:{/b}{BR/}
//          Write 0 to select Rx RF1/2 lowband path{BR/}
//          Write 1 to select Rx RF3 highband path{BR/}
//          </info>
//      </bitfield>
//      <bitfield name="RX_SWITCH_3" range="6..4" initialvalue="0">
//        <info>
//          Control for Rx Switch 3, note this is one hot encoding and not binary.
//          The configuration of this switch changes between RX paths.{BR/}
//          {b}FOR RX0:{/b}{BR/}
//          Write 1 to select Rx RF filter 2 path, 1.80 GHz - 2.30 GHz{BR/}
//          Write 2 to select Rx RF filter 1 path, 1.00 MHz - 1.80 GHz{BR/}
//          Write 4 to select Rx RF filter 3 path, 2.30 MHz - 3.00 GHz{BR/}
//          {b}FOR RX1:{/b}{BR/}
//          Write 1 to select Rx RF filter 3 path, 2.30 MHz - 3.00 GHz{BR/}
//          Write 2 to select Rx RF filter 1 path, 1.00 MHz - 1.80 GHz{BR/}
//          Write 4 to select Rx RF filter 2 path, 1.80 GHz - 2.30 GHz{BR/}
//          {i}*All other values are invalid{/i}{BR/}
//          </info>
//      </bitfield>
//      <bitfield name="RX_SWITCH_4" range="8" initialvalue="0">
//        <info>
//          {font color="red"}note to digital designer: control A is the only control, and control B is tied to ground{/font}{BR/}
//          Control for Rx Switch 4.{BR/}
//          Write 0 to select Rx RF1/2 lowband path{BR/}
//          Write 1 to select Rx RF3 highband path{BR/}
//          </info>
//      </bitfield>
//      <bitfield name="RX_SWITCH_5" range="11..10" initialvalue="0">
//        <info>
//          {font color="red"}note to digital designer: control A is LSB, and control B is MSB{/font}{BR/}{BR/}
//
//          Control for Rx Switch 5. The configuration of this switch changes between RX paths.{BR/}
//          {b}FOR RX0:{/b}{BR/}
//          Write 0 to select Rx RF filter 4 path, 7.0 - 8 GHz GHz, @.RX_SWITCH_6 must also select this path{BR/}
//          Write 1 to select Rx RF filter 3 path, 5.6 - 8 GHz, @.RX_SWITCH_6 must also select this path{BR/}
//          Write 2 to select Rx RF filter 2 path, 4.2 - 5.6 GHz, @.RX_SWITCH_6 must also select this path{BR/}
//          Write 3 to select Rx RF filter 1 path, 3.0 - 4.2 GHz, @.RX_SWITCH_6 must also select this path{BR/}
//          {b}FOR RX1:{/b}{BR/}
//          Write 0 to select Rx RF filter 1 path, 3.0 - 4.2 GHz, @.RX_SWITCH_6 must also select this path{BR/}
//          Write 1 to select Rx RF filter 2 path, 4.2 - 5.6 GHz, @.RX_SWITCH_6 must also select this path{BR/}
//          Write 2 to select Rx RF filter 3 path, 5.6 - 8 GHz, @.RX_SWITCH_6 must also select this path{BR/}
//          Write 3 to select Rx RF filter 4 path, 7.0 - 8 GHz GHz, @.RX_SWITCH_6 must also select this path{BR/}
//          </info>
//      </bitfield>
//      <bitfield name="RX_SWITCH_6" range="13..12" initialvalue="0">
//        <info>
//          {font color="red"}note to digital designer: control A is LSB, and control B is MSB{/font}{BR/}{BR/}
//
//          Control for Rx Switch 6. The configuration of this switch changes between RX paths.{BR/}
//          {b}FOR RX0:{/b}{BR/}
//          Write 0 to select Rx RF filter 1 path, 3.0 - 4.2 GHz, @.RX_SWITCH_6 must also select this path{BR/}
//          Write 1 to select Rx RF filter 2 path, 4.2 - 5.6 GHz, @.RX_SWITCH_6 must also select this path{BR/}
//          Write 2 to select Rx RF filter 3 path, 5.6 - 8 GHz, @.RX_SWITCH_6 must also select this path{BR/}
//          Write 3 to select Rx RF filter 4 path, 7.0 - 8 GHz GHz, @.RX_SWITCH_6 must also select this path{BR/}
//          {b}FOR RX1:{/b}{BR/}
//          Write 0 to select Rx RF filter 4 path, 7.0 - 8 GHz GHz, @.RX_SWITCH_6 must also select this path{BR/}
//          Write 1 to select Rx RF filter 3 path, 5.6 - 8 GHz, @.RX_SWITCH_6 must also select this path{BR/}
//          Write 2 to select Rx RF filter 2 path, 4.2 - 5.6 GHz, @.RX_SWITCH_6 must also select this path{BR/}
//          Write 3 to select Rx RF filter 1 path, 3.0 - 4.2 GHz, @.RX_SWITCH_6 must also select this path{BR/}
//          </info>
//      </bitfield>
//      <bitfield name="RX_SWITCH_7_8" range="14" initialvalue="0">
//        <info>
//          Shared control for Rx switch 7 and switch 8.{BR/}
//          {b}FOR RX0:{/b}{BR/}
//          Write 0 to select Rx IF2 filter 2, CF = 2050 MHz, BW = 400 MHz{BR/}
//          Write 1 to select Rx IF2 filter 1, CF = 1060 MHz, BW = 400 MHz{BR/}
//          {b}FOR RX1:{/b}{BR/}
//          Write 0 to select Rx IF2 filter 1, CF = 1060 MHz, BW = 400 MHz{BR/}
//          Write 1 to select Rx IF2 filter 2, CF = 2050 MHz, BW = 400 MHz{BR/}
//        </info>
//      </bitfield>
//      <bitfield name="RX_SWITCH_9" range="16" initialvalue="0">
//        <info>
//          Control for Rx Switch 9 LO path. The configuration of this switch changes between RX paths.{BR/}
//          {b}FOR RX0:{/b}{BR/}
//          Write 0 to select Rx internal LO path{BR/}
//          Write 1 to select Rx external LO path{BR/}
//          {b}FOR RX1:{/b}{BR/}
//          Write 0 to select Rx external LO path{BR/}
//          Write 1 to select Rx internal LO path{BR/}
//        </info>
//      </bitfield>
//      <bitfield name="RX_SWITCH_10" range="18" initialvalue="0">
//        <info>
//          Control for Rx Switch 10 LO path. The configuration of this switch changes between RX paths.{BR/}
//          {b}FOR RX0:{/b}{BR/}
//          Write 0 to select Rx internal LO path{BR/}
//          Write 1 to select Rx external LO path{BR/}
//          {b}FOR RX1:{/b}{BR/}
//          Write 0 to select Rx external LO path{BR/}
//          Write 1 to select Rx internal LO path{BR/}
//        </info>
//      </bitfield>
//      <bitfield name="RX_SWITCH_11" range="22..20" initialvalue="0">
//        <info>
//          Control for Rx Switch 11, note to digital designer: Control V2 is pulled to ground.{BR/}{BR/}
//          The configuration of this switch changes between RX paths.{BR/}
//          {b}FOR RX0:{/b}{BR/}
//          Write 1 to select Rx1 RF filter 3 path, 2.30 MHz - 3.00 GHz{BR/}
//          Write 2 to select Rx1 RF filter 1 path, 1.00 MHz - 1.80 GHz{BR/}
//          Write 4 to select Rx1 RF filter 2 path, 1.80 GHz - 2.30 GHz{BR/}
//          {b}FOR RX1:{/b}{BR/}
//          Write 1 to select Rx1 RF filter 2 path, 1.80 GHz - 2.30 GHz{BR/}
//          Write 2 to select Rx1 RF filter 1 path, 1.00 MHz - 1.80 GHz{BR/}
//          Write 4 to select Rx1 RF filter 3 path, 2.30 MHz - 3.00 GHz{BR/}
//          </info>
//      </bitfield>
//    </regtype>
//
//    <register name="TX0_PATH_CONTROL" typename="TX_PATH_CONTROL" offset="0x00" count="256" step="4">
//      <info>
//        This Register controls the Tx0 paths.{br}
//        This register array can hold settings for all ATR configurations.
//        The register index equals the ATR configuration.
//        The active configuration can be selected in @.ATR_REGMAP.
//        Independently all configurations can be read/written at any time.
//      </info>
//    </register>
//    <register name="TX1_PATH_CONTROL" typename="TX_PATH_CONTROL" offset="0x400" count="256" step="4">
//      <info>
//        This Register controls the Tx1 paths.{br}
//        This register array can hold settings for all ATR configurations.
//        The register index equals the ATR configuration.
//        The active configuration can be selected in @.ATR_REGMAP.
//        Independently all configurations can be read/written at any time.
//      </info>
//    </register>
//
//    <register name="RX0_PATH_CONTROL" typename="RX_PATH_CONTROL" offset="0x800" count="256" step="4">
//      <info>
//        This Register controls the Rx0 paths.{br}
//        This register array can hold settings for all ATR configurations.
//        The register index equals the ATR configuration.
//        The active configuration can be selected in @.ATR_REGMAP.
//        Independently all configurations can be read/written at any time.
//      </info>
//    </register>
//    <register name="RX1_PATH_CONTROL" typename="RX_PATH_CONTROL" offset="0xC00" count="256" step="4">
//      <info>
//        This Register controls the Rx1 paths.{br}
//        This register array can hold settings for all ATR configurations.
//        The register index equals the ATR configuration.
//        The active configuration can be selected in @.ATR_REGMAP.
//        Independently all configurations can be read/written at any time.
//      </info>
//    </register>
//  </group>
//</regmap>
//XmlParse xml_off

//
// Copyright 2021 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: dsa_control
//
// Description:
//  Implements control over Digital Step Attenuators via CtrlPort. Uses RAM to
//  store multiple ATR configurations. Provides gain table to abstract from raw
//  DSA values.
//
//  IMPORTANT: The default values here must be synchronized with the default
//  values in gen_defaults.py, they are not automatically kept in sync.
//

`default_nettype none

module dsa_control #(
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

  // The attenuation setting for TX paths is indexed from two,
  // to match schematic naming. In this case, the two LSBs
  // for parallel control going into the DSA chips are connected
  // to ground(those bits control fractional attenuation).

  //Tx0 DSA control (domain: ctrl_reg_clk)
  output wire [6:2] tx0_dsa1,
  output wire [6:2] tx0_dsa2,

  //Tx1 DSA control (domain: ctrl_reg_clk)
  output wire [6:2] tx1_dsa1,
  output wire [6:2] tx1_dsa2,

  // The attenuation setting for RX paths is indexed from one,
  // to match schematic naming. In this case, the LSB controls
  // the highest value, so re reverse the order of the vector.
  // Note the these signals are active low.

  //Rx0 DSA control (domain: ctrl_reg_clk)
  output wire [1:4] rx0_dsa1_n,
  output wire [1:4] rx0_dsa2_n,
  output wire [1:4] rx0_dsa3_a_n,
  output wire [1:4] rx0_dsa3_b_n,

  //Rx1 DSA control (domain: ctrl_reg_clk)
  output wire [1:4] rx1_dsa1_n,
  output wire [1:4] rx1_dsa2_n,
  output wire [1:4] rx1_dsa3_a_n,
  output wire [1:4] rx1_dsa3_b_n
);

  `include "../regmap/dsa_setup_regmap_utils.vh"
  `include "../../../../../../lib/rfnoc/core/ctrlport.vh"

  //---------------------------------------------------------------
  // register bitfields
  //---------------------------------------------------------------
  reg [TX_DSA1_SIZE -1:0]   tx0_dsa_1_reg   = {TX_DSA1_SIZE{1'b1}};
  reg [TX_DSA2_SIZE -1:0]   tx0_dsa_2_reg   = {TX_DSA2_SIZE{1'b1}};

  reg [TX_DSA1_SIZE -1:0]   tx1_dsa_1_reg   = {TX_DSA1_SIZE{1'b1}};
  reg [TX_DSA2_SIZE -1:0]   tx1_dsa_2_reg   = {TX_DSA2_SIZE{1'b1}};

  reg [RX_DSA1_SIZE -1:0]   rx0_dsa_1_reg   = {RX_DSA1_SIZE{1'b1}};
  reg [RX_DSA2_SIZE -1:0]   rx0_dsa_2_reg   = {RX_DSA2_SIZE{1'b1}};
  reg [RX_DSA3_A_SIZE -1:0] rx0_dsa_3_a_reg = {RX_DSA3_A_SIZE{1'b1}};
  reg [RX_DSA3_B_SIZE -1:0] rx0_dsa_3_b_reg = {RX_DSA3_B_SIZE{1'b1}};

  reg [RX_DSA1_SIZE -1:0]   rx1_dsa_1_reg   = {RX_DSA1_SIZE{1'b1}};
  reg [RX_DSA2_SIZE -1:0]   rx1_dsa_2_reg   = {RX_DSA2_SIZE{1'b1}};
  reg [RX_DSA3_A_SIZE -1:0] rx1_dsa_3_a_reg = {RX_DSA3_A_SIZE{1'b1}};
  reg [RX_DSA3_B_SIZE -1:0] rx1_dsa_3_b_reg = {RX_DSA3_B_SIZE{1'b1}};

  //---------------------------------------------------------------
  // ATR memory signals
  //---------------------------------------------------------------
  reg         ram_tx0_wea = 1'b0;
  wire [31:0] ram_tx0_doa;
  wire [31:0] ram_tx0_dob;
  reg         ram_tx1_wea = 1'b0;
  wire [31:0] ram_tx1_doa;
  wire [31:0] ram_tx1_dob;
  reg         ram_rx0_wea = 1'b0;
  wire [31:0] ram_rx0_doa;
  wire [31:0] ram_rx0_dob;
  reg         ram_rx1_wea = 1'b0;
  wire [31:0] ram_rx1_doa;
  wire [31:0] ram_rx1_dob;

  reg         table_tx0_wea = 1'b0;
  wire [31:0] table_tx0_doa;
  reg         table_tx1_wea = 1'b0;
  wire [31:0] table_tx1_doa;
  reg         table_rx0_wea = 1'b0;
  wire [31:0] table_rx0_doa;
  reg         table_rx1_wea = 1'b0;
  wire [31:0] table_rx1_doa;

  //---------------------------------------------------------------
  // Handling of CtrlPort
  //---------------------------------------------------------------
  // Check of request address is targeted for this module.
  wire address_in_range = (s_ctrlport_req_addr >= BASE_ADDRESS) && (s_ctrlport_req_addr < BASE_ADDRESS + SIZE_ADDRESS);
  // Read request shift register to align memory read and response generation.
  reg  [ 1:0] read_req_shift_reg = 2'b0;
  // Write request shift register to align gain table memory read and ATR memory
  // write operation.
  reg  [ 1:0] write_req_shift_reg = 2'b0;
  // Mask out 8 bits for ATR configurations to be able to compare all ATR
  // configurations against the same base register address.
  wire [31:0] register_base_address = {s_ctrlport_req_addr[19:10], 8'b0, s_ctrlport_req_addr[1:0]};
  // Extract masked out bits from the address, which represent the register
  // array index = ATR configuration index
  wire [ 7:0] register_index = s_ctrlport_req_addr[9:2];
  // switch between CtrlPort data and gain table data for ATR memories
  reg select_gain_table = 1'b0;

  always @(posedge ctrlport_clk) begin
    // reset internal registers and responses
    if (ctrlport_rst) begin
      s_ctrlport_resp_ack <= 1'b0;

      read_req_shift_reg  <= 2'b0;
      write_req_shift_reg <= 2'b0;

      ram_tx0_wea <= 1'b0;
      ram_tx1_wea <= 1'b0;
      ram_rx0_wea <= 1'b0;
      ram_rx1_wea <= 1'b0;

      table_tx0_wea <= 1'b0;
      table_tx1_wea <= 1'b0;
      table_rx0_wea <= 1'b0;
      table_rx1_wea <= 1'b0;

      select_gain_table <= 1'b0;

    end else begin
      // default assignments
      read_req_shift_reg  <= { read_req_shift_reg[0], s_ctrlport_req_rd};
      write_req_shift_reg <= {write_req_shift_reg[0], s_ctrlport_req_wr};

      ram_tx0_wea <= 1'b0;
      ram_tx1_wea <= 1'b0;
      ram_rx0_wea <= 1'b0;
      ram_rx1_wea <= 1'b0;

      table_tx0_wea <= 1'b0;
      table_tx1_wea <= 1'b0;
      table_rx0_wea <= 1'b0;
      table_rx1_wea <= 1'b0;

      select_gain_table <= 1'b0;

      // Answer write requests delayed by 2 clock cycles. This compensated for
      // register ram_addr and the memory internal address register to make sure
      // gain table output data is up to date when forwarding data to ATR memory
      if (write_req_shift_reg[1]) begin
        // always issue an ack and no data
        s_ctrlport_resp_ack     <= 1'b1;
        s_ctrlport_resp_data    <= {32{1'bx}};
        s_ctrlport_resp_status  <= CTRL_STS_OKAY;

        case (register_base_address)
          BASE_ADDRESS + TX0_DSA_ATR(0): begin
            ram_tx0_wea <= 1'b1;
          end
          BASE_ADDRESS + TX1_DSA_ATR(0): begin
            ram_tx1_wea <= 1'b1;
          end
          BASE_ADDRESS + RX0_DSA_ATR(0): begin
            ram_rx0_wea <= 1'b1;
          end
          BASE_ADDRESS + RX1_DSA_ATR(0): begin
            ram_rx1_wea <= 1'b1;
          end

          BASE_ADDRESS + TX0_DSA_TABLE(0): begin
            table_tx0_wea <= 1'b1;
          end
          BASE_ADDRESS + TX1_DSA_TABLE(0): begin
            table_tx1_wea <= 1'b1;
          end
          BASE_ADDRESS + RX0_DSA_TABLE(0): begin
            table_rx0_wea <= 1'b1;
          end
          BASE_ADDRESS + RX1_DSA_TABLE(0): begin
            table_rx1_wea <= 1'b1;
          end

          BASE_ADDRESS + TX0_DSA_TABLE_SELECT(0): begin
            ram_tx0_wea       <= 1'b1;
            select_gain_table <= 1'b1;
          end
          BASE_ADDRESS + TX1_DSA_TABLE_SELECT(0): begin
            ram_tx1_wea       <= 1'b1;
            select_gain_table <= 1'b1;
          end
          BASE_ADDRESS + RX0_DSA_TABLE_SELECT(0): begin
            ram_rx0_wea       <= 1'b1;
            select_gain_table <= 1'b1;
          end
          BASE_ADDRESS + RX1_DSA_TABLE_SELECT(0): begin
            ram_rx1_wea       <= 1'b1;
            select_gain_table <= 1'b1;
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
          BASE_ADDRESS + TX0_DSA_ATR(0): begin
            s_ctrlport_resp_data <= ram_tx0_doa & TX_DSA_CONTROL_MASK;
          end
          BASE_ADDRESS + TX1_DSA_ATR(0): begin
            s_ctrlport_resp_data <= ram_tx1_doa & TX_DSA_CONTROL_MASK;
          end
          BASE_ADDRESS + RX0_DSA_ATR(0): begin
            s_ctrlport_resp_data <= ram_rx0_doa & RX_DSA_CONTROL_MASK;
          end
          BASE_ADDRESS + RX1_DSA_ATR(0): begin
            s_ctrlport_resp_data <= ram_rx1_doa & RX_DSA_CONTROL_MASK;
          end
          BASE_ADDRESS + TX0_DSA_TABLE(0): begin
            s_ctrlport_resp_data <= table_tx0_doa & TX_DSA_CONTROL_MASK;
          end
          BASE_ADDRESS + TX1_DSA_TABLE(0): begin
            s_ctrlport_resp_data <= table_tx1_doa & TX_DSA_CONTROL_MASK;
          end
          BASE_ADDRESS + RX0_DSA_TABLE(0): begin
            s_ctrlport_resp_data <= table_rx0_doa & RX_DSA_CONTROL_MASK;
          end
          BASE_ADDRESS + RX1_DSA_TABLE(0): begin
            s_ctrlport_resp_data <= table_rx1_doa & RX_DSA_CONTROL_MASK;
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
  reg [ 7:0] ram_addr        =  8'b0;
  reg [ 7:0] gain_table_addr =  8'b0;
  reg [31:0] ram_datain      = 32'b0;
  always @(posedge ctrlport_clk) begin
    // Capture CtrlPort data and address on requests as only in this clock cycle
    // the data is valid.
    if (s_ctrlport_req_wr || s_ctrlport_req_rd) begin
      ram_addr        <= register_index;
      ram_datain      <= s_ctrlport_req_data;

      case (register_base_address)
        BASE_ADDRESS + TX0_DSA_TABLE_SELECT(0),
        BASE_ADDRESS + TX1_DSA_TABLE_SELECT(0),
        BASE_ADDRESS + RX0_DSA_TABLE_SELECT(0),
        BASE_ADDRESS + RX1_DSA_TABLE_SELECT(0): begin
          gain_table_addr <= s_ctrlport_req_data[TABLE_INDEX_MSB:TABLE_INDEX];
        end
        default: begin
          gain_table_addr <= register_index;
        end
      endcase
    end

    // outputs
    tx0_dsa_1_reg   <= ram_tx0_dob[  TX_DSA1_MSB :   TX_DSA1];
    tx0_dsa_2_reg   <= ram_tx0_dob[  TX_DSA2_MSB :   TX_DSA2];

    tx1_dsa_1_reg   <= ram_tx1_dob[  TX_DSA1_MSB :   TX_DSA1];
    tx1_dsa_2_reg   <= ram_tx1_dob[  TX_DSA2_MSB :   TX_DSA2];

    rx0_dsa_1_reg   <= ram_rx0_dob[  RX_DSA1_MSB :   RX_DSA1];
    rx0_dsa_2_reg   <= ram_rx0_dob[  RX_DSA2_MSB :   RX_DSA2];
    rx0_dsa_3_a_reg <= ram_rx0_dob[RX_DSA3_A_MSB : RX_DSA3_A];
    rx0_dsa_3_b_reg <= ram_rx0_dob[RX_DSA3_B_MSB : RX_DSA3_B];

    rx1_dsa_1_reg   <= ram_rx1_dob[  RX_DSA1_MSB :   RX_DSA1];
    rx1_dsa_2_reg   <= ram_rx1_dob[  RX_DSA2_MSB :   RX_DSA2];
    rx1_dsa_3_a_reg <= ram_rx1_dob[RX_DSA3_A_MSB : RX_DSA3_A];
    rx1_dsa_3_b_reg <= ram_rx1_dob[RX_DSA3_B_MSB : RX_DSA3_B];
  end

  assign tx0_dsa1[6:2] = tx0_dsa_1_reg;
  assign tx0_dsa2[6:2] = tx0_dsa_2_reg;

  assign tx1_dsa1[6:2] = tx1_dsa_1_reg;
  assign tx1_dsa2[6:2] = tx1_dsa_2_reg;

  //Rx DSAs behave differently from Tx DSAs
  //Flip MSB/LSB, and invert

  genvar vi;
  // take care of inverting the active low logic and bit-reversing
  // the DSA controls for RX paths.
  generate
    for (vi=1; vi<=4; vi=vi+1) begin : reverselogic
      //              [1:4]                     [3:0]
      assign rx0_dsa1_n[vi]    = !rx0_dsa_1_reg[4-vi];
      assign rx0_dsa2_n[vi]    = !rx0_dsa_2_reg[4-vi];
      assign rx0_dsa3_a_n[vi]  = !rx0_dsa_3_a_reg[4-vi];
      assign rx0_dsa3_b_n[vi]  = !rx0_dsa_3_b_reg[4-vi];

      assign rx1_dsa1_n[vi]    = !rx1_dsa_1_reg[4-vi];
      assign rx1_dsa2_n[vi]    = !rx1_dsa_2_reg[4-vi];
      assign rx1_dsa3_a_n[vi]  = !rx1_dsa_3_a_reg[4-vi];
      assign rx1_dsa3_b_n[vi]  = !rx1_dsa_3_b_reg[4-vi];
    end
  endgenerate

  //---------------------------------------------------------------
  // ATR memories
  //---------------------------------------------------------------
  // Choose data source for ATR configurations from CtrlPort or gain table.
  wire [31:0] ram_rx0_dia = select_gain_table ? table_rx0_doa : ram_datain;
  wire [31:0] ram_rx1_dia = select_gain_table ? table_rx1_doa : ram_datain;
  wire [31:0] ram_tx0_dia = select_gain_table ? table_tx0_doa : ram_datain;
  wire [31:0] ram_tx1_dia = select_gain_table ? table_tx1_doa : ram_datain;

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
    .INIT_FILE  ("register_endpoints/memory_init_files/tx_dsa_defaults.hex")
  ) ram_tx0_i (
    .clka   (ctrlport_clk),
    .ena    (1'b1),
    .wea    (ram_tx0_wea),
    .addra  (ram_addr),
    .dia    (ram_tx0_dia),
    .doa    (ram_tx0_doa),
    .clkb   (ctrlport_clk),
    .enb    (1'b1),
    .web    (1'b0),
    .addrb  (atr_config_rf0),
    .dib    (0),
    .dob    (ram_tx0_dob)
  );

  ram_2port #(
    .DWIDTH     (32),
    .AWIDTH     (8),
    .RW_MODE    (RAM_RW_MODE),
    .RAM_TYPE   ("AUTOMATIC"),
    .OUT_REG    (0),
    .INIT_FILE  ("register_endpoints/memory_init_files/tx_dsa_defaults.hex")
  ) ram_tx1_i (
    .clka   (ctrlport_clk),
    .ena    (1'b1),
    .wea    (ram_tx1_wea),
    .addra  (ram_addr),
    .dia    (ram_tx1_dia),
    .doa    (ram_tx1_doa),
    .clkb   (ctrlport_clk),
    .enb    (1'b1),
    .web    (1'b0),
    .addrb  (atr_config_rf1),
    .dib    (0),
    .dob    (ram_tx1_dob)
  );

  ram_2port #(
    .DWIDTH     (32),
    .AWIDTH     (8),
    .RW_MODE    (RAM_RW_MODE),
    .RAM_TYPE   ("AUTOMATIC"),
    .OUT_REG    (0),
    .INIT_FILE  ("register_endpoints/memory_init_files/rx_dsa_defaults.hex")
  ) ram_rx0_i (
    .clka   (ctrlport_clk),
    .ena    (1'b1),
    .wea    (ram_rx0_wea),
    .addra  (ram_addr),
    .dia    (ram_rx0_dia),
    .doa    (ram_rx0_doa),
    .clkb   (ctrlport_clk),
    .enb    (1'b1),
    .web    (1'b0),
    .addrb  (atr_config_rf0),
    .dib    (0),
    .dob    (ram_rx0_dob)
  );

  ram_2port #(
    .DWIDTH     (32),
    .AWIDTH     (8),
    .RW_MODE    (RAM_RW_MODE),
    .RAM_TYPE   ("AUTOMATIC"),
    .OUT_REG    (0),
    .INIT_FILE  ("register_endpoints/memory_init_files/rx_dsa_defaults.hex")
  ) ram_rx1_i (
    .clka   (ctrlport_clk),
    .ena    (1'b1),
    .wea    (ram_rx1_wea),
    .addra  (ram_addr),
    .dia    (ram_rx1_dia),
    .doa    (ram_rx1_doa),
    .clkb   (ctrlport_clk),
    .enb    (1'b1),
    .web    (1'b0),
    .addrb  (atr_config_rf1),
    .dib    (0),
    .dob    (ram_rx1_dob)
  );

  //---------------------------------------------------------------
  // Gain tables
  //---------------------------------------------------------------

  ram_2port #(
    .DWIDTH     (32),
    .AWIDTH     (8),
    .RW_MODE    (RAM_RW_MODE),
    .RAM_TYPE   ("AUTOMATIC"),
    .OUT_REG    (0),
    .INIT_FILE  ("register_endpoints/memory_init_files/tx_dsa_defaults.hex")
  ) table_tx0_i (
    .clka   (ctrlport_clk),
    .ena    (1'b1),
    .wea    (table_tx0_wea),
    .addra  (gain_table_addr),
    .dia    (ram_datain),
    .doa    (table_tx0_doa),
    .clkb   (ctrlport_clk),
    .enb    (1'b1),
    .web    (1'b0),
    .addrb  (8'b0),
    .dib    (32'b0),
    .dob    ()
  );

  ram_2port #(
    .DWIDTH     (32),
    .AWIDTH     (8),
    .RW_MODE    (RAM_RW_MODE),
    .RAM_TYPE   ("AUTOMATIC"),
    .OUT_REG    (0),
    .INIT_FILE  ("register_endpoints/memory_init_files/tx_dsa_defaults.hex")
  ) table_tx1_i (
    .clka   (ctrlport_clk),
    .ena    (1'b1),
    .wea    (table_tx1_wea),
    .addra  (gain_table_addr),
    .dia    (ram_datain),
    .doa    (table_tx1_doa),
    .clkb   (ctrlport_clk),
    .enb    (1'b1),
    .web    (1'b0),
    .addrb  (8'b0),
    .dib    (32'b0),
    .dob    ()
  );

  ram_2port #(
    .DWIDTH     (32),
    .AWIDTH     (8),
    .RW_MODE    (RAM_RW_MODE),
    .RAM_TYPE   ("AUTOMATIC"),
    .OUT_REG    (0),
    .INIT_FILE  ("register_endpoints/memory_init_files/rx_dsa_defaults.hex")
  ) table_rx0_i (
    .clka   (ctrlport_clk),
    .ena    (1'b1),
    .wea    (table_rx0_wea),
    .addra  (gain_table_addr),
    .dia    (ram_datain),
    .doa    (table_rx0_doa),
    .clkb   (ctrlport_clk),
    .enb    (1'b1),
    .web    (1'b0),
    .addrb  (8'b0),
    .dib    (32'b0),
    .dob    ()
  );

  ram_2port #(
    .DWIDTH     (32),
    .AWIDTH     (8),
    .RW_MODE    (RAM_RW_MODE),
    .RAM_TYPE   ("AUTOMATIC"),
    .OUT_REG    (0),
    .INIT_FILE  ("register_endpoints/memory_init_files/rx_dsa_defaults.hex")
  ) table_rx1_i (
    .clka   (ctrlport_clk),
    .ena    (1'b1),
    .wea    (table_rx1_wea),
    .addra  (gain_table_addr),
    .dia    (ram_datain),
    .doa    (table_rx1_doa),
    .clkb   (ctrlport_clk),
    .enb    (1'b1),
    .web    (1'b0),
    .addrb  (8'b0),
    .dib    (32'b0),
    .dob    ()
  );

endmodule

`default_nettype wire

//XmlParse xml_on
//<regmap name="DSA_SETUP_REGMAP" readablestrobes="false" generatevhdl="true" ettusguidelines="true" markdown="true">
//  <group name="DSA_SETUP_REGISTERS">
//    <info>
//       The following registers control the digital step attenuators (DSA).
//
//       There are two ways to set the DSA values, which are applied to the DB ICs.
//
//       1. The ...DSA_ATR registers can be used to access the raw
//       values of each ATR configuration.
//
//       2. Gain tables can be used as intermediate step to abstract from the
//       raw DB values. This gain table can be modified using the ...DSA_TABLE
//       registers according to the content of the registers from the first
//       option.  Initially each gain table is empty (all zeros). Each gain
//       table entry can be accessed at any time. Once the table is filled with
//       values the ...DSA_TABLE_SELECT registers can be used to get one gain
//       table entry with index TABLE_INDEX and write it to the appropriate ATR
//       configuration given by the address (see _show extended info_ link below
//       the register array headlines)
//    </info>
//    <regtype name="TX_DSA_CONTROL" size="32" attributes="Readable|Writable">
//      <bitfield name="TX_DSA1" range="4..0" initialvalue="31">
//        <info>
//          Sets the attenuation level for Tx DSA1. The resolution attenuation is 1 dB, with an attenuation range from 1 to 31 dB. Write this field with the
//          attenuation setting desired. Writing zero to this field results in no attenuation (different insertion loss expected for different frequency ranges).
//        </info>
//      </bitfield>
//      <bitfield name="TX_DSA2" range="12..8" initialvalue="31">
//        <info>
//          Sets the attenuation level for Tx DSA2. The resolution attenuation is 1 dB, with an attenuation range from 1 to 31 dB. Write this field with the
//          attenuation setting desired. Writing zero to this field results in no attenuation (different insertion loss expected for different frequency ranges).
//        </info>
//      </bitfield>
//    </regtype>
//
//    <regtype name="RX_DSA_CONTROL" size="32" attributes="Readable|Writable">
//      <bitfield name="RX_DSA1" range="3..0" initialvalue="15">
//        <info>
//          Sets the attenuation level for Rx DSA1. The resolution attenuation is 1 dB, with an attenuation range from 1 to 15 dB. Write this field with the
//          attenuation setting desired. Writing zero to this field results in no attenuation (different insertion loss expected for different frequency ranges).
//        </info>
//      </bitfield>
//      <bitfield name="RX_DSA2" range="7..4" initialvalue="15">
//        <info>
//          Sets the attenuation level for Rx DSA2. The resolution attenuation is 1 dB, with an attenuation range from 1 to 15 dB. Write this field with the
//          attenuation setting desired. Writing zero to this field results in no attenuation (different insertion loss expected for different frequency ranges).
//        </info>
//      </bitfield>
//      <bitfield name="RX_DSA3_A" range="11..8" initialvalue="15">
//        <info>
//          Sets the attenuation level for Rx DSA 3a and 3b. The resolution attenuation is 1 dB, with an attenuation range from 1 to 15 dB. Write this field with the
//          attenuation setting desired. Writing zero to this field results in no attenuation (different insertion loss expected for different frequency ranges).
//        </info>
//      </bitfield>
//      <bitfield name="RX_DSA3_B" range="15..12" initialvalue="15">
//        <info>
//          Sets the attenuation level for Rx DSA 3b(to input of IF1 Amplifier 2). The resolution attenuation is 1 dB, with an attenuation range from 1 to 15 dB. Write this field with the
//          attenuation setting desired. Writing zero to this field results in no attenuation (different insertion loss expected for different frequency ranges).. {BR/}
//        </info>
//      </bitfield>
//    </regtype>
//
//    <regtype name="DSA_TABLE_CONTROL" size="32" attributes="Writable">
//      <bitfield name="TABLE_INDEX" range="7..0">
//        <info>
//          Gain table index to be used for getting the raw attenuation values.
//        </info>
//      </bitfield>
//    </regtype>
//
//    <register name="TX0_DSA_ATR" offset="0x000" count="256" step="4" typename="TX_DSA_CONTROL">
//      <info>
//        Controls the Tx0 DSAs by accessing the raw attenuation levels.
//
//        This register array can hold settings for all ATR configurations.
//        The register index equals the ATR configuration.
//        The active configuration can be selected in @.ATR_REGMAP.
//        Independently all configurations can be read/written at any time.
//      </info>
//    </register>
//    <register name="TX1_DSA_ATR" offset="0x400" count="256" step="4" typename="TX_DSA_CONTROL">
//      <info>
//        Controls the Tx1 DSAs by accessing the raw attenuation levels.
//
//        This register array can hold settings for all ATR configurations.
//        The register index equals the ATR configuration.
//        The active configuration can be selected in @.ATR_REGMAP.
//        Independently all configurations can be read/written at any time.
//      </info>
//    </register>
//
//    <register name="RX0_DSA_ATR" offset="0x800" count="256" step="4" typename="RX_DSA_CONTROL">
//      <info>
//        Controls the Rx0 DSAs by accessing the raw attenuation levels.
//
//        This register array can hold settings for all ATR configurations.
//        The register index equals the ATR configuration.
//        The active configuration can be selected in @.ATR_REGMAP.
//        Independently all configurations can be read/written at any time.
//      </info>
//    </register>
//    <register name="RX1_DSA_ATR" offset="0xC00" count="256" step="4" typename="RX_DSA_CONTROL">
//      <info>
//        Controls the Rx1 DSAs by accessing the raw attenuation levels.
//
//        This register array can hold settings for all ATR configurations.
//        The register index equals the ATR configuration.
//        The active configuration can be selected in @.ATR_REGMAP.
//        Independently all configurations can be read/written at any time.
//      </info>
//    </register>
//
//    <register name="TX0_DSA_TABLE_SELECT" offset="0x1000" count="256" step="4" typename="DSA_TABLE_CONTROL">
//      <info>
//        Controls the Tx0 DSAs by using the gain table to translate the table
//        index to raw attenuation levels. The register offset (i) is targeting
//        an ATR configuration to store the values from the gain table.
//      </info>
//    </register>
//    <register name="TX1_DSA_TABLE_SELECT" offset="0x1400" count="256" step="4" typename="DSA_TABLE_CONTROL">
//      <info>
//        Controls the Tx1 DSAs by using the gain table to translate the table
//        index to raw attenuation levels. The register offset (i) is targeting
//        an ATR configuration to store the values from the gain table.
//      </info>
//    </register>
//    <register name="RX0_DSA_TABLE_SELECT" offset="0x1800" count="256" step="4" typename="DSA_TABLE_CONTROL">
//      <info>
//        Controls the Rx0 DSAs by using the gain table to translate the table
//        index to raw attenuation levels. The register offset (i) is targeting
//        an ATR configuration to store the values from the gain table.
//      </info>
//    </register>
//    <register name="RX1_DSA_TABLE_SELECT" offset="0x1C00" count="256" step="4" typename="DSA_TABLE_CONTROL">
//      <info>
//        Controls the Rx1 DSAs by using the gain table to translate the table
//        index to raw attenuation levels. The register offset (i) is targeting
//        an ATR configuration to store the values from the gain table.
//      </info>
//    </register>
//
//    <register name="TX0_DSA_TABLE" offset="0x2000" count="256" step="4" typename="TX_DSA_CONTROL">
//      <info>
//        Provides access to the gain table for Tx0.
//
//        Each entry i will be saved in the gain table without any implications
//        on HW. Enables SW to use the table index in @.TX0_DSA_TABLE_SELECT to
//        modify the ATR configurations.
//      </info>
//    </register>
//    <register name="TX1_DSA_TABLE" offset="0x2400" count="256" step="4" typename="TX_DSA_CONTROL">
//      <info>
//        Provides access to the gain table for Tx1.
//
//        Each entry i will be saved in the gain table without any implications
//        on HW. Enables SW to use the table index in @.TX1_DSA_TABLE_SELECT to
//        modify the ATR configurations.
//      </info>
//    </register>
//    <register name="RX0_DSA_TABLE" offset="0x2800" count="256" step="4" typename="RX_DSA_CONTROL">
//      <info>
//        Provides access to the gain table for Rx0.
//
//        Each entry i will be saved in the gain table without any implications
//        on HW. Enables SW to use the table index in @.RX0_DSA_TABLE_SELECT to
//        modify the ATR configurations.
//      </info>
//    </register>
//    <register name="RX1_DSA_TABLE" offset="0x2C00" count="256" step="4" typename="RX_DSA_CONTROL">
//      <info>
//        Provides access to the gain table for Rx1.
//
//        Each entry i will be saved in the gain table without any implications
//        on HW. Enables SW to use the table index in @.RX1_DSA_TABLE_SELECT to
//        modify the ATR configurations.
//      </info>
//    </register>
//  </group>
//</regmap>
//XmlParse xml_off

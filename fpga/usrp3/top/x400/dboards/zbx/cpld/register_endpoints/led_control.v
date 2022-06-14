//
// Copyright 2021 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: led_control
//
// Description:
//  Implements control over LED state via CtrlPort. The default state
//  has the LEDs disabled. Uses RAM to store multiple ATR configurations.
//

`default_nettype none

module led_control #(
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

  // LED Control (domain: ctrlport_clk)
  output reg         ch0_rx2_led,
  output reg         ch0_tx_led,
  output reg         ch0_rx_led,
  output reg         ch1_rx2_led,
  output reg         ch1_tx_led,
  output reg         ch1_rx_led,

  // ATR switching
  input  wire [ 7:0] atr_config_rf0,
  input  wire [ 7:0] atr_config_rf1
);

  `include "../regmap/led_setup_regmap_utils.vh"
  `include "../../../../../../lib/rfnoc/core/ctrlport.vh"

  //---------------------------------------------------------------
  // ATR memory signals
  //---------------------------------------------------------------
  reg         ram_ch0_wea;
  wire [31:0] ram_ch0_doa;
  wire [31:0] ram_ch0_dob;

  reg         ram_ch1_wea;
  wire [31:0] ram_ch1_dob;

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

      ram_ch0_wea <= 1'b0;
      ram_ch1_wea <= 1'b0;

    end else begin
      // default assignments
      read_req_shift_reg <= {read_req_shift_reg[0], s_ctrlport_req_rd};

      ram_ch0_wea <= 1'b0;
      ram_ch1_wea <= 1'b0;

      // write requests
      if (s_ctrlport_req_wr) begin
        // always issue an ack and no data
        s_ctrlport_resp_ack     <= 1'b1;
        s_ctrlport_resp_data    <= {32{1'bx}};
        s_ctrlport_resp_status  <= CTRL_STS_OKAY;

        case (register_base_address)
          BASE_ADDRESS + LED_CONTROL(0): begin
            ram_ch0_wea <= 1'b1;
            ram_ch1_wea <= 1'b1;
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
          BASE_ADDRESS + LED_CONTROL(0): begin
            s_ctrlport_resp_data <= ram_ch0_doa & LED_CONTROL_TYPE_MASK;
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

    //outputs
    ch0_rx2_led <= ram_ch0_dob[CH0_RX2_LED_EN];
    ch0_tx_led  <= ram_ch0_dob[CH0_TRX1_LED_EN + 1];
    ch0_rx_led  <= ram_ch0_dob[CH0_TRX1_LED_EN + 0];

    ch1_rx2_led <= ram_ch1_dob[CH1_RX2_LED_EN];
    ch1_tx_led  <= ram_ch1_dob[CH1_TRX1_LED_EN + 1];
    ch1_rx_led  <= ram_ch1_dob[CH1_TRX1_LED_EN + 0];
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
    .INIT_FILE  ("")
  ) ram_ch0_i (
    .clka   (ctrlport_clk),
    .ena    (1'b1),
    .wea    (ram_ch0_wea),
    .addra  (ram_addr),
    .dia    (ram_datain),
    .doa    (ram_ch0_doa),
    .clkb   (ctrlport_clk),
    .enb    (1'b1),
    .web    (1'b0),
    .addrb  (atr_config_rf0),
    .dib    (0),
    .dob    (ram_ch0_dob)
  );

  ram_2port #(
    .DWIDTH     (32),
    .AWIDTH     (8),
    .RW_MODE    (RAM_RW_MODE),
    .RAM_TYPE   ("AUTOMATIC"),
    .OUT_REG    (0),
    .INIT_FILE  ("")
  ) ram_ch1_i (
    .clka   (ctrlport_clk),
    .ena    (1'b1),
    .wea    (ram_ch1_wea),
    .addra  (ram_addr),
    .dia    (ram_datain),
    .doa    (),
    .clkb   (ctrlport_clk),
    .enb    (1'b1),
    .web    (1'b0),
    .addrb  (atr_config_rf1),
    .dib    (0),
    .dob    (ram_ch1_dob));

endmodule

`default_nettype wire

//XmlParse xml_on
//<regmap name="LED_SETUP_REGMAP" readablestrobes="false" generatevhdl="true" ettusguidelines="true">
//  <group name="LED_SETUP_REGISTERS">
//    <info>
//      Contains registers that control the LEDs.
//    </info>
//    <regtype name="LED_CONTROL_TYPE" size="32" attributes="Readable|Writable">
//      <info>
//        Defines LED functionality.
//      </info>
//      <bitfield name="CH0_RX2_LED_EN" range="0" initialvalue="0">
//        <info>
//          Enables the Ch0 Rx2 Green LED
//        </info>
//      </bitfield>
//      <bitfield name="CH0_TRX1_LED_EN" range="2..1" initialvalue="0">
//        <info>
//          This bitfield controls the RG LED{BR/}
//          Bit 6 controls the Ch0 Rx Green LED{BR/}
//          Bit 7 controls the Ch0 Tx Red LED{BR/}
//        </info>
//      </bitfield>
//      <bitfield name="CH1_RX2_LED_EN" range="16" initialvalue="0">
//        <info>
//          Enables the Ch1 Rx2 Green LED
//        </info>
//      </bitfield>
//      <bitfield name="CH1_TRX1_LED_EN" range="18..17" initialvalue="0">
//        <info>
//          This bitfield controls the RG LED{BR/}
//          Bit 15 controls the Ch1 Rx Green LED{BR/}
//          Bit 14 controls the Ch1 Tx Red LED{BR/}
//        </info>
//      </bitfield>
//    </regtype>
//
//    <register name="LED_CONTROL" offset="0x0" count="256" step="4" typename="LED_CONTROL_TYPE">
//      <info>
//        This register array can hold settings for all ATR configurations.
//        The register index equals the ATR configuration.
//        The active configuration can be selected in @.ATR_REGMAP.
//        Independently all configurations can be read/written at any time.
//      </info>
//    </register>
//  </group>
//</regmap>
//XmlParse xml_off

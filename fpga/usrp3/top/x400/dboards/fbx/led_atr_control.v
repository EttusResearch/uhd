//
// Copyright 2022 Ettus Research, A National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: led_atr_control
//
// Description:
//  Translates db_status to implement control over RF LEDs via CtrlPort.
//  Uses RAM to store multiple ATR configurations. Triggers CtrlPort
//  requests to transfer changes to LEDs to MB CPLD.
//  There are three supported control schemes for these switches:
//    - ATR Disabled - Single persistent state.
//    - Classic ATR  - Each channel's LEDs depend on the transmission state
//                     of the respective channel.
//    - DB State     - Each channel's LEDs depend on the transmission state
//                     of all channels in this radio.
//
// Parameters:
//
//   LED_REGISTER_ADDRESS : Address of LED register within CPLD.
//   REG_BASE : Base address to use for registers.
//   REG_SIZE : Register space size.
//

`default_nettype none

module led_atr_control #(
  parameter LED_REGISTER_ADDRESS = 0,
  parameter REG_BASE = 'h2000,
  parameter REG_SIZE = 'h2000

) (
  // Common ControlPort signals
  input wire ctrlport_clk,
  input wire ctrlport_rst,

  // Slave ctrlport inteledace
  input  wire        s_ctrlport_req_wr,
  input  wire        s_ctrlport_req_rd,
  input  wire [19:0] s_ctrlport_req_addr,
  input  wire [31:0] s_ctrlport_req_data,
  output reg         s_ctrlport_resp_ack    = 1'b0,
  output reg  [ 1:0] s_ctrlport_resp_status = 2'b00,
  output reg  [31:0] s_ctrlport_resp_data   = {32 {1'b0}},

  // DB state lines
  input wire [7:0] db_state,

  // ControlPort request
  output reg         m_ctrlport_req_wr,
  output wire        m_ctrlport_req_rd,
  output wire [19:0] m_ctrlport_req_addr,
  output wire [31:0] m_ctrlport_req_data,
  output wire [ 3:0] m_ctrlport_req_byte_en,

  // ControlPort response
  input wire        m_ctrlport_resp_ack,
  input wire [ 1:0] m_ctrlport_resp_status,
  input wire [31:0] m_ctrlport_resp_data

);

  `include "../../cpld/regmap/x440/led_setup_regmap_utils.vh"
  `include "../../../../lib/rfnoc/core/ctrlport.vh"
  `include "regmap/led_atr_regmap_utils.vh"


  //---------------------------------------------------------------
  // ATR memory signals
  //---------------------------------------------------------------
  reg                 ram_led0_wea;
  wire [LED_SIZE-1:0] ram_led0_doa;
  wire [LED_SIZE-1:0] ram_led0_dob;

  reg                 ram_led1_wea;
  wire [LED_SIZE-1:0] ram_led1_doa;
  wire [LED_SIZE-1:0] ram_led1_dob;

  reg                 ram_led2_wea;
  wire [LED_SIZE-1:0] ram_led2_doa;
  wire [LED_SIZE-1:0] ram_led2_dob;

  reg                 ram_led3_wea;
  wire [LED_SIZE-1:0] ram_led3_doa;
  wire [LED_SIZE-1:0] ram_led3_dob;


  //---------------------------------------------------------------
  // ATR Scheme signals
  //---------------------------------------------------------------
  reg [3:0]  atr_disable  = 4'b0;

  // DB state/Classic ATR selector
  reg [3:0]  atr_mode = 4'b0;

  //---------------------------------------------------------------------------
  // Control inteledace handling
  //---------------------------------------------------------------------------

  // Check that address is within this module's range.
  wire address_in_range = (s_ctrlport_req_addr >= REG_BASE) && (s_ctrlport_req_addr < REG_BASE + REG_SIZE);

  // Check that address is targeting an ATR state.
  wire address_is_atr = (s_ctrlport_req_addr >= REG_BASE + LED0_ATR_STATE(0)) && (s_ctrlport_req_addr <= REG_BASE + LED3_ATR_STATE(LED3_ATR_STATE_COUNT-1));

  // Read request shift register to align memory read and response generation.
  reg  [ 1:0] read_req_shift_reg = 2'b0;
  // Mask out 8 bits for ATR configurations to be able to compare all ATR
  // configurations against the same base register address.
  wire [31:0] register_base_address = {s_ctrlport_req_addr[19:10], 8'b0, s_ctrlport_req_addr[1:0]};
  // Decode the ATR state being addressed.
  wire [ 7:0] atr_address = s_ctrlport_req_addr[9:2];

  always @ (posedge ctrlport_clk) begin
    if (ctrlport_rst) begin
      s_ctrlport_resp_ack    <= 1'b0;
      s_ctrlport_resp_data   <= 32'b0;
      s_ctrlport_resp_status <= 2'b00;

      atr_disable         <= 4'b0;
      atr_mode            <= 4'b0;

      ram_led0_wea <= 1'b0;
      ram_led1_wea <= 1'b0;
      ram_led2_wea <= 1'b0;
      ram_led3_wea <= 1'b0;

    end else begin
      // default assignments
      read_req_shift_reg <= {read_req_shift_reg[0], s_ctrlport_req_rd};

      ram_led0_wea <= 1'b0;
      ram_led1_wea <= 1'b0;
      ram_led2_wea <= 1'b0;
      ram_led3_wea <= 1'b0;

      // Write registers
      if (s_ctrlport_req_wr) begin
        // Acknowledge by default
        s_ctrlport_resp_ack    <= 1'b1;
        s_ctrlport_resp_status <= CTRL_STS_OKAY;

        // Address ATR state writes
        if(address_is_atr) begin

          case (register_base_address)
            REG_BASE + LED0_ATR_STATE(0): begin
              ram_led0_wea <= 1'b1;
            end

            REG_BASE + LED1_ATR_STATE(0): begin
              ram_led1_wea <= 1'b1;
            end

            REG_BASE + LED2_ATR_STATE(0): begin
              ram_led2_wea <= 1'b1;
            end

            REG_BASE + LED3_ATR_STATE(0): begin
              ram_led3_wea <= 1'b1;
            end

            // error on undefined address
            default: begin
              s_ctrlport_resp_status <= CTRL_STS_CMDERR;
            end
          endcase
        end else begin

          // Address writes to the rest of the register space
          case (s_ctrlport_req_addr)

            REG_BASE + LED_ATR_OPTION_REGISTER: begin
              atr_mode[0] <= s_ctrlport_req_data[LED0_ATR_OPTION];
              atr_mode[1] <= s_ctrlport_req_data[LED1_ATR_OPTION];
              atr_mode[2] <= s_ctrlport_req_data[LED2_ATR_OPTION];
              atr_mode[3] <= s_ctrlport_req_data[LED3_ATR_OPTION];
            end

            REG_BASE + LED_ATR_DISABLED: begin
              atr_disable[0] <= s_ctrlport_req_data[LED0_ATR_DISABLED];
              atr_disable[1] <= s_ctrlport_req_data[LED1_ATR_DISABLED];
              atr_disable[2] <= s_ctrlport_req_data[LED2_ATR_DISABLED];
              atr_disable[3] <= s_ctrlport_req_data[LED3_ATR_DISABLED];
            end

            // No register implementation for provided address
            default: begin
              // Acknowledge and provide error status if address is in range
              if (address_in_range) begin
                s_ctrlport_resp_status <= CTRL_STS_CMDERR;

              // No response if out of range
              end else begin
                s_ctrlport_resp_ack <= 1'b0;
              end
            end
          endcase
        end

      // Read registers
      end else if (read_req_shift_reg[1]) begin
        // Acknowledge by default
        s_ctrlport_resp_ack    <= 1'b1;
        s_ctrlport_resp_status <= CTRL_STS_OKAY;

        // Address ATR state reads
        if(address_is_atr) begin

          case (register_base_address)
            REG_BASE + LED0_ATR_STATE(0): begin
              s_ctrlport_resp_data <= ram_led0_doa & LED_ATR_STATE_MASK;
            end
            REG_BASE + LED1_ATR_STATE(0): begin
              s_ctrlport_resp_data <= ram_led1_doa & LED_ATR_STATE_MASK;
            end
            REG_BASE + LED2_ATR_STATE(0): begin
              s_ctrlport_resp_data <= ram_led2_doa & LED_ATR_STATE_MASK;
            end
            REG_BASE + LED3_ATR_STATE(0): begin
              s_ctrlport_resp_data <= ram_led3_doa & LED_ATR_STATE_MASK;
            end

            default: begin
                s_ctrlport_resp_status <= CTRL_STS_CMDERR;
            end
          endcase

        end else begin

          // Address reads to the rest of the register space
          case (s_ctrlport_req_addr)

            REG_BASE + LED_ATR_OPTION_REGISTER: begin
              s_ctrlport_resp_data[LED0_ATR_OPTION] <= atr_mode[0];
              s_ctrlport_resp_data[LED1_ATR_OPTION] <= atr_mode[1];
              s_ctrlport_resp_data[LED2_ATR_OPTION] <= atr_mode[2];
              s_ctrlport_resp_data[LED3_ATR_OPTION] <= atr_mode[3];
            end

            REG_BASE + LED_ATR_DISABLED: begin
              s_ctrlport_resp_data[LED0_ATR_DISABLED] <= atr_disable[0];
              s_ctrlport_resp_data[LED1_ATR_DISABLED] <= atr_disable[1];
              s_ctrlport_resp_data[LED2_ATR_DISABLED] <= atr_disable[2];
              s_ctrlport_resp_data[LED3_ATR_DISABLED] <= atr_disable[3];
            end

            // No register implementation for provided address
            default: begin
              // Acknowledge and provide error status if address is in range
              if (address_in_range) begin
                s_ctrlport_resp_status <= CTRL_STS_CMDERR;

              // No response if out of range
              end else begin
                s_ctrlport_resp_ack <= 1'b0;
              end
            end
          endcase
        end

      end else begin
        s_ctrlport_resp_ack <= 1'b0;
      end
    end
  end

  // register without reset
  reg [         7:0] ram_addr   = 8'b0;
  reg [LED_SIZE-1:0] ram_datain = {LED_SIZE{1'b0}};
  always @(posedge ctrlport_clk) begin
    // memories
    ram_addr   <= atr_address;
    ram_datain <= s_ctrlport_req_data[LED_SIZE-1:0];
  end

  // ATR Scheme selection
  reg [7:0] atr_config_led [3:0];

  generate
    genvar i;
    for (i = 0; i < 4; i = i + 1) begin: read_address_gen

      always @(posedge ctrlport_clk) begin
        if (atr_disable[i]) begin
          atr_config_led[i] <= 8'b0;
        end else begin
          if (atr_mode[i]) begin
            atr_config_led[i] <= {6'b0, db_state[2*i+:2]};
          end else begin
            atr_config_led[i] <= db_state;
          end
        end
      end

    end
  endgenerate

  reg [31:0] led_combined = 32'b0;
  always @(posedge ctrlport_clk) begin
    led_combined[CH0_RX2_LED_EN]         <= ram_led0_dob[RX2_LED];
    led_combined[CH0_TRX1_LED_RED_EN]    <= ram_led0_dob[TXRX_RED_LED];
    led_combined[CH0_TRX1_LED_GR_EN]     <= ram_led0_dob[TXRX_GR_LED];

    led_combined[CH1_RX2_LED_EN]         <= ram_led1_dob[RX2_LED];
    led_combined[CH1_TRX1_LED_RED_EN]    <= ram_led1_dob[TXRX_RED_LED];
    led_combined[CH1_TRX1_LED_GR_EN]     <= ram_led1_dob[TXRX_GR_LED];

    led_combined[CH2_RX2_LED_EN]         <= ram_led2_dob[RX2_LED];
    led_combined[CH2_TRX1_LED_RED_EN]    <= ram_led2_dob[TXRX_RED_LED];
    led_combined[CH2_TRX1_LED_GR_EN]     <= ram_led2_dob[TXRX_GR_LED];

    led_combined[CH3_RX2_LED_EN]         <= ram_led3_dob[RX2_LED];
    led_combined[CH3_TRX1_LED_RED_EN]    <= ram_led3_dob[TXRX_RED_LED];
    led_combined[CH3_TRX1_LED_GR_EN]     <= ram_led3_dob[TXRX_GR_LED];

  end


  //---------------------------------------------------------------
  // ATR memory
  //---------------------------------------------------------------
  ram_2port #(
    .DWIDTH     (LED_SIZE),
    .AWIDTH     (8),
    .RW_MODE    ("READ-FIRST"),
    .RAM_TYPE   ("AUTOMATIC"),
    .OUT_REG    (0)
  ) ram_led0_i (
    .clka   (ctrlport_clk),
    .ena    (1'b1),
    .wea    (ram_led0_wea),
    .addra  (ram_addr),
    .dia    (ram_datain),
    .doa    (ram_led0_doa),
    .clkb   (ctrlport_clk),
    .enb    (1'b1),
    .web    (1'b0),
    .addrb  (atr_config_led[0]),
    .dib    ({LED_SIZE{1'b0}}),
    .dob    (ram_led0_dob));

  ram_2port #(
    .DWIDTH     (LED_SIZE),
    .AWIDTH     (8),
    .RW_MODE    ("READ-FIRST"),
    .RAM_TYPE   ("AUTOMATIC"),
    .OUT_REG    (0)
  ) ram_led1_i (
    .clka   (ctrlport_clk),
    .ena    (1'b1),
    .wea    (ram_led1_wea),
    .addra  (ram_addr),
    .dia    (ram_datain),
    .doa    (ram_led1_doa),
    .clkb   (ctrlport_clk),
    .enb    (1'b1),
    .web    (1'b0),
    .addrb  (atr_config_led[1]),
    .dib    ({LED_SIZE{1'b0}}),
    .dob    (ram_led1_dob));

  ram_2port #(
    .DWIDTH     (LED_SIZE),
    .AWIDTH     (8),
    .RW_MODE    ("READ-FIRST"),
    .RAM_TYPE   ("AUTOMATIC"),
    .OUT_REG    (0)
  ) ram_led2_i (
    .clka   (ctrlport_clk),
    .ena    (1'b1),
    .wea    (ram_led2_wea),
    .addra  (ram_addr),
    .dia    (ram_datain),
    .doa    (ram_led2_doa),
    .clkb   (ctrlport_clk),
    .enb    (1'b1),
    .web    (1'b0),
    .addrb  (atr_config_led[2]),
    .dib    ({LED_SIZE{1'b0}}),
    .dob    (ram_led2_dob));

  ram_2port #(
    .DWIDTH     (LED_SIZE),
    .AWIDTH     (8),
    .RW_MODE    ("READ-FIRST"),
    .RAM_TYPE   ("AUTOMATIC"),
    .OUT_REG    (0)
  ) ram_led3_i (
    .clka   (ctrlport_clk),
    .ena    (1'b1),
    .wea    (ram_led3_wea),
    .addra  (ram_addr),
    .dia    (ram_datain),
    .doa    (ram_led3_doa),
    .clkb   (ctrlport_clk),
    .enb    (1'b1),
    .web    (1'b0),
    .addrb  (atr_config_led[3]),
    .dib    ({LED_SIZE{1'b0}}),
    .dob    (ram_led3_dob));


  //----------------------------------------------------------
  // Logic to wait for response after triggering request
  //----------------------------------------------------------

  reg transfer_in_progress;
  reg [31:0] led_combined_delayed = 32'b0;

  always @(posedge ctrlport_clk) begin
    if (ctrlport_rst) begin
      m_ctrlport_req_wr <= 1'b0;
      transfer_in_progress <= 1'b0;
      led_combined_delayed <= 16'b0;
    end else begin
      // Default assignment
      m_ctrlport_req_wr <= 1'b0;

      // Issue new request on change if no request is pending
      if (led_combined != led_combined_delayed && ~transfer_in_progress) begin
        transfer_in_progress <= 1'b1;
        m_ctrlport_req_wr <= 1'b1;
        led_combined_delayed <= led_combined;
      end

      // Reset pending request
      if (m_ctrlport_resp_ack) begin
        transfer_in_progress <= 1'b0;
      end
    end
  end

  //----------------------------------------------------------
  // Static ControlPort assignments
  //----------------------------------------------------------

  assign m_ctrlport_req_rd      = 0;
  assign m_ctrlport_req_byte_en = 4'b1111;
  assign m_ctrlport_req_addr    = LED_REGISTER_ADDRESS;
  assign m_ctrlport_req_data    = {led_combined_delayed};

endmodule

//XmlParse xml_on
//<regmap name="LED_ATR_REGMAP" readablestrobes="false" generatevhdl="true" ettusguidelines="true">
//  <group name="LED_ATR_REGISTERS">
//    <info>
//      Each channel in the FBX daughterboard has 3 LEDs. TXRX Red/Green LEDs and RX2 Green LED.
//      This register map describes how to control the behavior of the 3 LEDs.
//      There are three supported control schemes for these LEDs:</br>
//      <ul>
//        <li>ATR Disabled - Single persistent state.</li>
//        <li>Classic ATR  - Each channel's LEDs depend on the transmission state
//                           of the respective channel.</li>
//        <li>DB State     - Each channel's LEDs depend on the transmission state
//                           of all channels in this radio.</li>
//      </ul>
//    </info>
//
//    <enumeratedtype name="LED_SIZE_TYPE">
//      <value name="LED_SIZE" integer="3"/>
//    </enumeratedtype>
//
//    <regtype name="LED_ATR_STATE" size="32">
//      <info>Holds the value for the control lines of each channel's LEDs
//      for a particular ATR sate</info>
//      <bitfield name="RX2_LED"      range="0" initialvalue="0"/>
//      <bitfield name="TXRX_RED_LED" range="1" initialvalue="0"/>
//      <bitfield name="TXRX_GR_LED"  range="2" initialvalue="0"/>
//    </regtype>
//
//    <register name="LED0_ATR_STATE" typename="LED_ATR_STATE"  offset="0x00" count="256" options="--step 4">
//      <info>
//        Describes led behavior for the different ATR states. When @.LED0_ATR_OPTION
//        is set to use the DB states, TX and RX states for LED0-LED3 are
//        combined to create a single vector. This creates 256 different
//        combinations, each with its own register. When @.LED0_ATR_OPTION is set to
//        classic ATR, the first 4 offsets in this register group will be driven
//        in accordance with the state of RF0.
//        CLASSIC ATR MAPPING: Idle[RF0: TX=0, RX=0], RX[RF0: TX=0, RX=1,
//        TX[RF0: TX=1, RX=0], FDX[RF0: TX=1, RX=1]
//      </info>
//    </register>
//
//    <register name="LED1_ATR_STATE" typename="LED_ATR_STATE"  offset="0x400" count="256" options="--step 4">
//      <info>
//        Describes led behavior for the different ATR states. When @.LED1_ATR_OPTION
//        is set to use the DB states, TX and RX states for LED0-LED3 are
//        combined to create a single vector. This creates 256 different
//        combinations, each with its own register. When @.LED1_ATR_OPTION is set to
//        classic ATR, the first 4 offsets in this register group will be driven
//        in accordance with the state of RF1.
//        CLASSIC ATR MAPPING: Idle[RF1: TX=0, RX=0], RX[RF1: TX=0, RX=1,
//        TX[RF1: TX=1, RX=0], FDX[RF1: TX=1, RX=1]
//      </info>
//    </register>
//
//    <register name="LED2_ATR_STATE" typename="LED_ATR_STATE"  offset="0x800" count="256" options="--step 4">
//      <info>
//        Describes led behavior for the different ATR states. When @.LED2_ATR_OPTION
//        is set to use the DB states, TX and RX states for LED0-LED3 are
//        combined to create a single vector. This creates 256 different
//        combinations, each with its own register. When @.LED2_ATR_OPTION is set to
//        classic ATR, the first 4 offsets in this register group will be driven
//        in accordance with the state of RF2.
//        CLASSIC ATR MAPPING: Idle[RF2: TX=0, RX=0], RX[RF2: TX=0, RX=1,
//        TX[RF2: TX=1, RX=0], FDX[RF2: TX=1, RX=1]
//      </info>
//    </register>
//
//    <register name="LED3_ATR_STATE" typename="LED_ATR_STATE"  offset="0xC00" count="256" options="--step 4">
//      <info>
//        Describes led behavior for the different ATR states. When @.LED3_ATR_OPTION
//        is set to use the DB states, TX and RX states for LED0-LED3 are
//        combined to create a single vector. This creates 256 different
//        combinations, each with its own register. When @.LED3_ATR_OPTION is set to
//        classic ATR, the first 4 offsets in this register group will be driven
//        in accordance with the state of RF3.
//        CLASSIC ATR MAPPING: Idle[RF3: TX=0, RX=0], RX[RF3: TX=0, RX=1,
//        TX[RF3: TX=1, RX=0], FDX[RF3: TX=1, RX=1]
//      </info>
//    </register>
//    <register name="LED_ATR_OPTION_REGISTER" offset="0x1000" size="32">
//      <info>
//        Controls whether switch control lines use the TX and RX state of
//        their respective channel (Classic ATR) or the daughterboard state
//        to select which state to use from values set in LED_ATR_STATE registers.
//        For each particular bit:</br>
//            0: Use DB state for ATR</br>
//            1: Classic ATR mode.
//      </info>
//      <bitfield name="LED0_ATR_OPTION" range="0" initialvalue="0">
//        <info>
//          Control ATR scheme for RF0.
//        </info>
//      </bitfield>
//      <bitfield name="LED1_ATR_OPTION" range="1" initialvalue="0">
//        <info>
//          Control ATR scheme for RF1.
//        </info>
//      </bitfield>
//      <bitfield name="LED2_ATR_OPTION" range="2" initialvalue="0">
//        <info>
//          Control ATR scheme for RF2.
//        </info>
//      </bitfield>
//      <bitfield name="LED3_ATR_OPTION" range="3" initialvalue="0">
//        <info>
//          Control ATR scheme for RF3.
//        </info>
//      </bitfield>
//    </register>
//    <register name="LED_ATR_DISABLED" offset="0x1004" size="32">
//      <info>
//        Disable ATR Control. DB state 0 will be reflected regardless of the ATR state.
//      </info>
//      <bitfield name="LED0_ATR_DISABLED" range="0" initialvalue="0"/>
//      <bitfield name="LED1_ATR_DISABLED" range="1" initialvalue="0"/>
//      <bitfield name="LED2_ATR_DISABLED" range="2" initialvalue="0"/>
//      <bitfield name="LED3_ATR_DISABLED" range="3" initialvalue="0"/>
//    </register>
//  </group>
//</regmap>
//XmlParse xml_off


`default_nettype wire

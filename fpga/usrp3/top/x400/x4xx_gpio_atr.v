//
// Copyright 2021 Ettus Research, A National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: x4xx_gpio_atr
//
// Description:
//
//  This module controls the behavior of a GPIO bus of arbitrary width
//  based on the current ATR state. The radio state is determined by
//  combining the TX and RX states of each rf channel in the radio in
//  the following order: {tx_rf1, rx_rf1, tx_rf0, rx_rf0}
//
// Parameters:
//
//   REG_BASE       : Base address to use for registers.
//   REG_SIZE       : Register space size.
//   WIDTH          : Number of GPIO lines controlled by this block.
//   NUM_CH_PER_DB  : Number of RF Channels per daughterboard
//
module x4xx_gpio_atr #(
  parameter REG_BASE      = 0,
  parameter REG_SIZE      = 'h20,
  parameter WIDTH         = 32,
  parameter NUM_CH_PER_DB = 2
) (
  // Slave ctrlport interface
  input  wire        ctrlport_clk,
  input  wire        ctrlport_rst,
  input  wire        s_ctrlport_req_wr,
  input  wire        s_ctrlport_req_rd,
  input  wire [19:0] s_ctrlport_req_addr,
  input  wire [31:0] s_ctrlport_req_data,
  output reg         s_ctrlport_resp_ack    = 1'b0,
  output reg  [ 1:0] s_ctrlport_resp_status = 2'b00,
  output reg  [31:0] s_ctrlport_resp_data   = {32 {1'bX}},
  // Run state signals that indicate tx and rx operation
  input wire [2*NUM_CH_PER_DB-1:0] db_state,
  // GPIO control signals
  input  wire [WIDTH-1:0]  gpio_in,                              //GPIO input state
  output reg  [WIDTH-1:0]  gpio_out  = {WIDTH {1'b0}},           //GPIO output state
  output reg  [WIDTH-1:0]  gpio_ddr  = {WIDTH {1'b0}}            //GPIO direction (0=input, 1=output)
);

  `include "../../lib/rfnoc/core/ctrlport.vh"
  `include "regmap/gpio_atr_regmap_utils.vh"

  reg [WIDTH-1:0]   in_atr_state [15:0];

  initial begin
    in_atr_state[0]   = {WIDTH {1'b0}};
    in_atr_state[1]   = {WIDTH {1'b0}};
    in_atr_state[2]   = {WIDTH {1'b0}};
    in_atr_state[3]   = {WIDTH {1'b0}};
    in_atr_state[4]   = {WIDTH {1'b0}};
    in_atr_state[5]   = {WIDTH {1'b0}};
    in_atr_state[6]   = {WIDTH {1'b0}};
    in_atr_state[7]   = {WIDTH {1'b0}};
    in_atr_state[8]   = {WIDTH {1'b0}};
    in_atr_state[9]   = {WIDTH {1'b0}};
    in_atr_state[10]  = {WIDTH {1'b0}};
    in_atr_state[11]  = {WIDTH {1'b0}};
    in_atr_state[12]  = {WIDTH {1'b0}};
    in_atr_state[13]  = {WIDTH {1'b0}};
    in_atr_state[14]  = {WIDTH {1'b0}};
    in_atr_state[15]  = {WIDTH {1'b0}};
  end

  reg [WIDTH-1:0]   ddr_reg, atr_disable, gpio_sw_rb = {WIDTH {1'b0}};
  reg [WIDTH-1:0]   ogpio, igpio = {WIDTH {1'b0}};
  reg [WIDTH-1:0]   classic_atr_select = {WIDTH {1'b0}};

  // DB state/Classic ATR selector
  reg atr_mode = 1'b0;

  genvar state;
  //---------------------------------------------------------------------------
  // Control interface handling
  //---------------------------------------------------------------------------

  // Check that address is within this module's range.
  wire address_in_range = (s_ctrlport_req_addr >= REG_BASE) && (s_ctrlport_req_addr < REG_BASE + REG_SIZE);

  // Check that address is targeting an ATR state.
  wire address_is_atr = (s_ctrlport_req_addr >= REG_BASE + ATR_STATE(0)) && (s_ctrlport_req_addr <= REG_BASE + ATR_STATE(15));
  // Decode the ATR state being addressed.
  wire [3:0]atr_address = s_ctrlport_req_addr[5:2];

  always @ (posedge ctrlport_clk) begin
    if (ctrlport_rst) begin
      s_ctrlport_resp_ack    <= 1'b0;
      s_ctrlport_resp_data   <= {32 {1'bX}};
      s_ctrlport_resp_status <= 2'b00;

      ddr_reg             <= {WIDTH {1'b0}};
      atr_disable         <= {WIDTH {1'b0}};
      classic_atr_select  <= {WIDTH {1'b0}};
      atr_mode            <= 1'b0;

      in_atr_state[0]   <= {WIDTH {1'b0}};
      in_atr_state[1]   <= {WIDTH {1'b0}};
      in_atr_state[2]   <= {WIDTH {1'b0}};
      in_atr_state[3]   <= {WIDTH {1'b0}};
      in_atr_state[4]   <= {WIDTH {1'b0}};
      in_atr_state[5]   <= {WIDTH {1'b0}};
      in_atr_state[6]   <= {WIDTH {1'b0}};
      in_atr_state[7]   <= {WIDTH {1'b0}};
      in_atr_state[8]   <= {WIDTH {1'b0}};
      in_atr_state[9]   <= {WIDTH {1'b0}};
      in_atr_state[10]  <= {WIDTH {1'b0}};
      in_atr_state[11]  <= {WIDTH {1'b0}};
      in_atr_state[12]  <= {WIDTH {1'b0}};
      in_atr_state[13]  <= {WIDTH {1'b0}};
      in_atr_state[14]  <= {WIDTH {1'b0}};
      in_atr_state[15]  <= {WIDTH {1'b0}};

    end else begin
      // Write registers
      if (s_ctrlport_req_wr) begin
        // Acknowledge by default
        s_ctrlport_resp_ack    <= 1'b1;
        s_ctrlport_resp_data   <= {CTRLPORT_DATA_W {1'b0}};
        s_ctrlport_resp_status <= CTRL_STS_OKAY;

        // Address ATR state writes
        if(address_is_atr) begin
          in_atr_state[atr_address][GPIO_STATE_A_MSB:GPIO_STATE_A] <= s_ctrlport_req_data[GPIO_STATE_A_MSB:GPIO_STATE_A];
          in_atr_state[atr_address][GPIO_STATE_B_MSB:GPIO_STATE_B] <= s_ctrlport_req_data[GPIO_STATE_B_MSB:GPIO_STATE_B];
        end else begin

          // Address writes to the rest of the register space
          case (s_ctrlport_req_addr)

            REG_BASE + ATR_OPTION_REGISTRER: begin
              atr_mode <= s_ctrlport_req_data[ATR_OPTION];
            end

            REG_BASE + CLASSIC_ATR_CONFIG: begin
              classic_atr_select[RF_SELECT_A_MSB:RF_SELECT_A] <= s_ctrlport_req_data[RF_SELECT_A_MSB:RF_SELECT_A];
              classic_atr_select[RF_SELECT_B_MSB:RF_SELECT_B] <= s_ctrlport_req_data[RF_SELECT_B_MSB:RF_SELECT_B];
            end

            REG_BASE + GPIO_DIR: begin
              ddr_reg[GPIO_DIR_A_MSB:GPIO_DIR_A] <= s_ctrlport_req_data[GPIO_DIR_A_MSB:GPIO_DIR_A];
              ddr_reg[GPIO_DIR_B_MSB:GPIO_DIR_B] <= s_ctrlport_req_data[GPIO_DIR_B_MSB:GPIO_DIR_B];
            end

            REG_BASE + GPIO_DISABLED: begin
              atr_disable[GPIO_DISABLED_A_MSB:GPIO_DISABLED_A] <= s_ctrlport_req_data[GPIO_DISABLED_A_MSB:GPIO_DISABLED_A];
              atr_disable[GPIO_DISABLED_B_MSB:GPIO_DISABLED_B] <= s_ctrlport_req_data[GPIO_DISABLED_B_MSB:GPIO_DISABLED_B];
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
      end else if (s_ctrlport_req_rd) begin
        // Acknowledge by default
        s_ctrlport_resp_ack    <= 1'b1;
        s_ctrlport_resp_data   <= {CTRLPORT_DATA_W {1'b0}};
        s_ctrlport_resp_status <= CTRL_STS_OKAY;

        // Address ATR state reads
        if(address_is_atr) begin
            s_ctrlport_resp_data[GPIO_STATE_A_MSB:GPIO_STATE_A] <= in_atr_state[atr_address][GPIO_STATE_A_MSB:GPIO_STATE_A];
            s_ctrlport_resp_data[GPIO_STATE_B_MSB:GPIO_STATE_B] <= in_atr_state[atr_address][GPIO_STATE_B_MSB:GPIO_STATE_B];
        end else begin

          // Address reads to the rest of the register space
          case (s_ctrlport_req_addr)

            REG_BASE + ATR_OPTION_REGISTRER: begin
              s_ctrlport_resp_data[ATR_OPTION] <= atr_mode;
            end

            REG_BASE + CLASSIC_ATR_CONFIG: begin
              s_ctrlport_resp_data[RF_SELECT_A_MSB:RF_SELECT_A] <= classic_atr_select[RF_SELECT_A_MSB:RF_SELECT_A];
              s_ctrlport_resp_data[RF_SELECT_B_MSB:RF_SELECT_B] <= classic_atr_select[RF_SELECT_B_MSB:RF_SELECT_B];
            end

            REG_BASE + GPIO_DIR: begin
              s_ctrlport_resp_data[GPIO_DIR_A_MSB:GPIO_DIR_A] <= ddr_reg[GPIO_DIR_A_MSB:GPIO_DIR_A];
              s_ctrlport_resp_data[GPIO_DIR_B_MSB:GPIO_DIR_B] <= ddr_reg[GPIO_DIR_B_MSB:GPIO_DIR_B];
            end

            REG_BASE + GPIO_DISABLED: begin
              s_ctrlport_resp_data[GPIO_DISABLED_A_MSB:GPIO_DISABLED_A] <= atr_disable[GPIO_DISABLED_A_MSB:GPIO_DISABLED_A];
              s_ctrlport_resp_data[GPIO_DISABLED_B_MSB:GPIO_DISABLED_B] <= atr_disable[GPIO_DISABLED_B_MSB:GPIO_DISABLED_B];
            end

            REG_BASE + GPIO_IN: begin
              s_ctrlport_resp_data[GPIO_IN_A_MSB:GPIO_IN_A] <= gpio_sw_rb[GPIO_IN_A_MSB:GPIO_IN_A];
              s_ctrlport_resp_data[GPIO_IN_B_MSB:GPIO_IN_B] <= gpio_sw_rb[GPIO_IN_B_MSB:GPIO_IN_B];
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

  genvar i;

  //Pipeline for easier timing closure
  reg [      3:0] db_state_d           = 4'b0;
  reg             atr_mode_d           = 1'b0;
  reg [WIDTH-1:0] classic_atr_select_d = {WIDTH {1'b0}};
  always @(posedge ctrlport_clk) begin
    db_state_d <= db_state;
    atr_mode_d <= atr_mode;
    classic_atr_select_d <= classic_atr_select;
  end

  generate
    for (i=0; i<WIDTH; i=i+1) begin: gpio_mux_gen
      //ATR selection MUX
      // Classic ATR
      always @(posedge ctrlport_clk) begin
        if(atr_mode_d) begin
          if (atr_disable[i]) begin
            // ATR state 0 for RF 0 and ATR state 4 for RF1
            ogpio[i] <= in_atr_state[classic_atr_select_d[i]*4][i];
          end else begin
            if (classic_atr_select_d[i]) begin // RF 1
              ogpio[i] <= in_atr_state[db_state_d[3:2] + 4][i];
            end else begin // RF 0
              ogpio[i] <= in_atr_state[db_state_d[1:0]][i];
            end
          end
        end else begin // Use DB state
          if (atr_disable[i]) begin
            ogpio[i] <= in_atr_state[0][i];
          end else begin
            ogpio[i] <= in_atr_state[db_state_d][i];
          end
        end
      end
    end
  endgenerate

  //Pipeline input, output and direction
  always @(posedge ctrlport_clk)
    gpio_out <= ogpio;

  always @(posedge ctrlport_clk)
    igpio <= gpio_in;

  always @(posedge ctrlport_clk)
    gpio_ddr <= ddr_reg;

  //Generate software readback state
  generate
    for (i=0; i<WIDTH; i=i+1) begin: gpio_rb_gen
    always @(posedge ctrlport_clk)
      gpio_sw_rb[i] <= gpio_ddr[i] ? gpio_out[i] : igpio[i];
    end
  endgenerate

endmodule

//XmlParse xml_on
//<regmap name="GPIO_ATR_REGMAP" readablestrobes="false" generatevhdl="true" ettusguidelines="true">
//  <group name="GPIO_ATR_REGS">
//    <info>
//      Describes the behavior of GPIO lines when controlled by the ATR state.
//    </info>
//
//    <regtype name="GPIO_ATR_STATE" size="32">
//      <info>Holds a single bit setting for GPIO lines in both ports for a particular ATR sate</info>
//      <bitfield name="GPIO_STATE_A" range="0..11" initialvalue="0"/>
//      <bitfield name="GPIO_STATE_B" range="16..27" initialvalue="0"/>
//    </regtype>
//
//    <register name="ATR_STATE" typename="GPIO_ATR_STATE"  offset="0x00" count="16" options="--step 4">
//      <info>
//        Describes GPIO behavior for the different ATR states. When @.ATR_OPTION
//        is set to use the DB states, TX and RX states for RF0 and RF1 are
//        combined to create a single vector. This creates 16 different
//        combinations, each with its own register. When @.ATR_OPTION is set to
//        classic ATR, offsets 0x00-0x03 in this register group will be driven
//        in accordance with the state of RF0, and offsets 0x04-0x07 will be
//        driven in accordance with the state of RF1.
//        CLASSIC ATR MAPPING: Idle[RF0:0x00; RF1:0x04], RX[RF0:0x01; RF1:0x05],
//        TX[RF0:0x02; RF1:0x06], FDX[RF0:0x03; RF1:0x07]
//      </info>
//    </register>
//    <register name="ATR_OPTION_REGISTRER" offset="0x44" size="32">
//      <info>
//        Controls whether GPIO lines use the TX and RX state of an RF channel
//        (Classic ATR) or the daughterboard state the selector for the
//        @.ATR_STATE.
//      </info>
//      <bitfield name="ATR_OPTION" range="0" initialvalue="0">
//        <info>
//          Sets the scheme in which RF states in the radio will control GPIO
//          lines. 0 = DB state is used. RF states are combined and the
//          GPIO state is driven based on all 16 @.ATR_STATE registers.
//          1 = Each RF channel has its separate ATR state(Classic ATR).
//          Use register @.CLASSIC_ATR_CONFIG to indicate the RF channel
//          to which each GPIO line responds to.
//        </info>
//      </bitfield>
//    </register>
//    <register name="CLASSIC_ATR_CONFIG" offset="0x40" size="32">
//      <info>
//        Controls the RF state mapping of each GPIO line when classic
//        ATR mode is active.
//      </info>
//      <bitfield name="RF_SELECT_A" range="0..11" initialvalue="0">
//        <info>
//          Set which RF channel's state to reflect in the pins for
//          HDMI connector A when @.ATR_OPTION is set to classic ATR.
//          Controlled in a per-pin basis.
//            0 = RF0 State(GPIO_ATR_STATE 0x00-0x03)
//            1 = RF1 State(GPIO_ATR_STATE 0x04-0x07)
//        </info>
//      </bitfield>
//      <bitfield name="RF_SELECT_B" range="16..27" initialvalue="0">
//        <info>
//          Set which RF channel's state to reflect in the pins of
//          HDMI connector B when @.ATR_OPTION is set to classic ATR.
//          Controlled in a per-pin basis.
//            0 = RF0 State(GPIO_ATR_STATE 0x00-0x03)
//            1 = RF1 State(GPIO_ATR_STATE 0x04-0x07)
//        </info>
//      </bitfield>
//    </register>
//    <register name="GPIO_DIR" offset="0x48" size="32">
//      <info>
//        Controls the direction of each GPIO signal when controlled by the radio state.
//        0 = GPIO pin set to input. 1 = GPIO pin set to output
//      </info>
//      <bitfield name="GPIO_DIR_A" range="0..11" initialvalue="0"/>
//      <bitfield name="GPIO_DIR_B" range="16..27" initialvalue="0"/>
//    </register>
//    <register name="GPIO_DISABLED" offset="0x4C" size="32">
//      <info>
//        Disable ATR Control. DB state 0 will be reflected regardless of the ATR state.
//      </info>
//      <bitfield name="GPIO_DISABLED_A" range="0..11" initialvalue="0"/>
//      <bitfield name="GPIO_DISABLED_B" range="16..27" initialvalue="0"/>
//    </register>
//    <register name="GPIO_IN" offset="0x50" size="32">
//      <info>
//        Reflects the logic state of each GPIO input.
//      </info>
//      <bitfield name="GPIO_IN_A" range="0..11" initialvalue="0"/>
//      <bitfield name="GPIO_IN_B" range="16..27" initialvalue="0"/>
//    </register>
//  </group>
//</regmap>
//XmlParse xml_off

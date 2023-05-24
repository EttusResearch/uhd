//
// Copyright 2022 Ettus Research, A National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: rf_atr_control
//
// Description:
//
//  Implements control over RF switches via CtrlPort. Uses RAM to store multiple
//  ATR configurations.
//  There are three supported control schemes for these switches:
//    - ATR Disabled - Single persistent state.
//    - Classic ATR  - Each channel's switches depend on the transmission state
//                     of the respective channel.
//    - DB State     - Each channel's switches depend on the transmission state
//                     of all channels in this radio.
//
// Parameters:
//
//   REG_BASE : Base address to use for registers.
//   REG_SIZE : Register space size.
//

module rf_atr_control #(
  parameter REG_BASE = 0,
  parameter REG_SIZE = 'h2000
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
  output reg  [31:0] s_ctrlport_resp_data   = {32 {1'b0}},
  // Run state signals that indicate tx and rx operation
  input wire [7:0] db_state,
  // Switch control outputs
  output reg rf0_tx_rx_rfs,
  output reg rf0_rx_rfs,
  output reg rf0_tdds,
  output reg rf1_tx_rx_rfs,
  output reg rf1_rx_rfs,
  output reg rf1_tdds,
  output reg rf2_tx_rx_rfs,
  output reg rf2_rx_rfs,
  output reg rf2_tdds,
  output reg rf3_tx_rx_rfs,
  output reg rf3_rx_rfs,
  output reg rf3_tdds
);

  `include "../../../../lib/rfnoc/core/ctrlport.vh"
  `include "regmap/rf_atr_regmap_utils.vh"

  //---------------------------------------------------------------
  // ATR memory signals
  //---------------------------------------------------------------
  reg                 ram_rf0_wea;
  wire [RFS_SIZE-1:0] ram_rf0_doa;
  wire [RFS_SIZE-1:0] ram_rf0_dob;

  reg                 ram_rf1_wea;
  wire [RFS_SIZE-1:0] ram_rf1_doa;
  wire [RFS_SIZE-1:0] ram_rf1_dob;

  reg                 ram_rf2_wea;
  wire [RFS_SIZE-1:0] ram_rf2_doa;
  wire [RFS_SIZE-1:0] ram_rf2_dob;

  reg                 ram_rf3_wea;
  wire [RFS_SIZE-1:0] ram_rf3_doa;
  wire [RFS_SIZE-1:0] ram_rf3_dob;


  //---------------------------------------------------------------
  // ATR Scheme signals
  //---------------------------------------------------------------
  reg [3:0]  atr_disable  = 4'b0;

  // DB state/Classic ATR selector
  reg [3:0]  atr_mode = 4'b0;

  //---------------------------------------------------------------------------
  // Control interface handling
  //---------------------------------------------------------------------------

  // Check that address is within this module's range.
  wire address_in_range = (s_ctrlport_req_addr >= REG_BASE) && (s_ctrlport_req_addr < REG_BASE + REG_SIZE);

  // Check that address is targeting an ATR state.
  wire address_is_atr = (s_ctrlport_req_addr >= REG_BASE + RF0_ATR_STATE(0)) && (s_ctrlport_req_addr <= REG_BASE + RF3_ATR_STATE(RF3_ATR_STATE_COUNT-1));

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

      ram_rf0_wea <= 1'b0;
      ram_rf1_wea <= 1'b0;
      ram_rf2_wea <= 1'b0;
      ram_rf3_wea <= 1'b0;

    end else begin
      // default assignments
      read_req_shift_reg <= {read_req_shift_reg[0], s_ctrlport_req_rd};

      ram_rf0_wea <= 1'b0;
      ram_rf1_wea <= 1'b0;
      ram_rf2_wea <= 1'b0;
      ram_rf3_wea <= 1'b0;

      // Write registers
      if (s_ctrlport_req_wr) begin
        // Acknowledge by default
        s_ctrlport_resp_ack    <= 1'b1;
        s_ctrlport_resp_status <= CTRL_STS_OKAY;

        // Address ATR state writes
        if(address_is_atr) begin

          case (register_base_address)
            REG_BASE + RF0_ATR_STATE(0): begin
              ram_rf0_wea <= 1'b1;
            end

            REG_BASE + RF1_ATR_STATE(0): begin
              ram_rf1_wea <= 1'b1;
            end

            REG_BASE + RF2_ATR_STATE(0): begin
              ram_rf2_wea <= 1'b1;
            end

            REG_BASE + RF3_ATR_STATE(0): begin
              ram_rf3_wea <= 1'b1;
            end

            // error on undefined address
            default: begin
              s_ctrlport_resp_status <= CTRL_STS_CMDERR;
            end
          endcase
        end else begin

          // Address writes to the rest of the register space
          case (s_ctrlport_req_addr)

            REG_BASE + ATR_OPTION_REGISTER: begin
              atr_mode[0] <= s_ctrlport_req_data[RF0_ATR_OPTION];
              atr_mode[1] <= s_ctrlport_req_data[RF1_ATR_OPTION];
              atr_mode[2] <= s_ctrlport_req_data[RF2_ATR_OPTION];
              atr_mode[3] <= s_ctrlport_req_data[RF3_ATR_OPTION];
            end

            REG_BASE + RF_ATR_DISABLED: begin
              atr_disable[0] <= s_ctrlport_req_data[RF0_ATR_DISABLED];
              atr_disable[1] <= s_ctrlport_req_data[RF1_ATR_DISABLED];
              atr_disable[2] <= s_ctrlport_req_data[RF2_ATR_DISABLED];
              atr_disable[3] <= s_ctrlport_req_data[RF3_ATR_DISABLED];
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
            REG_BASE + RF0_ATR_STATE(0): begin
              s_ctrlport_resp_data <= ram_rf0_doa & RF_ATR_STATE_MASK;
            end
            REG_BASE + RF1_ATR_STATE(0): begin
              s_ctrlport_resp_data <= ram_rf1_doa & RF_ATR_STATE_MASK;
            end
            REG_BASE + RF2_ATR_STATE(0): begin
              s_ctrlport_resp_data <= ram_rf2_doa & RF_ATR_STATE_MASK;
            end
            REG_BASE + RF3_ATR_STATE(0): begin
              s_ctrlport_resp_data <= ram_rf3_doa & RF_ATR_STATE_MASK;
            end

            default: begin
                s_ctrlport_resp_status <= CTRL_STS_CMDERR;
            end
          endcase

        end else begin

          // Address reads to the rest of the register space
          case (s_ctrlport_req_addr)

            REG_BASE + ATR_OPTION_REGISTER: begin
              s_ctrlport_resp_data[RF0_ATR_OPTION] <= atr_mode[0];
              s_ctrlport_resp_data[RF1_ATR_OPTION] <= atr_mode[1];
              s_ctrlport_resp_data[RF2_ATR_OPTION] <= atr_mode[2];
              s_ctrlport_resp_data[RF3_ATR_OPTION] <= atr_mode[3];
            end

            REG_BASE + RF_ATR_DISABLED: begin
              s_ctrlport_resp_data[RF0_ATR_DISABLED] <= atr_disable[0];
              s_ctrlport_resp_data[RF1_ATR_DISABLED] <= atr_disable[1];
              s_ctrlport_resp_data[RF2_ATR_DISABLED] <= atr_disable[2];
              s_ctrlport_resp_data[RF3_ATR_DISABLED] <= atr_disable[3];
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
  reg [RFS_SIZE-1:0] ram_datain = {RFS_SIZE{1'b0}};
  always @(posedge ctrlport_clk) begin
    // memories
    ram_addr   <= atr_address;
    ram_datain <= s_ctrlport_req_data[RFS_SIZE-1:0];
  end

  // ATR Scheme selection
  reg [7:0] atr_config_rf [3:0];

  generate
    genvar i;
    for (i = 0; i < 4; i = i + 1) begin: read_address_gen

      always @(posedge ctrlport_clk) begin
        if (atr_disable[i]) begin
          atr_config_rf[i] <= 8'b0;
        end else begin
          if (atr_mode[i]) begin
            atr_config_rf[i] <= {6'b0, db_state[2*i+:2]};
          end else begin
            atr_config_rf[i] <= db_state;
          end
        end
      end

    end
  endgenerate


  // Output decoding
  always @(posedge ctrlport_clk) begin
    rf0_tx_rx_rfs <= ram_rf0_dob[TX_RX_RFS];
    rf0_rx_rfs    <= ram_rf0_dob[RX_RFS];
    rf0_tdds      <= ram_rf0_dob[TDDS];

    rf1_tx_rx_rfs <= ram_rf1_dob[TX_RX_RFS];
    rf1_rx_rfs    <= ram_rf1_dob[RX_RFS];
    rf1_tdds      <= ram_rf1_dob[TDDS];

    rf2_tx_rx_rfs <= ram_rf2_dob[TX_RX_RFS];
    rf2_rx_rfs    <= ram_rf2_dob[RX_RFS];
    rf2_tdds      <= ram_rf2_dob[TDDS];

    rf3_tx_rx_rfs <= ram_rf3_dob[TX_RX_RFS];
    rf3_rx_rfs    <= ram_rf3_dob[RX_RFS];
    rf3_tdds      <= ram_rf3_dob[TDDS];
  end


  //---------------------------------------------------------------
  // ATR memory
  //---------------------------------------------------------------
  ram_2port #(
    .DWIDTH     (RFS_SIZE),
    .AWIDTH     (8),
    .RW_MODE    ("READ-FIRST"),
    .RAM_TYPE   ("AUTOMATIC"),
    .OUT_REG    (0)
  ) ram_rf0_i (
    .clka   (ctrlport_clk),
    .ena    (1'b1),
    .wea    (ram_rf0_wea),
    .addra  (ram_addr),
    .dia    (ram_datain),
    .doa    (ram_rf0_doa),
    .clkb   (ctrlport_clk),
    .enb    (1'b1),
    .web    (1'b0),
    .addrb  (atr_config_rf[0]),
    .dib    ({RFS_SIZE{1'b0}}),
    .dob    (ram_rf0_dob));

  ram_2port #(
    .DWIDTH     (RFS_SIZE),
    .AWIDTH     (8),
    .RW_MODE    ("READ-FIRST"),
    .RAM_TYPE   ("AUTOMATIC"),
    .OUT_REG    (0)
  ) ram_rf1_i (
    .clka   (ctrlport_clk),
    .ena    (1'b1),
    .wea    (ram_rf1_wea),
    .addra  (ram_addr),
    .dia    (ram_datain),
    .doa    (ram_rf1_doa),
    .clkb   (ctrlport_clk),
    .enb    (1'b1),
    .web    (1'b0),
    .addrb  (atr_config_rf[1]),
    .dib    ({RFS_SIZE{1'b0}}),
    .dob    (ram_rf1_dob));

  ram_2port #(
    .DWIDTH     (RFS_SIZE),
    .AWIDTH     (8),
    .RW_MODE    ("READ-FIRST"),
    .RAM_TYPE   ("AUTOMATIC"),
    .OUT_REG    (0)
  ) ram_rf2_i (
    .clka   (ctrlport_clk),
    .ena    (1'b1),
    .wea    (ram_rf2_wea),
    .addra  (ram_addr),
    .dia    (ram_datain),
    .doa    (ram_rf2_doa),
    .clkb   (ctrlport_clk),
    .enb    (1'b1),
    .web    (1'b0),
    .addrb  (atr_config_rf[2]),
    .dib    ({RFS_SIZE{1'b0}}),
    .dob    (ram_rf2_dob));

  ram_2port #(
    .DWIDTH     (RFS_SIZE),
    .AWIDTH     (8),
    .RW_MODE    ("READ-FIRST"),
    .RAM_TYPE   ("AUTOMATIC"),
    .OUT_REG    (0)
  ) ram_rf3_i (
    .clka   (ctrlport_clk),
    .ena    (1'b1),
    .wea    (ram_rf3_wea),
    .addra  (ram_addr),
    .dia    (ram_datain),
    .doa    (ram_rf3_doa),
    .clkb   (ctrlport_clk),
    .enb    (1'b1),
    .web    (1'b0),
    .addrb  (atr_config_rf[3]),
    .dib    ({RFS_SIZE{1'b0}}),
    .dob    (ram_rf3_dob));

endmodule

//XmlParse xml_on
//<regmap name="RF_ATR_REGMAP" readablestrobes="false" generatevhdl="true" ettusguidelines="true">
//  <group name="RF_ATR_REGISTERS">
//    <info>
//      Each channel in the FBX daughterboard has 4 switches in its path. 3 of these are HMC849A 2:1
//      switches and the last one is a PE42442 4:1 switch. The latter, as well as the enable lines for
//      all four switches are not considered time critical controls, and are hence driven by an I/O
//      expander controlled via I2C.
//      This register map describes how to control the behavior of the 3 HMC849A switches' control lines.
//      There are three supported control schemes for these switches:</br>
//      <ul>
//        <li>ATR Disabled - Single persistent state.</li>
//        <li>Classic ATR  - Each channel's switches depend on the transmission state
//                           of the respective channel.</li>
//        <li>DB State     - Each channel's switches depend on the transmission state
//                           of all channels in this radio.</li>
//      </ul>
//    </info>
//
//    <enumeratedtype name="RF_SWITCHES_SIZE_TYPE">
//      <value name="RFS_SIZE" integer="3"/>
//    </enumeratedtype>
//
//    <regtype name="RF_ATR_STATE" size="32">
//      <info>Holds the value for the control lines of each channel's switches
//      for a particular ATR sate</info>
//      <bitfield name="TX_RX_RFS"  range="0" initialvalue="0"/>
//      <bitfield name="RX_RFS"     range="1" initialvalue="0"/>
//      <bitfield name="TDDS"       range="2" initialvalue="0"/>
//    </regtype>
//
//    <register name="RF0_ATR_STATE" typename="RF_ATR_STATE"  offset="0x00" count="256" options="--step 4">
//      <info>
//        Describes switch control behavior for the different ATR states. When @.RF0_ATR_OPTION
//        is set to use the DB states, TX and RX states for RF0-RF3 are
//        combined to create a single vector. This creates 256 different
//        combinations, each with its own register. When @.RF0_ATR_OPTION is set to
//        classic ATR, the first 4 offsets in this register group will be driven
//        in accordance with the state of RF0.
//        CLASSIC ATR MAPPING: Idle[RF0: TX=0, RX=0], RX[RF0: TX=0, RX=1,
//        TX[RF0: TX=1, RX=0], FDX[RF0: TX=1, RX=1]
//      </info>
//    </register>
//
//    <register name="RF1_ATR_STATE" typename="RF_ATR_STATE"  offset="0x400" count="256" options="--step 4">
//      <info>
//        Describes switch control behavior for the different ATR states. When @.RF1_ATR_OPTION
//        is set to use the DB states, TX and RX states for RF0-RF3 are
//        combined to create a single vector. This creates 256 different
//        combinations, each with its own register. When @.RF1_ATR_OPTION is set to
//        classic ATR, the first 4 offsets in this register group will be driven
//        in accordance with the state of RF1.
//        CLASSIC ATR MAPPING: Idle[RF1: TX=0, RX=0], RX[RF1: TX=0, RX=1,
//        TX[RF1: TX=1, RX=0], FDX[RF1: TX=1, RX=1]
//      </info>
//    </register>
//
//    <register name="RF2_ATR_STATE" typename="RF_ATR_STATE"  offset="0x800" count="256" options="--step 4">
//      <info>
//        Describes switch control behavior for the different ATR states. When @.RF2_ATR_OPTION
//        is set to use the DB states, TX and RX states for RF0-RF3 are
//        combined to create a single vector. This creates 256 different
//        combinations, each with its own register. When @.RF2_ATR_OPTION is set to
//        classic ATR, the first 4 offsets in this register group will be driven
//        in accordance with the state of RF2.
//        CLASSIC ATR MAPPING: Idle[RF2: TX=0, RX=0], RX[RF2: TX=0, RX=1,
//        TX[RF2: TX=1, RX=0], FDX[RF2: TX=1, RX=1]
//      </info>
//    </register>
//
//    <register name="RF3_ATR_STATE" typename="RF_ATR_STATE"  offset="0xC00" count="256" options="--step 4">
//      <info>
//        Describes switch control behavior for the different ATR states. When @.RF3_ATR_OPTION
//        is set to use the DB states, TX and RX states for RF0-RF3 are
//        combined to create a single vector. This creates 256 different
//        combinations, each with its own register. When @.RF3_ATR_OPTION is set to
//        classic ATR, the first 4 offsets in this register group will be driven
//        in accordance with the state of RF3.
//        CLASSIC ATR MAPPING: Idle[RF3: TX=0, RX=0], RX[RF3: TX=0, RX=1,
//        TX[RF3: TX=1, RX=0], FDX[RF3: TX=1, RX=1]
//      </info>
//    </register>
//    <register name="ATR_OPTION_REGISTER" offset="0x1000" size="32">
//      <info>
//        Controls whether switch control lines use the TX and RX state of
//        their respective channel (Classic ATR) or the daughterboard state
//        to select which state to use from values set in RF_ATR_STATE registers.
//        For each particular bit:</br>
//            0: Use DB state for ATR</br>
//            1: Classic ATR mode.
//      </info>
//      <bitfield name="RF0_ATR_OPTION" range="0" initialvalue="0">
//        <info>
//          Control ATR scheme for RF0.
//        </info>
//      </bitfield>
//      <bitfield name="RF1_ATR_OPTION" range="1" initialvalue="0">
//        <info>
//          Control ATR scheme for RF1.
//        </info>
//      </bitfield>
//      <bitfield name="RF2_ATR_OPTION" range="2" initialvalue="0">
//        <info>
//          Control ATR scheme for RF2.
//        </info>
//      </bitfield>
//      <bitfield name="RF3_ATR_OPTION" range="3" initialvalue="0">
//        <info>
//          Control ATR scheme for RF3.
//        </info>
//      </bitfield>
//    </register>
//    <register name="RF_ATR_DISABLED" offset="0x1004" size="32">
//      <info>
//        Disable ATR Control. DB state 0 will be reflected regardless of the ATR state.
//      </info>
//      <bitfield name="RF0_ATR_DISABLED" range="0" initialvalue="0"/>
//      <bitfield name="RF1_ATR_DISABLED" range="1" initialvalue="0"/>
//      <bitfield name="RF2_ATR_DISABLED" range="2" initialvalue="0"/>
//      <bitfield name="RF3_ATR_DISABLED" range="3" initialvalue="0"/>
//    </register>
//  </group>
//</regmap>
//XmlParse xml_off

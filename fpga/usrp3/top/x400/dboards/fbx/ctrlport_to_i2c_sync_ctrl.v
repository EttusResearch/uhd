//
// Copyright 2022 Ettus Research, A National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: ctrlport_to_i2c_sync_ctrl
//
// Description:
//
//   This module wraps a I2C WB master and provides a ControlPort interface
//   specific to the SYNC IO expander peripheral.
//   Sync switch control sequence is as follows:
//     1. Setup core and IO expander:
//       a. Write to SETUP_REG
//       b. Poll/read from SETUP_STATUS_REG until bit SETUP_STATUS == 1
//     2. Write sync switch configuration to SYNC_[1-5]_REG and RFS_EN_REG
//     3. Initiate io expander config
//       a. Write to CONFIG_IO_REG
//       b. Poll/read from CONFIG_IO_STATUS_REG until bit CONFIG_IO_STATUS == 1
//   Step 1 only needs to happen once after power up or core reset.
//   Repeat steps 2 and 3 as needed.
//
// Parameters:
//
//   BASE_ADDRESS : Base address for CtrlPort registers.
//   PRESCALE     : Scaling factor to determine clock speed of I2C SCL.
//                  Must be divisible by 5.
//

`default_nettype wire

module ctrlport_to_i2c_sync_ctrl #(
  parameter BASE_ADDRESS = 0,
  parameter [15:0] PRESCALE = 'd500
) (
  //---------------------------------------------------------------
  // ControlPort Slave
  //---------------------------------------------------------------

  input  wire        ctrlport_clk,
  input  wire        ctrlport_rst,

  input  wire        s_ctrlport_req_wr,
  input  wire        s_ctrlport_req_rd,
  input  wire [19:0] s_ctrlport_req_addr,
  input  wire [31:0] s_ctrlport_req_data,

  output reg         s_ctrlport_resp_ack,
  output reg  [ 1:0] s_ctrlport_resp_status  =  2'b0,
  output reg  [31:0] s_ctrlport_resp_data    = 32'b0,

  //---------------------------------------------------------------
  // I2C signals
  //---------------------------------------------------------------
  // i2c clock line
  input  wire scl_pad_i,       // SCL-line input
  output wire scl_pad_o,       // SCL-line output
  output wire scl_pad_en_o,    // SCL-line output enable (active low)

  // i2c data line
  input  wire sda_pad_i,       // SDA-line input
  output wire sda_pad_o,       // SDA-line output
  output wire sda_pad_en_o    // SDA-line output enable (active low)
);

  `include "../../../../lib/rfnoc/core/ctrlport.vh"
  `include "../../../../lib/wishbone/i2c_master.vh"
  `include "./regmap/rf_sync_regmap_utils.vh"



  //---------------------------------------------------------------
  // CtrlPort Interface Regs and Triggers to Initiate I2C
  //---------------------------------------------------------------
  reg [2:0] sync1, sync2, sync3, sync4, sync5;
  reg rfs_en;
  reg setup_trig; // initiate setup
  reg config_io_trig; // configure IO
  reg setup_in_progress;
  reg config_io_in_progress;

  //---------------------------------------------------------------
  // WishBone interface
  //---------------------------------------------------------------
  reg         wb_cyc_i;         // Active bus cycle
  reg         wb_we_i  = 1'b0;  // Write access
  reg  [2:0]  wb_adr_i = 3'b0;
  reg  [7:0]  wb_dat_i = 8'b0;
  wire        wb_ack_o;
  wire [7:0]  wb_dat_o;

  // Check for address to be in range [base_addr..base_addr+32)
  localparam NUM_ADDRESSES = 32;
  wire address_in_range = (s_ctrlport_req_addr >= BASE_ADDRESS) &&
                          (s_ctrlport_req_addr < BASE_ADDRESS + NUM_ADDRESSES);

  // The scaling factor (PRESCALE) of the input clock (ctrlport_clk) to SCL
  // isn't direct but instead the relationship between the input clock and
  // the state machine inside the i2c_master that controls SCL.
  // This state machine has 5 repeated stages per SCL clock cycle so
  // the scaling is divided by a factor of 5. The division here
  // obscures a need for the user to do an extra step to calculate the
  // PRESCALE parameter.
  // For example, if a user wants to have an actual rescaling of 1:500
  // (SCL to ctrlpor_clk), the PRESCALE parameter is divided by 5
  // here so the i2c_master prescale value is set to 100.
  if (PRESCALE % 5 != 0) begin : gen_assertion
    ERROR_PRESCALE_must_be_div_by_5();
  end

  // constants to configure wb i2c master core
  localparam PRESCALE_5    = PRESCALE/5;
  localparam PRER_LO_CONST = PRESCALE_5[7:0];
  localparam PRER_HI_CONST = PRESCALE_5[15:8];

  //constants for interfacing with i2c master core
  localparam I2C_WR          = 1'b0;
  localparam I2C_RD          = 1'b1;
  localparam SYNC_IO_SLV_ADR = 7'h20;

  // CtrlPort Interface
  // configure sync switch IO and trigger I2C transactions
  always @(posedge ctrlport_clk) begin
    // Reset internal registers and responses
    if (ctrlport_rst) begin
      s_ctrlport_resp_ack    <= 1'b0;
      s_ctrlport_resp_status <= 2'b0;
      s_ctrlport_resp_data   <= 32'b0;
      setup_trig     <= 1'b0;
      config_io_trig <= 1'b0;
      sync1  <= 3'b0;
      sync2  <= 3'b0;
      sync3  <= 3'b0;
      sync4  <= 3'b0;
      sync5  <= 3'b0;
      rfs_en <= 1'b0;
    end else begin
      // De-assert trigger strobes
      setup_trig <= 1'b0;
      config_io_trig <= 1'b0;

      // Only ack under certain conditions
      s_ctrlport_resp_ack <= 1'b0;
      if (s_ctrlport_req_wr) begin // Write requests
        // if address is normal ctrlport, immediately ack
        if (address_in_range) begin
          s_ctrlport_resp_ack <= 1'b1;
          s_ctrlport_resp_status <= CTRL_STS_OKAY;
        end
        case (s_ctrlport_req_addr)
          BASE_ADDRESS + SYNC_1_REG: begin
            sync1 <= s_ctrlport_req_data[2:0];
          end

          BASE_ADDRESS + SYNC_2_REG: begin
            sync2 <= s_ctrlport_req_data[2:0];
          end

          BASE_ADDRESS + SYNC_3_REG: begin
            sync3 <= s_ctrlport_req_data[2:0];
          end

          BASE_ADDRESS + SYNC_4_REG: begin
            sync4 <= s_ctrlport_req_data[2:0];
          end

          BASE_ADDRESS + SYNC_5_REG: begin
            sync5 <= s_ctrlport_req_data[2:0];
          end

          BASE_ADDRESS + RFS_EN_REG: begin
            rfs_en <= s_ctrlport_req_data[0];
          end
          //initialize SYNC_IO peripheral
          BASE_ADDRESS + SETUP_REG: begin
            setup_trig <= 1'b1 & ~config_io_in_progress;
          end
          //configure IO
          BASE_ADDRESS + CONFIG_IO_REG: begin
            config_io_trig <= 1'b1 & ~setup_in_progress;
          end
        endcase

      // Read requests
      end else if (s_ctrlport_req_rd) begin
        // Acknowledge by default
        if (address_in_range) begin
          s_ctrlport_resp_ack <= 1'b1;
          s_ctrlport_resp_status <= CTRL_STS_OKAY;
        end
        case (s_ctrlport_req_addr)
          BASE_ADDRESS + SYNC_1_REG: begin
            s_ctrlport_resp_data[2:0] <= sync1;
          end
          BASE_ADDRESS + SYNC_2_REG: begin
            s_ctrlport_resp_data[2:0] <= sync2;
          end
          BASE_ADDRESS + SYNC_3_REG: begin
            s_ctrlport_resp_data[2:0] <= sync3;
          end
          BASE_ADDRESS + SYNC_4_REG: begin
            s_ctrlport_resp_data[2:0] <= sync4;
          end
          BASE_ADDRESS + SYNC_5_REG: begin
            s_ctrlport_resp_data[2:0] <= sync5;
          end
          BASE_ADDRESS + RFS_EN_REG: begin
            s_ctrlport_resp_data[2:0] <= {2'b00, rfs_en};
          end
          BASE_ADDRESS + SETUP_STATUS_REG: begin
            s_ctrlport_resp_data[2:0] <= {2'b00, ~setup_in_progress};
          end
          BASE_ADDRESS + CONFIG_IO_STATUS_REG: begin
            s_ctrlport_resp_data[2:0] <= {2'b00, ~config_io_in_progress};
          end
          // Respond with 0
          default: begin
            if (address_in_range) begin
              s_ctrlport_resp_status <= CTRL_STS_CMDERR;
            end
            s_ctrlport_resp_ack  <= 1'b0;
            s_ctrlport_resp_data <= 32'b0;
          end
        endcase
      // No request
      end else begin
        s_ctrlport_resp_ack <= 1'b0;
      end
    end
  end // always

  // state machine for I2C Write Transactions and WB Controls
  localparam I2C_IDLE      = 0;
  localparam I2C_PRE_LO    = 1;
  localparam I2C_PRE_HI    = 2;
  localparam I2C_EN        = 3;
  localparam I2C_TXR       = 4;
  localparam I2C_CR        = 5;
  localparam I2C_TIP_START = 6;
  localparam I2C_TIP       = 7;
  localparam I2C_TIP_FIN   = 8;

  reg       i2c_tip_fin; // indicates transfer is finished
  reg       i2c_master_en; // indicates i2c master setup and enabled
  reg [3:0] i2c_state;
  wire      wb_cyc_valid; // determines what states wb_cyc_i should be high
  reg       i2c_wr; //trigger an i2c wr cmd
  reg       i2c_core_setup; //trigger i2c master setup
  reg [7:0] wb_txr;
  reg [7:0] wb_cr;

  assign wb_cyc_valid = (i2c_state >= I2C_PRE_LO ) && (i2c_state < I2C_TIP_FIN);

  always @(posedge ctrlport_clk) begin
    if (ctrlport_rst) begin
      i2c_state     <= I2C_IDLE;
      wb_cyc_i      <= 1'b0;
      wb_we_i       <= 1'b0;
      wb_adr_i      <= 3'b0;
      wb_dat_i      <= 8'b0;
      i2c_tip_fin   <= 1'b0;
      i2c_master_en <= 1'b0;
      //more regs need to be set here.
    end
    else begin
      //handle wb_cyc_i for all states. dependent on i2c transaction states
      //and wb_ack_o
      if (wb_cyc_i) begin
        if (wb_ack_o) begin
          wb_cyc_i <= 1'b0;
        end
      end else begin
        if (wb_cyc_valid) begin
          wb_cyc_i <= 1'b1;
        end
      end
      case(i2c_state)
        I2C_IDLE: begin
          i2c_tip_fin <= 1'b0;
          if (i2c_wr) begin
            i2c_state <= I2C_TXR;
          end
          else if (i2c_core_setup) begin
            i2c_state <= I2C_PRE_LO;
          end
        end //I2C_IDLE
        //begin cases to setup i2c master core
        I2C_PRE_LO: begin
          wb_adr_i <= WB_PRER_LO;
          wb_dat_i <= PRER_LO_CONST;
          wb_we_i  <= 1'b1;
          if (wb_ack_o) begin
            i2c_state <= I2C_PRE_HI;
          end
        end
        I2C_PRE_HI: begin
          wb_adr_i <= WB_PRER_HI;
          wb_dat_i <= PRER_HI_CONST;
          wb_we_i  <= 1'b1;
          if (wb_ack_o) begin
            i2c_state <= I2C_EN;
          end
        end
        I2C_EN: begin
          wb_adr_i      <= WB_CTR;
          wb_dat_i      <= WB_CORE_EN;
          wb_we_i       <= 1'b1;
          i2c_master_en <= 1'b1;
          if (wb_ack_o) begin
            i2c_state <= I2C_IDLE;
          end
        end
        //end cases to setup i2c master core
        //begin cases to initiate i2c write transfer
        I2C_TXR: begin
          wb_adr_i <= WB_TXR;
          wb_dat_i <= wb_txr;
          wb_we_i  <= 1'b1;
          if (wb_ack_o) begin
            i2c_state <= I2C_CR;
          end
        end
        I2C_CR: begin
          wb_adr_i <= WB_CR;
          wb_dat_i <= wb_cr;
          wb_we_i  <= 1'b1;
          if (wb_ack_o) begin
            i2c_state <= I2C_TIP_START;
          end
        end
        I2C_TIP_START: begin
          wb_adr_i <= WB_SR;
          wb_dat_i <= 8'b0;
          wb_we_i  <= 1'b0;
          if (wb_ack_o && wb_dat_o[1]) begin
            i2c_state <= I2C_TIP;
          end
        end
        I2C_TIP: begin //TIP transfer in progress
          wb_adr_i <= WB_SR;
          wb_dat_i <= 8'b0;
          wb_we_i  <= 1'b0;
          if (wb_ack_o && !wb_dat_o[1]) begin
            i2c_state   <= I2C_TIP_FIN;
            i2c_tip_fin <= 1'b1; //pulse this as we transition to final state
          end
        end
        I2C_TIP_FIN: begin
          i2c_tip_fin <= 1'b0;
          if (!i2c_wr) begin
            i2c_state   <= I2C_IDLE;
          end
        end
        //end cases to initiate i2c write transfer
        //default case, just go to I2C_IDLE
        default : begin
          i2c_state <= I2C_IDLE;
        end
      endcase
    end
  end //always



  // i2c master top controller state machine
  localparam IDLE            = 0;
  localparam EN              = 1;
  localparam IO_EXP_CONFIG_1 = 2;
  localparam IO_EXP_CONFIG_2 = 3;
  localparam IO_EXP_CONFIG_3 = 4;
  localparam IO_EXP_CONFIG_4 = 5;

  wire [15:0] io_config;
  assign io_config = { rfs_en, sync5, sync4, sync3, sync2, sync1 };
  reg [2:0] master_state;
  always @(posedge ctrlport_clk) begin
    if (ctrlport_rst) begin
      master_state <= IDLE;
      wb_txr <= 8'b0;
      wb_cr  <= 8'b0;
      i2c_wr <= 1'b0;
      setup_in_progress     <= 1'b0;
      config_io_in_progress <= 1'b0;
      i2c_core_setup        <= 1'b0;
    end
    else begin
      case(master_state)
        IDLE: begin
          i2c_wr <= 1'b0;
          setup_in_progress     <= 1'b0;
          config_io_in_progress <= 1'b0;
          i2c_core_setup        <= 1'b0;
          if (setup_trig) begin
            setup_in_progress     <= 1'b1;
            master_state <= EN;
          end
          else if (config_io_trig) begin
            config_io_in_progress <= 1'b1;
            master_state <= IO_EXP_CONFIG_1;
          end
        end
        EN: begin //all the prescale and enable setup happens in i2c_state machine
          i2c_core_setup <= 1'b1; //triggers setup sequence in i2c state machine.
          i2c_wr         <= 1'b0;
          if (i2c_master_en) begin //indicates setup sequence finished
            master_state <= IO_EXP_CONFIG_1;
          end
        end
        IO_EXP_CONFIG_1: begin //first stage of setting up IO EXP with startup configurations
          wb_txr <= { SYNC_IO_SLV_ADR, I2C_WR};
          wb_cr  <= CR_START_AND_WRITE;
          if (i2c_tip_fin) begin //when transfer is done, i2c_wr is disabled
            i2c_wr <= 1'b0;
            master_state <= IO_EXP_CONFIG_2;
          end else begin
            i2c_wr <= 1'b1;
          end
        end
        IO_EXP_CONFIG_2: begin
          if (setup_in_progress) begin
            wb_txr <= IO_EXP_CONFIG0_REG; //CONFIG0 reg is what we set to configure I/O as output
          end else if (config_io_in_progress) begin
            wb_txr <= IO_EXP_OUTPUT_PORT0_REG;
          end
          wb_cr <= CR_WRITE;
          if (i2c_tip_fin) begin //when transfer is done, i2c_wr is disabled
            i2c_wr <= 1'b0;
            master_state <= IO_EXP_CONFIG_3;
          end else begin
            i2c_wr <= 1'b1;
          end
        end
        IO_EXP_CONFIG_3: begin
          if (setup_in_progress) begin
            wb_txr <= 1'b0;
          end else if (config_io_in_progress) begin
            wb_txr <= io_config[7:0];
          end
          wb_cr <= CR_WRITE;
          if (i2c_tip_fin) begin //when transfer is done, i2c_wr is disabled
            i2c_wr <= 1'b0;
            master_state <= IO_EXP_CONFIG_4;
          end else begin
            i2c_wr <= 1'b1;
          end
        end
        IO_EXP_CONFIG_4: begin
          if (setup_in_progress) begin
            wb_txr <= 1'b0;
          end else if (config_io_in_progress) begin
            wb_txr <= io_config[15:8];
          end
          wb_cr <= CR_WRITE_AND_STOP;
          if (i2c_tip_fin) begin //when transfer is done, i2c_wr is disabled
            i2c_wr <= 1'b0;
            setup_in_progress <= 1'b0;
            config_io_in_progress <= 1'b0;
            master_state <= IDLE;
          end else begin
            i2c_wr <= 1'b1;
          end
        end
        default : begin
          master_state <= IDLE;
        end
      endcase
    end
  end //always

  //wishbone-based i2c core
  i2c_master_top #(
    .ARST_LVL(1)
  ) i2c_master (
    .wb_clk_i(ctrlport_clk),
    .wb_rst_i(ctrlport_rst),
    .arst_i(1'b0),
    .wb_adr_i(wb_adr_i),
    .wb_dat_i(wb_dat_i),
    .wb_dat_o(wb_dat_o),
    .wb_we_i(wb_we_i),
    .wb_stb_i(wb_cyc_i),
    .wb_cyc_i(wb_cyc_i),
    .wb_ack_o(wb_ack_o),
    .wb_inta_o(),
    .scl_pad_i(scl_pad_i),
    .scl_pad_o(scl_pad_o),
    .scl_padoen_o(scl_pad_en_o),
    .sda_pad_i(sda_pad_i),
    .sda_pad_o(sda_pad_o),
    .sda_padoen_o(sda_pad_en_o)
  );

endmodule


//XmlParse xml_on
//<regmap name="RF_SYNC_REGMAP" readablestrobes="false" generatevhdl="true" ettusguidelines="true">
//  <group name="RF_SYNC_REGISTERS">
//    <info>
//      Each channel in the FBX daughterboard has 4 switches in its path. 3 of these are HMC849A 2:1
//      switches and the last one is a PE42442 4:1 switch. The latter, as well as the enable lines for
//      all four switches are not considered time critical controls, and are hence driven by a
//      TCA6416A I/O expander controlled via I2C.
//      This register map controls that I/O expander. The first 6 registers in
//      this space set the values of the 5 3-pin sync switches and then an
//      RFS enable bit. The 2 additional registers trigger a init sequence for
//      the expander peripheral and configuring the I/O with the contents of
//      the value registers both with a series of I2C commands.
//    </info>
//    <regtype name="SYNC_SWITCH" size="32" attributes="Readable|Writable">
//      <info>
//        Sets I/O to control Sync Switch.
//      </info>
//      <bitfield name="CONTROL_PIN_V1" range="0" initialvalue="0"/>
//      <bitfield name="CONTROL_PIN_V2" range="1" initialvalue="0"/>
//      <bitfield name="CONTROL_PIN_V3" range="2" initialvalue="0"/>
//    </regtype>
//    <register name="SYNC_1_REG" offset="0x00" typename="SYNC_SWITCH">
//      <info>
//        Sets I/O state to control Sync Switch 1.
//      </info>
//    </register>
//    <register name="SYNC_2_REG" offset="0x04" typename="SYNC_SWITCH">
//      <info>
//        Sets I/O state to control Sync Switch 2.
//      </info>
//    </register>
//    <register name="SYNC_3_REG" offset="0x08" typename="SYNC_SWITCH">
//      <info>
//        Sets I/O state to control Sync Switch 3.
//      </info>
//    </register>
//    <register name="SYNC_4_REG" offset="0x0C" typename="SYNC_SWITCH">
//      <info>
//        Sets I/O state to control Sync Switch 4.
//      </info>
//    </register>
//    <register name="SYNC_5_REG" offset="0x10" typename="SYNC_SWITCH">
//      <info>
//        Sets I/O state to control Sync Switch 5.
//      </info>
//    </register>
//    <register name="RFS_EN_REG" offset="0x014" size="32" attributes="Readable|Writable">
//      <info>
//        Sets I/O state to control RFS Enable.
//      </info>
//      <bitfield name="RFS_EN_PIN" range="0" initialvalue="0" />
//    </register>
//    <register name="SETUP_REG" offset="0x18" size="32" attributes="Writable">
//      <info>
//        Write to this register to trigger initialization sequence of
//        I2C Master core and TCA6416A I/O expander peripheral.
//      </info>
//      <bitfield name="SETUP_TRIG" range="0" initialvalue="0"/>
//    </register>
//    <register name="SETUP_STATUS_REG" offset="0x18" size="32" attributes="Readable">
//      <info>
//        Read from this register to check the status of setup.
//      </info>
//      <bitfield name="SETUP_STATUS" range="0" initialvalue="0"/>
//    </register>
//    <register name="CONFIG_IO_REG" offset="0x1C" size="32" attributes="Writable">
//      <info>
//        Write to this register to trigger I/O configuration I2C sequence of
//        TCA6416A I/O expander peripheral.
//        Read from this register to check the status of reconfiguring the setup.
//      </info>
//      <bitfield name="CONFIG_IO_TRIG" range="0" initialvalue="0"/>
//    </register>
//    <register name="CONFIG_IO_STATUS_REG" offset="0x1C" size="32" attributes="Readable">
//      <info>
//        Read from this register to check the status of reconfiguring the setup.
//      </info>
//      <bitfield name="CONFIG_IO_STATUS" range="0" initialvalue="0"/>
//    </register>
//
//    <enumeratedtype name="IO_EXPANDER_REGISTER_ADRS">
//      <info>
//        This is not a register spaces for this ctrlport interface but
//        instead register addresses on the TCA6416A. Therefore, we will just
//        define an enum with these addresses so there is no clash with the
//        above ctrlport regmap.
//      </info>
//      <value name="IO_EXP_INPUT_PORT0_REG"   integer="0"/>
//      <value name="IO_EXP_INPUT_PORT1_REG"   integer="1"/>
//      <value name="IO_EXP_OUTPUT_PORT0_REG"  integer="2"/>
//      <value name="IO_EXP_OUTPUT_PORT1_REG"  integer="3"/>
//      <value name="IO_EXP_POLARITY_INV0_REG" integer="4"/>
//      <value name="IO_EXP_POLARITY_INV1_REG" integer="5"/>
//      <value name="IO_EXP_CONFIG0_REG"       integer="6"/>
//      <value name="IO_EXP_CONFIG1_REG"       integer="7"/>
//    </enumeratedtype>
//  </group>
//</regmap>

//XmlParse xml_off


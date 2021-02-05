//
// Copyright 2021 Ettus Research, A National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: lo_control
//
// Description:
//   Implements control over signals interacting with LMX2572 chips on the
//   daughterboard. This includes a CtrlPort based control over a SPI master
//   that distributes transactions across the SPI buses of the LMX2572, as
//   well as the capability to synchronously generate pulses to their SYNC pins.
//

`default_nettype none

module lo_control #(
  parameter [19:0]  BASE_ADDRESS = 0,
  parameter [19:0]  SIZE_ADDRESS = 0
) (
  // Request
  input  wire        s_ctrlport_req_wr,
  input  wire        s_ctrlport_req_rd,
  input  wire [19:0] s_ctrlport_req_addr,
  input  wire [31:0] s_ctrlport_req_data,
  // Response
  output reg         s_ctrlport_resp_ack,
  output reg  [ 1:0] s_ctrlport_resp_status,
  output reg  [31:0] s_ctrlport_resp_data,

  //reg clk domain
  input  wire        ctrlport_clk,
  input  wire        ctrlport_rst,

  // LO SPI for LMX2572
  input  wire [7:0] miso,
  output reg  [7:0] ss = {8{1'b1}},
  output wire       sclk,
  output reg        mosi = 1'b0,

  // Incoming SYNC
  input wire mb_synth_sync,

  // SYNC for LMX2572
  output wire tx0_lo1_sync,
  output wire tx0_lo2_sync,
  output wire tx1_lo1_sync,
  output wire tx1_lo2_sync,
  output wire rx0_lo1_sync,
  output wire rx0_lo2_sync,
  output wire rx1_lo1_sync,
  output wire rx1_lo2_sync
);

  `include "../regmap/lo_control_regmap_utils.vh"
  `include "../../../../../../lib/rfnoc/core/ctrlport.vh"

  //---------------------------------------------------------------
  // register bitfields
  //---------------------------------------------------------------

  reg                           spi_start;
  reg [LO_SELECT_SIZE-1:0]      spi_cs;
  reg                           spi_rd;
  reg [LO_SPI_WT_ADDR_SIZE-1:0] spi_addr;
  reg [LO_SPI_WT_DATA_SIZE-1:0] spi_data;
  reg                           spi_data_valid;
  reg                           spi_ready = 1'b1;
  reg [LO_SPI_RD_DATA_SIZE-1:0] spi_rd_data;
  reg [LO_CHIP_SELECT_SIZE-1:0] lo_sync_reg = 8'b0;
  reg                           bypass_sync = 1'b0;

  //---------------------------------------------------------------
  // Handling of CtrlPort
  //---------------------------------------------------------------
  wire address_in_range = (s_ctrlport_req_addr >= BASE_ADDRESS) && (s_ctrlport_req_addr < BASE_ADDRESS + SIZE_ADDRESS);

  always @(posedge ctrlport_clk) begin
    // reset internal registers and responses
    if (ctrlport_rst) begin
      spi_start       <= 1'b0;
      spi_cs          <= 3'b0;
      spi_rd          <= 1'b0;
      spi_addr        <= {LO_SPI_WT_ADDR_SIZE{1'b0}};
      spi_data        <= {LO_SPI_WT_DATA_SIZE{1'b0}};
      lo_sync_reg     <= 8'b0;
      bypass_sync     <= 1'b0;

      s_ctrlport_resp_ack     <= 1'b0;
      s_ctrlport_resp_data    <= {32{1'bx}};
      s_ctrlport_resp_status  <= CTRL_STS_OKAY;

    end else begin

      //send only a pulse
      spi_start <= 1'b0;

      //pulse sync lines for a maximum of one cycle
      lo_sync_reg <= 8'b0;

      // write requests
      if (s_ctrlport_req_wr) begin
        // always issue an ack and no data
        s_ctrlport_resp_ack     <= 1'b1;
        s_ctrlport_resp_data    <= {32{1'bx}};
        s_ctrlport_resp_status  <= CTRL_STS_OKAY;

        case (s_ctrlport_req_addr)
          BASE_ADDRESS + LO_SPI_SETUP: begin
            spi_start <= s_ctrlport_req_data[LO_SPI_START_TRANSACTION];
            spi_cs    <= s_ctrlport_req_data[LO_SELECT_MSB : LO_SELECT];
            spi_rd    <= s_ctrlport_req_data[LO_SPI_RD];
            spi_addr  <= s_ctrlport_req_data[LO_SPI_WT_ADDR_MSB : LO_SPI_WT_ADDR];
            spi_data  <= s_ctrlport_req_data[LO_SPI_WT_DATA_MSB : LO_SPI_WT_DATA];
          end

          BASE_ADDRESS + LO_PULSE_SYNC: begin
            bypass_sync          <= s_ctrlport_req_data[BYPASS_SYNC_REGISTER];
            lo_sync_reg[TX0_LO1] <= s_ctrlport_req_data[PULSE_TX0_LO1_SYNC];
            lo_sync_reg[TX0_LO2] <= s_ctrlport_req_data[PULSE_TX0_LO2_SYNC];
            lo_sync_reg[TX1_LO1] <= s_ctrlport_req_data[PULSE_TX1_LO1_SYNC];
            lo_sync_reg[TX1_LO2] <= s_ctrlport_req_data[PULSE_TX1_LO2_SYNC];
            lo_sync_reg[RX0_LO1] <= s_ctrlport_req_data[PULSE_RX0_LO1_SYNC];
            lo_sync_reg[RX0_LO2] <= s_ctrlport_req_data[PULSE_RX0_LO2_SYNC];
            lo_sync_reg[RX1_LO1] <= s_ctrlport_req_data[PULSE_RX1_LO1_SYNC];
            lo_sync_reg[RX1_LO2] <= s_ctrlport_req_data[PULSE_RX1_LO2_SYNC];
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

      // read requests
      end else if (s_ctrlport_req_rd) begin
        // default assumption: valid request
        s_ctrlport_resp_ack     <= 1'b1;
        s_ctrlport_resp_status  <= CTRL_STS_OKAY;
        s_ctrlport_resp_data    <= {32{1'b0}};

        case (s_ctrlport_req_addr)
          BASE_ADDRESS + LO_SPI_STATUS: begin //same address as *_status form regmap
            s_ctrlport_resp_data[LO_SPI_DATA_VALID]                   <= spi_data_valid;
            s_ctrlport_resp_data[LO_SELECT_MSB : LO_SELECT]           <= spi_cs;
            s_ctrlport_resp_data[LO_SPI_READY]                        <= spi_ready;
            s_ctrlport_resp_data[LO_SPI_RD_ADDR_MSB : LO_SPI_RD_ADDR] <= spi_addr;
            s_ctrlport_resp_data[LO_SPI_RD_DATA_MSB : LO_SPI_RD_DATA] <= spi_rd_data;
          end

          // error on undefined address
          default: begin
            s_ctrlport_resp_data <= {32{1'b0}};
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

  // Spi_top controls
  reg [4:0]   wb_adr_i;
  reg         wb_cyc_i;
  reg [31:0]  wb_dat_i;
  reg         wb_we_i;

  // Spi_top outputs
  wire        wb_ack_o;
  wire [31:0] wb_dat_o;
  wire        wb_int_o;

  wire [15:0] ss_pad_o;
  wire        mosi_pad_o;
  wire        miso_pad_i;

  // There is a hold requirement of 10ns on the output path.
  // To ease meeting this requirement without to much routing added to the lines
  // these registers shift the output by 10ns (half 50 MHz period).
  always @(negedge ctrlport_clk) begin
    if (ctrlport_rst) begin
      ss   <= {8{1'b1}};
      mosi <= 1'b0;
    end
    else begin
      ss   <= ss_pad_o[LO_CHIP_SELECT_SIZE-1:0];
      mosi <= mosi_pad_o;
    end
  end

  assign miso_pad_i = ( ~ss[TX0_LO1] ) ? miso[TX0_LO1] :
                      ( ~ss[TX0_LO2] ) ? miso[TX0_LO2] :
                      ( ~ss[TX1_LO1] ) ? miso[TX1_LO1] :
                      ( ~ss[TX1_LO2] ) ? miso[TX1_LO2] :
                      ( ~ss[RX0_LO1] ) ? miso[RX0_LO1] :
                      ( ~ss[RX0_LO2] ) ? miso[RX0_LO2] :
                      ( ~ss[RX1_LO1] ) ? miso[RX1_LO1] :
                      ( ~ss[RX1_LO2] ) ? miso[RX1_LO2] :
                      1'b0;

  // Import offsets and functions to interact with spi_top
  `include "utils/spi_control_utils.vh"

  // The sclk signal generated by this spi engine will be constrained to be 1/4 of the
  // ctrl port frequency. The effective frequency of the clock output of spi_top is determined
  // by the equation wb_clk_i/((CLOCK_DIVIDER_VALUE+1)*2), so the value required for the clock
  // divider register is 1.
  localparam CLOCK_DIVIDER_VALUE = 32'h1;

  // Base Configuration for the control register. To start a transaction
  // modify this value to include the GO_BUSY bit set to high. The mapping of the
  // macro goes as follows:
  `define CONTROL_DATA(GO_BUSY) { 18'b0,    /* Reserved                   */ \
                                  1'b1,     /* Automatic SS(13)           */ \
                                  1'b1,     /* Interrupt Enable           */ \
                                  1'b0,     /* LSB                        */ \
                                  1'b1,     /* TX_NEG (10)                */ \
                                  1'b0,     /* RX_NEG (9)                 */ \
                                  GO_BUSY,  /* GO_BUSY (8)                */ \
                                  1'b0,     /* Reserved (7)               */ \
                                  7'd24}    /* Length of spi transaction  */

  // Declare the different state-machine states.
  localparam RESET_STATE          = 0;
  localparam CONFIG_DIVIDER       = 1;
  localparam INIT_CONTROL         = 2;
  localparam IDLE                 = 3;
  localparam LOAD_CS              = 4;
  localparam LOAD_DATA            = 5;
  localparam SEND_TRANSACTION     = 6;
  localparam WAIT_FOR_COMPLETION  = 7;
  localparam RETRIEVE_DATA        = 8;

  // FSM state variable
  reg [3:0] spi_state = RESET_STATE;

  always @(posedge ctrlport_clk) begin
    if (ctrlport_rst) begin
      // SPI_STATUS for CtrlPort
      spi_state       <= RESET_STATE;
      spi_ready       <= 1'b1;
      spi_data_valid  <= 1'b0;
      spi_rd_data     <= {LO_SPI_RD_DATA_SIZE{1'b0}};

      // SPI_TOP Bus access control.
      wb_we_i         <= 1'b0;
      wb_cyc_i        <= 1'b0;
      wb_adr_i        <= 5'h00;
      wb_dat_i        <= 32'h00;
    end else begin
      case (spi_state)
        RESET_STATE:
          if (spi_start) begin
            spi_state <= CONFIG_DIVIDER;
          end

        // keep driving a write to the CLOCK_DIVIDER Register until access is acknowledged.
        CONFIG_DIVIDER:
          if (wb_ack_o) begin
            spi_state <= INIT_CONTROL;
            wb_we_i   <= 1'b0;
            wb_cyc_i  <= 1'b0;
          end else begin
            wb_we_i   <=  1'b1;
            wb_cyc_i  <=  1'b1;
            wb_adr_i  <=  CLOCK_DIVIDER_REG; // CLOCK DIVIDER register offset
            wb_dat_i  <=  CLOCK_DIVIDER_VALUE;
          end

        // keep driving a write to the CONTROL Register until access is acknowledged.
        INIT_CONTROL:
          if (wb_ack_o) begin
            spi_state <= LOAD_CS;
            wb_we_i   <= 1'b0;
            wb_cyc_i  <= 1'b0;
          end else begin
            wb_we_i   <=  1'b1;
            wb_cyc_i  <=  1'b1;
            wb_adr_i  <=  CONTROL_REG;  //CONTROL register offset
            wb_dat_i  <=  `CONTROL_DATA(1'b0); //Write control register value with no GO_BUSY
          end

        // Wait for CtrlPort operation to trigger a SPI transaction.
        IDLE :
          if (spi_start) begin
            spi_state       <= LOAD_CS;
            spi_ready       <= 1'b0;
            spi_data_valid  <= 1'b0;
            // Clear data to be written next. This will make it so that the next state(LOAD_CS)
            // will only have to set the bits of the pertinent SS lines.
            wb_dat_i        <= 32'h00;
          end else begin
            spi_ready       <= 1'b1;
            wb_dat_i        <= 32'h00;
          end

        // keep driving a write to the SLAVE SELECT Register until access is acknowledged.
        LOAD_CS :
          if (wb_ack_o) begin
            spi_state         <= LOAD_DATA;
            wb_we_i           <= 1'b0;
            wb_cyc_i          <= 1'b0;
          end else begin
            wb_we_i           <=  1'b1;
            wb_cyc_i          <=  1'b1;
            wb_adr_i          <=  SS_REG; // SS Register offset.
            wb_dat_i          <=  set_ss_bit(spi_cs);   // Assign single bit.
          end

        // keep driving a write to the DATA TRANSMIT Register until access is acknowledged.
        // This includes the combination of CMD+ADDR+DATA to be driven on the MOSI lines.
        LOAD_DATA :
          if (wb_ack_o) begin
            spi_state <= SEND_TRANSACTION;
            wb_we_i   <= 1'b0;
            wb_cyc_i  <= 1'b0;
          end else begin
            wb_we_i   <=  1'b1;
            wb_cyc_i  <=  1'b1;
            wb_adr_i  <=  TX_DATA_REG;   // Data Transmit register offset.
            wb_dat_i  <=  {
                            8'b0,
                            spi_rd,
                            spi_addr,
                            spi_data
                          };
          end

        // Per indication in the SPI_TOP documentation, we write the same configuration as before to the
        // CONTROL register, with the addition of the GO bit.
        SEND_TRANSACTION :
          if (wb_ack_o) begin
            spi_state <= WAIT_FOR_COMPLETION;
            wb_we_i   <= 1'b0;
            wb_cyc_i  <= 1'b0;
          end else begin
            wb_we_i   <=  1'b1;
            wb_cyc_i  <=  1'b1;
            wb_adr_i  <=  CONTROL_REG;
            wb_dat_i  <=  `CONTROL_DATA(1'b1); //Write control register value with GO_BUSY set.
          end

        // This state waits until SPI access is complete
        WAIT_FOR_COMPLETION:
          if (wb_int_o) begin
            if (spi_rd) begin     // If reading, do an extra step
              spi_state <= RETRIEVE_DATA;
            end else begin
              spi_state <= IDLE;    // If not reading, wait for next transaction
            end
            wb_we_i  <= 1'b0;
            wb_cyc_i <= 1'b0;
          end else begin            // Keep polling CONTROL register.
            wb_we_i  <= 1'b0;
            wb_cyc_i <= 1'b0;
          end

        RETRIEVE_DATA:
          if (wb_ack_o) begin
            spi_state       <= IDLE;                               // as soon as data is available, record it and go back
            spi_rd_data     <= wb_dat_o[LO_SPI_RD_DATA_SIZE-1:0];  // to idle.
            spi_data_valid  <= 1'b1;
            wb_we_i         <= 1'b0;
            wb_cyc_i        <= 1'b0;
          end else begin
            wb_we_i         <=  1'b0;                              // Drive bus access.
            wb_cyc_i        <=  1'b1;
            wb_adr_i        <=  RX_DATA_REG;                        // DATA RETRIEVE Register offset
            wb_dat_i        <=  {LO_SPI_WT_DATA_SIZE{1'b0}};
          end

      endcase
    end
  end

  spi_top spi_top_i (
    .wb_clk_i    (ctrlport_clk),
    .wb_rst_i    (ctrlport_rst),
    .wb_adr_i    (wb_adr_i),
    .wb_dat_i    (wb_dat_i),
    .wb_dat_o    (wb_dat_o),
    .wb_sel_i    (4'hF),
    .wb_we_i     (wb_we_i),
    .wb_stb_i    (wb_cyc_i),
    .wb_cyc_i    (wb_cyc_i),
    .wb_ack_o    (wb_ack_o),
    .wb_err_o    (),
    .wb_int_o    (wb_int_o),
    .ss_pad_o    (ss_pad_o),
    .sclk_pad_o  (sclk),
    .mosi_pad_o  (mosi_pad_o),
    .miso_pad_i  (miso_pad_i));

  reg mb_sync_reg = 1'b0;

  // align incoming signal to clock
  always @(posedge ctrlport_clk) begin
    mb_sync_reg <= mb_synth_sync;
  end

  // Select between bypassing into input signal or registered pulse
  assign tx0_lo1_sync = bypass_sync ? mb_sync_reg : lo_sync_reg[TX0_LO1];
  assign tx0_lo2_sync = bypass_sync ? mb_sync_reg : lo_sync_reg[TX0_LO2];
  assign tx1_lo1_sync = bypass_sync ? mb_sync_reg : lo_sync_reg[TX1_LO1];
  assign tx1_lo2_sync = bypass_sync ? mb_sync_reg : lo_sync_reg[TX1_LO2];
  assign rx0_lo1_sync = bypass_sync ? mb_sync_reg : lo_sync_reg[RX0_LO1];
  assign rx0_lo2_sync = bypass_sync ? mb_sync_reg : lo_sync_reg[RX0_LO2];
  assign rx1_lo1_sync = bypass_sync ? mb_sync_reg : lo_sync_reg[RX1_LO1];
  assign rx1_lo2_sync = bypass_sync ? mb_sync_reg : lo_sync_reg[RX1_LO2];

endmodule

`default_nettype wire

//XmlParse xml_on
//<regmap name="LO_CONTROL_REGMAP" readablestrobes="false" generatevhdl="true" ettusguidelines="true">
//  <group name="LO_SPI_REGISTERS">
//    <info>
//      Controls the SPI transaction to the LMX2572
//    </info>
//    <enumeratedtype name="LO_CHIP_SELECT">
//      <value name="TX0_LO1" integer="0"/>
//      <value name="TX0_LO2" integer="1"/>
//      <value name="TX1_LO1" integer="2"/>
//      <value name="TX1_LO2" integer="3"/>
//      <value name="RX0_LO1" integer="4"/>
//      <value name="RX0_LO2" integer="5"/>
//      <value name="RX1_LO1" integer="6"/>
//      <value name="RX1_LO2" integer="7"/>
//    </enumeratedtype>
//    <register name="LO_SPI_SETUP" size="32" offset="0x00" attributes="Writable">
//      <info>
//         This register sets up the SPI transaction to read/write to/from to the LMX2572.
//      </info>
//     <bitfield name="LO_SPI_START_TRANSACTION" range="28" initialvalue="0" attributes="Strobe">
//       <info>
//          Strobe this bit high to start the SPI transaction with the bitfields below
//       </info>
//     </bitfield>
//     <bitfield name="LO_SELECT" range="26..24" type="LO_CHIP_SELECT" initialvalue="TX0_LO1" attributes="Strobe">
//       <info>
//          Sets the CS to the selected LO. The CS will assert until after @.LO_SPI_START_TRANSACTION has been asserted.
//       </info>
//     </bitfield>
//     <bitfield name="LO_SPI_RD" range="23" initialvalue="0">
//       <info>
//          Set this bit to '1' to read from the LMX2572. Set this bit to '0' to write to the LMX2572.
//       </info>
//     </bitfield>
//     <bitfield name="LO_SPI_WT_ADDR" range="22..16" initialvalue="0">
//       <info>
//          7 bit address of the LMX2572
//       </info>
//     </bitfield>
//     <bitfield name="LO_SPI_WT_DATA" range="15..0" initialvalue="0">
//       <info>
//          Write Data to the LMX2572
//       </info>
//     </bitfield>
//    </register>
//
//    <register name="LO_SPI_STATUS" size="32" offset="0x00" attributes="Readable">
//      <info>
//         This register returns the SPI master status, and also returns the read data from the LMX2572
//      </info>
//      <bitfield name="LO_SPI_DATA_VALID" range="31" initialvalue="0">
//        <info>
//           Returns '1' when a read SPI transaction is complete. This bit will remain high until a new SPI transaction has started.
//           i.e. @.LO_SPI_START_TRANSACTION is strobed. Poll this when expecting data from a read transaction.
//        </info>
//      </bitfield>
//      <bitfield name="LO_SPI_READY" range="30" initialvalue="0">
//        <info>
//           If this bit returns '1' then LMX2572 is ready for transaction. If it returns '0' then it is busy with a previous SPI transaction.
//           Poll this bit before starting a SPI transaction.
//        </info>
//      </bitfield>
//     <bitfield name="LO_SELECT_STATUS" range="26..24" type="LO_CHIP_SELECT" initialvalue="TX0_LO1">
//       <info>
//          Returns the current selected CS. This bitfield will return the value written to @.LO_SELECT bitfield in the @.LO_SPI_SETUP reg.
//       </info>
//     </bitfield>
//      <bitfield name="LO_SPI_RD_ADDR" range="22..16" initialvalue="0">
//        <info>
//           Returns the address of the current SPI address setup
//        </info>
//      </bitfield>
//      <bitfield name="LO_SPI_RD_DATA" range="15..0" initialvalue="0">
//        <info>
//           Returns the data of the SPI read. This bitfield will return 0x0000 until @.LO_SPI_DATA_VALID is true. This bit field will maintain it's
//           read value until a new SPI transaction has started. i.e. @.LO_SPI_START_TRANSACTION is strobed.
//        </info>
//      </bitfield>
//    </register>
//  </group>
//  <group name="LO_SYNC_REGS" offset="0x04">
//    <info>
//      Contains registers that control the logic lines in charge of synchronization
//    </info>
//    <register name="LO_PULSE_SYNC" size="32" offset="0x00" attributes="Writable">
//      <info>
//        Controls pulses driven to the SYNC pins of the LMX2572 chips
//      </info>
//      <bitfield name="PULSE_TX0_LO1_SYNC" range="0" initialvalue="0" attributes="Strobe">
//        <info>
//           Creates a single cycle pulse on the TX0_LO1_SYNC line.
//        </info>
//      </bitfield>
//      <bitfield name="PULSE_TX0_LO2_SYNC" range="1" initialvalue="0" attributes="Strobe">
//        <info>
//           Creates a single cycle pulse on the TX0_LO2_SYNC line.
//        </info>
//      </bitfield>
//      <bitfield name="PULSE_TX1_LO1_SYNC" range="2" initialvalue="0" attributes="Strobe">
//        <info>
//           Creates a single cycle pulse on the TX1_LO1_SYNC line.
//        </info>
//      </bitfield>
//      <bitfield name="PULSE_TX1_LO2_SYNC" range="3" initialvalue="0" attributes="Strobe">
//        <info>
//           Creates a single cycle pulse on the TX1_LO2_SYNC line.
//        </info>
//      </bitfield>
//      <bitfield name="PULSE_RX0_LO1_SYNC" range="4" initialvalue="0" attributes="Strobe">
//        <info>
//           Creates a single cycle pulse on the RX0_LO1_SYNC line.
//        </info>
//      </bitfield>
//      <bitfield name="PULSE_RX0_LO2_SYNC" range="5" initialvalue="0" attributes="Strobe">
//        <info>
//           Creates a single cycle pulse on the RX0_LO2_SYNC line.
//        </info>
//      </bitfield>
//      <bitfield name="PULSE_RX1_LO1_SYNC" range="6" initialvalue="0" attributes="Strobe">
//        <info>
//           Creates a single cycle pulse on the RX1_LO1_SYNC line.
//        </info>
//      </bitfield>
//      <bitfield name="PULSE_RX1_LO2_SYNC" range="7" initialvalue="0" attributes="Strobe">
//        <info>
//           Creates a single cycle pulse on the RX1_LO2_SYNC line.
//        </info>
//      </bitfield>
//      <bitfield name="BYPASS_SYNC_REGISTER" range="8" initialvalue="0">
//        <info>
//           Setting this bit to '1' will ignore writes to the PULSE_X_SYNC fields and allow
//           a buffered input SYNC pulse to be driven out instead.
//        </info>
//      </bitfield>
//    </register>
//  </group>
//</regmap>
//XmlParse xml_off

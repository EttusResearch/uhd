//
// Copyright 2021 Ettus Research, A National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0
//
// Module: x4xx_global_regs
//
// Description:
//
//   This module contains the motherboard registers for the USRP.
//
// Parameters:
//
//   REG_BASE        : Base address to use for the registers
//   CHDR_CLK_RATE   : Rate of rfnoc_chdr_clk in Hz
//   CHDR_W          : CHDR protocol width
//   RFNOC_PROTOVER  : RFNoC protocol version (major in most-significant byte,
//                     Minor is least significant byte).
//   NUM_TIMEKEEPERS : Number of timekeeper modules
//   PCIE_PRESENT    : Indicates if PCIe is present in this image
//

`default_nettype none


module x4xx_global_regs #(
  parameter REG_BASE        = 0,
  parameter REG_SIZE        = 'hC00,
  parameter CHDR_CLK_RATE   = 200000000,
  parameter CHDR_W          = 64,
  parameter RFNOC_PROTOVER  = {8'd1, 8'd0},
  parameter NUM_TIMEKEEPERS = 32'd1,
  parameter PCIE_PRESENT    = 0
) (
  // Slave ctrlport interface
  input  wire        s_ctrlport_clk,
  input  wire        s_ctrlport_rst,
  input  wire        s_ctrlport_req_wr,
  input  wire        s_ctrlport_req_rd,
  input  wire [19:0] s_ctrlport_req_addr,
  input  wire [31:0] s_ctrlport_req_data,
  output reg         s_ctrlport_resp_ack    = 1'b0,
  output reg  [ 1:0] s_ctrlport_resp_status = 2'b00,
  output reg  [31:0] s_ctrlport_resp_data   = {32 {1'bX}},

  // RFNoC CHDR clock/reset
  input wire rfnoc_chdr_clk,
  input wire rfnoc_chdr_rst,

  // PPS and clock control signals (domain: s_ctrlport_clk)
  output wire [ 1:0] pps_select,
  output wire [ 1:0] trig_io_select,
  output reg         pll_sync_trigger  = 1'b0,
  output reg  [ 7:0] pll_sync_delay    = 8'b0,
  input  wire        pll_sync_done,
  output reg  [ 7:0] pps_brc_delay     = 8'b0,
  output reg  [25:0] pps_prc_delay     = 26'b0,
  output reg  [ 4:0] prc_rc0_divider   = 5'b0,
  output reg  [ 4:0] prc_rc1_divider   = 5'b0,
  output reg         pps_rc_enabled    = 1'b0,

  // Misc control and status signals (domain: s_ctrlport_clk)
  input  wire [31:0] qsfp_port_0_0_info,
  input  wire [31:0] qsfp_port_0_1_info,
  input  wire [31:0] qsfp_port_0_2_info,
  input  wire [31:0] qsfp_port_0_3_info,
  input  wire [31:0] qsfp_port_1_0_info,
  input  wire [31:0] qsfp_port_1_1_info,
  input  wire [31:0] qsfp_port_1_2_info,
  input  wire [31:0] qsfp_port_1_3_info,
  output reg         mfg_test_en_fabric_clk   = 1'b0,
  output reg         mfg_test_en_gty_rcv_clk  = 1'b0,
  input  wire        fpga_aux_ref,

  // Device ID used by RFNoC, transports, etc. (Domain: rfnoc_chdr_clk)
  output reg [15:0] device_id
);

  `include "../../lib/rfnoc/core/ctrlport.vh"
  `include "regmap/global_regs_regmap_utils.vh"

  // Variant-dependent register map.
  `ifdef X440
    `include "regmap/x440/versioning_regs_regmap_utils.vh"
  `else // Use X410 as the default variant for regmap.
    `include "regmap/x410/versioning_regs_regmap_utils.vh"
  `endif


  // Make DEVICE_ID default to anything but 0, since that has special meaning
  localparam [DEVICE_ID_SIZE-1:0] DEFAULT_DEVICE_ID = 1;

  // Internal registers (Domain: s_ctrlport_clk)
  reg [  DEVICE_ID_SIZE-1:0] device_id_reg = DEFAULT_DEVICE_ID;
  reg [SCRATCH_REG_SIZE-1:0] scratch_reg   = {SCRATCH_REG_SIZE{1'b0}};
  reg [SERIAL_NUM_HIGH_REG_SIZE + SERIAL_NUM_LOW_REG_SIZE-1:0] serial_num_reg = 0;

  // CHDR clock counter (Domain: rfnoc_chdr_clk)
  reg  [CHDR_CLK_COUNT_REG_SIZE-1:0] chdr_counter = {CHDR_CLK_COUNT_REG_SIZE{1'b0}};

  // CHDR clock counter register (Domain: s_ctrlport_clk)
  wire chdr_counter_fifo_valid;
  wire [CHDR_CLK_COUNT_REG_SIZE-1:0] chdr_counter_fifo_data;
  reg  [CHDR_CLK_COUNT_REG_SIZE-1:0] chdr_counter_reg = 0;

  // Measure PPS for manufacturing test
  reg [MFG_TEST_FPGA_AUX_REF_FREQ_SIZE-1:0] fpga_aux_ref_freq = 0;

  reg [PPS_SELECT_SIZE-1:0] pps_select_reg = PPS_INT_25MHZ;
  assign pps_select = pps_select_reg;

  reg [TRIGGER_IO_SELECT_SIZE-1:0] trig_io_select_reg = TRIG_IO_INPUT;
  assign trig_io_select = trig_io_select_reg;

  // Bus counter in the rfnoc_chdr_clk domain.
  always @(posedge rfnoc_chdr_clk) begin
    if (rfnoc_chdr_rst) begin
      chdr_counter <= {CHDR_CLK_COUNT_REG_SIZE{1'b0}};
    end else begin
      chdr_counter <= chdr_counter + 1;
    end
  end

  // Safely cross clock domains for the CHDR counter.
  handshake #(
    .WIDTH (CHDR_CLK_COUNT_REG_SIZE)
  ) handshake_chdr_counter (
    .clk_a   (rfnoc_chdr_clk),
    .rst_a   (rfnoc_chdr_rst),
    .valid_a (1'b1),
    .data_a  (chdr_counter),
    .busy_a  (),
    .clk_b   (s_ctrlport_clk),
    .valid_b (chdr_counter_fifo_valid),
    .data_b  (chdr_counter_fifo_data)
  );

  // Register a valid FIFO output to ensure the counter is always valid.
  always @(posedge s_ctrlport_clk) begin
    if (s_ctrlport_rst) begin
      chdr_counter_reg <= 0;
    end else begin
      if (chdr_counter_fifo_valid) begin
        chdr_counter_reg <= chdr_counter_fifo_data;
      end
    end
  end

  wire [31:0] build_datestamp;

  USR_ACCESSE2 usr_access_i (
    .DATA(build_datestamp), .CFGCLK(), .DATAVALID()
  );


  //---------------------------------------------------------------------------
  // Global Registers
  //---------------------------------------------------------------------------

  // Check that address is within this module's range.
  wire address_in_range = (s_ctrlport_req_addr >= REG_BASE) && (s_ctrlport_req_addr < REG_BASE + REG_SIZE);

  // Registers implementation
  always @ (posedge s_ctrlport_clk) begin
    if (s_ctrlport_rst) begin
      s_ctrlport_resp_ack    <= 1'b0;
      s_ctrlport_resp_data   <= {32 {1'bX}};
      s_ctrlport_resp_status <= 2'b00;
      scratch_reg            <= {SCRATCH_REG_SIZE{1'b0}};
      serial_num_reg         <= 0;
      pps_select_reg         <= PPS_INT_25MHZ;
      trig_io_select_reg     <= TRIG_IO_INPUT;
      device_id_reg          <= DEFAULT_DEVICE_ID;

    end else begin
      // Write registers
      if (s_ctrlport_req_wr) begin
        // Acknowledge by default
        s_ctrlport_resp_ack    <= 1'b1;
        s_ctrlport_resp_data   <= 32'h0;
        s_ctrlport_resp_status <= CTRL_STS_OKAY;

        case (s_ctrlport_req_addr)
          REG_BASE + SCRATCH_REG: begin
            scratch_reg <= s_ctrlport_req_data;
          end

          REG_BASE + DEVICE_ID_REG: begin
            device_id_reg <= s_ctrlport_req_data[DEVICE_ID_MSB:DEVICE_ID];
          end

          REG_BASE + CLOCK_CTRL_REG: begin
            pps_select_reg     <= s_ctrlport_req_data[PPS_SELECT_MSB:PPS_SELECT];
            trig_io_select_reg <= s_ctrlport_req_data[TRIGGER_IO_SELECT_MSB:TRIGGER_IO_SELECT];
            pll_sync_delay     <= s_ctrlport_req_data[PLL_SYNC_DELAY_MSB:PLL_SYNC_DELAY];
            pll_sync_trigger   <= s_ctrlport_req_data[PLL_SYNC_TRIGGER];
            pps_brc_delay      <= s_ctrlport_req_data[PPS_BRC_DELAY_MSB:PPS_BRC_DELAY];
          end

          REG_BASE + PPS_CTRL_REG: begin
            pps_prc_delay  <= s_ctrlport_req_data[PPS_PRC_DELAY_MSB:PPS_PRC_DELAY];
            pps_rc_enabled <= s_ctrlport_req_data[PPS_RC_ENABLED];
          end

          REG_BASE + PPS_CROSSING_REG: begin
            prc_rc0_divider <= s_ctrlport_req_data[PRC_RC0_DIVIDER_MSB:PRC_RC0_DIVIDER];
            prc_rc1_divider <= s_ctrlport_req_data[PRC_RC1_DIVIDER_MSB:PRC_RC1_DIVIDER];
          end

          REG_BASE + SERIAL_NUM_LOW_REG: begin
            serial_num_reg[SERIAL_NUM_LOW_REG_SIZE-1:0] <= s_ctrlport_req_data;
          end

          REG_BASE + SERIAL_NUM_HIGH_REG: begin
            serial_num_reg[SERIAL_NUM_LOW_REG_SIZE +: SERIAL_NUM_HIGH_REG_SIZE] <= s_ctrlport_req_data;
          end

          REG_BASE + MFG_TEST_CTRL_REG: begin
            mfg_test_en_fabric_clk  <= s_ctrlport_req_data[MFG_TEST_EN_FABRIC_CLK];
            mfg_test_en_gty_rcv_clk <= s_ctrlport_req_data[MFG_TEST_EN_GTY_RCV_CLK];
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

      // Read registers
      end else if (s_ctrlport_req_rd) begin
        // Acknowledge by default
        s_ctrlport_resp_ack    <= 1'b1;
        s_ctrlport_resp_data   <= 32'h0;
        s_ctrlport_resp_status <= CTRL_STS_OKAY;

        case (s_ctrlport_req_addr)
          REG_BASE + COMPAT_NUM_REG: begin
            s_ctrlport_resp_data[COMPAT_MAJOR_MSB:COMPAT_MAJOR] <= FPGA_CURRENT_VERSION_MAJOR;
            s_ctrlport_resp_data[COMPAT_MINOR_MSB:COMPAT_MINOR] <= FPGA_CURRENT_VERSION_MINOR;
          end

          REG_BASE + DATESTAMP_REG: begin
            s_ctrlport_resp_data <= build_datestamp;
          end

          REG_BASE + GIT_HASH_REG: begin
            `ifndef GIT_HASH
              `define GIT_HASH 32'h0BADC0DE
            `endif
            s_ctrlport_resp_data <= `GIT_HASH;
          end

          REG_BASE + SCRATCH_REG: begin
            s_ctrlport_resp_data <= scratch_reg;
          end

          REG_BASE + DEVICE_ID_REG: begin
            if (PCIE_PRESENT) begin
              s_ctrlport_resp_data[PCIE_PRESENT_BIT] <= 1'b1;
            end
            s_ctrlport_resp_data[DEVICE_ID_MSB:DEVICE_ID] <= device_id_reg;
          end

          REG_BASE + RFNOC_INFO_REG: begin
            s_ctrlport_resp_data[CHDR_WIDTH_MSB:CHDR_WIDTH]               <= CHDR_W[CHDR_WIDTH_SIZE-1:0];
            s_ctrlport_resp_data[RFNOC_PROTO_MAJOR_MSB:RFNOC_PROTO_MAJOR] <= RFNOC_PROTOVER[RFNOC_PROTO_MAJOR_MSB:RFNOC_PROTO_MAJOR];
            s_ctrlport_resp_data[RFNOC_PROTO_MINOR_MSB:RFNOC_PROTO_MINOR] <= RFNOC_PROTOVER[RFNOC_PROTO_MINOR_MSB:RFNOC_PROTO_MINOR];
          end

          REG_BASE + CLOCK_CTRL_REG: begin
            s_ctrlport_resp_data[PPS_SELECT_MSB:PPS_SELECT]               <= pps_select_reg;
            s_ctrlport_resp_data[PLL_SYNC_DELAY_MSB:PLL_SYNC_DELAY]       <= pll_sync_delay;
            s_ctrlport_resp_data[PLL_SYNC_DONE]                           <= pll_sync_done;
            s_ctrlport_resp_data[TRIGGER_IO_SELECT_MSB:TRIGGER_IO_SELECT] <= trig_io_select_reg;
            s_ctrlport_resp_data[PPS_BRC_DELAY_MSB:PPS_BRC_DELAY]         <= pps_brc_delay;
          end

          REG_BASE + PPS_CTRL_REG: begin
            s_ctrlport_resp_data[PPS_RC_ENABLED]                    <= pps_rc_enabled;
            s_ctrlport_resp_data[PPS_PRC_DELAY_MSB:PPS_PRC_DELAY]   <= pps_prc_delay;
          end

          REG_BASE + PPS_CROSSING_REG: begin
            s_ctrlport_resp_data[PRC_RC0_DIVIDER_MSB:PRC_RC0_DIVIDER] <= prc_rc0_divider;
            s_ctrlport_resp_data[PRC_RC1_DIVIDER_MSB:PRC_RC1_DIVIDER] <= prc_rc1_divider;
          end

          REG_BASE + CHDR_CLK_RATE_REG: begin
            s_ctrlport_resp_data <= CHDR_CLK_RATE[CHDR_CLK_RATE_REG_SIZE-1:0];
          end

          REG_BASE + BUILD_SEED_REG: begin
            `ifndef BUILD_SEED
              `define BUILD_SEED 32'b0
            `endif
            s_ctrlport_resp_data <= `BUILD_SEED;
          end

          REG_BASE + CHDR_CLK_COUNT_REG: begin
            s_ctrlport_resp_data <= chdr_counter_reg;
          end

          REG_BASE + QSFP_PORT_0_0_INFO_REG: begin
            s_ctrlport_resp_data <= qsfp_port_0_0_info;
          end

          REG_BASE + QSFP_PORT_0_1_INFO_REG: begin
            s_ctrlport_resp_data <= qsfp_port_0_1_info;
          end

          REG_BASE + QSFP_PORT_0_2_INFO_REG: begin
            s_ctrlport_resp_data <= qsfp_port_0_2_info;
          end

          REG_BASE + QSFP_PORT_0_3_INFO_REG: begin
            s_ctrlport_resp_data <= qsfp_port_0_3_info;
          end

          REG_BASE + QSFP_PORT_1_0_INFO_REG: begin
            s_ctrlport_resp_data <= qsfp_port_1_0_info;
          end

          REG_BASE + QSFP_PORT_1_1_INFO_REG: begin
            s_ctrlport_resp_data <= qsfp_port_1_1_info;
          end

          REG_BASE + QSFP_PORT_1_2_INFO_REG: begin
            s_ctrlport_resp_data <= qsfp_port_1_2_info;
          end

          REG_BASE + QSFP_PORT_1_3_INFO_REG: begin
            s_ctrlport_resp_data <= qsfp_port_1_3_info;
          end

          REG_BASE + NUM_TIMEKEEPERS_REG: begin
            s_ctrlport_resp_data <= NUM_TIMEKEEPERS[NUM_TIMEKEEPERS_REG_SIZE-1:0];
          end

          REG_BASE + SERIAL_NUM_LOW_REG: begin
            s_ctrlport_resp_data <= serial_num_reg[SERIAL_NUM_LOW_REG_SIZE-1:0];
          end

          REG_BASE + SERIAL_NUM_HIGH_REG: begin
            s_ctrlport_resp_data <= serial_num_reg[SERIAL_NUM_LOW_REG_SIZE +: SERIAL_NUM_HIGH_REG_SIZE];
          end

          REG_BASE + MFG_TEST_CTRL_REG: begin
            s_ctrlport_resp_data[MFG_TEST_EN_FABRIC_CLK]  <= mfg_test_en_fabric_clk;
            s_ctrlport_resp_data[MFG_TEST_EN_GTY_RCV_CLK] <= mfg_test_en_gty_rcv_clk;
          end

          REG_BASE + MFG_TEST_STATUS_REG: begin
            s_ctrlport_resp_data[MFG_TEST_FPGA_AUX_REF_FREQ_MSB:MFG_TEST_FPGA_AUX_REF_FREQ] <= fpga_aux_ref_freq;
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

      end else begin
        s_ctrlport_resp_ack <= 1'b0;
      end
    end
  end

  // Assign Device ID register (Domain: s_ctrlport_clk) to module
  // output (Domain: rfnoc_chdr_clk).

  wire device_id_fifo_valid;
  wire [DEVICE_ID_SIZE-1:0] device_id_fifo_data;

  // Clock-crossing for device_id.
  handshake #(
    .WIDTH (DEVICE_ID_SIZE)
  ) handshake_device_id (
    .clk_a   (s_ctrlport_clk),
    .rst_a   (s_ctrlport_rst),
    .valid_a (1'b1),
    .data_a  (device_id_reg),
    .busy_a  (),
    .clk_b   (rfnoc_chdr_clk),
    .valid_b (device_id_fifo_valid),
    .data_b  (device_id_fifo_data)
  );

  // Register the handshake output to ensure device_id is always valid.
  always @(posedge rfnoc_chdr_clk) begin
    if (rfnoc_chdr_rst) begin
      device_id <= 'bX;
    end else begin
      if (device_id_fifo_valid) begin
        device_id <= device_id_fifo_data;
      end
    end
  end


  //---------------------------------------------------------------------------
  // FPGA_REF_CLK Test
  //---------------------------------------------------------------------------

  // Count the number of clocks on the incoming PPS for manufacturing test
  // validation.
  reg [25:0] fpga_aux_ref_cnt = 0;
  wire       fpga_aux_ref_sc1;
  reg        fpga_aux_ref_sc2 = 1'b0;

  synchronizer #(
    .STAGES      (2),
    .WIDTH       (1),
    .INITIAL_VAL (1'h0)
  ) synchronizer_fpga_aux_ref (
    .clk (s_ctrlport_clk),
    .rst (1'b0),
    .in  (fpga_aux_ref),
    .out (fpga_aux_ref_sc1)
  );

  // 1.2 seconds with a 40 Mhz clock
  localparam FPGA_AUX_REF_CNT_MAX = 48*1000*1000;

  // Registers implementation
  always @ (posedge s_ctrlport_clk) begin
    if (s_ctrlport_rst) begin
      fpga_aux_ref_sc2  <= 1'b0;
      fpga_aux_ref_freq <= 0;
      fpga_aux_ref_cnt  <= 0;
    end else begin
      fpga_aux_ref_sc2 <= fpga_aux_ref_sc1;
      // Detect rising edge (Was low, now is high)
      if (!fpga_aux_ref_sc2 && fpga_aux_ref_sc1) begin
        // if the count is less than max
        if (fpga_aux_ref_cnt < FPGA_AUX_REF_CNT_MAX) begin
          fpga_aux_ref_freq <= fpga_aux_ref_cnt;
        // if count reached max
        end else begin
          fpga_aux_ref_freq <= 0;
        end
        // reset the counter at each rising edge
        fpga_aux_ref_cnt  <= 0;
      end else begin
        //stop incrementing at the max value
        if (fpga_aux_ref_cnt < FPGA_AUX_REF_CNT_MAX) begin
          fpga_aux_ref_cnt  <= fpga_aux_ref_cnt+1;
        end
      end
    end
  end

endmodule


`default_nettype wire


//XmlParse xml_on
//<regmap name="GLOBAL_REGS_REGMAP" readablestrobes="false" generatevhdl="true" ettusguidelines="true">
//  <group name="GLOBAL_REGS">
//
//    <register name="COMPAT_NUM_REG"      offset="0x00" size="32" writable="false">
//      <info>Revision number</info>
//      <bitfield name="COMPAT_MINOR" range="15..0"/>
//      <bitfield name="COMPAT_MAJOR" range="31..16"/>
//    </register>
//    <register name="DATESTAMP_REG"       offset="0x04" size="32" writable="false">
//      <info>Build datestamp (32-bit)</info>
//      <bitfield name="SECONDS" range="5..0"/>
//      <bitfield name="MINUTES" range="11..6"/>
//      <bitfield name="HOUR"    range="16..12"/>
//      <bitfield name="YEAR"    range="22..17">
//        <info>This is the year number after 2000 (e.g. 2019 = d19).</info>
//      </bitfield>
//      <bitfield name="MONTH"   range="26..23"/>
//      <bitfield name="DAY"     range="31..27"/>
//    </register>
//    <register name="GIT_HASH_REG"        offset="0x08" size="32" writable="false">
//      <info>Git hash of source commit.</info>
//    </register>
//    <register name="SCRATCH_REG"         offset="0x0C" size="32">
//      <info>Scratch register for testing.</info>
//    </register>
//    <register name="DEVICE_ID_REG"       offset="0x10" size="32">
//      <info>Register that contains the motherboard's device ID.</info>
//      <bitfield name="PCIE_PRESENT_BIT" range="31">
//        <info>Set to 1 if PCI-Express core is present in FPGA design.</info>
//      </bitfield>
//      <bitfield name="DEVICE_ID" range="15..0"/>
//    </register>
//    <register name="RFNOC_INFO_REG"      offset="0x14" size="32" writable="false">
//      <info>Register that provides information on the RFNoC protocol.</info>
//      <bitfield name="RFNOC_PROTO_MINOR" range="7..0"/>
//      <bitfield name="RFNOC_PROTO_MAJOR" range="15..8"/>
//      <bitfield name="CHDR_WIDTH"        range="31..16"/>
//    </register>
//    <register name="CLOCK_CTRL_REG"      offset="0x18" size="32">
//      <info>Control register for clocking resources.</info>
//      <bitfield name="PPS_SELECT" range="1..0" initialvalue="PPS_INT_25MHZ">
//        <enumeratedtype name="PPS_ENUM">
//          <value name="PPS_INT_25MHZ" integer="0"/>
//          <value name="PPS_INT_10MHZ" integer="1"/>
//          <value name="PPS_EXT" integer="2"/>
//        </enumeratedtype>
//        <info>
//          Select the source of the PPS signal.
//          For the internal generation the value depending on the base reference clock has to be chosen.
//          The external reference is taken from the PPS_IN pin and is independent of the base reference clock.
//        </info>
//      </bitfield>
//      <bitfield name="REF_SELECT"    range="2">
//        <info>
//          RESERVED. This bit is not implemented on X4xx and reads as 0.
//        </info>
//      </bitfield>
//      <bitfield name="REFCLK_LOCKED" range="3" writable="false">
//        <info>
//          RESERVED. This bit is not implemented on X4xx and reads as 0.
//        </info>
//      </bitfield>
//      <bitfield name="TRIGGER_IO_SELECT" range="5..4" initialvalue="TRIG_IO_INPUT">
//        <info>
//          <b>IMPORTANT!</b> SW must ensure any TRIG_IO consumers (downstream devices) <b>ignore
//          and/or re-sync after enabling this port</b>, since the output-enable is basically
//          asynchronous to the actual TRIG_IO driver.
//        </info>
//        <enumeratedtype name="TRIG_IO_ENUM">
//          <value name="TRIG_IO_INPUT" integer="0"/>
//          <value name="TRIG_IO_PPS_OUTPUT" integer="1"/>
//        </enumeratedtype>
//        <info>
//          Select the direction and content of the trigger inout signal.
//        </info>
//      </bitfield>
//      <bitfield name="PLL_SYNC_TRIGGER" range="8" readable="false">
//        <info>
//          Assertion triggers the SYNC signal generation for LMK04832 after the next appearance of the PPS rising edge.
//          There is no self reset on this trigger.
//          Keep this trigger asserted until @.PLL_SYNC_DONE is asserted.
//        </info>
//      </bitfield>
//      <bitfield name="PLL_SYNC_DONE" range="9" writable="false">
//        <info>Indicates the success of the PLL reset started by @.PLL_SYNC_TRIGGER. Reset on deassertion of @.PLL_SYNC_TRIGGER.</info>
//      </bitfield>
//      <bitfield name="PLL_SYNC_DELAY" range="16..23">
//        <info>
//          Due to the HDL implementation the rising edge of the SYNC signal for
//          the LMK04832 is generated 2 clock cycles after the PPS rising edge.
//          This delay can be further increased by setting this delay value
//          (e.g. PLL_SYNC_DELAY=3 will result in a total delay of 5 clock cycles).<br>
//          In case two X400 devices are connected using the PPS and reference clock the master delay value needs to be 3 clock cycles
//          higher than the slave delay value to align the LMK sync edges in time.
//        </info>
//      </bitfield>
//      <bitfield name="PPS_BRC_DELAY" range="31..24">
//        <info>
//          Number of base reference clock cycles from appearance of the PPS
//          rising edge to the occurrence of the aligned edge of base reference
//          clock and PLL reference clock at the sample PLL output. This number
//          is the sum of the actual value based on @.PLL_SYNC_DELAY (also
//          accumulate the fixed amount of clock cycles) and if any the number of
//          cycles the SPLL requires from issuing of the SYNC signal to the
//          aligned edge (with LMK04832 = 0).<br>
//          The number written to this register has to be reduced by 1 due to
//          HDL implementation.
//        </info>
//      </bitfield>
//    </register>
//    <register name="PPS_CTRL_REG" offset="0x1C" size="32">
//      <info>Control registers for PPS generation.</info>
//      <bitfield name="PPS_PRC_DELAY" range="25..0">
//        <info>
//          The number of PLL reference clock cycles from one aligned edge to the
//          desired aligned edge to issue the PPS in radio clock domain. This
//          delay is configurable to any aligned edge within a maximum delay of 1
//          second (period of PPS). <br>
//          The value written to the register has to be reduced by 5 due to
//          HDL implementation.
//        </info>
//      </bitfield>
//      <bitfield name="PPS_RC_ENABLED" range="31">
//        <info>
//          Enables the PPS signal in radio clock domain. Please make sure that
//          the values of @.PPS_BRC_DELAY, @.PPS_PRC_DELAY, @.PRC_RC0_DIVIDER and
//          @.PRC_RC1_DIVIDER are set before enabling this bit. It is recommended
//          to disable the PPS for changes on the other values. Use a wait time of
//          at least 1 second before changing this value to ensure the values are
//          stable for the next PPS edge.
//        </info>
//      </bitfield>
//    </register>
//    <register name="CHDR_CLK_RATE_REG"   offset="0x20" size="32" writable="false">
//      <info>Returns the RFNoC bus clock rate (CHDR).</info>
//      <bitfield name="CHDR_CLK" range="31..0">
//      </bitfield>
//    </register>
//    <register name="CHDR_CLK_COUNT_REG"  offset="0x24" size="32" writable="false">
//      <info>
//        Returns the count value of a free-running counter driven by the RFNoC
//        CHDR bus clock.
//      </info>
//    </register>
//    <register name="BUILD_SEED_REG" offset="0x28" size="32" writable="false">
//      <info>
//        Build seed used for this compilation. Making this value readable
//        ensures that compilation results are affected by the value in this
//        register.
//      </info>
//    </register>
//    <register name="PPS_CROSSING_REG" offset="0x30" size="32">
//      <info>Control registers for PPS clock crossing to the radio clock domain.</info>
//      <bitfield name="PRC_RC1_DIVIDER" range="20..16">
//        <info>
//          Clock multiplier used to generate radio clock 1 from PLL reference clock.
//          The value written to the register has to follow the following formula:
//          <code>PRC_RC1_DIVIDER = (RADIO_CLK_1 / PRC_CLK) * 2 - 2</code>
//        </info>
//      </bitfield>
//      <bitfield name="PRC_RC0_DIVIDER" range="4..0">
//        <info>
//          Clock multiplier used to generate radio clock 0 from PLL reference clock.
//          The value written to the register has to follow the following formula:
//          <code>PRC_RC0_DIVIDER = (RADIO_CLK_0 / PRC_CLK) * 2 - 2</code>
//        </info>
//      </bitfield>
//    </register>
//    <register name="QSFP_PORT_0_0_INFO_REG"   offset="0x60" size="32" writable="false">
//      <info>
//        Returns information from the QSFP0 Lane0.
//      </info>
//    </register>
//    <register name="QSFP_PORT_0_1_INFO_REG"   offset="0x64" size="32" writable="false">
//      <info>
//        Returns information from the QSFP0 Lane1.
//      </info>
//    </register>
//    <register name="QSFP_PORT_0_2_INFO_REG"   offset="0x68" size="32" writable="false">
//      <info>
//        Returns information from the QSFP0 Lane2.
//      </info>
//    </register>
//    <register name="QSFP_PORT_0_3_INFO_REG"   offset="0x6C" size="32" writable="false">
//      <info>
//        Returns information from the QSFP0 Lane3.
//      </info>
//    </register>
//    <register name="QSFP_PORT_1_0_INFO_REG"   offset="0x70" size="32" writable="false">
//      <info>
//        Returns information from the QSFP1 Lane0.
//      </info>
//    </register>
//    <register name="QSFP_PORT_1_1_INFO_REG"   offset="0x74" size="32" writable="false">
//      <info>
//        Returns information from the QSFP1 Lane1.
//      </info>
//    </register>
//    <register name="QSFP_PORT_1_2_INFO_REG"   offset="0x78" size="32" writable="false">
//      <info>
//        Returns information from the QSFP1 Lane2.
//      </info>
//    </register>
//    <register name="QSFP_PORT_1_3_INFO_REG"   offset="0x7C" size="32" writable="false">
//      <info>
//        Returns information from the QSFP1 Lane3.
//      </info>
//    </register>
//    <register name="GPS_CTRL_REG"        offset="0x38" size="32">
//      <info>
//        RESERVED. This register is not implemented on X4xx. GPS is connected
//        to the PS via a UART.
//      </info>
//    </register>
//    <register name="GPS_STATUS_REG"      offset="0x3C" size="32" writable="false">
//      <info>
//        RESERVED. This register is not implemented on X4xx. GPS is connected
//        to the PS via a UART.
//      </info>
//    </register>
//    <register name="DBOARD_CTRL_REG"     offset="0x40" size="32">
//      <info>
//        RESERVED. This register is not implemented on X4xx.
//      </info>
//    </register>
//    <register name="DBOARD_STATUS_REG"   offset="0x44" size="32" writable="false">
//      <info>
//        RESERVED. This register is not implemented on X4xx.
//      </info>
//    </register>
//    <register name="NUM_TIMEKEEPERS_REG" offset="0x48" size="32" writable="false">
//      <info>Register that specifies the number of timekeepers in the core.</info>
//    </register>
//    <register name="SERIAL_NUM_LOW_REG"         offset="0x4C" size="32">
//      <info>Least significant bytes of 8 byte serial number</info>
//    </register>
//    <register name="SERIAL_NUM_HIGH_REG"         offset="0x50" size="32">
//      <info>Most significant bytes of 8 byte serial number</info>
//    </register>
//    <register name="MFG_TEST_CTRL_REG"      offset="0x54" size="32">
//      <info>Control register for mfg_test functions.</info>
//      <bitfield name="MFG_TEST_EN_GTY_RCV_CLK" range="0">
//        <info>
//          When enabled, routes data_clk to GTY_RCV_CLK output port.
//          When disabled, the GTY_RCV_CLK output is driven to 0.
//        </info>
//      </bitfield>
//      <bitfield name="MFG_TEST_EN_FABRIC_CLK" range="1">
//        <info>
//          When enabled, routes data_clk to FPGA_REF_CLK output port.
//          When disabled, the FPGA_REF_CLK output is driven to 0.
//        </info>
//      </bitfield>
//    </register>
//    <register name="MFG_TEST_STATUS_REG" offset="0x58" size="32">
//      <info>Status register for mfg_test functions.</info>
//      <bitfield name="MFG_TEST_FPGA_AUX_REF_FREQ" range="25..0">
//        <info>
//          Report the time between rising edges on the FPGA_REF_CLK
//          input port in 40 MHz Clock ticks. If the count extends
//          to 1.2 seconds without an edge, the value reported is set
//          to zero.
//        </info>
//      </bitfield>
//    </register>
//  </group>
//</regmap>
//XmlParse xml_off

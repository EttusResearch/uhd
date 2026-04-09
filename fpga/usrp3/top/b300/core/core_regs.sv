//
// Copyright 2025 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: core_regs
//
// Description:
//   Implements register endpoints for B310 FPGA core
//

`default_nettype none

module core_regs #(
  parameter logic [19:0]  BASE_ADDRESS    = 0,
  parameter logic [19:0]  SIZE_ADDRESS    = 0,
  parameter logic [15:0]  PROTOVER        = {8'd1, 8'd0},
  parameter int           CHDR_WIDTH      = 64,
  parameter int           NUM_TIMEKEEPERS = 1
) (
  ctrlport_if.slave s_ctrlport,
  output logic [15:0] device_id,

  // CORE_SW_RESETS
  output logic radio_clk_gen_rst,

  // CORE_CLOCK_CTRL
  output logic ref_clk_source,
  output logic lmk_source_select,
  output logic tcxo_en,
  output logic lmk_reset,
  output logic use_external_pps,

  // CORE_CLOCK_STATUS
  input wire lmk_lock_status,

  // Device DNA
  input wire [63:0] dev_dna,

  // Radio Timestamp
  input wire [63:0] radio_time,

  // PPS generator Divider
  output logic [31:0] int_pps_div,

  // PPS in to rclock delay
  output logic [ 9:0] pps_in_to_rclk_delay,
  output logic [26:0] lmk_sync_delay,
  output logic        lmk_sync_clk_sel,
  output logic        lmk_clkin0_sync_sel,
  output logic        lmk_sync_trigger,
  input  wire         lmk_sync_done,
  output logic        lmk05318_pd_n,

  // FP_GPIO Source
  output logic [19:0] fp_gpio_src,

  // GPS control
  output logic       gps_reset_n,
  output logic       gps_ant_pwr_en,
  input  wire  [1:0] gps_lmk_status,
  input  wire        gps_pw_fault,
  output logic       gps_lmk_gpio,
  input  wire        gps_lmk_pps_monitor,
  output logic       gps_bypass_lmk,

  // Thunderbolt resets
  output logic tbolt_pd_ctrl_pulse_rst,
  // Power control and status
  input  wire  pwr_1v2_pg,
  input  wire  pwr_25w_src,
  input  wire  pwr_typec_negotiated,
  input  wire  pwr_monitor_alert,
  output logic pwr_led_orange,

  // Temperature monitor
  input  wire [11:0] device_temp
);

  import ctrlport_pkg::*;
  `include "../regmap/core_regs_regmap_utils.vh"

  //vhook_sigstart
  //vhook_sigend


  //----------------------------------------------------------
  // Handling of CtrlPort
  //----------------------------------------------------------
  wire address_in_range = (s_ctrlport.req.addr >= BASE_ADDRESS) &&
                          (s_ctrlport.req.addr < BASE_ADDRESS + SIZE_ADDRESS);

  // Local register copies
  logic [DEVICE_ID_SIZE-1:0] device_id_reg      = '0;
  logic [              31:0] int_pps_div_reg    = 32'd122_880_000;
  logic                      ref_clk_source_reg = 1'b1;

  logic [31:0] radio_time_hi;

  assign device_id      = device_id_reg;
  assign ref_clk_source = ref_clk_source_reg;
  assign int_pps_div    = int_pps_div_reg;

  always_ff @(posedge s_ctrlport.clk) begin
    // reset internal registers and responses
    if (s_ctrlport.rst) begin
      device_id_reg           <= '0;

      s_ctrlport.resp.ack     <= 1'b0;
      s_ctrlport.resp.data    <= 'x;
      s_ctrlport.resp.status  <= STS_OKAY;

      // reset controls
      radio_clk_gen_rst             <= 1'b0;
      // By default, use internal reference clock
      ref_clk_source_reg            <= 1'b1;
      lmk_source_select             <= 1'b0;
      tcxo_en                       <= 1'b0;
      lmk_reset                     <= 1'b0;
      use_external_pps              <= 1'b0;
      int_pps_div_reg               <= 32'd122_880_000;
      pps_in_to_rclk_delay          <= 10'b0;
      lmk_sync_delay                <= 27'b0;
      lmk_sync_trigger              <= 1'b0;
      lmk_sync_clk_sel              <= 1'b0;
      lmk_clkin0_sync_sel           <= 1'b0;
      lmk05318_pd_n                 <= 1'b0;
      radio_time_hi                 <= 'X;

      // Thunderbolt defaults
      tbolt_pd_ctrl_pulse_rst       <= 1'b0;
      pwr_led_orange                <= 1'b0;

      // MISC defaults
      fp_gpio_src                   <= '0;
      gps_reset_n                   <= 1'b0;
      gps_ant_pwr_en                <= 1'b0;
      gps_lmk_gpio                  <= 1'b0;
      gps_bypass_lmk                <= 1'b0;
    end else begin

      // Clear pulse reset signals by default
      tbolt_pd_ctrl_pulse_rst <= 1'b0;

      // write requests
      if (s_ctrlport.req.wr) begin
        // always issue an ack and no data
        s_ctrlport.resp.ack     <= 1'b1;
        s_ctrlport.resp.data    <= 'x;
        s_ctrlport.resp.status  <= STS_OKAY;

        case (s_ctrlport.req.addr)
          BASE_ADDRESS + FPGA_DEVICE_ID: begin
            device_id_reg <= s_ctrlport.req.data[ DEVICE_ID_MSB : DEVICE_ID ];
          end

          BASE_ADDRESS + CORE_SW_RESETS: begin
            radio_clk_gen_rst <= s_ctrlport.req.data[ RADIO_CLK_GEN_RST ];
          end

          BASE_ADDRESS + CORE_CLOCK_CTRL: begin
            ref_clk_source_reg  <= s_ctrlport.req.data[ REF_CLK_SOURCE ];
            lmk_source_select   <= s_ctrlport.req.data[ LMK_SOURCE_SELECT ];
            tcxo_en             <= s_ctrlport.req.data[ TCXO_EN ];
            lmk_reset           <= s_ctrlport.req.data[ LMK_RESET ];
            use_external_pps    <= s_ctrlport.req.data[ USE_EXTERNAL_PPS ];
            lmk05318_pd_n       <= s_ctrlport.req.data[ LMK05318_PD_N ];
          end

          BASE_ADDRESS + FP_GPIO_SRC: begin
            fp_gpio_src[ FP_GPIO9_SRC_MSB : FP_GPIO9_SRC ] <=
              s_ctrlport.req.data[ FP_GPIO9_SRC_MSB : FP_GPIO9_SRC ];
            fp_gpio_src[ FP_GPIO8_SRC_MSB : FP_GPIO8_SRC ] <=
              s_ctrlport.req.data[ FP_GPIO8_SRC_MSB : FP_GPIO8_SRC ];
            fp_gpio_src[ FP_GPIO7_SRC_MSB : FP_GPIO7_SRC ] <=
              s_ctrlport.req.data[ FP_GPIO7_SRC_MSB : FP_GPIO7_SRC ];
            fp_gpio_src[ FP_GPIO6_SRC_MSB : FP_GPIO6_SRC ] <=
              s_ctrlport.req.data[ FP_GPIO6_SRC_MSB : FP_GPIO6_SRC ];
            fp_gpio_src[ FP_GPIO5_SRC_MSB : FP_GPIO5_SRC ] <=
              s_ctrlport.req.data[ FP_GPIO5_SRC_MSB : FP_GPIO5_SRC ];
            fp_gpio_src[ FP_GPIO4_SRC_MSB : FP_GPIO4_SRC ] <=
              s_ctrlport.req.data[ FP_GPIO4_SRC_MSB : FP_GPIO4_SRC ];
            fp_gpio_src[ FP_GPIO3_SRC_MSB : FP_GPIO3_SRC ] <=
              s_ctrlport.req.data[ FP_GPIO3_SRC_MSB : FP_GPIO3_SRC ];
            fp_gpio_src[ FP_GPIO2_SRC_MSB : FP_GPIO2_SRC ] <=
              s_ctrlport.req.data[ FP_GPIO2_SRC_MSB : FP_GPIO2_SRC ];
            fp_gpio_src[ FP_GPIO1_SRC_MSB : FP_GPIO1_SRC ] <=
              s_ctrlport.req.data[ FP_GPIO1_SRC_MSB : FP_GPIO1_SRC ];
            fp_gpio_src[ FP_GPIO0_SRC_MSB : FP_GPIO0_SRC ] <=
              s_ctrlport.req.data[ FP_GPIO0_SRC_MSB : FP_GPIO0_SRC ];
          end

          BASE_ADDRESS + GPS_CTRL: begin
            gps_reset_n       <= s_ctrlport.req.data[ REG_GPS_RESET_N ];
            gps_ant_pwr_en    <= s_ctrlport.req.data[ REG_GPS_ANT_PWR_EN ];
            gps_bypass_lmk    <= s_ctrlport.req.data[ GPS_BYPASS_LMK ];
            gps_lmk_gpio      <= s_ctrlport.req.data[ GPS_LMK_GPIO ];
          end

          BASE_ADDRESS + INT_PPS_DIVIDER: begin
            int_pps_div_reg   <= s_ctrlport.req.data[INT_PPS_DIV_MSB : INT_PPS_DIV];
          end

          BASE_ADDRESS + PPS_IN_CTRL: begin
            pps_in_to_rclk_delay <=
              s_ctrlport.req.data[PPS_IN_TO_RCLK_DELAY_MSB : PPS_IN_TO_RCLK_DELAY];
          end

          BASE_ADDRESS + LMK_SYNC_CTRL: begin
            lmk_sync_delay      <= s_ctrlport.req.data[LMK_SYNC_DELAY_MSB : LMK_SYNC_DELAY];
            lmk_sync_trigger    <= s_ctrlport.req.data[LMK_SYNC_TRIGGER];
            lmk_sync_clk_sel    <= s_ctrlport.req.data[LMK_SYNC_CLK_SEL];
            lmk_clkin0_sync_sel <= s_ctrlport.req.data[LMK_CLKIN0_SYNC_SEL];
          end

          BASE_ADDRESS + TBOLT_CTRL: begin
            tbolt_pd_ctrl_pulse_rst   <= s_ctrlport.req.data[TBOLT_PD_CTRL_PULSE_RST];
          end

          BASE_ADDRESS + POWER_CONTROL_STS: begin
            pwr_led_orange         <= s_ctrlport.req.data[POWER_LED_OVERRIDE];
          end

          // error on undefined address
          default: begin
            if (address_in_range) begin
              s_ctrlport.resp.status  <= STS_CMDERR;

            // no response if out of range
            end else begin
              s_ctrlport.resp.ack     <= 1'b0;
            end
          end
        endcase

      // read requests
      end else if (s_ctrlport.req.rd) begin
        // default assumption: valid request
        s_ctrlport.resp.ack     <= 1'b1;
        s_ctrlport.resp.status  <= STS_OKAY;
        s_ctrlport.resp.data    <= '0;

        case (s_ctrlport.req.addr)
          BASE_ADDRESS + FPGA_DEVICE_ID: begin
            s_ctrlport.resp.data[DEVICE_ID_MSB : DEVICE_ID]
              <= device_id_reg;
          end

          BASE_ADDRESS + CORE_SW_RESETS: begin
            s_ctrlport.resp.data[RADIO_CLK_GEN_RST]
              <= radio_clk_gen_rst;
          end

          BASE_ADDRESS + CORE_CLOCK_CTRL: begin
            s_ctrlport.resp.data[REF_CLK_SOURCE]
              <= ref_clk_source_reg;
            s_ctrlport.resp.data[LMK_SOURCE_SELECT]
              <= lmk_source_select;
            s_ctrlport.resp.data[TCXO_EN]
              <= tcxo_en;
            s_ctrlport.resp.data[LMK_RESET]
              <= lmk_reset;
            s_ctrlport.resp.data[USE_EXTERNAL_PPS]
              <= use_external_pps;
            s_ctrlport.resp.data[LMK05318_PD_N]
              <= lmk05318_pd_n;
          end

          BASE_ADDRESS + CORE_CLOCK_STATUS: begin
            s_ctrlport.resp.data[LMK_LOCK_STATUS]
              <= lmk_lock_status;
          end

          BASE_ADDRESS + DEVICE_DNA_LOW: begin
            s_ctrlport.resp.data[DEV_DNA_LO_MSB : DEV_DNA_LO]
              <= dev_dna[31:0];
          end

          BASE_ADDRESS + DEVICE_DNA_HIGH: begin
            s_ctrlport.resp.data[DEV_DNA_HI_MSB : DEV_DNA_HI]
              <= dev_dna[63:32];
          end

          BASE_ADDRESS + CORE_COMPAT_NUM: begin
            s_ctrlport.resp.data[CORE_MAJOR_COMPAT_MSB : CORE_MAJOR_COMPAT]
              <= CORE_COMPAT_MAJOR_VAL[CORE_MAJOR_COMPAT_SIZE-1:0];
            s_ctrlport.resp.data[CORE_MINOR_COMPAT_MSB : CORE_MINOR_COMPAT]
              <= CORE_COMPAT_MINOR_VAL[CORE_MINOR_COMPAT_SIZE-1:0];
          end

          BASE_ADDRESS + CORE_RFNOC_INFO: begin
            s_ctrlport.resp.data[RFNOC_PROTO_VERSION_MSB : RFNOC_PROTO_VERSION]
              <= PROTOVER[RFNOC_PROTO_VERSION_SIZE-1:0];
            s_ctrlport.resp.data[RFNOC_CHDR_WIDTH_MSB : RFNOC_CHDR_WIDTH]
              <= CHDR_WIDTH[RFNOC_CHDR_WIDTH_SIZE-1:0];
          end

          BASE_ADDRESS + RADIO_TIME_LOW: begin
            s_ctrlport.resp.data <= radio_time[31:0];
            radio_time_hi        <= radio_time[63:32];
          end

          BASE_ADDRESS + RADIO_TIME_HIGH: begin
            s_ctrlport.resp.data <= radio_time_hi;
          end

          BASE_ADDRESS + INT_PPS_DIVIDER: begin
            s_ctrlport.resp.data[INT_PPS_DIV_MSB : INT_PPS_DIV]
              <= int_pps_div_reg;
          end

          BASE_ADDRESS + PPS_IN_CTRL: begin
            s_ctrlport.resp.data[PPS_IN_TO_RCLK_DELAY_MSB : PPS_IN_TO_RCLK_DELAY]
              <= pps_in_to_rclk_delay;
          end

          BASE_ADDRESS + LMK_SYNC_CTRL: begin
            s_ctrlport.resp.data[LMK_SYNC_DELAY_MSB : LMK_SYNC_DELAY]
              <= lmk_sync_delay;
            s_ctrlport.resp.data[LMK_SYNC_DONE]
              <= lmk_sync_done;
            s_ctrlport.resp.data[LMK_SYNC_TRIGGER]
              <= lmk_sync_trigger;
            s_ctrlport.resp.data[LMK_SYNC_CLK_SEL]
              <= lmk_sync_clk_sel;
            s_ctrlport.resp.data[LMK_CLKIN0_SYNC_SEL]
              <= lmk_clkin0_sync_sel;
          end

          BASE_ADDRESS + CORE_NUM_TIMEKEEPERS: begin
            s_ctrlport.resp.data[CORE_NUM_TIMEKEEPERS_VAL_MSB : CORE_NUM_TIMEKEEPERS_VAL]
              <= NUM_TIMEKEEPERS[CORE_NUM_TIMEKEEPERS_VAL_SIZE-1:0];
          end

          BASE_ADDRESS + CORE_BUILD_SEED: begin
            `ifndef BUILD_SEED
              `define BUILD_SEED 32'b0
            `endif
            s_ctrlport.resp.data[CORE_BUILD_SEED_VAL_MSB : CORE_BUILD_SEED_VAL]
              <= `BUILD_SEED;
          end

          BASE_ADDRESS + FP_GPIO_SRC: begin
            s_ctrlport.resp.data[FP_GPIO9_SRC_MSB : FP_GPIO9_SRC]
              <= fp_gpio_src[FP_GPIO9_SRC_MSB : FP_GPIO9_SRC];
            s_ctrlport.resp.data[FP_GPIO8_SRC_MSB : FP_GPIO8_SRC]
              <= fp_gpio_src[FP_GPIO8_SRC_MSB : FP_GPIO8_SRC];
            s_ctrlport.resp.data[FP_GPIO7_SRC_MSB : FP_GPIO7_SRC]
              <= fp_gpio_src[FP_GPIO7_SRC_MSB : FP_GPIO7_SRC];
            s_ctrlport.resp.data[FP_GPIO6_SRC_MSB : FP_GPIO6_SRC]
              <= fp_gpio_src[FP_GPIO6_SRC_MSB : FP_GPIO6_SRC];
            s_ctrlport.resp.data[FP_GPIO5_SRC_MSB : FP_GPIO5_SRC]
              <= fp_gpio_src[FP_GPIO5_SRC_MSB : FP_GPIO5_SRC];
            s_ctrlport.resp.data[FP_GPIO4_SRC_MSB : FP_GPIO4_SRC]
              <= fp_gpio_src[FP_GPIO4_SRC_MSB : FP_GPIO4_SRC];
            s_ctrlport.resp.data[FP_GPIO3_SRC_MSB : FP_GPIO3_SRC]
              <= fp_gpio_src[FP_GPIO3_SRC_MSB : FP_GPIO3_SRC];
            s_ctrlport.resp.data[FP_GPIO2_SRC_MSB : FP_GPIO2_SRC]
              <= fp_gpio_src[FP_GPIO2_SRC_MSB : FP_GPIO2_SRC];
            s_ctrlport.resp.data[FP_GPIO1_SRC_MSB : FP_GPIO1_SRC]
              <= fp_gpio_src[FP_GPIO1_SRC_MSB : FP_GPIO1_SRC];
            s_ctrlport.resp.data[FP_GPIO0_SRC_MSB : FP_GPIO0_SRC]
              <= fp_gpio_src[FP_GPIO0_SRC_MSB : FP_GPIO0_SRC];
          end

          BASE_ADDRESS + GPS_CTRL: begin
            s_ctrlport.resp.data[REG_GPS_RESET_N]
              <= gps_reset_n;
            s_ctrlport.resp.data[REG_GPS_ANT_PWR_EN]
              <= gps_ant_pwr_en;
            s_ctrlport.resp.data[GPS_PW_FAULT]
              <= gps_pw_fault;
            s_ctrlport.resp.data[GPS_LMK_PPS_MONITOR]
              <= gps_lmk_pps_monitor;
            s_ctrlport.resp.data[GPS_LMK_STATUS_MSB : GPS_LMK_STATUS]
              <= gps_lmk_status;
            s_ctrlport.resp.data[GPS_LMK_GPIO]
              <= gps_lmk_gpio;
            s_ctrlport.resp.data[GPS_BYPASS_LMK]
              <= gps_bypass_lmk;
          end

          BASE_ADDRESS + TEMP_MON: begin
            s_ctrlport.resp.data[DEVICE_TEMP_MSB : DEVICE_TEMP]
              <= device_temp;
          end

          BASE_ADDRESS + TBOLT_CTRL: begin
            // Return a value to service IO mapped accesses that expect to read back
            // from the address they write.
            s_ctrlport.resp.data[TBOLT_PD_CTRL_PULSE_RST] <= '0;
          end

          BASE_ADDRESS + POWER_CONTROL_STS: begin
            s_ctrlport.resp.data[POWER_GOOD_1V2]
              <= pwr_1v2_pg;
            s_ctrlport.resp.data[POWER_MONITOR_ALERT]
              <= pwr_monitor_alert;
            s_ctrlport.resp.data[POWER_25W_SRC]
              <= pwr_25w_src;
            s_ctrlport.resp.data[TYPEC_POWER_NEGOTIATED]
              <= pwr_typec_negotiated;
            s_ctrlport.resp.data[POWER_LED_OVERRIDE]
              <= pwr_led_orange;
          end

          // error on undefined address
          default: begin
            s_ctrlport.resp.data <= '0;
            if (address_in_range) begin
              s_ctrlport.resp.status <= STS_CMDERR;

            // no response if out of range
            end else begin
              s_ctrlport.resp.ack <= 1'b0;
            end
          end
        endcase

      // no request
      end else begin
        s_ctrlport.resp.ack <= 1'b0;
      end
    end
  end

endmodule : core_regs

`default_nettype wire

//XmlParse xml_on
//<regmap name="CORE_REGS_REGMAP" readablestrobes="false" generatesv="false" generateverilog="true" ettusguidelines="true">
// <group name="CORE_REGS_REGISTERS" size="0x080">
//   <info>
//     This regmap contains general control registers for the FPGA core.
//   </info>
//
//    <enumeratedtype name="CLOCKING_SPI_SLAVES" showhex="true">
//      <info>
//        Mapping of the SPI slaves on the FPGA core.
//      </info>
//      <value name="CLOCKING_SPI_LMK04832_SS"           integer="0"/>
//      <value name="CLOCKING_SPI_TCXO_DAC_SS"           integer="1"/>
//      <value name="CLOCKING_SPI_LMK053_SS"             integer="2"/>
//    </enumeratedtype>
//
//    <enumeratedtype name="CORE_COMPATIBILITY" showhex="true">
//      <info>
//        Mapping of the SPI slaves on the FPGA core.
//      </info>
//      <value name="CORE_COMPAT_MAJOR_VAL"           integer="2"/>
//      <value name="CORE_COMPAT_MINOR_VAL"           integer="1"/>
//    </enumeratedtype>
//
//   <register name="FPGA_DEVICE_ID" size="32" offset="0x00" attributes="Readable|Writable">
//     <info>
//       Sets the device ID of the FPGA core. This is used to enumerate the device in RFNoC.
//     </info>
//     <bitfield name="DEVICE_ID" range="15..0" type="integer">
//       <info>
//          Device ID.
//       </info>
//     </bitfield>
//   </register>
//
//   <register name="CORE_SW_RESETS" size="32" offset="0x04" attributes="Writable">
//     <info>
//       Controls soft-resets across the FPGA
//     </info>
//     <bitfield name="RADIO_CLK_GEN_RST" range="0">
//       <info>
//          Resets clocking resources for radio clock generation.
//       </info>
//     </bitfield>
//   </register>
//
//   <register name="CORE_CLOCK_CTRL" size="32" offset="0x08" attributes="Writable">
//     <info>
//       Miscellaneous clock control register.
//     </info>
//     <bitfield name="REF_CLK_SOURCE" range="0" initialvalue="1">
//       <info>
//          Selects the reference clock source. 0 = GPSDO, 1 = Internal.
//       </info>
//     </bitfield>
//     <bitfield name="LMK_SOURCE_SELECT" range="4">
//       <info>
//          Selects the LMK clock source. 0 = 122.88MHz, 1 = 125MHz
//       </info>
//     </bitfield>
//     <bitfield name="TCXO_EN" range="8">
//       <info>
//          Enable external TCXO oscillator.
//       </info>
//     </bitfield>
//     <bitfield name="LMK_RESET" range="12">
//       <info>
//          Controls the reset pin on the LMK clock generator.
//       </info>
//     </bitfield>
//     <bitfield name="LMK05318_PD_N" range="13">
//       <info>
//          Controls the power down pin to the LMK05318 chip.  0 = power down. 1 = power up.
//       </info>
//     </bitfield>
//     <bitfield name="USE_EXTERNAL_PPS" range="16">
//       <info>
//          Selects if the FPGA should use the internal PPS signal for its timekeeping.
//       </info>
//     </bitfield>
//   </register>
//
//   <register name="CORE_CLOCK_STATUS" size="32" offset="0x0C" attributes="Readable">
//     <info>
//       Miscellaneous clock status register.
//     </info>
//     <bitfield name="LMK_LOCK_STATUS" range="8">
//       <info>
//          Reports the lock status of the LMK clock generator.
//       </info>
//     </bitfield>
//   </register>
//
//   <register name="DEVICE_DNA_LOW" size="32" offset="0x10" attributes="Readable">
//     <info>
//       Holds the lower 32 bits of the device DNA.
//     </info>
//     <bitfield name="DEV_DNA_LO" range="31..0">
//       <info>
//          DNA low bits.
//       </info>
//     </bitfield>
//   </register>
//
//   <register name="DEVICE_DNA_HIGH" size="32" offset="0x14" attributes="Readable">
//     <info>
//       Holds the upper 32 bits of the device DNA.
//     </info>
//     <bitfield name="DEV_DNA_HI" range="31..0">
//       <info>
//          DNA high bits.
//       </info>
//     </bitfield>
//   </register>
//
//   <register name="CORE_COMPAT_NUM" size="32" offset="0x18" attributes="Readable">
//     <info>
//       Holds the compatibility number for the core.
//     </info>
//     <bitfield name="CORE_MAJOR_COMPAT" range="31..16" type="integer">
//       <info>
//          Major compatibility number.
//       </info>
//     </bitfield>
//    <bitfield name="CORE_MINOR_COMPAT" range="15..0" type="integer">
//       <info>
//          Minor compatibility number.
//       </info>
//     </bitfield>
//   </register>
//
//   <register name="CORE_RFNOC_INFO" size="32" offset="0x1C" attributes="Readable">
//     <info>
//       Holds the RFNoC Core settings.
//     </info>
//     <bitfield name="RFNOC_PROTO_VERSION" range="15..0" type="integer">
//       <info>
//          RFNoC protocol version.
//       </info>
//     </bitfield>
//    <bitfield name="RFNOC_CHDR_WIDTH" range="31..16" type="integer">
//       <info>
//          RFNoC CHDR width.
//       </info>
//     </bitfield>
//   </register>
//
//   <register name="RADIO_TIME_LOW" size="32" offset="0x20" attributes="Readable">
//     <info>
//       Holds the lower 32 bits of the current radio time. Read the LOW
//       register followed by the HIGH register to get a coherent 64-bit value.
//     </info>
//   </register>
//
//   <register name="RADIO_TIME_HIGH" size="32" offset="0x24" attributes="Readable">
//     <info>
//       Holds the upper 32 bits of the radio time from when the LOW register
//       was read. Read the LOW register followed by the HIGH register to get a
//       coherent 64-bit value.
//     </info>
//   </register>
//
//   <register name="INT_PPS_DIVIDER" size="32" offset="0x28" attributes="Writable|Readable">
//     <info>
//       Controls the divider for the internal PPS generator.
//     </info>
//     <bitfield name="INT_PPS_DIV" range="31..0" type="integer" initialvalue="122880000">
//       <info>
//          Divider value for internal PPS generator.
//       </info>
//     </bitfield>
//   </register>
//
//   <register name="CORE_NUM_TIMEKEEPERS" size="32" offset="0x2C" attributes="Readable">
//     <info>
//       Reports the number of timekeepers in the system.
//     </info>
//     <bitfield name="CORE_NUM_TIMEKEEPERS_VAL" range="31..0" type="integer">
//       <info>
//          Number of timekeepers in the system.
//       </info>
//     </bitfield>
//   </register>
//
//   <register name="CORE_BUILD_SEED" size="32" offset="0x30" attributes="Readable">
//     <info>
//       Holds the build seed for the current FPGA build.
//     </info>
//     <bitfield name="CORE_BUILD_SEED_VAL" range="31..0" type="integer">
//       <info>
//          Build seed value.
//       </info>
//     </bitfield>
//   </register>
//
//   <register name="FP_GPIO_SRC" size="32" offset="0x34" attributes="Writable|Readable">
//     <info>
//       Controls the source of the front panel GPIOs.
//     </info>
//     <bitfield name="FP_GPIO0_SRC" range="1..0">
//       <info>
//          Source for front panel GPIO0. 0 = CH0_ATR, 1 = CH1_ATR.
//       </info>
//     </bitfield>
//     <bitfield name="FP_GPIO1_SRC" range="3..2">
//       <info>
//          Source for front panel GPIO1. 0 = CH0_ATR, 1 = CH1_ATR.
//       </info>
//     </bitfield>
//     <bitfield name="FP_GPIO2_SRC" range="5..4">
//       <info>
//          Source for front panel GPIO2. 0 = CH0_ATR, 1 = CH1_ATR.
//       </info>
//     </bitfield>
//     <bitfield name="FP_GPIO3_SRC" range="7..6">
//       <info>
//          Source for front panel GPIO3. 0 = CH0_ATR, 1 = CH1_ATR.
//       </info>
//     </bitfield>
//     <bitfield name="FP_GPIO4_SRC" range="9..8">
//       <info>
//          Source for front panel GPIO4. 0 = CH0_ATR, 1 = CH1_ATR.
//       </info>
//     </bitfield>
//     <bitfield name="FP_GPIO5_SRC" range="11..10">
//       <info>
//          Source for front panel GPIO5. 0 = CH0_ATR, 1 = CH1_ATR.
//       </info>
//     </bitfield>
//     <bitfield name="FP_GPIO6_SRC" range="13..12">
//       <info>
//          Source for front panel GPIO6. 0 = CH0_ATR, 1 = CH1_ATR.
//       </info>
//     </bitfield>
//     <bitfield name="FP_GPIO7_SRC" range="15..14">
//       <info>
//          Source for front panel GPIO7. 0 = CH0_ATR, 1 = CH1_ATR.
//       </info>
//     </bitfield>
//     <bitfield name="FP_GPIO8_SRC" range="17..16">
//       <info>
//          Source for front panel GPIO8. 0 = CH0_ATR, 1 = CH1_ATR.
//       </info>
//     </bitfield>
//     <bitfield name="FP_GPIO9_SRC" range="19..18">
//       <info>
//          Source for front panel GPIO9. 0 = CH0_ATR, 1 = CH1_ATR.
//       </info>
//     </bitfield>
//   </register>
//
//   <register name="GPS_CTRL" size="32" offset="0x38" attributes="Writable|Readable">
//     <info>
//       Controls GPIO lines to the GPS circuits.
//     </info>
//     <bitfield name="REG_GPS_RESET_N" range="0">
//       <info>
//          0 = GPS in reset.  1 = GPS out of reset.
//       </info>
//     </bitfield>
//     <bitfield name="REG_GPS_ANT_PWR_EN" range="1">
//       <info>
//          0 = GPS antenna power disabled.  1 = GPS antenna power enabled.
//       </info>
//     </bitfield>
//     <bitfield name="GPS_PW_FAULT" range="4" attributes="Readable">
//       <info>
//          Read-only bit that indicates an overcurrent fault on the GPS power
//          for active antennas.
//       </info>
//     </bitfield>
//     <bitfield name="GPS_LMK_PPS_MONITOR" range="8" attributes="Readable">
//       <info>
//          This bit toggles on the rising edge of the GPS LMK PPS signal.
//          Can be used to monitor the GPS LMK PPS signal in software.
//       </info>
//     </bitfield>
//     <bitfield name="GPS_LMK_STATUS" range="13..12" attributes="Readable">
//       <info>
//          Reports the state of the status indicators from the LMK05318.
//       </info>
//     </bitfield>
//    <bitfield name="GPS_LMK_GPIO" range="16">
//       <info>
//          Controls output to the LMK05318's GPIO0/SYNCN line.
//       </info>
//     </bitfield>
//     <bitfield name="GPS_BYPASS_LMK" range="20">
//       <info>
//          Allows using the PPS signal directly from the GPS chip instead of the LMK05318 when
//          using the GPSDO.
//            0 = LMK05318 is used to generate the GPS PPS signal.
//            1 = Use PPS signal from an GPS chip.
//       </info>
//     </bitfield>
//   </register>
//
//   <register name="TEMP_MON" size="32" offset="0x3C" attributes="Readable">
//     <info>
//       Reports values from the FPGA temperature monitor.
//     </info>
//     <bitfield name="DEVICE_TEMP" range="11..0" type="integer">
//       <info>
//          Current FPGA temperature. To convert to degrees Celsius, use the
//          formula: Temp (C) = [(DEVICE_TEMP x 503.975) / 4096 - 273.15]
//       </info>
//     </bitfield>
//   </register>
//
//  <register name="LMK_SYNC_CTRL" size="32" offset="0x40" attributes="Writable|Readable">
//     <info>
//     </info>
//     <bitfield name="LMK_CLKIN0_SYNC_SEL" range="31">
//       <info>
//        REVA sets this to 0 to select SYNC pin as syc pin with not as good
//        setup/hold time.
//        REVB sets this to 1 to select ClkIN0 pin as sync pin with better
//        setup/hold time.  When setting this pin to 1, then 1 needs
//        subtracted from the @PPS_IN_TO_RCLK_DELAY field as the LMK has one
//        register delay on ClkIN0 vs Sync.
//       </info>
//     </bitfield>
//     <bitfield name="LMK_SYNC_CLK_SEL" range="30">
//       <info>
//        Set to 0 to make lmk sync output synchronous to radio_clk (for clock distribution mode)
//        Set to 1 to make lmk sync output synchronous to ref_clk (for ext 10MHz ref in)
//        - "0" needed for clock distribution mode to meet setup/hold of flip
//        flop inside the LMK04832 synchronous to ref_clk.  Not intuitive this
//        is needed, but reason why timing is better is radio_clk is from
//        a pll where ref_clk is from an IO buffer with large delay.
//        - "1" needed for external 10MHz mode because during synchronization
//        the radio_clk looses lock due to clk0 being reset.  Thus sync needs
//        driven from active ref_clk.  It is easy to meet timing to LMK with
//        10MHz clock due to it having a 100ns period.
//       </info>
//     </bitfield>
//     <bitfield name="LMK_SYNC_DONE" range="29" attributes = "Readable">
//        <info>Indicates the success of the PLL reset started by @.PLL_SYNC_TRIGGER. Reset on deassertion of @.PLL_SYNC_TRIGGER.</info>
//     </bitfield>
//     <bitfield name="LMK_SYNC_TRIGGER" range="28">
//       <info>
//          Assertion triggers the SYNC signal generation for LMK04832 after the next appearance of the PPS rising edge.
//          There is no self reset on this trigger.
//          Keep this trigger asserted until @.PLL_SYNC_DONE is asserted.
//       </info>
//     </bitfield>
//     <bitfield name="LMK_SYNC_DELAY" range="26..0">
//       <info>
//       Controls the delay from PPS_IN to LMK_SYNC being sent to the LMK
//       chip in in ext_ref_clk cycles.
//       </info>
//     </bitfield>
//   </register>
//
//   <register name="PPS_IN_CTRL" size="32" offset="0x44" attributes="Writable|Readable">
//     <info>
//     </info>
//     <bitfield name="PPS_IN_TO_RCLK_DELAY" range="9..0">
//       Due to the HDL implementation the rising edge of the SYNC signal for
//       the LMK04832 is generated 2 clock cycles after the PPS rising edge.
//       This delay can be further increased by setting this delay value
//       (e.g. PLL_SYNC_DELAY=3 will result in a total delay of 5 clock cycles).{br}
//
//       Controls the delay from PPS_IN to radio clock PPS IN, in ext_ref_clk
//       cycles.  Sync gets sent to LMK, LMK clears its output counters to
//       zero.  DEV_CLK/radio_clk start toggling now synchronized to the
//       10Mhz ref_clk edge.  Every PPS signal DEV_CLK/radio_clk will
//       have the same relationship, but offset in phase.
//       Delay PPS this many 10Mhz ext_ref_clk cycles such that its
//       rising edge aligns with the radio_clk domain.  Internal PLL divider
//       is 250 for 10MHz clock, therefore 250 is a typical delay for this.
//       Set this register to 248 to get zero offset between the two clocks.
//       We however don't want 0 offset, so we set to 251 instead (253 actual
//       delay) to cause radio_clk to be 7.03125ns out of 8.128021ns for an
//       offset of 1.10677ns.
//       In practice it is off slightly to account for extra RTL delays.
//     </bitfield>
//   </register>
//   <register name="TBOLT_CTRL" size="32" offset="0x48" attributes="Writable|Readable">
//     <info>
//       Control register for Thunderbolt controller resets.
//     </info>
//     <bitfield name="TBOLT_PD_CTRL_PULSE_RST" range="0" attributes="Strobe">
//       <info>
//          Writing a 1 to this bit will pulse the reset line to the Thunderbolt PD
//          controller, forcing the FPGA to reconfigure. There's is a 500ms delay
//          before the pulse is generated to allow software to service the write
//          transaction and return an ACK before the FPGA resets.
//       </info>
//     </bitfield>
//   </register>
//   <register name="POWER_CONTROL_STS" size="32" offset="0x4C" attributes="Writable|Readable">
//     <info>
//       Control/status register for power signals
//     </info>
//     <bitfield name="POWER_GOOD_1V2" range="0" attributes="Readable">
//       <info>
//          Status of 1.2V power supply.
//       </info>
//     </bitfield>
//     <bitfield name="POWER_MONITOR_ALERT" range="1" attributes="Readable">
//       <info>
//          Indicates if the INA219 power monitor has asserted an alert.
//       </info>
//     </bitfield>
//     <bitfield name="POWER_25W_SRC" range="4" attributes="Readable">
//       <info>
//          Indicates if an external source is providing 25W of power to the device.
//          0: Power is provided by PCIe slot, 1: Power is provided by external source.
//       </info>
//     </bitfield>
//     <bitfield name="TYPEC_POWER_NEGOTIATED" range="5" attributes="Readable">
//       <info>
//          Indicates if the USB Type-C power negotiation has completed successfully.
//       </info>
//     </bitfield>
//     <bitfield name="POWER_LED_OVERRIDE" range="8" attributes="Writable|Readable">
//       <info>
//          Control for power status LED.
//            0 = Blue if powered up, 1 = force orange.
//       </info>
//     </bitfield>
//   </register>
//
//  </group>
//</regmap>
//XmlParse xml_off

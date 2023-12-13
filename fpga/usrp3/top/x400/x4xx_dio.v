//
// Copyright 2021 Ettus Research, A National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: x4xx_dio
//
// Description:
//
//   This module contains the motherboard registers for the DIO
//   auxiliary board and the logic to drive these GPIO signals.
//   Arbitration between different sources to control the state
//   of the GPIO lines includes support for the following sources:
//      - s_ctrlport_* (combination of CtrlPort from PS AXI and radio blocks)
//      - PS dio control from Processing System's GPIOs
//      - ATR state (up to two DBs)
//      - User Application
//      - radio-controlled digital bus interface
//   For a visual representation of how the different sources are
//   arbitrated, as well as representation on what each source control
//   register refers to, please refer to the "Front-Panel Programmable
//   GPIOs" section of the USRP Manual.
//
// Parameters:
//
//   REG_BASE    : Base address to use for registers.
//   REG_SIZE    : Register space size.
//   NUM_DBOARDS : Number of daughterboards to support.
//

`default_nettype none


module x4xx_dio #(
  parameter REG_BASE = 0,
  parameter REG_SIZE = 'h30,
  parameter NUM_DBOARDS = 2
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

  // GPIO to DIO board (ctrlport_clk)
  output wire [11:0] gpio_en_a,
  output wire [11:0] gpio_en_b,

  // GPIO to DIO board (async)
  input  wire [11:0] gpio_in_a,
  input  wire [11:0] gpio_in_b,
  output wire [11:0] gpio_out_a,
  output wire [11:0] gpio_out_b,

  // ATR GPIO Control (ctrlport_clk)
  input wire [NUM_DBOARDS*32-1:0] atr_gpio_out,
  input wire [NUM_DBOARDS*32-1:0] atr_gpio_ddr,

  // PS GPIO Control from Block Design (async)
  input wire [31:0] ps_gpio_out,
  input wire [31:0] ps_gpio_ddr,

  // Digital Interface Control (ctrlport_clk)
  input wire [31:0] digital_ifc_gpio_out_radio0,
  input wire [31:0] digital_ifc_gpio_ddr_radio0,
  input wire [31:0] digital_ifc_gpio_out_radio1,
  input wire [31:0] digital_ifc_gpio_ddr_radio1,

  // GPIO to user application (async)
  // User application relies on the local direction register
  // for GPIO direction control. For this reason, we skip
  // a mux to select between the user application and the
  // local register direction control in the mux chain, and
  // propagate their shared direction(from the local register)
  // to the remainder of the mux chain.
  output wire [11:0] user_app_in_a,
  output wire [11:0] user_app_in_b,
  input  wire [11:0] user_app_out_a,
  input  wire [11:0] user_app_out_b
);

  `include "../../lib/rfnoc/core/ctrlport.vh"
  `include "regmap/dio_regmap_utils.vh"

  //---------------------------------------------------------------------------
  // Constants
  //---------------------------------------------------------------------------

  localparam DIO_WIDTH = 12;


  //---------------------------------------------------------------------------
  // DIO Registers
  //---------------------------------------------------------------------------

  reg  [DIO_WIDTH-1:0] dio_direction_a = {DIO_WIDTH {1'b0}};
  reg  [DIO_WIDTH-1:0] dio_direction_b = {DIO_WIDTH {1'b0}};
  reg  [DIO_WIDTH-1:0] dio_master_a    = {DIO_WIDTH {1'b0}};
  reg  [DIO_WIDTH-1:0] dio_master_b    = {DIO_WIDTH {1'b0}};
  reg  [DIO_WIDTH-1:0] dio_output_a    = {DIO_WIDTH {1'b0}};
  reg  [DIO_WIDTH-1:0] dio_output_b    = {DIO_WIDTH {1'b0}};
  reg  [DIO_WIDTH-1:0] dio_source_a    = {DIO_WIDTH {1'b0}};
  reg  [DIO_WIDTH-1:0] dio_source_b    = {DIO_WIDTH {1'b0}};
  reg  [DIO_WIDTH-1:0] dio_radio_a     = {DIO_WIDTH {1'b0}};
  reg  [DIO_WIDTH-1:0] dio_radio_b     = {DIO_WIDTH {1'b0}};
  reg  [DIO_WIDTH-1:0] dio_interface_a = {DIO_WIDTH {1'b0}};
  reg  [DIO_WIDTH-1:0] dio_interface_b = {DIO_WIDTH {1'b0}};
  reg  [DIO_WIDTH-1:0] dio_override_a  = {DIO_WIDTH {1'b0}};
  reg  [DIO_WIDTH-1:0] dio_override_b  = {DIO_WIDTH {1'b0}};
  reg  [DIO_WIDTH-1:0] dio_sw_ctrl_a   = {DIO_WIDTH {1'b0}};
  reg  [DIO_WIDTH-1:0] dio_sw_ctrl_b   = {DIO_WIDTH {1'b0}};
  wire [DIO_WIDTH-1:0] dio_input_a;
  wire [DIO_WIDTH-1:0] dio_input_b;


  //---------------------------------------------------------------------------
  // Control interface handling
  //---------------------------------------------------------------------------

  // Check that address is within this module's range.
  wire address_in_range = (s_ctrlport_req_addr >= REG_BASE) && (s_ctrlport_req_addr < REG_BASE + REG_SIZE);

  always @ (posedge ctrlport_clk) begin
    if (ctrlport_rst) begin
      s_ctrlport_resp_ack    <= 1'b0;
      s_ctrlport_resp_data   <= {32 {1'bX}};
      s_ctrlport_resp_status <= 2'b00;

      dio_direction_a <= {DIO_WIDTH {1'b0}};
      dio_direction_b <= {DIO_WIDTH {1'b0}};
      dio_master_a    <= {DIO_WIDTH {1'b0}};
      dio_master_b    <= {DIO_WIDTH {1'b0}};
      dio_output_a    <= {DIO_WIDTH {1'b0}};
      dio_output_b    <= {DIO_WIDTH {1'b0}};
      dio_source_a    <= {DIO_WIDTH {1'b0}};
      dio_source_b    <= {DIO_WIDTH {1'b0}};
      dio_radio_a     <= {DIO_WIDTH {1'b0}};
      dio_radio_b     <= {DIO_WIDTH {1'b0}};
      dio_interface_a <= {DIO_WIDTH {1'b0}};
      dio_interface_b <= {DIO_WIDTH {1'b0}};
      dio_override_a  <= {DIO_WIDTH {1'b0}};
      dio_override_b  <= {DIO_WIDTH {1'b0}};
      dio_sw_ctrl_a   <= {DIO_WIDTH {1'b0}};
      dio_sw_ctrl_b   <= {DIO_WIDTH {1'b0}};

    end else begin
      // Write registers
      if (s_ctrlport_req_wr) begin
        // Acknowledge by default
        s_ctrlport_resp_ack    <= 1'b1;
        s_ctrlport_resp_data   <= {CTRLPORT_DATA_W {1'b0}};
        s_ctrlport_resp_status <= CTRL_STS_OKAY;

        case (s_ctrlport_req_addr)
          REG_BASE + DIO_MASTER_REGISTER: begin
            dio_master_a <= s_ctrlport_req_data[DIO_PORT_A_MSB:DIO_PORT_A];
            dio_master_b <= s_ctrlport_req_data[DIO_PORT_B_MSB:DIO_PORT_B];
          end

          REG_BASE + DIO_DIRECTION_REGISTER: begin
            dio_direction_a <= s_ctrlport_req_data[DIO_PORT_A_MSB:DIO_PORT_A];
            dio_direction_b <= s_ctrlport_req_data[DIO_PORT_B_MSB:DIO_PORT_B];
          end

          REG_BASE + DIO_OUTPUT_REGISTER: begin
            dio_output_a <= s_ctrlport_req_data[DIO_PORT_A_MSB:DIO_PORT_A];
            dio_output_b <= s_ctrlport_req_data[DIO_PORT_B_MSB:DIO_PORT_B];
          end

          REG_BASE + DIO_SOURCE_REGISTER: begin
            dio_source_a <= s_ctrlport_req_data[DIO_PORT_A_MSB:DIO_PORT_A];
            dio_source_b <= s_ctrlport_req_data[DIO_PORT_B_MSB:DIO_PORT_B];
          end

          REG_BASE + RADIO_SOURCE_REGISTER: begin
            dio_radio_a <= s_ctrlport_req_data[DIO_PORT_A_MSB:DIO_PORT_A];
            dio_radio_b <= s_ctrlport_req_data[DIO_PORT_B_MSB:DIO_PORT_B];
          end

          REG_BASE + INTERFACE_DIO_SELECT: begin
            dio_interface_a <= s_ctrlport_req_data[DIO_PORT_A_MSB:DIO_PORT_A];
            dio_interface_b <= s_ctrlport_req_data[DIO_PORT_B_MSB:DIO_PORT_B];
          end

          REG_BASE + DIO_OVERRIDE: begin
            dio_override_a <= s_ctrlport_req_data[DIO_PORT_A_MSB:DIO_PORT_A];
            dio_override_b <= s_ctrlport_req_data[DIO_PORT_B_MSB:DIO_PORT_B];
          end

          REG_BASE + SW_DIO_CONTROL: begin
            dio_sw_ctrl_a <= s_ctrlport_req_data[DIO_PORT_A_MSB:DIO_PORT_A];
            dio_sw_ctrl_b <= s_ctrlport_req_data[DIO_PORT_B_MSB:DIO_PORT_B];
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
        s_ctrlport_resp_data   <= {CTRLPORT_DATA_W {1'b0}};
        s_ctrlport_resp_status <= CTRL_STS_OKAY;

        case (s_ctrlport_req_addr)
          REG_BASE + DIO_MASTER_REGISTER: begin
            s_ctrlport_resp_data[DIO_PORT_A_MSB:DIO_PORT_A] <= dio_master_a;
            s_ctrlport_resp_data[DIO_PORT_B_MSB:DIO_PORT_B] <= dio_master_b;
          end

          REG_BASE + DIO_DIRECTION_REGISTER: begin
            s_ctrlport_resp_data[DIO_PORT_A_MSB:DIO_PORT_A] <= dio_direction_a;
            s_ctrlport_resp_data[DIO_PORT_B_MSB:DIO_PORT_B] <= dio_direction_b;
          end

          REG_BASE + DIO_OUTPUT_REGISTER: begin
            s_ctrlport_resp_data[DIO_PORT_A_MSB:DIO_PORT_A] <= dio_output_a;
            s_ctrlport_resp_data[DIO_PORT_B_MSB:DIO_PORT_B] <= dio_output_b;
          end

          REG_BASE + DIO_INPUT_REGISTER: begin
            s_ctrlport_resp_data[DIO_PORT_A_MSB:DIO_PORT_A] <= dio_input_a;
            s_ctrlport_resp_data[DIO_PORT_B_MSB:DIO_PORT_B] <= dio_input_b;
          end

          REG_BASE + DIO_SOURCE_REGISTER: begin
            s_ctrlport_resp_data[DIO_PORT_A_MSB:DIO_PORT_A] <= dio_source_a;
            s_ctrlport_resp_data[DIO_PORT_B_MSB:DIO_PORT_B] <= dio_source_b;
          end

          REG_BASE + RADIO_SOURCE_REGISTER: begin
            s_ctrlport_resp_data[DIO_PORT_A_MSB:DIO_PORT_A] <= dio_radio_a;
            s_ctrlport_resp_data[DIO_PORT_B_MSB:DIO_PORT_B] <= dio_radio_b;
          end

          REG_BASE + INTERFACE_DIO_SELECT: begin
            s_ctrlport_resp_data[DIO_PORT_A_MSB:DIO_PORT_A] <= dio_interface_a;
            s_ctrlport_resp_data[DIO_PORT_B_MSB:DIO_PORT_B] <= dio_interface_b;
          end

          REG_BASE + DIO_OVERRIDE: begin
            s_ctrlport_resp_data[DIO_PORT_A_MSB:DIO_PORT_A] <= dio_override_a;
            s_ctrlport_resp_data[DIO_PORT_B_MSB:DIO_PORT_B] <= dio_override_b;
          end

          REG_BASE + SW_DIO_CONTROL: begin
            s_ctrlport_resp_data[DIO_PORT_A_MSB:DIO_PORT_A] <= dio_sw_ctrl_a;
            s_ctrlport_resp_data[DIO_PORT_B_MSB:DIO_PORT_B] <= dio_sw_ctrl_b;
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


  //---------------------------------------------------------------------------
  // DIO handling
  //---------------------------------------------------------------------------

  // Synchronizer for asynchronous inputs.
  // Downstream user logic has to ensure bus coherency if required.
  synchronizer #(
    .WIDTH            (DIO_WIDTH*2),
    .STAGES           (2),
    .INITIAL_VAL      ({DIO_WIDTH*2 {1'b0}}),
    .FALSE_PATH_TO_IN (1)
  ) synchronizer_dio (
    .clk (ctrlport_clk),
    .rst (ctrlport_rst),
    .in  ({gpio_in_a, gpio_in_b}),
    .out ({dio_input_a, dio_input_b})
  );

  // Forward raw input to user application
  assign user_app_in_a = gpio_in_a;
  assign user_app_in_b = gpio_in_b;

  wire  [DIO_WIDTH-1:0] gpio_out_sw_a;
  wire  [DIO_WIDTH-1:0] gpio_out_sw_b;

  // Output assignment depending on master
  generate
    genvar i;
    for (i = 0; i < DIO_WIDTH; i = i + 1) begin: dio_output_gen

      reg atr_gpio_src_out_a_reg = 1'b0;
      reg atr_gpio_src_out_b_reg = 1'b0;
      reg atr_gpio_src_ddr_a_reg = 1'b0;
      reg atr_gpio_src_ddr_b_reg = 1'b0;

      // 1) Select which radio drives the output
      always @ (posedge ctrlport_clk) begin
        if (ctrlport_rst) begin
          atr_gpio_src_out_a_reg <= 1'b0;
          atr_gpio_src_out_b_reg <= 1'b0;
        end else begin
          atr_gpio_src_out_a_reg <= atr_gpio_out[dio_radio_a[i]*32 + DIO_PORT_A + i];
          atr_gpio_src_out_b_reg <= atr_gpio_out[dio_radio_b[i]*32 + DIO_PORT_B + i];
        end
      end

      // 2) Select which radio drives the direction
      always @ (posedge ctrlport_clk) begin
        if (ctrlport_rst) begin
          atr_gpio_src_ddr_a_reg <= 1'b0;
          atr_gpio_src_ddr_b_reg <= 1'b0;
        end else begin
          atr_gpio_src_ddr_a_reg <= atr_gpio_ddr[dio_radio_a[i]*32 + DIO_PORT_A + i];
          atr_gpio_src_ddr_b_reg <= atr_gpio_ddr[dio_radio_b[i]*32 + DIO_PORT_B + i];
        end
      end

      // Select between the Digital Interface in each radio(INTERFACE_DIO_SELECT)
      wire dio_interface_mux_out_a;
      wire dio_interface_mux_out_b;
      wire dio_interface_mux_ddr_a;
      wire dio_interface_mux_ddr_b;

      glitch_free_mux glitch_free_interface_out_mux_dio_a (
        .select       (dio_interface_a[i]),
        .signal0      (digital_ifc_gpio_out_radio0[DIO_PORT_A + i]),
        .signal1      (digital_ifc_gpio_out_radio1[DIO_PORT_A + i]),
        .muxed_signal (dio_interface_mux_out_a)
      );

      glitch_free_mux glitch_free_interface_out_mux_dio_b (
        .select       (dio_interface_b[i]),
        .signal0      (digital_ifc_gpio_out_radio0[DIO_PORT_B + i]),
        .signal1      (digital_ifc_gpio_out_radio1[DIO_PORT_B + i]),
        .muxed_signal (dio_interface_mux_out_b)
      );

      glitch_free_mux glitch_free_interface_ddr_mux_dio_a (
        .select       (dio_interface_a[i]),
        .signal0      (digital_ifc_gpio_ddr_radio0[DIO_PORT_A + i]),
        .signal1      (digital_ifc_gpio_ddr_radio1[DIO_PORT_A + i]),
        .muxed_signal (dio_interface_mux_ddr_a)
      );

      glitch_free_mux glitch_free_interface_ddr_mux_dio_b (
        .select       (dio_interface_b[i]),
        .signal0      (digital_ifc_gpio_ddr_radio0[DIO_PORT_B + i]),
        .signal1      (digital_ifc_gpio_ddr_radio1[DIO_PORT_B + i]),
        .muxed_signal (dio_interface_mux_ddr_b)
      );


      // Select between ATR or Digital control(DIO_OVERRIDE)
      wire dio_override_mux_out_a;
      wire dio_override_mux_out_b;
      wire dio_override_mux_ddr_a;
      wire dio_override_mux_ddr_b;

      glitch_free_mux glitch_free_override_out_mux_dio_a (
        .select       (dio_override_a[i]),
        .signal0      (atr_gpio_src_out_a_reg),
        .signal1      (dio_interface_mux_out_a),
        .muxed_signal (dio_override_mux_out_a)
      );

      glitch_free_mux glitch_free_override_out_mux_dio_b (
        .select       (dio_override_b[i]),
        .signal0      (atr_gpio_src_out_b_reg),
        .signal1      (dio_interface_mux_out_b),
        .muxed_signal (dio_override_mux_out_b)
      );

      glitch_free_mux glitch_free_override_ddr_mux_dio_a (
        .select       (dio_override_a[i]),
        .signal0      (atr_gpio_src_ddr_a_reg),
        .signal1      (dio_interface_mux_ddr_a),
        .muxed_signal (dio_override_mux_ddr_a)
      );

      glitch_free_mux glitch_free_override_ddr_mux_dio_b (
        .select       (dio_override_b[i]),
        .signal0      (atr_gpio_src_ddr_b_reg),
        .signal1      (dio_interface_mux_ddr_b),
        .muxed_signal (dio_override_mux_ddr_b)
      );


      // SW source select
      // SW_DIO_CONTROL, select between PS and local register

      wire dio_sw_control_mux_out_a;
      wire dio_sw_control_mux_out_b;
      wire dio_sw_control_mux_ddr_a;
      wire dio_sw_control_mux_ddr_b;

      glitch_free_mux glitch_free_sw_control_out_mux_dio_a (
        .select       (dio_sw_ctrl_a[i]),
        .signal0      (dio_output_a[i]),
        .signal1      (ps_gpio_out[DIO_PORT_A + i]),
        .muxed_signal (dio_sw_control_mux_out_a)
      );

      glitch_free_mux glitch_free_sw_control_out_mux_dio_b (
        .select       (dio_sw_ctrl_b[i]),
        .signal0      (dio_output_b[i]),
        .signal1      (ps_gpio_out[DIO_PORT_B + i]),
        .muxed_signal (dio_sw_control_mux_out_b)
      );

      glitch_free_mux glitch_free_sw_control_ddr_mux_dio_a (
        .select       (dio_sw_ctrl_a[i]),
        .signal0      (dio_direction_a[i]),
        .signal1      (ps_gpio_ddr[DIO_PORT_A + i]),
        .muxed_signal (dio_sw_control_mux_ddr_a)
      );

      glitch_free_mux glitch_free_sw_control_ddr_mux_dio_b (
        .select       (dio_sw_ctrl_b[i]),
        .signal0      (dio_direction_b[i]),
        .signal1      (ps_gpio_ddr[DIO_PORT_B + i]),
        .muxed_signal (dio_sw_control_mux_ddr_b)
      );


      // DIO_MASTER_REGISTER Mux, select between SW_DIO_CONTROL
      // and user application
      // User application relies on the local direction register
      // for GPIO direction control. For this reason, we skip
      // a mux to select between the user application and the
      // local register direction control in the mux chain, and
      // propagate their shared direction(from the local register)
      // to the remainder of the mux chain.
      glitch_free_mux glitch_free_master_mux_dio_a (
        .select       (dio_master_a[i]),
        .signal0      (user_app_out_a[i]),
        .signal1      (dio_sw_control_mux_out_a),
        .muxed_signal (gpio_out_sw_a[i])
      );

      glitch_free_mux glitch_free_master_mux_dio_b (
        .select       (dio_master_b[i]),
        .signal0      (user_app_out_b[i]),
        .signal1      (dio_sw_control_mux_out_b),
        .muxed_signal (gpio_out_sw_b[i])
      );

      // DIO_SOURCE_REGISTER mux, select between (DIO_MASTER_REGISTER output) and
      // radio controlled source (DIO_OVERRIDE output).
      glitch_free_mux glitch_free_source_out_mux_dio_a (
        .select       (dio_source_a[i]),
        .signal0      (gpio_out_sw_a[i]),
        .signal1      (dio_override_mux_out_a),
        .muxed_signal (gpio_out_a[i])
      );

      glitch_free_mux glitch_free_source_out_mux_dio_b (
        .select       (dio_source_b[i]),
        .signal0      (gpio_out_sw_b[i]),
        .signal1      (dio_override_mux_out_b),
        .muxed_signal (gpio_out_b[i])
      );

      // Direction control
      glitch_free_mux glitch_free_dir_mux_dio_a (
        .select       (dio_source_a[i]),
        .signal0      (dio_sw_control_mux_ddr_a),
        .signal1      (dio_override_mux_ddr_a),
        .muxed_signal (gpio_en_a[i])
      );

      glitch_free_mux glitch_free_dir_mux_dio_b (
        .select       (dio_source_b[i]),
        .signal0      (dio_sw_control_mux_ddr_b),
        .signal1      (dio_override_mux_ddr_b),
        .muxed_signal (gpio_en_b[i])
      );
    end

  endgenerate

endmodule


`default_nettype wire


//XmlParse xml_on
//<regmap name="DIO_REGMAP" readablestrobes="false" generatevhdl="true" ettusguidelines="true">
//  <group name="DIO_REGS">
//    <info>
//      Registers to control the GPIO buffer direction on the FPGA connected to
//      the DIO board. Further registers enable different sources to control and
//      read the GPIO lines as master. The following diagram shows how source
//      selection multiplexers are arranged, as well as an indicator for the
//      register that control them. </br>
//      <img src = "..\..\..\..\..\..\host\docs\res\x4xx_dio_source_muxes.svg"
//          alt="Front-Panel Programmable GPIOs"/></br>
//      Make sure the GPIO lines between FPGA and GPIO board are not driven by
//      two drivers. Set the DIO registers in @.PS_CPLD_BASE_REGMAP appropriately.
//    </info>
//
//    <regtype name="DIO_CONTROL_REG" size="32">
//      <info>
//        Holds a single bit setting for DIO lines in both ports. One bit per pin.
//      </info>
//      <bitfield name="DIO_PORT_A" range="0..11" initialvalue="0"/>
//      <bitfield name="DIO_PORT_B" range="16..27" initialvalue="0"/>
//    </regtype>
//
//    <register name="DIO_MASTER_REGISTER" offset="0x00" typename="DIO_CONTROL_REG">
//      <info>
//        Sets whether the DIO signal line is driven by this register interface
//        or the user application.{br/}
//        0 = user application is master, 1 = output of @.SW_DIO_CONTROL is master
//      </info>
//    </register>
//    <register name="DIO_DIRECTION_REGISTER" offset="0x04" typename="DIO_CONTROL_REG">
//      <info>
//        Set the direction of FPGA buffer connected to DIO ports on the DIO board.{br/}
//        Each bit represents one signal line. 0 = line is an input to the FPGA,
//        1 = line is an output driven by the FPGA.
//      </info>
//    </register>
//    <register name="DIO_INPUT_REGISTER" offset="0x08" typename="DIO_CONTROL_REG" writable="false">
//      <info>
//        Status of each bit at the FPGA input.
//      </info>
//    </register>
//    <register name="DIO_OUTPUT_REGISTER" offset="0x0C" typename="DIO_CONTROL_REG">
//      <info>
//        Controls the values on each DIO signal line in case the line master is
//        set to PS in @.DIO_MASTER_REGISTER.
//      </info>
//    </register>
//    <register name="DIO_SOURCE_REGISTER" offset="0x10" typename="DIO_CONTROL_REG">
//      <info>
//        Controls whether the DIO lines reflect the state of @.DIO_MASTER_REGISTER
//        or the radio blocks. 0 = @.DIO_MASTER_REGISTER,
//        1 = Radio block output(@.DIO_OVERRIDE)
//      </info>
//    </register>
//    <register name="RADIO_SOURCE_REGISTER" offset="0x14" typename="DIO_CONTROL_REG">
//      <info>
//        Controls which radio block to use the ATR state from to determine the
//        state of the DIO lines.
//        0 = Radio#0
//        1 = Radio#1
//      </info>
//    </register>
//    <register name="INTERFACE_DIO_SELECT" offset="0x18" typename="DIO_CONTROL_REG">
//      <info>
//        Controls which of the two available digital interfaces controls the DIO lines.
//        0 = Digital interface from Radio#0,
//        1 = Digital Interface from Radio#1.
//      </info>
//    </register>
//    <register name="DIO_OVERRIDE" offset="0x1C" typename="DIO_CONTROL_REG">
//      <info>
//        Controls whether the radio input to the @.DIO_SOURCE_REGISTER mux
//        connects to the ATR control or a Digital interface block. The output
//        of the mux controlled by this bit goes to @.DIO_SOURCE_REGISTER.
//        0 = Drive the ATR state(@.RADIO_SOURCE_REGISTER), 1 = Drive
//        Digital interface block(Output of @.INTERFACE_DIO_SELECT).
//      </info>
//    </register>
//    <register name="SW_DIO_CONTROL" offset="0x20" typename="DIO_CONTROL_REG">
//      <info>
//        Controls which source is forwarded to the @.DIO_MASTER_REGISTER mux.
//        This configuration is applied independently for each DIO line.
//        0 = MPM Ctrlport endpoint, 1 = PS Netlist DIO signal.
//      </info>
//    </register>
//  </group>
//</regmap>
//XmlParse xml_off

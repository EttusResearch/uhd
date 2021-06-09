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
//
// Parameters:
//
//   REG_BASE : Base address to use for registers.
//

`default_nettype none


module x4xx_dio #(
  parameter REG_BASE = 0,
  parameter REG_SIZE = 'h20
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

  // GPIO to application (async)
  output wire [11:0] gpio_in_fabric_a,
  output wire [11:0] gpio_in_fabric_b,
  input  wire [11:0] gpio_out_fabric_a,
  input  wire [11:0] gpio_out_fabric_b
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

    end else begin
      // Write registers
      if (s_ctrlport_req_wr) begin
        // Acknowledge by default
        s_ctrlport_resp_ack    <= 1'b1;
        s_ctrlport_resp_data   <= {CTRLPORT_DATA_W {1'b0}};
        s_ctrlport_resp_status <= CTRL_STS_OKAY;

        case (s_ctrlport_req_addr)
          REG_BASE + DIO_MASTER_REGISTER: begin
            dio_master_a <= s_ctrlport_req_data[DIO_MASTER_A_MSB:DIO_MASTER_A];
            dio_master_b <= s_ctrlport_req_data[DIO_MASTER_B_MSB:DIO_MASTER_B];
          end

          REG_BASE + DIO_DIRECTION_REGISTER: begin
            dio_direction_a <= s_ctrlport_req_data[DIO_DIRECTION_A_MSB:DIO_DIRECTION_A];
            dio_direction_b <= s_ctrlport_req_data[DIO_DIRECTION_B_MSB:DIO_DIRECTION_B];
          end

          REG_BASE + DIO_OUTPUT_REGISTER: begin
            dio_output_a <= s_ctrlport_req_data[DIO_OUTPUT_A_MSB:DIO_OUTPUT_A];
            dio_output_b <= s_ctrlport_req_data[DIO_OUTPUT_B_MSB:DIO_OUTPUT_B];
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
            s_ctrlport_resp_data[DIO_MASTER_A_MSB:DIO_MASTER_A] <= dio_master_a;
            s_ctrlport_resp_data[DIO_MASTER_B_MSB:DIO_MASTER_B] <= dio_master_b;
          end

          REG_BASE + DIO_DIRECTION_REGISTER: begin
            s_ctrlport_resp_data[DIO_DIRECTION_A_MSB:DIO_DIRECTION_A] <= dio_direction_a;
            s_ctrlport_resp_data[DIO_DIRECTION_B_MSB:DIO_DIRECTION_B] <= dio_direction_b;
          end

          REG_BASE + DIO_OUTPUT_REGISTER: begin
            s_ctrlport_resp_data[DIO_OUTPUT_A_MSB:DIO_OUTPUT_A] <= dio_output_a;
            s_ctrlport_resp_data[DIO_OUTPUT_B_MSB:DIO_OUTPUT_B] <= dio_output_b;
          end

          REG_BASE + DIO_INPUT_REGISTER: begin
            s_ctrlport_resp_data[DIO_INPUT_A_MSB:DIO_INPUT_A] <= dio_input_a;
            s_ctrlport_resp_data[DIO_INPUT_B_MSB:DIO_INPUT_B] <= dio_input_b;
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
  assign gpio_in_fabric_a = gpio_in_a;
  assign gpio_in_fabric_b = gpio_in_b;

  // Direction control
  assign gpio_en_a = dio_direction_a;
  assign gpio_en_b = dio_direction_b;

  // Output assignment depending on master
  generate
    genvar i;
    for (i = 0; i < DIO_WIDTH; i = i + 1) begin: dio_output_gen
      glitch_free_mux glitch_free_mux_dio_a (
        .select       (dio_master_a[i]),
        .signal0      (gpio_out_fabric_a[i]),
        .signal1      (dio_output_a[i]),
        .muxed_signal (gpio_out_a[i])
      );

      glitch_free_mux glitch_free_mux_dio_b (
        .select       (dio_master_b[i]),
        .signal0      (gpio_out_fabric_b[i]),
        .signal1      (dio_output_b[i]),
        .muxed_signal (gpio_out_b[i])
      );
    end
  endgenerate

endmodule


`default_nettype wire


//XmlParse xml_on
//<regmap name="DIO_REGMAP" readablestrobes="false" ettusguidelines="true">
//  <group name="DIO_REGS">
//    <info>
//      Registers to control the GPIO buffer direction on the FPGA connected to the DIO board.
//      Further registers enable the PS to control and read the GPIO lines as master.
//      Make sure the GPIO lines between FPGA and GPIO board are not driven by two drivers.
//      Set the DIO registers in @.PS_CPLD_BASE_REGMAP appropriately.
//    </info>
//
//    <register name="DIO_MASTER_REGISTER" offset="0x00" size="32">
//      <info>
//        Sets whether the DIO signal line is driven by this register interface or the user application.{br/}
//        0 = user application is master, 1 = PS is master
//      </info>
//      <bitfield name="DIO_MASTER_A" range="0..11" initialvalue="0"/>
//      <bitfield name="DIO_MASTER_B" range="16..27" initialvalue="0"/>
//    </register>
//    <register name="DIO_DIRECTION_REGISTER" offset="0x04" size="32">
//      <info>
//        Set the direction of FPGA buffer connected to DIO ports on the DIO board.{br/}
//        Each bit represents one signal line. 0 = line is an input to the FPGA, 1 = line is an output driven by the FPGA.
//      </info>
//      <bitfield name="DIO_DIRECTION_A" range="0..11" initialvalue="0"/>
//      <bitfield name="DIO_DIRECTION_B" range="16..27" initialvalue="0"/>
//    </register>
//    <register name="DIO_INPUT_REGISTER" offset="0x08" size="32" writable="false">
//      <info>
//        Status of each bit at the FPGA input.
//      </info>
//      <bitfield name="DIO_INPUT_A" range="0..11"/>
//      <bitfield name="DIO_INPUT_B" range="16..27"/>
//    </register>
//    <register name="DIO_OUTPUT_REGISTER" offset="0x0C" size="32">
//      <info>
//        Controls the values on each DIO signal line in case the line master is set to PS in @.DIO_MASTER_REGISTER.
//      </info>
//      <bitfield name="DIO_OUTPUT_A" range="0..11" initialvalue="0"/>
//      <bitfield name="DIO_OUTPUT_B" range="16..27" initialvalue="0"/>
//    </register>
//  </group>
//</regmap>
//XmlParse xml_off

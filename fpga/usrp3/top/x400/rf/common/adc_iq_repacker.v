//
// Copyright 2022 Ettus Research, A National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: adc_iq_repacker
//
// Description:
//
// This component repacks IQ from independent vectors into a single
// output signal, and implements data swapping when requested.
//
// The parameters for this component describe the expected amount of
// data to be received as well as the data to be generated.
//
//  - SPC          = Samples per cycle: amount of samples to be expected
//                   on each I and Q input vector on each "clk" cycle.
//  - SAMPLE_WIDTH = Amount of bits composing each sample
//
// This modules incurs in two clk cycles of delay on the data and valid
// signals from input to output.
//
// Example case : SPC = 2, SAMPLE_WIDTH = 16
//
//  adc_x_in size(for I and Q) = 2 x 16 = 32
//  adc_out size = 2 x 2 x 16 = 64
//
//             _______         _______         _______         ______
// clk       _|       |_______|       |_______|       |_______|
//           _ _______________ _______________ _______________ ______
// adc_i_in  _X_____I1,I0_____X_____I3,I2_____X____32{'X'}____X______
//           _ _______________ _______________ _______________ ______
// adc_q_in  _X_____Q1,Q0_____X_____Q3,Q2_____X____32{'X'}____X______
//             _______________________________
// valid_in  _|                               |______________________
//           _ _______________ _______________ _______________ ______
// adc_out   _X____64{'X'}____X____64{'X'}____X__Q1,I1,Q0,I0__X__Q3,..
//                                             ______________________
// valid_out _________________________________|
//
// When the swap input is high, the order in which Q and I samples
// appear on the output vector is inverted
//
//             _______         _______         _______         ______
// clk       _|       |_______|       |_______|       |_______|
//           _ _______________ _______________ _______________ ______
// adc_i_in  _X_____I1,I0_____X_____I3,I2_____X____32{'X'}____X______
//           _ _______________ _______________ _______________ ______
// adc_q_in  _X_____Q1,Q0_____X_____Q3,Q2_____X____32{'X'}____X______
//             _______________________________
// valid_in  _|                               |______________________
//           _ _______________ _______________ _______________ ______
// adc_out   _X____64{'X'}____X____64{'X'}____X__I1,Q1,I0,Q0__X__I3,..
//                                             ______________________
// valid_out _________________________________|
//
// Parameters:
// SPC = Samples per cycle
// SAMPLE_WIDTH = width of i/q sample inputs. Output will be 2*SAMPLE_WIDTH
//

module adc_iq_repacker #(
  parameter SPC = 1,
  parameter SAMPLE_WIDTH = 16
)
(
  input  wire clk,
  // Data in
  input  wire [SPC*SAMPLE_WIDTH-1:0] adc_q_in,
  input  wire [SPC*SAMPLE_WIDTH-1:0] adc_i_in,
  input  wire valid_in,
  // This signal is currently driven in a related clock, and even though it runs at half the rate is should be fine
  // to handle it in this clock domain(in nature it will also stay high one asserted until the next reset.)
  input  wire enable,
  // Data is packed [Q,I] (I in LSBs) when swap_iq is '0', and [I,Q] otherwise
  input  wire swap_iq,

  // Data out
  output reg [SPC*SAMPLE_WIDTH*2-1:0] data_out_tdata,
  output reg                          data_out_tvalid
  );

  localparam IQ_WIDTH = SAMPLE_WIDTH*2;

  reg valid = 1'b0, valid_dly = 1'b0;
  reg [SPC*SAMPLE_WIDTH-1:0] adc_q_data_in = {SPC*SAMPLE_WIDTH{1'b0}};
  reg [SPC*SAMPLE_WIDTH-1:0] adc_i_data_in = {SPC*SAMPLE_WIDTH{1'b0}};

  integer sample_num;

  // It is safe to not reset this domain because all of the input signals will be cleared
  // by a synchronous reset. Safe default values are assigned to all these registers.
  always @(posedge clk) begin
    adc_q_data_in      <= adc_q_in;
    adc_i_data_in      <= adc_i_in;
    // Place Q in the MSBs, I in the LSBs by default, unless swapped = 1.
    for (sample_num=0; sample_num < (SPC); sample_num = sample_num + 1)
      begin : data_out_gen
        if (swap_iq) begin
          data_out_tdata[sample_num*(IQ_WIDTH) +: IQ_WIDTH] <=
            {adc_i_data_in[sample_num*(SAMPLE_WIDTH) +: SAMPLE_WIDTH],
             adc_q_data_in[sample_num*(SAMPLE_WIDTH) +: SAMPLE_WIDTH]};
        end else begin
          data_out_tdata[sample_num*(IQ_WIDTH) +: IQ_WIDTH] <=
            {adc_q_data_in[sample_num*(SAMPLE_WIDTH) +: SAMPLE_WIDTH],
             adc_i_data_in[sample_num*(SAMPLE_WIDTH) +: SAMPLE_WIDTH]};
        end
      end
    // Valid is simply a transferred version of the 1x clock's valid. Delay it one
    // more cycle to align outputs.
    valid             <= valid_in && enable;
    data_out_tvalid   <= valid;
  end

endmodule

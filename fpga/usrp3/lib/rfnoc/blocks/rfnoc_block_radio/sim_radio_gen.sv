//
// Copyright 2019 Ettus Research, A National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: sim_radio_gen
//
// Description: Generate radio data for simulation purposes. The strobe pattern 
// is random, which is not like a normal radio but covers every possibility. 
// The data pattern is an incrementing sequence of samples, with each channel 
// starting at a different value to differentiate them. Strobe and time are 
// common between channels.
//

module sim_radio_gen #(
  parameter int NSPC         = 1,  // Number of samples per clock cycle
  parameter int SAMP_W       = 32, // Length of each radio sample
  parameter int NUM_CHANNELS = 1,  // Number of radio RX ports
  parameter int STB_PROB     = 50, // Probability of STB being asserted on each clock cycle
  parameter int INCREMENT    = 2,  // Amount by which to increment
  parameter int PPS_PERIOD   = 50  // Period of the PPS output
) (
  input  bit                                radio_clk,
  input  bit                                radio_rst,
  output bit [NUM_CHANNELS*SAMP_W*NSPC-1:0] radio_rx_data,
  output bit [            NUM_CHANNELS-1:0] radio_rx_stb,
  output bit [                        63:0] radio_time,
  output bit                                radio_pps
);

  localparam int RADIO_W = SAMP_W*NSPC;
  typedef bit [RADIO_W-1:0] radio_t;    // Radio output word
  typedef bit [SAMP_W-1:0] sample_t;    // Single sample

  initial assert (PPS_PERIOD % INCREMENT == 0) else
    $fatal(1, "PPS_PERIOD must be a multiple of INCREMENT");


  // Generate an initial value all radio channels
  function radio_t [NUM_CHANNELS-1:0] radio_init();
    radio_t [NUM_CHANNELS-1:0] ret_val;
   
    for (int n = 0; n < NUM_CHANNELS; n++) begin
      sample_t sample;

      // Calculate the value of first sample in this radio channel
      sample = sample_t'((2.0 ** SAMP_W) / NUM_CHANNELS * n);

      // Calculate the value of subsequent samples in the channel
      for (int s = 0; s < NSPC; s++) begin
        ret_val[n][s*SAMP_W +: SAMP_W] = sample + s;
      end
    end

    return ret_val;
  endfunction : radio_init


  //---------------------------------------------------------------------------
  // Radio Data Generation
  //---------------------------------------------------------------------------

  radio_t [NUM_CHANNELS-1:0] data = radio_init();

  assign radio_rx_data = data;

  always @(posedge radio_clk) begin : radio_data_count_reg
    if (radio_rst) begin
      data         <= radio_init();
      radio_rx_stb <= '0;
    end else begin
      radio_rx_stb <= '0;
      if ($urandom_range(100) < STB_PROB) begin
        for (int n = 0; n < NUM_CHANNELS; n++) begin
          for (int s = 0; s < NSPC; s++) begin
            data[n][s*SAMP_W +: SAMP_W] <= data[n][s*SAMP_W +: SAMP_W] + NSPC;
          end
        end
        radio_rx_stb <= '1;
      end
    end
  end : radio_data_count_reg


  //---------------------------------------------------------------------------
  // Radio Time
  //---------------------------------------------------------------------------

  always @(posedge radio_clk) begin
    if (radio_rst) begin
      radio_time <= 64'b0;
      radio_pps  <=  1'b0;
    end else begin
      radio_pps  <= 1'b0;
      if (radio_rx_stb[0]) begin
        radio_time <= radio_time + INCREMENT;
        if (radio_time % PPS_PERIOD == 0 && radio_time != 0) begin
          radio_pps <= 1'b1;
        end
      end
    end
  end

endmodule : sim_radio_gen
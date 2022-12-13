//
// Copyright 2022 Ettus Research, A National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: sim_radio_gen
//
// Description:
//
// Generate radio data for simulation purposes. The strobe pattern is random,
// which is not like a normal radio but covers every possibility. The data
// pattern is an incrementing sequence of samples, with each channel starting
// at a different value to differentiate them. Strobe and time are common
// between channels.
//
// The initial rx_data and radio_time values can be set at run-time using the
// functions provided.
//
// Parameters:
//
//   NSPC         : Number of samples per clock cycle
//   SAMP_W       : Length of each radio sample
//   NUM_CHANNELS : Number of radio RX ports
//   STB_PROB     : Probability of STB being asserted on each clock cycle
//   INCREMENT    : Amount by which to increment radio time each strobe
//

module sim_radio_gen #(
  parameter int NSPC         = 1,
  parameter int SAMP_W       = 32,
  parameter int NUM_CHANNELS = 1,
  parameter int STB_PROB     = 50,
  parameter int INCREMENT    = NSPC,

  localparam int RADIO_W = SAMP_W*NSPC
) (
  input  logic                                 radio_clk,
  input  logic                                 radio_rst,
  output logic [NUM_CHANNELS-1:0][RADIO_W-1:0] radio_rx_data,
  output logic [             NUM_CHANNELS-1:0] radio_rx_stb,
  output logic [                         63:0] radio_time
);

  typedef bit      [      SAMP_W-1:0] sample_t;    // Single sample
  typedef sample_t [        NSPC-1:0] radio_t;     // Radio output word
  typedef radio_t  [NUM_CHANNELS-1:0] data_t;      // Radio output for all channels
  typedef bit      [            63:0] timestamp_t; // Radio timestamp


  //---------------------------------------------------------------------------
  // Functions
  //---------------------------------------------------------------------------

  // Generate initial value for a single radio channel
  function radio_t radio_init(
    sample_t first_sample = '0
  );
    radio_t ret_val;

    for (int samp_i = 0; samp_i < NSPC; samp_i++) begin
      ret_val[samp_i] = first_sample + samp_i;
    end

    return ret_val;
  endfunction : radio_init

  // Generate an initial value all radio channels
  function radio_t [NUM_CHANNELS-1:0] radio_init_all(
    bit [SAMP_W-1:0] first_sample = '0
  );
    data_t   ret_val;
    sample_t sample;

    // Calculate the value of subsequent samples in the channel
    for (int ch_i = 0; ch_i < NUM_CHANNELS; ch_i++) begin
      sample = sample_t'((2.0 ** SAMP_W) / NUM_CHANNELS * ch_i + first_sample);
      ret_val[ch_i] = radio_init(first_sample);
    end

    return ret_val;
  endfunction : radio_init_all

  timestamp_t next_time    = '0;
  bit         next_time_ld =  0;

  radio_t [NUM_CHANNELS-1:0] next_data    = '0;
  bit     [NUM_CHANNELS-1:0] next_data_ld = '0;

  // Change the radio time on the next clock edge
  function void set_time(timestamp_t timestamp);
    next_time    = timestamp;
    next_time_ld = 1;
  endfunction : set_time

  // Change the radio data value for the given channel on the next clock edge
  function void set_data(
    int     channel,
    radio_t data);
    next_data   [channel] = data;
    next_data_ld[channel] = 1;
  endfunction : set_data

  // Change the radio data value for all channels on the next clock edge
  function void set_data_all(
    data_t data);
    next_data    = data;
    next_data_ld = '1;
  endfunction : set_data_all


  //---------------------------------------------------------------------------
  // Radio Output Generation
  //---------------------------------------------------------------------------

  radio_t    [NUM_CHANNELS-1:0] reg_data = radio_init_all();
  timestamp_t                   reg_time = '0;

  // Output X when strobe is low to cause errors when we use the time or data
  // during the wrong clock cycle.
  assign radio_rx_data = radio_rx_stb ? reg_data : 'X;
  assign radio_time    = radio_rx_stb ? reg_time : 'X;

  always @(posedge radio_clk) begin : radio_data_count_reg
      if (radio_rst) begin
        reg_data     <= radio_init();
        radio_rx_stb <= '0;
        reg_time     <= '0;
      end else begin
        radio_rx_stb <= '0;
        if ($urandom_range(99) < STB_PROB) begin
          for (int ch_i = 0; ch_i < NUM_CHANNELS; ch_i++) begin
            for (int samp_i = 0; samp_i < NSPC; samp_i++) begin
              reg_data[ch_i][samp_i] = reg_data[ch_i][samp_i] + NSPC;
            end
          end
          reg_time <= reg_time + INCREMENT;
          radio_rx_stb <= '1;
        end
        // Override the radio data
        if (next_data_ld) begin
          for (int ch_i=0; ch_i < NUM_CHANNELS; ch_i++) begin
            if (next_data_ld[ch_i]) reg_data[ch_i] <= next_data[ch_i];
          end
          next_data_ld = 0;
        end
        // Override the radio time
        if (next_time_ld) begin
          reg_time <= next_time;
          next_time_ld = 0;
        end
      end
    end : radio_data_count_reg

endmodule : sim_radio_gen

//
// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Package: cic_comb_filter_test:pkg
//
// Description:
//
//   Package containing utilities and simulation models for CIC comb filter testbenches.
//
// Parameters:
//
//   SPC          : Number of samples per clock cycle on DUT
//   ACCUM_W      : Width of a DUT input sample. Equivalent to the accumulator width.
//   COMP_W       : Width of I and Q components in the input samples. This is
//                  used to generate test inputs and should be less than or equal to
//                  ACCUM_W/2.
//

`default_nettype none

package cic_comb_filter_test_pkg;

  `include "test_exec.svh"
  
  import cic_utils_pkg::*;
  import cic_test_pkg::*;
  import PkgAxiStreamBfm::*;
  import PkgRandom::*;
  import PkgTestExec::*;

  //---------------------------------------------------------------------------
  // Parametrized test utility class
  //---------------------------------------------------------------------------
  class cic_comb_filter_test_utils #(
    parameter int SPC      = 4,   // Number of samples per clock cycle on DUT
    parameter int ACCUM_W  = 48,  // Width of a DUT input sample.
    parameter int COMP_W   = 24   // Width of I and Q components in the input samples.
  );

    //-------------------------------------------------------------------------
    // Type definitions
    //-------------------------------------------------------------------------
    typedef cic_test_utils#(
      .SPC    (SPC),
      .ACCUM_W(ACCUM_W),
      .COMP_W (COMP_W)
    ) ctest_utils_t;
    typedef ctest_utils_t::axis_pkt_t                   axis_packet_t;
    typedef ctest_utils_t::axis_single_sample_pkt_t     axis_single_sample_packet_t;
    typedef ctest_utils_t::pkt_burst_t                  axis_packet_queue_t;
    typedef ctest_utils_t::pkt_burst_single_sample_t    axis_single_sample_packet_queue_t;

    typedef cic_utils#(
      .SPC   (SPC),
      .SAMP_W(ACCUM_W)
    ) utils_t;
    typedef utils_t::sample_t sample_t;
    typedef utils_t::word_t   word_t;
    typedef sample_t          sample_queue_t[$];



    //-------------------------------------------------------------------------
    // Test utilities
    //-------------------------------------------------------------------------

    //-------------------------------------------------------------------------
    // serialize_packet: flatten a multi-sample AXI-Stream packet into a
    // single-sample packet by unpacking each SPC-wide word into SPC
    // individual sample entries, preserving sample order.
    //   packet_in : input packet with SPC samples packed per data word.
    //   returns   : packet with one sample per data word.
    //-------------------------------------------------------------------------
    static function axis_single_sample_packet_t serialize_packet(
      input axis_packet_t packet_in
    );
      automatic axis_single_sample_packet_t packet_out = new();
      foreach (packet_in.data[i]) begin
        word_t current_word = word_t'(packet_in.data[i]);
        for (int j = 0; j < SPC; j++) begin
          packet_out.data.push_back(current_word[j]);
        end
      end
      return packet_out;
    endfunction : serialize_packet

    //-------------------------------------------------------------------------
    // deserialize_packet: pack a single-sample packet back into a
    // multi-sample AXI-Stream packet by grouping consecutive SPC samples
    // into one data word. The number of input samples must be a multiple of
    // SPC; an assertion error is raised otherwise.
    //   packet_in : input packet with one sample per data word.
    //   returns   : packet with SPC samples packed per data word.
    //-------------------------------------------------------------------------
    static function axis_packet_t deserialize_packet(
      input axis_single_sample_packet_t packet_in
    );
      automatic axis_packet_t packet_out = new();
      axis_single_sample_packet_t temp_packet = packet_in; // Make a copy to index into input packet.
      int num_samples = packet_in.data.size();
      `ASSERT_ERROR(num_samples % SPC == 0,
        {
          "Input packet sample count must be a multiple of SPC. ",
          "Got ", num_samples, " samples with SPC=", SPC, "."
        }
      );
      for (int i = 0; i < num_samples; i += SPC) begin
        word_t current_word;
        for (int j = 0; j < SPC; j++) begin
          current_word[j] = temp_packet.data[i + j];
        end
        packet_out.data.push_back(current_word);
      end
      return packet_out;
    endfunction : deserialize_packet
    
    //-------------------------------------------------------------------------
    // to_sample_queue: convert a single-sample AXI-Stream packet to a flat
    // sample queue, preserving sample order. Useful for passing data into
    // the cic_comb_filter_test_model, which operates on sample queues.
    //   packet_in : input packet with one sample per data word.
    //   returns   : queue of sample_t, one entry per sample.
    //-------------------------------------------------------------------------
    static function sample_queue_t to_sample_queue(
      input axis_single_sample_packet_t packet_in
    );
      sample_queue_t queue_out;
      foreach (packet_in.data[i]) begin
        queue_out.push_back(packet_in.data[i]);
      end
      return queue_out;
    endfunction : to_sample_queue

    //-------------------------------------------------------------------------
    // from_sample_queue: convert a flat sample queue back into a
    // single-sample AXI-Stream packet, preserving sample order. Useful for
    // converting cic_comb_filter_test_model output back into packet form for
    // comparison against DUT output.
    //   queue_in : input queue of sample_t, one entry per sample.
    //   returns  : packet with one sample per data word.
    //-------------------------------------------------------------------------
    static function axis_single_sample_packet_t from_sample_queue(
      input sample_queue_t queue_in
    );
      automatic axis_single_sample_packet_t packet_out = new();
      foreach (queue_in[i]) begin
        packet_out.data.push_back(queue_in[i]);
      end
      return packet_out;
    endfunction : from_sample_queue

    //-------------------------------------------------------------------------
    // serialize_packets_to_sample_queue: flatten a queue of multi-sample
    // AXI-Stream packets into a single flat sample queue by serializing each
    // packet in order. Combines serialize_packet and to_sample_queue across
    // all packets, concatenating the resulting samples.
    //   packet_queue_in : input queue of multi-sample packets.
    //   returns         : flat queue of sample_t in stream order.
    //-------------------------------------------------------------------------
    static function sample_queue_t serialize_packets_to_sample_queue(
      input axis_packet_queue_t packet_queue_in
    );
      sample_queue_t queue_out;
      foreach (packet_queue_in[i]) begin
        axis_single_sample_packet_t single_sample_packet = serialize_packet(packet_queue_in[i]);
        sample_queue_t sample_queue = to_sample_queue(single_sample_packet);
        foreach (sample_queue[j]) begin
          queue_out.push_back(sample_queue[j]);
        end
      end
      return queue_out;
    endfunction : serialize_packets_to_sample_queue

    //-------------------------------------------------------------------------
    // deserialize_sample_queue_to_packet_queue: repack a flat sample queue
    // into a queue of multi-sample AXI-Stream packets. Produces num_packets
    // packets, each containing words_per_packet SPC-wide words. Samples are
    // consumed from the front of sample_queue_in in order. An assertion is
    // raised if the input queue does not hold enough samples to fill all
    // requested packets.
    //   sample_queue_in  : flat queue of sample_t in stream order.
    //   num_packets      : number of output packets to produce (default: 1).
    //   words_per_packet : words per output packet. Pass 0 (default) to
    //                      distribute all samples evenly across num_packets.
    //   returns          : queue of multi-sample packets, SPC samples per word.
    //-------------------------------------------------------------------------
    static function axis_packet_queue_t deserialize_sample_queue_to_packet_queue(
      input sample_queue_t sample_queue_in,
      input int num_packets = 1,
      input int words_per_packet = 0  // 0 = auto: distribute samples evenly across packets
    );
      // Local mutable copy, input arguments are formally read-only.
      sample_queue_t      local_queue = sample_queue_in;
      int                 resolved_words_per_packet;
      axis_packet_queue_t packet_queue_out;

      // If auto distribution of samples per packet is requested, ensure that the number of
      // samples can be evenly distributed across the requested number of packets.
      if (words_per_packet == 0) begin
        `ASSERT_ERROR(local_queue.size() % (SPC * num_packets) == 0,
          {
            "When words_per_packet=0, the number of input samples must be a multiple of ",
            "SPC * num_packets. Got ", local_queue.size(), " samples with SPC=", SPC,
            " and num_packets=", num_packets, "."
          }
        );
      end
      resolved_words_per_packet = (words_per_packet != 0)
                                ? words_per_packet
                                : (local_queue.size() / (SPC * num_packets));

      // Verify the input queue has enough samples for all requested packets.
      `ASSERT_ERROR(local_queue.size() >= num_packets * resolved_words_per_packet * SPC,
        $sformatf({"Input queue too small: need %0d samples ",
                   "(num_packets=%0d * words_per_packet=%0d * SPC=%0d), but got %0d."},
                  num_packets * resolved_words_per_packet * SPC,
                  num_packets, resolved_words_per_packet, SPC, local_queue.size())
      );

      for (int pkt_idx = 0; pkt_idx < num_packets; pkt_idx++) begin
        automatic sample_queue_t sample_queue_chunk = {};
        automatic axis_single_sample_packet_t single_sample_packet = new();
        automatic axis_packet_t packet = new();
        for (int j = 0; j < resolved_words_per_packet; j++) begin
          repeat (SPC) begin
            sample_queue_chunk.push_back(local_queue.pop_front());
          end
        end
        single_sample_packet = from_sample_queue(sample_queue_chunk);
        packet = deserialize_packet(single_sample_packet);
        packet_queue_out.push_back(packet);
      end
      return packet_queue_out;
    endfunction : deserialize_sample_queue_to_packet_queue


  endclass : cic_comb_filter_test_utils

  //---------------------------------------------------------------------------
  // Simulation model of the CIC comb filter
  //
  // Implements the difference equation y[n] = x[n] - x[n-D] with a fixed
  // differential delay D=1 for a batch of samples (stateless, no internal
  // state between calls). For the first D samples, the delayed term is zero
  // (matching zero initial conditions in the RTL).
  //
  // I and Q components are subtracted independently to match the RTL which
  // prevents borrow propagation across the half-word boundary.
  //
  // Parameters:
  //   ACCUM_W : Sample bit width. I occupies the upper half [ACCUM_W-1:ACCUM_W/2],
  //             Q occupies the lower half [ACCUM_W/2-1:0].
  //---------------------------------------------------------------------------
  class cic_comb_filter_test_model #(
    parameter int ACCUM_W = 32,
    parameter int DELAY   = 1
  );

    //-------------------------------------------------------------------------
    // Type definitions
    //-------------------------------------------------------------------------
    typedef cic_utils#(
      .SPC   (1),
      .SAMP_W(ACCUM_W)
    ) utils_t;

    typedef utils_t::sample_t          sample_t;
    typedef utils_t::comp_t            comp_t;
    typedef sample_t                   sample_queue_t[$];

    localparam int COMP_W = ACCUM_W / 2;

    //-------------------------------------------------------------------------
    // Constructor
    //-------------------------------------------------------------------------
    function new();
    endfunction : new

    //-------------------------------------------------------------------------
    // process_samples: compute y[n] = x[n] - x[n-D] for the entire input
    // queue in one shot. For n < D, the delayed term is zero.
    //-------------------------------------------------------------------------
    function void process_samples(
      input  sample_queue_t samples_in,
      output sample_queue_t samples_out
    );
      comp_t in_i, in_q, delayed_i, delayed_q;
      samples_out = {};
      foreach (samples_in[i]) begin
        in_q = comp_t'(samples_in[i][COMP_W-1:0]);
        in_i = comp_t'(samples_in[i][ACCUM_W-1:COMP_W]);
        if (i < DELAY) begin
          delayed_q = '0;
          delayed_i = '0;
        end else begin
          delayed_q = comp_t'(samples_in[i-DELAY][COMP_W-1:0]);
          delayed_i = comp_t'(samples_in[i-DELAY][ACCUM_W-1:COMP_W]);
        end
        samples_out.push_back({comp_t'(in_i - delayed_i), comp_t'(in_q - delayed_q)});
      end
    endfunction : process_samples

  endclass : cic_comb_filter_test_model

endpackage : cic_comb_filter_test_pkg

`default_nettype wire

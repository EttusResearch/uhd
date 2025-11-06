//
// Copyright 2025 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: complex_multiply_iq_tb
//
// Description:
//
//  Testbench for the complex_multiply_iq module.
//

module complex_multiply_iq_tb #(
  // Parameters for the testbench
  parameter int WIDTH_A = 20,
  parameter int WIDTH_B = 16,
  parameter int WIDTH_PRODUCT = 18,
  parameter int FRACTIONAL_BITS_A = 4,
  parameter int FRACTIONAL_BITS_B = 4,
  parameter int FRACTIONAL_BITS_PRODUCT = 0
);

  // Include macros and time declarations for use with PkgTestExec
  `include "test_exec.svh"

  import PkgTestExec::*;
  import PkgAxiStreamBfm::*;
  import PkgComplex::*;
  import PkgMath::*;
  import PkgRandom::*;

  // Clock and reset
  logic clk;
  logic rst;

  // AXI Stream interfaces
  AxiStreamIf #(.DATA_WIDTH(WIDTH_A*2), .TUSER(0), .TKEEP(0), .USER_WIDTH(0))
    factor_a(clk, rst);
  AxiStreamIf #(.DATA_WIDTH(WIDTH_B*2), .TUSER(0), .TKEEP(0), .USER_WIDTH(0))
    factor_b(clk, rst);
  AxiStreamIf #(.DATA_WIDTH(WIDTH_PRODUCT*2), .TUSER(0), .TKEEP(0), .USER_WIDTH(0))
    product(clk, rst);

  // BFM for the AXI-Stream interface to DUT
  AxiStreamBfm #(.DATA_WIDTH(WIDTH_A*2), .TUSER(0), .TKEEP(0), .USER_WIDTH(0))
    factor_a_bfm = new(factor_a, null);
  AxiStreamBfm #(.DATA_WIDTH(WIDTH_B*2), .TUSER(0), .TKEEP(0), .USER_WIDTH(0))
    factor_b_bfm = new(factor_b, null);
  AxiStreamBfm #(.DATA_WIDTH(WIDTH_PRODUCT*2), .TUSER(0), .TKEEP(0), .USER_WIDTH(0))
    product_bfm = new(null, product);

  // Types
  typedef AxiStreamPacket #(.DATA_WIDTH(WIDTH_A*2), .USER_WIDTH(0)) factor_a_packet_t;
  typedef AxiStreamPacket #(.DATA_WIDTH(WIDTH_B*2), .USER_WIDTH(0)) factor_b_packet_t;
  typedef AxiStreamPacket #(.DATA_WIDTH(WIDTH_PRODUCT*2), .USER_WIDTH(0)) product_packet_t;

  typedef complex #(WIDTH_A, FRACTIONAL_BITS_A) sc_a;
  typedef complex #(WIDTH_B, FRACTIONAL_BITS_B) sc_b;

  sim_clock_gen #(.PERIOD(10)) clk_gen (.clk(clk), .rst(rst));

  // DUT
  complex_multiply_iq #(
    .FRACTIONAL_BITS_A(FRACTIONAL_BITS_A),
    .FRACTIONAL_BITS_B(FRACTIONAL_BITS_B),
    .FRACTIONAL_BITS_PRODUCT(FRACTIONAL_BITS_PRODUCT)
  ) dut (
    .factor_a(factor_a),
    .factor_b(factor_b),
    .product(product)
  );

  function automatic complex_t get_complex_rand(
    input int integer_bits,
    input int fractional_bits
  );
    complex_t sample;
    sample.re = round_bits(frand(2.0**(integer_bits-1)), fractional_bits);
    sample.im = round_bits(frand(2.0**(integer_bits-1)), fractional_bits);
    if ($urandom_range(1)) sample.re = -sample.re;
    if ($urandom_range(1)) sample.im = -sample.im;
    return sample;
  endfunction

  function automatic logic[WIDTH_PRODUCT*2-1:0] convert_to_product(
    input complex_t sample
  );
    logic[WIDTH_PRODUCT*2-1:0] iq_sample;
    real max_value = 2.0**(WIDTH_PRODUCT-1);

    // scale sample to fit into WIDTH_PRODUCT
    sample.re = sample.re * (1 << FRACTIONAL_BITS_PRODUCT);
    sample.im = sample.im * (1 << FRACTIONAL_BITS_PRODUCT);
    // round sample according to axi_round module
    sample.re = round(sample.re, ROUND_HALF_UP);
    sample.im = round(sample.im, ROUND_HALF_UP);
    // saturate sample
    sample.re = fmin(sample.re, max_value-1);
    sample.re = fmax(sample.re, -max_value);
    sample.im = fmin(sample.im, max_value-1);
    sample.im = fmax(sample.im, -max_value);

    iq_sample[0+:WIDTH_PRODUCT] = longint'(sample.re);
    iq_sample[WIDTH_PRODUCT+:WIDTH_PRODUCT] = longint'(sample.im);
    return iq_sample;
  endfunction

  initial begin : main
    clk_gen.kill();
    test.start_tb($sformatf({"complex_multiply_iq_tb: WIDTH_A=%0d, WIDTH_B=%0d, ",
      "WIDTH_PRODUCT=%0d, FRACTIONAL_BITS_A=%0d, FRACTIONAL_BITS_B=%0d, ",
      "FRACTIONAL_BITS_PRODUCT=%0d"}, WIDTH_A, WIDTH_B, WIDTH_PRODUCT,
      FRACTIONAL_BITS_A, FRACTIONAL_BITS_B, FRACTIONAL_BITS_PRODUCT));
    clk_gen.revive();

    // Reset DUT
    clk_gen.reset();
    wait(rst == 0);

    // Start BFMs
    factor_a_bfm.run();
    factor_b_bfm.run();
    product_bfm.run();
    test.start_test("Multiplier");

    begin
      factor_a_packet_t packet_a;
      factor_b_packet_t packet_b;
      product_packet_t packet_product;
      complex_t expected_products [$];
      complex_t sample_a, sample_b, sample_product;
      packet_a = new();
      packet_b = new();
      for (int i=0; i<1000; i++) begin
        sample_a = get_complex_rand(WIDTH_A-FRACTIONAL_BITS_A-1, FRACTIONAL_BITS_A);
        sample_b = get_complex_rand(WIDTH_B-FRACTIONAL_BITS_B-1, FRACTIONAL_BITS_B);
        sample_product = mul(sample_a, sample_b);

        packet_a.data.push_back(sc_a::real_to_logic(sample_a));
        packet_b.data.push_back(sc_b::real_to_logic(sample_b));
        expected_products.push_back(sample_product);
      end
      factor_a_bfm.put(packet_a);
      factor_b_bfm.put(packet_b);
      product_bfm.get(packet_product);

      for (int i=0; i<packet_product.data.size(); i++) begin
        automatic logic [WIDTH_PRODUCT*2-1:0] expected = convert_to_product(expected_products[i]);
        `ASSERT_ERROR(packet_product.data[i] == expected,
          $sformatf("Mismatch at index %0d: received %0h, expected %0h",
            i, packet_product.data[i], expected));
      end
    end
    test.end_test();

    clk_gen.kill();
    test.end_tb(0);
  end

endmodule

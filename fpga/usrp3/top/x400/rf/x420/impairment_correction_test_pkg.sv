// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module:  impairment_correction_pkg.sv
//
// Description: Test package for IQ impairment correction
package impairment_correction_test_pkg;

    import PkgTestExec::*;
    import ctrlport_bfm_pkg::*;
    import XmlSvPkgIQ_IMPAIRMENT_REGMAP::*;
    import PkgAxiStreamBfm::*;

    localparam int IQ_SAMPLE_WIDTH = 32;

    // class for FIR processing testing
    class ImpairmentCorrectionTestClass #(
      NUM_SPC = 1,
      NUM_COEFFS = 1
    );

      // as defined in the register map (Q2.23 format)
      localparam int COEFFS_FRACTIONAL_BITS = 23;
      localparam int COEFF_ONE = (1 << COEFFS_FRACTIONAL_BITS);

      localparam int TIMEOUT_CYCLES = 1000;
      localparam realtime TEST_TIMEOUT = 1ms;

      local int ctrlport_addr_offset = 0;

      typedef logic signed [15:0] sample_t;
      typedef logic signed [24:0] coeff_t;

      typedef coeff_t coeff_array_t [NUM_COEFFS];
      typedef sample_t sample_array_t [$];

      local coeff_t iinline_coeff;
      local coeff_array_t icross_coeffs, qinline_coeffs;
      local sample_array_t isample_in, qsample_in, isample_out, qsample_out;
      local logic [IQ_SAMPLE_WIDTH-1:0] output_samples [$];

      local ctrlport_bfm ctrl_bfm;
      local AxiStreamBfm #(.DATA_WIDTH(IQ_SAMPLE_WIDTH*NUM_SPC), .TUSER(0), .TKEEP(0), .TLAST(1)) axi_stream_bfm;
      typedef AxiStreamBfm #(.DATA_WIDTH(IQ_SAMPLE_WIDTH*NUM_SPC), .TUSER(0), .TKEEP(0), .TLAST(1))::AxisPacket_t AxisPacket_t;

      function new(
        AxiStreamBfm #(.DATA_WIDTH(IQ_SAMPLE_WIDTH*NUM_SPC), .TUSER(0), .TKEEP(0), .TLAST(1)) axi_stream_bfm,
        ctrlport_bfm ctrl_bfm,
        int ctrl_addr_offset
      );
        this.axi_stream_bfm = axi_stream_bfm;
        this.ctrl_bfm = ctrl_bfm;
        this.ctrlport_addr_offset = ctrl_addr_offset;
      endfunction

       // check the number of coefficients
      task verify_num_coeffs();
        test.start_test("Test number of coefficients", TEST_TIMEOUT);
        ctrl_bfm.async_read(ctrlport_addr_offset + kDSP_REG, NUM_COEFFS);
        ctrl_bfm.wait_complete();
        test.end_test();
      endtask

      // push zeros packet to flush filter
      task flush_filter();
        AxisPacket_t zero_pkt;

        // create a long stream of zeros to push through the filter to flush out
        // any old data
        zero_pkt = new();
        for (int i = 0; i < 100; i++) begin
          zero_pkt.data.push_back('0);
        end
        axi_stream_bfm.put(zero_pkt);

      endtask


      // FIR filter initialization to all zeros and then flushing of data path
      task initialize_filter();
        AxisPacket_t rev_pkt, last_pkt;

        test.start_test("FIR filter initialization", TEST_TIMEOUT);

        // initialize group delay to 0
        ctrl_bfm.async_write(ctrlport_addr_offset + kDELAY_REG, 0);

        // set all coefficients to 0
        ctrl_bfm.async_write(ctrlport_addr_offset + kIINLINE_COEFF_REG, 0);
        for (int i = 0; i < NUM_COEFFS; i++) begin
          ctrl_bfm.async_write(ctrlport_addr_offset + kICROSS_COEFF_REG , 0);
          ctrl_bfm.async_write(ctrlport_addr_offset + kQINLINE_COEFF_REG, 0);
        end
        ctrl_bfm.wait_complete();

        // flush filter and wait for transmission to complete and check if the last output
        flush_filter();
        axi_stream_bfm.wait_complete();

        // get all packets and check that the last one is all zeros
        assert (axi_stream_bfm.num_received() > 0) else $error("no output packets received");
        while (axi_stream_bfm.try_get(rev_pkt)) begin
          last_pkt = rev_pkt;
        end
        // due to tlast being unused in this design each packet has just a
        // single data item
        assert(last_pkt.data[0] == '0) else $error("filter could not be initialized");

        test.end_test();
      endtask

      local task reset_samples();
        isample_in = {};
        qsample_in = {};
      endtask;

      local task push_sample(
        input sample_t isample,
        input sample_t qsample
      );
        isample_in.push_back(isample);
        qsample_in.push_back(qsample);
      endtask;

    // calculate the expected output of the IQ impairment DUT
    local task automatic calculate_output(
      input int group_delay
    );
      begin
        // create local copies of the queues
        automatic logic signed [15:0] isample_in_local [$] = isample_in;
        automatic logic signed [15:0] qsample_in_local [$] = qsample_in;

        // check queue length
        assert(isample_in.size() == qsample_in.size()) else $error("I and Q have different queue lengths");

        // pad input with zeros to match SPC
        for (int i = 0; i < NUM_SPC; i++) begin
          isample_in_local.push_back('0);
          qsample_in_local.push_back('0);
        end

        // pad input with zeros for ring in and ring out
        for (int i = 0; i < NUM_COEFFS-1; i++) begin
          isample_in_local.push_front('0);
          qsample_in_local.push_front('0);
          isample_in_local.push_back('0);
          qsample_in_local.push_back('0);
        end

        // calculate Q output from FIR filter
        qsample_out = {};
        // start after zeros for ring in
        for (int i = NUM_COEFFS-1; i < qsample_in_local.size(); i++) begin
          // create very large signed number to not overflow
          automatic logic signed [63:0] expected_q_output = 0;
          // multiply values
          for (int j = 0; j < NUM_COEFFS; j++) begin
            expected_q_output = expected_q_output + isample_in_local[i-j] * signed'(icross_coeffs[j]);
            expected_q_output = expected_q_output + qsample_in_local[i-j] * signed'(qinline_coeffs[j]);
          end
          // round towards nearest integer (half up)
          expected_q_output = expected_q_output + (1 << (COEFFS_FRACTIONAL_BITS-1));
          // shift
          expected_q_output = expected_q_output >>> COEFFS_FRACTIONAL_BITS;
          // saturate
          if (expected_q_output > 2**15-1) begin
            expected_q_output = 2**15-1;
          end else if (expected_q_output < -2**15) begin
            expected_q_output = -2**15;
          end
          // store result
          qsample_out.push_back(expected_q_output[15:0]);
        end

        // calculate I output
        isample_out = {};
        // delay input by group delay
        for(int i = 0; i < group_delay; i++) begin
          isample_out.push_back('0);
        end
        // copy samples from input stream
        for (int i = 0; i < isample_in.size(); i++) begin
          automatic logic signed [63:0] expected_i_output;
          expected_i_output = isample_in[i] * signed'(iinline_coeff);
          // round towards nearest integer (half up)
          expected_i_output = expected_i_output + (1 << (COEFFS_FRACTIONAL_BITS-1));
          // shift
          expected_i_output = expected_i_output >>> COEFFS_FRACTIONAL_BITS;
          // saturate
          if (expected_i_output > 2**15-1) begin
            expected_i_output = 2**15-1;
          end else if (expected_i_output < -2**15) begin
            expected_i_output = -2**15;
          end
          isample_out.push_back(expected_i_output[15:0]);
        end
        // pad to same length as Q
        for (int i = isample_out.size(); i < qsample_out.size(); i++) begin
          isample_out.push_back('0);
        end
      end
    endtask;

    // test the data path given the group_delay and based on the coefficient arrays
    local task automatic test_data_path(
      input int group_delay
    );
      AxisPacket_t input_pkt, rev_pkt;

      // set group delay
      ctrl_bfm.async_write(ctrlport_addr_offset + kDELAY_REG, group_delay);

      // write coefficients
      ctrl_bfm.async_write(ctrlport_addr_offset + kIINLINE_COEFF_REG, iinline_coeff);
      for (int i = NUM_COEFFS; i >=0; i--) begin
        ctrl_bfm.async_write(ctrlport_addr_offset + kICROSS_COEFF_REG , icross_coeffs[i]);
        ctrl_bfm.async_write(ctrlport_addr_offset + kQINLINE_COEFF_REG, qinline_coeffs[i]);
      end
      ctrl_bfm.wait_complete();

      // calculate expected output
      calculate_output(group_delay);

      // drive input samples
      input_pkt = new();
      begin : drive_input
        automatic int i = 0;
        automatic logic [IQ_SAMPLE_WIDTH*NUM_SPC-1:0] tdata;

        input_pkt = new();
        while(i < isample_in.size()) begin
          tdata = '0;
          for (int j = 0; j < NUM_SPC; j++) begin
            if (i < isample_in.size()) begin
              tdata[IQ_SAMPLE_WIDTH*j +: IQ_SAMPLE_WIDTH] = {qsample_in[i], isample_in[i]};
              i++;
            end
          end
          input_pkt.data.push_back(tdata);
        end

        // transmit samples and flush filter
        axi_stream_bfm.put(input_pkt);
        flush_filter();
      end

      // wait for tranmission to complete
      axi_stream_bfm.wait_complete();

      begin: collect_output
        // search for values other than zero
        repeat(TIMEOUT_CYCLES) begin
          axi_stream_bfm.get(rev_pkt);
          if (rev_pkt.data[0] != '0) begin
            break;
          end
        end

        // collect output samples
        output_samples = {};
        do begin
          for (int i = 0; i < NUM_SPC; i++) begin
            output_samples.push_back(rev_pkt.data[0][i*IQ_SAMPLE_WIDTH +: IQ_SAMPLE_WIDTH]);
          end
          axi_stream_bfm.get(rev_pkt);
        end while(output_samples.size() < qsample_out.size());
      end

      // compare against the expected output
      for (int i=0; i < qsample_out.size(); i++) begin
        assert (output_samples[i] == {qsample_out[i], isample_out[i]}) else
        $error("sample %d: output_sample = %h, expected = %h", i, output_samples[i], {qsample_out[i], isample_out[i]});
      end
    endtask;

    // Checking data path alignment (Icross FIR vs delay)
    task test_alignment_icross();
      test.start_test("Checking data path alignment (Icross FIR vs. delay)", TEST_TIMEOUT);

      // setting Icross filter to 1.0 in the first coefficient for reference
      iinline_coeff = COEFF_ONE;
      for (int i = 0; i < NUM_COEFFS; i++) begin
        // representation of 1.0 in fixed point format Q2.23
        icross_coeffs[i] = 0;
        qinline_coeffs[i] = 0;
      end
      icross_coeffs[0] = COEFF_ONE;

      // input is a pulse
      reset_samples();
      push_sample('1, '1);

      // test all possible group delays
      for (int group_delay = 0; group_delay < NUM_COEFFS; group_delay++) begin
        $display("group_delay = %0d", group_delay);
        test_data_path(group_delay);
      end

      test.end_test();
    endtask;

    // Checking data path alignment (Qinline FIR vs delay)
    task test_alignment_qinline();
      test.start_test("Checking data path alignment (Qinline FIR vs. delay)", TEST_TIMEOUT);

      // setting Icross filter to 1.0 in the first coefficient for reference
      iinline_coeff = COEFF_ONE;
      for (int i = 0; i < NUM_COEFFS; i++) begin
        // representation of 1.0 in fixed point format Q2.23
        icross_coeffs[i] = 0;
        qinline_coeffs[i] = 0;
      end
      qinline_coeffs[0] = COEFF_ONE;

      // input is a pulse
      reset_samples();
      push_sample('1, '1);

      // decrement group delay for this filter as this order caused issues in the past
      for (int group_delay = NUM_COEFFS-1; group_delay >= 0; group_delay--) begin
        $display("group_delay = %0d", group_delay);
        test_data_path(group_delay);
      end

      test.end_test();
    endtask;

    // Checking Icross FIR filter
    task test_icross_fir();
      test.start_test("Checking Icross FIR filter", TEST_TIMEOUT);

      // assign random coefficients in reverse order as required by the FIR filter
      iinline_coeff = COEFF_ONE;
      for (int i = NUM_COEFFS-1; i >= 0; i--) begin
        icross_coeffs[i] = $urandom_range(0,2**kICROSS_COEFFSize);
        qinline_coeffs[i] = 0;
      end

      // push single pulse (half size) into the filter
      reset_samples();
      push_sample(16'h4000, '0);
      test_data_path(0);

      // add more data to test filter adder tree
      for (int i = 0; i < 100; i++) begin
        push_sample($urandom_range(0,2**16), '0);
      end
      test_data_path(0);

      test.end_test();
    endtask;

    // Checking Qinline FIR filter
    task test_qinline_fir();
      test.start_test("Checking Qinline FIR filter", TEST_TIMEOUT);

      // assign random coefficients in reverse order as required by the FIR filter
      iinline_coeff = COEFF_ONE;
      for (int i = NUM_COEFFS-1; i >= 0; i--) begin
        icross_coeffs[i] = 0;
        qinline_coeffs[i] = $urandom_range(0,2**kQINLINE_COEFFSize);
      end

      // push single pulse (half size) into the filter
      reset_samples();
      push_sample('0, 16'h4000);
      test_data_path(0);

      // add more data to test filter adder tree
      for (int i = 0; i < 100; i++) begin
        isample_in.push_back('0);
        qsample_in.push_back($urandom_range(0,2**16));
      end
      test_data_path(0);

      test.end_test();
    endtask;

    // Checking full arithmetic
    task test_full_arithmetic();
      test.start_test("Checking full arithmetic", TEST_TIMEOUT);

      // assign random coefficients in reverse order as required by the FIR filter
      iinline_coeff = $urandom_range(0,2**kIINLINE_COEFFSize);
      for (int i = NUM_COEFFS-1; i >= 0; i--) begin
        icross_coeffs[i] = $urandom_range(0,2**kICROSS_COEFFSize);
        qinline_coeffs[i] = $urandom_range(0,2**kQINLINE_COEFFSize);
      end

      // add more data to test filter adder tree
      reset_samples();
      for (int i = 0; i < 1000; i++) begin
        isample_in.push_back($urandom_range(0,2**16));
        qsample_in.push_back($urandom_range(0,2**16));
      end
      test_data_path($urandom_range(0,NUM_COEFFS-1));

      test.end_test();
    endtask;

    // run all tests
    task run_all_tests();
      verify_num_coeffs();
      initialize_filter();
      test_alignment_icross();
      test_alignment_qinline();
      test_icross_fir();
      test_qinline_fir();
      test_full_arithmetic();
    endtask;

  endclass
endpackage

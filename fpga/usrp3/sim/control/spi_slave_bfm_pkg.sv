//
// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Package: spi_slave_bfm_pkg
//
// Description:
//
//   Package defining a SPI slave bus functional model (BFM).
//
//   The only supported SPI mode is CPOL=0, CPHA=0.
//   (clock is low during idle, data is captured on rising edge, data is changed
//   on falling edge)
//

package spi_slave_bfm_pkg;

  timeunit 1ns / 1ps;

  class spi_slave_bfm #(int TRANSACTION_LENGTH = 32);

    local mailbox #(logic [TRANSACTION_LENGTH-1:0]) mosi_mailbox;
    local mailbox #(logic [TRANSACTION_LENGTH-1:0]) miso_mailbox;

    // link to interface
    local virtual spi_if iface;


    local int unsigned error_count;

    // ------------------------------------------------------------------------
    // Constructor
    // ------------------------------------------------------------------------
    // Class constructor. This BFM models a SPI slave and must be given a
    // single interface providing the SPI signals for the slave connection.
    function new(virtual spi_if iface);
      this.iface = iface;
      mosi_mailbox = new;
      miso_mailbox = new;
      error_count = 0;
    endfunction

    // Number of errors reported by the BFM.
    function int unsigned get_error_count();
      return error_count;
    endfunction

    // ------------------------------------------------------------------------
    // Asynchronous transaction handling
    // ------------------------------------------------------------------------
    // Queue a write and read transaction
    task async_write_read(
      logic[TRANSACTION_LENGTH-1:0] requested_data,
      logic[TRANSACTION_LENGTH-1:0] response_data
    );
      // put the requested data into the mosi mailbox
      mosi_mailbox.put(requested_data);
      // put the response data into the miso mailbox
      miso_mailbox.put(response_data);
    endtask;

    // wait for all transactions to be processed
    task wait_write_read_complete();
      while(mosi_mailbox.num() > 0 || miso_mailbox.num() > 0) begin
        @(posedge iface.cs_n);
      end
    endtask

    // ------------------------------------------------------------------------
    // Synchronous transaction handling
    // ------------------------------------------------------------------------
    task write_read(
      logic[TRANSACTION_LENGTH-1:0] requested_data,
      logic[TRANSACTION_LENGTH-1:0] response_data
      );
      async_write_read(requested_data, response_data);
      wait_write_read_complete();
    endtask

    // ------------------------------------------------------------------------
    // Transaction handling
    // ------------------------------------------------------------------------

    // Start the BFM
    // This will spawn separate threads for the interface handling.
    task run();
      fork
        cs_sck_check_body();
        mosi_capture_body();
        miso_response_body();
      join_none
    endtask

    local function void report_error(string msg);
      error_count++;
      $error("%s @ t=%0t", msg, $time);
    endfunction

    // check constantly the status of the CS and SCK signal
    task cs_sck_check_body();
      automatic int clock_edge_counter = 0;

      // check constantly the timing of the CS edges
      forever begin
        // reset the clock counter to 0 with every SPI transaction start
        @(negedge iface.cs_n);
        clock_edge_counter = 0;
        // wait for SPI transaction to finish or clock edge
        while (iface.cs_n == 0) begin
          @(posedge iface.cs_n or edge iface.sck);
          if (iface.cs_n == 0) begin
            // if there is a clock edge during the transaction, count it
            clock_edge_counter++;
            // check if the transaction length is correct
            // 2 clock edges per transaction bit
            assert(clock_edge_counter <= 2*TRANSACTION_LENGTH) else
              report_error($sformatf(
                "Too many SCLK cycles received. Clock cycles: %0d", clock_edge_counter));
          end
        end
        // ignore special case when cs turn to high impedance
        if (iface.cs_n !== 'z) begin
          // if cs goes high check if the transaction length is correct
          // 2 clock edges per transaction bit
          assert(clock_edge_counter == 2*TRANSACTION_LENGTH) else
            report_error($sformatf(
              "Unexpected SPI CS interrupt. Clock cycles: %0d", clock_edge_counter));
        end
      end
    endtask

    // capture data on the mosi line
    task mosi_capture_body();
      // Check constantly for new transmitted data.
      // When data is written to the SPI chip clock in the mosi data
      // and compare it with the expected data from the testbench.

      automatic logic [TRANSACTION_LENGTH-1:0] expected_data;
      automatic logic [TRANSACTION_LENGTH-1:0] received_data;

      forever begin
        // wait for SPI CS to go low
        @(negedge iface.cs_n);
        // check if there is a new value in mailbox
        if (mosi_mailbox.try_get(expected_data)) begin
          // clock in the mosi data with every rising SPI clock edge
          for (int i = 0; i < TRANSACTION_LENGTH; i++) begin
            // wait for rising edges of SPI clock
            @(posedge iface.sck);
            // read mosi data from master
            received_data[TRANSACTION_LENGTH-1-i] = iface.mosi;
          end
          // compare received data with expected data
          assert(received_data == expected_data) else
            report_error($sformatf(
              "Unexpected data received on SPI interface. Expected: %h Received: %h",
              expected_data, received_data));
        end else begin
          // ignore special case when cs turn to high impedance
          if (iface.cs_n !== 'z) begin
            report_error("No data available in SPI BFM MOSI mailbox.");
          end
        end
      end
    endtask

    // respond with data on the miso line
    task miso_response_body();
      // Check constantly for new transactions.
      // When a read transaction is started clock out the response data
      // from the SPI slave e.g. a PLL chip onto the miso port.

      automatic logic [TRANSACTION_LENGTH-1:0] response_data;

      forever begin
        // default to high impedance
        iface.miso <= 'z;

        // wait for SPI CS to go low
        @(negedge iface.cs_n);
        // check if there is a new value in mailbox
        if (miso_mailbox.try_get(response_data)) begin
          // clock out the response data with every falling SPI clock edge
          for (int i = 0; i < TRANSACTION_LENGTH; i++) begin
            // send data on miso line back to the master
            iface.miso <= response_data[TRANSACTION_LENGTH-1-i];
            // wait for falling edges of SPI clock
            @(negedge iface.sck);
          end
          // wait for SPI CS to go high
          @(posedge iface.cs_n);
        end else begin
          // ignore special case when cs turn to high impedance
          if (iface.cs_n !== 'z) begin
            report_error("No data available in SPI BFM MISO mailbox.");
          end
        end
      end
    endtask

  endclass

  // ------------------------------------------------------------------------
  // SPI timing monitor BFM
  // ------------------------------------------------------------------------
  // Passive monitor for CPOL=0, CPHA=0 SPI timing checks.
  //
  // On each transaction (CS_N low pulse), the monitor checks:
  //   1) CS_N assert to first SCLK rise (minimum)
  //   2) SCLK high period (exact)
  //   3) SCLK low period:
  //      - intra-byte: exact
  //      - inter-byte: minimum (optional)
  //   4) CS hold time after final SCLK fall (minimum)
  //   5) CS guard time between transactions (minimum)
  //
  // The monitor is reusable across tests within a testbench by updating timing with
  // set_timing() when expected timings change.
  class spi_timing_monitor_bfm;

    local virtual spi_if iface;

    local time half_per_time;
    local time cs_hold_time;
    local time cs_to_first_rise_time;
    local time         cs_guard_time;

    local int unsigned bits_per_byte;
    local bit          check_inter_byte_gaps;

    local int unsigned transaction_count;
    local int unsigned error_count;

    // ----------------------------------------------------------------------
    // Constructor
    // ----------------------------------------------------------------------
    // Arguments:
    //   iface:                   SPI interface to monitor
    //   half_per_time:           expected SCLK half period (must be non-zero)
    //   cs_hold_time:            minimum time from last SCLK fall to CS_N deassert
    //                            (0 disables CS hold checks)
    //   cs_to_first_rise_time:   minimum time from CS_N assert to first SCLK rise
    //                            (0 disables first-rise check)
    //   cs_guard_time:           minimum time between transactions
    //   bits_per_byte:           bit grouping for inter-byte low-gap checks
    //   check_inter_byte_gaps:   enable minimum-gap checks at byte boundaries
    function new(
      virtual spi_if iface,
      time half_per_time,
      time cs_hold_time = 0,
      time cs_to_first_rise_time = 0,
      time cs_guard_time = 0,
      int unsigned bits_per_byte = 8,
      bit check_inter_byte_gaps = 1'b1
    );
      assert (half_per_time != 0)
        else $fatal(1, "half_per_time must be non-zero");

      this.iface                  = iface;
      this.half_per_time          = half_per_time;
      this.cs_hold_time           = cs_hold_time;
      this.cs_to_first_rise_time  = cs_to_first_rise_time;
      this.cs_guard_time          = cs_guard_time;
      this.bits_per_byte          = bits_per_byte;
      this.check_inter_byte_gaps  = check_inter_byte_gaps;
      this.transaction_count      = 0;
      this.error_count            = 0;
    endfunction

    // Update timing expectations at runtime.
    // Use this after changing SPI rate configuration in the DUT.
    function void set_timing(
      time half_per_time,
      time cs_hold_time,
      time cs_to_first_rise_time,
      time cs_guard_time
    );
      assert (half_per_time != 0)
        else $fatal(1, "half_per_time must be non-zero");

      this.half_per_time     = half_per_time;
      this.cs_hold_time      = cs_hold_time;
      this.cs_to_first_rise_time = cs_to_first_rise_time;
      this.cs_guard_time     = cs_guard_time;
    endfunction

    // Number of observed CS_N assertions (transactions).
    function int unsigned get_transaction_count();
      return transaction_count;
    endfunction

    // Number of timing violations reported by the monitor.
    function int unsigned get_error_count();
      return error_count;
    endfunction

    // Start monitoring. Intended to run in a forked process.
    task run();
      fork
        monitor_body();
      join_none;
    endtask

    local function void report_error(string msg);
      error_count++;
      $error("%s @ t=%0t", msg, $time);
    endfunction

    // Main timing monitor loop.
    // This runs forever and validates each SPI transaction independently.
    local task monitor_body();
      time t_cs_assert;
      time t_rise;
      time t_fall;
      time t_cs_deassert;
      time t_cs_deassert_prev;
      int  rise_cnt;
      bit  first_txn;

      first_txn = 1'b1;

      forever begin

        //-----------------------------------------------------------------------
        // Wait for CS_N assertion (start of transaction)
        //-----------------------------------------------------------------------
        @(negedge iface.cs_n);
        t_cs_assert = $time;
        transaction_count++;

        //-----------------------------------------------------------------------
        // Check 5: CS_N guard: previous transaction's CS_N deassert must be >=
        // CS_GUARD_TIME before this assertion. Checked here (at the
        // start of each transaction) so the guard @(negedge iface.cs_n) doesn't
        // consume a transaction count. Skipped on the first transaction.
        //-----------------------------------------------------------------------
        if (!first_txn) begin
          if (t_cs_assert - t_cs_deassert_prev < cs_guard_time) begin
            report_error($sformatf(
              "CS_N guard too short: minimum %0t, got %0t",
              cs_guard_time,
              t_cs_assert - t_cs_deassert_prev));
          end
        end
        first_txn = 1'b0;

        //-----------------------------------------------------------------------
        // Check 1: CS_N assert -> first SCLK rising edge >= cs_to_first_rise_time
        //-----------------------------------------------------------------------
        @(posedge iface.sck);
        t_rise = $time;
        rise_cnt = 1;
        if ((cs_to_first_rise_time != 0) && (t_rise - t_cs_assert < cs_to_first_rise_time)) begin
          report_error($sformatf(
            "CS_N assert to first SCLK rise too short: minimum %0t, got %0t",
            cs_to_first_rise_time, t_rise - t_cs_assert));
        end


        //-----------------------------------------------------------------------
        // Checks 2, 3, 4: Walk all SCLK edges until CS_N deasserts.
        //
        // State at loop top: we are at a posedge sclk; rise_cnt is current.
        //
        //   Step A: Wait for negedge sclk OR posedge cs_n.
        //   Step B: If cs_n is still low, wait for posedge sclk OR posedge
        //           cs_n.
        //-----------------------------------------------------------------------
        forever begin

          // Step A: wait for SCLK fall (or simultaneous CS_N deassert)
          fork : wait_fall
            begin : fall_th @(negedge iface.sck); end
            begin : cs0_th  @(posedge iface.cs_n); end
          join_any
          disable wait_fall;
          t_fall = $time;

          // Check 2: SCLK high period must be exactly half_per_time
          // This indirectly also ensures that CS is not deasserted too early
          if (t_fall - t_rise != half_per_time) begin
            report_error($sformatf(
              "SCLK high period: expected %0t, got %0t",
              half_per_time, t_fall - t_rise));
          end

          if (iface.cs_n) begin
            // CS_N deasserted on the same clock edge as last SCLK
            t_cs_deassert = t_fall;
            if ((cs_hold_time != 0)) begin
              report_error($sformatf(
                "CS hold time at SCLK fall too short: minimum %0t, got 0",
                cs_hold_time));
            end
            break;
          end

          // Step B: wait for SCLK rise or CS_N deassert (CS_HOLD=1)
          fork : wait_rise
            begin : rise_th @(posedge iface.sck); end
            begin : cs1_th  @(posedge iface.cs_n); end
          join_any
          disable wait_rise;

          if (iface.cs_n) begin
            // CS_N deasserted after the last SCLK fall; enforce minimum hold.
            t_cs_deassert = $time;
            if ((cs_hold_time != 0) && (t_cs_deassert - t_fall < cs_hold_time)) begin
              report_error($sformatf(
                "CS hold time after last SCLK fall too short: minimum %0t, got %0t",
                cs_hold_time, t_cs_deassert - t_fall));
            end
            break;
          end

          t_rise = $time;
          rise_cnt++;

          // Check 3: SCLK low period.
          //
          // rise_cnt just incremented, so the preceding fall was fall number
          // (rise_cnt - 1). A fall at a multiple of bits_per_byte is an
          // inter-byte boundary
          // where the FSM waits in ST_IDLE for the next byte; duration is
          // unbounded but must be >= 1 half-period. All other falls are
          // intra-byte and must be exactly 1 half-period.
          if (check_inter_byte_gaps && ((rise_cnt - 1) % bits_per_byte == 0)) begin
            if (t_rise - t_fall < half_per_time) begin
              report_error($sformatf(
                "SCLK low period (inter-byte) too short: minimum %0t, got %0t",
                half_per_time, t_rise - t_fall));
            end
          end else begin
            if (t_rise - t_fall != half_per_time) begin
              report_error($sformatf(
                "SCLK low period: expected %0t, got %0t",
                half_per_time, t_rise - t_fall));
            end
          end

            // CS_N is still low and we got posedge sclk; continue to next bit

        end // inner forever (SCLK edge walk)

        // Save deassert time for next iteration's guard check
        t_cs_deassert_prev = t_cs_deassert;
      end // outer forever (transaction loop)
    endtask

  endclass

endpackage

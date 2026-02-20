//
// Copyright 2025 Ettus Research, a National Instruments Brand
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

  class spi_slave_bfm #(int TRANSACTION_LENGTH = 32);

    local mailbox #(logic [TRANSACTION_LENGTH-1:0]) mosi_mailbox;
    local mailbox #(logic [TRANSACTION_LENGTH-1:0]) miso_mailbox;

    // link to interface
    local virtual spi_if iface;

    // ------------------------------------------------------------------------
    // Constructor
    // ------------------------------------------------------------------------
    // Class constructor. This BFM models a SPI slave and must be given a
    // single interface providing the SPI signals for the slave connection.
    function new(virtual spi_if iface);
      this.iface = iface;
      mosi_mailbox = new;
      miso_mailbox = new;
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
              $error("Too many SCLK cycles received. Clock cycles: %0d", clock_edge_counter);
          end
        end
        // ignore special case when cs turn to high impedance
        if (iface.cs_n !== 'z) begin
          // if cs goes high check if the transaction length is correct
          // 2 clock edges per transaction bit
          assert(clock_edge_counter == 2*TRANSACTION_LENGTH) else
            $error("Unexpected SPI CS interrupt. Clock cycles: %0d", clock_edge_counter);
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
            $error("Unexpected data received on SPI interface. Expected: %h Received: %h",
              expected_data, received_data);
        end else begin
          // ignore special case when cs turn to high impedance
          if (iface.cs_n !== 'z) begin
            $error("No data available in SPI BFM MOSI mailbox.");
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
            $error("No data available in SPI BFM MISO mailbox.");
          end
        end
      end
    endtask

  endclass

endpackage

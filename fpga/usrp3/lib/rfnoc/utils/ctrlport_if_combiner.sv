//
// Copyright 2025 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: ctrlport_if_combiner
//
// Description:
//
//  This block is an arbiter that merges control-port interfaces. This block is
//  used when you have multiple control-port masters that need to access a
//  single slave. For example, a NoC block with multiple submodules that each
//  need to read and/or write registers outside of themselves.
//
//  This module combines the control-port requests from multiple masters into a
//  single request for one slave. Simultaneous requests are handled in the order
//  specified by PRIORITY. The responding ACK is routed back to the requester.
//
//  The module has been designed so that the latency through it is always the
//  same when PRIORITY=1 and there is no contention, so that it can be used in
//  applications where deterministic behavior is desired.
//
// Parameters:
//
//   NUM_MASTERS : The number of control-port masters to connect to a single
//                 control-port slave.
//   PRIORITY    : Use PRIORITY = 0 for round robin arbitration, PRIORITY = 1
//                 for priority arbitration (lowest number port serviced first).
//   TIMEOUT     : Number of cycles to wait for a response before aborting the wait for an ack.
//                 TIMEOUT = 0 will not add a timeout.
//


module ctrlport_if_combiner #(
  int NUM_MASTERS = 2,
  bit PRIORITY    = 0,
  int TIMEOUT     = 0
) (
  // Slave Interfaces
  ctrlport_if.slave s_ctrlport [NUM_MASTERS-1:0],
  // Master Interface
  ctrlport_if.master m_ctrlport
);

  import ctrlport_pkg::*;

  logic [$clog2(NUM_MASTERS)-1:0] slave_sel = '0;  // Tracks which slave port is
                                                   // currently being serviced.
  logic req_load_output = '0;
  logic timeout_occurred = '0;

  // Helper function to convert one hot vector to binary index
  // (LSB = index 0)
  function integer one_hot_to_binary(input [NUM_MASTERS-1:0] one_hot_vec);
    integer i, total;
  begin
    total = 0;
    for (i = 0; i < NUM_MASTERS; i++) begin
      if (one_hot_vec[i]) begin
        total = total + i;
      end
    end
    one_hot_to_binary = total;
  end
  endfunction

  //---------------------------------------------------------------------------
  // Input Registers
  //---------------------------------------------------------------------------
  //
  // Latch each request until it can be serviced. Only one request per slave
  // can be in progress at a time.
  //
  //---------------------------------------------------------------------------
  ctrlport_request_t req_buffer [NUM_MASTERS-1:0];
  logic [NUM_MASTERS-1:0] req_valid;

  for (genvar i = 0; i < NUM_MASTERS; i++) begin : gen_input_regs
    always_ff @(posedge m_ctrlport.clk) begin
      if (m_ctrlport.rst) begin
        req_valid[i] <= '0;
      end else begin
        if (s_ctrlport[i].req.wr | s_ctrlport[i].req.rd) begin
          // Mark this slave's request valid and save the request information
          req_valid[i]  <= '1;
        end

        // Clear the active request when it gets output
        if (req_load_output && (i == slave_sel)) begin
          req_valid[i] <= '0;
        end
      end

      // Save buffer without reset
      if (s_ctrlport[i].req.wr | s_ctrlport[i].req.rd) begin
        req_buffer[i] <= s_ctrlport[i].req;
      end
    end
  end

  //---------------------------------------------------------------------------
  // Arbitration State Machine
  //---------------------------------------------------------------------------
  //
  // This state machine tracks which slave port is being serviced and which to
  // service next. This is done using a counter that simply checks each port in
  // sequential order and then stops when it finds one that has a valid request.
  //
  //---------------------------------------------------------------------------

  logic req_active = '0;  // Indicates if there's a request being serviced
  logic [NUM_MASTERS-1:0] next_slave_one_hot; // one hot for next active request
                                             // (used for PRIORITY = 1)

  for (genvar i = 0; i < NUM_MASTERS; i++) begin : gen_next_slave_one_hot
    if (i == 0) begin
      assign next_slave_one_hot[i] = req_valid[i];
    end else begin
      assign next_slave_one_hot[i] = req_valid[i] & ~next_slave_one_hot[i-1];
    end
  end

  always_ff @(posedge m_ctrlport.clk) begin
    if (m_ctrlport.rst) begin
      slave_sel       <= '0;
      req_active      <= '0;
      req_load_output <= '0;
    end else begin
      req_load_output <= '0;

      if (req_active) begin
        // Wait until we get the response before we allow another request
        if (m_ctrlport.resp.ack || timeout_occurred) begin
          req_active <= '0;

          // Go to next slave immediately
          if(PRIORITY == 1)
            slave_sel <= one_hot_to_binary(next_slave_one_hot);
          // Round robin - Go to the next slave so we don't service the same
          // slave again
          else if(slave_sel == NUM_MASTERS-1)
            slave_sel <= '0;
          else
            slave_sel <= slave_sel + 1;
        end
      end else begin
        // No active request in progress, so check if there's a new request on
        // the selected slave.
        if (req_valid[slave_sel]) begin
          req_active      <= '1;
          req_load_output <= '1;
        end else begin
          // Go to next slave immediately
          if(PRIORITY == 1)
            slave_sel <= one_hot_to_binary(next_slave_one_hot);
          // Round robin - Nothing from this slave, so move to the next slave.
          else if (slave_sel == NUM_MASTERS-1)
            slave_sel <= '0;
          else
            slave_sel <= slave_sel + 1;
        end
      end
    end
  end


  //---------------------------------------------------------------------------
  // Output Register
  //---------------------------------------------------------------------------
  //
  // Here we load the active request for a single clock cycle and demultiplex
  // the response back to the requesting master.
  //
  //---------------------------------------------------------------------------

  always_ff @(posedge m_ctrlport.clk) begin
    // Load the active request
    if (req_load_output) begin
      m_ctrlport.req <= req_buffer[slave_sel];
    end else begin
      m_ctrlport.req.wr <= '0;
      m_ctrlport.req.rd <= '0;
    end

    if (m_ctrlport.rst) begin
      // only reset the flags of the master interface
      m_ctrlport.req.wr <= '0;
      m_ctrlport.req.rd <= '0;
    end
  end

  // Output any response to the master that made the request
  for (genvar i = 0; i < NUM_MASTERS; i++) begin : gen_output_regs
    always_ff @(posedge m_ctrlport.clk) begin
      // Give the response data to all the slaves (no demux, to save logic)
      s_ctrlport[i].resp <= m_ctrlport.resp;

      // Give the ack only to the master that made the request (use a demux)
      if (m_ctrlport.rst) begin
        s_ctrlport[i].resp.ack <= '0;
      end else if (req_active && i == slave_sel && m_ctrlport.resp.ack) begin
        s_ctrlport[i].resp.ack <= '1;
      end else begin
        s_ctrlport[i].resp.ack <= '0;
      end
    end
  end

  //---------------------------------------------------------------------------
  // optional watchdog
  //---------------------------------------------------------------------------
  if (TIMEOUT == 0) begin: gen_no_timeout
    // never set a timeout but in always_ff block to avoid error with synchronous assignment
    // in gen_timeout block below
    always_ff @(posedge m_ctrlport.clk) begin
      timeout_occurred = '0;
    end

  end else begin: gen_timeout
    // When the timeout occurred it takes 2 more clock cycles for the counter to reach zero
    // The path is: timeout_occurred -> req_active = 0 -> timeout_counter = 0
    // As the timeout counter has a minimum width of 2 bits the counter cannot reach TIMEOUT+1
    // again within these two clock cycles.
    // Therefore no other reset other than req_active is needed on this signal.
    logic [$clog2(TIMEOUT+2):0] timeout_counter = '0;

    // Reset the timeout counter when there is no active request
    always_ff @(posedge m_ctrlport.clk) begin
      if (~req_active) begin
        timeout_counter <= '0;
      end else begin
        timeout_counter <= timeout_counter + 1;
      end

      // Set the timeout flag for one cycle when the counter reaches the timeout value.
      // The delay of the additional register compensates the delay of the logic above to
      // load the active request into the output register.
      timeout_occurred <= timeout_counter == TIMEOUT;

      // synthesis translate_off
      if (timeout_occurred && req_active) begin
        $warning("Ctrlport combiner timeout occurred!");
      end
      // synthesis translate_on
    end
  end

endmodule

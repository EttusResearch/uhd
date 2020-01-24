//
// Copyright 2019 Ettus Research, A National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: ctrlport_combiner
//
// Description:
//
// This block is an arbiter that merges control-port interfaces. This block is 
// used when you have multiple control-port masters that need to access a 
// single slave. For example, a NoC block with multiple submodules that each 
// need to read and/or write registers outside of themselves.
//
// This module combines the control-port requests from multiple masters into a 
// single request for one slave. Simultaneous requests are handled in the order 
// specified by PRIORITY. The responding ACK is routed back to the requester.
//
// Parameters:
//
//   NUM_MASTERS : The number of control-port masters to connect to a single 
//                 control-port slave.
//   PRIORITY    : Use PRIORITY = 0 for round robin arbitration, PRIORITY = 1 
//                 for priority arbitration (lowest number port serviced first).
//


module ctrlport_combiner #(
  parameter NUM_MASTERS = 2,
  parameter PRIORITY    = 0
) (
  input wire ctrlport_clk,
  input wire ctrlport_rst,

  // Requests from multiple masters
  input  wire [   NUM_MASTERS-1:0] s_ctrlport_req_wr,
  input  wire [   NUM_MASTERS-1:0] s_ctrlport_req_rd,
  input  wire [20*NUM_MASTERS-1:0] s_ctrlport_req_addr,
  input  wire [10*NUM_MASTERS-1:0] s_ctrlport_req_portid,
  input  wire [16*NUM_MASTERS-1:0] s_ctrlport_req_rem_epid,
  input  wire [10*NUM_MASTERS-1:0] s_ctrlport_req_rem_portid,
  input  wire [32*NUM_MASTERS-1:0] s_ctrlport_req_data,
  input  wire [ 4*NUM_MASTERS-1:0] s_ctrlport_req_byte_en,
  input  wire [   NUM_MASTERS-1:0] s_ctrlport_req_has_time,
  input  wire [64*NUM_MASTERS-1:0] s_ctrlport_req_time,
  // Responses to multiple masters
  output reg  [   NUM_MASTERS-1:0] s_ctrlport_resp_ack,
  output reg  [ 2*NUM_MASTERS-1:0] s_ctrlport_resp_status,
  output reg  [32*NUM_MASTERS-1:0] s_ctrlport_resp_data,

  // Request to a single slave
  output reg         m_ctrlport_req_wr,
  output reg         m_ctrlport_req_rd,
  output reg  [19:0] m_ctrlport_req_addr,
  output reg  [ 9:0] m_ctrlport_req_portid,
  output reg  [15:0] m_ctrlport_req_rem_epid,
  output reg  [ 9:0] m_ctrlport_req_rem_portid,
  output reg  [31:0] m_ctrlport_req_data,
  output reg  [ 3:0] m_ctrlport_req_byte_en,
  output reg         m_ctrlport_req_has_time,
  output reg  [63:0] m_ctrlport_req_time,
  // Response from a single slave
  input  wire        m_ctrlport_resp_ack,
  input  wire [ 1:0] m_ctrlport_resp_status,
  input  wire [31:0] m_ctrlport_resp_data
);

  reg [$clog2(NUM_MASTERS)-1:0] slave_sel = 0;  // Tracks which slave port is
                                                // currently being serviced.
  reg req_load_output = 1'b0;


  //---------------------------------------------------------------------------
  // Input Registers
  //---------------------------------------------------------------------------
  //
  // Latch each request until it can be serviced. Only one request per slave
  // can be in progress at a time.
  //
  //---------------------------------------------------------------------------

  reg [   NUM_MASTERS-1:0] req_valid = 0;
  reg [   NUM_MASTERS-1:0] req_wr;
  reg [   NUM_MASTERS-1:0] req_rd;
  reg [20*NUM_MASTERS-1:0] req_addr;
  reg [10*NUM_MASTERS-1:0] req_portid;
  reg [16*NUM_MASTERS-1:0] req_rem_epid;
  reg [10*NUM_MASTERS-1:0] req_rem_portid;
  reg [32*NUM_MASTERS-1:0] req_data;
  reg [ 4*NUM_MASTERS-1:0] req_byte_en;
  reg [   NUM_MASTERS-1:0] req_has_time;
  reg [64*NUM_MASTERS-1:0] req_time;

  always @(posedge ctrlport_clk) begin
    if (ctrlport_rst) begin
      req_valid <= 0;
    end else begin : input_reg_gen
      integer i;
      for (i = 0; i < NUM_MASTERS; i = i + 1) begin
        if (s_ctrlport_req_wr[i] | s_ctrlport_req_rd[i]) begin
          // Mark this slave's request valid and save the request information
          req_valid[i]             <= 1'b1;
          req_wr[i]                <= s_ctrlport_req_wr[i];
          req_rd[i]                <= s_ctrlport_req_rd[i];
          req_addr[20*i+:20]       <= s_ctrlport_req_addr[20*i+:20];
          req_portid[10*i+:10]     <= s_ctrlport_req_portid[10*i+:10];
          req_rem_epid[16*i+:16]   <= s_ctrlport_req_rem_epid[16*i+:16];
          req_rem_portid[10*i+:10] <= s_ctrlport_req_rem_portid[10*i+:10];
          req_data[32*i+:32]       <= s_ctrlport_req_data[32*i+:32];
          req_byte_en[4*i+:4]      <= s_ctrlport_req_byte_en[4*i+:4];
          req_has_time[i]          <= s_ctrlport_req_has_time[i];
          req_time[64*i+:64]       <= s_ctrlport_req_time[64*i+:64];
        end
      end

      // Clear the active request when it gets output
      if (req_load_output) begin
        req_valid[slave_sel] <= 1'b0;
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

  reg req_active = 0;  // Indicates if there's a request being serviced

  always @(posedge ctrlport_clk) begin
    if (ctrlport_rst) begin
      slave_sel       <= 0;
      req_active      <= 1'b0;
      req_load_output <= 1'b0;
    end else begin
      req_load_output <= 1'b0;

      if (req_active) begin
        // Wait until we get the response before we allow another request
        if (m_ctrlport_resp_ack) begin
          req_active <= 1'b0;

          // Go to the next slave so we don't service the same slave again
          if(PRIORITY == 1 || slave_sel == NUM_MASTERS-1)
            slave_sel <= 0;
          else
            slave_sel <= slave_sel + 1;
        end
      end else begin
        // No active request in progress, so check if there's a new request on
        // the selected slave.
        if (req_valid[slave_sel]) begin
          req_active      <= 1'b1;
          req_load_output <= 1'b1;
        end else begin
          // Nothing from this slave, so move to the next slave.
          if (slave_sel == NUM_MASTERS-1)
            slave_sel <= 0;
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

  always @(posedge ctrlport_clk) begin
    if (ctrlport_rst) begin
      m_ctrlport_req_wr <= 1'b0;
      m_ctrlport_req_rd <= 1'b0;
    end else begin : output_reg_gen
      integer i;

      // Load the active request
      if (req_load_output) begin
        m_ctrlport_req_wr         <= req_wr        [slave_sel];
        m_ctrlport_req_rd         <= req_rd        [slave_sel];
        m_ctrlport_req_addr       <= req_addr      [20*slave_sel +: 20];
        m_ctrlport_req_portid     <= req_portid    [10*slave_sel +: 10];
        m_ctrlport_req_rem_epid   <= req_rem_epid  [16*slave_sel +: 16];
        m_ctrlport_req_rem_portid <= req_rem_portid[10*slave_sel +: 10];
        m_ctrlport_req_data       <= req_data      [32*slave_sel +: 32];
        m_ctrlport_req_byte_en    <= req_byte_en   [ 4*slave_sel +: 4];
        m_ctrlport_req_has_time   <= req_has_time  [slave_sel];
        m_ctrlport_req_time       <= req_time      [64*slave_sel +: 64];
      end else begin
        m_ctrlport_req_wr <= 1'b0;
        m_ctrlport_req_rd <= 1'b0;
      end

      // Output any response to the master that made the request
      for (i = 0; i < NUM_MASTERS; i = i + 1) begin
        // Give the response data to all the slaves (no demux, to save logic)
        s_ctrlport_resp_status[2*i +: 2] <= m_ctrlport_resp_status;
        s_ctrlport_resp_data[32*i +: 32] <= m_ctrlport_resp_data;

        // Give the ack only to the master that made the request (use a demux)
        if (i == slave_sel && m_ctrlport_resp_ack) begin
          s_ctrlport_resp_ack[i] <= 1'b1;
        end else begin
          s_ctrlport_resp_ack[i] <= 1'b0;
        end
      end
    end
  end

endmodule

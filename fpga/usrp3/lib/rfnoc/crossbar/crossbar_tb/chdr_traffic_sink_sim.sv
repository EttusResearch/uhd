//
// Copyright 2018 Ettus Research, A National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: chdr_traffic_sink_sim
// Description:
//   A sink for CHDR traffic. Simulation only.
//   Accepts packets and computes the following metrics:
//   - Data integrity errors
//   - Packet latency
//   - Throughput counts
//   All metrics can optionally be written to a file to
//   generate load-latency graphs.

`timescale 1ns/1ps

`include "sim_cvita_lib.svh"

module chdr_traffic_sink_sim #(
  parameter        WIDTH     = 64,
  parameter        MTU       = 5,
  parameter [15:0] NODE_ID   = 'd0,
  parameter [15:0] NUM_NODES = 'd16,
  parameter        FILE_PATH = ".",
  parameter        FLUSH_N   = 4
) (
  // Clocks and resets
  input             clk,
  input             rst,
  // Settings       
  input [63:0]      current_time,
  input             start_stb,
  input [7:0]       injection_rate,
  input [15:0]      lines_per_pkt,
  input [7:0]       traffic_patt,
  // CHDR master interface
  input [WIDTH-1:0] s_axis_tdata,
  input             s_axis_tlast,
  input             s_axis_tvalid,
  output            s_axis_tready,
  // Metrics        
  output            session_active,
  output [31:0]     xfer_count,
  output [31:0]     pkt_count,
  output [31:0]     data_err_count,
  output [31:0]     route_err_count
);

  // Constants
  localparam integer ERR_BIT_PKT_SIZE_MISMATCH = 1;
  localparam integer ERR_BIT_PKT_DATA_MISMATCH = 2;
  localparam integer ERR_BIT_PKT_DEST_MISMATCH = 4;
  localparam integer ERR_BIT_PKT_SEQUENCE_ERR  = 8;

  cvita_slave  #(.DWIDTH(WIDTH)) s_chdr (.clk(clk));
  cvita_pkt_t  pkt;

  assign s_chdr.axis.tdata = s_axis_tdata;
  assign s_chdr.axis.tlast = s_axis_tlast;
  assign s_chdr.axis.tvalid = s_axis_tvalid;
  assign s_axis_tready = s_chdr.axis.tready;

  logic         running = 0;
  integer       num_data_errs = 0;
  integer       num_route_errs = 0;
  logic [31:0]  num_pkts_xferd = 0;
  logic [31:0]  num_samps_xferd = 0;

  assign data_err_count = num_data_errs;
  assign route_err_count = num_route_errs;
  assign xfer_count = num_samps_xferd;
  assign pkt_count = num_pkts_xferd;
  assign session_active = running;

  integer session = 0;
  string filename;
  integer handle = 0;
  integer err = 0;
  integer bus_idle_cnt = 0;
  logic [WIDTH-1:0] i;

  // Egress buff in source is MTU + 4
  localparam integer IDLE_TIMEOUT = (1 << (MTU + 4 + FLUSH_N));

  initial begin: consume_blk
    // Consume infinitely
    s_chdr.reset();
    while (1) begin
      // A session begins on the posedge of start_stb
      while (~start_stb) @(posedge clk);
      session = session + 1;
      $sformat(filename, "%s/pkts_node%05d_inj%03d_lpp%05d_traffic%c_sess%04d.csv",
        FILE_PATH, NODE_ID, injection_rate, lines_per_pkt, traffic_patt, session);
      if (FILE_PATH != "") begin
        handle = $fopen(filename, "w");
        if (handle == 0) begin
          $error("Could not open file: %s", filename);
          $finish();
        end
      end
      if (handle != 0) $fdisplay(handle, "Src,Dst,Seqno,Error,Latency");
      s_chdr.reset();
      num_data_errs = 0;
      num_route_errs = 0;
      num_pkts_xferd = 0;
      num_samps_xferd = 0;
      bus_idle_cnt = 0;
      running = 1;
      while (1) begin
        // Pull packet from bus
        err = 0;
        if (~s_chdr.axis.tvalid[0]) begin
          @(posedge clk);
          bus_idle_cnt = bus_idle_cnt + 1;
          if (bus_idle_cnt <= IDLE_TIMEOUT)
            continue;
          else
            break;
        end
        s_chdr.pull_pkt(pkt, 0);
        bus_idle_cnt = 0;
        num_pkts_xferd = num_pkts_xferd + 1;
        num_samps_xferd = num_samps_xferd + lines_per_pkt;
        // Validate packet
        if (pkt.hdr.dst_sid != NODE_ID) begin
          err = err + ERR_BIT_PKT_DEST_MISMATCH;
          num_route_errs = num_route_errs + 1;
        end
        if (pkt.payload.size() != lines_per_pkt-2) begin
          err = err + ERR_BIT_PKT_SIZE_MISMATCH;
          num_data_errs = num_data_errs + 1;
        end else begin
          for (i = 'd0; i < (lines_per_pkt-2); i=i+1) begin
            if (pkt.payload[i] != i) begin
              err = err + ERR_BIT_PKT_DATA_MISMATCH;
              num_data_errs = num_data_errs + 1;
              break;
            end
          end
        end
        if (handle != 0) $fdisplay(handle, "%00d,%00d,%00d,%00d,%00d",
          pkt.hdr.src_sid, pkt.hdr.dst_sid, pkt.hdr.seqnum, err, (current_time - pkt.hdr.timestamp));
      end
      running = 0;
      if (handle != 0) $fclose(handle);
    end
  end

endmodule
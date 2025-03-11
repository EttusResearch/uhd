//
// Copyright 2025 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: rfdc_info_memory
//
// Description:
//
//   This module implements a memory that stores the RFDC information for the
//   RFDCs in the RFSoC. The memory is accessed via a controlport interface.
//   The memory is initialized by a device specfic pkg_rfdc_memory_content.
//   The memory is read only.
//

module rfdc_info_memory
(
  // Clocks and Resets
  input  logic  clk,
  input  logic  rst,

  // request
  input  logic [19:0] s_ctrlport_req_addr,
  input  logic [ 3:0] s_ctrlport_req_byte_en,
  input  logic [31:0] s_ctrlport_req_data,
  input  logic        s_ctrlport_req_rd,
  input  logic        s_ctrlport_req_wr,

  // response
  output logic        s_ctrlport_resp_ack,
  output logic [31:0] s_ctrlport_resp_data,
  output logic [ 1:0] s_ctrlport_resp_status
);

  `include "../../../../lib/rfnoc/core/ctrlport.vh"
  import rfdc_info_pkg::*;
  `ifdef X410
    import x410_rfdc_memory_content_pkg::*;
  `else
    import x440_rfdc_memory_content_pkg::*;
  `endif

  localparam int MEMORY_ADDR_SIZE = $clog2(NUM_ENTRIES);
  localparam int MEM_ADDR_INCR = 4;
  localparam int MEMORY_ADDR_LSB = $clog2(MEM_ADDR_INCR);

  // create memory with the RFDC information
  rfdc_memory_entry_t rfdc_memory [0:NUM_ENTRIES-1];

  initial begin
    // initialize memory
    for (int i = 0; i < NUM_ENTRIES; i++) begin
      rfdc_memory[i] = get_memory_entry(i);
      $info("Memory entry %d: %p", i, rfdc_memory[i]);
    end
  end

  // connect memory to the controlport interface
  always_ff @(posedge clk) begin
    if (rst) begin
      s_ctrlport_resp_ack    <= 1'b0;
      s_ctrlport_resp_data   <= 32'b0;
      s_ctrlport_resp_status <= 2'b00;

    // handle reads
    end else if (s_ctrlport_req_rd) begin
      s_ctrlport_resp_ack <= 1'b1;

      // check overrange
      if (s_ctrlport_req_addr >= NUM_ENTRIES*MEM_ADDR_INCR) begin
        s_ctrlport_resp_data   <= 32'b0;
        s_ctrlport_resp_status <= CTRL_STS_CMDERR;
      end else begin
        s_ctrlport_resp_data   <= rfdc_memory[s_ctrlport_req_addr[MEMORY_ADDR_LSB +: MEMORY_ADDR_SIZE]];
        s_ctrlport_resp_status <= CTRL_STS_OKAY;
      end

    // handle writes
    end else if (s_ctrlport_req_wr) begin
      s_ctrlport_resp_ack    <= 1'b1;
      s_ctrlport_resp_status <= CTRL_STS_CMDERR;

    // any other case
    end else begin
      s_ctrlport_resp_ack <= 1'b0;
    end
  end

endmodule

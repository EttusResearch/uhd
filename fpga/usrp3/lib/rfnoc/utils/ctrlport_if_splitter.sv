//
// Copyright 2025 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: ctrlport_if_splitter
//
// Description:
//
//  This block splits a single control-port interface into multiple. It is used
//  when you have a single master that needs to access multiple slaves. For
//  example, a NoC block where the registers are implemented in multiple
//  submodules that must be read/written by a single NoC shell.
//
//  Note that this block does not do any address decoding, so the connected
//  slaves must use non-overlapping address spaces.
//
//  This module takes the request received by its single slave interface and
//  outputs it on all its master interfaces. In the opposite direction, it takes
//  the responses received by its multiple master interfaces and combines them
//  into a single response on its slave interface. This is done by using the ack
//  bit of each response to mask the other bits of the response, then OR'ing all
//  of the masked responses together onto a single response bus. This is valid
//  because only one block is allowed to respond to a single request.
//
// Parameters:
//
//   NUM_SLAVES : The number of slaves you want to connect to a master.
//


module ctrlport_if_splitter #(
  int NUM_SLAVES = 2
) (
  // Slave Interface
  ctrlport_if.slave s_ctrlport,
  // Master Interfaces
  ctrlport_if.master m_ctrlport [NUM_SLAVES-1:0]
);

  import ctrlport_pkg::*;

  generate
    if (NUM_SLAVES == 1) begin : gen_no_split
      assign m_ctrlport[0].req = s_ctrlport.req;
      assign s_ctrlport.resp   = m_ctrlport[0].resp;
    end else begin : gen_splitter
      //---------------------------------------------------------------------------
      // Split the requests among the slaves
      //---------------------------------------------------------------------------
      for (genvar i = 0; i < NUM_SLAVES; i++) begin : gen_split
        // No special logic is required to split the requests from the master among
        // multiple slaves.
        assign m_ctrlport[i].req = s_ctrlport.req;
      end

      //---------------------------------------------------------------------------
      // Decode the responses
      //---------------------------------------------------------------------------
      // Take the responses and mask them with their respective ack
      ctrlport_response_t masked_resp [NUM_SLAVES-1:0];
      for (genvar i = 0; i < NUM_SLAVES; i++) begin : gen_mask
          assign masked_resp[i] = m_ctrlport[i].resp.ack ? m_ctrlport[i].resp : '0;
      end

      // Combine the masked responses by OR'ing them together
      ctrlport_response_t combined_resp;
      always_comb begin : response_combine
        combined_resp = '0;
        for (int i = 0; i < NUM_SLAVES; i++) begin : gen_or
          combined_resp = combined_resp | masked_resp[i];
        end
      end

      // Register the output to break combinatorial path
      always_ff @(posedge s_ctrlport.clk) begin : response_reg
        s_ctrlport.resp <= combined_resp;

        if (s_ctrlport.rst) begin
          s_ctrlport.resp.ack <= '0;
        end
      end
    end
  endgenerate
endmodule

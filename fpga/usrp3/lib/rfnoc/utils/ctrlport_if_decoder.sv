//
// Copyright 2025 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: ctrlport_if_decoder
//
// Description:
//
//   This block splits a single control port interface into multiple. It is
//   used when you have a single master that needs to access multiple slaves.
//
//   This version also implements address decoding. The request is passed to a
//   slave only if the address falls within that slave's address space. Each
//   slave can have a unique base address and address space size.
//
//   When passed to the slave, the base address is subtracted from the request
//   address.
//
// Parameters:
//
//   NUM_SLAVES : The number of slaves to connect to a master.
//   PORT_BASE  : Base addresses to use for each slave.
//   PORT_SIZE  : Size of the address space for each slave.
//

module ctrlport_if_decoder #(
  int NUM_SLAVES             = 2,
  int PORT_BASE [NUM_SLAVES] = '{'h0,   'h100},
  int PORT_SIZE [NUM_SLAVES] = '{'h100, 'h100}
) (
  // Slave Interface
  ctrlport_if.slave s_ctrlport,
  // Master Interfaces
  ctrlport_if.master m_ctrlport [NUM_SLAVES]
);

  import ctrlport_pkg::*;

  //---------------------------------------------------------------------------
  // Check the address ranges
  //---------------------------------------------------------------------------
  for (genvar i = 0; i < NUM_SLAVES; i = i+1) begin : gen_overlap_1
    for (genvar j = 0; j < NUM_SLAVES; j = j+1) begin : gen_overlap_2
      if (i != j) begin
        if ((PORT_BASE[i] >= PORT_BASE[j]) &&
            (PORT_BASE[i] < PORT_BASE[j] + PORT_SIZE[j])) begin
          $error("Port %0d overlaps with port %0d.", i, j);
        end
      end
    end
  end

  //---------------------------------------------------------------------------
  // Split the requests among the slaves
  //---------------------------------------------------------------------------
  for (genvar i = 0; i < NUM_SLAVES; i = i+1) begin : gen_split
    always_ff @(posedge s_ctrlport.clk) begin
      // unconditionally pass the request by default
      m_ctrlport[i].req <= s_ctrlport.req;
      // pass only the respective address bits
      m_ctrlport[i].req.addr <= '0;
      m_ctrlport[i].req.addr[$clog2(PORT_SIZE[i])-1:0] <= s_ctrlport.req.addr - PORT_BASE[i];

      // read and write trigger transactions and therefore need to react to reset
      if (s_ctrlport.rst) begin
        m_ctrlport[i].req.wr <= 1'b0;
        m_ctrlport[i].req.rd <= 1'b0;
      end else begin
        automatic logic address_in_range;
        address_in_range = (s_ctrlport.req.addr >= PORT_BASE[i]) &&
                           (s_ctrlport.req.addr < PORT_BASE[i] + PORT_SIZE[i]);
        m_ctrlport[i].req.wr <= s_ctrlport.req.wr & address_in_range;
        m_ctrlport[i].req.rd <= s_ctrlport.req.rd & address_in_range;
      end
    end
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

endmodule

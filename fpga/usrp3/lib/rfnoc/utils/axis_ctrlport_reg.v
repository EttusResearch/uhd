//
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module:  axis_ctrlport_reg
//
// Description:
//
//   Converts control port writes to an AXI-stream data stream. Flow control is 
//   handled by pushing back on the ctrlport interface (i.e., by not 
//   acknowledging ctrlport writes until the AXI-stream data is accepted).
//
// Parameters:
//
//   ADDR           : Writes to this address will makeup the payload of the 
//                    packet.
//
//   USE_ADDR_LAST  : Indicate if we the ADDR_LAST register generated. Set to 1 
//                    if TLAST is needed.
//
//   ADDR_LAST      : A write to this address will complete the packet (output 
//                    the last word with TLAST asserted).
//
//   DWIDTH         : Width of the AXI-stream data bus
//
//   USE_FIFO       : Indicate if you want a FIFO to be inserted before the output.
//
//   FIFm_SIZE      : The FIFO depth will be 2^FIFm_SIZE
//
//   DATA_AT_RESET  : Value of TDATA at reset.
//
//   VALID_AT_RESET : State of TVALID at reset.
//
//   LAST_AT_RESET  : State of TLAST at reset.
//

module axis_ctrlport_reg #(
  parameter ADDR           = 0,
  parameter USE_ADDR_LAST  = 0,
  parameter ADDR_LAST      = ADDR+1,
  parameter DWIDTH         = 32,
  parameter USE_FIFO       = 0,
  parameter FIFm_SIZE      = 5,
  parameter DATA_AT_RESET  = 0,
  parameter VALID_AT_RESET = 0,
  parameter LAST_AT_RESET  = 0
) (
  input clk,
  input reset,

  //---------------------------------------------------------------------------
  // Control Port
  //---------------------------------------------------------------------------

  // Control Port Slave (Request)
  input  wire        s_ctrlport_req_wr,
  input  wire [19:0] s_ctrlport_req_addr,
  input  wire [31:0] s_ctrlport_req_data,

  // Control Port Slave (Response)
  output reg         s_ctrlport_resp_ack,

  //---------------------------------------------------------------------------
  // AXI-Stream Master
  //---------------------------------------------------------------------------

  // AXI-Stream Output
  output [DWIDTH-1:0] m_tdata,
  output              m_tlast,
  output              m_tvalid,
  input               m_tready
);

  reg  [DWIDTH-1:0] m_tdata_int  = DATA_AT_RESET;
  reg               m_tlast_int  = VALID_AT_RESET;
  reg               m_tvalid_int = LAST_AT_RESET;
  wire              m_tready_int;


  //---------------------------------------------------------------------------
  // CtrlPort to AXI-Stream Logic
  //---------------------------------------------------------------------------

  always @(posedge clk) begin
    if (reset) begin
      m_tdata_int         <= DATA_AT_RESET;
      m_tvalid_int        <= VALID_AT_RESET;
      m_tlast_int         <= LAST_AT_RESET;
      s_ctrlport_resp_ack <= 1'b0;
    end else begin
      if (m_tvalid_int & m_tready_int) begin
        s_ctrlport_resp_ack <= 1'b1;
        m_tvalid_int        <= 1'b0;
        m_tlast_int         <= 1'b0;
      end else begin
        s_ctrlport_resp_ack <= 1'b0;
      end

      if (s_ctrlport_req_wr) begin
        if (s_ctrlport_req_addr == ADDR) begin
          m_tdata_int  <= s_ctrlport_req_data;
          m_tvalid_int <= 1'b1;
          m_tlast_int  <= 1'b0;
        end else if (USE_ADDR_LAST && ADDR_LAST == s_ctrlport_req_addr) begin
          m_tdata_int  <= s_ctrlport_req_data;
          m_tvalid_int <= 1'b1;
          m_tlast_int  <= 1'b1;
        end
      end
    end
  end


  //---------------------------------------------------------------------------
  // Output FIFO
  //---------------------------------------------------------------------------

  if (USE_FIFO) begin : gen_fifo
    axi_fifo #(
      .DWIDTH (DWIDTH+1),
      .SIZE   (FIFm_SIZE)
    ) axi_fifo (
      .clk      (clk),
      .reset    (reset),
      .clear    (1'b0),
      .i_tdata  ({m_tlast_int, m_tdata_int}),
      .i_tvalid (m_tvalid_int),
      .i_tready (m_tready_int),
      .o_tdata  ({m_tlast, m_tdata}),
      .o_tvalid (m_tvalid),
      .o_tready (m_tready),
      .space    (),
      .occupied ()
    );
  end else begin : nm_gen_fifo
    assign m_tdata      = m_tdata_int;
    assign m_tlast      = m_tlast_int;
    assign m_tvalid     = m_tvalid_int;
    assign m_tready_int = m_tready;
  end

endmodule

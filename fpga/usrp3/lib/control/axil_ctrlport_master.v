//
// Copyright 2019 Ettus Research, A National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: axil_ctrlport_master
// Description:
// An AXI4-Lite read/write control port adapter
//
// Converts AXI4-Lite transactions into control port requests.
// Converts all AXI requests to control port by only forwarding the
// CTRLPORT_AWIDTH LSBs of the address.
//
// Limitation:
// The control port interface will only use address, data, byte enable and
// wr/rd flags. All other signals are tied to 0.


module axil_ctrlport_master #(
  parameter TIMEOUT         = 10, // log2(timeout). Control port will timeout after 2^TIMEOUT AXI clock cycles
  parameter AXI_AWIDTH      = 17, // Width of the AXI bus. Aliasing occurs of AXI_AWIDTH > CTRLPORT_AWIDTH
  parameter CTRLPORT_AWIDTH = 17  // Number of address LSBs forwarded to m_ctrlport_req_addr
)(
  //Clock and reset
  input  wire                  s_axi_aclk,
  input  wire                  s_axi_aresetn,
  // AXI4-Lite: Write address port (domain: s_axi_aclk)
  input  wire [AXI_AWIDTH-1:0] s_axi_awaddr,
  input  wire                  s_axi_awvalid,
  output reg                   s_axi_awready,
  // AXI4-Lite: Write data port (domain: s_axi_aclk)
  input  wire [31:0]           s_axi_wdata,
  input  wire [ 3:0]           s_axi_wstrb,
  input  wire                  s_axi_wvalid,
  output reg                   s_axi_wready,
  // AXI4-Lite: Write response port (domain: s_axi_aclk)
  output reg  [ 1:0]           s_axi_bresp = 0,
  output reg                   s_axi_bvalid,
  input  wire                  s_axi_bready,
  // AXI4-Lite: Read address port (domain: s_axi_aclk)
  input  wire [AXI_AWIDTH-1:0] s_axi_araddr,
  input  wire                  s_axi_arvalid,
  output reg                   s_axi_arready,
  // AXI4-Lite: Read data port (domain: s_axi_aclk)
  output reg [31:0]            s_axi_rdata = 0,
  output reg [ 1:0]            s_axi_rresp = 0,
  output reg                   s_axi_rvalid,
  input  wire                  s_axi_rready,
  // Control port master request interface
  output reg                   m_ctrlport_req_wr,
  output reg                   m_ctrlport_req_rd,
  output reg  [19:0]           m_ctrlport_req_addr = 0,
  output wire [ 9:0]           m_ctrlport_req_portid,
  output wire [15:0]           m_ctrlport_req_rem_epid,
  output wire [ 9:0]           m_ctrlport_req_rem_portid,
  output reg  [31:0]           m_ctrlport_req_data = 0,
  output reg  [ 3:0]           m_ctrlport_req_byte_en = 0,
  output wire                  m_ctrlport_req_has_time,
  output wire [63:0]           m_ctrlport_req_time,
  // Control port master response interface
  input  wire                  m_ctrlport_resp_ack,
  input  wire [ 1:0]           m_ctrlport_resp_status,
  input  wire [31:0]           m_ctrlport_resp_data
);

  `include "../axi/axi_defs.v"
  `include "../rfnoc/core/ctrlport.vh"

  //----------------------------------------------------------
  // unused ctrlport outputs
  //----------------------------------------------------------
  assign m_ctrlport_req_portid = 10'b0;
  assign m_ctrlport_req_rem_epid = 16'b0;
  assign m_ctrlport_req_rem_portid = 10'b0;
  assign m_ctrlport_req_has_time = 1'b0;
  assign m_ctrlport_req_time = 64'b0;

  //----------------------------------------------------------
  // Address calculation
  //----------------------------------------------------------
  // define configuration for the address calculation
  localparam [CTRLPORT_ADDR_W-1:0] ADDRESS_MASK = {CTRLPORT_ADDR_W {1'b0}} | {CTRLPORT_AWIDTH {1'b1}};

  // bits to extract from AXI address
  localparam AXI_ADDR_BITS_TO_FORWARD = (AXI_AWIDTH < CTRLPORT_ADDR_W) ? AXI_AWIDTH : CTRLPORT_ADDR_W;

  //----------------------------------------------------------
  // State machine for read and write
  //----------------------------------------------------------
  localparam IDLE              = 4'd0;
  localparam READ_INIT         = 4'd1;
  localparam WRITE_INIT        = 4'd2;
  localparam READ_TRANSFER     = 4'd3;
  localparam WRITE_TRANSFER    = 4'd4;
  localparam READ_IN_PROGRESS  = 4'd5;
  localparam WRITE_IN_PROGRESS = 4'd6;
  localparam WRITE_DONE        = 4'd7;
  localparam READ_DONE         = 4'd8;

  reg [3:0] state;
  reg [TIMEOUT-1:0] timeout_counter;

  always @ (posedge s_axi_aclk) begin
    if (~s_axi_aresetn) begin
      state <= IDLE;

      // clear AXI feedback paths and controlport requests
      s_axi_awready <= 1'b0;
      s_axi_wready  <= 1'b0;
      s_axi_bvalid  <= 1'b0;
      s_axi_arready <= 1'b0;
      s_axi_rvalid  <= 1'b0;
      m_ctrlport_req_rd <= 1'b0;
      m_ctrlport_req_wr <= 1'b0;
    end else begin
      case (state)
        // decide whether a read or write should be handled
        IDLE: begin
          timeout_counter <= {TIMEOUT {1'b1}};

          if (s_axi_arvalid) begin
            state <= READ_INIT;
          end
          else if (s_axi_awvalid) begin
            state <= WRITE_INIT;
          end
        end

        // wait for FIFO to get read to assign valid
        READ_INIT: begin
          // signal ready to upstream module
          s_axi_arready <= 1'b1;

          state <= READ_TRANSFER;
        end

        // transfer data to FIFO
        READ_TRANSFER: begin
          // clear ready flag from READ_INIT state
          s_axi_arready <= 1'b0;
          // transfer data to controlport
          m_ctrlport_req_rd <= 1'b1;
          m_ctrlport_req_addr <= s_axi_araddr[AXI_ADDR_BITS_TO_FORWARD-1:0] & ADDRESS_MASK;
          m_ctrlport_req_byte_en <= 4'b1111;

          state <= READ_IN_PROGRESS;
        end

        // wait for controlport response is available
        READ_IN_PROGRESS: begin
          // clear read flag from previous state
          m_ctrlport_req_rd <= 1'b0;

          //decrement timeout
          timeout_counter <= timeout_counter - 1;

          if (m_ctrlport_resp_ack == 1'b1 || timeout_counter == 0) begin
            s_axi_rvalid <= 1'b1;
            s_axi_rdata <= m_ctrlport_resp_data;
            s_axi_rresp <= `AXI4_RESP_OKAY;

            // use AXI DECERR to inform about failed transaction
            if (timeout_counter == 0) begin
              s_axi_rresp <= `AXI4_RESP_DECERR;
            end else begin
              // if controlport response is not OKAY use AXI SLVERR to propagate error
              if (m_ctrlport_resp_status != CTRL_STS_OKAY) begin
                s_axi_rresp <= `AXI4_RESP_SLVERR;
              end
            end

            state <= READ_DONE;
          end
        end

        // wait until read response is transferred
        READ_DONE: begin
          if (s_axi_rready) begin
            s_axi_rvalid <= 1'b0;
            state <= IDLE;
          end
        end

        //wait for FIFO and data to process
        WRITE_INIT: begin
          if (s_axi_wvalid) begin
            s_axi_awready <= 1'b1;
            s_axi_wready <= 1'b1;
            state <= WRITE_TRANSFER;
          end
        end

        // transfer data to FIFO
        WRITE_TRANSFER: begin
          // clear ready flags from READ_INIT state
          s_axi_awready <= 1'b0;
          s_axi_wready <= 1'b0;
          // transfer data to controlport
          m_ctrlport_req_wr <= 1'b1;
          m_ctrlport_req_addr <= s_axi_awaddr[AXI_ADDR_BITS_TO_FORWARD-1:0] & ADDRESS_MASK;
          m_ctrlport_req_data <= s_axi_wdata;
          m_ctrlport_req_byte_en <= s_axi_wstrb;

          state <= WRITE_IN_PROGRESS;
        end

        // wait for write to complete
        WRITE_IN_PROGRESS: begin
          // clear write flag from previous state
          m_ctrlport_req_wr <= 1'b0;

          //decrement timeout
          timeout_counter <= timeout_counter - 1;

          if (m_ctrlport_resp_ack == 1'b1 || timeout_counter == 0) begin
            s_axi_bvalid <= 1'b1;
            s_axi_rdata <= 32'b0;
            s_axi_bresp <= `AXI4_RESP_OKAY;

            // use AXI DECERR to inform about failed transaction
            if (timeout_counter == 0) begin
              s_axi_bresp <= `AXI4_RESP_DECERR;
            end else begin
              // if controlport response is not OKAY use AXI SLVERR to propagate error
              if (m_ctrlport_resp_status != CTRL_STS_OKAY) begin
                s_axi_bresp <= `AXI4_RESP_SLVERR;
              end
            end

            state <= WRITE_DONE;
          end
        end

        WRITE_DONE: begin
          if (s_axi_bready) begin
            state <= IDLE;
            s_axi_bvalid <= 1'b0;
          end
        end

        default: begin
          state <= IDLE;
        end
      endcase
    end
  end

endmodule

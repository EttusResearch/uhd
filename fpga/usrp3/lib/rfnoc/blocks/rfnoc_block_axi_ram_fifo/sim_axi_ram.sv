//
// Copyright 2019 Ettus Research, A National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: sim_axi_ram
//
// Description: 
//
//   Simulation model for a basic AXI4 memory mapped memory. A few notes on its 
//   behavior:
//
//     - This model does not reorder requests (regardless of WID/RID). All 
//       requests are evaluated strictly in order.
//     - The only supported response is OKAY
//     - This model supports misaligned memory accesses, which cause a 
//       simulation warning.
//     - A reset does not clear the memory contents
//     - The memory itself is implemented using an associative array (sparse 
//       matrix) so that large memories can be supported.
//     - This model is half duplex, meaning read and write data transfers won't
//       happen at the same time. A new data transfer won't begin until the
//       previous one has completed.
//

module sim_axi_ram #(
  parameter AWIDTH     = 32,
  parameter DWIDTH     = 64,
  parameter IDWIDTH    = 2,
  parameter BIG_ENDIAN = 0,
  parameter STALL_PROB = 25
) (
  input logic s_aclk,
  input logic s_aresetn,

  // Write Address Channel
  input  logic [IDWIDTH-1:0] s_axi_awid,
  input  logic [ AWIDTH-1:0] s_axi_awaddr,
  input  logic [        7:0] s_axi_awlen,
  input  logic [        2:0] s_axi_awsize,
  input  logic [        1:0] s_axi_awburst,
  input  logic               s_axi_awvalid,
  output logic               s_axi_awready,

  // Write Data Channel
  input  logic [  DWIDTH-1:0] s_axi_wdata,
  input  logic [DWIDTH/8-1:0] s_axi_wstrb,
  input  logic                s_axi_wlast,
  input  logic                s_axi_wvalid,
  output logic                s_axi_wready,

  // Write Response Channel
  output logic [IDWIDTH-1:0] s_axi_bid,
  output logic [        1:0] s_axi_bresp,
  output logic               s_axi_bvalid,
  input  logic               s_axi_bready,

  // Read Address Channel
  input  logic [IDWIDTH-1:0] s_axi_arid,
  input  logic [ AWIDTH-1:0] s_axi_araddr,
  input  logic [        7:0] s_axi_arlen,
  input  logic [        2:0] s_axi_arsize,
  input  logic [        1:0] s_axi_arburst,
  input  logic               s_axi_arvalid,
  output logic               s_axi_arready,

  // Read Data Channel
  output logic [       0:0] s_axi_rid,
  output logic [DWIDTH-1:0] s_axi_rdata,
  output logic [       1:0] s_axi_rresp,
  output logic              s_axi_rlast,
  output logic              s_axi_rvalid,
  input  logic              s_axi_rready
);

  localparam DEBUG = 0;

  //---------------------------------------------------------------------------
  // Data Types
  //---------------------------------------------------------------------------
  
  typedef enum logic [1:0] { FIXED, INCR, WRAP }            burst_t;
  typedef enum logic [1:0] { OKAY, EXOKAY, SLVERR, DECERR } resp_t;

  typedef struct packed {
    longint             count;  // Number of requests to wait for before executing
    logic [IDWIDTH-1:0] id;
    logic [AWIDTH-1:0]  addr;
    logic [8:0]         len;    // Add an extra bit, since actual true length is +1
    logic [7:0]         size;   // Add extra bits to store size in bytes, instead of clog2(size)
    burst_t             burst;
  } req_t;

  // Make the address type an extra bit wide so that we can detect 
  // out-of-bounds accesses easily.
  typedef bit [AWIDTH:0] addr_t;

  // Data word type
  typedef logic [DWIDTH-1:0] data_t;

  // Mask to indicate which bits should be written.
  typedef bit [DWIDTH/8-1:0] mask_t;


  //---------------------------------------------------------------------------
  // Data Structures
  //---------------------------------------------------------------------------

  byte             memory [addr_t];     // Byte addressable memory
  mailbox #(req_t) read_req   = new();  // Read request queue
  mailbox #(req_t) write_req  = new();  // Write request queue
  mailbox #(req_t) write_resp = new();  // Write response queue

  longint req_count;    // Number of requests received
  longint compl_count;  // Number of requests completed


  //---------------------------------------------------------------------------
  // External Configuration Interface
  //---------------------------------------------------------------------------

  int waddr_stall_prob = STALL_PROB;
  int wdata_stall_prob = STALL_PROB;
  int wresp_stall_prob = STALL_PROB;
  int raddr_stall_prob = STALL_PROB;
  int rdata_stall_prob = STALL_PROB;

  // Set ALL stall probabilities to the same value
  function void set_stall_prob(int probability);
    assert(probability >= 0 && probability <= 100) else begin
      $error("Probability must be from 0 to 100");
    end
    waddr_stall_prob = probability;
    wdata_stall_prob = probability;
    wresp_stall_prob = probability;
    raddr_stall_prob = probability;
    rdata_stall_prob = probability;
  endfunction : set_stall_prob

  // Set WRITE stall probabilities to the same value
  function void set_write_stall_prob(int probability);
    assert(probability >= 0 && probability <= 100) else begin
      $error("Probability must be from 0 to 100");
    end
    waddr_stall_prob = probability;
    wdata_stall_prob = probability;
    wresp_stall_prob = probability;
  endfunction : set_write_stall_prob

  // Set READ stall probabilities to the same value
  function void set_read_stall_prob(int probability);
    assert(probability >= 0 && probability <= 100) else begin
      $error("Probability must be from 0 to 100");
    end
    raddr_stall_prob = probability;
    rdata_stall_prob = probability;
  endfunction : set_read_stall_prob

  // Set Write Address Channel stall probability  
  function void set_waddr_stall_prob(int probability);
    assert(probability >= 0 && probability <= 100) else begin
      $error("Probability must be from 0 to 100");
    end
    waddr_stall_prob = probability;
  endfunction : set_waddr_stall_prob

  // Set Write Data Channel stall probability
  function void set_wdata_stall_prob(int probability);
    assert(probability >= 0 && probability <= 100) else begin
      $error("Probability must be from 0 to 100");
    end
    wdata_stall_prob = probability;
  endfunction : set_wdata_stall_prob

  // Set Write Response Channel stall probability
  function void set_wresp_stall_prob(int probability);
    assert(probability >= 0 && probability <= 100) else begin
      $error("Probability must be from 0 to 100");
    end
    wresp_stall_prob = probability;
  endfunction : set_wresp_stall_prob

  // Set Read Address Channel stall probability
  function void set_raddr_stall_prob(int probability);
    assert(probability >= 0 && probability <= 100) else begin
      $error("Probability must be from 0 to 100");
    end
    raddr_stall_prob = probability;
  endfunction : set_raddr_stall_prob

  // Set Read Data Channel stall probability
  function void set_rdata_stall_prob(int probability);
    assert(probability >= 0 && probability <= 100) else begin
      $error("Probability must be from 0 to 100");
    end
    rdata_stall_prob = probability;
  endfunction : set_rdata_stall_prob

  // Get Write Address Channel stall probability  
  function int get_waddr_stall_prob();
    return waddr_stall_prob;
  endfunction : get_waddr_stall_prob

  // Get Write Data Channel stall probability
  function int get_wdata_stall_prob();
    return wdata_stall_prob;
  endfunction : get_wdata_stall_prob

  // Get Write Response Channel stall probability
  function int get_wresp_stall_prob();
    return wresp_stall_prob;
  endfunction : get_wresp_stall_prob

  // Get Read Address Channel stall probability
  function int get_raddr_stall_prob();
    return raddr_stall_prob;
  endfunction : get_raddr_stall_prob

  // Get Read Data Channel stall probability
  function int get_rdata_stall_prob();
    return rdata_stall_prob;
  endfunction : get_rdata_stall_prob



  //---------------------------------------------------------------------------
  // Helper Functions
  //---------------------------------------------------------------------------

  function data_t read_mem(addr_t byte_addr, int num_bytes);
    data_t data;
    addr_t incr;

    if (BIG_ENDIAN) begin
      byte_addr = byte_addr + num_bytes-1;
      incr      = -1;
    end else begin
      incr      = 1;
    end

    for (int i = 0; i < num_bytes; i++) begin
      if (byte_addr >= 2**AWIDTH) begin
        $fatal(1, "Read extends beyond memory range");
      end
      if (memory.exists(byte_addr)) data[i*8 +: 8] = memory[byte_addr];
      else data[i*8 +: 8] = 'X;
      byte_addr += incr;
    end

    return data;
  endfunction : read_mem


  function void write_mem(addr_t byte_addr, int num_bytes, data_t data, mask_t mask);
    addr_t incr;

    if (BIG_ENDIAN) begin
      byte_addr = byte_addr + num_bytes-1;
      incr      = -1;
    end else begin
      incr      = 1;
    end

    for (int i = 0; i < num_bytes; i++) begin
      if (mask[i]) begin
        if (byte_addr >= 2**AWIDTH) begin
          $fatal(1, "Write extends beyond memory range");
        end
        memory[byte_addr] = data[i*8 +: 8];
      end
      byte_addr += incr;
    end
  endfunction : write_mem


  //---------------------------------------------------------------------------
  // Write Requests
  //---------------------------------------------------------------------------

  initial begin : write_req_proc
    req_t   req;
    burst_t burst;

    s_axi_awready <= 0;

    forever begin
      @(posedge s_aclk);
      if (!s_aresetn) continue;

      if (s_axi_awvalid) begin
        if (s_axi_awready) begin
          req.count = req_count;
          req.id    = s_axi_awid;
          req.addr  = s_axi_awaddr;
          req.len   = s_axi_awlen + 1;    // Per AXI4 spec, Burst_length = AxLEN[7:0] + 1
          req.size  = 2**s_axi_awsize;    // Store as true size in bytes, not clog2(size)
          req.burst = burst_t'(s_axi_awburst);

          // Check that the request is valid
          assert (!$isunknown(req)) else begin
            $fatal(1, "Write request signals are unknown");
          end
          assert (s_axi_araddr % (DWIDTH/8) == 0) else begin
            $warning("Unaligned memory write");
          end
          assert (2**s_axi_awsize <= DWIDTH/8) else begin
            $fatal(1, "AWSIZE must not be larger than DWIDTH");
          end
          assert ($cast(burst, s_axi_awburst)) else begin
            $fatal(1, "Invalid AWBURST value");
          end

          if (DEBUG) begin
            $display("WRITE REQ: id=%X, addr=%X, len=%X, size=%X, burst=%s, %t, %m", 
              req.id, req.addr, req.len, req.size, req.burst.name, $realtime);
          end

          req_count++;
          write_req.put(req);
        end

        // Randomly deassert ready
        s_axi_awready <= $urandom_range(99) < waddr_stall_prob ? 0 : 1;
      end
    end
  end : write_req_proc


  //---------------------------------------------------------------------------
  // Read Requests
  //---------------------------------------------------------------------------

  initial begin : read_req_proc
    req_t   req;
    burst_t burst;

    s_axi_arready <= 0;

    forever begin
      @(posedge s_aclk);
      if (!s_aresetn) continue;

      if (s_axi_arvalid) begin
        if (s_axi_arready) begin
          req.count = req_count;
          req.id    = s_axi_arid;
          req.addr  = s_axi_araddr;
          req.len   = s_axi_arlen + 1;    // Per AXI4 spec, Burst_length = AxLEN[7:0] + 1
          req.size  = 2**s_axi_arsize;    // Store as true size in bytes, not clog2(size)
          req.burst = burst_t'(s_axi_arburst);

          // Check that the request is valid
          assert(!$isunknown(req)) else begin
            $fatal(1, "Read request signals are unknown");
          end
          assert(s_axi_araddr % (DWIDTH/8) == 0) else begin
            $warning("Unaligned memory read");
          end
          assert(2**s_axi_arsize <= DWIDTH/8) else begin
            $fatal(1, "ARSIZE must not be larger than DWIDTH");
          end
          assert ($cast(burst, s_axi_awburst)) else begin
            $fatal(1, "Invalid ARBURST value");
          end

          if (DEBUG) begin
            $display("READ REQ:  id=%X, addr=%X, len=%X, size=%X, burst=%s, %t, %m", 
              req.id, req.addr, req.len, req.size, req.burst.name, $realtime);
          end

          req_count++;
          read_req.put(req);
        end

        // Randomly deassert ready to cause a stall
        s_axi_arready <= $urandom_range(99) < raddr_stall_prob ? 0 : 1;
      end
    end
  end : read_req_proc


  //---------------------------------------------------------------------------
  // Write Data
  //---------------------------------------------------------------------------

  initial begin : write_data_proc
    req_t req;
    bit [AWIDTH-1:0] addr;

    forever begin
      // Wait for the next write request
      s_axi_wready <= 0;
      write_req.get(req);

      // Wait for previous requests to complete
      while (compl_count < req.count) begin
        @(posedge s_aclk);
        if (!s_aresetn) break;
      end

      // If reset was asserted, clear the request queue and start over
      if (!s_aresetn) begin
        while(write_req.try_get(req));
        continue;
      end

      // Iterate over the number of words in the request
      for (int i = 0; i < req.len; ) begin
        @(posedge s_aclk);
        if (!s_aresetn) break;

        // Check if we have a new data word
        if (s_axi_wvalid) begin
          if (s_axi_wready) begin
            // Check the inputs
            if ($isunknown(s_axi_wstrb)) begin
              $fatal(1, "WSTRB is unknown");
            end
            if ($isunknown(s_axi_wdata)) begin
              $warning(1, "WDATA is unknown; data will be changed to zero");
            end

            case (req.burst)
              FIXED : begin
                addr = req.addr;
              end
              INCR : begin
                // If the address rolls over, we've reached the end of the 
                // memory and we should stop here.
                addr = req.addr + i*req.size;
                if (addr < req.addr) break;
              end
              WRAP : begin
                // Allow roll-over
                addr = req.addr + i*req.size;
              end
            endcase

            write_mem(addr, req.size, s_axi_wdata, s_axi_wstrb);

            if (DEBUG) begin
              $display("WRITE: count=%3X, ADDR=%X, DATA=%X, SIZE=%X, STRB=%X, %t, %m", 
                i, addr, s_axi_wdata, req.size, s_axi_wstrb, $realtime);
            end

            i++;
          end

          // Randomly deassert ready to cause a stall
          s_axi_wready <= $urandom_range(99) < wdata_stall_prob ? 0 : 1;
        end
      end // for

      // If reset was asserted, clear the request queue and start over
      if (!s_aresetn) begin
        while(write_req.try_get(req));
        continue;
      end

      compl_count++;

      // Enqueue write response
      write_resp.put(req);

      // Make sure WLAST asserted for the last word. If not we report an error. 
      // Per the AXI4 standard, "a slave is not required to use the WLAST 
      // signal" because "a slave can calculate the last write data transfer 
      // from the burst length AWLEN".
      if (s_axi_wlast != 1'b1) begin
        $error("WLAST not asserted on last word of burst");
      end

    end // forever
  end : write_data_proc


  //---------------------------------------------------------------------------
  // Write Response
  //---------------------------------------------------------------------------

  initial begin : write_resp_proc
    req_t resp;
    bit [AWIDTH-1:0] addr;

    forever begin
      s_axi_bid    <= 'X;
      s_axi_bresp  <= 'X;
      s_axi_bvalid <= 0;

      // Wait for the next write response
      write_resp.get(resp);
      @(posedge s_aclk);

      // If there's a reset, clear the response queue and start over
      if (!s_aresetn) begin
        while(write_resp.try_get(resp));
        continue;
      end

      // Randomly keep bvalid deasserted for next word to cause a stall
      if ($urandom_range(99) < wresp_stall_prob) begin
        do begin
          @(posedge s_aclk);
          if (!s_aresetn) break;
        end while ($urandom_range(99) < wresp_stall_prob);

        // If reset was asserted, clear the response queue and start over
        if (!s_aresetn) begin
          while(write_resp.try_get(resp));
          continue;
        end
      end

      // Output the next response
      s_axi_bid    <= resp.id;
      s_axi_bresp  <= OKAY;
      s_axi_bvalid <= 1;

      if (DEBUG) begin
        $display("WRITE RESP: ID=%X, %t, %m", resp.id, $realtime);
      end

      // Wait for the response to be accepted
      do begin
        @(posedge s_aclk);
        if (!s_aresetn) break;
      end while (!s_axi_bready);

      // Output the next response
      s_axi_bid    <= 'X;
      s_axi_bresp  <= 'X;
      s_axi_bvalid <= 0;

      // If reset was asserted, clear the response queue and start over
      if (!s_aresetn) begin
        while(write_resp.try_get(resp));
        continue;
      end
    end // forever
  end : write_resp_proc


  //---------------------------------------------------------------------------
  // Read Data
  //---------------------------------------------------------------------------

  initial begin : read_data_proc
    req_t req;
    bit   [AWIDTH-1:0] addr;
    logic [DWIDTH-1:0] data;

    forever begin
      s_axi_rid    <= 'X;
      s_axi_rdata  <= 'X;
      s_axi_rresp  <= 'X;
      s_axi_rlast  <= 'X;
      s_axi_rvalid <= 0;

      // Wait for the next read request
      read_req.get(req);

      // Wait for previous requests to complete
      do begin
        @(posedge s_aclk);
        if (!s_aresetn) break;
      end while (compl_count < req.count);

      // If reset was asserted, clear the request queue and start over
      if (!s_aresetn) begin
        while(read_req.try_get(req));
        continue;
      end

      for (int i = 0; i < req.len; i++) begin
        // Randomly keep rvalid deasserted for next word to cause a stall
        if ($urandom_range(99) < rdata_stall_prob) begin
          do begin
            @(posedge s_aclk);
            if (!s_aresetn) break;
          end while ($urandom_range(99) < rdata_stall_prob);
          if (!s_aresetn) break;
        end

        case (req.burst)
          FIXED : begin
            addr = req.addr;
          end
          INCR : begin
            // If the address rolls over, we've reached the end of the memory 
            // and we should stop here.
            addr = req.addr + i*req.size;
            if (addr < req.addr) break;
          end
          WRAP : begin
            // Allow roll-over
            addr = req.addr + i*req.size;
          end
        endcase

        // Read the memory
        data = read_mem(addr, req.size);

        // Output the next word
        s_axi_rid    <= req.id;
        s_axi_rdata  <= data;
        s_axi_rresp  <= OKAY;
        s_axi_rlast  <= (i == req.len-1);
        s_axi_rvalid <= 1;

        if (DEBUG) begin
          $display("READ:  count=%3X, ADDR=%X, DATA=%X, SIZE=%X, %t, %m", i, addr, data, req.size, $realtime);
        end

        // Wait for the word to be captured
        do begin
          @(posedge s_aclk);
          if (!s_aresetn) break;
        end while (!s_axi_rready);

        s_axi_rid    <= 'X;
        s_axi_rdata  <= 'X;
        s_axi_rresp  <= 'X;
        s_axi_rlast  <= 'X;
        s_axi_rvalid <= 0;
      end // for

      // If reset was asserted, clear the request queue and start over
      if (!s_aresetn) begin
        while(read_req.try_get(req));
      end

      compl_count++;

    end // forever
  end : read_data_proc

endmodule

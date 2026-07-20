//
// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: axis_ctrl_slave
//
// Description:
//
//   Implements an AXIS-Control slave (and a Control-Port master). Requests are
//   accepted on the slave AXIS-Ctrl port and responses are emitted on the
//   master AXIS-Ctrl port.
//
//   The following opcodes are supported:
//
//     SLEEP       : Hold the bus idle for a specified number of cycles.
//     WRITE       : Write num_data words to the same address.
//     READ        : Read one req_size words from the same address.
//     READ_WRITE  : Simultaneously issues a CtrlPort read and write to
//                   address. The read data (pre-write value) is returned in
//                   the response.
//     BLOCK_WRITE : Write num_data words to consecutive word addresses (addr,
//                   addr+4, addr+8, ...).
//     BLOCK_READ  : Read req_size words from consecutive addresses (addr,
//                   addr+4, addr+8, ...).
//     POLL        : Poll on address until its value for all bits in mask
//                   matches data & mask, or until timeout.
//
//   All other opcodes are treated as a NOP and the response will carry a
//   CMDERR status.
//
//   The response packet is not emitted until all CtrlPort acknowledgements
//   have been collected so that the response packet can be transmitted
//   contiguously with no gaps. Block-read data words are buffered internally
//   in a FIFO while acks are being gathered.
//
//   Response packets match the request. All fields are are set to match the
//   request, except that IsAck is set and the status is updated. The Status
//   field in the response op-word reflects the worst-case CtrlPort status
//   across all sub-transactions, where CMDERR has the highest precedence,
//   followed by TSERR, WARNING, and OK.
//
// Signals:
//
//   - s_axis_ctrl_* : Input AXIS-Ctrl stream (requests)
//   - m_axis_ctrl_* : Output AXIS-Ctrl stream (responses)
//   - ctrlport_req_*  : Control-port master request port
//   - ctrlport_resp_* : Control-port master response port
//

`default_nettype none


module axis_ctrl_slave (
  input  wire         clk,
  input  wire         rst,

  // AXIS-Control Bus (Request)
  input  wire [31:0]  s_axis_ctrl_tdata,
  input  wire         s_axis_ctrl_tlast,
  input  wire         s_axis_ctrl_tvalid,
  output logic        s_axis_ctrl_tready,

  // AXIS-Control Bus (Response)
  output logic [31:0] m_axis_ctrl_tdata,
  output logic        m_axis_ctrl_tlast,
  output logic        m_axis_ctrl_tvalid,
  input  wire         m_axis_ctrl_tready,

  // Control Port Endpoint (Request)
  output logic        ctrlport_req_wr,
  output logic        ctrlport_req_rd,
  output logic [19:0] ctrlport_req_addr,
  output logic [31:0] ctrlport_req_data,
  output logic [ 3:0] ctrlport_req_byte_en,
  output logic        ctrlport_req_has_time,
  output logic [63:0] ctrlport_req_time,

  // Control Port Endpoint (Response)
  input  wire        ctrlport_resp_ack,
  input  wire [ 1:0] ctrlport_resp_status,
  input  wire [31:0] ctrlport_resp_data
);

  import rfnoc_chdr_utils_pkg::*;
  `include "rfnoc_axis_ctrl_utils.vh"


  //---------------------------------------------------------------------------
  // Local Functions
  //---------------------------------------------------------------------------

  // Returns a numerical severity for priority-ordered status accumulation.
  function automatic int status_priority(ctrl_status_t status);
    case (status)
      CTRL_STS_CMDERR:  return 3;
      CTRL_STS_TSERR:   return 2;
      CTRL_STS_WARNING: return 1;
      default:          return 0;  // CTRL_STS_OKAY
    endcase
  endfunction


  // Returns the higher-priority status between a new CtrlPort response status
  // and the accumulated status.
  function automatic ctrl_status_t resolve_status(
    logic [1:0]   new_status,
    ctrl_status_t acc_status
  );
    if (status_priority(ctrl_status_t'(new_status)) >
        status_priority(acc_status)) begin
      return ctrl_status_t'(new_status);
    end else begin
      return acc_status;
    end
  endfunction : resolve_status


  //---------------------------------------------------------------------------
  // Signal Declarations
  //---------------------------------------------------------------------------

  typedef enum logic [4:0] {
    ST_IN_HDR_0          = 5'd0,
    ST_IN_HDR_1          = 5'd1,
    ST_IN_TS_LO          = 5'd2,
    ST_IN_TS_HI          = 5'd3,
    ST_IN_OP_WORD        = 5'd4,
    ST_WR_IN_DATA        = 5'd5,
    ST_WR_WAIT_ACK       = 5'd6,
    ST_RW_IN_DATA        = 5'd7,
    ST_RW_WAIT_ACK       = 5'd8,
    ST_RD_REQ            = 5'd9,
    ST_RD_WAIT_ACK       = 5'd10,
    ST_SLEEP             = 5'd11,
    ST_SLEEP_LOAD        = 5'd19,
    ST_OUT_HDR_0         = 5'd12,
    ST_OUT_HDR_1         = 5'd13,
    ST_OUT_TS_LO         = 5'd14,
    ST_OUT_TS_HI         = 5'd15,
    ST_OUT_OP_WORD       = 5'd16,
    ST_OUT_RD_DATA       = 5'd17,
    ST_DROP              = 5'd18,
    ST_DRAIN             = 5'd20,
    ST_POLL_LOAD_DATA    = 5'd21,
    ST_POLL_LOAD_MASK    = 5'd22,
    ST_POLL_LOAD_TIMEOUT = 5'd23,
    ST_POLL_REQ          = 5'd24,
    ST_POLL_WAIT_ACK     = 5'd25,
    ST_POLL_CHECK        = 5'd26
  } state_t;

  state_t state = ST_IN_HDR_0;

  // Cached copies of request header, timestamp, and op-word
  axis_ctrl_header_t cached_hdr = '0;
  chdr_timestamp_t   cached_ts  = '0;
  ctrl_op_word_t     cached_op  = '0;

  // Block/multi-word operation tracking
  ctrl_num_data_t blk_words_rem = '0;   // Ctrlport transactions remaining
  ctrl_address_t  blk_addr      = '0;   // Current CtrlPort address
  logic           blk_inc_addr  = '0;   // Increment address after each CtrlPort transaction

  // Response accumulation
  ctrl_status_t   resp_status   = CTRL_STS_OKAY;  // Accumulated CtrlPort status
  ctrl_num_data_t num_data      = '0;             // Actual data words in response

  logic fifo_has_rd_data = '0;

  // What state to go to after draining optional data words
  state_t drain_to = ST_IN_HDR_0;

  // Sleep counter
  logic [31:0] sleep_cntr = '0;

  // Poll operands
  logic [31:0] poll_data         = '0;  // Target value (data[0])
  logic [31:0] poll_mask         = '0;  // Mask         (data[1])
  logic [31:0] poll_timeout      = '0;  // Countdown timer (data[2])
  logic [31:0] poll_data_masked  = '0;  // poll_data & poll_mask

  // Poll response capture (registered on ctrlport_resp_ack in ST_POLL_WAIT_ACK)
  logic [31:0] poll_resp_data  = '0;  // Capture ctrlport_resp_data
  logic        poll_cond_met   = '0;  // Capture if polling condition met
  logic        poll_timed_out  = '0;  // Capture if timeout elapsed

  // Read-data FIFO signals
  logic        rd_fifo_i_tvalid;
  logic [31:0] rd_fifo_i_tdata;
  logic [31:0] rd_fifo_o_tdata;
  logic        rd_fifo_o_tvalid;
  logic        rd_fifo_o_tready;
  logic [15:0] rd_fifo_occupied;


  //---------------------------------------------------------------------------
  // Field extraction
  //---------------------------------------------------------------------------

  // Op-word: ctrl_op_word_t is exactly 32 bits, direct cast is correct.
  ctrl_op_word_t op_word;
  assign op_word = ctrl_op_word_t'(s_axis_ctrl_tdata);

  // Shorthand for input/output AXI-Stream transfer
  wire in_xfer  = s_axis_ctrl_tvalid && s_axis_ctrl_tready;
  wire out_xfer = m_axis_ctrl_tvalid && m_axis_ctrl_tready;


  //---------------------------------------------------------------------------
  // State Machine Registered Process
  //---------------------------------------------------------------------------

  always_ff @(posedge clk) begin
    if (rst) begin
      state        <= ST_IN_HDR_0;
      cached_hdr   <= 'X;
      cached_ts    <= 'X;
      cached_op    <= 'X;
      num_data     <= 'X;
      drain_to     <= state_t'('X);
      blk_addr     <= 'X;
      blk_inc_addr <= 'X;
      resp_status  <= ctrl_status_t'('X);
    end else begin
      unique case (state)

        ST_IN_HDR_0: begin
          if (in_xfer) begin
            cached_hdr <= s_axis_ctrl_tdata;
            state      <= s_axis_ctrl_tlast ? ST_IN_HDR_0 : ST_IN_HDR_1;
          end
        end

        ST_IN_HDR_1: begin
          // Accept HDR word 1. Latch remote routing fields.
          if (in_xfer) begin
            cached_hdr[63:32] <= s_axis_ctrl_tdata;
            if (s_axis_ctrl_tlast)
              state <= ST_IN_HDR_0;  // Premature end
            else if (cached_hdr.has_time)
              state <= ST_IN_TS_LO;
            else
              state <= ST_IN_OP_WORD;
          end
        end

        ST_IN_TS_LO: begin
          if (in_xfer) begin
            cached_ts[31:0] <= s_axis_ctrl_tdata;
            if (s_axis_ctrl_tlast)
              state <= ST_IN_HDR_0;
            else
              state <= ST_IN_TS_HI;
          end
        end

        ST_IN_TS_HI: begin
          if (in_xfer) begin
            cached_ts[63:32] <= s_axis_ctrl_tdata;
            if (s_axis_ctrl_tlast)
              state <= ST_IN_HDR_0;
            else
              state <= ST_IN_OP_WORD;
          end
        end

        ST_IN_OP_WORD: begin
          // Latch op-word, decode opcode, set up counters, and dispatch.
          if (s_axis_ctrl_tvalid) begin
            cached_op     <= ctrl_op_word_t'(s_axis_ctrl_tdata);
            blk_addr      <= op_word.address;
            blk_words_rem <= cached_hdr.num_data;
            blk_inc_addr  <= (op_word.op_code == CTRL_OP_BLOCK_READ);

            unique case (op_word.op_code)

              // READ and BLOCK_READ carry no data words, so tlast on op-word
              // is normal.
              CTRL_OP_READ, CTRL_OP_BLOCK_READ: begin
                num_data <= cached_hdr.req_size;
                blk_words_rem <= cached_hdr.req_size;
                if (cached_hdr.req_size == 0) begin
                  // req_size == 0 is invalid for a read. Respond with CMDERR.
                  resp_status <= CTRL_STS_CMDERR;
                  if (!s_axis_ctrl_tlast) begin
                    drain_to <= ST_OUT_HDR_0;
                    state    <= ST_DRAIN;
                  end else begin
                    state <= ST_OUT_HDR_0;
                  end
                end else begin
                  resp_status <= CTRL_STS_OKAY;
                  if (!s_axis_ctrl_tlast) begin
                    // Extra data words are present in request. Drain them then
                    // do the read.
                    drain_to <= ST_RD_REQ;
                    state    <= ST_DRAIN;
                  end else begin
                    state <= ST_RD_REQ;
                  end
                end
              end

              // The remaining opcodes all require data words after the
              // op-word. If tlast is already set on the op-word (either
              // because num_data == 0 or because the packet was truncated)
              // respond with CMDERR.

              CTRL_OP_SLEEP: begin
                num_data <= 4'd0;
                if (s_axis_ctrl_tlast || cached_hdr.num_data != 1) begin
                  // No data words (num_data == 0 or truncated). Respond with
                  // CMDERR.
                  resp_status <= CTRL_STS_CMDERR;
                  state       <= s_axis_ctrl_tlast ? ST_OUT_HDR_0 : ST_DRAIN;
                  drain_to    <= ST_OUT_HDR_0;
                end else begin
                  resp_status <= CTRL_STS_OKAY;
                  state       <= ST_SLEEP_LOAD;
                end
              end

              CTRL_OP_POLL: begin
                if (s_axis_ctrl_tlast || cached_hdr.num_data != 3) begin
                  // Truncated or wrong num_data. Respond with CMDERR.
                  num_data    <= 4'd0;
                  resp_status <= CTRL_STS_CMDERR;
                  state       <= s_axis_ctrl_tlast ? ST_OUT_HDR_0 : ST_DRAIN;
                  drain_to    <= ST_OUT_HDR_0;
                end else begin
                  num_data    <= 4'd1;
                  resp_status <= CTRL_STS_OKAY;
                  state       <= ST_POLL_LOAD_DATA;
                end
              end

              CTRL_OP_WRITE, CTRL_OP_BLOCK_WRITE: begin
                num_data <= 4'd0;
                if (s_axis_ctrl_tlast || cached_hdr.num_data == 0) begin
                  // No data words (num_data == 0 or truncated). Respond with
                  // CMDERR.
                  resp_status <= CTRL_STS_CMDERR;
                  state       <= s_axis_ctrl_tlast ? ST_OUT_HDR_0 : ST_DRAIN;
                  drain_to    <= ST_OUT_HDR_0;
                end else begin
                  blk_addr      <= op_word.address;
                  blk_words_rem <= cached_hdr.num_data;
                  blk_inc_addr  <= (op_word.op_code == CTRL_OP_BLOCK_WRITE);
                  resp_status   <= CTRL_STS_OKAY;
                  state         <= ST_WR_IN_DATA;
                end
              end

              CTRL_OP_READ_WRITE: begin
                if (s_axis_ctrl_tlast || cached_hdr.num_data != 1) begin
                  // Invalid number of words or truncated. Respond with CMDERR.
                  num_data    <= 4'd0;
                  resp_status <= CTRL_STS_CMDERR;
                  state       <= s_axis_ctrl_tlast ? ST_OUT_HDR_0 : ST_DRAIN;
                  drain_to    <= ST_OUT_HDR_0;
                end else begin
                  num_data    <= 4'd1;
                  blk_addr    <= op_word.address;
                  resp_status <= CTRL_STS_OKAY;
                  state       <= ST_RW_IN_DATA;
                end
              end

              default: begin
                // Unknown opcode. Drain data words then respond with CMDERR.
                num_data    <= 4'd0;
                resp_status <= CTRL_STS_CMDERR;
                state       <= s_axis_ctrl_tlast ? ST_OUT_HDR_0 : ST_DRAIN;
                drain_to    <= ST_OUT_HDR_0;
              end

            endcase
          end
        end

        //---------------------------------------------------------------------
        // Write path (WRITE / BLOCK_WRITE)
        //---------------------------------------------------------------------

        ST_WR_IN_DATA: begin
          // Combinational process drives ctrlport_req_wr this cycle. Advance
          // address and decrement counter, then wait for ack.
          if (in_xfer) begin
            blk_addr      <= blk_inc_addr ? blk_addr + 4 : blk_addr;
            blk_words_rem <= blk_words_rem - 1;
            if (s_axis_ctrl_tlast && blk_words_rem > 1) begin
              // Packet ended before all expected data words arrived. Force
              // blk_words_rem to 0 so ST_WR_WAIT_ACK exits after the current
              // ack, and then report CMDERR.
              blk_words_rem <= 0;
              resp_status   <= CTRL_STS_CMDERR;
            end
            state <= ST_WR_WAIT_ACK;
          end
        end

        ST_WR_WAIT_ACK: begin
          if (ctrlport_resp_ack) begin
            resp_status <= resolve_status(ctrlport_resp_status, resp_status);
            if (blk_words_rem == 0)
              state <= ST_OUT_HDR_0;
            else
              state <= ST_WR_IN_DATA;
          end
        end

        //---------------------------------------------------------------------
        // Read-then-Write path (READ_WRITE)
        //---------------------------------------------------------------------

        ST_RW_IN_DATA: begin
          // data[0] (the write operand) arrives in this state. CtrlPort read
          // and write are issued combinationally in this state, unless the
          // packet is the wrong length.
          if (in_xfer) begin
            if (s_axis_ctrl_tlast)
              state <= ST_RW_WAIT_ACK;
            else begin
              // Packet has extra words. Drain them then return an error.
              resp_status <= CTRL_STS_CMDERR;
              drain_to    <= ST_OUT_HDR_0;
              state       <= ST_DRAIN;
            end
          end
        end

        ST_RW_WAIT_ACK: begin
          // Single ack. Push read result into FIFO.
          if (ctrlport_resp_ack) begin
            resp_status <= ctrl_status_t'(ctrlport_resp_status);
            state       <= ST_OUT_HDR_0;
          end
        end

        //---------------------------------------------------------------------
        // Read path (READ / BLOCK_READ)
        //---------------------------------------------------------------------

        ST_RD_REQ: begin
          // Combinational process drives ctrlport_req_rd this cycle. Advance
          // address and decrement counter, then wait for ack.
          blk_addr      <= blk_inc_addr ? blk_addr + 4 : blk_addr;
          blk_words_rem <= blk_words_rem - 1;
          state         <= ST_RD_WAIT_ACK;
        end

        ST_RD_WAIT_ACK: begin
          // Push received data into rd_fifo (combinational process).
          if (ctrlport_resp_ack) begin
            resp_status <= resolve_status(ctrlport_resp_status, resp_status);
            if (blk_words_rem == 0)
              state <= ST_OUT_HDR_0;
            else
              state <= ST_RD_REQ;
          end
        end

        //---------------------------------------------------------------------
        // Poll path (POLL)
        //---------------------------------------------------------------------

        ST_POLL_LOAD_DATA: begin
          poll_data <= s_axis_ctrl_tdata;
          if (in_xfer) begin
            if (s_axis_ctrl_tlast) begin
              // Truncated packet.
              resp_status <= CTRL_STS_CMDERR;
              state       <= ST_OUT_HDR_0;
            end else
              state <= ST_POLL_LOAD_MASK;
          end
        end

        ST_POLL_LOAD_MASK: begin
          poll_mask <= s_axis_ctrl_tdata;
          if (in_xfer) begin
            if (s_axis_ctrl_tlast) begin
              resp_status <= CTRL_STS_CMDERR;
              state       <= ST_OUT_HDR_0;
            end else
              state <= ST_POLL_LOAD_TIMEOUT;
          end
        end

        ST_POLL_LOAD_TIMEOUT: begin
          poll_timeout     <= s_axis_ctrl_tdata;
          poll_data_masked <= poll_data & poll_mask;
          if (in_xfer) begin
            if (s_axis_ctrl_tlast)
              state <= ST_POLL_REQ;
            else begin
              // More words than expected. Drain then begin polling.
              drain_to <= ST_POLL_REQ;
              state    <= ST_DRAIN;
            end
          end
        end

        ST_POLL_REQ: begin
          // Issue one CtrlPort read (asserted combinationally). The timeout
          // counter is intentionally NOT decremented here so that the first
          // read always goes before any timeout check.
          state <= ST_POLL_WAIT_ACK;
        end

        ST_POLL_WAIT_ACK: begin
          // Capture response data, get condition result, and set timeout flag
          // so that ST_POLL_CHECK only needs to check 1-bit registers.
          poll_resp_data <= ctrlport_resp_data;
          poll_cond_met  <= (ctrlport_resp_data & poll_mask) == poll_data_masked;
          poll_timed_out <= (poll_timeout == 0);

          // Timeout counter runs every cycle while waiting for the ack. We
          // only check whether it has expired after the ack arrives, so at
          // least one read always completes before a timeout is reported.
          if (poll_timeout != 0) begin
            poll_timeout <= poll_timeout - 1;
          end

          if (ctrlport_resp_ack) begin
            state <= ST_POLL_CHECK;
          end
        end

        ST_POLL_CHECK: begin
          if (poll_cond_met) begin
            // Condition met. Push response into FIFO (done combinationally)
            // and report success.
            resp_status <= CTRL_STS_OKAY;
            state       <= ST_OUT_HDR_0;
          end else if (poll_timed_out) begin
            // No match and timeout expired. Push response and report failure.
            resp_status <= CTRL_STS_CMDERR;
            state       <= ST_OUT_HDR_0;
          end else begin
            // No match but there's time remaining. Issue another read.
            state <= ST_POLL_REQ;
          end
        end

        //---------------------------------------------------------------------
        // Sleep
        //---------------------------------------------------------------------

        ST_SLEEP_LOAD: begin
          // Consume data[0], which carries the sleep count.
          sleep_cntr <= s_axis_ctrl_tdata;
          if (in_xfer) begin
            if (s_axis_ctrl_tlast)
              state <= ST_SLEEP;
            else begin
              drain_to <= ST_SLEEP;
              state    <= ST_DRAIN;
            end
          end
        end

        ST_SLEEP: begin
          resp_status <= CTRL_STS_OKAY;
          if (sleep_cntr == 0) begin
            state <= ST_OUT_HDR_0;
          end
          sleep_cntr <= sleep_cntr - 1;
        end

        //---------------------------------------------------------------------
        // Response output
        //---------------------------------------------------------------------

        ST_OUT_HDR_0: begin
          fifo_has_rd_data <= rd_fifo_o_tvalid;
          if (out_xfer)
            state <= ST_OUT_HDR_1;
        end

        ST_OUT_HDR_1: begin
          if (out_xfer)
            state <= cached_hdr.has_time ? ST_OUT_TS_LO : ST_OUT_OP_WORD;
        end

        ST_OUT_TS_LO: begin
          if (out_xfer)
            state <= ST_OUT_TS_HI;
        end

        ST_OUT_TS_HI: begin
          if (out_xfer)
            state <= ST_OUT_OP_WORD;
        end

        ST_OUT_OP_WORD: begin
          // Emit op-word. If read data is in the FIFO, send it out next.
          if (out_xfer) begin
            if (fifo_has_rd_data)
              state <= ST_OUT_RD_DATA;
            else
              state <= ST_IN_HDR_0;
          end
        end

        ST_OUT_RD_DATA: begin
          if (out_xfer && rd_fifo_occupied == 1)
            state <= ST_IN_HDR_0;
        end

        //---------------------------------------------------------------------
        // Drop / drain
        //---------------------------------------------------------------------

        ST_DROP: begin
          if (in_xfer && s_axis_ctrl_tlast)
            state <= ST_IN_HDR_0;
        end

        ST_DRAIN: begin
          // Consume words until tlast, then proceed to the drain_to state.
          if (in_xfer && s_axis_ctrl_tlast) begin
            state <= drain_to;
          end
        end

        default: begin
          state <= ST_IN_HDR_0;
        end

      endcase
    end
  end


  //---------------------------------------------------------------------------
  // State Machine Combinational Process
  //---------------------------------------------------------------------------

  always_comb begin
    // Defaults assignments
    s_axis_ctrl_tready    = '0;
    m_axis_ctrl_tdata     = '0;
    m_axis_ctrl_tlast     = '0;
    m_axis_ctrl_tvalid    = '0;
    ctrlport_req_wr       = '0;
    ctrlport_req_rd       = '0;
    ctrlport_req_addr     = blk_addr;
    ctrlport_req_data     = s_axis_ctrl_tdata;
    ctrlport_req_byte_en  = cached_op.byte_enable;
    ctrlport_req_has_time = cached_hdr.has_time;
    ctrlport_req_time     = cached_ts;
    rd_fifo_i_tvalid      = '0;
    rd_fifo_i_tdata       = '0;
    rd_fifo_o_tready      = '0;

    unique case (state)
      ST_IN_HDR_0, ST_IN_HDR_1, ST_IN_TS_LO, ST_IN_TS_HI, ST_IN_OP_WORD: begin
        s_axis_ctrl_tready = 1;
      end

      ST_WR_IN_DATA: begin
        // Accept data word and issue CtrlPort write
        s_axis_ctrl_tready = 1;
        ctrlport_req_wr    = s_axis_ctrl_tvalid;
      end

      ST_WR_WAIT_ACK: begin
        ; // No input/output activity while waiting
      end

      ST_RW_IN_DATA: begin
        // Accept data[0] and simultaneously issue read and write
        s_axis_ctrl_tready = 1;
        ctrlport_req_wr    = s_axis_ctrl_tvalid & s_axis_ctrl_tlast;
        ctrlport_req_rd    = s_axis_ctrl_tvalid & s_axis_ctrl_tlast;
      end

      ST_RW_WAIT_ACK: begin
        // Push read result into FIFO when ack arrives
        rd_fifo_i_tvalid = ctrlport_resp_ack;
        rd_fifo_i_tdata  = ctrlport_resp_data;
      end

      ST_RD_REQ: begin
        // Issue CtrlPort read for one cycle
        ctrlport_req_rd = 1;
      end

      ST_RD_WAIT_ACK: begin
        // Push data into FIFO when ack arrives
        rd_fifo_i_tvalid = ctrlport_resp_ack;
        rd_fifo_i_tdata  = ctrlport_resp_data;
      end

      ST_SLEEP_LOAD: begin
        s_axis_ctrl_tready = 1;
        // Sleep count is consumed but not echoed in the response.
      end

      ST_POLL_LOAD_DATA, ST_POLL_LOAD_MASK, ST_POLL_LOAD_TIMEOUT: begin
        s_axis_ctrl_tready = 1;
      end

      ST_POLL_REQ: begin
        // Issue one CtrlPort read for one cycle.
        ctrlport_req_rd = 1;
      end

      ST_POLL_WAIT_ACK: begin
        ; // Response is captured into registers this cycle.
      end

      ST_POLL_CHECK: begin
        // Push the captured response into the FIFO only when the poll
        // terminates (condition matched or timeout expired).
        rd_fifo_i_tvalid = poll_cond_met || poll_timed_out;
        rd_fifo_i_tdata  = poll_resp_data;
      end

      ST_SLEEP: begin
        ; // Nothing to drive
      end

      ST_OUT_HDR_0: begin
        m_axis_ctrl_tvalid = 1;
        m_axis_ctrl_tlast  = 0;
        m_axis_ctrl_tdata  = axis_ctrl_build_hdr_lo(
          cached_hdr.rem_dst_epid,
          1'b1,                    // is_ack
          cached_hdr.has_time,
          num_data,
          cached_hdr.src_port      // Swap the source and destination
        );
      end

      ST_OUT_HDR_1: begin
        m_axis_ctrl_tvalid = 1;
        m_axis_ctrl_tlast  = 0;
        m_axis_ctrl_tdata  = axis_ctrl_build_hdr_hi(
          cached_hdr.req_size,
          cached_hdr.seq_num,
          cached_hdr.rem_dst_port,
          cached_hdr.dst_port      // Swap the source and destination
        );
      end

      ST_OUT_TS_LO: begin
        m_axis_ctrl_tvalid = 1;
        m_axis_ctrl_tlast  = 0;
        m_axis_ctrl_tdata  = cached_ts[31:0];
      end

      ST_OUT_TS_HI: begin
        m_axis_ctrl_tvalid = 1;
        m_axis_ctrl_tlast  = 0;
        m_axis_ctrl_tdata  = cached_ts[63:32];
      end

      ST_OUT_OP_WORD: begin
        m_axis_ctrl_tvalid = 1;
        m_axis_ctrl_tlast  = !fifo_has_rd_data;  // Last word if no read data follows
        m_axis_ctrl_tdata  = axis_ctrl_build_op_word(
          resp_status,
          cached_op.op_code,
          cached_op.byte_enable,
          cached_op.address
        );
      end

      ST_OUT_RD_DATA: begin
        m_axis_ctrl_tvalid = rd_fifo_o_tvalid;
        m_axis_ctrl_tlast  = (rd_fifo_occupied == 1);
        m_axis_ctrl_tdata  = rd_fifo_o_tdata;
        rd_fifo_o_tready   = m_axis_ctrl_tready;
      end

      ST_DROP, ST_DRAIN: begin
        s_axis_ctrl_tready = 1;
      end

      default: begin
        ; // Covered by defaults above
      end
    endcase
  end


  //---------------------------------------------------------------------------
  // Read-data FIFO
  //---------------------------------------------------------------------------

  // Buffers read data words while acks are being gathered. All read paths
  // (READ, BLOCK_READ, READ_WRITE) push data here. ST_OUT_RD_DATA drains it.

  axi_fifo #(
    .WIDTH (32),
    .SIZE  (4)
  ) rd_fifo (
    .clk      (clk),
    .reset    (rst),
    .clear    ('0),
    .i_tdata  (rd_fifo_i_tdata),
    .i_tvalid (rd_fifo_i_tvalid),
    .i_tready (),
    .o_tdata  (rd_fifo_o_tdata),
    .o_tvalid (rd_fifo_o_tvalid),
    .o_tready (rd_fifo_o_tready),
    .space    (),
    .occupied (rd_fifo_occupied)
  );

endmodule : axis_ctrl_slave


`default_nettype wire

//
// Copyright 2020 Ettus Research, A National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: PkgCtrlIfaceBfm
//
// Description: This package includes high-level bus functional models (BFM)
// for the AXIS-Ctrl interface of a Stream Endpoint.
//


package PkgCtrlIfaceBfm;

  `include "usrp_utils.svh"

  import rfnoc_chdr_utils_pkg::*;
  import PkgAxisCtrlBfm::*;

  // Default timestamp value to indicate no timestamp is provided in a control packet
  // (i.e. has_time = 0).
  localparam chdr_timestamp_t RESERVED_TS = {CHDR_TIMESTAMP_W{1'b1}} - 1;

  class CtrlIfaceBfm extends AxisCtrlBfm;
    ctrl_port_t    dst_port;
    ctrl_port_t    src_port;
    ctrl_seq_num_t seq_num;

    // Maximum number of data words per AXIS-Ctrl packet.
    localparam int MAX_CTRL_NUM_DATA = 2**$bits(ctrl_num_data_t)-1;

    // Class constructor to create a new BFM instance.
    //
    //   m_chdr:    Interface for the master connection (BFM's AXIS output)
    //   s_chdr:    Interface for the slave connection (BFM's AXIS input)
    //   src_port:  Source port to use in generated control packets
    //
    function new(
      virtual AxiStreamIf #(32).master m_chdr,
      virtual AxiStreamIf #(32).slave  s_chdr,
      ctrl_port_t dst_port,
      ctrl_port_t src_port
    );
      super.new(m_chdr, s_chdr);
      this.dst_port = dst_port;
      this.src_port = src_port;
      this.seq_num  = '0;
    endfunction : new


    // Send an AXIS-Ctrl read request packet and get the response.
    //
    //   addr:       Address for the read request
    //   word:       Data word that was returned in response to the read
    //   timestamp:  Timestamp for the read request (Optional)
    //
    task reg_read (
      input  ctrl_address_t   addr,
      output ctrl_word_t      word,
      input  chdr_timestamp_t timestamp = RESERVED_TS
    );
      AxisCtrlPacket ctrl_packet;

      // Create the AXIS-Ctrl packet
      ctrl_packet = new();
      ctrl_packet.header = '{
        seq_num  : seq_num++,
        num_data : 1,
        src_port : src_port,
        dst_port : dst_port,
        has_time : (timestamp != RESERVED_TS),
        default  : 0
      };
      ctrl_packet.op_word = '{
        status      : CTRL_STS_OKAY,
        op_code     : CTRL_OP_READ,
        byte_enable : '1,
        address     : addr,
        default     : 0
      };
      ctrl_packet.data = {};

      if (ctrl_packet.header.has_time) begin
        ctrl_packet.timestamp = timestamp;
      end

      // Send the control packet and get the response
      put_ctrl(ctrl_packet);
      get_ctrl(ctrl_packet);
      word = ctrl_packet.data[0];

      assert(ctrl_packet.header.is_ack == 1 &&
             ctrl_packet.op_word.status == CTRL_STS_OKAY) else begin
        $fatal(1, "CtrlIfaceBfm::reg_read: Did not receive CTRL_STS_OKAY status");
      end
    endtask : reg_read


    // Send an AXIS-Ctrl write request packet and get the response.
    //
    //   addr:       Address for the write request
    //   word:       Data word to write
    //   timestamp:  Timestamp for the write request(Optional)
    //
    task reg_write (
      ctrl_address_t   addr,
      ctrl_word_t      word,
      chdr_timestamp_t timestamp = RESERVED_TS
    );
      AxisCtrlPacket ctrl_packet;

      // Create the AXIS-Ctrl packet
      ctrl_packet = new();
      ctrl_packet.header = '{
        seq_num  : seq_num++,
        num_data : 1,
        src_port : src_port,
        dst_port : dst_port,
        has_time : (timestamp != RESERVED_TS),
        default  : 0
      };
      ctrl_packet.op_word = '{
        status      : CTRL_STS_OKAY,
        op_code     : CTRL_OP_WRITE,
        byte_enable : '1,
        address     : addr,
        default     : 0
      };

      if (ctrl_packet.header.has_time) begin
        ctrl_packet.timestamp = timestamp;
      end

      // Send the packet and get the response
      ctrl_packet.data = { word };
      put_ctrl(ctrl_packet);
      get_ctrl(ctrl_packet);
      word = ctrl_packet.data[0];

      assert(ctrl_packet.header.is_ack == 1 &&
             ctrl_packet.op_word.status == CTRL_STS_OKAY) else begin
        $fatal(1, "CtrlIfaceBfm::reg_write: Did not receive CTRL_STS_OKAY status");
      end
    endtask : reg_write


    // Send an AXIS-Ctrl block read request for two consecutive 32-bit words
    // and return them as a single 64-bit value. The lower word is read from
    // addr and the upper word from addr+4.
    //
    //   addr:       Address of the lower 32-bit word
    //   word:       64-bit value returned (lower word at addr, upper at addr+4)
    //   timestamp:  Timestamp for the read request (Optional)
    //
    task reg_read64 (
      input  ctrl_address_t   addr,
      output logic [63:0]     word,
      input  chdr_timestamp_t timestamp = RESERVED_TS
    );
      ctrl_word_t data[$];
      block_read(addr, 2, data, timestamp);
      word = { data[1], data[0] };
    endtask : reg_read64


    // Send an AXIS-Ctrl block write request for two consecutive 32-bit words
    // from a single 64-bit value. The lower word is written to addr and the
    // upper word to addr+4.
    //
    //   addr:       Address of the lower 32-bit word
    //   word:       64-bit value to write (lower word to addr, upper to addr+4)
    //   timestamp:  Timestamp for the write request (Optional)
    //
    task reg_write64 (
      input  ctrl_address_t   addr,
      input  logic [63:0]     word,
      input  chdr_timestamp_t timestamp = RESERVED_TS
    );
      ctrl_word_t data[$] = '{ word[31:0], word[63:32] };
      block_write(addr, data, timestamp);
    endtask : reg_write64


    // Send AXIS-Ctrl block write requests and get the responses. Writes
    // num_data words to consecutive addresses starting at addr (addr, addr+4,
    // addr+8, ...). Requests are split into chunks of at most
    // MAX_CTRL_NUM_DATA.
    //
    //   addr:       Starting address for the block write
    //   data:       Data words to write
    //   timestamp:  Timestamp for the write request (Optional)
    //
    task block_write (
      input  ctrl_address_t   addr,
      input  ctrl_word_t      data[$],
      input  chdr_timestamp_t timestamp = RESERVED_TS
    );
      bulk_write(CTRL_OP_BLOCK_WRITE, addr, data, timestamp);
    endtask : block_write


    // Send AXIS-Ctrl block read requests and get the responses. Reads
    // num_words words from consecutive addresses starting at addr (addr,
    // addr+4, addr+8, ...). Requests are split into chunks of at most
    // MAX_CTRL_NUM_DATA.
    //
    //   addr:       Starting address for the block read
    //   num_words:  Number of words to read
    //   data:       Data words returned in the response
    //   timestamp:  Timestamp for the read request (Optional)
    //
    task block_read (
      input  ctrl_address_t   addr,
      input  int              num_words,
      output ctrl_word_t      data[$],
      input  chdr_timestamp_t timestamp = RESERVED_TS
    );
      bulk_read(CTRL_OP_BLOCK_READ, addr, num_words, data, timestamp);
    endtask : block_read


    // Send AXIS-Ctrl burst write requests and get the responses. Writes
    // num_data words to the same address (no address increment). Requests are
    // split into chunks of at most MAX_CTRL_NUM_DATA.
    //
    //   addr:       Address for all writes
    //   data:       Data words to write
    //   timestamp:  Timestamp for the write request (Optional)
    //
    task burst_write (
      input  ctrl_address_t   addr,
      input  ctrl_word_t      data[$],
      input  chdr_timestamp_t timestamp = RESERVED_TS
    );
      bulk_write(CTRL_OP_WRITE, addr, data, timestamp);
    endtask : burst_write


    // Send an AXIS-Ctrl burst read requests and get the responses. Reads
    // num_words words from the same address (no address increment). Requests
    // are split into chunks of at most MAX_CTRL_NUM_DATA.
    //
    //   addr:       Address for all reads
    //   num_words:  Number of words to read
    //   data:       Data words returned in the response
    //   timestamp:  Timestamp for the read request (Optional)
    //
    task burst_read (
      input  ctrl_address_t   addr,
      input  int              num_words,
      output ctrl_word_t      data[$],
      input  chdr_timestamp_t timestamp = RESERVED_TS
    );
      bulk_read(CTRL_OP_READ, addr, num_words, data, timestamp);
    endtask : burst_read


    //---------------------------------------------------------------------------
    // Internal Helpers
    //---------------------------------------------------------------------------

    // Helper for block_write and burst_write. Sends AXIS-Ctrl write requests
    // in chunks and gets the responses. The op_code determines whether the
    // address increments between chunks (CTRL_OP_BLOCK_WRITE) or stays fixed
    // (CTRL_OP_WRITE).
    //
    //   op_code:    CTRL_OP_BLOCK_WRITE (incrementing) or CTRL_OP_WRITE (fixed)
    //   addr:       Starting address for the writes
    //   data:       Data words to write
    //   timestamp:  Timestamp for the write request (Optional)
    //
    task bulk_write (
      input  ctrl_opcode_t    op_code,
      input  ctrl_address_t   addr,
      input  ctrl_word_t      data[$],
      input  chdr_timestamp_t timestamp = RESERVED_TS
    );
      AxisCtrlPacket ctrl_packet;
      int num_words = data.size();
      int offset    = 0;

      while (offset < num_words) begin
        int chunk_size = `MIN(num_words - offset, MAX_CTRL_NUM_DATA);

        ctrl_packet = new();
        ctrl_packet.header = '{
          seq_num  : seq_num++,
          num_data : chunk_size,
          src_port : src_port,
          dst_port : dst_port,
          has_time : (offset == 0 && timestamp != RESERVED_TS),
          default  : 0
        };
        ctrl_packet.op_word = '{
          status      : CTRL_STS_OKAY,
          op_code     : op_code,
          byte_enable : '1,
          address     : (op_code == CTRL_OP_BLOCK_WRITE) ?
                        addr + ctrl_address_t'(offset * 4) : addr,
          default     : 0
        };
        ctrl_packet.data = data[offset : offset + chunk_size - 1];

        if (ctrl_packet.header.has_time) begin
          ctrl_packet.timestamp = timestamp;
        end

        put_ctrl(ctrl_packet);
        get_ctrl(ctrl_packet);

        assert(ctrl_packet.header.is_ack == 1 &&
               ctrl_packet.op_word.status == CTRL_STS_OKAY) else begin
          $fatal(1, "CtrlIfaceBfm: bulk_write: Did not receive CTRL_STS_OKAY status");
        end

        offset += chunk_size;
      end
    endtask : bulk_write


    // Helper for block_read and burst_read. Sends AXIS-Ctrl read requests in
    // chunks and gets the responses. The op_code determines whether the
    // address increments between chunks (CTRL_OP_BLOCK_READ) or stays fixed
    // (CTRL_OP_READ).
    //
    //   op_code:    CTRL_OP_BLOCK_READ (incrementing) or CTRL_OP_READ (fixed)
    //   addr:       Starting address for the reads
    //   num_words:  Number of words to read
    //   data:       Data words returned in the response
    //   timestamp:  Timestamp for the read request (Optional)
    //
    task bulk_read (
      input  ctrl_opcode_t    op_code,
      input  ctrl_address_t   addr,
      input  int              num_words,
      output ctrl_word_t      data[$],
      input  chdr_timestamp_t timestamp = RESERVED_TS
    );
      AxisCtrlPacket ctrl_packet;
      int offset = 0;
      data = {};

      while (offset < num_words) begin
        int chunk_size = `MIN(num_words - offset, MAX_CTRL_NUM_DATA);

        ctrl_packet = new();
        ctrl_packet.header = '{
          seq_num  : seq_num++,
          num_data : chunk_size,
          src_port : src_port,
          dst_port : dst_port,
          has_time : (offset == 0 && timestamp != RESERVED_TS),
          default  : 0
        };
        ctrl_packet.op_word = '{
          status      : CTRL_STS_OKAY,
          op_code     : op_code,
          byte_enable : '1,
          address     : (op_code == CTRL_OP_BLOCK_READ) ?
                        addr + ctrl_address_t'(offset * 4) : addr,
          default     : 0
        };
        ctrl_packet.data = {};

        if (ctrl_packet.header.has_time) begin
          ctrl_packet.timestamp = timestamp;
        end

        put_ctrl(ctrl_packet);
        get_ctrl(ctrl_packet);

        assert(ctrl_packet.header.is_ack == 1 &&
               ctrl_packet.op_word.status == CTRL_STS_OKAY) else begin
          $fatal(1, "CtrlIfaceBfm: bulk_read: Did not receive CTRL_STS_OKAY status");
        end

        data = {data, ctrl_packet.data};
        offset += chunk_size;
      end
    endtask : bulk_read

  endclass : CtrlIfaceBfm


endpackage : PkgCtrlIfaceBfm

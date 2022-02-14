//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: axis_replay.v
//
// Description:
//
//   This block implements the registers, state machines, and control logic for
//   recording and playback of AXI-Stream data using an attached memory as a
//   buffer. It has a set of registers for controlling recording and a set of
//   registers for controlling playback. See rfnoc_replay_regs.vh for a
//   description of the registers.
//
//   RECORDING
//
//   The AXI-Stream data received on the input port is written to the attached
//   memory into a buffer space configured by the record registers. The
//   REG_REC_BASE_ADDR register indicates the starting address for the record
//   buffer and REG_REC_BUFFER_SIZE indicates how much memory to allocate for
//   recording. REG_REC_FULLNESS can be used to determine how much data has
//   been buffered. Once the configured buffer size has filled, the block stops
//   accepting data. That is, it will deassert i_tready to stall any input
//   data. Recording can be restarted (REG_REC_RESTART) to accept the remaining
//   data and write it at the beginning of the configured buffer.
//
//   PLAYBACK
//
//   Playback is completely independent of recording. The playback buffer is
//   configured similarly using its own registers. Playback is started by
//   writing a command to the REG_PLAY_CMD register. The play command indicates
//   if it should play a fixed number of words then stop (PLAY_CMD_FINITE),
//   playback forever (PLAY_CMD_CONTINUOUS), or stop playback (PLAY_CMD_STOP).
//   The beginning of the playback is set by first writing to
//   REG_PLAY_BASE_ADDR. The number of words to play back is set by first
//   writing to REG_PLAY_CMD_NUM_WORDS.
//
//   The length of the packets generated during playback is configured by the
//   REG_PLAY_WORDS_PER_PKT register.
//
//   A timestamp for playback can also be specified by setting
//   REG_PLAY_CMD_TIME and setting the REG_PLAY_TIMED_POS bit as part of the
//   command write. The timestamp will be included in the first output packet.
//
//   When playback reaches the end of the configured playback buffer, if more
//   words were requested, it will loop back to the beginning of the buffer to
//   continue playing data. The last packet of playback will always have the
//   EOB flag set (e.g., after REG_PLAY_CMD_NUM_WORDS have been played back or
//   after PLAY_CMD_STOP has been issued).
//
//   MEMORY SHARING
//
//   Because the record and playback logic share the same memory and can
//   operate independently, care must be taken to manage the record and
//   playback buffers. You should ensure that recording is complete before
//   trying to play back the recorded data. Simultaneous recording and playing
//   back is allowed, but is only recommended when the recording and playback
//   are to different sections of memory, such that unintended overlap of the
//   write/read pointers will never occur.
//
//   Furthermore, if multiple replay modules are instantiated and share the
//   same external memory, care must be taken to not unintentionally affect the
//   contents of neighboring buffers.
//
//   MEMORY WORD SIZE
//
//   The address and size registers are in terms of bytes. But playback and
//   recording length and fullness are in terms of memory words (MEM_DATA_W
//   bits wide). The current implementation can't read/write to the memory in
//   units other than the memory word size. So care must be taken to ensure
//   that REG_PLAY_CMD_NUM_WORDS and REG_PLAY_WORDS_PER_PKT always indicate the
//   number of memory words intended. The number of samples to playback or
//   record must always represent an amount of data that is a multiple of the
//   memory word size.
//

`default_nettype none


module axis_replay #(
  parameter MEM_DATA_W  = 64,
  parameter MEM_ADDR_W  = 34, // Byte address width used by memory controller
  parameter MEM_COUNT_W = 8   // Length of counters used to connect to the
                              // memory interface's read and write ports.
) (
  input wire clk,
  input wire rst,  // Synchronous to clk

  //---------------------------------------------------------------------------
  // Settings Bus
  //---------------------------------------------------------------------------

  input  wire        s_ctrlport_req_wr,
  input  wire        s_ctrlport_req_rd,
  input  wire [19:0] s_ctrlport_req_addr,
  input  wire [31:0] s_ctrlport_req_data,
  output reg         s_ctrlport_resp_ack,
  output reg  [31:0] s_ctrlport_resp_data,

  //---------------------------------------------------------------------------
  // AXI Stream Interface
  //---------------------------------------------------------------------------

  // Input
  input  wire [MEM_DATA_W-1:0] i_tdata,
  input  wire                  i_tvalid,
  input  wire                  i_tlast,
  output wire                  i_tready,

  // Output
  output wire [MEM_DATA_W-1:0] o_tdata,
  output wire [          63:0] o_ttimestamp,
  output wire                  o_thas_time,
  output wire                  o_teob,
  output wire                  o_tvalid,
  output wire                  o_tlast,
  input  wire                  o_tready,

  //---------------------------------------------------------------------------
  // Memory Interface
  //---------------------------------------------------------------------------

  // Write interface
  output reg  [ MEM_ADDR_W-1:0] write_addr,      // Byte address for start of write
                                                 // transaction (64-bit aligned).
  output reg  [MEM_COUNT_W-1:0] write_count,     // Count of 64-bit words to write, minus 1.
  output reg                    write_ctrl_valid,
  input  wire                   write_ctrl_ready,
  output wire [ MEM_DATA_W-1:0] write_data,
  output wire                   write_data_valid,
  input  wire                   write_data_ready,

  // Read interface
  output reg  [ MEM_ADDR_W-1:0] read_addr,       // Byte address for start of read
                                                 // transaction (64-bit aligned).
  output reg  [MEM_COUNT_W-1:0] read_count,      // Count of 64-bit words to read, minus 1.
  output reg                    read_ctrl_valid,
  input  wire                   read_ctrl_ready,
  input  wire [ MEM_DATA_W-1:0] read_data,
  input  wire                   read_data_valid,
  output wire                   read_data_ready
);

  `include "rfnoc_block_replay_regs.vh"

  //---------------------------------------------------------------------------
  // Constants
  //---------------------------------------------------------------------------

  localparam [REG_MAJOR_LEN-1:0] COMPAT_MAJOR = 1;
  localparam [REG_MINOR_LEN-1:0] COMPAT_MINOR = 1;

  localparam [REG_ITEM_SIZE_LEN-1:0] DEFAULT_ITEM_SIZE = 4;  // 4 bytes for sc16

  localparam NUM_WORDS_W = REG_CMD_NUM_WORDS_LEN;
  localparam TIME_W      = REG_CMD_TIME_LEN;
  localparam CMD_W       = REG_PLAY_CMD_LEN;
  localparam WPP_W       = REG_PLAY_WORDS_PER_PKT_LEN;
  localparam MEM_SIZE_W  = MEM_ADDR_W + 1;    // Number of bits needed to
                                              // represent memory size in bytes.

  // Memory Alignment
  //
  // Size of DATA_WIDTH in bytes
  localparam BYTES_PER_WORD = MEM_DATA_W/8;
  //
  // The lower MEM_ALIGN bits for all memory byte addresses should be 0.
  localparam MEM_ALIGN = $clog2(MEM_DATA_W / 8);
  //
  // AXI alignment requirement (4096 bytes) in MEM_DATA_W-bit words
  localparam AXI_ALIGNMENT = 4096 / BYTES_PER_WORD;

  // Memory Buffering Parameters
  //
  // Log base 2 of the depth of the input and output FIFOs to use. The FIFOs
  // should be large enough to store more than a complete burst
  // (MEM_BURST_LEN). A size of 9 (512 64-bit words) is one 36-kbit BRAM.
  localparam REC_FIFO_ADDR_WIDTH  = MEM_COUNT_W+1;  // Log2 of input/record FIFO size
  localparam PLAY_FIFO_ADDR_WIDTH = MEM_COUNT_W+1;  // Log2 of output/playback FIFO size
  localparam HDR_FIFO_ADDR_WIDTH  = 5;              // Log2 of output/time FIFO size
  //
  // Amount of data to buffer before writing to RAM. It must not exceed
  // 2**MEM_COUNT_W (the maximum count allowed by an AXI master).
  localparam MEM_BURST_LEN = 2**MEM_COUNT_W;  // Size in MEM_DATA_W-sized words
  //
  // Clock cycles to wait before writing something less than MEM_BURST_LEN
  // to memory.
  localparam DATA_WAIT_TIMEOUT = 31;


  //---------------------------------------------------------------------------
  // Functions
  //---------------------------------------------------------------------------

  function integer max(input integer a, b);
    begin
      if (a > b) max = a;
      else max = b;
    end
  endfunction

  function integer min(input integer a, b);
    begin
      if (a < b) min = a;
      else min = b;
    end
  endfunction

  // This zeros the lower MEM_ALIGN bits of the input address.
  function [MEM_SIZE_W-1:0] mem_align(input [MEM_SIZE_W-1:0] addr);
    begin
      mem_align = { addr[MEM_SIZE_W-1 : MEM_ALIGN], {MEM_ALIGN{1'b0}} };
    end
  endfunction


  //---------------------------------------------------------------------------
  // Data FIFO Signals
  //---------------------------------------------------------------------------

  // Record Data FIFO (Input)
  wire [MEM_DATA_W-1:0] rec_fifo_o_tdata;
  wire                  rec_fifo_o_tvalid;
  wire                  rec_fifo_o_tready;
  wire [          15:0] rec_fifo_occupied;

  // Playback Data FIFO (Output)
  wire [MEM_DATA_W-1:0] play_fifo_i_tdata;
  wire                  play_fifo_i_tvalid;
  wire                  play_fifo_i_tready;
  wire [          15:0] play_fifo_space;


  //---------------------------------------------------------------------------
  // Registers
  //---------------------------------------------------------------------------

  reg        [MEM_ADDR_W-1:0] reg_rec_base_addr;
  reg        [MEM_SIZE_W-1:0] reg_rec_buffer_size;
  reg                  [31:0] reg_rec_fullness_hi;
  reg                  [31:0] reg_rec_pos_hi;
  reg                         rec_restart;
  reg        [MEM_ADDR_W-1:0] reg_play_base_addr;
  reg        [MEM_SIZE_W-1:0] reg_play_buffer_size;
  reg                  [31:0] reg_play_pos_hi;
  reg       [NUM_WORDS_W-1:0] reg_play_cmd_num_words;
  reg            [TIME_W-1:0] reg_play_cmd_time;
  reg             [CMD_W-1:0] reg_play_cmd;
  reg                         reg_play_cmd_timed;
  reg                         reg_play_cmd_valid;
  wire                        reg_play_cmd_ready;
  reg                         play_cmd_stop;
  reg                         clear_cmd_fifo;
  reg             [WPP_W-1:0] reg_play_words_per_pkt = REG_PLAY_WORDS_PER_PKT_INIT;
  reg [REG_ITEM_SIZE_LEN-1:0] reg_item_size = DEFAULT_ITEM_SIZE;
  wire                  [5:0] reg_cmd_fifo_space;

  wire [63:0] reg_rec_fullness;
  wire [63:0] reg_rec_pos;
  wire [63:0] reg_play_pos;
  reg         rec_restart_clear;
  reg         play_cmd_stop_ack;

  reg [REG_ITEM_SIZE_LEN-1:0] items_per_word;

  // Create aligned versions of the settings registers
  wire [MEM_ADDR_W-1:0] rec_base_addr_sr;    // Byte address
  wire [MEM_SIZE_W-1:0] rec_buffer_size_sr;  // Size in bytes
  wire [MEM_ADDR_W-1:0] play_base_addr_sr;   // Byte address
  wire [MEM_SIZE_W-1:0] play_buffer_size_sr; // Size in bytes

  assign rec_base_addr_sr    = mem_align(reg_rec_base_addr);
  assign rec_buffer_size_sr  = mem_align(reg_rec_buffer_size);
  assign play_base_addr_sr   = mem_align(reg_play_base_addr);
  assign play_buffer_size_sr = mem_align(reg_play_buffer_size);

  always @(posedge clk) begin
    if (rst) begin
      reg_rec_base_addr      <= 0;
      reg_rec_buffer_size    <= 0;
      reg_rec_fullness_hi    <= 'bX;
      reg_rec_pos_hi         <= 'bX;
      reg_play_base_addr     <= 0;
      reg_play_buffer_size   <= 0;
      reg_play_pos_hi        <= 'bX;
      reg_play_cmd_num_words <= 0;
      reg_play_cmd_time      <= 0;
      reg_play_words_per_pkt <= REG_PLAY_WORDS_PER_PKT_INIT;
      reg_item_size          <= DEFAULT_ITEM_SIZE;
      items_per_word         <= 'bX;
      rec_restart            <= 0;
      play_cmd_stop          <= 0;
      clear_cmd_fifo         <= 0;
      reg_play_cmd           <= 'bX;
      reg_play_cmd_timed     <= 'bX;
      reg_play_cmd_valid     <= 0;
      s_ctrlport_resp_data   <= 'bX;
      s_ctrlport_resp_ack    <= 0;
    end else begin
      // Default assignments
      s_ctrlport_resp_data   <= 0;
      s_ctrlport_resp_ack    <= 0;
      reg_play_cmd_valid     <= 0;
      clear_cmd_fifo         <= 0;

      if (rec_restart_clear) begin
        rec_restart <= 0;
      end

      if (play_cmd_stop_ack) begin
        play_cmd_stop <= 0;
      end

      //-----------------------------------------
      // Register Reads
      //-----------------------------------------

      if (s_ctrlport_req_rd) begin
        s_ctrlport_resp_ack  <= 1;
        case (s_ctrlport_req_addr)
          REG_COMPAT : begin
            s_ctrlport_resp_data[REG_MAJOR_POS+:REG_MAJOR_LEN]
              <= COMPAT_MAJOR;
            s_ctrlport_resp_data[REG_MINOR_POS+:REG_MINOR_LEN]
              <= COMPAT_MINOR;
          end
          REG_MEM_SIZE : begin
            s_ctrlport_resp_data[REG_DATA_SIZE_POS+:REG_DATA_SIZE_LEN]
              <= MEM_DATA_W;
            s_ctrlport_resp_data[REG_ADDR_SIZE_POS+:REG_ADDR_SIZE_LEN]
              <= MEM_ADDR_W;
          end
          REG_REC_BASE_ADDR_LO :
            s_ctrlport_resp_data[min(32, MEM_ADDR_W)-1:0]
              <= reg_rec_base_addr[min(32, MEM_ADDR_W)-1:0];
          REG_REC_BASE_ADDR_HI :
            if (MEM_ADDR_W > 32)
              s_ctrlport_resp_data[0 +: max(MEM_ADDR_W-32, 1)]
                <= reg_rec_base_addr[32 +: max(MEM_ADDR_W-32, 1)];
          REG_REC_BUFFER_SIZE_LO :
            s_ctrlport_resp_data
              <= reg_rec_buffer_size[min(32, MEM_SIZE_W)-1:0];
          REG_REC_BUFFER_SIZE_HI :
            if (MEM_SIZE_W > 32)
              s_ctrlport_resp_data[0 +: max(MEM_SIZE_W-32, 1)]
                <= reg_rec_buffer_size[32 +: max(MEM_SIZE_W-32, 1)];
          REG_REC_FULLNESS_LO : begin
            s_ctrlport_resp_data <= reg_rec_fullness[31:0];
            if (MEM_SIZE_W > 32) begin
              // The LO register must be read first. Save HI part now to
              // guarantee coherence when HI register is read.
              reg_rec_fullness_hi <= 0;
              reg_rec_fullness_hi[0 +: max(MEM_SIZE_W-32, 1)]
                <= reg_rec_fullness[32 +: max(MEM_SIZE_W-32, 1)];
            end
          end
          REG_REC_FULLNESS_HI :
            if (MEM_SIZE_W > 32)
              // Return the saved value to guarantee coherence
              s_ctrlport_resp_data <= reg_rec_fullness_hi;
          REG_PLAY_BASE_ADDR_LO :
            s_ctrlport_resp_data[min(32, MEM_ADDR_W)-1:0]
              <= reg_play_base_addr[min(32, MEM_ADDR_W)-1:0];
          REG_PLAY_BASE_ADDR_HI :
            if (MEM_ADDR_W > 32)
              s_ctrlport_resp_data[0 +: max(MEM_ADDR_W-32, 1)]
                <= reg_play_base_addr[32 +: max(MEM_ADDR_W-32, 1)];
          REG_PLAY_BUFFER_SIZE_LO :
            s_ctrlport_resp_data[min(32, MEM_SIZE_W)-1:0]
              <= reg_play_buffer_size[min(32, MEM_SIZE_W)-1:0];
          REG_PLAY_BUFFER_SIZE_HI :
            if (MEM_SIZE_W > 32)
              s_ctrlport_resp_data[0 +: max(MEM_SIZE_W-32, 1)]
                <= reg_play_buffer_size[32 +: max(MEM_SIZE_W-32, 1)];
          REG_PLAY_CMD_NUM_WORDS_LO :
            s_ctrlport_resp_data <= reg_play_cmd_num_words[31:0];
          REG_PLAY_CMD_NUM_WORDS_HI :
              s_ctrlport_resp_data <= reg_play_cmd_num_words[63:32];
          REG_PLAY_CMD_TIME_LO :
            s_ctrlport_resp_data <= reg_play_cmd_time[31:0];
          REG_PLAY_CMD_TIME_HI :
            s_ctrlport_resp_data <= reg_play_cmd_time[63:32];
          REG_PLAY_WORDS_PER_PKT :
            s_ctrlport_resp_data[WPP_W-1:0] <= reg_play_words_per_pkt;
          REG_PLAY_ITEM_SIZE :
            s_ctrlport_resp_data[REG_ITEM_SIZE_POS+:REG_ITEM_SIZE_LEN]
              <= reg_item_size;
          REG_REC_POS_LO : begin
            s_ctrlport_resp_data <= reg_rec_pos[31:0];
            if (MEM_SIZE_W > 32) begin
              // The LO register must be read first. Save HI part now to
              // guarantee coherence when HI register is read.
              reg_rec_pos_hi <= 0;
              reg_rec_pos_hi[0 +: max(MEM_SIZE_W-32, 1)]
                <= reg_rec_pos[32 +: max(MEM_SIZE_W-32, 1)];
            end
          end
          REG_REC_POS_HI :
            if (MEM_SIZE_W > 32) begin
              // Return the saved value to guarantee coherence
              s_ctrlport_resp_data <= reg_rec_pos_hi;
            end
          REG_PLAY_POS_LO : begin
            s_ctrlport_resp_data <= reg_play_pos[31:0];
            if (MEM_SIZE_W > 32) begin
              // The LO register must be read first. Save HI part now to
              // guarantee coherence when HI register is read.
              reg_play_pos_hi <= 0;
              reg_play_pos_hi[0 +: max(MEM_SIZE_W-32, 1)]
                <= reg_play_pos[32 +: max(MEM_SIZE_W-32, 1)];
            end
          end
          REG_PLAY_POS_HI :
            if (MEM_SIZE_W > 32) begin
              // Return the saved value to guarantee coherence
              s_ctrlport_resp_data <= reg_play_pos_hi;
            end
          REG_PLAY_CMD_FIFO_SPACE :
            s_ctrlport_resp_data[5:0] <= reg_cmd_fifo_space;
        endcase
      end

      //-----------------------------------------
      // Register Writes
      //-----------------------------------------

      if (s_ctrlport_req_wr) begin
        s_ctrlport_resp_ack  <= 1;
        case (s_ctrlport_req_addr)
          REG_REC_BASE_ADDR_LO :
            reg_rec_base_addr[min(32, MEM_ADDR_W)-1:0]
              <= s_ctrlport_req_data;
          REG_REC_BASE_ADDR_HI :
            if (MEM_ADDR_W > 32)
              reg_rec_base_addr[32 +: max(MEM_ADDR_W-32, 1)]
                <= s_ctrlport_req_data[0 +: max(MEM_ADDR_W-32, 1)];
          REG_REC_BUFFER_SIZE_LO :
            reg_rec_buffer_size[min(32, MEM_SIZE_W)-1:0]
              <= s_ctrlport_req_data;
          REG_REC_BUFFER_SIZE_HI :
            if (MEM_SIZE_W > 32)
              reg_rec_buffer_size[32 +: max(MEM_SIZE_W-32, 1)]
                <= s_ctrlport_req_data[0 +: max(MEM_SIZE_W-32, 1)];
          REG_REC_RESTART :
            rec_restart <= 1'b1;
          REG_PLAY_BASE_ADDR_LO :
            reg_play_base_addr[min(32, MEM_ADDR_W)-1:0]
              <= s_ctrlport_req_data;
          REG_PLAY_BASE_ADDR_HI :
            if (MEM_ADDR_W > 32)
              reg_play_base_addr[32 +: max(MEM_ADDR_W-32, 1)]
                <= s_ctrlport_req_data[0 +: max(MEM_ADDR_W-32, 1)];
          REG_PLAY_BUFFER_SIZE_LO :
            reg_play_buffer_size[min(32, MEM_SIZE_W)-1:0]
              <= s_ctrlport_req_data;
          REG_PLAY_BUFFER_SIZE_HI :
            if (MEM_SIZE_W > 32)
              reg_play_buffer_size[32 +: max(MEM_SIZE_W-32, 1)]
                <= s_ctrlport_req_data[0 +: max(MEM_SIZE_W-32, 1)];
          REG_PLAY_CMD_NUM_WORDS_LO :
            reg_play_cmd_num_words[31:0] <= s_ctrlport_req_data;
          REG_PLAY_CMD_NUM_WORDS_HI :
              reg_play_cmd_num_words[63:32] <= s_ctrlport_req_data;
          REG_PLAY_CMD_TIME_LO :
            reg_play_cmd_time[31:0] <= s_ctrlport_req_data;
          REG_PLAY_CMD_TIME_HI :
            reg_play_cmd_time[63:32] <= s_ctrlport_req_data;
          REG_PLAY_CMD : begin
            reg_play_cmd       <= s_ctrlport_req_data[REG_PLAY_CMD_POS+:REG_PLAY_CMD_LEN];
            reg_play_cmd_timed <= s_ctrlport_req_data[REG_PLAY_TIMED_POS];
            reg_play_cmd_valid <= 1'b1;
            if (!play_cmd_stop && s_ctrlport_req_data[REG_PLAY_CMD_LEN-1:0] == PLAY_CMD_STOP) begin
              play_cmd_stop  <= 1;
              clear_cmd_fifo <= 1;
            end
          end
          REG_PLAY_WORDS_PER_PKT :
            reg_play_words_per_pkt <= s_ctrlport_req_data[WPP_W-1:0];
          REG_PLAY_ITEM_SIZE :
            reg_item_size <= s_ctrlport_req_data[REG_ITEM_SIZE_POS+:REG_ITEM_SIZE_LEN];
        endcase
      end

      // Compute the amount by which to increment time for each memory word, as
      // indicated by reg_item_size.
      (* parallel_case *)
      casex (reg_item_size)
        8'bxxxxxxx1: items_per_word <= (MEM_DATA_W/8) >> 0;
        8'bxxxxxx1x: items_per_word <= (MEM_DATA_W/8) >> 1;
        8'bxxxxx1xx: items_per_word <= (MEM_DATA_W/8) >> 2;
        8'bxxxx1xxx: items_per_word <= (MEM_DATA_W/8) >> 3;
        8'bxxx1xxxx: items_per_word <= (MEM_DATA_W/8) >> 4;
        8'bxx1xxxxx: items_per_word <= (MEM_DATA_W/8) >> 5;
        8'bx1xxxxxx: items_per_word <= (MEM_DATA_W/8) >> 6;
        8'b1xxxxxxx: items_per_word <= (MEM_DATA_W/8) >> 7;
      endcase

    end
  end


  //---------------------------------------------------------------------------
  // Playback Command FIFO
  //---------------------------------------------------------------------------
  //
  // This block queues up commands for playback.
  //
  //---------------------------------------------------------------------------

  // Command FIFO Signals
  wire      [CMD_W-1:0]  cmd_cf;
  wire                   cmd_timed_cf;
  wire [NUM_WORDS_W-1:0] cmd_num_words_cf;
  wire      [TIME_W-1:0] cmd_time_cf;
  wire  [MEM_ADDR_W-1:0] cmd_base_addr_cf;
  wire  [MEM_SIZE_W-1:0] cmd_buffer_size_cf;
  wire                   cmd_fifo_valid;
  reg                    cmd_fifo_ready;

  axi_fifo_short #(
    .WIDTH (MEM_ADDR_W + MEM_SIZE_W + 1 + CMD_W + NUM_WORDS_W + TIME_W)
  ) command_fifo (
    .clk      (clk),
    .reset    (rst),
    .clear    (clear_cmd_fifo),
    .i_tdata  ({play_base_addr_sr, play_buffer_size_sr, reg_play_cmd_timed, reg_play_cmd, reg_play_cmd_num_words, reg_play_cmd_time}),
    .i_tvalid (reg_play_cmd_valid),
    .i_tready (reg_play_cmd_ready),
    .o_tdata  ({cmd_base_addr_cf, cmd_buffer_size_cf, cmd_timed_cf, cmd_cf, cmd_num_words_cf, cmd_time_cf}),
    .o_tvalid (cmd_fifo_valid),
    .o_tready (cmd_fifo_ready),
    .occupied (),
    .space    (reg_cmd_fifo_space)
  );


  //---------------------------------------------------------------------------
  // Record Input Data FIFO
  //---------------------------------------------------------------------------
  //
  // This FIFO stores data to be recorded into the external memory.
  //
  //---------------------------------------------------------------------------

  axi_fifo #(
    .WIDTH (MEM_DATA_W),
    .SIZE  (REC_FIFO_ADDR_WIDTH)
  ) rec_axi_fifo (
    .clk      (clk),
    .reset    (rst),
    .clear    (1'b0),
    //
    .i_tdata  (i_tdata),
    .i_tvalid (i_tvalid),
    .i_tready (i_tready),
    //
    .o_tdata  (rec_fifo_o_tdata),
    .o_tvalid (rec_fifo_o_tvalid),
    .o_tready (rec_fifo_o_tready),
    //
    .space    (),
    .occupied (rec_fifo_occupied)
  );


  //---------------------------------------------------------------------------
  // Record State Machine
  //---------------------------------------------------------------------------

  // FSM States
  localparam REC_WAIT_FIFO       = 0;
  localparam REC_CHECK_ALIGN     = 1;
  localparam REC_MEM_REQ         = 2;
  localparam REC_WAIT_MEM_START  = 3;
  localparam REC_WAIT_MEM_COMMIT = 4;

  // State Signals
  reg [2:0] rec_state;

  // Registers
  reg [MEM_SIZE_W-1:0] rec_buffer_size; // Last buffer size pulled from register
  reg [MEM_ADDR_W-1:0] rec_addr;        // Current offset into record buffer
  reg [MEM_ADDR_W-1:0] rec_size;        // Number of words to transfer next
  reg [MEM_ADDR_W-1:0] rec_size_0;      // Pipeline stage for computation of rec_size

  // Buffer usage registers
  reg [MEM_SIZE_W-1:0] rec_buffer_avail;  // Amount of free buffer space in words
  reg [MEM_SIZE_W-1:0] rec_buffer_used;   // Amount of occupied buffer space in words

  reg [MEM_SIZE_W-1:0] rec_size_aligned;  // Max record size until the next 4k boundary

  // Timer to count how many cycles we've been waiting for new data
  reg [$clog2(DATA_WAIT_TIMEOUT+1)-1:0] rec_wait_timer;
  reg                                   rec_wait_timeout;

  assign reg_rec_fullness = rec_buffer_used * BYTES_PER_WORD;
  assign reg_rec_pos = rec_addr;

  always @(posedge clk) begin
    if (rst) begin
      rec_state        <= REC_WAIT_FIFO;
      write_ctrl_valid <= 1'b0;
      rec_wait_timer   <= 0;
      rec_wait_timeout <= 0;
      rec_buffer_avail <= 0;
      rec_buffer_used  <= 0;

      // Don't care:
      rec_addr    <= {MEM_ADDR_W{1'b0}};
      rec_size_0  <= {MEM_ADDR_W{1'bX}};
      rec_size    <= {MEM_ADDR_W{1'bX}};
      write_count <= {MEM_COUNT_W{1'bX}};
      write_addr  <= {MEM_ADDR_W{1'bX}};

    end else begin

      // Default assignments
      rec_restart_clear <= 1'b0;

      // Update wait timer
      if ((i_tvalid && i_tready) || !rec_fifo_occupied) begin
        // If a new word is presented to the input FIFO, or the FIFO is empty,
        // then reset the timer.
        rec_wait_timer   <= 0;
        rec_wait_timeout <= 1'b0;
      end else if (rec_fifo_occupied) begin
        // If no new word is written, but there's data in the FIFO, update the
        // timer. Latch timeout condition when we reach our limit.
        rec_wait_timer <= rec_wait_timer + 1;

        if (rec_wait_timer == DATA_WAIT_TIMEOUT) begin
          rec_wait_timeout <= 1'b1;
        end
      end

      // Pre-calculate the aligned size in words
      rec_size_aligned <= AXI_ALIGNMENT - ((rec_addr/BYTES_PER_WORD) & (AXI_ALIGNMENT-1));

      //
      // State logic
      //
      case (rec_state)

        REC_WAIT_FIFO : begin
          // Wait until there's enough data to initiate a transfer from the
          // FIFO to the RAM.

          // Check if a restart was requested on the record interface
          if (rec_restart) begin
            rec_restart_clear <= 1'b1;

            // Latch the new register values. We don't want them to change
            // while we're running.
            rec_buffer_size <= rec_buffer_size_sr / BYTES_PER_WORD;   // Store size in words

            // Reset counters and address any time we update the buffer size or
            // base address.
            rec_buffer_avail <= rec_buffer_size_sr / BYTES_PER_WORD;  // Store size in words
            rec_buffer_used  <= 0;
            rec_addr         <= rec_base_addr_sr;

          // Check if there's room left in the record RAM buffer
          end else if (rec_buffer_used < rec_buffer_size) begin
            // See if we can transfer a full burst
            if (rec_fifo_occupied >= MEM_BURST_LEN && rec_buffer_avail >= MEM_BURST_LEN) begin
              rec_size_0 <= MEM_BURST_LEN;
              rec_state  <= REC_CHECK_ALIGN;

            // Otherwise, if we've been waiting a long time, see if we can
            // transfer less than a burst.
            end else if (rec_fifo_occupied > 0 && rec_wait_timeout) begin
              rec_size_0 <= (rec_fifo_occupied <= rec_buffer_avail) ?
                            rec_fifo_occupied : rec_buffer_avail;
              rec_state  <= REC_CHECK_ALIGN;
            end
          end
        end

        REC_CHECK_ALIGN : begin
          // Check the address alignment, since AXI requires that an access not
          // cross 4k boundaries (boo), and the memory interface doesn't handle
          // this automatically (boo again).
          rec_size <= rec_size_0 > rec_size_aligned ?
                      rec_size_aligned : rec_size_0;

          // Memory interface is ready, so transaction will begin
          rec_state <= REC_MEM_REQ;
        end

        REC_MEM_REQ : begin
          // The write count written to the memory interface should be 1 less
          // than the number of words you want to write (not the number of
          // bytes).
          write_count <= rec_size - 1;

          // Create the physical RAM byte address by combining the address and
          // base address.
          write_addr <= rec_addr;

          // Once the interface is ready, make the memory request
          if (write_ctrl_ready) begin
            // Request the write transaction
            write_ctrl_valid <= 1'b1;
            rec_state        <= REC_WAIT_MEM_START;
          end
        end

        REC_WAIT_MEM_START : begin
          // Wait until memory interface deasserts ready, indicating it has
          // started on the request.
          write_ctrl_valid <= 1'b0;
          if (!write_ctrl_ready) begin
            rec_state <= REC_WAIT_MEM_COMMIT;
          end
        end

        REC_WAIT_MEM_COMMIT : begin
          // Wait for the memory interface to reassert write_ctrl_ready, which
          // signals that the interface has received a response for the whole
          // write transaction and (we assume) it has been committed to RAM.
          // After this, we can update the write address and start the next
          // transaction.
          if (write_ctrl_ready) begin
             rec_addr         <= rec_addr + (rec_size * BYTES_PER_WORD);
             rec_buffer_used  <= rec_buffer_used + rec_size;
             rec_buffer_avail <= rec_buffer_avail - rec_size;
             rec_state        <= REC_WAIT_FIFO;
          end
        end

        default : begin
          rec_state <= REC_WAIT_FIFO;
        end

      endcase
    end
  end

  // Connect output of record FIFO to input of the memory write interface
  assign write_data        = rec_fifo_o_tdata;
  assign write_data_valid  = rec_fifo_o_tvalid;
  assign rec_fifo_o_tready = write_data_ready;


  //---------------------------------------------------------------------------
  // Playback State Machine
  //---------------------------------------------------------------------------

  // FSM States
  localparam PLAY_IDLE            = 0;
  localparam PLAY_CHECK_SIZES     = 1;
  localparam PLAY_WAIT_DATA_READY = 2;
  localparam PLAY_CHECK_ALIGN     = 3;
  localparam PLAY_SIZE_CALC       = 4;
  localparam PLAY_MEM_REQ         = 5;
  localparam PLAY_WAIT_MEM_START  = 6;
  localparam PLAY_WAIT_MEM_COMMIT = 7;
  localparam PLAY_DONE_CHECK      = 8;

  // State Signals
  reg [3:0] play_state;

  // Registers
  reg [MEM_ADDR_W-1:0] play_addr;         // Current byte offset into record buffer
  reg [  MEM_ADDR_W:0] play_addr_0;       // Pipeline stage for computing play_addr.
                                          // One bit larger to detect address wrapping.
  reg [MEM_ADDR_W-1:0] play_addr_1;       // Pipeline stage for computing play_addr
  reg [MEM_SIZE_W-1:0] play_buffer_end;   // Address of location after end of buffer
  reg [MEM_ADDR_W-1:0] max_read_size;     // Maximum size of next transfer, in words
  reg [MEM_ADDR_W-1:0] next_read_size;    // Actual size of next transfer, in words
  reg [MEM_ADDR_W-1:0] play_size_aligned; // Max play size until the next 4K boundary
  //
  reg [NUM_WORDS_W-1:0] play_words_remaining; // Number of words left for playback command
  reg       [CMD_W-1:0] cmd;                  // Copy of cmd_cf from last command
  reg  [MEM_ADDR_W-1:0] cmd_base_addr;        // Copy of cmd_base_addr_cf from last command
  reg  [MEM_SIZE_W-1:0] cmd_buffer_size;      // Copy of cmd_buffer_size_cf from last command
  reg                   last_trans;           // Is this the last read transaction for the command?

  reg play_full_burst_avail;      // True if we there's a full burst to read
  reg next_read_size_ok;          // True if it's OK to read next_read_size
  reg play_buffer_zero;           // True if play buffer size is zero
  reg num_words_zero;             // True if number of words to play is zero

  reg [MEM_ADDR_W-1:0] next_read_size_m1;       // next_read_size - 1
  reg [MEM_ADDR_W-1:0] play_words_remaining_m1; // play_words_remaining - 1

  reg [MEM_SIZE_W-1:0] play_buffer_avail;   // Number of words left to read in record buffer
  reg [MEM_SIZE_W-1:0] play_buffer_avail_0; // Pipeline stage for computing play_buffer_avail

  reg pause_data_transfer;

  assign reg_play_pos = play_addr;

  always @(posedge clk)
  begin
    if (rst) begin
      play_state     <= PLAY_IDLE;
      cmd_fifo_ready <= 1'b0;
      play_addr      <= {MEM_ADDR_W{1'b0}};
      last_trans     <= 1'b0;

      // Don't care:
      play_full_burst_avail     <= 1'bX;
      play_buffer_end           <= {MEM_SIZE_W{1'bX}};
      read_ctrl_valid           <= 1'bX;
      cmd                       <= {CMD_W{1'bX}};
      cmd_base_addr             <= {MEM_ADDR_W{1'bX}};
      cmd_buffer_size           <= {MEM_SIZE_W{1'bX}};
      play_buffer_avail         <= {MEM_SIZE_W{1'bX}};
      play_size_aligned         <= {MEM_SIZE_W{1'bX}};
      play_words_remaining      <= {NUM_WORDS_W{1'bX}};
      max_read_size             <= {MEM_ADDR_W{1'bX}};
      next_read_size            <= {MEM_ADDR_W{1'bX}};
      play_words_remaining_m1   <= {MEM_ADDR_W{1'bX}};
      next_read_size_m1         <= {MEM_ADDR_W{1'bX}};
      next_read_size_ok         <= 1'bX;
      read_count                <= {MEM_COUNT_W{1'bX}};
      read_addr                 <= {MEM_ADDR_W{1'bX}};
      play_addr_0               <= {MEM_ADDR_W+1{1'bX}};
      play_buffer_avail_0       <= {MEM_SIZE_W{1'bX}};
      play_addr_1               <= {MEM_ADDR_W{1'bX}};
      play_buffer_zero          <= 1'bX;
      num_words_zero            <= 1'bX;

    end else begin

      // Calculate how many words are left to read from the record buffer
      play_full_burst_avail     <= (play_buffer_avail >= MEM_BURST_LEN);

      play_size_aligned <= AXI_ALIGNMENT - ((play_addr/BYTES_PER_WORD) & (AXI_ALIGNMENT-1));

      // Default values
      cmd_fifo_ready    <= 1'b0;
      read_ctrl_valid   <= 1'b0;
      play_cmd_stop_ack <= 1'b0;

      //
      // State logic
      //
      case (play_state)
        PLAY_IDLE : begin
          // Save needed command info
          cmd             <= cmd_cf;
          cmd_base_addr   <= cmd_base_addr_cf;
          cmd_buffer_size <= cmd_buffer_size_cf  / BYTES_PER_WORD;

          // Initialize the play variables
          if (cmd_cf == PLAY_CMD_CONTINUOUS) begin
            play_words_remaining <= MEM_BURST_LEN;
            num_words_zero <= 0;
          end else begin
            play_words_remaining <= cmd_num_words_cf;
            num_words_zero <= (cmd_num_words_cf == 0);
          end
          play_buffer_avail     <= cmd_buffer_size_cf / BYTES_PER_WORD;
          play_buffer_end       <= {1'b0, cmd_base_addr_cf} + cmd_buffer_size_cf;
          play_buffer_zero      <= (cmd_buffer_size_cf == 0);

          // Wait until we receive a command
          if (play_cmd_stop) begin
            play_cmd_stop_ack <= 1'b1;
          end else if (cmd_fifo_valid) begin
            // Only update the play address when valid so readback is accurate
            play_addr <= cmd_base_addr_cf;

            // Dequeue the command from the FIFO
            cmd_fifo_ready <= 1'b1;

            play_state <= PLAY_CHECK_SIZES;
          end
        end

        PLAY_CHECK_SIZES : begin
          // Check buffer and num_word sizes and allow propagation of
          // play_full_burst_avail.
          if (play_buffer_zero | num_words_zero) begin
            play_state <= PLAY_IDLE;
          end else begin
            play_state <= PLAY_WAIT_DATA_READY;
          end
        end

        PLAY_WAIT_DATA_READY : begin
          // Save the maximum size we can read from RAM
          max_read_size <= play_full_burst_avail ? MEM_BURST_LEN : play_buffer_avail;

          // Wait for output FIFO to empty sufficiently so we can read an
          // entire burst at once. This may be more space than needed, but we
          // won't know the exact size until the next state.
          if (play_fifo_space >= MEM_BURST_LEN) begin
            play_state <= PLAY_CHECK_ALIGN;
          end
        end

        PLAY_CHECK_ALIGN : begin
          // Check the address alignment, since AXI requires that an access not
          // cross 4k boundaries (boo), and the memory interface doesn't handle
          // this automatically (boo again).
          next_read_size <= max_read_size > play_size_aligned ?
                            play_size_aligned : max_read_size;
          play_state <= PLAY_SIZE_CALC;
        end

        PLAY_SIZE_CALC : begin
          // Do some intermediate calculations to determine what the read_count
          // should be.
          play_words_remaining_m1 <= play_words_remaining-1;
          next_read_size_m1       <= next_read_size-1;
          next_read_size_ok       <= play_words_remaining >= next_read_size;
          play_state              <= PLAY_MEM_REQ;

          // Check if this is the last memory transaction
          if (play_cmd_stop) begin
            last_trans        <= 1'b1;
            play_cmd_stop_ack <= 1'b1;
          end else if (cmd == PLAY_CMD_CONTINUOUS) begin
            last_trans <= 1'b0;
          end else begin
            // If not stopping, see if this is the last transaction for a
            // finite playback command.
            last_trans <= (play_words_remaining <= next_read_size);
          end
        end

        PLAY_MEM_REQ : begin
          // Load the size of the next read into a register. We try to read the
          // max amount available (up to the burst size) or however many words
          // are needed to reach the end of the RAM buffer.
          //
          // The read count written to the memory interface should be 1 less
          // than the number of words you want to read (not the number of
          // bytes).
          read_count <= next_read_size_ok ? next_read_size_m1 : play_words_remaining_m1;

          // Load the address to read
          read_addr <= play_addr;

          // Request the read transaction as soon as memory interface is ready
          if (read_ctrl_ready) begin
            read_ctrl_valid <= 1'b1;
            play_state      <= PLAY_WAIT_MEM_START;
          end
        end

        PLAY_WAIT_MEM_START : begin
          // Wait until memory interface deasserts ready, indicating it has
          // started on the request.
          read_ctrl_valid <= 1'b0;
          if (!read_ctrl_ready) begin
            // Update values for next transaction
            play_addr_0 <= play_addr +
              ({{(MEM_ADDR_W-MEM_COUNT_W){1'b0}}, read_count} + 1) * BYTES_PER_WORD;
            play_words_remaining <= play_words_remaining - ({1'b0, read_count} + 1);
            play_buffer_avail_0  <= play_buffer_avail - ({1'b0, read_count} + 1);

            play_state <= PLAY_WAIT_MEM_COMMIT;
          end
        end

        PLAY_WAIT_MEM_COMMIT : begin
          // Wait for the memory interface to reassert read_ctrl_ready, which
          // signals that the interface has received a response for the whole
          // read transaction.
          if (read_ctrl_ready) begin
            // Check if this is the last transaction.
            if (last_trans) begin
              play_addr_1       <= play_addr_0[MEM_ADDR_W-1:0];
              play_buffer_avail <= 0;

            // Check if we need to wrap the address for the next transaction.
            end else if (play_addr_0 >= play_buffer_end) begin
              play_addr_1       <= cmd_base_addr;
              play_buffer_avail <= cmd_buffer_size;

            end else begin
              play_addr_1       <= play_addr_0[MEM_ADDR_W-1:0];
              play_buffer_avail <= play_buffer_avail_0;
            end

            play_state <= PLAY_DONE_CHECK;
          end
        end

        PLAY_DONE_CHECK : begin
          play_addr <= play_addr_1;

          // Check if we have more data to transfer for this command
          if (cmd == PLAY_CMD_CONTINUOUS && !last_trans) begin
            play_words_remaining <= MEM_BURST_LEN;
            play_state           <= PLAY_WAIT_DATA_READY;
          end else if (play_words_remaining && !last_trans) begin
            play_state <= PLAY_WAIT_DATA_READY;
          end else begin
            play_state <= PLAY_IDLE;
          end
        end
      endcase

    end
  end


  //---------------------------------------------------------------------------
  // TLAST and Sideband Generation
  //---------------------------------------------------------------------------
  //
  // This section monitors the signals to/from the memory interface and
  // generates the TLAST and sideband signals. We assert TLAST at the end of
  // every reg_play_words_per_pkt words and at the end of the last packet, so
  // that no packets are longer than the length indicated by the
  // REG_PLAY_WORDS_PER_PKT register.
  //
  // The sideband signals consist of the timestamp, has-time flag, and EOB (end
  // of burst) flag. Timestamp and has_time are set for the first packet of
  // each playback. EOB applies to each packet but is only set to 1 for the
  // last packet of a playback.
  //
  // The timing of this section relies on the fact axi_dma_master doesn't allow
  // overlapping read transactions. This means that the next read_ctrl_ready
  // won't be asserted until after previous memory transaction finishes being
  // read out.
  //
  //---------------------------------------------------------------------------

  reg [MEM_COUNT_W-1:0] read_counter;      // Track outstanding words to read
  reg [      WPP_W-1:0] length_counter;    // Track packet length
  reg [     TIME_W-1:0] timestamp;         // Timestamp for the current burst
  reg                   has_time;          // Is current burst timed?
  reg                   eob;               // End of burst
  reg                   play_fifo_i_tlast; // End of packet

  always @(posedge clk)
  begin
    // synthesis translate_off
    //
    // Check our assumption about non-overlapping read transactions.
    if (read_ctrl_ready && play_fifo_i_tvalid) begin
      $fatal(1, "New read transaction started before the previous one completed!");
    end
    // synthesis translate_on

    if (read_ctrl_valid && read_ctrl_ready) begin
      read_counter <= read_count;

      // If read_count is 0, then the next word is also the last word
      if (read_count == 0) begin
        play_fifo_i_tlast <= 1'b1;
        eob               <= last_trans;
      end
    end

    if (play_fifo_i_tvalid && play_fifo_i_tready) begin
      read_counter   <= read_counter - 1;
      length_counter <= length_counter - 1;

      // Check if the current word is the last of the packet
      if (play_fifo_i_tlast) begin
        length_counter <= reg_play_words_per_pkt;

        // Clear tlast, unless the first word of the next packet is also the
        // last word of the next packet.
        if (!(last_trans && read_counter == 1)) begin
          play_fifo_i_tlast <= 1'b0;
        end

        // The timestamp only applies to the first packet, so disable for
        // subsequent packets.
        has_time <= 1'b0;
      end

      // Check if the next word will be the last of the packet.
      //
      // First, check if the next word is the last word of playback, in which
      // case it's both the last word of the packet and the end of the burst.
      if (last_trans && read_counter == 1) begin
        play_fifo_i_tlast <= 1'b1;
        eob <= 1;

      // Next, check if this is the last word of the packet according to packet
      // length. But note that the next word won't be the last if we're already
      // outputting the last word of a burst on the current cycle.
      end else if (length_counter == 2 && !(eob && play_fifo_i_tlast)) begin
        play_fifo_i_tlast <= 1'b1;
      end
    end

    if (play_state == PLAY_IDLE) begin
      // Reset signals for the next playback
      length_counter    <= reg_play_words_per_pkt;
      timestamp         <= cmd_time_cf;
      has_time          <= cmd_timed_cf;
      eob               <= 0;
      play_fifo_i_tlast <= 1'b0;
    end
  end


  //---------------------------------------------------------------------------
  // Playback Output Data FIFO
  //---------------------------------------------------------------------------
  //
  // The play_axi_fifo buffers data that has been read out of RAM as part of a
  // playback operation.
  //
  //---------------------------------------------------------------------------

  // Connect output of memory read interface to play_axi_fifo
  assign play_fifo_i_tdata  = read_data;
  assign play_fifo_i_tvalid = read_data_valid    & ~pause_data_transfer;
  assign read_data_ready    = play_fifo_i_tready & ~pause_data_transfer;

  axi_fifo #(
    .WIDTH (MEM_DATA_W+1),
    .SIZE  (PLAY_FIFO_ADDR_WIDTH)
  ) play_axi_fifo (
    .clk      (clk),
    .reset    (rst),
    .clear    (1'b0),
    //
    .i_tdata  ({play_fifo_i_tlast, play_fifo_i_tdata}),
    .i_tvalid (play_fifo_i_tvalid),
    .i_tready (play_fifo_i_tready),
    //
    .o_tdata  ({o_tlast, o_tdata}),
    .o_tvalid (o_tvalid),
    .o_tready (o_tready),
    //
    .space    (play_fifo_space),
    .occupied ()
  );


  //---------------------------------------------------------------------------
  // Header Info FIFO
  //---------------------------------------------------------------------------
  //
  // The hdr_axi_fifo contains the header information for the next packet, with
  // one word per packet.
  //
  //---------------------------------------------------------------------------

  wire [(TIME_W+2)-1:0] hdr_fifo_i_tdata;
  wire                  hdr_fifo_i_tvalid;
  wire [(TIME_W+2)-1:0] hdr_fifo_o_tdata;
  wire                  hdr_fifo_o_tvalid;
  wire                  hdr_fifo_o_tready;

  wire [15:0] hdr_fifo_space;

  axi_fifo #(
    .WIDTH (TIME_W+2),
    .SIZE  (HDR_FIFO_ADDR_WIDTH)
  ) hdr_axi_fifo (
    .clk      (clk),
    .reset    (rst),
    .clear    (1'b0),
    //
    .i_tdata  (hdr_fifo_i_tdata),
    .i_tvalid (hdr_fifo_i_tvalid),
    .i_tready (),
    //
    .o_tdata  (hdr_fifo_o_tdata),
    .o_tvalid (hdr_fifo_o_tvalid),
    .o_tready (hdr_fifo_o_tready),
    //
    .space    (hdr_fifo_space),
    .occupied ()
  );

  // synthesis translate_off
  //
  // The FIFO code above assumes the header info will always be available when
  // the last word of the payload FIFO is read out. Check that assumption here.
  always @(posedge clk) begin
    if (hdr_fifo_o_tready && !hdr_fifo_o_tvalid) begin
      $fatal(1, "Header FIFO read without valid data!");
    end
  end
  // synthesis translate_on

  assign hdr_fifo_i_tdata = {has_time, eob, timestamp };

  // Pop the timestamp whenever we finish reading out a data packet
  assign hdr_fifo_o_tready = o_tvalid & o_tready & o_tlast;

  // Write the timestamp at the start of each packet
  assign hdr_fifo_i_tvalid = play_fifo_i_tvalid & play_fifo_i_tready & play_fifo_i_tlast;

  assign { o_thas_time, o_teob, o_ttimestamp } = hdr_fifo_o_tdata;


  // The following state machine prevents overflow of the hdr_axi_fifo by
  // stopping data transfer if it is almost full. It monitors the state of the
  // current transfer so as to not violate the AXI-Stream protocol.
  reg hdr_fifo_almost_full;

  always @(posedge clk) begin
    if (rst) begin
      hdr_fifo_almost_full <= 0;
      pause_data_transfer  <= 0;
    end else begin
      hdr_fifo_almost_full <= (hdr_fifo_space < 4);

      if (pause_data_transfer) begin
        if (!hdr_fifo_almost_full) pause_data_transfer <= 0;
      end else begin
        // If we're not asserting tvalid, or we're completing a transfer this
        // cycle, then it is safe to gate tvalid on the next cycle.
        if (hdr_fifo_almost_full &&
           (!play_fifo_i_tvalid || (play_fifo_i_tvalid && play_fifo_i_tready))) begin
          pause_data_transfer <= 1;
        end
      end
    end
  end

endmodule


`default_nettype wire

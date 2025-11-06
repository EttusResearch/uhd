//
// Copyright 2024 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: fft_reorder
//
// Description:
//
//   This module optionally rearranges the order of FFT bins to put them in the
//   desired order. It also supports cyclic prefix insertion.
//
//   The input order that this module receives is a parameter that must be
//   chosen at compile time. The following input orders are supported:
//
//     NATURAL:     Positive frequencies are input first, starting with 0 Hz,
//                  followed by negative frequencies. Frequencies are input in
//                  ascending order.
//     BIT_REVERSE: Like natural, but the bits of the indices are in reverse
//                  order. For example, for a size 16 FFT, bin 0000 is input
//                  first, followed by bin 1000, 0100, 1100, 0010, etc.
//
//   The output order can be chosen at run time. The following output orders
//   are supported:
//
//     NORMAL:      Negative frequencies first, then positive frequencies. 0 Hz
//                  is in the center. Frequencies are output in ascending order.
//     REVERSE:     Reverse order of NORMAL. Positive frequencies first, then
//                  negative frequencies. 0 Hz in the center. Frequencies are
//                  output in descending order.
//     NATURAL:     Positive frequencies are first, starting with 0 Hz,
//                  followed by negative frequencies. Frequencies are output in
//                  ascending order.
//     BIT_REVERSE: Like natural, but the bits of the indices are in reverse
//                  order. For example, for a size 16 FFT, bin 0000 is output
//                  first, followed by bin 1000, 0100, 1100, 0010, etc.
//
//   Typically the FFT IP feeding this module will output data in BIT_REVERSE
//   order. The FFT IP may have an option to rearrange the data into NATURAL
//   order but enabling this feature causes a large memory to be added to the
//   IP to do the reordering. Since we want to also be able to provide NORMAL
//   order, and we don't want to add a second memory for that reordering, we do
//   all the reordering here in one memory.
//
//   If the FFT core is outputting in the order you want, then this module
//   should probably be removed to save RAM and logic.
//
//   The TLAST input/output corresponds to when the FFT input/output ends for a
//   single FFT-sized sequence of data. i_tlast must be asserted during the
//   last transfer of the input FFT to reset things for the next FFT input.
//
//   For cyclic prefix insertion, the EN_CP_INSERTION parameter must be true
//   and i_tuser contains the cyclic prefix size to insert. It must be valid
//   during the first transfer of the packet. It can be any size from 0 to
//   2**MAX_FFT_LEN_LOG2-1.
//
// Parameters:
//
//   IN_FIFO_LOG2     : Log base-2 of the input FIFO size. Set to -1 to remove
//                      the input FIFO. This FIFO is intended as a pipeline
//                      stage to cut the timing path on the input.
//   OUT_FIFO_LOG2    : Log base-2 of the output FIFO size. This must be set to
//                      at least 3.
//   INPUT_ORDER      : BIT_REVERSE or NATURAL. See fft_reorder_pkg for values.
//   MAX_FFT_LEN_LOG2 : Ceiling of log base-2 of the maximum FFT size to be
//                      supported.
//   DATA_W           : Data width. Typically 32 for sc16 data type.
//   EN_CP_INSERTION  : Controls whether or not the CP insertion logic is
//                      included.
//
// Signals:
//
//   i_t* : AXI-Stream data input. Each packet is one FFT to be processed. The
//          length of the packet must match the FFT size. i_tuser contains the
//          cyclic prefix size to insert for this packet and must be valid
//          during the first transfer of the packet.
//   o_t* : AXI-Stream data output. Each packet is one FFT with optional cyclic
//          prefix.
//

`default_nettype none


module fft_reorder
  import fft_reorder_pkg::*;
#(
  parameter int         IN_FIFO_LOG2     = 1,
  parameter int         OUT_FIFO_LOG2    = 3,
  parameter fft_order_t INPUT_ORDER      = BIT_REVERSE,
  parameter int         MAX_FFT_LEN_LOG2 = 12,
  parameter int         DATA_W           = 32,
  parameter bit         EN_CP_INSERTION  = 1,

  localparam int FFT_LEN_LOG2_W = $clog2(MAX_FFT_LEN_LOG2+1),
  localparam int CP_LEN_W       = MAX_FFT_LEN_LOG2
) (
  input wire clk,
  input wire rst,

  input wire                      fft_cfg_wr,
  input wire [FFT_LEN_LOG2_W-1:0] fft_len_log2,
  input fft_order_t               fft_out_order,

  // Data Input
  input  wire [  DATA_W-1:0] i_tdata,
  input  wire [CP_LEN_W-1:0] i_tuser,
  input  wire                i_tlast,
  input  wire                i_tvalid,
  output wire                i_tready,

  // Data Output
  output wire [DATA_W-1:0] o_tdata,
  output wire              o_tlast,
  output wire              o_tvalid,
  input  wire              o_tready
);

  // These registers track if the current read/write buffers are OK to use
  logic ok_to_write = 1'b1;  // Current write buffer is free for writes
  logic ok_to_read  = 1'b0;  // Current read buffer has data to read


  //---------------------------------------------------------------------------
  // Optional Data Input Pipeline
  //---------------------------------------------------------------------------

  logic [  DATA_W-1:0] in_fifo_o_tdata;
  logic [CP_LEN_W-1:0] in_fifo_o_tuser;
  logic                in_fifo_o_tvalid;
  logic                in_fifo_o_tready;
  logic                in_fifo_o_tlast;

  if (IN_FIFO_LOG2 >= 0) begin : gen_in_fifo
    axi_fifo #(
      .WIDTH(1 + CP_LEN_W + DATA_W),
      .SIZE (IN_FIFO_LOG2)
    ) axi_fifo_in (
      .clk     (clk),
      .reset   (rst),
      .clear   ('0),
      .i_tdata ({i_tlast, i_tuser, i_tdata}),
      .i_tvalid(i_tvalid),
      .i_tready(i_tready),
      .o_tdata ({in_fifo_o_tlast, in_fifo_o_tuser, in_fifo_o_tdata}),
      .o_tvalid(in_fifo_o_tvalid),
      .o_tready(in_fifo_o_tready),
      .space   (),
      .occupied()
    );
  end else begin : gen_no_in_fifo
    assign in_fifo_o_tdata  = i_tdata;
    assign in_fifo_o_tuser  = i_tuser;
    assign in_fifo_o_tlast  = i_tlast;
    assign in_fifo_o_tvalid = i_tvalid;
    assign i_tready         = in_fifo_o_tready;
  end


  //---------------------------------------------------------------------------
  // Optional Data Output Pipeline
  //---------------------------------------------------------------------------

  if (OUT_FIFO_LOG2 < 3) begin
    OUT_FIFO_LOG2_must_be_at_least_3();
  end

  logic [DATA_W-1:0] out_fifo_i_tdata;
  logic              out_fifo_i_tvalid;
  logic              out_fifo_i_tlast;

  // We use out_fifo_space instead of out_fifo_i_tready to allow extra space
  // for the RAM output read delay.
  logic [15:0] out_fifo_space;

  axi_fifo #(
    .WIDTH(DATA_W+1),
    .SIZE (OUT_FIFO_LOG2)
  ) axi_fifo_out (
    .clk     (clk),
    .reset   (rst),
    .clear   ('0),
    .i_tdata ({out_fifo_i_tlast, out_fifo_i_tdata}),
    .i_tvalid(out_fifo_i_tvalid),
    .i_tready(),
    .o_tdata ({o_tlast, o_tdata}),
    .o_tvalid(o_tvalid),
    .o_tready(o_tready),
    .space   (out_fifo_space),
    .occupied()
  );


  //---------------------------------------------------------------------------
  // Configuration Registers
  //---------------------------------------------------------------------------
  //
  // Store relevant FFT configuration values in registers for use elsewhere. We
  // assume that the configuration is set in advance of any operation and is
  // only changed when the FFT is idle, so we ignore the latency here.
  //
  //---------------------------------------------------------------------------

  // Number of bits needed to represent the maximum FFT size
  localparam FFT_LEN_W = MAX_FFT_LEN_LOG2+1;

  logic                      fft_cfg_wr_stb    = 1'b0;
  fft_order_t                fft_out_order_reg = NORMAL;
  logic [FFT_LEN_LOG2_W-1:0] fft_len_log2_reg  = MAX_FFT_LEN_LOG2;
  logic [FFT_LEN_W-1:0]      fft_len           = 1 << MAX_FFT_LEN_LOG2;
  logic [FFT_LEN_W-1:0]      fft_len_m1        = (1 << MAX_FFT_LEN_LOG2)-1;

  always_ff @(posedge clk) begin
    if(rst) begin
      fft_cfg_wr_stb    <= 1'b0;
      fft_out_order_reg <= NORMAL;
      fft_len_log2_reg  <= MAX_FFT_LEN_LOG2;
      fft_len           <= 1 << MAX_FFT_LEN_LOG2;
      fft_len_m1        <= (1 << MAX_FFT_LEN_LOG2)-1;
    end else begin
      fft_cfg_wr_stb <= 1'b0;
      if (fft_cfg_wr) begin
        fft_cfg_wr_stb    <= 1'b1;
        fft_out_order_reg <= fft_out_order;
        fft_len_log2_reg  <= fft_len_log2;
        fft_len           <= (1 << fft_len_log2);
        fft_len_m1        <= (1 << fft_len_log2)-1;
      end
    end
  end


  //---------------------------------------------------------------------------
  // RAM Buffer
  //---------------------------------------------------------------------------
  //
  // This RAM stores the data that's being input, writing it the order needed
  // such that when read out sequentially, it will be in the correct order.
  //
  // The RAM is divided into two halves, which we'll call buffers. Each buffer
  // is used exclusively for read or write, until they switch.
  //
  //---------------------------------------------------------------------------

  // Address width for each buffer. Must be big enough to store the maximum
  // length FFT.
  localparam ADDR_W = MAX_FFT_LEN_LOG2;

  // RAM read latency
  localparam READ_LATENCY = 2;

  logic              ram_rd_buffer;      // Indicates which buffer is currently used for reads
  logic              ram_wr_buffer;      // Indicates which buffer is currently used for writes
  logic              ram_wr_en;
  logic              ram_wr_en_0;        // One RAM read enable for each buffer
  logic              ram_wr_en_1;
  logic [ADDR_W-1:0] ram_wr_addr;
  logic [DATA_W-1:0] ram_wr_data;
  logic              ram_rd_en;
  logic [ADDR_W-1:0] ram_rd_addr;
  logic [DATA_W-1:0] ram_rd_data_raw_0;  // One RAM read output for each buffer
  logic [DATA_W-1:0] ram_rd_data_raw_1;

  ram_2port #(
    .DWIDTH (DATA_W),
    .AWIDTH (ADDR_W),  // Make the RAM two buffers big
    .OUT_REG(1)
  ) ram_2port_0 (
    .clka (clk),
    .ena  ('1),
    .wea  (ram_wr_en_0),
    .addra(ram_wr_addr),
    .dia  (ram_wr_data),
    .doa  (),
    .clkb (clk),
    .enb  ('1),
    .web  ('0),
    .addrb(ram_rd_addr),
    .dib  ('0),
    .dob  (ram_rd_data_raw_0)
  );

  ram_2port #(
    .DWIDTH (DATA_W),
    .AWIDTH (ADDR_W),  // Make the RAM two buffers big
    .OUT_REG(1)
  ) ram_2port_1 (
    .clka (clk),
    .ena  ('1),
    .wea  (ram_wr_en_1),
    .addra(ram_wr_addr),
    .dia  (ram_wr_data),
    .doa  (),
    .clkb (clk),
    .enb  ('1),
    .web  ('0),
    .addrb(ram_rd_addr),
    .dib  ('0),
    .dob  (ram_rd_data_raw_1)
  );


  //---------------------------------------------------------------------------
  // Write Logic
  //---------------------------------------------------------------------------
  //
  // Here we write the data into the memory in a carefully controlled order
  // such that we can read it out in sequential or bit-reversed order to get
  // the order we want.
  //
  //---------------------------------------------------------------------------

  logic [FFT_LEN_W-1:0] fft_addr_mask;
  logic [FFT_LEN_W-1:0] wr_count;

  logic ram_wr_last;

  assign ram_wr_data      = in_fifo_o_tdata;
  assign ram_wr_en        = in_fifo_o_tvalid && in_fifo_o_tready;
  assign ram_wr_en_0      = ram_wr_en && (ram_wr_buffer == 1'b0);
  assign ram_wr_en_1      = ram_wr_en && (ram_wr_buffer == 1'b1);
  assign in_fifo_o_tready = ok_to_write;
  assign ram_wr_last      = in_fifo_o_tlast;

  always_ff @(posedge clk) begin
    if (fft_cfg_wr_stb || (ram_wr_en && ram_wr_last)) begin
      if (fft_out_order_reg == NATURAL) begin
        // Natural to natural. No mask needed to affect the order.
        fft_addr_mask <= '0;
        ram_wr_addr   <= '0;
      end else if (fft_out_order_reg == REVERSE) begin
        // Natural to reverse. Invert all bits except the MSB. Inverting the
        // lower bits reverses the order. Leaving the MSB unchanged ensures we
        // output positive frequencies first, then negative frequencies.
        fft_addr_mask <= fft_len_m1 >> 1; // e.g., 8'b0111_1111
        ram_wr_addr   <= fft_len_m1 >> 1;
      end else if (fft_out_order_reg == NORMAL) begin
        // Natural to normal. Invert the MSB, so that we output negative
        // frequencies first, then positive frequencies.
        fft_addr_mask <= fft_len >> 1;  // e.g., 8'b1000_0000
        ram_wr_addr   <= fft_len >> 1;
      end else begin // (fft_order_t == BIT_REVERSE)
        // Natural to bit-reverse. For this we also use natural order, and we
        // enable/disable the bit-reversal on the read side as needed.
        fft_addr_mask <= '0;
        ram_wr_addr   <= '0;
      end
    end

    if (ram_wr_en) begin
      wr_count <= wr_count+1;

      if (ram_wr_last) begin
        // Switch to the other buffer
        ram_wr_buffer <= ~ram_wr_buffer;
        wr_count <= '0;
      end else begin
        // Calculate the the next write address
        if (
          (INPUT_ORDER == BIT_REVERSE && fft_out_order_reg != BIT_REVERSE) ||
          (INPUT_ORDER == NATURAL     && fft_out_order_reg == BIT_REVERSE)
        ) begin : bit_reversed
          // If the input is bit-reversed and we're not outputting
          // bit-reversed, then we bit reverse the RAM address to convert from
          // bit-reversed to natural order. Then apply the mask to that to
          // convert from natural to the desired output order.
          ram_wr_addr <= bit_reverse(wr_count+1, fft_len_log2_reg) ^ fft_addr_mask;
        end else begin : natural
          // Apply the mask to convert from natural to to the desired output
          // order.
          ram_wr_addr <= (wr_count+1) ^ fft_addr_mask;
        end
      end
    end

    if (rst) begin
      ram_wr_buffer <= '0;
      ram_wr_addr   <= '0;
      wr_count      <= '0;
    end
  end


  //---------------------------------------------------------------------------
  // CP Insertion Length FIFO
  //---------------------------------------------------------------------------

  // Cyclic prefix logic interface signals
  logic              cp_valid;      // Indicates the CP FIFO has an output
  logic              cp_non_zero;   // Indicates the CP value is > 0
  logic [ADDR_W-1:0] cp_start_addr; // Indicates the CP RAM start address
  logic              cp_consume;    // Control to indicate we've captured the CP length output

  if (EN_CP_INSERTION) begin: gen_cp_ins_fifo
    logic [CP_LEN_W-1:0] cp_len_tdata;
    logic                cp_len_tvalid;
    logic                tmp_i_tvalid;
    logic                in_fifo_o_tfirst = '1;   // First transfer of packet

    // Create a register that indicates when the next transfer is the start of
    // a new packet.
    always_ff @(posedge clk) begin
      if (rst) begin
        in_fifo_o_tfirst <= '1;
      end else begin
        if (in_fifo_o_tvalid && in_fifo_o_tready) begin
          in_fifo_o_tfirst <= in_fifo_o_tlast;
        end
      end
    end

    // Write the first tuser word of the packet into the CP length FIFO
    assign tmp_i_tvalid = in_fifo_o_tvalid && in_fifo_o_tready && in_fifo_o_tfirst;

    // The dual RAM buffer can only hold two FFTs at a time, so we can
    // guarantee this FIFO has sufficient room and will always be ready by
    // setting its size appropriately.
    axi_fifo #(
      .WIDTH(CP_LEN_W),
      .SIZE (1)
    ) axi_fifo_cp_length (
      .clk     (clk),
      .reset   (rst),
      .clear   ('0),
      .i_tdata (in_fifo_o_tuser),
      .i_tvalid(tmp_i_tvalid),
      .i_tready(),
      .o_tdata (cp_len_tdata),
      .o_tvalid(cp_len_tvalid),
      .o_tready(cp_consume),
      .space   (),
      .occupied()
    );

    // Add a register to calculate the cyclic prefix start read address and
    // figure out if we need to do a cyclic prefix insertion. The latency of
    // this register will be much less than the FFT write time.
    always_ff @(posedge clk) begin
      cp_valid      <= cp_len_tvalid;
      cp_non_zero   <= (cp_len_tdata != 0);
      cp_start_addr <= fft_len - cp_len_tdata;
    end
  end else begin : gen_no_cp_ins_fifo
    assign cp_valid      = '0;
    assign cp_non_zero   = '0;
    assign cp_start_addr = '0;
  end


  //---------------------------------------------------------------------------
  // Read Logic
  //---------------------------------------------------------------------------

  typedef enum logic [1:0] { READ_CHECK, READ_CP, READ_FFT} read_state_t;
  read_state_t read_state = EN_CP_INSERTION ? READ_CHECK : READ_FFT;
  read_state_t read_state_nx;

  logic [ADDR_W-1:0] ram_rd_addr_nx;
  logic              ram_rd_buffer_nx;
  logic              ram_rd_last;   // Indicates when ram_rd_en asserts for the last sample
  logic              out_fifo_avail;

  // Delayed versions of read signals to align with read output timing
  logic [READ_LATENCY-1:0] ram_rd_buffer_del;
  logic [READ_LATENCY-1:0] ram_rd_en_del;
  logic [READ_LATENCY-1:0] ram_rd_last_del;

  logic [DATA_W-1:0] ram_rd_data;
  logic              ram_rd_data_valid;  // Indicates ram_rd_data has data
  logic              ram_rd_data_last;   // Indicates ram_rd_data is the last of the FFT

  assign out_fifo_i_tdata  = ram_rd_data;
  assign out_fifo_i_tvalid = ram_rd_data_valid;
  assign out_fifo_i_tlast  = ram_rd_data_last;

  always_ff @(posedge clk) begin : read_fsm_reg
    if (rst) begin
      read_state        <= EN_CP_INSERTION ? READ_CHECK : READ_FFT;
      ram_rd_buffer     <= '0;
      ram_rd_addr       <= '0;
      ram_rd_buffer_del <= '0;
      ram_rd_en_del     <= '0;
      ram_rd_last_del   <= '0;
      ram_rd_data       <= 'X;
      ram_rd_data_valid <= '0;
      ram_rd_data_last  <= '0;
      out_fifo_avail    <= '0;
    end else begin
      read_state        <= read_state_nx;
      ram_rd_buffer     <= ram_rd_buffer_nx;
      ram_rd_addr       <= ram_rd_addr_nx;

      // Pipeline the buffer selection, enable, and last to align with RAM output
      ram_rd_buffer_del <= (ram_rd_buffer_del << 1) | ram_rd_buffer;
      ram_rd_en_del     <= (ram_rd_en_del     << 1) | ram_rd_en;
      ram_rd_last_del   <= (ram_rd_last_del   << 1) | ram_rd_last;

      // Select the RAM output that was used for the read
      ram_rd_data       <= ram_rd_buffer_del[READ_LATENCY-1] ?
                           ram_rd_data_raw_1 : ram_rd_data_raw_0;
      ram_rd_data_valid <= ram_rd_en_del[READ_LATENCY-1];
      ram_rd_data_last  <= ram_rd_last_del[READ_LATENCY-1];

      // Ensure there's enough room in the output FIFO to account for the
      // latency through the read logic.
      out_fifo_avail    <= out_fifo_space > 4;
    end
  end

  always_comb begin : read_fsm_comb
    ram_rd_en        = '0;
    ram_rd_last      = '0;
    ram_rd_buffer_nx = ram_rd_buffer;
    ram_rd_addr_nx   = ram_rd_addr;
    read_state_nx    = read_state;
    cp_consume       = '0;

    case (read_state)
      READ_CHECK : begin
        // Wait until the next cyclic prefix is available and update the RAM
        // read address appropriately.
        if (cp_valid) begin
          cp_consume = '1;
          if (cp_non_zero) begin
            read_state_nx  = READ_CP;
            ram_rd_addr_nx = cp_start_addr;
          end else begin
            read_state_nx  = READ_FFT;
            ram_rd_addr_nx = '0;
          end
        end
      end
      READ_CP : begin
        // Read out the cyclic prefix
        ram_rd_en = (ok_to_read && out_fifo_avail);
        if (ram_rd_en) begin
          if (ram_rd_addr == fft_len_m1) begin
            ram_rd_addr_nx = '0;
            read_state_nx  = READ_FFT;
          end else begin
            ram_rd_addr_nx = ram_rd_addr + 1;
          end
        end
      end
      default : begin  // READ_FFT
        // Read out the whole FFT
        ram_rd_en = (ok_to_read && out_fifo_avail);
        if (ram_rd_en) begin
          if (ram_rd_addr == fft_len_m1) begin
            ram_rd_last      = '1;
            ram_rd_addr_nx   = '0;
            ram_rd_buffer_nx = ~ram_rd_buffer;
            read_state_nx    = EN_CP_INSERTION ? READ_CHECK : READ_FFT;
          end else begin
            ram_rd_addr_nx = ram_rd_addr + 1;
          end
        end
      end
    endcase
  end


  //---------------------------------------------------------------------------
  // Read/Write Arbitration Logic
  //---------------------------------------------------------------------------
  //
  // Here we ensure that we only write when the write buffer is free and that
  // we only read when the read buffer has an FFT in it. Because we're reading
  // and writing simultaneously, we swap between the lower and upper parts of
  // the RAM as data gets written and read out.
  //
  //---------------------------------------------------------------------------

  always_ff @(posedge clk) begin
    if (ram_wr_en && ram_rd_en) begin
      if (ram_wr_last && ram_rd_last) begin
        // Both buffers are switching on the same cycle
        ok_to_write <= 1'b1;
        ok_to_read  <= 1'b1;
      end else if (ram_wr_last) begin
        // Switching write buffer to the one being used for reads
        ok_to_write <= 1'b0;
      end else if (ram_rd_last) begin
        // Switching read buffer to the one being used for writes
        ok_to_read <= 1'b0;
      end
    end else if (ram_wr_en && ram_wr_last) begin
      // Write buffer is switching
      if (ram_wr_buffer == ram_rd_buffer) begin
        // Write buffer is switching away from the current read buffer
        ok_to_write <= 1'b1;
        ok_to_read  <= 1'b1;
      end else begin
        // Write buffer is switching to the current read buffer
        ok_to_write <= 1'b0;
      end
    end else if (ram_rd_en && ram_rd_last) begin
      // Read buffer is switching
      if (ram_wr_buffer == ram_rd_buffer) begin
        // Read buffer is switching away from the current write buffer
        ok_to_write <= 1'b1;
        ok_to_read  <= 1'b1;
      end else begin
        // Read buffer is switching to the current write buffer
        ok_to_read <= 1'b0;
      end
    end

    //synthesis translate_off
    if (ram_wr_en && ram_rd_en && (ram_wr_buffer == ram_rd_buffer)) begin
      $error("Attempt to read and write the same buffer!");
    end
    //synthesis translate_on

    if (rst) begin
      ok_to_write <= 1'b1;  // Buffers empty after reset
      ok_to_read  <= 1'b0;  // Can't read until we fill the first buffer
    end
  end

endmodule

`default_nettype wire

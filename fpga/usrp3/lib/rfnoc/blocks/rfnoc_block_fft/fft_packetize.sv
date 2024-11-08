//
// Copyright 2025 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: fft_packetize
//
// Description:
//
//   This module converts the packets from the RFNoC shell's AXI-stream data
//   interface into packets to be consumed by the FFT logic. It works in
//   collaboration with the fft_depacketize module, which does the reverse.
//   Relevant information about the burst and number of FFTs is passed along
//   using side-band AXI-stream buses (o_info and o_symbol) to the depacketizer.
//
//   This module takes into account any cyclic prefix removal being performed
//   by the FFT block to ensure the data is packetized for the FFT logic
//   appropriately.
//
//   The RFNoC packet from the NoC shell is input onto the i_noc bus. The
//   repacketized FFT data, resized to the requested symbol size, is output on
//   the o_fft bus.
//
//   The cyclic prefix length to be removed for each symbol is input on the
//   i_cp_rem bus. A copy of the cyclic prefix length that was input is then
//   stored in a FIFO to be output onto the o_cp_rem bus for use by downstream
//   FFT logic, where the actual removal is performed. The cyclic prefix to be
//   used must be set on the i_cp_rem_tdata input at the time the associated
//   symbol begins to be received because it will not wait for i_cp_rem_tvalid.
//   In the case where no cyclic prefix was provided by the user, it is assumed
//   that the logic driving this input will give it a reasonable default (e.g.,
//   a prefix length of 0).
//
//   If the last sample of the input burst does not coincide with the end of a
//   symbol, then data will be inserted, so that the FFT block does not end a
//   burst in the middle of an FFT transfer. This may result in extra data
//   being output at the end of a burst, and the values of the final FFT/IFFT
//   will be corrupted in an unpredictable way. So, users should always input
//   full symbols into the FFT block.
//
//   Timestamps are also supported. The timestamp from the first packet of the
//   burst input into i_noc is captured by this module and passed to the
//   fft_depacketize module via the o_info bus. See the fft_depacketize module
//   for details of how timestamps are generated for output packets.
//
//   This module also supports multiple synchronized channels by setting
//   NUM_CHAN to a number greater than 1. In this case, all the sideband
//   signals are assumed to be shared by all channels (tvalid, tready, tkeep,
//   tlength, ttimestamp, etc.) but the tdata field is NUM_CHAN times wider to
//   accommodate the data from the other channels.
//
//   The minimum FFT size supported by this module is the next power of two
//   that's greater than or equal to 2*NIPC, due to the pipeline delay of
//   calculating when we've reached the end of the packet.
//
//   The incoming RFNoC packets and the cyclic prefix length must both be a
//   multiple of NIPC. Trailing data (i.e., when TKEEP is not all ones) is only
//   allowed on the last transfer of a burst.
//
// Parameters:
//
//   ITEM_W               : Item size (or sample size) in bits for the FFT/IFFT
//                          core.
//   NIPC                 : Number of items per clock cycle. Each word is
//                          NIPC*ITEM_W bits wide. It must be a power of 2.
//   NUM_CHAN             : Number of parallel channels sharing the sideband
//                          information.
//   EN_CP_REMOVAL        : Indicates whether to support cyclic prefix removal.
//   MAX_PKT_SIZE_LOG2    : Maximum packet payload size in items, expressed as
//                          a log base 2. In other words, the maximum packet
//                          size is 2**PKT_SIZE_LOG items.
//   MAX_FFT_SIZE_LOG2    : Maximum FFT size in items, expressed as a log base
//                          2. In other words, the maximum FFT size is
//                          2**MAX_FFT_SIZE_LOG2 items.
//   DATA_FIFO_SIZE_LOG2  : Depth of the internal FIFO that stores output data
//                          to o_fft, expressed as a log base 2. In other
//                          words, the FIFO size is 2**DATA_FIFO_SIZE_LOG2
//                          items for each channel. This can be used to provide
//                          additional buffering, if needed. Set to -1 to
//                          remove the FIFO.
//   CP_FIFO_SIZE_LOG2    : Depth of the internal FIFO that stores cyclic
//                          prefix lengths, expressed as a log base 2. In other
//                          words, the FIFO size is 2**CP_FIFO_SIZE_LOG2
//                          lengths deep. This FIFO is used to pass i_cp_rem to
//                          o_cp_rem and must be deep enough to account for the
//                          maximum number of FFT operations that are in flight
//                          at one time.
//   BURST_FIFO_SIZE_LOG2 : Depth of the internal FIFO that stores burst
//                          information, expressed as a log base 2. In other
//                          words, the FIFO size is 2**BURST_FIFO_SIZE_LOG2
//                          bursts deep. This FIFO stores information about
//                          each burst and must be deep enough to account for
//                          the maximum number of bursts that are in flight at
//                          one time.
//   SYMB_FIFO_SIZE_LOG2  : Depth of the internal FIFO that stores symbol
//                          information, expressed as a log base 2. In other
//                          words, the FIFO size is 2**SYMB_FIFO_SIZE_LOG2
//                          symbols deep. This FIFO is used to store
//                          information about each symbol and must be deep
//                          enough to account for the maximum number of FFT
//                          operations that are in flight at one time.
//

`default_nettype none


module fft_packetize
  import rfnoc_chdr_utils_pkg::*;
  import fft_packetize_pkg::*;
#(
  int ITEM_W               = 32,
  int NIPC                 = 1,
  int NUM_CHAN             = 1,
  bit EN_CP_REMOVAL        = 1'b1,
  int MAX_PKT_SIZE_LOG2    = 11,
  int MAX_FFT_SIZE_LOG2    = 10,
  int DATA_FIFO_SIZE_LOG2  = -1,
  int CP_FIFO_SIZE_LOG2    = 5,
  int BURST_FIFO_SIZE_LOG2 = 5,
  int SYMB_FIFO_SIZE_LOG2  = 5,

  // Internal constants
  localparam int DATA_W          = NUM_CHAN * ITEM_W * NIPC,
  localparam int KEEP_W          = NIPC,
  localparam int PKT_SIZE_W      = MAX_PKT_SIZE_LOG2 + 1,
  localparam int FFT_SIZE_W      = MAX_FFT_SIZE_LOG2 + 1,
  localparam int FFT_SIZE_LOG2_W = $clog2(MAX_FFT_SIZE_LOG2 + 1),
  localparam int CP_LEN_W        = MAX_FFT_SIZE_LOG2
) (
  input  wire                         clk,
  input  wire                         rst,

  input  wire  [ FFT_SIZE_LOG2_W-1:0] fft_size_log2,

  // Input from cyclic prefix removal list
  input  wire  [        CP_LEN_W-1:0] i_cp_rem_tdata,
  input  wire                         i_cp_rem_tvalid,
  output logic                        i_cp_rem_tready,

  // Output to cyclic prefix removal logic
  output logic [        CP_LEN_W-1:0] o_cp_rem_tdata,
  output logic                        o_cp_rem_tvalid,
  input  wire                         o_cp_rem_tready,

  // Input from NoC Shell
  input  wire  [          DATA_W-1:0] i_noc_tdata,
  input  wire  [          KEEP_W-1:0] i_noc_tkeep,
  input  wire                         i_noc_tlast,
  input  wire                         i_noc_tvalid,
  output logic                        i_noc_tready,
  input  wire  [CHDR_TIMESTAMP_W-1:0] i_noc_ttimestamp,
  input  wire                         i_noc_thas_time,
  input  wire  [   CHDR_LENGTH_W-1:0] i_noc_tlength,
  input  wire                         i_noc_teov,
  input  wire                         i_noc_teob,

  // Output to FFT core
  output logic [          DATA_W-1:0] o_fft_tdata,
  output logic [          KEEP_W-1:0] o_fft_tkeep,
  output logic                        o_fft_tlast,
  output logic                        o_fft_tvalid,
  input  wire                         o_fft_tready,

  // Information about each burst (packet size in items, timestamp), going to
  // the depacketizer.
  output burst_info_t                 o_burst_tdata,
  output logic                        o_burst_tvalid,
  input  wire                         o_burst_tready,

  // Information about each symbol (whether it is the last of a burst), going
  // to the depacketizer.
  output symbol_info_t                o_symbol_tdata,
  output logic                        o_symbol_tvalid,
  input  wire                         o_symbol_tready
);

  // Make sure NIPC is a power of 2
  if (NIPC != 2**$clog2(NIPC)) begin : gen_nipc_assertion
    $error("NIPC must be a power of 2");
  end

  // Create a mask to remove unused bits
  localparam logic [  CP_LEN_W-1:0] CP_LEN_MASK   = $clog2(NIPC);
  localparam logic [PKT_SIZE_W-1:0] PKT_SIZE_MASK = $clog2(NIPC);


  //---------------------------------------------------------------------------
  // FFT Size Register
  //---------------------------------------------------------------------------
  //
  // We assume the fft_size input is set well in advance of any data being
  // received and that it does not change during a burst. This means that we
  // can tolerate a few cycles of delay on these registers.
  //
  //---------------------------------------------------------------------------

  logic [FFT_SIZE_W-1:0] fft_size;

  always_ff @(posedge clk) begin
    fft_size <= 1 << fft_size_log2;
  end


  //---------------------------------------------------------------------------
  // State Machine
  //---------------------------------------------------------------------------

  typedef enum logic [1:0] {
    WAIT_BURST_ST,
    GET_PREFIX_ST,
    PASS_SYMBOL_ST,
    FINISH_SYMBOL_ST
  } state_t;

  state_t state = WAIT_BURST_ST;

  // Burst information
  logic [CHDR_TIMESTAMP_W-1:0] timestamp;
  logic                        has_time;
  logic [      PKT_SIZE_W-1:0] pkt_size;             // Packet size in items
  logic                        burst_wr_stb = 1'b0;  // Info write strobe

  // Symbol size (IFFT data + cyclic prefix)
  logic [FFT_SIZE_W-1:0] symbol_size;
  logic                  prefix_wr_stb = 1'b0;  // Write strobe

  // Symbol information
  logic last_symbol;           // Last symbol of burst
  logic symbol_wr_stb = 1'b0;  // Write strobe

  // Counter to track how much of current symbol we've output
  logic [FFT_SIZE_W-1:0] item_count = NIPC;

  // Indicates we're on the last sample/item of the symbol
  logic symbol_tlast;

  // Data FIFO inputs
  logic [DATA_W-1:0] i_fft_tdata;
  logic [KEEP_W-1:0] i_fft_tkeep;
  logic              i_fft_tlast;
  logic              i_fft_tvalid;
  logic              i_fft_tready;

  // Cyclic prefix removal length FIFO inputs
  logic [CP_LEN_W-1:0] cp_rem_fifo_tdata;
  logic                cp_rem_fifo_tvalid;
  logic                cp_rem_fifo_tready;

  // Burst information FIFO inputs
  burst_info_t burst_fifo_tdata;
  logic        burst_fifo_tvalid;
  logic        burst_fifo_tready;

  // Symbol information FIFO inputs
  symbol_info_t symbol_fifo_tdata;
  logic         symbol_fifo_tvalid;
  logic         symbol_fifo_tready;

  always_ff @(posedge clk) begin
    burst_wr_stb  <= 1'b0;
    prefix_wr_stb <= 1'b0;
    symbol_wr_stb <= 1'b0;

    unique case (state)
      WAIT_BURST_ST : begin
        // Grab the packet and FFT size for this burst
        item_count   <= 2*NIPC;  // Account for one cycle of delay, plus one for tlast
        symbol_tlast <= 1'b0;
        timestamp    <= i_noc_ttimestamp;
        has_time     <= i_noc_thas_time;
        pkt_size     <= (i_noc_tlength / (ITEM_W/8)) & ~PKT_SIZE_MASK;

        // We wait until we have a new packet and the downstream info FIFOs
        // have room to accept another entry (CP removal length, burst info,
        // and symbol info).
        if (i_noc_tvalid && cp_rem_fifo_tready && burst_fifo_tready && symbol_fifo_tready) begin
          //synthesis translate_off
          assert (i_noc_tlength % (NIPC * ITEM_W/8) == 0) else
            $error("fft_packetize: Input packet length is not a multiple of NIPC");
          //synthesis translate_on
          burst_wr_stb <= 1'b1;
          if (EN_CP_REMOVAL) begin
            state <= GET_PREFIX_ST;
          end else begin
            symbol_size <= fft_size;
            state       <= PASS_SYMBOL_ST;
          end
        end
      end

      GET_PREFIX_ST : begin
        // Get the cyclic prefix length for this the next symbol. We assume it
        // is always valid because it defaults to the desired default value.
        item_count   <= 2*NIPC;
        symbol_tlast <= 1'b0;

        // Wait until there's room in the downstream FIFOs for the next symbol
        if (symbol_fifo_tready && cp_rem_fifo_tready) begin
          // Symbol size only changes when CP removal is enabled
          if (EN_CP_REMOVAL) begin
            symbol_size <= fft_size + (i_cp_rem_tdata & ~CP_LEN_MASK);
          end
          prefix_wr_stb <= 1'b1;
          state         <= PASS_SYMBOL_ST;
        end
      end

      PASS_SYMBOL_ST : begin
        // Pass the symbol through
        if (i_fft_tvalid && i_fft_tready) begin
          item_count   <= item_count + NIPC;
          symbol_tlast <= (item_count >= symbol_size);

          if (symbol_tlast) begin
            symbol_wr_stb <= 1'b1;
            if (i_noc_tlast && i_noc_teob) begin
              // All done! Wait for the next burst.
              last_symbol <= 1'b1;
              state       <= WAIT_BURST_ST;
            end else begin
              last_symbol <= 1'b0;
              // Finished the symbol. Figure out the length of the next one.
              state <= GET_PREFIX_ST;
            end
          end else if (i_noc_tlast && i_noc_teob) begin
            // We've reached the end of the burst, but we haven't finished the
            // current symbol.
            state <= FINISH_SYMBOL_ST;
          end else begin
            ;  // Let the next item/sample pass through
          end
        end
      end

      FINISH_SYMBOL_ST : begin
        // Push through enough data to finish the symbol, so that the
        // downstream FFT logic doesn't get left in a bad state.
        last_symbol <= 1'b1;
        if (i_fft_tvalid && i_fft_tready) begin
          item_count   <= item_count + NIPC;
          symbol_tlast <= (item_count >= symbol_size);

          if (symbol_tlast) begin
            // All done! Wait for the next burst.
            symbol_wr_stb <= 1'b1;
            state         <= WAIT_BURST_ST;
          end
        end
      end
    endcase

    if (rst) begin
      state         <= WAIT_BURST_ST;
      prefix_wr_stb <= 1'b0;
      timestamp     <= 'X;
      has_time      <= 'X;
      pkt_size      <= 'X;
      burst_wr_stb  <= 1'b0;
      symbol_size   <= 'X;
      item_count    <= 'X;
      symbol_tlast  <= 'X;
      symbol_wr_stb <= 1'b0;
      last_symbol   <= 'X;
    end
  end


  //---------------------------------------------------------------------------
  // Data Pass-through Logic
  //---------------------------------------------------------------------------

  always_comb begin
    i_fft_tdata = i_noc_tdata;
    i_fft_tkeep = {KEEP_W{1'b1}};
    i_fft_tlast = symbol_tlast;

    unique case (state)
      WAIT_BURST_ST : begin
        i_fft_tvalid = 1'b0;
        i_noc_tready = 1'b0;
      end

      GET_PREFIX_ST : begin
        i_fft_tvalid = 1'b0;
        i_noc_tready = 1'b0;
      end

      PASS_SYMBOL_ST : begin
        // Pass the next item/sample through
        i_fft_tvalid = i_noc_tvalid;
        i_noc_tready = i_fft_tready;
      end

      FINISH_SYMBOL_ST : begin
        // Flush the data through the FFT
        i_fft_tvalid = 1'b1;
        i_noc_tready = 1'b0;
      end
    endcase
  end


  //---------------------------------------------------------------------------
  // Output Data FIFO
  //---------------------------------------------------------------------------
  //
  // This FIFO is required to handle the worst-case scenario in which the user
  // wants small FFTs and large packets. In the case where the FFT size equals
  // the packet size, this FIFO is not required.
  //
  //---------------------------------------------------------------------------

  if (DATA_FIFO_SIZE_LOG2 > -1) begin : gen_output_fifo
    axi_fifo #(
      .WIDTH(1 + KEEP_W + DATA_W               ),
      .SIZE (DATA_FIFO_SIZE_LOG2 - $clog2(NIPC))
    ) axi_fifo_i (
      .clk     (clk                                    ),
      .reset   (rst                                    ),
      .clear   (1'b0                                   ),
      .i_tdata ({i_fft_tlast, i_fft_tkeep, i_fft_tdata}),
      .i_tvalid(i_fft_tvalid                           ),
      .i_tready(i_fft_tready                           ),
      .o_tdata ({o_fft_tlast, o_fft_tkeep, o_fft_tdata}),
      .o_tvalid(o_fft_tvalid                           ),
      .o_tready(o_fft_tready                           ),
      .space   (                                       ),
      .occupied(                                       )
    );
  end else begin : gen_no_output_fifo
    assign o_fft_tdata  = i_fft_tdata;
    assign o_fft_tkeep  = i_fft_tkeep;
    assign o_fft_tlast  = i_fft_tlast;
    assign o_fft_tvalid = i_fft_tvalid;
    assign i_fft_tready = o_fft_tready;
  end


  //---------------------------------------------------------------------------
  // Cyclic Prefix Pass-through Logic
  //---------------------------------------------------------------------------

  if (EN_CP_REMOVAL) begin : gen_cp_rem_pass_through
    logic [15:0] cp_rem_fifo_space;

    assign i_cp_rem_tready    = prefix_wr_stb;
    assign cp_rem_fifo_tdata  = i_cp_rem_tdata & ~CP_LEN_MASK; // Clear the unused bits
    assign cp_rem_fifo_tvalid = prefix_wr_stb;

    axi_fifo #(
      .WIDTH(CP_LEN_W         ),
      .SIZE (CP_FIFO_SIZE_LOG2)
    ) axis_fifo_cp_length (
      .clk     (clk               ),
      .reset   (rst               ),
      .clear   (1'b0              ),
      .i_tdata (cp_rem_fifo_tdata ),
      .i_tvalid(cp_rem_fifo_tvalid),
      .i_tready(cp_rem_fifo_tready),
      .o_tdata (o_cp_rem_tdata    ),
      .o_tvalid(o_cp_rem_tvalid   ),
      .o_tready(o_cp_rem_tready   ),
      .space   (cp_rem_fifo_space ),
      .occupied(                  )
    );

    // The CP removal length FIFO should be large enough to hold all the
    // symbols that are in flight. Filling up might be an indication that it's
    // sized too small.
    //synthesis translate_off
    logic cp_rem_fifo_empty_prev = 0;
    always_ff @(posedge clk) begin
      cp_rem_fifo_empty_prev <= (cp_rem_fifo_space == 0);
      if (!cp_rem_fifo_empty_prev && cp_rem_fifo_space == 0) begin
        $warning("CP removal FIFO has filled");
      end
    end
    //synthesis translate_on
  end else begin : gen_no_cp_rem_pass_through
    assign cp_rem_fifo_tready = 1'b1;
    assign i_cp_rem_tready    = 1'b1;
    assign o_cp_rem_tdata     = '0;
    assign o_cp_rem_tvalid    = 1'b0;
  end


  //---------------------------------------------------------------------------
  // Burst Info Logic
  //---------------------------------------------------------------------------

  logic [15:0] burst_fifo_space;

  assign burst_fifo_tdata  = '{ timestamp, has_time, pkt_size & ~PKT_SIZE_MASK};
  assign burst_fifo_tvalid = burst_wr_stb;

  axi_fifo #(
    .WIDTH(BURST_INFO_W        ),
    .SIZE (BURST_FIFO_SIZE_LOG2)
  ) axis_fifo_burst (
    .clk     (clk              ),
    .reset   (rst              ),
    .clear   (1'b0             ),
    .i_tdata (burst_fifo_tdata ),
    .i_tvalid(burst_fifo_tvalid),
    .i_tready(burst_fifo_tready),
    .o_tdata (o_burst_tdata    ),
    .o_tvalid(o_burst_tvalid   ),
    .o_tready(o_burst_tready   ),
    .space   (burst_fifo_space ),
    .occupied(                 )
  );

  // The burst information FIFO should be large enough to hold all the
  // bursts that are in flight. Filling up might be an indication that it's
  // sized too small.
  //synthesis translate_off
  logic burst_fifo_empty_prev = 0;
  always_ff @(posedge clk) begin
    burst_fifo_empty_prev <= (burst_fifo_space == 0);
    if (!burst_fifo_empty_prev && burst_fifo_space == 0) begin
      $warning("Burst info FIFO has filled");
    end
  end
  //synthesis translate_on


  //---------------------------------------------------------------------------
  // Symbol Information FIFO
  //---------------------------------------------------------------------------
  //
  // This FIFO is used to store the information about the the symbols that have
  // been input to the FFT block. Each element in the FIFO corresponds to one
  // symbol. If the data bit is 0, then the corresponding symbol is not the
  // last symbol of the burst. If the data bit is 1, then it is the last symbol
  // of the burst.
  //
  //---------------------------------------------------------------------------

  logic [15:0] o_symbol_space;

  assign symbol_fifo_tdata.last = last_symbol;
  assign symbol_fifo_tvalid     = symbol_wr_stb;

  axi_fifo #(
    .WIDTH(SYMBOL_INFO_W      ),
    .SIZE (SYMB_FIFO_SIZE_LOG2)
  ) axis_fifo_symb (
    .clk     (clk               ),
    .reset   (rst               ),
    .clear   (1'b0              ),
    .i_tdata (symbol_fifo_tdata ),
    .i_tvalid(symbol_fifo_tvalid),
    .i_tready(symbol_fifo_tready),
    .o_tdata (o_symbol_tdata    ),
    .o_tvalid(o_symbol_tvalid   ),
    .o_tready(o_symbol_tready   ),
    .space   (o_symbol_space    ),
    .occupied(                  )
  );

  // The symbol information FIFO should be large enough to hold all the
  // symbols that are in flight. Filling up might be an indication that it's
  // sized too small.
  //synthesis translate_off
  logic symbol_fifo_empty_prev = 0;
  always_ff @(posedge clk) begin
    symbol_fifo_empty_prev <= (o_symbol_space == 0);
    if (!symbol_fifo_empty_prev && o_symbol_space == 0) begin
      $warning("Symbol info FIFO has filled");
    end
  end
  //synthesis translate_on

endmodule : fft_packetize


`default_nettype wire

//
// Copyright 2025 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: fft_depacketize
//
// Description:
//
//   This module converts the packets coming from the FFT logic into packets
//   for the RFNoC shell's AXI-stream data interface. It works in collaboration
//   with the fft_packetize module, which converts in the other direction.
//   Relevant information about the burst and number of FFTs is passed to this
//   module using side-band AXI-stream buses (i_burst and i_symbol) from the
//   packetizer.
//
//   This module takes into account any cyclic prefix insertion being performed
//   by the FFT block to ensure that the data is packetized for the NoC shell
//   appropriately, including setting EOV.
//
//   The FFT packet, which may include a cyclic prefix, is input onto the i_fft
//   bus. The repacketized RFNoC data, resized to the burst's packet size, is
//   output on the o_noc bus.
//
//   The cyclic prefix length to be inserted for each symbol is input on the
//   i_cp_ins bus. A copy of the cyclic prefix length that was input is then
//   output onto the o_cp_ins bus for use by downstream FFT logic, where the
//   actual insertion is performed.
//
//   Timestamps are also supported. Per the RFNoC specification, bursts are
//   expected to be a contiguous stream of samples. With cyclic prefix
//   insertion or removal, this may not be the case. To keep things simple, we
//   output the samples/items from the FFT block as if they were contiguous.
//   The timestamp from the start of each burst (provided via the i_burst bus)
//   is used as the timestamp for the first packet of the burst output on
//   o_noc. Subsequent timestamps on o_noc will be automatically calculated and
//   included for the remainder of the burst if EN_TIME_ALL_PKTS is 1. In this
//   case, the timestamp of each packet output on o_noc will be incremented as
//   if the data were contiguous. It's up to the user application to correct
//   the time for the packets based on the cyclic prefix information if needed.
//
//   This module also supports multiple synchronized channels by setting
//   NUM_CHAN to a number greater than 1. In this case, all the sideband
//   signals are assumed to be shared by all channels (tvalid, tready, tkeep,
//   tlength, ttimestamp, etc.) but the tdata field is NUM_CHAN times wider to
//   accommodate the data from the other channels.
//
//   The outgoing RFNoC packets and the cyclic prefix length must both be a
//   multiple of NIPC. Trailing data (i.e., when TKEEP is not all ones) is only
//   allowed on the last transfer of the last FFT of a burst.
//
// Parameters:
//
//   ITEM_W              : Item size (or sample size) in bits for the FFT/IFFT
//                         core.
//   NIPC                : Number of items per clock cycle. Each word is
//                         NIPC*ITEM_W bits wide. It must be a power of 2.
//   NUM_CHAN            : Number of parallel channels sharing the sideband
//                         information.
//   EN_CP_INSERTION     : Indicates whether to support cyclic prefix insertion.
//   MAX_PKT_SIZE_LOG2   : Maximum packet payload size in items, expressed as a
//                         log base 2. In other words, the maximum packet size
//                         is 2**PKT_SIZE_LOG items.
//   MAX_FFT_SIZE_LOG2   : Maximum FFT size in items, expressed as a log base
//                         2. In other words, the maximum FFT size is
//                         2**MAX_FFT_SIZE_LOG2 items.
//   DATA_FIFO_SIZE_LOG2 : Depth of the internal FIFO that stores input data
//                         from i_fft, expressed as a log base 2. In other
//                         words, the FIFO size is 2**DATA_FIFO_SIZE_LOG2 items
//                         for each channel. This can be used to provide
//                         additional buffering, if needed. Set to -1 to remove
//                         the FIFO.
//   CP_FIFO_SIZE_LOG2   : Depth of the internal FIFO that stores cyclic prefix
//                         lengths, expressed as a log base 2. In other words,
//                         the FIFO size is 2**CP_FIFO_SIZE_LOG2 lengths deep.
//                         This FIFO is used to pass i_cp_ins to o_cp_ins and
//                         must be deep enough to account for the maximum
//                         number of FFT operations that are in flight at one
//                         time.
//   SYMB_FIFO_SIZE_LOG2 : Depth of the internal FIFO that stores symbol
//                         information, expressed as a log base 2. In other
//                         words, the FIFO size is 2**SYMB_FIFO_SIZE_LOG2
//                         entries deep (one entry per symbol). This FIFO is
//                         used to store information about each symbol and must
//                         be deep enough to account for the maximum number of
//                         FFT operations that are in flight at one time.
//   EN_TIME_ALL_PKTS    : When set to 1, the timestamp is updated for each
//                         packet. When 0, only the first packet of each burst
//                         will have a timestamp.
//

`default_nettype none


module fft_depacketize
  import rfnoc_chdr_utils_pkg::*;
  import fft_packetize_pkg::*;
#(
  int ITEM_W              = 32,
  int NIPC                = 1,
  int NUM_CHAN            = 1,
  bit EN_CP_INSERTION     = 1'b1,
  int MAX_PKT_SIZE_LOG2   = 11,
  int MAX_FFT_SIZE_LOG2   = 10,
  int DATA_FIFO_SIZE_LOG2 = -1,
  int CP_FIFO_SIZE_LOG2   = 5,
  int SYMB_FIFO_SIZE_LOG2 = 5,
  bit EN_TIME_ALL_PKTS    = 1,

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

  // Information about each burst (packet size in items, timestamp)
  input  wire  burst_info_t           i_burst_tdata,
  input  wire                         i_burst_tvalid,
  output logic                        i_burst_tready = 1'b0,

  // The symbol information, which comes from the packetizer, tells us whether
  // or not each symbol is the last in the burst.
  input  wire  symbol_info_t          i_symbol_tdata,
  input  wire                         i_symbol_tvalid,
  output logic                        i_symbol_tready = 1'b0,

  // Input from cyclic prefix insertion list
  input  wire  [        CP_LEN_W-1:0] i_cp_ins_tdata,
  input  wire                         i_cp_ins_tvalid,
  output logic                        i_cp_ins_tready,

  // Output to cyclic prefix insertion logic
  output logic [        CP_LEN_W-1:0] o_cp_ins_tdata,
  output logic                        o_cp_ins_tvalid,
  input  wire                         o_cp_ins_tready,

  // Input from FFT core
  input  wire  [          DATA_W-1:0] i_fft_tdata,
  input  wire  [          KEEP_W-1:0] i_fft_tkeep,
  input  wire                         i_fft_tlast,
  input  wire                         i_fft_tvalid,
  output logic                        i_fft_tready,

  // Output to NoC Shell
  output logic [          DATA_W-1:0] o_noc_tdata,
  output logic [          KEEP_W-1:0] o_noc_tkeep,
  output logic                        o_noc_tlast,
  output logic                        o_noc_tvalid,
  input  wire                         o_noc_tready,
  output logic [CHDR_TIMESTAMP_W-1:0] o_noc_ttimestamp,
  output logic                        o_noc_thas_time,
  output logic [   CHDR_LENGTH_W-1:0] o_noc_tlength,
  output logic                        o_noc_teov,
  output logic                        o_noc_teob
);

  // Make sure NIPC is a power of 2
  if (NIPC != 2**$clog2(NIPC)) begin : gen_nipc_assertion
    $error("NIPC must be a power of 2");
  end

  // Create masks to remove unused bits
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

  localparam int FFT_SIZE_MASK_W = FFT_SIZE_W - 1;

  logic [     FFT_SIZE_W-1:0] fft_size;
  logic [FFT_SIZE_MASK_W-1:0] fft_size_mask;

  always_ff @(posedge clk) begin
    fft_size      <= 1 << fft_size_log2;
    fft_size_mask <= fft_size-1;
  end


  //---------------------------------------------------------------------------
  // Symbol Size and Cyclic Prefix Logic
  //---------------------------------------------------------------------------
  //
  // This logic here figures out the next symbol size, based on the cyclic
  // prefix, and passes it along to downstream logic. If cyclic prefix is
  // disabled, then most of this logic is not needed.
  //
  //---------------------------------------------------------------------------

  typedef struct packed {
    logic                  last;    // Is this symbol the last of the burst?
    logic [FFT_SIZE_W-1:0] length;  // Length of symbol in items/samples
  } symbol_fifo_t;

  // Output of the symbol information FIFO
  symbol_fifo_t o_symbol_fifo_tdata;
  logic         o_symbol_fifo_tvalid;
  logic         o_symbol_fifo_tready;


  if (EN_CP_INSERTION) begin : gen_symbol_size_fsm

    //---------------------------------------------
    // Symbol Size and Cyclic Prefix State Machine
    //---------------------------------------------

    typedef enum logic [1:0] {
      WAIT_SYMBOL_ST,
      CALC_SYMBOL_ST,
      PASS_SYMBOL_ST
    } symbol_state_t;

    symbol_state_t symbol_state = WAIT_SYMBOL_ST;

    logic                  cp_last_symbol;
    logic                  prefix_rd_stb = 1'b0;
    logic [  CP_LEN_W-1:0] cp_len;
    logic [FFT_SIZE_W-1:0] symbol_size;

    // Input to the symbol information FIFO
    symbol_fifo_t i_symbol_fifo_tdata;
    logic         i_symbol_fifo_tvalid;
    logic         i_symbol_fifo_tready;

    // Input to the CP insertion length FIFO
    logic [CP_LEN_W-1:0] i_cp_ins_fifo_tdata;
    logic                i_cp_ins_fifo_tvalid;
    logic                i_cp_ins_fifo_tready;

    always_ff @(posedge clk) begin : symbol_fsm_reg
      i_symbol_tready      <= 1'b0;
      prefix_rd_stb        <= 1'b0;
      i_symbol_fifo_tvalid <= 1'b0;

      case (symbol_state)
        WAIT_SYMBOL_ST : begin
          // Wait until we are told by the packetizer about a new symbol. When
          // we are, we capture the current cyclic-prefix length. We require
          // that this always be valid, so it's OK to read it without checking
          // i_cp_ins_tvalid.
          //
          // To avoid overfilling the symbol and CP insertion length FIFOs, we
          // wait for their tready signals to be asserted, which on the
          // axi_fifo indicates that they are not full.
          i_symbol_tready <= i_symbol_fifo_tready && i_cp_ins_fifo_tready;
          cp_last_symbol  <= i_symbol_tdata.last;
          cp_len          <= i_cp_ins_tdata;
          if (i_symbol_tvalid && i_symbol_tready) begin
            i_symbol_tready <= 1'b0;
            prefix_rd_stb <= 1'b1;
            symbol_state    <= CALC_SYMBOL_ST;
          end
        end

        CALC_SYMBOL_ST : begin
          // Calculate the length of the next symbol to be output.
          i_symbol_fifo_tvalid <= 1'b1;
          symbol_size          <= fft_size + (cp_len & ~CP_LEN_MASK);
          symbol_state         <= PASS_SYMBOL_ST;
        end

        PASS_SYMBOL_ST : begin
          // Pass the calculated length to a FIFO.
          i_symbol_fifo_tvalid <= 1'b1;
          if (i_symbol_fifo_tready) begin
            i_symbol_fifo_tvalid <= 1'b0;
            symbol_state         <= WAIT_SYMBOL_ST;
          end
        end
      endcase

      if (rst) begin
        symbol_state         <= WAIT_SYMBOL_ST;
        prefix_rd_stb        <= 1'b0;
        i_symbol_tready      <= 1'b0;
        i_symbol_fifo_tvalid <= 1'b0;
        cp_last_symbol       <= 1'bX;
        cp_len               <= 'X;
        symbol_size          <= 'X;
      end
    end : symbol_fsm_reg


    //---------------------------------
    // Symbol Information FIFO
    //---------------------------------

    assign i_symbol_fifo_tdata = '{ cp_last_symbol, symbol_size };

    axi_fifo #(
      .WIDTH($bits(symbol_fifo_t)),
      .SIZE (SYMB_FIFO_SIZE_LOG2 )
    ) axis_fifo_symbol_info (
      .clk     (clk                 ),
      .reset   (rst                 ),
      .clear   (1'b0                ),
      .i_tdata (i_symbol_fifo_tdata ),
      .i_tvalid(i_symbol_fifo_tvalid),
      .i_tready(i_symbol_fifo_tready),
      .o_tdata (o_symbol_fifo_tdata ),
      .o_tvalid(o_symbol_fifo_tvalid),
      .o_tready(o_symbol_fifo_tready),
      .space   (                    ),
      .occupied(                    )
    );


    //---------------------------------
    // Cyclic Prefix Length FIFO
    //---------------------------------

    logic [15:0] cp_ins_fifo_space;

    assign i_cp_ins_tready      = prefix_rd_stb;
    assign i_cp_ins_fifo_tdata  = i_cp_ins_tdata & ~CP_LEN_MASK; // Clear the unused bits
    assign i_cp_ins_fifo_tvalid = prefix_rd_stb;

    axi_fifo #(
      .WIDTH(CP_LEN_W         ),
      .SIZE (CP_FIFO_SIZE_LOG2)
    ) axis_fifo_cp_length (
      .clk     (clk                 ),
      .reset   (rst                 ),
      .clear   (1'b0                ),
      .i_tdata (i_cp_ins_fifo_tdata ),
      .i_tvalid(i_cp_ins_fifo_tvalid),
      .i_tready(i_cp_ins_fifo_tready),
      .o_tdata (o_cp_ins_tdata      ),
      .o_tvalid(o_cp_ins_tvalid     ),
      .o_tready(o_cp_ins_tready     ),
      .space   (cp_ins_fifo_space   ),
      .occupied(                    )
    );

    // The cyclic prefix length FIFO should be large enough for all the symbols
    // that are in flight. Filling up might be an indication that it's sized
    // too small.
    //synthesis translate_off
    logic cp_ins_fifo_empty_prev = 0;
    always_ff @(posedge clk) begin
      cp_ins_fifo_empty_prev <= (cp_ins_fifo_space == 0);
      if (!cp_ins_fifo_empty_prev && cp_ins_fifo_space == 0) begin
        $warning("CP insertion FIFO has filled");
      end
    end
    //synthesis translate_on


  end else begin : gen_no_symbol_size_fsm
    //---------------------------------
    // Cyclic Prefix Disabled
    //---------------------------------

    // If there's no cyclic prefix, then the symbol length is fixed, so we only
    // need to pass along the symbol info and the configured fft_size.
    assign o_symbol_fifo_tdata  = '{ i_symbol_tdata.last, fft_size };
    assign o_symbol_fifo_tvalid = i_symbol_tvalid;
    always_comb i_symbol_tready = o_symbol_fifo_tready;

    // There's no cyclic prefix length to pass through.
    assign i_cp_ins_tready = 1'b1;
    assign o_cp_ins_tdata  = '0;
    assign o_cp_ins_tvalid = 1'b0;
  end


  //---------------------------------------------------------------------------
  // Input Data FIFO
  //---------------------------------------------------------------------------

  logic [DATA_W-1:0] o_fft_tdata;
  logic [KEEP_W-1:0] o_fft_tkeep;
  logic              o_fft_tlast;
  logic              o_fft_tvalid;
  logic              o_fft_tready;

  if (DATA_FIFO_SIZE_LOG2 > -1) begin : gen_input_fifo
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
  end else begin : gen_no_input_fifo
    assign o_fft_tdata  = i_fft_tdata;
    assign o_fft_tkeep  = i_fft_tkeep;
    assign o_fft_tlast  = i_fft_tlast;
    assign o_fft_tvalid = i_fft_tvalid;
    assign i_fft_tready = o_fft_tready;
  end


  //---------------------------------------------------------------------------
  // Packet Resize State Machine
  //---------------------------------------------------------------------------
  //
  // Here we figure out the information for each packet to be output to the NoC
  // shell (length, EOV, EOB), resize the symbol-sized packets to RFNoC packet
  // sizes, and pass through the FFT data.
  //
  //---------------------------------------------------------------------------

  typedef enum logic [2:0] {
    WAIT_BURST_ST,
    CALC_ITEMS_ST,
    CALC_PACKET_ST,
    CALC_VECTOR_ST,
    CALC_EOV_ST,
    PASS_PACKET_ST
  } state_t;

  state_t state = WAIT_BURST_ST;

  // Information for the current burst
  logic [      PKT_SIZE_W-1:0] pkt_size;   // Packet size in items
  logic [CHDR_TIMESTAMP_W-1:0] timestamp;
  logic                        has_time;

  // Sideband information for the next packet to send
  logic                        next_pkt_last;
  logic [      PKT_SIZE_W-1:0] next_pkt_size;
  logic                        next_pkt_eob;
  logic                        next_pkt_eov;
  logic [CHDR_TIMESTAMP_W-1:0] next_pkt_timestamp;
  logic                        next_pkt_has_time;

  logic last_symbol;

  // Item counter to track progress in current packet
  logic [PKT_SIZE_W-1:0] pkt_item_count;

  // Item counter to track vector alignment
  logic [MAX_FFT_SIZE_LOG2-1:0] vect_item_count;

  // Total number of items left to send for the symbols we know about so far.
  // In the worst case, this must be large enough to hold just less than the
  // number of items in a maximum sized packet (2**MAX_PKT_SIZE_LOG2) plus a
  // maximum sized symbol (2**MAX_FFT_SIZE_LOG2), including a maximum cyclic
  // prefix (2**MAX_FFT_SIZE_LOG-1).
  localparam int ITEMS_TO_SEND_W = (EN_CP_INSERTION) ?
     $clog2(2**MAX_PKT_SIZE_LOG2 + 2**(MAX_FFT_SIZE_LOG2+1)-1 + 1) :
     $clog2(2**MAX_PKT_SIZE_LOG2 + 2**MAX_FFT_SIZE_LOG2 + 1);
  logic [ITEMS_TO_SEND_W-1:0] items_to_send;


  always_ff @(posedge clk) begin
    i_burst_tready       <= 1'b0;
    o_symbol_fifo_tready <= 1'b0;

    unique case (state)
      WAIT_BURST_ST : begin
        // Grab the packet and FFT size for this burst
        items_to_send   <= '0;
        vect_item_count <= '0;
        i_burst_tready  <= 1'b1;
        if (i_burst_tvalid) begin
          //synthesis translate_off
          assert (i_burst_tdata.length % (NIPC) == 0) else
            $error("fft_depacketize: Input packet length is not a multiple of NIPC");
          //synthesis translate_on
          pkt_size             <= i_burst_tdata.length & ~PKT_SIZE_MASK;
          timestamp            <= i_burst_tdata.timestamp;
          has_time             <= i_burst_tdata.has_time;
          o_symbol_fifo_tready <= 1'b1;
          state                <= CALC_ITEMS_ST;
          if (!EN_TIME_ALL_PKTS) begin
            next_pkt_timestamp <= i_burst_tdata.timestamp;
          end
        end
      end

      CALC_ITEMS_ST : begin
        // Wait for the next symbol's information to arrive
        o_symbol_fifo_tready <= 1'b1;
        last_symbol          <= o_symbol_fifo_tdata.last;
        if (o_symbol_fifo_tvalid) begin
          items_to_send        <= items_to_send + o_symbol_fifo_tdata.length;
          o_symbol_fifo_tready <= 1'b0;
          state                <= CALC_PACKET_ST;
        end
      end

      CALC_PACKET_ST : begin
        // Do we have enough to send a packet? If not, get another prefix
        // unless we're at the end.
        pkt_item_count    <= 2*NIPC;   // Account for one cycle of delay, plus one for tlast
        next_pkt_size     <= pkt_size;
        next_pkt_eob      <= 1'b0;
        next_pkt_has_time <= has_time;
        if (EN_TIME_ALL_PKTS) begin
          next_pkt_timestamp <= timestamp;
        end

        if (items_to_send > pkt_size) begin
          // Send the next packet, but we know we have at least one more packet
          // to send after this.
          next_pkt_size <= pkt_size;
          next_pkt_eob  <= 1'b0;
          state         <= CALC_VECTOR_ST;
        end else if (last_symbol) begin
          // We don't have a full packet, but we're on the last symbol, so
          // send what we have.
          next_pkt_size <= items_to_send;
          next_pkt_eob  <= 1'b1;
          state         <= CALC_VECTOR_ST;
        end else if (items_to_send == pkt_size) begin
          // We have exactly a full packet, but we're NOT on the last symbol
          next_pkt_size <= pkt_size;
          next_pkt_eob  <= 1'b0;
          state         <= CALC_VECTOR_ST;
        end else begin
          // We don't have a full packet, but we have more symbols to go, so
          // get the next symbol size.
          next_pkt_size        <= 'X;
          next_pkt_eob         <= 'X;
          o_symbol_fifo_tready <= 1'b1;
          state                <= CALC_ITEMS_ST;
        end
      end

      CALC_VECTOR_ST : begin
        // Calculate where we are in the current vector
        vect_item_count <= vect_item_count + next_pkt_size;
        state           <= CALC_EOV_ST;
      end

      CALC_EOV_ST : begin
        // Calculate if EOV flag should be set for this packet
        next_pkt_eov  <= (vect_item_count[FFT_SIZE_MASK_W-1:0] & fft_size_mask) == 0;
        // Check if the packet is a single transfer
        next_pkt_last <= NIPC >= next_pkt_size;
        state         <= PASS_PACKET_ST;
      end

      PASS_PACKET_ST : begin
        if (!EN_TIME_ALL_PKTS) begin
          has_time <= 1'b0;
        end
        if (o_noc_tvalid && o_noc_tready) begin
          if (EN_TIME_ALL_PKTS) begin
            timestamp <= timestamp + NIPC;
          end
          items_to_send  <= items_to_send - NIPC;
          pkt_item_count <= pkt_item_count + NIPC;
          next_pkt_last  <= pkt_item_count >= next_pkt_size;

          if (o_noc_tlast) begin
            if (o_noc_teob) begin
              state <= WAIT_BURST_ST;
            end else begin
              state <= CALC_PACKET_ST;
            end
          end
        end
      end
    endcase

    if (rst) begin
      state                <= WAIT_BURST_ST;
      i_burst_tready       <= 1'b0;
      o_symbol_fifo_tready <= 1'b0;
      items_to_send        <= 'X;
      pkt_size             <= 'X;
      timestamp            <= 'X;
      has_time             <= 'X;
      last_symbol          <= 'X;
      next_pkt_last        <= 'X;
      next_pkt_size        <= 'X;
      next_pkt_eob         <= 'X;
      next_pkt_eov         <= 'X;
      next_pkt_timestamp   <= 'X;
      next_pkt_has_time    <= 'X;
      pkt_item_count       <= 'X;
      vect_item_count      <= 'X;
    end
  end


  //---------------------------------------------------------------------------
  // Data Pass-through Logic
  //---------------------------------------------------------------------------

  assign o_noc_tdata      = o_fft_tdata;
  assign o_noc_tkeep      = o_fft_tkeep;
  assign o_noc_tlast      = next_pkt_last;
  assign o_noc_tlength    = next_pkt_size * (ITEM_W/8);  // Convert to bytes
  assign o_noc_teob       = next_pkt_eob;
  assign o_noc_teov       = next_pkt_eov;
  assign o_noc_ttimestamp = next_pkt_timestamp;
  assign o_noc_thas_time  = next_pkt_has_time;
  assign o_noc_tvalid     = (state == PASS_PACKET_ST) ? o_fft_tvalid : 1'b0;
  assign o_fft_tready     = (state == PASS_PACKET_ST) ? o_noc_tready : 1'b0;

endmodule : fft_depacketize


`default_nettype wire

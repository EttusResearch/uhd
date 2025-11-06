//
// Copyright 2025 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: fft_packetize_tb
//
// Description:
//
//   Testbench for fft_packetize and fft_depacketize.
//
// Parameters:
//
//   NIPC              : Number of items per clock cycle on DUT
//   NUM_CHAN          : Number of channels to configure for DUT
//   EN_CP_REMOVAL     : Enable cyclic-prefix removal on DUT and test it
//   EN_CP_INSERTION   : Enable cyclic-prefix insertion on DUT and test it
//   EN_DATA_FIFOS     : Enable data FIFOs in the DUT
//   EN_TIME_ALL_PKTS  : Enabled timestamps on all packets in DUT
//   MAX_PKT_SIZE_LOG2 : Maximum packet size to support
//   MAX_FFT_SIZE_LOG2 : Maximum FFT size to support
//

`default_nettype none


module fft_packetize_tb #(
  int NIPC              = 1,
  int NUM_CHAN          = 1,
  bit EN_CP_REMOVAL     = 1,
  bit EN_CP_INSERTION   = 1,
  bit EN_DATA_FIFOS     = 1,
  bit EN_TIME_ALL_PKTS  = 1,
  int MAX_PKT_SIZE_LOG2 = 8,
  int MAX_FFT_SIZE_LOG2 = 8
);

  // Include macros and time declarations for use with PkgTestExec
  `include "test_exec.svh"
  import PkgTestExec::*;

  import rfnoc_chdr_utils_pkg::*;
  import PkgAxiStreamBfm::*;
  import PkgRandom::*;
  import PkgChdrData::*;

  `include "usrp_utils.svh"

  import fft_packetize_pkg::*;

  localparam real CLK_PERIOD = 10.0;
  localparam int  STALL_PROB = 25;
  localparam bit  VERBOSE    = 0;

  localparam int ITEM_W               = 32;
  localparam int DATA_W               = ITEM_W*NIPC;
  localparam int CP_LEN_W             = MAX_FFT_SIZE_LOG2;
  localparam int CP_FIFO_SIZE_LOG2    = 5;
  localparam int BURST_FIFO_SIZE_LOG2 = 5;
  localparam int SYMB_FIFO_SIZE_LOG2  = 5;

  localparam int PKT_SIZE_W      = MAX_PKT_SIZE_LOG2 + 1;
  localparam int FFT_SIZE_W      = MAX_FFT_SIZE_LOG2 + 1;
  localparam int FFT_SIZE_LOG2_W = $clog2(MAX_FFT_SIZE_LOG2 + 1);

  // Define parameters for packet randomization
  localparam int MIN_FFT_SIZE_LOG2 = 3;  // Same as Xilinx FFT core
  localparam int MIN_FFT_SIZE      = 2**MIN_FFT_SIZE_LOG2;
  localparam int MAX_FFT_SIZE      = 2**MAX_FFT_SIZE_LOG2;
  localparam int MAX_CP_LEN        = 2**MAX_FFT_SIZE_LOG2-1;
  localparam int MAX_NUM_MDATA     = 4;

  typedef struct packed {
    logic [CHDR_TIMESTAMP_W-1:0] timestamp;
    logic                        has_time;
    logic                        eob;
    logic                        eov;
    logic [   CHDR_LENGTH_W-1:0] length;
  } sideband_t;

  localparam int SB_W = $bits(sideband_t);


  //---------------------------------------------------------------------------
  // Clocks and Resets
  //---------------------------------------------------------------------------

  bit clk;
  bit rst;

  sim_clock_gen #(.PERIOD(CLK_PERIOD), .AUTOSTART(0))
    clk_gen (.clk(clk), .rst(rst));


  //---------------------------------------------------------------------------
  // Bus Functional Models
  //---------------------------------------------------------------------------

  // Interfaces for DUT
  AxiStreamIf #(DATA_W, SB_W) i_noc    (clk, rst);
  AxiStreamIf #(DATA_W, SB_W) o_noc    (clk, rst);
  AxiStreamIf #(DATA_W)       i_fft    (clk, rst);
  AxiStreamIf #(DATA_W)       o_fft    (clk, rst);
  AxiStreamIf #(CP_LEN_W)     i_cp_rem (clk, rst);
  AxiStreamIf #(CP_LEN_W)     i_cp_ins (clk, rst);
  AxiStreamIf #(CP_LEN_W)     o_cp_rem (clk, rst);
  AxiStreamIf #(CP_LEN_W)     o_cp_ins (clk, rst);

  // AXI-Stream BFMs
  AxiStreamBfm #(DATA_W, SB_W) noc_bfm    = new(i_noc, o_noc);
  AxiStreamBfm #(DATA_W)       fft_bfm    = new(i_fft, o_fft);
  AxiStreamBfm #(CP_LEN_W)     cp_rem_bfm = new(i_cp_rem, o_cp_rem);
  AxiStreamBfm #(CP_LEN_W)     cp_ins_bfm = new(i_cp_ins, o_cp_ins);

  typedef AxiStreamBfm #(DATA_W, SB_W)::AxisPacket_t noc_pkt_t;
  typedef AxiStreamBfm #(DATA_W)::AxisPacket_t       fft_pkt_t;
  typedef AxiStreamBfm #(CP_LEN_W)::AxisPacket_t     cp_len_pkt_t;


  //---------------------------------------------------------------------------
  // Device Under Test (DUT)
  //---------------------------------------------------------------------------

  logic [FFT_SIZE_LOG2_W-1:0] fft_size_log2;

  sideband_t i_noc_sb, o_noc_sb;

  logic [NIPC-1:0] i_noc_tkeep, o_noc_tkeep;
  logic [NIPC-1:0] i_fft_tkeep, o_fft_tkeep;

  burst_info_t burst_tdata;
  logic        burst_tvalid;
  logic        burst_tready;

  symbol_info_t symbol_tdata;
  logic         symbol_tvalid;
  logic         symbol_tready;

  logic [CP_LEN_W-1:0] i_cp_rem_tdata;
  logic [CP_LEN_W-1:0] i_cp_ins_tdata;

  // Make the default CP length 0, to match behavior of axis_cp_list.
  assign i_cp_rem_tdata = i_cp_rem.tvalid ? i_cp_rem.tdata : '0;
  assign i_cp_ins_tdata = i_cp_ins.tvalid ? i_cp_ins.tdata : '0;

  // Make an array for the data, where each element corresponds to one channel.
  logic [NUM_CHAN-1:0][DATA_W-1:0] i_noc_ch_tdata;
  logic [NUM_CHAN-1:0][DATA_W-1:0] o_fft_ch_tdata;
  logic [NUM_CHAN-1:0][DATA_W-1:0] i_fft_ch_tdata;
  logic [NUM_CHAN-1:0][DATA_W-1:0] o_noc_ch_tdata;

  fft_packetize #(
    .ITEM_W              (ITEM_W              ),
    .NIPC                (NIPC                ),
    .NUM_CHAN            (NUM_CHAN            ),
    .EN_CP_REMOVAL       (EN_CP_REMOVAL       ),
    .MAX_PKT_SIZE_LOG2   (MAX_PKT_SIZE_LOG2   ),
    .MAX_FFT_SIZE_LOG2   (MAX_FFT_SIZE_LOG2   ),
    .CP_FIFO_SIZE_LOG2   (CP_FIFO_SIZE_LOG2   ),
    .BURST_FIFO_SIZE_LOG2(BURST_FIFO_SIZE_LOG2),
    .SYMB_FIFO_SIZE_LOG2 (SYMB_FIFO_SIZE_LOG2 )
  ) fft_packetize_dut (
    .clk             (clk               ),
    .rst             (rst               ),
    .fft_size_log2   (fft_size_log2     ),
    .i_cp_rem_tdata  (i_cp_rem_tdata    ),
    .i_cp_rem_tvalid (i_cp_rem.tvalid   ),
    .i_cp_rem_tready (i_cp_rem.tready   ),
    .o_cp_rem_tdata  (o_cp_rem.tdata    ),
    .o_cp_rem_tvalid (o_cp_rem.tvalid   ),
    .o_cp_rem_tready (o_cp_rem.tready   ),
    .i_noc_tdata     (i_noc_ch_tdata    ),
    .i_noc_tkeep     (i_noc_tkeep       ),
    .i_noc_tlast     (i_noc.tlast       ),
    .i_noc_tvalid    (i_noc.tvalid      ),
    .i_noc_tready    (i_noc.tready      ),
    .i_noc_ttimestamp(i_noc_sb.timestamp),
    .i_noc_thas_time (i_noc_sb.has_time ),
    .i_noc_tlength   (i_noc_sb.length   ),
    .i_noc_teov      (i_noc_sb.eov      ),
    .i_noc_teob      (i_noc_sb.eob      ),
    .o_fft_tdata     (o_fft_ch_tdata    ),
    .o_fft_tkeep     (o_fft_tkeep       ),
    .o_fft_tlast     (o_fft.tlast       ),
    .o_fft_tvalid    (o_fft.tvalid      ),
    .o_fft_tready    (o_fft.tready      ),
    .o_burst_tdata   (burst_tdata       ),
    .o_burst_tvalid  (burst_tvalid      ),
    .o_burst_tready  (burst_tready      ),
    .o_symbol_tdata  (symbol_tdata      ),
    .o_symbol_tvalid (symbol_tvalid     ),
    .o_symbol_tready (symbol_tready     )
  );

  fft_depacketize #(
    .ITEM_W             (ITEM_W             ),
    .NIPC               (NIPC               ),
    .NUM_CHAN           (NUM_CHAN           ),
    .EN_CP_INSERTION    (EN_CP_INSERTION    ),
    .MAX_PKT_SIZE_LOG2  (MAX_PKT_SIZE_LOG2  ),
    .MAX_FFT_SIZE_LOG2  (MAX_FFT_SIZE_LOG2  ),
    .CP_FIFO_SIZE_LOG2  (CP_FIFO_SIZE_LOG2  ),
    .SYMB_FIFO_SIZE_LOG2(SYMB_FIFO_SIZE_LOG2),
    .EN_TIME_ALL_PKTS   (EN_TIME_ALL_PKTS   )
  ) fft_depacketize_dut (
    .clk             (clk               ),
    .rst             (rst               ),
    .fft_size_log2   (fft_size_log2     ),
    .i_cp_ins_tdata  (i_cp_ins_tdata    ),
    .i_cp_ins_tvalid (i_cp_ins.tvalid   ),
    .i_cp_ins_tready (i_cp_ins.tready   ),
    .o_cp_ins_tdata  (o_cp_ins.tdata    ),
    .o_cp_ins_tvalid (o_cp_ins.tvalid   ),
    .o_cp_ins_tready (o_cp_ins.tready   ),
    .i_burst_tdata   (burst_tdata       ),
    .i_burst_tvalid  (burst_tvalid      ),
    .i_burst_tready  (burst_tready      ),
    .i_symbol_tdata  (symbol_tdata      ),
    .i_symbol_tvalid (symbol_tvalid     ),
    .i_symbol_tready (symbol_tready     ),
    .i_fft_tdata     (i_fft_ch_tdata    ),
    .i_fft_tkeep     (i_fft_tkeep       ),
    .i_fft_tlast     (i_fft.tlast       ),
    .i_fft_tvalid    (i_fft.tvalid      ),
    .i_fft_tready    (i_fft.tready      ),
    .o_noc_tdata     (o_noc_ch_tdata    ),
    .o_noc_tkeep     (o_noc_tkeep       ),
    .o_noc_tlast     (o_noc.tlast       ),
    .o_noc_tvalid    (o_noc.tvalid      ),
    .o_noc_tready    (o_noc.tready      ),
    .o_noc_ttimestamp(o_noc_sb.timestamp),
    .o_noc_thas_time (o_noc_sb.has_time ),
    .o_noc_tlength   (o_noc_sb.length   ),
    .o_noc_teov      (o_noc_sb.eov      ),
    .o_noc_teob      (o_noc_sb.eob      )
  );

  // Resize tkeep, since the BFM uses bytes, but RFNoC uses words
  always_comb begin
    i_noc_tkeep = i_noc.tkeep[NIPC-1:0];
    o_noc.tkeep = '0;
    o_noc.tkeep[NIPC-1:0] = o_noc_tkeep;
    o_fft.tkeep = '0;
    o_fft.tkeep[NIPC-1:0] = o_fft_tkeep;
    i_fft_tkeep = i_fft.tkeep[NIPC-1:0];
  end

  // Assign sideband to/from a struct for easier field parsing
  assign i_noc_sb    = i_noc.tuser;
  assign o_noc.tuser = o_noc_sb;

  // Add TLAST to BFM interfaces that are missing it
  assign o_cp_rem.tlast = o_cp_rem.tvalid;
  assign o_cp_ins.tlast = o_cp_ins.tvalid;


  //---------------------------------------------------------------------------
  // Handle Multiple Channels
  //---------------------------------------------------------------------------
  //
  // All channels are required to have the exact same timing (since they share
  // the same AXI-Stream control signals). To test multiple channels, the
  // testbench ensures that the value in each channel is the same as the
  // previous channel's value plus 1. In this section, we create this input
  // data for each channel and verify that the output data always follows this
  // rule. The rest of the testbench will ensure that channel 0 is correct. If
  // channel 0 is correct, and all other channels follow channel 0, then we
  // assume the other channels are correct.
  //
  //---------------------------------------------------------------------------

  if (NUM_CHAN == 1) begin : one_channel
    assign i_noc_ch_tdata = i_noc.tdata;
    assign o_fft.tdata    = o_fft_ch_tdata;
    assign i_fft_ch_tdata = i_fft.tdata;
    assign o_noc.tdata    = o_noc_ch_tdata;
  end else begin : multi_channel
    // Pass channel 0 to the testbench checkers
    assign o_fft.tdata = o_fft_ch_tdata[0];
    assign o_noc.tdata = o_noc_ch_tdata[0];

    // Give each channel unique data, based on the first channel's data.
    always_comb foreach(i_noc_ch_tdata[i]) begin
      i_noc_ch_tdata[i] = i_noc.tdata + i;
      i_fft_ch_tdata[i] = i_fft.tdata + i;
    end

    // The other channels should always correspond to the data in the first
    // channel.
    always_ff @(posedge clk) begin
      if (o_fft.tvalid && o_fft.tready) begin
        for (int i = 1; i < NUM_CHAN; i++) begin
          // Use === since output data will be "don't care" in the case where
          // the packetizer needs to insert data.
          `ASSERT_ERROR(o_fft_ch_tdata[i] === o_fft_ch_tdata[0] + i,
            $sformatf({
              "FFT data in ch %0d doesn't correspond to ch 0. ",
              "Expected 0x%X, received 0x%X"},
              i, o_fft_ch_tdata[0] + i, o_fft_ch_tdata[i]));
        end
      end
      if (o_noc.tvalid && o_noc.tready) begin
        for (int i = 1; i < NUM_CHAN; i++) begin
          `ASSERT_ERROR(o_noc_ch_tdata[i] == o_noc_ch_tdata[0] + i,
            $sformatf({
              "NoC data in ch %0d doesn't correspond to ch 0. ",
              "Expected 0x%X, received 0x%X"},
              i, o_noc_ch_tdata[0] + i, o_noc_ch_tdata[i]));
        end
      end
    end
  end


  //---------------------------------------------------------------------------
  // FFT Model
  //---------------------------------------------------------------------------
  //
  // This model reads the o_fft output and generates the i_fft input. Because
  // we're dealing with FFT packets, each packet corresponds to one symbol,
  // which may or may not include a cyclic prefix.
  //
  // To model the FFT, we only care about the packet lengths and not the actual
  // FFT calculation. To make things simple, we expect the data from the DUT
  // (o_fft) to be incrementing, restarting the count for each symbol/packet.
  // For the FFT result we generate here (for i_fft), we ensure that it
  // increments in the same fashion, and we make sure that the packets are the
  // correct size, including adding/removing the cyclic prefix length as
  // required.
  //
  //---------------------------------------------------------------------------

  // To communicate the number of items that was input into the DUT on i_noc.
  mailbox #(int) items_sent_mb = new();

  // To communicate the cyclic prefix to be removed from each symbol on o_fft.
  mailbox #(int) cp_rem_mb = new();

  // To communicate the cyclic prefix to be inserted for each symbol on i_fft.
  mailbox #(int) cp_ins_mb = new();

  initial begin
    fft_pkt_t          in_pkt;
    fft_pkt_t          out_pkt;
    int                cp_rem_len;
    int                cp_ins_len;
    int                fft_size;
    int                num_items;
    int                items_sent;
    logic [ITEM_W-1:0] items [$];
    int                item_count;

    // Wait a bit for the BFMs to start
    @(posedge clk);
    forever begin
      item_count = 0;

      // Get the next output packet from o_fft
      fft_bfm.get(in_pkt);
      items_sent_mb.get(items_sent);
      fft_size = 2**fft_size_log2;

      // Check for CP removal
      if (EN_CP_REMOVAL) begin
        cp_len_pkt_t cp_pkt;
        int cp_len_exp;
        cp_rem_len = 0;
        cp_rem_bfm.get(cp_pkt);
        cp_rem_len = cp_pkt.data[0];
        // If the testbench didn't put anything in the mailbox, then we're not
        // doing CP removal and the result should be 0.
        if (cp_rem_mb.num() > 0) begin
          cp_rem_mb.get(cp_len_exp);
        end else begin
          cp_len_exp = 0;
        end
        `ASSERT_ERROR(cp_rem_len == cp_len_exp,
          $sformatf("o_cp_rem: CP removal length doesn't match. Expected: %0d, Received: %0d",
            cp_len_exp, cp_rem_len
          )
        );
      end

      // Check for CP insertion
      if (EN_CP_INSERTION) begin
        cp_len_pkt_t cp_pkt;
        int cp_len_exp;
        cp_ins_len = 0;
        cp_ins_bfm.get(cp_pkt);
        cp_ins_len = cp_pkt.data[0];
        // If the testbench didn't put anything in the mailbox, then we're not
        // doing CP insertion and the result should be 0.
        if (cp_ins_mb.num() > 0) begin
          cp_ins_mb.get(cp_len_exp);
        end else begin
          cp_len_exp = 0;
        end
        `ASSERT_ERROR(cp_ins_len == cp_len_exp,
          $sformatf("o_cp_ins: CP insertion length doesn't match. Expected: %0d, Received: %0d",
            cp_len_exp, cp_ins_len
          )
        );
      end

      // Check the input packet length
      num_items = (in_pkt.data.size()-1)*NIPC + keep_to_trailing(in_pkt.keep[$]);
      `ASSERT_ERROR(num_items == fft_size + cp_rem_len,
        $sformatf("o_fft: Input to FFT model is not the expected size. Expected %0d, Actual %0d",
          fft_size + cp_rem_len, num_items)
      );

      // Check input data values
      items = ChdrData#(DATA_W, ITEM_W)::chdr_to_item(in_pkt.data);
      foreach (items[idx]) begin
        if (idx < items_sent) begin
          // We only expect valid data on o_fft for samples that were input.
          // Any extra generated by the DUT to finish the last symbol will have
          // an undefined value.
          `ASSERT_ERROR(items[idx] == item_count,
            $sformatf("o_fft: Unexpected FFT value at index %0d. Expected %X, Actual %X",
              idx, item_count, items[idx])
          );
        end
        item_count++;
      end

      // Generate the resulting packet for i_fft
      num_items = fft_size + cp_ins_len;
      items = {};
      for (int count = 0; count < num_items; count++) begin
        items.push_back(count);
      end
      out_pkt = new();
      out_pkt.data = ChdrData#(DATA_W, ITEM_W)::item_to_chdr(items);
      foreach(out_pkt.data[idx]) begin
        out_pkt.keep[idx] = {NIPC{1'b1}};
      end
      out_pkt.keep[$] = trailing_to_keep(num_items % NIPC);

      // Send the output packet
      fft_bfm.put(out_pkt);
    end
  end


  //---------------------------------------------------------------------------
  // Debug Monitor
  //---------------------------------------------------------------------------
  //
  // This adds some additional prints statements (if VERBOSE is set) about what
  // is observed going into and leaving the DUT. This is helpful for debugging.
  //
  //---------------------------------------------------------------------------

  generate

    // Print what's going into i_noc
    begin : input_monitor
      bit sop = 1;  // Start of packet
      int num_items;
      bit eob;
      bit has_time;
      int item_count = 0;

      always_ff @(posedge clk) begin
        if (i_cp_ins.tvalid && i_cp_ins.tready) begin
          if (VERBOSE) $display("i_cp_ins: %0d", i_cp_ins.tdata);
        end

        if (i_cp_rem.tvalid && i_cp_rem.tready) begin
          if (VERBOSE) $display("i_cp_rem: %0d", i_cp_rem.tdata);
        end

        if (i_noc.tvalid && i_noc.tready) begin
          if (sop) begin
            sideband_t sb;
            sb        = i_noc.tuser;
            num_items = sb.length / (ITEM_W/8);
            eob       = sb.eob;
            has_time  = sb.has_time;
            if (VERBOSE) $display("i_noc: num_items=%0d, eob=%0d, has_time=%0d",
              num_items, eob, has_time);
            assert (num_items > 0) else $fatal(1, "Incorrect i_noc payload size");
          end

          if (i_noc.tlast) begin
            item_count += keep_to_trailing(i_noc.tkeep);
            `ASSERT_ERROR(item_count == num_items,
              $sformatf("i_noc: Incorrect packet length. Expected %0d, Actual %0d",
                num_items, item_count)
            );
            `ASSERT_ERROR(i_noc.tkeep == trailing_to_keep(num_items % NIPC),
              "i_noc: Incorrect tkeep");
            item_count = 0;
          end else begin
            item_count += NIPC;
            `ASSERT_ERROR(i_noc.tkeep == {NIPC{1'b1}}, "i_noc: Incorrect tkeep");
          end

          sop = i_noc.tlast;
        end
      end
    end : input_monitor

    // Print what's coming out of o_noc
    begin : output_monitor
      bit sop = 1;  // Start of packet
      int num_items;
      bit eob, eov;
      bit has_time;
      int item_count = 0;

      always_ff @(posedge clk) begin
        if (o_noc.tvalid && o_noc.tready) begin
          if (sop) begin
            sideband_t sb;
            sb        = o_noc.tuser;
            num_items = sb.length / (ITEM_W/8);
            eob       = sb.eob;
            eov       = sb.eov;
            has_time  = sb.has_time;
            if (VERBOSE) $display("%s o_noc: num_items=%0d, eob=%0d, eov=%0d, has_time=%0d",
              {49{" "}}, num_items, eob, eov, has_time);
            `ASSERT_ERROR(num_items > 0, "Incorrect o_noc payload size");
          end

          if (o_noc.tlast) begin
            item_count += keep_to_trailing(o_noc.tkeep);
            `ASSERT_ERROR(item_count == num_items,
              $sformatf("o_noc: Incorrect packet length. Expected: %0d, Actual: %0d",
                num_items, item_count));
            item_count = 0;
          end else begin
            item_count += NIPC;
            `ASSERT_ERROR(o_noc.tkeep == {NIPC{1'b1}}, "o_noc: Incorrect tkeep");
          end

          sop = o_noc.tlast;
        end
      end
    end : output_monitor

  endgenerate


  //---------------------------------------------------------------------------
  // Helper Functions
  //---------------------------------------------------------------------------

  // Translate from TKEEP to number of trailing items
  function automatic int keep_to_trailing(logic [NIPC-1:0] keep);
    int items = 0;
    for(int idx = 0; idx < NIPC ; idx++) begin
      if (keep[idx]) items = idx+1;
    end
    return items;
  endfunction : keep_to_trailing


  // Translate from number of trailing items to TKEEP
  function automatic logic [NIPC-1:0] trailing_to_keep(int items);
    logic [NIPC-1:0] keep = '1;
    if (items != 0) begin
      foreach(keep[idx]) begin
        keep[idx] = items > idx;
      end
    end
    return keep;
  endfunction : trailing_to_keep


  // Compare the sideband information between what was expected and what was
  // actually received.
  //
  //   sb_exp     : What was expected
  //   sb_act     : The actual result received
  //   pkt_count  : The packet number for the sideband information being checked
  //   line_ionfo : A string indicating which line the check was called from
  //
  function automatic void check_sideband(
    sideband_t sb_exp,
    sideband_t sb_act,
    int        pkt_count,
    string     line_info
  );
    `ASSERT_ERROR(
      sb_exp.has_time == sb_act.has_time,
      $sformatf("Has-time mismatch. Expected: %X, Actual: %X, Packet: %0d, Line: %s",
        sb_exp.has_time, sb_act.has_time, pkt_count, line_info
      )
    );
    if (sb_exp.has_time && sb_act.has_time) begin
      `ASSERT_ERROR(
        sb_exp.timestamp == sb_act.timestamp,
        $sformatf("Timestamp mismatch. Expected: 0x%X, Actual: 0x%X, Packet: %0d, Line: %s",
          sb_exp.timestamp, sb_act.timestamp, pkt_count, line_info
        )
      );
    end
    `ASSERT_ERROR(
      sb_exp.length == sb_act.length,
      $sformatf("Length mismatch. Expected: %0d, Actual: %0d, Packet: %0d, Line: %s",
        sb_exp.length, sb_act.length, pkt_count, line_info
      )
    );
    `ASSERT_ERROR(
      sb_exp.eob == sb_act.eob,
      $sformatf("EOB mismatch. Expected: %X, Actual: %X, Packet: %0d, Line: %s",
        sb_exp.eob, sb_act.eob, pkt_count, line_info
      )
    );
    `ASSERT_ERROR(
      sb_exp.eov == sb_act.eov,
      $sformatf("EOV mismatch. Expected: %X, Actual: %X, Packet: %0d, Line: %s",
        sb_exp.eov, sb_act.eov, pkt_count, line_info
      )
    );
  endfunction : check_sideband


  // Returns a random number in the range [min, max] that is also a multiple of
  // NIPC.
  function automatic int unsigned round_rand_range(int unsigned min, int unsigned max);
    return ($urandom_range(min, max) / NIPC) * NIPC;
  endfunction : round_rand_range


  //---------------------------------------------------------------------------
  // Tests
  //---------------------------------------------------------------------------

  // Runs a single test of one or more bursts.
  //
  //   num_bursts  : Number of bursts to generate for this test.
  //   num_ffts    : Number of FFTs in each burst.
  //   fft_size    : FFT size to use for this test.
  //   do_cp_rem   : Indicates whether to add random cyclic prefix removal
  //   do_cp_ins   : Indicates whether to add random cyclic prefix insertion
  //   pkt_size    : Size of CHDR payload in samples (-1 means choose a
  //                 random size).
  //   has_time    : Indicates whether the test should used timed packets (-1
  //                 means choose randomly)
  //   add_partial : Whether or not to test adding a partial symbol at the end.
  //                 (-1 means choose randomly).
  //
  task automatic test_bursts(
    int num_bursts,
    int num_ffts,
    int fft_size,
    bit do_cp_rem,
    bit do_cp_ins,
    int pkt_size    = -1,
    int has_time    = -1,
    int add_partial = -1
  );
    int pkt_size_setting = pkt_size;
    bit has_time_setting = has_time > 0;
    bit rand_pkt_size    = (pkt_size < 0);
    bit rand_has_time    = (has_time < 0);
    bit rand_partial     = (add_partial < 0);

    typedef struct packed {
      int num_items;
      int num_symbols;
      int total_cp_rem;
      int total_cp_ins;
      sideband_t sideband;
    } sim_burst_t;

    sim_burst_t sim_burst;

    mailbox #(sim_burst_t) sim_burst_mb = new();

    `ASSERT_FATAL(!do_cp_rem || !do_cp_ins,
      "Cannot remove and insert CP at the same time");
    `ASSERT_FATAL(!do_cp_rem || EN_CP_REMOVAL,
      "Cannot do CP removal when EN_CP_REMOVAL is false");
    `ASSERT_FATAL(!do_cp_ins || EN_CP_INSERTION,
      "Cannot do CP insertion when EN_CP_INSERTION is false");
    `ASSERT_FATAL(2**$clog2(fft_size) == fft_size,
      "FFT size is not a power of 2");
    `ASSERT_FATAL(fft_size >= MIN_FFT_SIZE && fft_size <= MAX_FFT_SIZE,
      "FFT size out of range");

    if (VERBOSE) begin
      // Print information about the test being run
      $display({
        "test_bursts: num_bursts=%0d, num_ffts=%0d, fft_size=%0d, ",
        "do_cp_rem=%b, do_cp_ins=%b, ",
        "pkt_size=%0d, has_time=%0d, add_partial=%0d"},
        num_bursts, num_ffts, fft_size, do_cp_rem, do_cp_ins, pkt_size,
        has_time, add_partial
      );
    end

    fork

      //-----------------------------------------------------------------------
      // Input Packet Generator
      //-----------------------------------------------------------------------

      begin : input_generator
        sideband_t sideband;      // Packet sideband information
        int burst_items_to_send;  // Number of items to send to i_noc.
        int burst_items_sent;     // Number of items we've sent to i_noc.
        int total_cp_len;         // Total cyclic prefix length (sum of all
                                  // prefixes).
        bit eob;                  // End of burst flag.
        int symb_items_sent;      // Number of items we've sent in the current
                                  // symbol.
        int symb_len_list [$];    // List of the symbol lengths for this burst.
        int partial_items;        // Number of extra items we're adding to make
                                  // it not an even number of symbols.
        int missing_items;        // Number of items after partial_items needed
                                  // to complete the symbol.

        // The FFT size is constant for the duration of the burst. The DUT
        // requires a couple cycles to capture FFT size before data arrives.
        fft_size_log2 = $clog2(fft_size);
        clk_gen.clk_wait_r(3);

        for (int burst_count = 0; burst_count < num_bursts; burst_count++) begin
          burst_items_to_send = 0;
          burst_items_sent    = 0;
          total_cp_len        = 0;
          eob                 = 0;
          symb_items_sent     = 0;
          symb_len_list       = {};
          partial_items       = 0;
          missing_items       = 0;

          // Decide if this burst is going to have a partial symbol
          if (rand_partial) add_partial = $urandom_range(0, 1);

          // Figure out how much data we're going to send
          if (do_cp_rem && EN_CP_REMOVAL) begin
            for (int fft_count = 0; fft_count < num_ffts + add_partial; fft_count++) begin
              int cp_len;
              cp_len_pkt_t cp_len_pkt = new();
              cp_len = round_rand_range(0, `MIN(fft_size-1, MAX_CP_LEN));
              total_cp_len += cp_len;
              cp_rem_mb.put(cp_len);
              cp_len_pkt.data.push_back(cp_len);
              cp_rem_bfm.put(cp_len_pkt);
              if (fft_count == num_ffts) begin
                partial_items = round_rand_range(NIPC, fft_size + cp_len);
                missing_items = fft_size + cp_len - partial_items;
                burst_items_to_send += partial_items;
                symb_len_list.push_back(partial_items);
                items_sent_mb.put(partial_items);
              end else begin
                burst_items_to_send += (fft_size + cp_len);
                symb_len_list.push_back(fft_size + cp_len);
                items_sent_mb.put(fft_size + cp_len);
              end
            end
          end else if (do_cp_ins && EN_CP_INSERTION) begin
            for (int fft_count = 0; fft_count < num_ffts + add_partial; fft_count++) begin
              int cp_len;
              cp_len_pkt_t cp_len_pkt = new();
              cp_len = round_rand_range(0, `MIN(fft_size-1, MAX_CP_LEN));
              total_cp_len += cp_len;
              cp_ins_mb.put(cp_len);
              cp_len_pkt.data.push_back(cp_len);
              cp_ins_bfm.put(cp_len_pkt);
              if (fft_count == num_ffts) begin
                partial_items = round_rand_range(NIPC, fft_size);
                missing_items = fft_size - partial_items;
                burst_items_to_send += partial_items;
                symb_len_list.push_back(partial_items);
                items_sent_mb.put(partial_items);
              end else begin
                burst_items_to_send += fft_size;
                symb_len_list.push_back(fft_size);
                items_sent_mb.put(fft_size);
              end
            end
          end else begin
            repeat (num_ffts) begin
              symb_len_list.push_back(fft_size);
              items_sent_mb.put(fft_size);
            end
            if (add_partial) begin
              partial_items = round_rand_range(NIPC, fft_size);
              missing_items = fft_size - partial_items;
              symb_len_list.push_back(partial_items);
              items_sent_mb.put(partial_items);
            end
            burst_items_to_send = fft_size * num_ffts + partial_items;
          end

          // Send total number of items we're sending, plus the amount needed
          // to bring it up to a whole number of symbols.
          sim_burst.num_items  = burst_items_to_send + missing_items;
          sim_burst.num_symbols = num_ffts + add_partial;
          sim_burst.total_cp_rem = do_cp_rem ? total_cp_len : 0;
          sim_burst.total_cp_ins = do_cp_ins ? total_cp_len : 0;

          if (VERBOSE) begin
            if (do_cp_rem) begin
              $display({"i_noc: Burst of %0d items, %0d are partial items ",
                "(%0d to be added by DUT), %0d CP to be removed"},
                burst_items_to_send, partial_items, missing_items, total_cp_len);
            end else if (do_cp_ins) begin
              $display({"i_noc: Burst of %0d items, %0d are partial items ",
                "(%0d to be added by DUT), %0d CP to be inserted"},
                burst_items_to_send, total_cp_len, missing_items, partial_items);
            end else begin
              $display({"i_noc: Burst of %0d items, %0d are partial items ",
                "(%0d to be added by DUT; no ins/rem)"},
                burst_items_to_send, partial_items, missing_items);
            end
          end

          // Decide if this burst is timed or not
          if (rand_has_time) has_time = $urandom_range(0, 1);
          else has_time = has_time_setting;

          // At this point we know what we're going to send. Now, we generate
          // the packets and send them, one packet per loop iteration.
          forever begin
            chdr_pkt_type_t pkt_type;
            noc_pkt_t pkt;
            logic [ITEM_W-1:0] items [$];

            pkt = new();

            if (rand_pkt_size) pkt_size = round_rand_range(NIPC, 2**MAX_PKT_SIZE_LOG2);
            else pkt_size = (pkt_size_setting / NIPC) * NIPC;

            // Check if we've reached the end of the burst
            if (burst_items_sent + pkt_size >= burst_items_to_send) begin
              pkt_size = burst_items_to_send - burst_items_sent;
              eob = 1;
            end

            // Build the header word
            sideband = 'X;
            sideband.length    = pkt_size * (ITEM_W/8);
            sideband.eob       = eob;
            sideband.eov       = 'X;
            sideband.timestamp = 'X;
            if (burst_items_sent == 0) begin
              // Start of burst, so set the time for the first packet
              if (has_time) begin
                sideband.timestamp = Rand#(CHDR_TIMESTAMP_W)::rand_bit();
                sideband.has_time  = 1;
              end else begin
                sideband.has_time  = 0;
              end
              sim_burst.sideband = sideband;

              // Send the information needed to verify the results of this
              // burst to the checker.
              sim_burst_mb.put(sim_burst);
            end else begin
              // This is not the start of a burst, so we randomly include the
              // time to make sure it's correctly ignored by the DUT.
              sideband.has_time  = $urandom_range(0, 1);
            end

            // Generate samples. Each item should increment by 1, starting from
            // 0 at the start of each symbol.
            for (int count = 0; count < pkt_size; count++) begin
              items.push_back(symb_items_sent++);
              if (symb_items_sent == symb_len_list[0]) begin
                // We've finished a symbol, so restart our count
                void'(symb_len_list.pop_front());
                symb_items_sent = 0;
              end
            end

            // Resize to data width
            pkt.data = ChdrData#(DATA_W, ITEM_W)::item_to_chdr(items);

            // Add sideband and tkeep
            foreach (pkt.data[idx]) begin
              pkt.user.push_back(sideband);
              pkt.keep.push_back({NIPC{1'b1}});
            end
            pkt.keep[$] = trailing_to_keep(pkt_size % NIPC);

            // Send this packet to the DUT
            noc_bfm.put(pkt);
            burst_items_sent += pkt_size;

            // Check if we've just sent the last packet of this burst
            if (eob) break;
          end
        end
      end : input_generator

      //-----------------------------------------------------------------------
      // Output Packet Checker
      //-----------------------------------------------------------------------

      begin : output_checker
        noc_pkt_t   pkt_rcvd;          // Packet we received
        sideband_t  sb_exp;            // Sideband information we expect
        sideband_t  sb_rcvd;           // Sideband information we received
        int         pyld_size;         // Payload size of current packet
        int         max_pyld_size;     // Max payload size we expect
        int         burst_items_rcvd;  // Items received so far this burst
        sim_burst_t sim_burst;         // Information about the burst we expect

        for (int burst_count = 0; burst_count < num_bursts; burst_count++) begin
          max_pyld_size    = 0;
          burst_items_rcvd = 0;

          sim_burst_mb.get(sim_burst);
          max_pyld_size = sim_burst.sideband.length / (ITEM_W/8);
          sb_exp = sim_burst.sideband;

          // Use the information we gathered to verify the actual output, one
          // packet at a time.
          burst_items_rcvd = 0;
          for (int pkt_count = 0; ; pkt_count++) begin
            int pyld_size;

            noc_bfm.get(pkt_rcvd);
            sb_rcvd = pkt_rcvd.user[0];

            if (!EN_TIME_ALL_PKTS && burst_items_rcvd > 0) begin
              // We only expect a timestamp on the first packet in the burst
              // when EN_TIME_ALL_PKTS is false.
              sb_exp.has_time = 0;
            end

            pyld_size = sb_rcvd.length / (ITEM_W/8);
            burst_items_rcvd += pyld_size;

            `ASSERT_ERROR(`DIV_CEIL(sb_rcvd.length, DATA_W/8) == pkt_rcvd.data.size(),
              "o_noc: Packet size doesn't match length in sideband");

            `ASSERT_ERROR(sb_rcvd.length % (ITEM_W/8) == 0,
              $sformatf("o_noc: Sideband length %0d is not a multiple of the item size.",
                sb_rcvd.length));

            // Verify the header matches the expected value. Ignore the exact
            // length here, since it's difficult to predict and we'll make
            // sure the total length of all packets equals the burst.
            sb_exp.length = sb_rcvd.length;
            sb_exp.eob = (burst_items_rcvd >= sim_burst.num_items +
              sim_burst.total_cp_ins - sim_burst.total_cp_rem);
            sb_exp.eov = (burst_items_rcvd % fft_size == 0);
            check_sideband(sb_exp, sb_rcvd, pkt_count, `LINE_INFO);

            sb_exp.timestamp += pyld_size;

            // Make sure the length is in the allowed range
            `ASSERT_ERROR(
              pyld_size >= 1 && pyld_size <= max_pyld_size,
              $sformatf("o_noc: Payload size is outside expected range. Max: %0d, Received: %0d",
                max_pyld_size, pyld_size
              )
            );

            // Check that the sideband information is constant
            foreach (pkt_rcvd.user[idx]) begin
              `ASSERT_ERROR(
                pkt_rcvd.user[idx] === sb_rcvd,
                $sformatf(
                  "o_noc: Inconsistent sideband data. Expected: %X, Received: %X, Index: %0d",
                  sb_rcvd, pkt_rcvd.user[idx], idx
                )
              );
            end

            // Check for end of burst
            if (sb_rcvd.eob) break;
          end

          // Make sure the total length of the CHDR data is what we expected.
          `ASSERT_ERROR(
            burst_items_rcvd == sim_burst.num_items +
              sim_burst.total_cp_ins - sim_burst.total_cp_rem,
            $sformatf(
              "o_noc: Burst length mismatch. Sent: %0d, Inserted: %0d, Removed: %0d, Received: %0d",
              sim_burst.num_items, sim_burst.total_cp_ins,
              sim_burst.total_cp_rem, burst_items_rcvd
            )
          )
        end

        // Make sure there aren't any remaining lengths queued up
        clk_gen.clk_wait_r(10);
        `ASSERT_ERROR(cp_rem_bfm.num_received() == 0,
          "Extra CP rem lengths were received"
        );
        `ASSERT_ERROR(cp_ins_bfm.num_received() == 0,
          "Extra CP ins lengths were received"
        );
      end : output_checker
    join
  endtask


  // Run num_iter randomly generated tests
  task automatic test_random(int num_iter);
    int num_bursts;
    int num_ffts;
    int fft_size;
    bit do_cp_rem = 0;
    bit do_cp_ins = 0;
    int pkt_size;

    test.start_test(
      $sformatf("Random (%0d iterations)", num_iter),
        num_iter*MAX_FFT_SIZE*10us
    );

    repeat (num_iter) begin
      // Choose random parameters for this iteration
      num_bursts = $urandom_range(1, 3);
      num_ffts   = $urandom_range(1, 8);
      fft_size   = 2**$urandom_range(MIN_FFT_SIZE_LOG2, MAX_FFT_SIZE_LOG2);
      do_cp_rem  = 0;
      do_cp_ins  = 0;
      case ($urandom_range(0, 2))
        0 : ; // No CP insertion/removal
        1 : do_cp_rem = EN_CP_REMOVAL;
        2 : do_cp_ins = EN_CP_INSERTION;
      endcase

      test_bursts(
        .num_bursts (num_bursts),
        .num_ffts   (num_ffts),
        .fft_size   (fft_size),
        .do_cp_rem  (do_cp_rem),
        .do_cp_ins  (do_cp_ins),
        .pkt_size   (-1),
        .has_time   (-1),
        .add_partial(-1)
      );
    end

    test.end_test();
  endtask


  // Perform some directed tests
  //
  // These are picked to quickly test corner cases. If these pass, there's a
  // good chance the random testing will be OK.
  //
  task automatic test_directed();
    bit do_cp_rem;
    bit do_cp_ins;
    bit has_time;
    bit add_partial;

    test.start_test("Directed", 10ms);

    for (int count = 0; count < 16; count++) begin
      {do_cp_rem, do_cp_ins, has_time, add_partial} = count;

      // Test all permutations except the ones which are not enabled.
      if (do_cp_rem && do_cp_ins) continue;  // Not supported
      if (do_cp_rem && !EN_CP_REMOVAL) continue;
      if (do_cp_ins && !EN_CP_INSERTION) continue;

      // Args:
      //           ┌ num_bursts,
      //           |   ┌ num_ffts,
      //           |   |   ┌ fft_size,
      //           |   |   |          ┌ do_cp_rem,
      //           |   |   |          |          ┌ do_cp_ins,
      //           |   |   |          |          |       ┌ pkt_size,
      //           |   |   |          |          |       |         ┌ has_time
      //           |   |   |          |          |       |         |            ┌ add_partial
      test_bursts( 1,  1, 16, do_cp_rem, do_cp_ins,     16, has_time, add_partial);
      test_bursts( 1,  2, 16, do_cp_rem, do_cp_ins,     16, has_time, add_partial);
      test_bursts( 2,  2, 16, do_cp_rem, do_cp_ins,     16, has_time, add_partial);
      test_bursts( 1,  1, 16, do_cp_rem, do_cp_ins,     16, has_time, add_partial);
      test_bursts( 1,  1, 32, do_cp_rem, do_cp_ins,      8, has_time, add_partial);
      test_bursts( 1,  1,  8, do_cp_rem, do_cp_ins,     32, has_time, add_partial);
      test_bursts( 1,  4,  8, do_cp_rem, do_cp_ins,     32, has_time, add_partial);
      test_bursts( 2,  2, 16, do_cp_rem, do_cp_ins,      8, has_time, add_partial);
      test_bursts( 1,  2, 16, do_cp_rem, do_cp_ins,      8, has_time, add_partial);
      // Test odd packet size, but make sure it's a multiple of NIPC
      test_bursts( 1,  2,  8, do_cp_rem, do_cp_ins, NIPC*7, has_time, add_partial);
      test_bursts( 2,  2,  8, do_cp_rem, do_cp_ins, NIPC*7, has_time, add_partial);
      test_bursts( 2,  3,  8, do_cp_rem, do_cp_ins, NIPC*7, has_time, add_partial);
      test_bursts( 3, 13,  8, do_cp_rem, do_cp_ins, NIPC*7, has_time, add_partial);
    end

    test.end_test();
  endtask : test_directed


  //---------------------------------------------------------------------------
  // Main Test Process
  //---------------------------------------------------------------------------

  initial begin : tb_main
    //string msg;
    string tb_name;
    tb_name = $sformatf( {
      "fft_packetize_tb\n",
      "NIPC              = %0d\n",
      "MAX_PKT_SIZE_LOG2 = %0d\n",
      "MAX_FFT_SIZE_LOG2 = %0d\n",
      "EN_CP_REMOVAL     = %0d\n",
      "EN_CP_INSERTION   = %0d\n"},
      NIPC, MAX_PKT_SIZE_LOG2, MAX_FFT_SIZE_LOG2, EN_CP_REMOVAL, EN_CP_INSERTION
    );
    test.start_tb(tb_name, 100ms);

    // Don't start the clocks until after start_tb() returns. This ensures that
    // the clocks aren't toggling while other instances of this testbench are
    // running, which speeds up simulation time.
    clk_gen.start();

    // Start the BFM
    noc_bfm.run();
    fft_bfm.run();
    cp_rem_bfm.run();
    cp_ins_bfm.run();
    noc_bfm.set_master_stall_prob(STALL_PROB);
    noc_bfm.set_slave_stall_prob(STALL_PROB);
    fft_bfm.set_master_stall_prob(STALL_PROB);
    fft_bfm.set_slave_stall_prob(STALL_PROB);
    // DUT expects cyclic-prefix inputs to always be ready, but outputs are
    // allowed to stall.
    cp_rem_bfm.set_master_stall_prob(0);
    cp_ins_bfm.set_master_stall_prob(0);
    cp_rem_bfm.set_slave_stall_prob(STALL_PROB);
    cp_ins_bfm.set_slave_stall_prob(STALL_PROB);

    //--------------------------------
    // Reset
    //--------------------------------

    test.start_test("Reset", 10us);
    clk_gen.reset(1);
    clk_gen.clk_wait_f(3);
    test.end_test();

    //--------------------------------
    // Test Sequences
    //--------------------------------

    test_directed();

    // Do 100 random tests at a time to keep the timeout relatively short
    repeat (25) test_random(100);

    //--------------------------------
    // Finish Up
    //--------------------------------

    // End the TB, but don't $finish, since we don't want to kill other
    // instances of this testbench that may be running.
    test.end_tb(0);
    // Kill the clocks to end this instance of the testbench
    clk_gen.kill();
  end : tb_main

endmodule : fft_packetize_tb


`default_nettype wire

//
// Copyright 2025 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Package: strobe_data_bfm_pkg
//
// Description:
//
//   Package defining a strobe data bus functional model (BFM).
//
//
//

package strobe_data_bfm_pkg;
  import PkgMath::*;
  import rfnoc_chdr_utils_pkg::*;

  //----------------------------------------------------------------------------

  class strobe_data_block #(
      parameter int NSPC   = 1,  // Number of samples per clock cycle
      parameter int SAMP_W = 32  // Length of each data sample
  );
    //--------------------------------------------------------------------------
    // Type definitions
    //--------------------------------------------------------------------------
    localparam int DATA_W = SAMP_W * NSPC;

    typedef strobe_data_block#(
        .NSPC  (NSPC),
        .SAMP_W(SAMP_W)
    ) strobe_data_block_t;

    typedef logic [DATA_W-1:0] data_t;


    //--------------------------------------------------------------------------
    // Class members
    //--------------------------------------------------------------------------
    bit                           verbose = 0;
    data_t                        data[$];
    logic  [CHDR_TIMESTAMP_W-1:0] timestamp;

    //--------------------------------------------------------------------------
    // Methods
    //--------------------------------------------------------------------------
    // Copy constructor
    function strobe_data_block_t copy();
      strobe_data_block_t new_pkt = new();

      new_pkt.data = this.data;
      new_pkt.timestamp = this.timestamp;
      return new_pkt;
    endfunction : copy

    // Comparator method, option to enable approximate equality with allowed deviation
    // between samples.
    // If allowed_deviation is 0 (default), samples must match exactly.
    // If allowed_deviation > 0, samples are considered equal if the absolute
    // difference between them is less than or equal to allowed_deviation.
    function bit equal(strobe_data_block_t pkt, int allowed_deviation = 0);
      if (this.data.size() != pkt.data.size()) begin
        return 0;
      end
      foreach (data[i]) begin
        for (int item = 0; item < NSPC; item++) begin
          logic signed [SAMP_W/2 -1:0] a_im;
          logic signed [SAMP_W/2 -1:0] a_re;
          logic signed [SAMP_W/2 -1:0] b_im;
          logic signed [SAMP_W/2 -1:0] b_re;
          {a_im, a_re} = data[i];
          {b_im, b_re} = pkt.data[i];
          if (a_im !== b_im && !(abs(a_im - b_im) <= allowed_deviation)) begin
            if (verbose) begin
              $display("Data mismatch in imaginary part at index %0d: This: 0x%0h, Other: 0x%0h",
                       i, a_im, b_im);
            end
            return 0;
          end
          if (a_re !== b_re && !(abs(a_re - b_re) <= allowed_deviation)) begin
            if (verbose) begin
              $display("Data mismatch in real part at index %0d: This: 0x%0h, Other: 0x%0h", i,
                       a_re, b_re);
            end
            return 0;
          end
        end
      end
      return 1;
    endfunction : equal

    // String representation of the data block
    function string to_string();
      string header = "";
      string data_str = "";

      header = $sformatf("Strobe Data Block: %0d samples:\n", data.size());

      foreach (data[i]) begin
        data_str = {data_str, $sformatf("Word[%0d]: 0x%0h\n", i, data[i])};
      end

      return {header, data_str};
    endfunction : to_string

    // Print the data to console
    function void print();
      $display("%s", to_string());
    endfunction : print

  endclass : strobe_data_block

  //----------------------------------------------------------------------------
  // Strobe Data BFM class
  //----------------------------------------------------------------------------
  class strobe_data_bfm #(
      parameter int NSPC   = 1,  // Number of samples per clock cycle
      parameter int SAMP_W = 32  // Length of each strobe sample
  );
    //--------------------------------------------------------------------------
    // Type definitions
    //--------------------------------------------------------------------------
    localparam int DATA_W = SAMP_W * NSPC;
    typedef strobe_data_block#(
        .NSPC  (NSPC),
        .SAMP_W(SAMP_W)
    ) strobe_data_block_t;
    typedef strobe_data_block_t::data_t data_t;

    //------------------------------------------------------------------------
    // Class members
    //------------------------------------------------------------------------
    local virtual strobe_data_if #(NSPC, SAMP_W).master master;
    local virtual strobe_data_if #(NSPC, SAMP_W).slave slave;

    bit verbose = 0;

    logic [CHDR_TIMESTAMP_W-1:0] timebase;

    local bit inter_block_gap = 0;  // If set, insert a gap (data='X') between blocks

    local bit master_en;
    local bit slave_en;

    local int master_stall_prob = 0;  // Probability (0-100) of stalling master on any given cycle

    mailbox #(strobe_data_block_t) tx_data_blocks;
    mailbox #(strobe_data_block_t) rx_data_blocks;

    //------------------------------------------------------------------------
    // Constructor
    //------------------------------------------------------------------------
    function new(virtual strobe_data_if #(NSPC, SAMP_W).master master,
                 virtual strobe_data_if #(NSPC, SAMP_W).slave slave);
      this.master_en = (master != null);
      this.slave_en = (slave != null);
      this.master = master;
      this.slave = slave;
      this.timebase = master_en ? master.timebase : '0;
      tx_data_blocks = new;
      rx_data_blocks = new;
    endfunction : new

    //------------------------------------------------------------------------
    // Class Methods
    //------------------------------------------------------------------------

    task put(strobe_data_block_t data);
      assert (master_en)
      else $fatal(1, "Cannot use TX operations for a null master");
      tx_data_blocks.put(data);
    endtask : put

    function bit try_put(strobe_data_block_t data);
      assert (master_en)
      else $fatal(1, "Cannot use TX operations for a null master");
      return tx_data_blocks.try_put(data);
    endfunction : try_put

    task get(output strobe_data_block_t data);
      assert (slave_en)
      else $fatal(1, "Cannot use RX operations for a null slave");
      rx_data_blocks.get(data);
    endtask : get

    function bit try_get(output strobe_data_block_t data);
      assert (slave_en)
      else $fatal(1, "Cannot use RX operations for a null slave");
      return rx_data_blocks.try_get(data);
    endfunction : try_get

    task peek(output strobe_data_block_t data);
      assert (slave_en)
      else $fatal(1, "Cannot use RX operations for a null slave");
      rx_data_blocks.peek(data);
    endtask : peek

    function bit try_peek(output strobe_data_block_t data);
      assert (slave_en)
      else $fatal(1, "Cannot use RX operations for a null slave");
      return rx_data_blocks.try_peek(data);
    endfunction : try_peek

    function int num_received();
      assert (slave_en)
      else $fatal(1, "Cannot use RX operations for a null slave");
      return rx_data_blocks.num();
    endfunction : num_received

    function int num_queued();
      assert (master_en)
      else $fatal(1, "Cannot use TX operations for a null master");
      return tx_data_blocks.num();
    endfunction : num_queued

    function void set_master_stall_prob(int stall_probability);
      assert (stall_probability >= 0 && stall_probability <= 100)
      else $fatal(1, "Master stall probability must be between 0 and 100");
      master_stall_prob = stall_probability;
    endfunction : set_master_stall_prob

    function void set_verbose(bit verbose = 1);
      this.verbose = verbose;
    endfunction : set_verbose

    function void set_inter_block_gap(bit enable = 1);
      this.inter_block_gap = enable;
    endfunction : set_inter_block_gap

    task run();
      fork
        if (master_en) begin
          master_thread();
        end
        if (slave_en) begin
          slave_thread();
        end
      join_none
    endtask : run

    //------------------------------------------------------------------------
    // Master thread
    //------------------------------------------------------------------------
    // Each clockcycle, if not stalled, the master will send the next word of
    // the current data block. If there is no current data block, it will try to get
    // one from the tx_data_blocks mailbox. If there is no data block to send, it will
    // send X's.
    //
    // If inter_block_gap is enabled, it will send a gap (data='X') between
    // blocks. This can be helpful to distinguish data block boundaries on the slave
    // side.
    //
    // If a data block has a timestamp, the master will wait until the timebase of
    // the master interface (which is incremented each cycle the master is not
    // stalled) reaches the timestamp before sending the first word of the data block.
    //

    task master_thread();
      strobe_data_block_t data;
      bit gap_sent = 0, tmp;

      forever begin
        @(posedge master.strobe_clk);
        if (master.strobe_rst) begin
          master.timebase <= '0;
          continue;
        end
        // If we are curently not sending a data block, try to retrieve one.
        if (data == null) begin
          if (!gap_sent) begin
            tmp = tx_data_blocks.try_get(data);
          end
        end

        // Randomly stall the master interface
        if ($urandom_range(0, 99) < master_stall_prob) begin
          master.data   <= 'X;
          master.strobe <= 0;
          // If master is not stalling, send data.
        end else begin
          master.strobe <= 1;
          master.timebase++;
          // If there is a data block to send, send the next word.
          if (data != null) begin
            if (data.timestamp <= master.timebase) begin
              master.data <= data.data.pop_front();
              if (data.data.size() == 0) begin
                data = null;
                // If inter-block gap is enabled, send a gap (data='X') next cycle
                if (inter_block_gap && !gap_sent) begin
                  gap_sent = 1;
                end
              end
            end else begin
              // If the data timestamp is in the future, send X's until we reach it
              master.data <= 'X;
            end
            // Otherwise send X's
          end else begin
            master.data <= 'X;
            if (gap_sent) begin
              gap_sent = 0;
            end
          end
        end
      end
    endtask : master_thread

    //------------------------------------------------------------------------
    // Slave thread
    //------------------------------------------------------------------------
    // Each clockcycle, if the strobe is high, the slave will check if the
    // associated data corresponds to a data block(data all X's). If it does,
    // it will capture the data. Otherwise it will ignore the data.
    //
    task slave_thread();
      strobe_data_block_t data = new();

      forever begin
        @(posedge slave.strobe_clk);

        if (slave.strobe) begin
          if (slave.data === 'X) begin
            if (data != null && data.data.size() > 0) begin
              rx_data_blocks.put(data.copy());
              data = null;
            end
          end else begin
            // If we are not currently capturing a data block, start a new one
            if (data == null) begin
              data = new();
            end
            // Capture the data
            data.data.push_back(slave.data);
          end
        end
      end
    endtask : slave_thread

  endclass : strobe_data_bfm

endpackage

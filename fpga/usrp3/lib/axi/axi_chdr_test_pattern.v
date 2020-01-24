//
// Copyright 2014 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
//
// Synthesizable test pattern generator and checker
// for AXI-Stream that can be used to test transparent blocks
// (FIFOs, switches, etc)
//

module axi_chdr_test_pattern #(
  parameter SR_BASE     = 8'h0,         //Base address for settings in this module
  parameter DELAY_MODE  = "DYNAMIC",    //Are delays configurable at runtime {STATIC, DYNAMIC}
  parameter SID_MODE    = "DYNAMIC",    //Is the SID configurable at runtime {STATIC, DYNAMIC}
  parameter STATIC_SID  = 32'h0,        //SID Value if it is static
  parameter BW_COUNTER  = 1             //Instantiate counters to measure bandwidth (Cycles of Data Xfer / Total cycles)
) (
  input             clk,
  input             reset,

  // AXI stream to hook up to input of DUT
  output reg [63:0] i_tdata,
  output reg        i_tlast,
  output reg        i_tvalid,
  input             i_tready,

  // AXI stream to hook up to output of DUT
  input      [63:0] o_tdata,
  input             o_tlast,
  input             o_tvalid,
  output reg        o_tready,

  //Settings bus interface
  input             set_stb,
  input       [7:0] set_addr,
  input      [31:0] set_data,

  // Test flags
  output reg        running,    //Test is currently in progress
  output reg        done,       //(Sticky) Test has finished executing
  output reg  [1:0] error,      //Error code from last test execution

  output    [127:0] status_vtr, //More information about test failure.
  output     [95:0] bw_ratio    //Bandwidth counter info
);

  //
  // Error Codes
  //
  localparam ERR_SUCCESS                  = 0;
  localparam ERR_DATA_MISMATCH            = 1;
  localparam ERR_SIZE_MISMATCH_TOO_LONG   = 2;
  localparam ERR_SIZE_MISMATCH_TOO_SHORT  = 3;
  
  localparam ERR_TIMEOUT_LOG2             = 10;

  //
  // Settings
  //
  wire        bist_size_ramp;
  wire [1:0]  bist_test_patt;
  wire [12:0] bist_max_pkt_size;
  wire        bist_go, bist_cont, bist_ctrl_wr;
  wire [1:0]  bist_ctrl_reserved;
  wire [17:0] bist_max_pkts;
  wire [15:0] bist_tx_pkt_delay;
  wire [7:0]  bist_rx_samp_delay;
  wire [31:0] bist_cvita_sid;

  localparam TEST_PATT_ZERO_ONE     = 2'd0;
  localparam TEST_PATT_CHECKERBOARD = 2'd1;
  localparam TEST_PATT_COUNT        = 2'd2;
  localparam TEST_PATT_COUNT_INV    = 2'd3;

  // SETTING: Test Control Register
  // Fields:
  // - [0]    : (Strobe) Start the test if 1, otherwise stop a running test.
  //            If no test is running then reset the status. (Reseting a
  //            continuously running test requires two writes to this reg)
  // - [1]    : Start the test in continuous mode. (Run until reset or failure)
  // - [3:2]  : <Unused>
  // - [5:4]  : Test pattern:
  //            * 00 = Zeros and Ones (0x0000000000000000 <-> 0xFFFFFFFFFFFFFFFF)
  //            * 01 = Checkerboard   (0x5555555555555555 <-> 0xAAAAAAAAAAAAAAAA)
  //            * 10 = Counter        (Each byte will count up)
  //            * 11 = Invert Counter (Each byte will count up and invert)
  setting_reg #(
    .my_addr(SR_BASE + 0), .width(6), .at_reset(3'b0)
  ) reg_ctrl (
    .clk(clk), .rst(reset),
    .strobe(set_stb), .addr(set_addr), .in(set_data),
    .out({bist_test_patt, bist_ctrl_reserved, bist_cont, bist_go}),.changed(bist_ctrl_wr)
  );
  
  wire bist_start = bist_ctrl_wr & bist_go;
  wire bist_clear = bist_ctrl_wr & ~bist_go;

  // SETTING: Test Packet Configuration Register
  // Fields:
  // - [17:0]  : Number of packets to transfer per BIST execution
  // - [30:18] : Max number of bytes of payload per packet
  // - [31]    : Send variable (ramping) sized packets
  setting_reg #(
    .my_addr(SR_BASE + 1), .width(32), .at_reset(32'b0)
  ) reg_pkt_config (
    .clk(clk), .rst(reset),
    .strobe(set_stb), .addr(set_addr), .in(set_data),
    .out({bist_size_ramp, bist_max_pkt_size, bist_max_pkts}),.changed()
  );

  generate if (DELAY_MODE == "DYNAMIC") begin
    // SETTING: Delay Register
    // Fields:
    // - [15:0]   : Number of cycles to wait between generating consecutive *packets*
    // - [23:16]  : Number of cycles to wait between consuming consecutive *samples*
    setting_reg #(
      .my_addr(SR_BASE + 2), .width(24), .at_reset(24'b0)
    ) reg_delay (
      .clk(clk), .rst(reset),
      .strobe(set_stb), .addr(set_addr), .in(set_data),
      .out({bist_rx_samp_delay, bist_tx_pkt_delay}),.changed()
    );
  end else begin
    assign {bist_rx_samp_delay, bist_tx_pkt_delay} = 24'h0;
  end endgenerate

  generate if (SID_MODE == "DYNAMIC") begin
    // SETTING: CHDR Stream ID Register
    // Fields:
    // - [31:0]   : Stream ID to attach to CHDR packets
    setting_reg #(
      .my_addr(SR_BASE + 3), .width(32), .at_reset(32'b0)
    ) reg_sid (
      .clk(clk), .rst(reset),
      .strobe(set_stb), .addr(set_addr), .in(set_data),
      .out(bist_cvita_sid),.changed()
    );
  end else begin
    assign bist_cvita_sid = STATIC_SID;
  end endgenerate

  //
  // State
  //
  localparam TX_IDLE    = 3'd0;
  localparam TX_START   = 3'd1;
  localparam TX_ACTIVE  = 3'd2;
  localparam TX_GAP     = 3'd3;
  localparam TX_DONE    = 3'd4;
  localparam TX_WAIT    = 3'd5;

  localparam RX_IDLE    = 3'd0;
  localparam RX_ACTIVE  = 3'd1;
  localparam RX_FAIL    = 3'd2;
  localparam RX_DONE    = 3'd3;
  localparam RX_WAIT    = 3'd4;

  reg [2:0]                   tx_state, rx_state;
  reg [ERR_TIMEOUT_LOG2-1:0]  err_timeout;
  reg [1:0]                   test_pattern;
  reg                         rearm_test;

  reg [17:0]  tx_pkt_cnt, rx_pkt_cnt;
  reg [13:0]  tx_byte_cnt, rx_byte_cnt;
  reg [23:0]  test_run_cnt;
  reg [15:0]  tx_delay;
  reg [7:0]   rx_delay;
  wire [63:0] tx_cvita_hdr, rx_cvita_hdr;

  wire tx_next_pkt_cond, rx_next_pkt_cond;
  assign tx_next_pkt_cond = (tx_byte_cnt[12:3] == bist_max_pkt_size[12:3]) ||               //Packet size reaches max OR
                            (bist_size_ramp && ({7'h0, tx_byte_cnt[13:3]} == tx_pkt_cnt));  //Packet size / 8 == Packet Count
  assign rx_next_pkt_cond = (rx_byte_cnt[12:3] == bist_max_pkt_size[12:3]) || 
                            (bist_size_ramp && ({7'h0, rx_byte_cnt[13:3]} == rx_pkt_cnt));

  wire tx_test_done_cond, rx_test_done_cond;
  assign tx_test_done_cond = (tx_pkt_cnt == bist_max_pkts);
  assign rx_test_done_cond = (rx_pkt_cnt == bist_max_pkts);
  
  reg [63:0] tx_data_next, rx_data_exp;
  always @(*) begin
    case (test_pattern)
      TEST_PATT_ZERO_ONE: begin
        tx_data_next  <= {8{tx_byte_cnt[3] ? 8'h00 : 8'hFF}};
        rx_data_exp   <= {8{rx_byte_cnt[3] ? 8'h00 : 8'hFF}};
      end
      TEST_PATT_CHECKERBOARD: begin
        tx_data_next  <= {32{tx_byte_cnt[3] ? 2'b01 : 2'b10}};
        rx_data_exp   <= {32{rx_byte_cnt[3] ? 2'b01 : 2'b10}};
      end
      TEST_PATT_COUNT: begin
        tx_data_next  <= {8{tx_byte_cnt[10:3]}};
        rx_data_exp   <= {8{rx_byte_cnt[10:3]}};
      end
      TEST_PATT_COUNT_INV: begin
        tx_data_next  <= {8{(tx_byte_cnt[3] ? 8'hFF : 8'h00) ^ tx_byte_cnt[10:3]}};
        rx_data_exp   <= {8{(rx_byte_cnt[3] ? 8'hFF : 8'h00) ^ rx_byte_cnt[10:3]}};
      end
      default: begin
        tx_data_next  <= 64'd0;
        rx_data_exp   <= 64'd0;
      end
    endcase
  end

  //NOTE: We always attach the max size in the packet header for simplicity.
  //      This will not work with state machines that validate the packet length in the
  //      header with the tlast position.
  assign tx_cvita_hdr = {4'h0, tx_pkt_cnt[11:0], 2'b00, bist_max_pkt_size, bist_cvita_sid};
  assign rx_cvita_hdr = {4'h0, rx_pkt_cnt[11:0], 2'b00, bist_max_pkt_size, bist_cvita_sid};

  reg [63:0] o_tdata_fail;
  assign status_vtr = {     //Status at the time of failure
    o_tdata_fail,           //[127:64]
    test_run_cnt,           //[63:40]
    rx_data_exp[7:0],       //[39:32]
    rx_pkt_cnt,             //[31:14]
    rx_byte_cnt             //[13:0]
  };

  //-------------------------------------------------------
  // Transmitter
  //-------------------------------------------------------
  always @(posedge clk) begin
    if (reset | (bist_clear & ~rearm_test)) begin
      tx_delay      <= 0;
      tx_pkt_cnt    <= 0;
      tx_byte_cnt   <= 0;
      i_tdata       <= 64'h0;
      i_tlast       <= 1'b0;
      i_tvalid      <= 1'b0;
      tx_state      <= TX_IDLE;
    end else begin
      case(tx_state)
        TX_IDLE: begin
          tx_delay      <= 0;
          tx_pkt_cnt    <= 1;
          tx_byte_cnt   <= 0;
          i_tdata       <= 64'h0;
          i_tlast       <= 1'b0;
          i_tvalid      <= 1'b0;
          // Run when bist_start asserted.
          if (bist_start | rearm_test) begin
            tx_state      <= TX_START;
            test_pattern  <= bist_test_patt;
          end
        end // case: TX_IDLE

        // START signal is asserted.
        // Now need to start transmiting a packet.
        TX_START: begin
          // At the next clock edge drive first beat of new packet onto HDR bus.
          i_tlast     <= 1'b0;
          i_tvalid    <= 1'b1;
          tx_byte_cnt <= tx_byte_cnt + 8;
          i_tdata     <= tx_cvita_hdr;
          tx_state    <= TX_ACTIVE;
        end

        // Valid data is (already) being driven onto the CHDR bus.
        // i_tlast may also be driven asserted if current data count has reached EOP.
        // Watch i_tready to see when it's consumed.
        // When packets are consumed increment data counter or transition state if
        // EOP has sucsesfully concluded.
        TX_ACTIVE: begin
          i_tvalid <= 1'b1; // Always assert tvalid
          if (i_tready) begin
            i_tdata <= tx_data_next;
            // Will this next beat be the last in a packet?
            if (tx_next_pkt_cond) begin
              tx_byte_cnt <= 0;
              i_tlast     <= 1'b1;
              tx_state    <= TX_GAP;
            end else begin
              tx_byte_cnt <= tx_byte_cnt + 8;
              i_tlast     <= 1'b0;
              tx_state    <= TX_ACTIVE;
            end
          end else begin
            //Keep driving all CHDR bus signals as-is until i_tready is asserted.
            tx_state <= TX_ACTIVE;
          end
        end // case: TX_ACTIVE

        // Force an inter-packet gap between packets in a BIST sequence where tvalid is driven low.
        // As we leave this state check if all packets in BIST sequence have been generated yet,
        // and if so go to done state.
        TX_GAP: begin
          if (i_tready) begin
            i_tvalid    <= 1'b0;
            i_tdata     <= 64'h0;
            i_tlast     <= 1'b0;
            tx_pkt_cnt  <= tx_pkt_cnt + 1;

            if (tx_test_done_cond) begin
              tx_state <= TX_DONE;
            end else begin
              tx_state <= TX_WAIT;
              tx_delay <= bist_tx_pkt_delay;
            end
          end else begin // if (i_tready)
            tx_state <= TX_GAP;
          end
        end // case: TX_GAP

        // Simulate inter packet gap in real UHD system
        TX_WAIT: begin
          if (tx_delay == 0)
            tx_state <= TX_START;
          else begin
            tx_delay <= tx_delay - 1;
            tx_state <= TX_WAIT;
          end
        end

        // Complete test pattern BIST sequence has been transmitted.
        // Sit in this state until the RX side consumes all packets except
        // for when the test is running in continuous mode.
        TX_DONE: begin
          i_tvalid  <= 1'b0;
          i_tlast   <= 1'b0;
          i_tdata   <= 64'd0;

          if (running & ~rearm_test) begin
            tx_state <= TX_DONE;
          end else begin
            tx_state <= TX_IDLE;
          end
        end
      endcase // case (tx_state)
    end
  end

  //-------------------------------------------------------
  // Receiver
  //-------------------------------------------------------
  always @(posedge clk) begin
    if (reset | (bist_clear & ~rearm_test)) begin
      rx_delay      <= 0;
      rx_pkt_cnt    <= 0;
      rx_byte_cnt   <= 0;
      o_tdata_fail  <= 64'h0;
      o_tready      <= 1'b0;
      error         <= ERR_SUCCESS;
      done          <= 1'b0;
      rx_state      <= RX_IDLE;
      err_timeout   <= {ERR_TIMEOUT_LOG2{1'b0}};
      test_run_cnt  <= 0;
    end else begin
      case(rx_state)
        RX_IDLE: begin
          rx_delay      <= 0;
          rx_pkt_cnt    <= 1;
          rx_byte_cnt   <= 0;
          o_tdata_fail  <= 64'h0;
          o_tready      <= 1'b0;
          error         <= ERR_SUCCESS;
          done          <= 1'b0;
          err_timeout   <= {ERR_TIMEOUT_LOG2{1'b0}};
          // Not accepting data whilst Idle,
          // switch to active when packet arrives
          if (o_tvalid) begin
            o_tready <= 1'b1;
            rx_state <= RX_ACTIVE;
          end else begin
            rx_state <= RX_IDLE;
          end
        end

        RX_ACTIVE: begin
          o_tready <= 1'b1;
          if (o_tvalid) begin
            if (o_tdata != (rx_byte_cnt == 0 ? rx_cvita_hdr : rx_data_exp)) begin
              $display("axis_test_pattern: o_tdata: %x  !=  expected:  %x @ time: %d", o_tdata, rx_data_exp, $time);
              error         <= ERR_DATA_MISMATCH;
              rx_state      <= RX_FAIL;
              o_tdata_fail  <= o_tdata;
            end else if (rx_next_pkt_cond) begin
              // Last not asserted when it should be!
              if (~(o_tlast === 1)) begin
                $display("axis_test_pattern: o_tlast not asserted when it should be @ time: %d", $time);
                error    <= ERR_SIZE_MISMATCH_TOO_LONG;
                rx_state <= RX_FAIL;
              end else begin
                // End of packet, set up to RX next
                rx_byte_cnt <= 0;
                rx_pkt_cnt  <= rx_pkt_cnt + 1;
                rx_delay    <= bist_rx_samp_delay;
                if (rx_test_done_cond) begin
                  rx_state      <= rearm_test ? RX_IDLE : RX_DONE;
                  error         <= ERR_SUCCESS;
                  test_run_cnt  <= test_run_cnt + 1;
                end else begin
                  rx_state      <= RX_WAIT;
                end
                o_tready        <= 1'b0;
              end
            end else begin
              // ...last asserted when it should not be!
              if (~(o_tlast === 0)) begin
                $display("axis_test_pattern: o_tlast asserted when it should not be @ time: %d", $time);
                error       <= ERR_SIZE_MISMATCH_TOO_SHORT;
                rx_state    <= RX_FAIL;
              end else begin
                // Still in packet body
                rx_byte_cnt <= rx_byte_cnt + 8;
                rx_delay    <= bist_rx_samp_delay;
                if (bist_rx_samp_delay == 0) begin
                  rx_state    <= RX_ACTIVE;
                end else begin
                  rx_state    <= RX_WAIT;
                  o_tready    <= 1'b0;
                end
              end
            end
          end else begin
            // Nothing to do this cycle
            rx_state <= RX_ACTIVE;
          end
        end // case: RX_ACTIVE

        // To simulate the radio consuming samples at a steady rate set by the decimation
        // have a programable delay here
        RX_WAIT: begin
          if (rx_delay == 0) begin
            rx_state <= RX_ACTIVE;
            o_tready <= 1'b1;
          end else begin
            rx_delay <= rx_delay - 1;
            rx_state <= RX_WAIT;
          end
        end

        RX_FAIL: begin
          //The test has failed but the sender still has packets en route
          //Consume all of them before asserting done. Packets could be
          //malformed so just blindly consume lines and count cycles of
          //gaps. If non-valid cycles are more than 2^ERR_TIMEOUT_LOG2 then stop.
          o_tready <= 1'b1;
          if (~o_tvalid) begin
            if (err_timeout == {ERR_TIMEOUT_LOG2{1'b1}}) begin
              rx_state <= RX_DONE;
            end
            err_timeout <= err_timeout + 1;
          end
        end

        RX_DONE: begin
          o_tready    <= 1'b0;
          done        <= 1'b1;
          //The only way to exit this state is by asserting bist_clear
        end
      endcase // case (rx_state)
    end
  end

  //-------------------------------------------------------
  // Status Monitor
  //-------------------------------------------------------
  always @(posedge clk) begin
    if (reset)
      running <= 1'b0;
    else if (tx_state == TX_START)
      running <= 1'b1;
    else if (rx_state == RX_DONE)
      running <= 1'b0;
  end

  always @(posedge clk) begin
    if (reset | bist_clear)
      rearm_test  <= 1'b0;
    else if (bist_start & bist_cont)
      rearm_test  <= 1'b1;
    else if (rx_state == RX_FAIL)
      rearm_test  <= 1'b0;
  end
  
  //-------------------------------------------------------
  // Bandwidth Counter
  //-------------------------------------------------------
  generate if (BW_COUNTER) begin
    reg [47:0]         word_count, cyc_count;
    assign bw_ratio = {word_count, cyc_count};

    //Count number of lines transferred
    always @(posedge clk) begin
      if (reset| (bist_clear & ~rearm_test) | bist_start)
        word_count  <= 48'd0;
      else if (o_tvalid && rx_state == RX_ACTIVE)
        word_count  <= word_count + 48'd1;
    end

    //Count cycles as long as test is running
    always @(posedge clk) begin
      if (reset| (bist_clear & ~rearm_test) | bist_start)
        cyc_count   <= 48'd0;
      else if (rx_state == RX_ACTIVE || rx_state == RX_WAIT)
        cyc_count   <= cyc_count + 48'd1;
    end
  end else begin
    assign bw_ratio = 96'h0;
  end endgenerate

endmodule

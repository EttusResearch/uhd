//
// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: ctrlport_spi_adrv_tb
//
// Description: Testbench for ctrlport_spi_adrv.
//
// Parameters:
//
//   QUICK_TEST  : When 1, run a reduced number of random transactions to
//                 shorten simulation time.
//   NUM_BYTES_W : Passed to ctrlport_spi_adrv. Sets the log base 2 of the
//                 maximum number of data bytes per transaction.
//   HALF_PER    : Passed to ctrlport_spi_adrv. Sets the default SPI clock
//                 half-period in clock cycles.
//   HALF_PER_EN : Passed to ctrlport_spi_adrv. When 1, the REG_HALF_PER
//                 register is present and software-writable. When 0, the SPI
//                 clock rate is fixed at the HALF_PER parameter value.
//   CS_HOLD     : Passed to ctrlport_spi_adrv. When 1, CS_N is held low for
//                 one extra half-period after the last SCLK falling edge.
//   CS_GUARD    : Passed to ctrlport_spi_adrv. Number of CS_N-high
//                 half-periods between transactions. Must be 1 or 2.
//

`default_nettype none


module ctrlport_spi_adrv_tb #(
  parameter bit QUICK_TEST  = 0,
  parameter int NUM_BYTES_W = 8,
  parameter int HALF_PER    = 4,
  parameter int HALF_PER_EN = 1,
  parameter bit CS_HOLD     = 1,
  parameter int CS_GUARD    = 2
);

  `include "test_exec.svh"

  import PkgTestExec::*;
  import ctrlport_pkg::*;
  import ctrlport_spi_adrv_pkg::*;
  import ctrlport_bfm_pkg::*;


  //---------------------------------------------------------------------------
  // Testbench-only type definitions
  //---------------------------------------------------------------------------
  //
  // tb_mode_t extends the four hardware modes in spi_adrv_mode_t with a fifth
  // value for the REG_SI single-instruction path. The first four values are
  // intentionally assigned the same encoding as spi_adrv_mode_t so that a
  // simple cast works when calling the hardware-mode helpers.
  //
  //---------------------------------------------------------------------------

  typedef enum int {
    TB_MODE_RAW       = 0,  // Raw byte stream (REG_CONTROL MODE_RAW)
    TB_MODE_SI_SEQ    = 1,  // Single Instruction, sequential address
    TB_MODE_SI_REP    = 2,  // Single Instruction, repeated address
    TB_MODE_STREAMING = 3,  // Streaming burst
    TB_MODE_SI        = 4   // REG_SI single-triplet path
  } tb_mode_t;


  //---------------------------------------------------------------------------
  // Testbench configuration
  //---------------------------------------------------------------------------

  localparam real CLK_PERIOD_NS = 10.0; // 100 MHz
  localparam bit  VERBOSE       = 0;    // Set to 1 to print useful debug info

  // Number of random transactions to run
  localparam int NUM_RAND_TESTS = QUICK_TEST ? 10 : 1000;

  // DUT parameters
  localparam int BASE_ADDR     = 0;
  localparam int HALF_PER_W    = 8;

  // Maximum number of bytes per transaction (inclusive)
  localparam int MAX_NUM_BYTES = 2**NUM_BYTES_W - 1;

  // Expected SPI half-period in simulation time. All SPI signal edges are
  // registered on posedge clk, so every half-period is exactly this long.
  realtime half_per_time = (HALF_PER + 1) * CLK_PERIOD_NS * 1.0ns;

  // Number of CS_N assertions observed by the timing monitor. Checked at the
  // end of the test to confirm the monitor actually ran for every transaction.
  int cs_n_monitor_count  = 0; // Incremented by the timing monitor
  int expected_cs_n_count = 0; // Incremented once per issued transaction


  //---------------------------------------------------------------------------
  // Clocks and resets
  //---------------------------------------------------------------------------

  bit clk, rst;

  sim_clock_gen #(.PERIOD(CLK_PERIOD_NS), .AUTOSTART(0))
    clk_gen (.clk(clk), .rst(rst));


  //---------------------------------------------------------------------------
  // CtrlPort interface + BFM
  //---------------------------------------------------------------------------

  ctrlport_if cp_if (.clk(clk), .rst(rst));

  ctrlport_bfm cp_bfm = new(cp_if);


  //---------------------------------------------------------------------------
  // SPI Slave Model
  //---------------------------------------------------------------------------
  //
  // Takes the MOSI data and stuffs it into a queue that can be read by the
  // test.
  //
  // Drives an incrementing sequence 0x01, 0x02, 0x03, ... onto MISO, one value
  // per byte (MSB-first), resetting to 0x00 at the start of each cs_n
  // assertion. This makes read-data predictable and test-case-agnostic.
  //
  //---------------------------------------------------------------------------

  logic spi_sclk, spi_cs_n, spi_mosi, spi_miso;

  logic [7:0] miso_byte_ctr = '0;
  logic [7:0] miso_shift    = '0;
  logic [2:0] miso_bit_ctr  = '0;

  assign spi_miso = spi_cs_n ? 1'bX : miso_shift[7];

  always_ff @(negedge spi_sclk, posedge spi_cs_n) begin : miso_shifter
    if (spi_cs_n) begin
      miso_byte_ctr <= '0;
      miso_shift    <= '0;
      miso_bit_ctr  <= '0;
    end else begin
      if (miso_bit_ctr == 7) begin
        miso_byte_ctr <= miso_byte_ctr + 1;
        miso_shift    <= miso_byte_ctr + 1;
        miso_bit_ctr  <= '0;
      end else begin
        miso_shift   <= {miso_shift[6:0], 1'b0};
        miso_bit_ctr <= miso_bit_ctr + 1;
      end
    end
  end : miso_shifter

  // MOSI capture mailbox. Collects all bytes driven by the DUT on MOSI during
  // each CS_N assertion. Used by verify_write() to confirm write correctness.
  mailbox #(logic [7:0]) mosi_mailbox = new();

  // Samples MOSI at posedge SCLK (the slave's capture edge, since the DUT
  // launches on negedge SCLK) and puts completed bytes into mosi_mailbox.
  // Runs continuously in parallel with the main test process.
  initial begin : mosi_collector
    logic [7:0] mosi_shift;
    int         bit_cnt;
    forever begin
      @(negedge spi_cs_n);       // wait for CS_N assertion (transaction start)
      mosi_shift = '0;
      bit_cnt    = 0;
      while (!spi_cs_n) begin
        @(posedge spi_sclk);

        if (spi_cs_n) begin
          `ASSERT_ERROR(0, { "CS_N deasserted mid-byte: transaction ended ",
                             "before byte was complete" });
          break;
        end

        mosi_shift = {mosi_shift[6:0], spi_mosi};
        if (++bit_cnt == 8) begin
          mosi_mailbox.put(mosi_shift);
          mosi_shift = '0;
          bit_cnt    = 0;
        end
      end
    end
  end : mosi_collector


  //---------------------------------------------------------------------------
  // DUT
  //---------------------------------------------------------------------------

  ctrlport_spi_adrv #(
    .BASE_ADDR   (BASE_ADDR),
    .NUM_BYTES_W (NUM_BYTES_W),
    .HALF_PER    (HALF_PER),
    .HALF_PER_W  (HALF_PER_W),
    .HALF_PER_EN (HALF_PER_EN),
    .CS_HOLD     (CS_HOLD),
    .CS_GUARD    (CS_GUARD)
  ) dut (
    .clk                    (clk),
    .rst                    (rst),
    .s_ctrlport_req_wr      (cp_if.req.wr),
    .s_ctrlport_req_rd      (cp_if.req.rd),
    .s_ctrlport_req_addr    (cp_if.req.addr),
    .s_ctrlport_req_data    (cp_if.req.data),
    .s_ctrlport_resp_ack    (cp_if.resp.ack),
    .s_ctrlport_resp_status (cp_if.resp.status),
    .s_ctrlport_resp_data   (cp_if.resp.data),
    .sclk                   (spi_sclk),
    .cs_n                   (spi_cs_n),
    .mosi                   (spi_mosi),
    .miso                   (spi_miso)
  );


  //---------------------------------------------------------------------------
  // SPI helper tasks
  //---------------------------------------------------------------------------

  // Write a single 3-byte SPI instruction using REG_SI.
  //
  // addr[15] (addr_hi[7]) must be 0 to indicate a write; this task does not
  // enforce that; caller is responsible for passing a write-direction address.
  //
  // Args:
  //   addr      : 16-bit SPI register address (bit 15 must be 0 for writes).
  //   data_byte : Data byte to send.
  //
  task automatic spi_si_write(
    input logic [15:0] addr,
    input logic [7:0]  data_byte
  );
    logic [31:0] reg_val;
    reg_val        = 'X;          // Let unused bits be garbage
    reg_val[7:0]   = addr[15:8];  // addr_hi first on wire
    reg_val[15:8]  = addr[7:0];   // addr_lo
    reg_val[23:16] = data_byte;
    cp_bfm.write(BASE_ADDR + REG_SI, reg_val);
  endtask : spi_si_write


  // Read all 3 MISO bytes using REG_SI.
  //
  // Sets addr_hi[7]=1 (read direction), writes REG_SI, then blocks on
  // REG_DATA. Returns all 3 received bytes packed in a word:
  //
  //   rx_word[7:0]   = MISO byte 0 (addr_hi echo)
  //   rx_word[15:8]  = MISO byte 1 (addr_lo echo)
  //   rx_word[23:16] = MISO byte 2 (data byte)
  //
  // Args:
  //   addr    : 16-bit SPI register address; bit 15 is forced to 1.
  //   rx_word : All 3 received bytes packed little-endian into bits [23:0].
  //
  task automatic spi_si_read(
    input  logic [15:0] addr,
    output logic [31:0] rx_word
  );
    logic [31:0] reg_val;
    logic [15:0] rd_addr = addr | 16'h8000; // force bit 15 = read direction
    reg_val[7:0]   = rd_addr[15:8];         // addr_hi, bit 7=1 -> read
    reg_val[15:8]  = rd_addr[7:0];          // addr_lo
    reg_val[23:16] = 'X;                    // data_byte (don't care for reads)
    reg_val[31:24] = 'X;
    cp_bfm.write(BASE_ADDR + REG_SI, reg_val);
    cp_bfm.read(BASE_ADDR + REG_DATA, rx_word);
  endtask : spi_si_read


  // Write an arbitrary number of bytes over SPI in RAW mode.
  //
  // Args:
  //   data : Bytes to transmit, in wire order (data[0] is first on the wire).
  //
  task automatic spi_write_raw(input logic [7:0] data[]);
    spi_adrv_ctrl_t ctrl      = '0;
    int             num_bytes = data.size();
    logic [31:0]    word      = 'X;

    // Trigger the transaction: write the control register.
    ctrl.dir       = SPI_WRITE;
    ctrl.mode      = MODE_RAW;
    ctrl.num_bytes = num_bytes;
    cp_bfm.write(REG_CONTROL, ctrl);

    // Pack bytes little-endian into 32-bit words and write each to REG_DATA.
    // Each write blocks until the DUT ACKs, which happens once the holding
    // FIFO is free.
    foreach (data[byte_idx]) begin
      word[(byte_idx % 4)*8 +: 8] = data[byte_idx];
      if ((byte_idx % 4 == 3) || (byte_idx == num_bytes-1)) begin
        cp_bfm.write(REG_DATA, word);
        word = 'X;
      end
    end
  endtask : spi_write_raw


  // Read an arbitrary number of bytes over SPI in RAW mode. Both MOSI and MISO
  // are active for every byte; tx_data and rx_data are the same length.
  //
  // Args:
  //   tx_data : Bytes to drive on MOSI, in wire order (tx_data[0] is first).
  //   rx_data : Received bytes from MISO, in wire order.
  //
  task automatic spi_read_raw(input logic [7:0] tx_data[], output logic [7:0] rx_data[]);
    spi_adrv_ctrl_t             ctrl      = '0;
    int                         num_bytes = tx_data.size();
    logic [31:0]                word      = 'X;
    logic [CTRLPORT_DATA_W-1:0] rd_word   = 'X;

    rx_data = new[num_bytes];

    // Trigger the transaction.
    ctrl.dir       = SPI_READ;
    ctrl.mode      = MODE_RAW;
    ctrl.num_bytes = num_bytes;
    cp_bfm.write(REG_CONTROL, ctrl);

    // Write TX words. The first write releases the FSM from ST_WAIT_HOLD;
    // subsequent writes stall until the data in FIFO is consumed.
    foreach (tx_data[byte_idx]) begin
      word[(byte_idx % 4)*8 +: 8] = tx_data[byte_idx];
      if ((byte_idx % 4 == 3) || (byte_idx == num_bytes-1)) begin
        if (VERBOSE) $display("[RAW_RD] TX write: word[%0d] (byte %0d) @ t=%0t",
                              byte_idx/4, byte_idx, $time);
        cp_bfm.write(REG_DATA, word);
        if (VERBOSE) $display("[RAW_RD] TX ack:   word[%0d]           @ t=%0t",
                              byte_idx/4, $time);
        word = 'X;
      end
    end
    if (VERBOSE) $display("[RAW_RD] TX done (%0d bytes), starting RX reads @ t=%0t",
                          num_bytes, $time);

    // Read back RX words. Each read blocks until the DUT ACKs, which happens
    // once four received bytes have been assembled into a word (or at end-of-
    // transaction for a partial word). Unpack little-endian back to bytes.
    foreach (rx_data[byte_idx]) begin
      if (byte_idx % 4 == 0) begin
        if (VERBOSE) $display("[RAW_RD] RX read:  word[%0d] (byte %0d) @ t=%0t",
                              byte_idx/4, byte_idx, $time);
        cp_bfm.read(REG_DATA, rd_word);
        if (VERBOSE) $display("[RAW_RD] RX ack:   word[%0d] = 0x%08h  @ t=%0t",
                              byte_idx/4, rd_word, $time);
      end
      rx_data[byte_idx] = rd_word[(byte_idx % 4)*8 +: 8];
    end
    if (VERBOSE) $display("[RAW_RD] RX done (%0d bytes)               @ t=%0t",
                          num_bytes, $time);
  endtask : spi_read_raw


  // Write an arbitrary number of bytes over SPI in SI_SEQ mode. The FSM sends
  // [addr_hi, addr_lo, data_byte] per byte, auto-incrementing the address for
  // each successive byte.
  //
  // Args:
  //   addr : Starting 16-bit SPI register address (auto-incremented per byte).
  //   data : Data bytes to write, in wire order (data[0] goes to addr).
  //
  task automatic spi_write_si_seq(
    input logic [15:0] addr,
    input logic [7:0]  data[]
  );
    spi_adrv_ctrl_t ctrl      = '0;
    int             num_bytes = data.size();
    logic [31:0]    word      = 'X;

    // Trigger the transaction.
    ctrl.dir       = SPI_WRITE;
    ctrl.mode      = MODE_SI_SEQ;
    ctrl.num_bytes = num_bytes;
    ctrl.addr      = addr;
    cp_bfm.write(REG_CONTROL, ctrl);

    // Pack and write TX words. Each write blocks until the DUT ACKs, which
    // happens once the holding FIFO is free.
    foreach (data[byte_idx]) begin
      word[(byte_idx % 4)*8 +: 8] = data[byte_idx];
      if ((byte_idx % 4 == 3) || (byte_idx == num_bytes-1)) begin
        cp_bfm.write(REG_DATA, word);
        word = 'X;
      end
    end
  endtask : spi_write_si_seq


  // Write an arbitrary number of bytes over SPI in SI_REP mode. The FSM sends
  // [addr_hi, addr_lo, data_byte] per byte; the address is fixed (does not
  // increment) for every byte.
  //
  // Args:
  //   addr : Fixed 16-bit SPI register address used for every byte.
  //   data : Data bytes to write, in wire order.
  //
  task automatic spi_write_si_rep(
    input logic [15:0] addr,
    input logic [7:0]  data[]
  );
    spi_adrv_ctrl_t ctrl      = '0;
    int             num_bytes = data.size();
    logic [31:0]    word      = 'X;

    ctrl.dir       = SPI_WRITE;
    ctrl.mode      = MODE_SI_REP;
    ctrl.num_bytes = num_bytes;
    ctrl.addr      = addr;
    cp_bfm.write(REG_CONTROL, ctrl);

    foreach (data[byte_idx]) begin
      word[(byte_idx % 4)*8 +: 8] = data[byte_idx];
      if ((byte_idx % 4 == 3) || (byte_idx == num_bytes-1)) begin
        cp_bfm.write(REG_DATA, word);
        word = 'X;
      end
    end
  endtask : spi_write_si_rep


  // Read an arbitrary number of bytes over SPI in SI_SEQ mode. The FSM sends
  // [addr_hi, addr_lo, 0x00] per byte, auto-incrementing the address, and
  // discards the two address-echo bytes before capturing the data byte.
  //
  // Args:
  //   addr      : Starting 16-bit SPI register address (auto-incremented per
  //               byte).
  //   num_bytes : Number of data bytes to read.
  //   rx_data   : Received data bytes, in wire order.
  //
  task automatic spi_read_si_seq(
    input  logic [15:0] addr,
    input  int          num_bytes,
    output logic [7:0]  rx_data[]
  );
    spi_adrv_ctrl_t             ctrl = '0;
    logic [CTRLPORT_DATA_W-1:0] rd_word;

    rx_data = new[num_bytes];

    // Trigger the transaction. No TX data is needed: the FSM drives zeros on
    // MOSI during data phases.
    ctrl.dir       = SPI_READ;
    ctrl.mode      = MODE_SI_SEQ;
    ctrl.num_bytes = num_bytes;
    ctrl.addr      = addr;
    cp_bfm.write(REG_CONTROL, ctrl);

    // Read RX words. Each read blocks until the DUT ACKs. Data bytes from all
    // address rounds are packed contiguously into successive words. Unpack
    // little-endian back to bytes.
    foreach (rx_data[byte_idx]) begin
      if (byte_idx % 4 == 0) begin
        cp_bfm.read(REG_DATA, rd_word);
      end
      rx_data[byte_idx] = rd_word[(byte_idx % 4)*8 +: 8];
    end
  endtask : spi_read_si_seq


  // Read an arbitrary number of bytes over SPI in SI_REP mode. The FSM sends
  // [addr_hi, addr_lo, 0x00] per byte with the address fixed, and discards the
  // two address-echo bytes before capturing the data byte.
  //
  // Args:
  //   addr      : Fixed 16-bit SPI register address used for every round.
  //   num_bytes : Number of data bytes to read.
  //   rx_data   : Received data bytes, in wire order.
  //
  task automatic spi_read_si_rep(
    input  logic [15:0] addr,
    input  int          num_bytes,
    output logic [7:0]  rx_data[]
  );
    spi_adrv_ctrl_t             ctrl = '0;
    logic [CTRLPORT_DATA_W-1:0] rd_word;

    rx_data = new[num_bytes];

    ctrl.dir       = SPI_READ;
    ctrl.mode      = MODE_SI_REP;
    ctrl.num_bytes = num_bytes;
    ctrl.addr      = addr;
    cp_bfm.write(REG_CONTROL, ctrl);

    foreach (rx_data[byte_idx]) begin
      if (byte_idx % 4 == 0) begin
        cp_bfm.read(REG_DATA, rd_word);
      end
      rx_data[byte_idx] = rd_word[(byte_idx % 4)*8 +: 8];
    end
  endtask : spi_read_si_rep


  // Write an arbitrary number of bytes over SPI in STREAMING mode. The FSM
  // sends [addr_hi, addr_lo] once, then all data bytes follow in a single CS_N
  // assertion.
  //
  // Args:
  //   addr : 16-bit SPI register address sent at the start of the burst.
  //   data : Data bytes to write, in wire order (data[0] is first after addr).
  //
  task automatic spi_write_streaming(
    input logic [15:0] addr,
    input logic [7:0]  data[]
  );
    spi_adrv_ctrl_t ctrl      = '0;
    int             num_bytes = data.size();
    logic [31:0]    word;

    // Trigger the transaction.
    ctrl.dir       = SPI_WRITE;
    ctrl.mode      = MODE_STREAMING;
    ctrl.num_bytes = num_bytes;
    ctrl.addr      = addr;
    cp_bfm.write(REG_CONTROL, ctrl);

    // Pack and write TX words. Each write blocks until the DUT ACKs, which
    // happens once the holding FIFO is free.
    word = '0;
    foreach (data[byte_idx]) begin
      word[(byte_idx % 4)*8 +: 8] = data[byte_idx];
      if ((byte_idx % 4 == 3) || (byte_idx == num_bytes-1)) begin
        cp_bfm.write(REG_DATA, word);
        word = '0;
      end
    end
  endtask : spi_write_streaming


  // Read an arbitrary number of bytes over SPI in STREAMING mode. The FSM
  // sends [addr_hi, addr_lo] once, discards the two address-echo bytes, then
  // captures all remaining bytes with MOSI driven to zero.
  //
  // Args:
  //   addr      : 16-bit SPI register address sent at the start of the burst.
  //   num_bytes : Number of data bytes to read.
  //   rx_data   : Received data bytes, in wire order.
  //
  task automatic spi_read_streaming(
    input  logic [15:0] addr,
    input  int          num_bytes,
    output logic [7:0]  rx_data[]
  );
    spi_adrv_ctrl_t             ctrl = '0;
    logic [CTRLPORT_DATA_W-1:0] rd_word;

    rx_data = new[num_bytes];

    // Trigger the transaction. No TX data is needed: the FSM drives zeros on
    // MOSI for all data bytes after the address.
    ctrl.dir       = SPI_READ;
    ctrl.mode      = MODE_STREAMING;
    ctrl.num_bytes = num_bytes;
    ctrl.addr      = addr;
    cp_bfm.write(REG_CONTROL, ctrl);

    // Read RX words. Each read blocks until the DUT ACKs. Unpack
    // little-endian back to bytes.
    foreach (rx_data[byte_idx]) begin
      if (byte_idx % 4 == 0) begin
        cp_bfm.read(REG_DATA, rd_word);
      end
      rx_data[byte_idx] = rd_word[(byte_idx % 4)*8 +: 8];
    end
  endtask : spi_read_streaming


  // Verify bytes captured by the MOSI collector against the expected write
  // data. Pops the expected number of bytes from mosi_queue and checks each.
  //
  // Wire byte layout per mode (DUT master perspective, MISO discarded):
  //
  //   RAW       : [data_0, data_1, ..., data_{N-1}]
  //   SI_SEQ    : [addr_hi_0, addr_lo_0, data_0,  addr_hi_1, addr_lo_1, data_1, ...]
  //               (address auto-increments each round)
  //   SI_REP    : [addr_hi, addr_lo, data_0,  addr_hi, addr_lo, data_1, ...]
  //               (address is fixed for every round)
  //   STREAMING : [addr_hi, addr_lo, data_0, data_1, ..., data_{N-1}]
  //
  // Args:
  //   mode       : SPI mode controlling the wire byte layout.
  //   start_addr : Starting SPI address (SI_SEQ/SI_REP/STREAMING address
  //                header).
  //   data       : Data bytes that were written; defines expected values.
  //
  task automatic verify_write(
    input spi_adrv_mode_t mode,
    input logic [15:0]    start_addr,
    input logic [7:0]     data[]
  );
    logic [7:0]  actual;
    logic [7:0]  expected;
    logic [15:0] round_addr;

    case (mode)

      // RAW: Every captured byte is a data byte.
      MODE_RAW: begin
        foreach (data[byte_idx]) begin
          mosi_mailbox.get(actual);
          expected = data[byte_idx];
          `ASSERT_ERROR(actual === expected,
            $sformatf("RAW write byte[%0d]: got 0x%h, expected 0x%h",
                      byte_idx, actual, expected));
        end
      end

      // SI_SEQ: Three bytes per data byte; addr_hi, addr_lo, data.
      // The address auto-increments by 1 for each successive data byte.
      MODE_SI_SEQ: begin
        foreach (data[byte_idx]) begin
          round_addr = start_addr + byte_idx;

          mosi_mailbox.get(actual);
          expected = round_addr[15:8];
          `ASSERT_ERROR(actual === expected,
            $sformatf("SI_SEQ write round[%0d] addr_hi: got 0x%h, expected 0x%h",
                      byte_idx, actual, expected));

          mosi_mailbox.get(actual);
          expected = round_addr[7:0];
          `ASSERT_ERROR(actual === expected,
            $sformatf("SI_SEQ write round[%0d] addr_lo: got 0x%h, expected 0x%h",
                      byte_idx, actual, expected));

          mosi_mailbox.get(actual);
          expected = data[byte_idx];
          `ASSERT_ERROR(actual === expected,
            $sformatf("SI_SEQ write round[%0d] data: got 0x%h, expected 0x%h",
                      byte_idx, actual, expected));
        end
      end

      // SI_REP: Three bytes per data byte; addr_hi, addr_lo, data. The
      // address is fixed (same start_addr repeated for every round).
      MODE_SI_REP: begin
        foreach (data[byte_idx]) begin
          round_addr = start_addr; // fixed

          mosi_mailbox.get(actual);
          expected = round_addr[15:8];
          `ASSERT_ERROR(actual === expected,
            $sformatf("SI_REP write round[%0d] addr_hi: got 0x%h, expected 0x%h",
                      byte_idx, actual, expected));

          mosi_mailbox.get(actual);
          expected = round_addr[7:0];
          `ASSERT_ERROR(actual === expected,
            $sformatf("SI_REP write round[%0d] addr_lo: got 0x%h, expected 0x%h",
                      byte_idx, actual, expected));

          mosi_mailbox.get(actual);
          expected = data[byte_idx];
          `ASSERT_ERROR(actual === expected,
            $sformatf("SI_REP write round[%0d] data: got 0x%h, expected 0x%h",
                      byte_idx, actual, expected));
        end
      end

      // STREAMING: addr_hi and addr_lo once, then all data bytes.
      MODE_STREAMING: begin
        // Verify the one-time address header.
        mosi_mailbox.get(actual);
        expected = start_addr[15:8];
        `ASSERT_ERROR(actual === expected,
          $sformatf("STREAMING write addr_hi: got 0x%h, expected 0x%h",
                    actual, expected));

        mosi_mailbox.get(actual);
        expected = start_addr[7:0];
        `ASSERT_ERROR(actual === expected,
          $sformatf("STREAMING write addr_lo: got 0x%h, expected 0x%h",
                    actual, expected));

        // Verify each data byte.
        foreach (data[byte_idx]) begin
          mosi_mailbox.get(actual);
          expected = data[byte_idx];
          `ASSERT_ERROR(actual === expected,
            $sformatf("STREAMING write data[%0d]: got 0x%h, expected 0x%h",
                      byte_idx, actual, expected));
        end
      end

    endcase
  endtask : verify_write


  // Verify received read data against the expected MISO counter values.
  //
  // The MISO counter resets to 0x00 at each CS_N assertion and increments once
  // per wire byte regardless of mode. Expected data-byte values are:
  //
  //   RAW             : rx_data[k] = byte'(k)
  //   STREAMING       : rx_data[k] = byte'(2 + k)
  //   SI_SEQ / SI_REP : rx_data[k] = byte'(3*k + 2)
  //                     Each data byte is preceded by addr_hi + addr_lo per
  //                     round; address values don't affect the counter.
  //
  // All counter arithmetic is byte-wide (wraps at 256).
  //
  // Args:
  //   mode    : SPI mode used for the read.
  //   rx_data : Bytes received from spi_read_*; compared to expected values.
  //
  task automatic verify_read(
    input spi_adrv_mode_t mode,
    input logic [7:0]     rx_data[]
  );
    foreach (rx_data[byte_idx]) begin
      logic [7:0] expected;

      // Compute the expected counter value for this data byte.
      case (mode)
        MODE_RAW:                    expected = 8'(byte_idx);
        MODE_STREAMING:              expected = 8'(2 + byte_idx);
        MODE_SI_SEQ, MODE_SI_REP:    expected = 8'(3 * byte_idx + 2);
      endcase

      `ASSERT_ERROR(rx_data[byte_idx] === expected,
        $sformatf("%s read byte[%0d]: got 0x%h, expected 0x%h",
                  mode.name(), byte_idx, rx_data[byte_idx], expected));
    end
  endtask : verify_read


  // Execute a single SPI transaction in the specified mode and direction, then
  // verify correctness.
  //
  // For writes: issues the write transaction and verifies the captured MOSI
  // bytes. mosi_mailbox.get() blocks naturally until bytes are clocked out.
  //
  // For reads: issues the read transaction and verifies the received bytes.
  // Read tasks block on REG_DATA until the DUT delivers results.
  //
  // TB_MODE_SI is always a single data byte per CS_N assertion. num_bytes must
  // be 1 when this mode is used; addr[15] is forced to 0 for writes and to 1
  // for reads by the underlying helpers.
  //
  // Args:
  //   mode      : Transaction mode (tb_mode_t, includes TB_MODE_SI).
  //   dir       : Transaction direction (SPI_WRITE or SPI_READ).
  //   num_bytes : Number of data bytes (must be 1 for TB_MODE_SI;
  //               enforced with an assertion).
  //   addr      : 16-bit SPI register address.
  //   tx_data   : Data bytes to transmit (write) or drive on MOSI (RAW read).
  //   rx_data   : Received bytes (populated for read transactions).
  //
  task automatic run_test(
    input  tb_mode_t       mode,
    input  spi_adrv_dir_t  dir,
    input  int             num_bytes,
    input  logic [15:0]    addr,
    input  logic [7:0]     tx_data[],
    output logic [7:0]     rx_data[]
  );
    if (VERBOSE) begin
      $display("[TEST] mode: %-14s dir: %-9s num_bytes: %-03d addr: 0x%04h",
               mode.name(), dir.name(), num_bytes, addr);
    end

    if (mode == TB_MODE_SI) begin

      //-----------------------------------------------------------------------
      // TB_MODE_SI: single [addr_hi, addr_lo, data] triplet via REG_SI.
      // Always exactly 1 data byte per transaction.
      //-----------------------------------------------------------------------
      `ASSERT_ERROR(num_bytes == 1,
        $sformatf("TB_MODE_SI requires num_bytes=1, got %0d", num_bytes));
      if (dir == SPI_WRITE) begin
        logic [7:0]  actual;
        logic [15:0] wr_addr = addr & 16'h7FFF; // force bit 15=0 (write dir)

        spi_si_write(wr_addr, tx_data[0]);
        expected_cs_n_count++;

        // Verify 3 MOSI bytes: addr_hi, addr_lo, data.
        mosi_mailbox.get(actual);
        `ASSERT_ERROR(actual === wr_addr[15:8],
          $sformatf("SI write addr_hi: got 0x%h, expected 0x%h",
                    actual, wr_addr[15:8]));
        mosi_mailbox.get(actual);
        `ASSERT_ERROR(actual === wr_addr[7:0],
          $sformatf("SI write addr_lo: got 0x%h, expected 0x%h",
                    actual, wr_addr[7:0]));
        mosi_mailbox.get(actual);
        `ASSERT_ERROR(actual === tx_data[0],
          $sformatf("SI write data: got 0x%h, expected 0x%h",
                    actual, tx_data[0]));

      end else begin  // SPI_READ
        logic [31:0] rd_word;
        logic [7:0]  dummy;

        // spi_si_read forces bit 15=1 (read dir) and blocks on REG_DATA.
        spi_si_read(addr, rd_word);
        expected_cs_n_count++;

        // MISO counter resets per CS_N; values are always 0x00, 0x01, 0x02.
        `ASSERT_ERROR(rd_word[7:0]   === 8'h00,
          $sformatf("SI read byte0: got 0x%h, expected 0x00", rd_word[7:0]));
        `ASSERT_ERROR(rd_word[15:8]  === 8'h01,
          $sformatf("SI read byte1: got 0x%h, expected 0x01", rd_word[15:8]));
        `ASSERT_ERROR(rd_word[23:16] === 8'h02,
          $sformatf("SI read byte2: got 0x%h, expected 0x02", rd_word[23:16]));

        rx_data = new[1];
        rx_data[0] = rd_word[23:16];

        // Drain the 3 MOSI bytes (addr_hi with R/W=1, addr_lo, 0x00 dummy).
        repeat (3) mosi_mailbox.get(dummy);
      end

    end else begin

      //-----------------------------------------------------------------------
      // Hardware control-register modes (0-3): cast tb_mode_t to spi_adrv_mode_t.
      //-----------------------------------------------------------------------
      spi_adrv_mode_t hw_mode = spi_adrv_mode_t'(mode);

      if (dir == SPI_WRITE) begin

        // Issue the write transaction.
        case (hw_mode)
          MODE_RAW:       spi_write_raw(tx_data);
          MODE_SI_SEQ:    spi_write_si_seq(addr, tx_data);
          MODE_SI_REP:    spi_write_si_rep(addr, tx_data);
          MODE_STREAMING: spi_write_streaming(addr, tx_data);
        endcase

        // mosi_mailbox.get() inside verify_write() blocks until each byte
        // arrives naturally; no need to stall for CS_N deassert here.
        expected_cs_n_count++;
        verify_write(hw_mode, addr, tx_data);

      end else begin  // SPI_READ

        // Issue the read transaction.
        case (hw_mode)
          MODE_RAW:       spi_read_raw(tx_data, rx_data);
          MODE_SI_SEQ:    spi_read_si_seq(addr, num_bytes, rx_data);
          MODE_SI_REP:    spi_read_si_rep(addr, num_bytes, rx_data);
          MODE_STREAMING: spi_read_streaming(addr, num_bytes, rx_data);
        endcase

        // Read tasks block until all data arrives; the transaction is already
        // complete by the time we reach here.
        expected_cs_n_count++;
        verify_read(hw_mode, rx_data);

        // Drain MOSI bytes captured during this read transaction so they
        // do not pollute the mailbox for subsequent write verifications.
        // Expected wire-byte counts:
        //   RAW:       num_bytes
        //   SI_SEQ/REP: num_bytes * 3  (addr_hi + addr_lo + 0x00 per byte)
        //   STREAMING: 2 + num_bytes   (one addr header + data bytes)
        begin
          logic [7:0] dummy;
          int         drain_count;
          case (hw_mode)
            MODE_RAW:       drain_count = num_bytes;
            MODE_SI_SEQ:    drain_count = num_bytes * 3;
            MODE_SI_REP:    drain_count = num_bytes * 3;
            MODE_STREAMING: drain_count = 2 + num_bytes;
            default:        drain_count = 0;
          endcase
          repeat (drain_count) mosi_mailbox.get(dummy);
        end

      end

    end
  endtask : run_test


  // Run a randomized stress test, exercising all mode/direction combinations
  // with random byte counts, addresses, and data.
  //
  // Range notes:
  //   - num_bytes : 1-MAX_NUM_BYTES
  //   - addr      : full 16-bit range
  //   - data      : uniformly random per-byte
  //
  // Writes are verified via the MOSI capture mailbox (mosi_mailbox). Reads are
  // verified against the deterministic MISO counter formula.
  //
  // Args:
  //   num_iters : Number of random transactions to execute and verify.
  //
  task automatic run_rand_tests(input int num_iters);
    tb_mode_t      mode;
    spi_adrv_dir_t dir;
    int            num_bytes;
    logic [15:0]   addr;
    logic [7:0]    tx_data[];
    logic [7:0]    rx_data[];

    test.start_test($sformatf("Randomized tests (%0d iterations)", num_iters));
    if (VERBOSE) $display("[RAND] num_iters=%0d", num_iters);

    repeat (num_iters) begin

      // Randomize all transaction parameters across all 5 modes.
      mode = tb_mode_t'($urandom_range(0, 4));
      dir  = spi_adrv_dir_t'($urandom_range(0, 1));
      addr = $urandom();
      num_bytes = mode == TB_MODE_SI ?
        1 : $urandom_range(1, MAX_NUM_BYTES);

      tx_data = new[num_bytes];
      foreach (tx_data[byte_idx]) begin
        tx_data[byte_idx] = 8'($urandom);
      end

      run_test(mode, dir, num_bytes, addr, tx_data, rx_data);

    end
    test.end_test();
  endtask : run_rand_tests


  //---------------------------------------------------------------------------
  // SI address-behavior test
  //---------------------------------------------------------------------------
  //
  // Directly verifies the only behavioral difference between SI_SEQ and
  // SI_REP: whether the address on MOSI increments per byte (SI_SEQ) or stays
  // fixed (SI_REP).
  //
  // Uses a 4-byte transaction so multiple rounds are exercised, with a
  // starting address chosen to exercise both bytes of the 16-bit field.
  //
  //---------------------------------------------------------------------------

  task automatic test_si_addr_behavior();
    localparam logic [15:0] START_ADDR = 16'hA5BC;
    localparam int          N          = 4;
    logic [7:0] tx_data[N];
    logic [7:0] rx_data[];

    test.start_test("SI_SEQ vs SI_REP address increment behavior");
    foreach (tx_data[i]) tx_data[i] = 8'(i + 1);

    //-------------------------------------------------------------------------
    // SI_SEQ write: Address must increment each round
    //-------------------------------------------------------------------------
    spi_write_si_seq(START_ADDR, tx_data);
    expected_cs_n_count++;
    begin
      logic [7:0] actual;
      for (int i = 0; i < N; i++) begin
        logic [15:0] expected_addr = START_ADDR + i;

        mosi_mailbox.get(actual);  // addr_hi
        `ASSERT_ERROR(actual === expected_addr[15:8],
          $sformatf("SI_SEQ addr_hi round %0d: got 0x%h, expected 0x%h",
                    i, actual, expected_addr[15:8]));

        mosi_mailbox.get(actual);  // addr_lo
        `ASSERT_ERROR(actual === expected_addr[7:0],
          $sformatf("SI_SEQ addr_lo round %0d: got 0x%h, expected 0x%h",
                    i, actual, expected_addr[7:0]));

        mosi_mailbox.get(actual);  // data
        `ASSERT_ERROR(actual === tx_data[i],
          $sformatf("SI_SEQ data round %0d: got 0x%h, expected 0x%h",
                    i, actual, tx_data[i]));
      end
    end

    //-------------------------------------------------------------------------
    // SI_REP write: Address must be identical for every round
    //-------------------------------------------------------------------------
    spi_write_si_rep(START_ADDR, tx_data);
    expected_cs_n_count++;
    begin
      logic [7:0] actual;
      for (int i = 0; i < N; i++) begin
        mosi_mailbox.get(actual);  // addr_hi
        `ASSERT_ERROR(actual === START_ADDR[15:8],
          $sformatf("SI_REP addr_hi round %0d: got 0x%h, expected 0x%h (fixed)",
                    i, actual, START_ADDR[15:8]));

        mosi_mailbox.get(actual);  // addr_lo
        `ASSERT_ERROR(actual === START_ADDR[7:0],
          $sformatf("SI_REP addr_lo round %0d: got 0x%h, expected 0x%h (fixed)",
                    i, actual, START_ADDR[7:0]));

        mosi_mailbox.get(actual);  // data
        `ASSERT_ERROR(actual === tx_data[i],
          $sformatf("SI_REP data round %0d: got 0x%h, expected 0x%h",
                    i, actual, tx_data[i]));
      end
    end

    //-------------------------------------------------------------------------
    // SI_SEQ read: MISO counter formula = 3*k+2 (address values don't matter)
    //-------------------------------------------------------------------------
    spi_read_si_seq(START_ADDR, N, rx_data);
    expected_cs_n_count++;
    verify_read(MODE_SI_SEQ, rx_data);
    begin
      logic [7:0] dummy;
      repeat (N * 3) mosi_mailbox.get(dummy);
    end

    //-------------------------------------------------------------------------
    // SI_REP read: Same MISO counter formula, different address on MOSI
    //-------------------------------------------------------------------------
    spi_read_si_rep(START_ADDR, N, rx_data);
    expected_cs_n_count++;
    verify_read(MODE_SI_REP, rx_data);
    begin
      logic [7:0] actual;
      // Also confirm MOSI address bytes are all fixed at START_ADDR
      for (int i = 0; i < N; i++) begin
        mosi_mailbox.get(actual);
        `ASSERT_ERROR(actual === START_ADDR[15:8],
          $sformatf("SI_REP read addr_hi round %0d: got 0x%h, expected 0x%h",
                    i, actual, START_ADDR[15:8]));
        mosi_mailbox.get(actual);
        `ASSERT_ERROR(actual === START_ADDR[7:0],
          $sformatf("SI_REP read addr_lo round %0d: got 0x%h, expected 0x%h",
                    i, actual, START_ADDR[7:0]));
        mosi_mailbox.get(actual);  // 0x00 driven by DUT during data phase
      end
    end
    test.end_test();

  endtask : test_si_addr_behavior


  //---------------------------------------------------------------------------
  // Simple RAW mode smoke test
  //---------------------------------------------------------------------------

  task automatic test_raw_smoke();
    typedef struct { bit is_read; int num_bytes; } tc_t;
    tc_t tests[] = '{
      '{0, 3},
      '{1, 3},
      '{0, 8},
      '{1, 8}
    };
    logic [7:0] tx_data[];
    logic [7:0] rx_data[];
    logic [7:0] dummy;

    foreach (tests[i]) begin
      automatic string name = $sformatf("RAW %s %0d bytes",
        tests[i].is_read ? "read" : "write", tests[i].num_bytes);
      test.start_test(name);
      if (!tests[i].is_read) begin
        tx_data = new[tests[i].num_bytes];
        foreach (tx_data[j]) tx_data[j] = 8'(j + 1);
        spi_write_raw(tx_data);
        expected_cs_n_count++;
        verify_write(MODE_RAW, 16'h0000, tx_data);
      end else begin
        tx_data = new[tests[i].num_bytes];
        foreach (tx_data[j]) tx_data[j] = 8'(j + 1);
        spi_read_raw(tx_data, rx_data);
        expected_cs_n_count++;
        verify_read(MODE_RAW, rx_data);
        repeat (tests[i].num_bytes) mosi_mailbox.get(dummy);
      end
      test.end_test();
    end

  endtask : test_raw_smoke


  //---------------------------------------------------------------------------
  // Simple SI_SEQ mode smoke test
  //---------------------------------------------------------------------------

  task automatic test_si_seq_smoke();
    localparam logic [15:0] ADDR = 16'h1234;
    typedef struct { bit is_read; int num_bytes; } tc_t;
    tc_t tests[] = '{
      '{0, 3},
      '{1, 3},
      '{0, 8},
      '{1, 8}
    };
    logic [7:0] tx_data[];
    logic [7:0] rx_data[];
    logic [7:0] dummy;

    foreach (tests[i]) begin
      automatic string name = $sformatf("SI_SEQ %s %0d bytes",
        tests[i].is_read ? "read" : "write", tests[i].num_bytes);
      test.start_test(name);
      if (!tests[i].is_read) begin
        tx_data = new[tests[i].num_bytes];
        foreach (tx_data[j]) tx_data[j] = 8'(j + 1);
        spi_write_si_seq(ADDR, tx_data);
        expected_cs_n_count++;
        verify_write(MODE_SI_SEQ, ADDR, tx_data);
      end else begin
        spi_read_si_seq(ADDR, tests[i].num_bytes, rx_data);
        expected_cs_n_count++;
        verify_read(MODE_SI_SEQ, rx_data);
        repeat (tests[i].num_bytes * 3) mosi_mailbox.get(dummy);
      end
      test.end_test();
    end

  endtask : test_si_seq_smoke


  //---------------------------------------------------------------------------
  // Simple STREAMING mode smoke test
  //---------------------------------------------------------------------------

  task automatic test_streaming_smoke();
    localparam logic [15:0] ADDR = 16'h5678;
    typedef struct { bit is_read; int num_bytes; } tc_t;
    tc_t tests[] = '{
      '{0, 3},
      '{1, 3},
      '{0, 8},
      '{1, 8}
    };
    logic [7:0] tx_data[];
    logic [7:0] rx_data[];
    logic [7:0] dummy;

    foreach (tests[i]) begin
      automatic string name = $sformatf("STREAMING %s %0d bytes",
        tests[i].is_read ? "read" : "write", tests[i].num_bytes);
      test.start_test(name);
      if (!tests[i].is_read) begin
        tx_data = new[tests[i].num_bytes];
        foreach (tx_data[j]) tx_data[j] = 8'(j + 1);
        spi_write_streaming(ADDR, tx_data);
        expected_cs_n_count++;
        verify_write(MODE_STREAMING, ADDR, tx_data);
      end else begin
        spi_read_streaming(ADDR, tests[i].num_bytes, rx_data);
        expected_cs_n_count++;
        verify_read(MODE_STREAMING, rx_data);
        // 2 address bytes + num_bytes data bytes echoed on MOSI
        repeat (2 + tests[i].num_bytes) mosi_mailbox.get(dummy);
      end
      test.end_test();
    end

  endtask : test_streaming_smoke


  //---------------------------------------------------------------------------
  // Simple REG_SI register smoke test
  //---------------------------------------------------------------------------
  //
  // Three back-to-back REG_SI writes followed by three back-to-back REG_SI
  // reads. Each is its own test case for easy waveform navigation.
  //
  //---------------------------------------------------------------------------

  task automatic test_si_reg_smoke();
    // Write addresses (bit 15 must be 0) and data bytes.
    localparam logic [15:0] WR_ADDR0 = 16'h0011;
    localparam logic [15:0] WR_ADDR1 = 16'h0022;
    localparam logic [15:0] WR_ADDR2 = 16'h0033;
    localparam logic [7:0]  WR_DATA0 = 8'hAA;
    localparam logic [7:0]  WR_DATA1 = 8'hBB;
    localparam logic [7:0]  WR_DATA2 = 8'hCC;
    localparam logic [15:0] RD_ADDR0 = 16'h0044;
    localparam logic [15:0] RD_ADDR1 = 16'h0055;
    localparam logic [15:0] RD_ADDR2 = 16'h0066;

    logic [7:0]  actual;
    logic [31:0] rd_word;

    // --- Write 1 ---
    test.start_test("REG_SI write 1");
    spi_si_write(WR_ADDR0, WR_DATA0);
    expected_cs_n_count++;
    mosi_mailbox.get(actual);
    `ASSERT_ERROR(actual === WR_ADDR0[15:8],
      $sformatf("REG_SI write1 addr_hi: got 0x%h, expected 0x%h", actual, WR_ADDR0[15:8]));
    mosi_mailbox.get(actual);
    `ASSERT_ERROR(actual === WR_ADDR0[7:0],
      $sformatf("REG_SI write1 addr_lo: got 0x%h, expected 0x%h", actual, WR_ADDR0[7:0]));
    mosi_mailbox.get(actual);
    `ASSERT_ERROR(actual === WR_DATA0,
      $sformatf("REG_SI write1 data: got 0x%h, expected 0x%h", actual, WR_DATA0));
    test.end_test();

    // --- Write 2 ---
    test.start_test("REG_SI write 2");
    spi_si_write(WR_ADDR1, WR_DATA1);
    expected_cs_n_count++;
    mosi_mailbox.get(actual);
    `ASSERT_ERROR(actual === WR_ADDR1[15:8],
      $sformatf("REG_SI write2 addr_hi: got 0x%h, expected 0x%h", actual, WR_ADDR1[15:8]));
    mosi_mailbox.get(actual);
    `ASSERT_ERROR(actual === WR_ADDR1[7:0],
      $sformatf("REG_SI write2 addr_lo: got 0x%h, expected 0x%h", actual, WR_ADDR1[7:0]));
    mosi_mailbox.get(actual);
    `ASSERT_ERROR(actual === WR_DATA1,
      $sformatf("REG_SI write2 data: got 0x%h, expected 0x%h", actual, WR_DATA1));
    test.end_test();

    // --- Write 3 ---
    test.start_test("REG_SI write 3");
    spi_si_write(WR_ADDR2, WR_DATA2);
    expected_cs_n_count++;
    mosi_mailbox.get(actual);
    `ASSERT_ERROR(actual === WR_ADDR2[15:8],
      $sformatf("REG_SI write3 addr_hi: got 0x%h, expected 0x%h", actual, WR_ADDR2[15:8]));
    mosi_mailbox.get(actual);
    `ASSERT_ERROR(actual === WR_ADDR2[7:0],
      $sformatf("REG_SI write3 addr_lo: got 0x%h, expected 0x%h", actual, WR_ADDR2[7:0]));
    mosi_mailbox.get(actual);
    `ASSERT_ERROR(actual === WR_DATA2,
      $sformatf("REG_SI write3 data: got 0x%h, expected 0x%h", actual, WR_DATA2));
    test.end_test();

    // --- Read 1 ---
    // MISO counter resets per CS_N; bytes are always 0x00, 0x01, 0x02.
    test.start_test("REG_SI read 1");
    spi_si_read(RD_ADDR0, rd_word);
    expected_cs_n_count++;
    `ASSERT_ERROR(rd_word[7:0]   === 8'h00,
      $sformatf("REG_SI read1 byte0: got 0x%h, expected 0x00", rd_word[7:0]));
    `ASSERT_ERROR(rd_word[15:8]  === 8'h01,
      $sformatf("REG_SI read1 byte1: got 0x%h, expected 0x01", rd_word[15:8]));
    `ASSERT_ERROR(rd_word[23:16] === 8'h02,
      $sformatf("REG_SI read1 byte2: got 0x%h, expected 0x02", rd_word[23:16]));
    begin logic [7:0] dummy; repeat (3) mosi_mailbox.get(dummy); end
    test.end_test();

    // --- Read 2 ---
    test.start_test("REG_SI read 2");
    spi_si_read(RD_ADDR1, rd_word);
    expected_cs_n_count++;
    `ASSERT_ERROR(rd_word[7:0]   === 8'h00,
      $sformatf("REG_SI read2 byte0: got 0x%h, expected 0x00", rd_word[7:0]));
    `ASSERT_ERROR(rd_word[15:8]  === 8'h01,
      $sformatf("REG_SI read2 byte1: got 0x%h, expected 0x01", rd_word[15:8]));
    `ASSERT_ERROR(rd_word[23:16] === 8'h02,
      $sformatf("REG_SI read2 byte2: got 0x%h, expected 0x02", rd_word[23:16]));
    begin logic [7:0] dummy; repeat (3) mosi_mailbox.get(dummy); end
    test.end_test();

    // --- Read 3 ---
    test.start_test("REG_SI read 3");
    spi_si_read(RD_ADDR2, rd_word);
    expected_cs_n_count++;
    `ASSERT_ERROR(rd_word[7:0]   === 8'h00,
      $sformatf("REG_SI read3 byte0: got 0x%h, expected 0x00", rd_word[7:0]));
    `ASSERT_ERROR(rd_word[15:8]  === 8'h01,
      $sformatf("REG_SI read3 byte1: got 0x%h, expected 0x01", rd_word[15:8]));
    `ASSERT_ERROR(rd_word[23:16] === 8'h02,
      $sformatf("REG_SI read3 byte2: got 0x%h, expected 0x02", rd_word[23:16]));
    begin logic [7:0] dummy; repeat (3) mosi_mailbox.get(dummy); end
    test.end_test();

  endtask : test_si_reg_smoke


  //---------------------------------------------------------------------------
  // Boundary size test
  //---------------------------------------------------------------------------
  //
  // Exercises the minimum (1 byte) and maximum (MAX_NUM_BYTES) transfer sizes
  // in every mode and both directions.
  //
  //---------------------------------------------------------------------------

  task automatic test_max_and_min();
    spi_adrv_mode_t mode = mode.first();
    logic [7:0]     tx_data[];
    logic [7:0]     rx_data[];

    do begin
      //-----------------------------------------------------------------------
      // 1-byte write and read
      //-----------------------------------------------------------------------
      test.start_test($sformatf("Min %s write 1 byte", mode.name()));
      tx_data    = new[1];
      tx_data[0] = 8'hA5;
      run_test(tb_mode_t'(mode), SPI_WRITE, 1, 16'hABCD, tx_data, rx_data);
      test.end_test();

      test.start_test($sformatf("Min %s read 1 byte", mode.name()));
      tx_data    = new[1];
      tx_data[0] = 8'h00;
      run_test(tb_mode_t'(mode), SPI_READ, 1, 16'hEF01, tx_data, rx_data);
      test.end_test();

      //-----------------------------------------------------------------------
      // Maximum-size write and read
      //-----------------------------------------------------------------------
      test.start_test($sformatf("Max %s write %0d bytes", mode.name(), MAX_NUM_BYTES));
      tx_data = new[MAX_NUM_BYTES];
      foreach (tx_data[byte_idx]) begin
        tx_data[byte_idx] = byte_idx + 1;
      end
      run_test(tb_mode_t'(mode), SPI_WRITE, MAX_NUM_BYTES, 16'hDEAD, tx_data, rx_data);
      test.end_test();

      test.start_test($sformatf("Max %s read %0d bytes", mode.name(), MAX_NUM_BYTES));
      tx_data = new[MAX_NUM_BYTES];
      foreach (tx_data[byte_idx]) begin
        tx_data[byte_idx] = 8'h00;
      end
      run_test(tb_mode_t'(mode), SPI_READ, MAX_NUM_BYTES, 16'hBEEF, tx_data, rx_data);
      test.end_test();

      mode = mode.next();
    end while (mode != mode.first());
  endtask : test_max_and_min


  //---------------------------------------------------------------------------
  // REG_SI test
  //---------------------------------------------------------------------------
  //
  // Verifies REG_SI writes (3 MOSI bytes checked) and reads (all 3 MISO bytes
  // captured and verified against the MISO counter).
  //
  // MISO counter resets to 0x00 on each CS_N assertion and increments once per
  // wire byte. For a 3-byte transaction the received bytes are:
  //
  //   wire byte 0 (addr_hi): MISO=0x00
  //   wire byte 1 (addr_lo): MISO=0x01
  //   wire byte 2 (data)   : MISO=0x02
  //
  //---------------------------------------------------------------------------

  task automatic test_si();
    logic [15:0] addr;
    logic [7:0]  data_byte;
    logic [7:0]  actual;
    logic [31:0] rx_word;

    test.start_test("REG_SI single-instruction register");
    //-------------------------------------------------------------------------
    // Fixed-value write transaction
    //-------------------------------------------------------------------------
    addr      = 16'h1234;
    data_byte = 8'hAB;

    spi_si_write(addr, data_byte);
    expected_cs_n_count++;
    mosi_mailbox.get(actual);
    `ASSERT_ERROR(actual === addr[15:8],
      $sformatf("REG_SI write addr_hi: got 0x%h, expected 0x%h", actual, addr[15:8]));

    mosi_mailbox.get(actual);
    `ASSERT_ERROR(actual === addr[7:0],
      $sformatf("REG_SI write addr_lo: got 0x%h, expected 0x%h", actual, addr[7:0]));

    mosi_mailbox.get(actual);
    `ASSERT_ERROR(actual === data_byte,
      $sformatf("REG_SI write data: got 0x%h, expected 0x%h", actual, data_byte));

    //-------------------------------------------------------------------------
    // Fixed-value read transaction: verify all 3 MISO bytes
    //-------------------------------------------------------------------------
    addr = 16'h0034;

    spi_si_read(addr, rx_word);
    expected_cs_n_count++;
    `ASSERT_ERROR(rx_word[7:0]   === 8'h00,
      $sformatf("REG_SI read byte0: got 0x%h, expected 0x00", rx_word[7:0]));
    `ASSERT_ERROR(rx_word[15:8]  === 8'h01,
      $sformatf("REG_SI read byte1: got 0x%h, expected 0x01", rx_word[15:8]));
    `ASSERT_ERROR(rx_word[23:16] === 8'h02,
      $sformatf("REG_SI read byte2: got 0x%h, expected 0x02", rx_word[23:16]));

    // Drain 3 MOSI bytes (addr_hi with R/W=1, addr_lo, 0x00 dummy data).
    begin
      logic [7:0] dummy;
      repeat (3) mosi_mailbox.get(dummy);
    end

    //-------------------------------------------------------------------------
    // Randomized write transactions
    //-------------------------------------------------------------------------
    repeat (4) begin
      addr      = $urandom() & 16'h7FFF; // keep bit 15=0 for writes
      data_byte = $urandom();

      spi_si_write(addr, data_byte);
      expected_cs_n_count++;
      mosi_mailbox.get(actual);
      `ASSERT_ERROR(actual === addr[15:8],
        $sformatf("REG_SI (rand) write addr_hi: got 0x%h, expected 0x%h",
                  actual, addr[15:8]));

      mosi_mailbox.get(actual);
      `ASSERT_ERROR(actual === addr[7:0],
        $sformatf("REG_SI (rand) write addr_lo: got 0x%h, expected 0x%h",
                  actual, addr[7:0]));

      mosi_mailbox.get(actual);
      `ASSERT_ERROR(actual === data_byte,
        $sformatf("REG_SI (rand) write data: got 0x%h, expected 0x%h",
                  actual, data_byte));
    end

    //-------------------------------------------------------------------------
    // Randomized read transactions: verify all 3 MISO bytes each time
    //-------------------------------------------------------------------------
    repeat (4) begin
      addr = $urandom();

      spi_si_read(addr, rx_word);
      expected_cs_n_count++;
      `ASSERT_ERROR(rx_word[7:0]   === 8'h00,
        $sformatf("REG_SI (rand) read byte0: got 0x%h, expected 0x00", rx_word[7:0]));
      `ASSERT_ERROR(rx_word[15:8]  === 8'h01,
        $sformatf("REG_SI (rand) read byte1: got 0x%h, expected 0x01", rx_word[15:8]));
      `ASSERT_ERROR(rx_word[23:16] === 8'h02,
        $sformatf("REG_SI (rand) read byte2: got 0x%h, expected 0x02", rx_word[23:16]));

      begin
        logic [7:0] dummy;
        repeat (3) mosi_mailbox.get(dummy);
      end
    end
    test.end_test();

  endtask : test_si


  //---------------------------------------------------------------------------
  // REG_SI burst test
  //---------------------------------------------------------------------------
  //
  // Issues BURST_N back-to-back REG_SI writes (burst write phase), then issues
  // BURST_N back-to-back REG_SI reads followed by BURST_N REG_DATA reads
  // (burst read phase).
  //
  // BURST_N = 2**(NUM_BYTES_W-2) equals the RX FIFO depth. Each read
  // transaction now stores 3 bytes (one partial word) in the FIFO, so this is
  // still within the FIFO capacity.
  //
  // Each REG_SI CtrlPort write stalls until the previous latch is consumed
  // (backpressure), so writes naturally pipeline while SPI transactions run.
  //
  //---------------------------------------------------------------------------

  task automatic test_si_burst();
    localparam int BURST_N = 2**(NUM_BYTES_W-2);

    logic [15:0] wr_addrs[BURST_N];
    logic [7:0]  wr_data [BURST_N];
    logic [15:0] rd_addrs[BURST_N];
    logic [31:0] rd_words[BURST_N];
    logic [7:0]  actual;

    test.start_test($sformatf("REG_SI burst (%0d transactions)", BURST_N));
    // Randomize write addresses (bit 15 = 0 -> write direction) and data bytes.
    foreach (wr_addrs[i]) begin
      wr_addrs[i] = $urandom() & 16'h7FFF;
      wr_data[i]  = $urandom();
    end
    foreach (rd_addrs[i]) rd_addrs[i] = $urandom();

    //-----------------------------------------------------------------------
    // Burst write phase: fire all BURST_N writes back-to-back, then
    // immediately start the read phase.
    //
    // spi_si_write stalls on si_pending CtrlPort backpressure so only one
    // latch is pending at a time and the FPGA executes each write in order.
    // mosi_mailbox.get() blocks until each byte is clocked out, confirming the
    // full wire transaction completed.
    //-----------------------------------------------------------------------
    foreach (wr_addrs[i]) spi_si_write(wr_addrs[i], wr_data[i]);

    foreach (wr_addrs[i]) begin
      mosi_mailbox.get(actual);
      `ASSERT_ERROR(actual === wr_addrs[i][15:8],
        $sformatf("burst write [%0d] addr_hi: got 0x%h, expected 0x%h",
                  i, actual, wr_addrs[i][15:8]));
      mosi_mailbox.get(actual);
      `ASSERT_ERROR(actual === wr_addrs[i][7:0],
        $sformatf("burst write [%0d] addr_lo: got 0x%h, expected 0x%h",
                  i, actual, wr_addrs[i][7:0]));
      mosi_mailbox.get(actual);
      `ASSERT_ERROR(actual === wr_data[i],
        $sformatf("burst write [%0d] data: got 0x%h, expected 0x%h",
                  i, actual, wr_data[i]));
    end

    //-----------------------------------------------------------------------
    // Burst read phase: two sub-phases, NOT interleaved.
    //
    // Sub-phase 1: fire all BURST_N REG_SI pokes back-to-back. Each poke
    //   blocks on si_pending backpressure; the FPGA executes each read
    //   transaction in order and pushes the 3 MISO bytes into the RX FIFO.
    //   BURST_N is sized to equal the FIFO depth, so all results fit without
    //   overflow.
    //
    // Sub-phase 2:  drain all BURST_N results with back-to-back REG_DATA
    //   reads. Each read blocks until a FIFO word is available.
    //-----------------------------------------------------------------------
    foreach (rd_addrs[i]) begin
      logic [31:0] reg_val;
      logic [15:0] rd_addr_rw = rd_addrs[i] | 16'h8000;
      reg_val[7:0]   = rd_addr_rw[15:8];
      reg_val[15:8]  = rd_addr_rw[7:0];
      reg_val[23:16] = 8'h00;
      reg_val[31:24] = 8'h00;
      cp_bfm.write(BASE_ADDR + REG_SI, reg_val);
    end

    foreach (rd_addrs[i]) cp_bfm.read(BASE_ADDR + REG_DATA, rd_words[i]);

    // Verify all three MISO bytes. Counter resets per CS_N so values are
    // always 0x00, 0x01, 0x02 regardless of address.
    foreach (rd_addrs[i]) begin
      `ASSERT_ERROR(rd_words[i][7:0]   === 8'h00,
        $sformatf("burst read [%0d] byte0: got 0x%h, expected 0x00",
                  i, rd_words[i][7:0]));
      `ASSERT_ERROR(rd_words[i][15:8]  === 8'h01,
        $sformatf("burst read [%0d] byte1: got 0x%h, expected 0x01",
                  i, rd_words[i][15:8]));
      `ASSERT_ERROR(rd_words[i][23:16] === 8'h02,
        $sformatf("burst read [%0d] byte2: got 0x%h, expected 0x02",
                  i, rd_words[i][23:16]));
    end

    // Verify all three MOSI bytes for every read transaction. The FPGA drives
    // addr_hi with bit 7 forced to 1 (R/W# = read), addr_lo, then 0x00 as the
    // dummy data byte.
    foreach (rd_addrs[i]) begin
      logic [15:0] rd_addr_rw = rd_addrs[i] | 16'h8000;
      mosi_mailbox.get(actual);
      `ASSERT_ERROR(actual === rd_addr_rw[15:8],
        $sformatf("burst read [%0d] MOSI addr_hi: got 0x%h, expected 0x%h",
                  i, actual, rd_addr_rw[15:8]));
      mosi_mailbox.get(actual);
      `ASSERT_ERROR(actual === rd_addr_rw[7:0],
        $sformatf("burst read [%0d] MOSI addr_lo: got 0x%h, expected 0x%h",
                  i, actual, rd_addr_rw[7:0]));
      mosi_mailbox.get(actual);
      `ASSERT_ERROR(actual === 8'h00,
        $sformatf("burst read [%0d] MOSI dummy: got 0x%h, expected 0x00", i, actual));
    end
    expected_cs_n_count += 2 * BURST_N;
    test.end_test();

  endtask : test_si_burst


  //---------------------------------------------------------------------------
  // REG_SI: N writes then 1 read (pipeline stress test)
  //---------------------------------------------------------------------------
  //
  // Models the exact failure scenario from hardware: a run of back-to-back SI
  // writes via REG_SI, followed immediately by a single SI read with NO wait
  // between the last write and the read. The read transaction is queued while
  // the last write's SPI transaction may still be running inside axis_spi
  // (si_pending clears as soon as si_start fires, before the physical SPI
  // bytes have all been clocked out).
  //
  // Each iteration applies NUM_WRITES back-to-back writes then issues one read
  // and verifies all three returned bytes.
  //
  // If this test passes in simulation but fails on hardware, the root cause is
  // a timing condition not captured by the BFM (e.g., the CS_GUARD window seen
  // by axis_spi).
  //
  //---------------------------------------------------------------------------

  task automatic test_si_n_writes_then_read();
    logic [15:0] wr_addr;
    logic [7:0]  wr_data;
    logic [15:0] rd_addr;
    logic [7:0]  actual;
    logic [7:0]  dummy;
    logic [31:0] rd_word;
    logic [15:0] exp_addr;
    logic [7:0]  exp_data;

    test.start_test("REG_SI N writes then 1 read (pipeline stress)");
    for (int num_writes = 1; num_writes <= 8; num_writes++) begin
      wr_addr = $urandom() & 16'h7FFF;  // bit 15=0 for writes
      wr_data = $urandom();
      rd_addr = $urandom();

      // Issue num_writes back-to-back writes. Each poke stalls on !si_pending
      // and returns immediately on ACK, before the SPI transaction completes.
      for (int i = 0; i < num_writes; i++)
        spi_si_write(wr_addr + 16'(i), wr_data + 8'(i));

      // Issue the read immediately after the writes. spi_si_read blocks on
      // REG_DATA until the FPGA has the read result, meaning all num_writes+1
      // SPI transactions have completed (all MOSI bytes are already in the
      // mailbox by then).
      spi_si_read(rd_addr, rd_word);

      // spi_si_read() blocked on REG_DATA until the read result was available,
      // so all num_writes+1 SPI transactions have completed and all MOSI bytes
      // are already in the mailbox.
      expected_cs_n_count += num_writes + 1;

      // Verify write MOSI bytes.
      for (int i = 0; i < num_writes; i++) begin
        exp_addr = wr_addr + 16'(i);
        exp_data = wr_data + 8'(i);
        mosi_mailbox.get(actual);
        `ASSERT_ERROR(actual === exp_addr[15:8],
          $sformatf("N=%0d write[%0d] addr_hi: got 0x%h, expected 0x%h",
                    num_writes, i, actual, exp_addr[15:8]));
        mosi_mailbox.get(actual);
        `ASSERT_ERROR(actual === exp_addr[7:0],
          $sformatf("N=%0d write[%0d] addr_lo: got 0x%h, expected 0x%h",
                    num_writes, i, actual, exp_addr[7:0]));
        mosi_mailbox.get(actual);
        `ASSERT_ERROR(actual === exp_data,
          $sformatf("N=%0d write[%0d] data: got 0x%h, expected 0x%h",
                    num_writes, i, actual, exp_data));
      end
      // Drain 3 MOSI bytes for the read transaction.
      repeat (3) mosi_mailbox.get(dummy);

      // MISO counter resets per CS_N; read bytes are always 0x00, 0x01, 0x02.
      `ASSERT_ERROR(rd_word[7:0]   === 8'h00,
        $sformatf("N=%0d read byte0: got 0x%h, expected 0x00",
                  num_writes, rd_word[7:0]));
      `ASSERT_ERROR(rd_word[15:8]  === 8'h01,
        $sformatf("N=%0d read byte1: got 0x%h, expected 0x01",
                  num_writes, rd_word[15:8]));
      `ASSERT_ERROR(rd_word[23:16] === 8'h02,
        $sformatf("N=%0d read byte2: got 0x%h, expected 0x02",
                  num_writes, rd_word[23:16]));
    end
    test.end_test();

  endtask : test_si_n_writes_then_read


  //---------------------------------------------------------------------------
  // STREAMING -> SI -> STREAMING back-to-back sequence
  //---------------------------------------------------------------------------
  //
  // Models the hardware failure scenario: three transactions issued with no
  // inter-transaction idle:
  //
  //   1. STREAMING write (3 data bytes)
  //   2. SI write (1 triplet: addr_hi, addr_lo, data)
  //   3. STREAMING write (3 data bytes)
  //
  // Each transaction is issued as soon as the previous one's last CtrlPort
  // write is ACKed (backpressure only); the FPGA may still be clocking out
  // the prior transaction's SPI bytes when the next request arrives.
  //
  // All MOSI bytes for all three transactions are verified in order after all
  // three have been poked.
  //
  // Run NUM_ITERS times with freshly randomized addresses and data each time.
  //
  //---------------------------------------------------------------------------

  task automatic test_streaming_then_si();
    localparam int NUM_ITERS = 16;

    logic [7:0]  stream1_data[3];
    logic [15:0] stream1_addr;
    logic [7:0]  stream2_data[3];
    logic [15:0] stream2_addr;
    logic [15:0] si_addr;
    logic [7:0]  si_data_byte;
    logic [7:0]  actual;

    test.start_test($sformatf("STREAMING->SI->STREAMING back-to-back (%0d iters)", NUM_ITERS));
    repeat (NUM_ITERS) begin
      // Randomise all addresses and data.
      stream1_addr  = $urandom();
      stream2_addr  = $urandom();
      si_addr       = $urandom() & 16'h7FFF; // bit 15=0 -> write
      si_data_byte  = $urandom();
      foreach (stream1_data[i]) stream1_data[i] = $urandom();
      foreach (stream2_data[i]) stream2_data[i] = $urandom();

      //---------------------------------------------------------------------
      // Transaction 1: STREAMING write (3 bytes). Returns as soon as the last
      // REG_DATA word is ACKed; SPI may still be clocking final bytes when
      // transaction 2 is posted.
      //---------------------------------------------------------------------
      spi_write_streaming(stream1_addr, stream1_data);

      //---------------------------------------------------------------------
      // Transaction 2: REG_SI write issued back-to-back after transaction 1.
      // Stalls on si_pending backpressure until the STREAMING FSMs go idle,
      // then fires.
      //---------------------------------------------------------------------
      spi_si_write(si_addr, si_data_byte);

      //---------------------------------------------------------------------
      // Transaction 3: STREAMING write issued back-to-back after transaction
      // 2. Stalls on REG_CONTROL backpressure (both FSMs must be idle before a
      // new REG_CONTROL write is ACKed).
      //---------------------------------------------------------------------
      spi_write_streaming(stream2_addr, stream2_data);

      //---------------------------------------------------------------------
      // All three transactions poked. Drain and verify every MOSI byte in wire
      // order. mosi_mailbox.get() blocks until each byte is clocked out.
      //---------------------------------------------------------------------

      // Transaction 1: addr_hi, addr_lo, data[0..2].
      mosi_mailbox.get(actual);
      `ASSERT_ERROR(actual === stream1_addr[15:8],
        $sformatf("STREAM1 addr_hi: got 0x%h, expected 0x%h",
                  actual, stream1_addr[15:8]));
      mosi_mailbox.get(actual);
      `ASSERT_ERROR(actual === stream1_addr[7:0],
        $sformatf("STREAM1 addr_lo: got 0x%h, expected 0x%h",
                  actual, stream1_addr[7:0]));
      foreach (stream1_data[i]) begin
        mosi_mailbox.get(actual);
        `ASSERT_ERROR(actual === stream1_data[i],
          $sformatf("STREAM1 data[%0d]: got 0x%h, expected 0x%h",
                    i, actual, stream1_data[i]));
      end

      // Transaction 2: addr_hi, addr_lo, data_byte.
      mosi_mailbox.get(actual);
      `ASSERT_ERROR(actual === si_addr[15:8],
        $sformatf("SI addr_hi: got 0x%h, expected 0x%h",
                  actual, si_addr[15:8]));
      mosi_mailbox.get(actual);
      `ASSERT_ERROR(actual === si_addr[7:0],
        $sformatf("SI addr_lo: got 0x%h, expected 0x%h",
                  actual, si_addr[7:0]));
      mosi_mailbox.get(actual);
      `ASSERT_ERROR(actual === si_data_byte,
        $sformatf("SI data: got 0x%h, expected 0x%h",
                  actual, si_data_byte));

      // Transaction 3: addr_hi, addr_lo, data[0..2].
      mosi_mailbox.get(actual);
      `ASSERT_ERROR(actual === stream2_addr[15:8],
        $sformatf("STREAM2 addr_hi: got 0x%h, expected 0x%h",
                  actual, stream2_addr[15:8]));
      mosi_mailbox.get(actual);
      `ASSERT_ERROR(actual === stream2_addr[7:0],
        $sformatf("STREAM2 addr_lo: got 0x%h, expected 0x%h",
                  actual, stream2_addr[7:0]));
      foreach (stream2_data[i]) begin
        mosi_mailbox.get(actual);
        `ASSERT_ERROR(actual === stream2_data[i],
          $sformatf("STREAM2 data[%0d]: got 0x%h, expected 0x%h",
                    i, actual, stream2_data[i]));
      end

      // All three transactions complete. Update the timing-monitor counter.
      expected_cs_n_count += 3;
    end
    test.end_test();

  endtask : test_streaming_then_si


  //---------------------------------------------------------------------------
  //
  // Verifies that writing REG_HALF_PER at runtime changes the SCLK frequency
  // correctly.
  //
  // Runs two RAW write transactions, one at the default HALF_PER setting and
  // one at HALF_PER+/-1 (one step away). The SPI timing monitor validates the
  // SCLK frequency for every transaction automatically, so no extra edge-level
  // checks are needed here.
  //
  // When HALF_PER=0 (maximum clock rate), NEW_HALF_PER=1 (one step slower)
  // is used instead of HALF_PER-1 to keep the value in range.
  //
  // Restores the half-period to HALF_PER at the end so subsequent tests see
  // the same timing as the rest of the suite.
  //
  // Skipped when HALF_PER_EN=0 because the half-period register is not
  // writable in that configuration.
  //
  //---------------------------------------------------------------------------

  task automatic test_sclk_freq();
    localparam int NEW_HALF_PER = HALF_PER > 0 ? HALF_PER - 1 : HALF_PER + 1;
    logic [7:0]    tx_data[]    = '{8'hA5, 8'h3C, 8'hF0};
    logic [7:0]    rx_data[];
    logic [31:0]   rd_val;

    test.start_test("Half-period register");
    //-------------------------------------------------------------------------
    // Verify the register starts at the default HALF_PER value
    //-------------------------------------------------------------------------
    cp_bfm.read(BASE_ADDR + REG_HALF_PER, rd_val);
    `ASSERT_ERROR(rd_val[HALF_PER_W-1:0] == HALF_PER,
      $sformatf("REG_HALF_PER initial value: got 0x%h, expected 0x%h",
                rd_val[HALF_PER_W-1:0], HALF_PER));

    //-------------------------------------------------------------------------
    // Write 3 bytes at the default HALF_PER setting
    //-------------------------------------------------------------------------
    run_test(TB_MODE_RAW, SPI_WRITE, 3, '0, tx_data, rx_data);

    //-------------------------------------------------------------------------
    // Change to HALF_PER-1, verify readback, then update the monitor's
    // expected period
    //-------------------------------------------------------------------------
    wait (spi_cs_n); // Make sure SPI is idle before changing rate
    cp_bfm.write(BASE_ADDR + REG_HALF_PER, NEW_HALF_PER);
    cp_bfm.read(BASE_ADDR + REG_HALF_PER, rd_val);
    `ASSERT_ERROR(rd_val[HALF_PER_W-1:0] == NEW_HALF_PER,
      $sformatf("REG_HALF_PER after write: got 0x%h, expected 0x%h",
                rd_val[HALF_PER_W-1:0], NEW_HALF_PER));
    half_per_time = (NEW_HALF_PER + 1) * CLK_PERIOD_NS * 1.0ns;

    //-------------------------------------------------------------------------
    // Write 3 bytes at the new HALF_PER setting
    //-------------------------------------------------------------------------
    run_test(TB_MODE_RAW, SPI_WRITE, 3, '0, tx_data, rx_data);

    //-------------------------------------------------------------------------
    // Restore original HALF_PER setting and verify readback
    //-------------------------------------------------------------------------
    wait (spi_cs_n); // Make sure SPI is idle before changing rate
    cp_bfm.write(BASE_ADDR + REG_HALF_PER, HALF_PER);
    cp_bfm.read(BASE_ADDR + REG_HALF_PER, rd_val);
    `ASSERT_ERROR(rd_val[HALF_PER_W-1:0] == HALF_PER,
      $sformatf("REG_HALF_PER after restore: got 0x%h, expected 0x%h",
                rd_val[HALF_PER_W-1:0], HALF_PER));
    half_per_time = (HALF_PER + 1) * CLK_PERIOD_NS * 1.0ns;
    test.end_test();

  endtask : test_sclk_freq


  //---------------------------------------------------------------------------
  // SPI Timing Monitor
  //---------------------------------------------------------------------------
  //
  // Runs in parallel with the main test. On every transaction (CS_N
  // assertion), checks:
  //
  //   1. CS_N assert -> first SCLK rise is exactly 1 half-period
  //   2. SCLK high period (posedge -> negedge) is exactly 1 half-period
  //   3. SCLK low period (negedge -> posedge):
  //      - intra-byte (fall N where N % 8 != 0): exactly 1 half-period
  //      - inter-byte (fall N where N % 8 == 0): >= 1 half-period
  //   4. Last SCLK fall -> CS_N deassert:
  //      - CS_HOLD=1 : exactly 1 half-period
  //      - CS_HOLD=0 : 0 (same registered clock edge)
  //   5. CS_N deassert -> next CS_N assert : >= CS_GUARD half-periods
  //
  // CS_HOLD and CS_GUARD are read hierarchically from dut.axis_spi_i.
  //---------------------------------------------------------------------------

  initial begin : spi_timing_monitor
    realtime t_cs_assert, t_rise, t_fall, t_cs_deassert, t_cs_deassert_prev;
    int      rise_cnt;  // rising edge count within the current transaction
    static bit first_txn = 1'b1;  // suppress guard check on first transaction

    @(negedge rst); // wait for reset to deassert

    forever begin

      //-----------------------------------------------------------------------
      // Wait for CS_N assertion (start of transaction)
      //-----------------------------------------------------------------------
      @(negedge spi_cs_n);
      t_cs_assert = $realtime;
      cs_n_monitor_count++;

      //-----------------------------------------------------------------------
      // Check 5: CS_N guard: previous transaction's CS_N deassert must be >=
      // CS_GUARD half-periods before this assertion. Checked here (at the
      // start of each transaction) so the guard @(negedge spi_cs_n) doesn't
      // consume a transaction count. Skipped on the first transaction.
      //-----------------------------------------------------------------------
      if (!first_txn) begin
        `ASSERT_ERROR(t_cs_assert - t_cs_deassert_prev >=
                        dut.axis_spi_i.CS_GUARD * half_per_time,
          $sformatf("CS_N guard too short: minimum %0t, got %0t",
                    dut.axis_spi_i.CS_GUARD * half_per_time,
                    t_cs_assert - t_cs_deassert_prev));
      end
      first_txn = 1'b0;

      //-----------------------------------------------------------------------
      // Check 1: CS_N assert -> first SCLK rising edge = exactly 1 half-period
      //-----------------------------------------------------------------------
      @(posedge spi_sclk);
      t_rise    = $realtime;
      rise_cnt  = 1;
      `ASSERT_ERROR(t_rise - t_cs_assert == half_per_time,
        $sformatf("CS_N assert to first SCLK rise: expected %0t, got %0t",
                  half_per_time, t_rise - t_cs_assert));

      //-----------------------------------------------------------------------
      // Checks 2, 3, 4: Walk all SCLK edges until CS_N deasserts.
      //
      // State at loop top: we are at a posedge sclk; rise_cnt is current.
      //
      //   Step A: Wait for negedge sclk OR posedge cs_n (CS_HOLD=0 causes
      //           both to arrive on the same clock edge).
      //   Step B: If cs_n is still low, wait for posedge sclk OR posedge
      //           cs_n (CS_HOLD=1 deasserts cs_n here after ST_HOLD).
      //-----------------------------------------------------------------------
      forever begin

        // Step A: wait for SCLK fall (or simultaneous CS_N deassert)
        fork : wait_fall
          begin : fall_th  @(negedge spi_sclk); end
          begin : cs0_th   @(posedge spi_cs_n);  end
        join_any
        disable wait_fall;
        t_fall = $realtime;

        // Check 2: SCLK high period must be exactly 1 half-period
        `ASSERT_ERROR(t_fall - t_rise == half_per_time,
          $sformatf("SCLK high period: expected %0t, got %0t",
                    half_per_time, t_fall - t_rise));

        if (spi_cs_n) begin
          // CS_HOLD=0: CS_N deasserted on the same clock edge as last SCLK
          // fall, so time difference is 0.
          t_cs_deassert = t_fall;
          `ASSERT_ERROR(dut.axis_spi_i.CS_HOLD == 0,
            "CS_N deasserted coincident with last SCLK fall but CS_HOLD != 0");
          break;
        end

        // Step B: wait for SCLK rise or CS_N deassert (CS_HOLD=1)
        fork : wait_rise
          begin : rise_th  @(posedge spi_sclk); end
          begin : cs1_th   @(posedge spi_cs_n);  end
        join_any
        disable wait_rise;
        t_rise = $realtime;
        rise_cnt++;

        // Check 3: SCLK low period.
        //
        // rise_cnt just incremented, so the preceding fall was fall number
        // (rise_cnt - 1). A fall at a multiple of 8 is an inter-byte boundary
        // where the FSM waits in ST_IDLE for the next byte; duration is
        // unbounded but must be >= 1 half-period. All other falls are
        // intra-byte and must be exactly 1 half-period.
        if ((rise_cnt - 1) % 8 == 0) begin
          `ASSERT_ERROR(t_rise - t_fall >= half_per_time,
            $sformatf("SCLK low period (inter-byte) too short: minimum %0t, got %0t",
                      half_per_time, t_rise - t_fall));
        end else begin
          `ASSERT_ERROR(t_rise - t_fall == half_per_time,
            $sformatf("SCLK low period (intra-byte, fall %0d): expected %0t, got %0t",
                      rise_cnt - 1, half_per_time, t_rise - t_fall));
        end

        if (spi_cs_n) begin
          // CS_HOLD=1: CS_N deasserted exactly 1 half-period after last
          // SCLK fall (after ST_HOLD).
          t_cs_deassert = t_rise;
          `ASSERT_ERROR(dut.axis_spi_i.CS_HOLD == 1,
            "CS_N deasserted 1 half-period after SCLK fall but CS_HOLD != 1");
          `ASSERT_ERROR(t_cs_deassert - t_fall == half_per_time,
            $sformatf("CS_HOLD=1 hold time: expected %0t, got %0t",
                      half_per_time, t_cs_deassert - t_fall));
          break;
        end
        // cs_n still low and we got posedge sclk; continue to next bit

      end // inner forever (SCLK edge walk)

      // Save deassert time for next iteration's guard check
      t_cs_deassert_prev = t_cs_deassert;

    end // outer forever (transaction loop)

  end : spi_timing_monitor



  //---------------------------------------------------------------------------
  // Main test
  //---------------------------------------------------------------------------

  initial begin : main
    test.start_tb(
      $sformatf({"ctrlport_spi_adrv_tb\n",
        "QUICK_TEST=%0d\n",
        "NUM_BYTES_W=%0d\n",
        "HALF_PER=%0d\n",
        "HALF_PER_EN=%0d\n",
        "CS_HOLD=%0d\n",
        "CS_GUARD=%0d"},
        QUICK_TEST, NUM_BYTES_W, HALF_PER, HALF_PER_EN, CS_HOLD, CS_GUARD),
      500ms
    );

    // Don't start the clock until after start_tb() returns. This ensures that
    // only one instance of this testbench is running its clock at a time.
    clk_gen.start();

    // Start the CtrlPort BFM
    cp_bfm.run();

    // Assert reset for 8 clock cycles then release
    clk_gen.reset();
    @(negedge rst);

    //-------------------------------------------------------------------------
    // Tests
    //-------------------------------------------------------------------------

    test_raw_smoke();
    test_si_seq_smoke();
    test_streaming_smoke();
    test_si_reg_smoke();
    test_max_and_min();
    test_si_addr_behavior();
    test_si();
    test_si_burst();
    test_si_n_writes_then_read();
    test_streaming_then_si();
    if (HALF_PER_EN) test_sclk_freq();
    run_rand_tests(NUM_RAND_TESTS);

    // Confirm the timing monitor ran exactly once per SPI transaction.
    // expected_cs_n_count is incremented once per issued transaction, so it
    // tracks the true number of transactions without manual bookkeeping.
    `ASSERT_ERROR(cs_n_monitor_count == expected_cs_n_count,
      $sformatf("Timing monitor CS_N count mismatch: expected %0d, got %0d",
                expected_cs_n_count, cs_n_monitor_count));

    // Don't call $finish; other instances of this testbench may follow.
    test.end_tb(0);

    // Kill the clock so this instance generates no further simulation events.
    clk_gen.kill();
  end

endmodule : ctrlport_spi_adrv_tb

`default_nettype wire


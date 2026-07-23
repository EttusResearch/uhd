//
// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: ctrlport_spi_adrv
//
// Description:
//
//   SPI master for the ADI ADRV transceiver family. It is controlled via a
//   CtrlPort slave interface and it drives a standard 4-wire SPI bus (CPOL=0,
//   CPHA=0). It is designed to make efficient use of the CtrlPort bus by
//   eliminating the need to write redundant address bytes (e.g., repeating the
//   address for writes to the same address, or writes to sequential
//   addresses), the need to write dummy bytes in order to flush out a read
//   result, or the need to read back echo bytes in responses. Only the
//   relevant bytes need to be communicated over CtrlPort.
//
//   Furthermore, the SPI transactions are blocking in the sense that the
//   CtrlPort interface will not ACK until the request is accepted and
//   guaranteed to complete. This allows multiple SPI transactions to be
//   enqueued in the underlying CtrlPort endpoint FIFO without the need for
//   polling the core for status.
//
//   Two methods are available for issuing SPI transactions: REG_CONTROl and
//   REG_SI.
//
//   REG_CONTROL Interface
//
//     With this interface, the REG_CONTROL register controls the mode,
//     direction, and the number of data bytes in the transaction. Four
//     transaction modes are supported via the REG_CONTROL register.
//
//       RAW       : The exact byte stream in the data register is shifted out.
//                   The caller is responsible for embedding any SPI address.
//
//       SI_SEQ    : Single instruction, sequential. Each data byte is preceded
//                   by the 16-bit SPI address written in REG_CONTROL, which
//                   auto-increments by one for every byte. Three SPI bytes are
//                   sent per data byte.
//
//       SI_REP    : Single instruction, repeated. Same as SI_SEQ but the
//                   address does not increment. The same address is sent
//                   before every data byte.
//
//       STREAMING : The 16-bit SPI address is sent once, followed by all data
//                   bytes back-to-back. Two address bytes plus num_bytes data
//                   bytes are sent total.
//
//
//     For write operations in all the above modes, the RX state machine
//     ignores the incoming bytes from the SPI interface without storing them.
//     So, it is not necessary to read anything when doing a write.
//
//     For read operations in RAW mode, all MISO bytes are returned such that
//     you must read the same number of bytes that were written.
//
//     For read operations in SI_SEQ, SI_REP, and STREAMING mode, the state
//     machine automatically drives MOSI with zeros during the data phase, so
//     that it is not necessary to write any data bytes to execute a read. And
//     it automatically captures only the data bytes from MISO; the address
//     echo bytes are discarded. So, you only need to read the number of data
//     bytes that were requested to be written (excluding the address bytes).
//
//   REG_SI Interface
//
//     For a single-byte read or write (referred to as "single instruction" in
//     the ADI code), there is also a REG_SI register to which a single 3-byte
//     instruction can be written (addr_hi, addr_lo, and data byte). This is
//     equivalent to using RAW mode with a length of 3 bytes. If the MSB of the
//     address (bit 15) is set, then it is treated as a read and the 3 RX data
//     bytes are inserted into the RX FIFO and can be read by reading the
//     REG_DATA register.
//
//     The benefit of using REG_SI is that you don't need to also write to the
//     REG_CONTROL register. So, fewer CtrlPort writes are needed for three
//     instructions or less compared to RAW mode. On the downside, each
//     instruction is its own SPI transaction (i.e., CS_n deasserts between
//     each instruction). So, for 2 or more instructions, additional time may
//     be used on the SPI bus.
//
//   RX Data FIFO
//
//     Received bytes are packed into 32-bit words (first byte in bits [7:0],
//     second byte in bits [15:8], etc.) and stored in the RX word FIFO. Each
//     transaction ends on a word boundary; if the number of received bytes is
//     not a multiple of 4, the remaining bytes in the last word are undefined.
//     This means that the first byte of a transaction always starts with bits
//     [7:0] of a new FIFO word, regardless of how many bytes the previous
//     transaction produced.
//
//     Each REG_SI read request always produces exactly 3 received bytes, which
//     are padded to a full 32-bit word (one FIFO entry). So each REG_SI read
//     fills one word of the FIFO, but only bytes [23:0] are valid.
//
//     The host software must not overfill the FIFO. The default depth holds
//     one complete maximum-size transaction.
//
//   Data Word Byte Ordering
//
//     Words are little-endian packed. That is, byte 0 (the first byte to be
//     sent over SPI) occupies data[7:0], byte 1 occupies data[15:8], etc. Each
//     byte is serialized MSB-first. That is, bit 7 leaves the MOSI/MISO pin
//     first, bit 0 last.
//
//   CS_N Behavior
//
//     Asserts when REG_CONTROL is written and deasserts automatically after
//     all num_bytes have been shifted and captured. CS_N remains asserted until
//     the entire transaction has completed; the SPI clock simply pauses
//     between data bytes when the TX buffer is empty.
//
//     For REG_SI, CS_N asserts/deasserts for each single instruction.
//
//   CtrlPort Flow Control
//
//     REG_CONTROL write : ACKs after the control word has been accepted.
//     REG_DATA write    : ACKs after the write has been accepted and is
//                         guaranteed to complete.
//     REG_DATA read     : ACK'd when the data to be read becomes available.
//
//   SPI Clock Rate
//
//     The SPI clock rate is controlled by setting the desired number of clock
//     cycles per half SPI clock period, minus 1. In other words, the actual
//     SPI clock frequency for a given clock frequency will be:
//
//       f_sclk = f_clk / (2 * (half_per + 1))
//
//     Or, the required half period setting for a given SPI clock frequency can
//     be calculated as:
//
//       half_per = (f_clk / (2 * f_sclk)) - 1
//
// Parameters:
//
//   BASE_ADDR    : CtrlPort base address.
//   NUM_BYTES_W  : Log base 2 of maximum number data bytes supported per
//                  transaction. Must not exceed SPI_NUM_BYTES_W.
//   HALF_PER     : Default sclk half-period length in clk cycles
//   HALF_PER_W   : Width of the half-period register in bits.
//   HALF_PER_EN  : When 1, the half-period register (REG_HALF_PER) exists and
//                  is software-readable and writable. When 0, the register
//                  does not exist; the SCLK rate is fixed at the HALF_PER
//                  parameter value.
//   CS_HOLD      : 1 = hold CS_N low for one extra half-period after the last
//                  SCLK falling edge. 0 = deassert CS_N immediately on the
//                  last SCLK falling edge.
//   CS_GUARD     : Number of CS_N-high half-periods between transactions. Must
//                  be 1 or 2.
//   RX_FIFO_SIZE : Log base 2 of the depth of the 32-bit wide RX word FIFO. By
//                  default, it holds one complete maximum-size transaction
//                  (2**NUM_BYTES_W bytes / 4 bytes per word). The host
//                  software must NOT overfill the FIFO.
//
// Signals:
//
//   clk / rst    : Clock and active-high synchronous reset.
//   s_ctrlport_* : CtrlPort slave request/response signals.
//   sclk         : SPI clock output (registered).
//   cs_n         : Active-low chip select (registered).
//   mosi         : Master-out / slave-in data (registered).
//   miso         : Master-in / slave-out data input.
//

`default_nettype none


module ctrlport_spi_adrv
  import ctrlport_pkg::*;
  import ctrlport_spi_adrv_pkg::*;
#(
  int BASE_ADDR    = 0,
  int NUM_BYTES_W  = 8,
  int HALF_PER     = 9,
  int HALF_PER_W   = 8,
  bit HALF_PER_EN  = 1,
  bit CS_HOLD      = 1,
  int CS_GUARD     = 2,
  int RX_FIFO_SIZE = NUM_BYTES_W-2
) (
  input wire clk,
  input wire rst,

  // CtrlPort Slave Request
  input  wire                        s_ctrlport_req_wr,
  input  wire                        s_ctrlport_req_rd,
  input  wire [CTRLPORT_ADDR_W-1:0]  s_ctrlport_req_addr,
  input  wire [CTRLPORT_DATA_W-1:0]  s_ctrlport_req_data,

  // CtrlPort Slave Response
  output logic                       s_ctrlport_resp_ack,
  output ctrlport_status_t           s_ctrlport_resp_status,
  output logic [CTRLPORT_DATA_W-1:0] s_ctrlport_resp_data,

  // SPI Interface
  output logic sclk,
  output logic cs_n,
  output logic mosi,
  input  wire  miso
);

  //---------------------------------------------------------------------------
  // Parameter Checks
  //---------------------------------------------------------------------------

  if (NUM_BYTES_W > SPI_NUM_BYTES_W) begin : gen_error
    $error("NUM_BYTES_W (%0d) must not exceed SPI_NUM_BYTES_W (%0d).",
           NUM_BYTES_W, SPI_NUM_BYTES_W);
  end : gen_error


  //---------------------------------------------------------------------------
  // Declarations
  //---------------------------------------------------------------------------

  // SPI clock half-period. Driven by a software-writable register when
  // HALF_PER_EN=1; wired to the parameter value when HALF_PER_EN=0.
  logic [HALF_PER_W-1:0] half_per;

  // TX 32-bit AXI-Stream (from register block into TX width converter)
  logic [31:0] tx_word_tdata;
  logic [ 3:0] tx_word_tkeep;
  logic        tx_word_tvalid;
  logic        tx_word_tready;
  logic        tx_word_tlast;

  // TX 8-bit AXI-Stream (from TX width converter into axis_spi)
  logic  [7:0] tx_byte_tdata;
  logic        tx_byte_tvalid;
  logic        tx_byte_tready;
  logic        tx_byte_tlast;

  // RX 8-bit AXI-Stream (from axis_spi into RX FSM)
  logic  [7:0] spi_rx_tdata;
  logic        spi_rx_tvalid;
  logic        spi_rx_tready;
  logic        spi_rx_tlast;

  // RX 8-bit AXI-Stream (from RX FSM into RX width converter)
  logic  [7:0] rx_byte_tdata;
  logic        rx_byte_tvalid;
  logic        rx_byte_tready;
  logic        rx_byte_tlast;

  // RX 32-bit AXI-Stream (from RX width converter into RX FIFO)
  logic [31:0] rx_word_tdata;
  logic        rx_word_tvalid;
  logic        rx_word_tready;
  logic        rx_word_tlast;

  // RX FIFO output
  logic [31:0] rx_fifo_tdata;
  logic        rx_fifo_tvalid;
  logic        rx_fifo_tready;

  //---------------------------------------------------------------------------
  // Address Decode
  //---------------------------------------------------------------------------

  logic address_in_range;
  logic is_si_reg;
  logic is_ctrl_reg;
  logic is_data_win;

  assign address_in_range =
    (s_ctrlport_req_addr[CTRLPORT_ADDR_W-1:REG_ADDR_W] ==
     BASE_ADDR[CTRLPORT_ADDR_W-1:REG_ADDR_W]);

  assign is_si_reg   = address_in_range &&
    (s_ctrlport_req_addr[0+:REG_ADDR_W] == REG_SI);
  assign is_ctrl_reg = address_in_range &&
    (s_ctrlport_req_addr[0+:REG_ADDR_W] == REG_CONTROL);
  assign is_data_win = address_in_range &&
    (s_ctrlport_req_addr[0+:REG_ADDR_W] >= REG_DATA) &&
    (s_ctrlport_req_addr[0+:REG_ADDR_W] <  REG_DATA + 64);


  //---------------------------------------------------------------------------
  // Half-Period Register
  //---------------------------------------------------------------------------
  //
  // When HALF_PER_EN=1 the register exists and is software-readable and
  // writable. When HALF_PER_EN=0 none of this logic exists and half_per is a
  // constant tied to the HALF_PER parameter.
  //---------------------------------------------------------------------------

  logic half_per_ack;

  if (HALF_PER_EN) begin : gen_half_per_reg
    logic is_half_per_reg;
    assign is_half_per_reg = address_in_range &&
      (s_ctrlport_req_addr[0+:REG_ADDR_W] == REG_HALF_PER);

    always_ff @(posedge clk) begin
      if (rst) begin
        half_per     <= HALF_PER;
        half_per_ack <= 1'b0;
      end else begin
        half_per_ack <= 1'b0;

        if (is_half_per_reg) begin
          if (s_ctrlport_req_wr) begin
            half_per     <= s_ctrlport_req_data[HALF_PER_W-1:0];
            half_per_ack <= 1'b1;
          end else if (s_ctrlport_req_rd) begin
            half_per_ack <= 1'b1;
          end
        end
      end
    end
  end else begin : gen_half_per_fixed
    assign half_per     = HALF_PER;
    assign half_per_ack = 1'b0;
  end : gen_half_per_fixed


  //---------------------------------------------------------------------------
  // Control Register Write Handling
  //---------------------------------------------------------------------------

  // For writes to REG_SI, we must also write a control word equivalent to a
  // 3-byte RAW control word, with read or write set based on the MSB of the
  // address.
  spi_adrv_ctrl_t si_ctrl_word;
  assign si_ctrl_word = '{
    dir:       s_ctrlport_req_data[7] ? SPI_READ : SPI_WRITE,
    rsvd1:     'X,
    mode:      MODE_RAW,
    num_bytes: 3,
    addr:      'X
  };

  logic [CTRLPORT_DATA_W-1:0] ctrl_data;
  logic                       ctrl_wr;
  logic                       ctrl_ack;
  logic [CTRLPORT_DATA_W-1:0] ctrl_tdata;
  logic                       ctrl_tvalid;
  logic                       ctrl_tready;

  // Control word split to TX and RX FSMs.
  spi_adrv_ctrl_t ctrl_tx_tdata;
  logic           ctrl_tx_tvalid;
  logic           ctrl_tx_tready;
  spi_adrv_ctrl_t ctrl_rx_tdata;
  logic           ctrl_rx_tvalid;
  logic           ctrl_rx_tready;

  // REG_CONTROL writes pass the data word through. REG_SI writes use the
  // synthesized control word above.
  assign ctrl_data = is_si_reg ? si_ctrl_word : s_ctrlport_req_data;
  assign ctrl_wr   = s_ctrlport_req_wr && (is_ctrl_reg || is_si_reg);

  ctrlport_to_axis #(
    .WIDTH          (CTRLPORT_DATA_W),
    .FIFO_SIZE_LOG2 (1)
  ) ctrlport_to_axis_ctrl (
    .clk           (clk),
    .rst           (rst),
    .ctrl_data     (ctrl_data),
    .ctrl_wr       (ctrl_wr),
    .ctrl_ack      (ctrl_ack),
    .m_axis_tdata  (ctrl_tdata),
    .m_axis_tvalid (ctrl_tvalid),
    .m_axis_tready (ctrl_tready)
  );

  // Duplicate the control word to the TX FSM (port 0) and RX FSM (port 1).
  axis_split #(
    .DATA_W        (CTRLPORT_DATA_W),
    .NUM_PORTS     (2),
    .INPUT_REG     (0),
    .OUT_FIFO_SIZE (1)
  ) ctrl_split_i (
    .clk           (clk),
    .rst           (rst),
    .s_axis_tdata  (ctrl_tdata),
    .s_axis_tvalid (ctrl_tvalid),
    .s_axis_tready (ctrl_tready),
    .m_axis_tdata  ({ctrl_rx_tdata,  ctrl_tx_tdata}),
    .m_axis_tvalid ({ctrl_rx_tvalid, ctrl_tx_tvalid}),
    .m_axis_tready ({ctrl_rx_tready, ctrl_tx_tready})
  );


  //---------------------------------------------------------------------------
  // Data Register Write Handling
  //---------------------------------------------------------------------------

  logic data_wr;
  logic data_ack;

  assign data_wr  = s_ctrlport_req_wr && (is_data_win || is_si_reg);

  ctrlport_to_axis #(
    .WIDTH          (CTRLPORT_DATA_W),
    .FIFO_SIZE_LOG2 (0)
  ) ctrlport_to_axis_data (
    .clk           (clk),
    .rst           (rst),
    .ctrl_data     (s_ctrlport_req_data),
    .ctrl_wr       (data_wr),
    .ctrl_ack      (data_ack),
    .m_axis_tdata  (tx_word_tdata),
    .m_axis_tvalid (tx_word_tvalid),
    .m_axis_tready (tx_word_tready)
  );


  //---------------------------------------------------------------------------
  // Data Register Read Handling
  //---------------------------------------------------------------------------

  logic [CTRLPORT_DATA_W-1:0] data_a2c_data;
  logic                       data_a2c_ack;
  logic                       data_a2c_rd;

  assign data_a2c_rd = s_ctrlport_req_rd && is_data_win;

  axis_to_ctrlport #(
    .WIDTH          (CTRLPORT_DATA_W),
    .FIFO_SIZE_LOG2 (0)
  ) axis_to_ctrlport_rx (
    .clk           (clk),
    .rst           (rst),
    .s_axis_tdata  (rx_fifo_tdata),
    .s_axis_tvalid (rx_fifo_tvalid),
    .s_axis_tready (rx_fifo_tready),
    .ctrl_rd       (data_a2c_rd),
    .ctrl_data     (data_a2c_data),
    .ctrl_ack      (data_a2c_ack)
  );


  //---------------------------------------------------------------------------
  // CtrlPort Response Acknowledgement
  //---------------------------------------------------------------------------
  //
  // A REG_SI write triggers both ctrl_wr and data_wr simultaneously, so
  // ctrl_ack and data_ack fire independently at different times. This state
  // machine ensures exactly one s_ctrlport_resp_ack pulse is issued per
  // request, once all expected ACKs have arrived.
  //
  //   ST_ACK_IDLE     : Waiting for a write request.
  //   ST_ACK_WAIT_ONE : Expecting one more ACK (REG_CONTROL or REG_DATA write,
  //                     or REG_SI write after one of the two ACKs has arrived).
  //   ST_ACK_WAIT_TWO : Expecting two ACKs (REG_SI write, neither arrived yet);
  //                     if both arrive on the same cycle, coalesce immediately.
  //
  // data_a2c_ack (axis-to-ctrlport for REG_DATA reads) is always single-source
  // and bypasses the state machine, ORing directly into s_ctrlport_resp_ack.
  //
  //---------------------------------------------------------------------------

  typedef enum logic [1:0] {
    ST_ACK_IDLE     = 0,
    ST_ACK_WAIT_ONE = 1,
    ST_ACK_WAIT_TWO = 2
  } ack_state_t;

  ack_state_t ack_state;
  logic       ack_out;

  always_ff @(posedge clk) begin
    if (rst) begin
      ack_state <= ST_ACK_IDLE;
      ack_out   <= 1'b0;
    end else begin
      ack_out <= 1'b0;

      case (ack_state)
        ST_ACK_IDLE : begin
          if (s_ctrlport_req_wr) begin
            if (is_si_reg)
              ack_state <= ST_ACK_WAIT_TWO;
            else
              ack_state <= ST_ACK_WAIT_ONE;
          end
        end

        ST_ACK_WAIT_ONE : begin
          if (ctrl_ack || data_ack) begin
            ack_out   <= 1'b1;
            ack_state <= ST_ACK_IDLE;
          end
        end

        ST_ACK_WAIT_TWO : begin
          if (ctrl_ack && data_ack) begin
            // Both arrived on the same cycle.
            ack_out   <= 1'b1;
            ack_state <= ST_ACK_IDLE;
          end else if (ctrl_ack || data_ack) begin
            // One arrived; wait for the other.
            ack_state <= ST_ACK_WAIT_ONE;
          end
        end

        default : ack_state <= ST_ACK_IDLE;
      endcase
    end
  end

  always_ff @(posedge clk) begin
    if (rst) begin
      s_ctrlport_resp_ack  <= 1'b0;
      s_ctrlport_resp_data <= 'X;
    end else begin
      s_ctrlport_resp_ack  <= ack_out | data_a2c_ack | half_per_ack;
      s_ctrlport_resp_data <= half_per_ack ? half_per : data_a2c_data;
    end
  end
  assign s_ctrlport_resp_status = STS_OKAY;


  //---------------------------------------------------------------------------
  // TX Width Converter (32-bit -> 8-bit)
  //---------------------------------------------------------------------------

  axis_width_conv #(
    .WORD_W    (8),
    .IN_WORDS  (4),
    .OUT_WORDS (1),
    .SYNC_CLKS (1),
    .PIPELINE  ("OUT")
  ) axis_width_conv_tx (
    .s_axis_aclk   (clk),
    .s_axis_rst    (rst),
    .s_axis_tdata  (tx_word_tdata),
    .s_axis_tkeep  ('1),
    .s_axis_tlast  ('0),
    .s_axis_tvalid (tx_word_tvalid),
    .s_axis_tready (tx_word_tready),
    .m_axis_aclk   (clk),
    .m_axis_rst    (rst),
    .m_axis_tdata  (tx_byte_tdata),
    .m_axis_tkeep  (),
    .m_axis_tlast  (tx_byte_tlast),
    .m_axis_tvalid (tx_byte_tvalid),
    .m_axis_tready (tx_byte_tready)
  );


  //---------------------------------------------------------------------------
  // TX State Machine
  //---------------------------------------------------------------------------

  // AXI-Stream from TX FSM into axis_spi.
  logic [7:0] spi_tx_tdata;
  logic       spi_tx_tvalid;
  logic       spi_tx_tready;
  logic       spi_tx_tlast;

  typedef enum logic [3:0] {
    ST_TX_IDLE           = 0,
    ST_TX_RAW            = 1,
    ST_TX_SI_ADDR_HI     = 2,
    ST_TX_SI_ADDR_LO     = 3,
    ST_TX_SI_DATA        = 4,
    ST_TX_STREAM_ADDR_HI = 5,
    ST_TX_STREAM_ADDR_LO = 6,
    ST_TX_STREAM_DATA    = 7,
    ST_TX_DRAIN          = 8
  } tx_state_t;

  tx_state_t                  tx_state;
  logic [SPI_NUM_BYTES_W-1:0] tx_bytes_rem; // Data bytes remaining
  logic [               15:0] tx_cur_addr;  // Running SPI address
  logic [                1:0] tx_drain_cnt; // Padding bytes to drain after transaction

  always_ff @(posedge clk) begin
    if (rst) begin
      tx_state     <= ST_TX_IDLE;
      tx_bytes_rem <= '0;
      tx_cur_addr  <= '0;
      tx_drain_cnt <= 2'b0;
    end else begin
      case (tx_state)
        ST_TX_IDLE : begin
          if (ctrl_tx_tvalid) begin
            tx_bytes_rem <= ctrl_tx_tdata.num_bytes;
            tx_cur_addr  <= ctrl_tx_tdata.addr;
            tx_drain_cnt <= 2'b0 - ctrl_tx_tdata.num_bytes[1:0];
            case (ctrl_tx_tdata.mode)
              MODE_RAW       : tx_state <= ST_TX_RAW;
              MODE_SI_SEQ,
              MODE_SI_REP    : tx_state <= ST_TX_SI_ADDR_HI;
              MODE_STREAMING : tx_state <= ST_TX_STREAM_ADDR_HI;
              default        : tx_state <= ST_TX_RAW;
            endcase
          end
        end

        ST_TX_RAW : begin
          if (spi_tx_tvalid && spi_tx_tready) begin
            tx_bytes_rem <= tx_bytes_rem - 1;
            if (tx_bytes_rem == 1) begin
              tx_state <= (tx_drain_cnt != 0) ? ST_TX_DRAIN : ST_TX_IDLE;
            end
          end
        end

        ST_TX_SI_ADDR_HI : begin
          if (spi_tx_tvalid && spi_tx_tready) begin
            tx_state <= ST_TX_SI_ADDR_LO;
          end
        end

        ST_TX_SI_ADDR_LO : begin
          if (spi_tx_tvalid && spi_tx_tready) begin
            tx_state <= ST_TX_SI_DATA;
          end
        end

        ST_TX_SI_DATA : begin
          if (spi_tx_tvalid && spi_tx_tready) begin
            tx_bytes_rem <= tx_bytes_rem - 1;
            if (tx_bytes_rem == 1) begin
              if (ctrl_tx_tdata.dir == SPI_WRITE && tx_drain_cnt != 0) begin
                tx_state <= ST_TX_DRAIN;
              end else begin
                tx_state <= ST_TX_IDLE;
              end
            end else begin
              if (ctrl_tx_tdata.mode == MODE_SI_SEQ) begin
                tx_cur_addr <= tx_cur_addr + 1;
              end
              tx_state <= ST_TX_SI_ADDR_HI;
            end
          end
        end

        ST_TX_STREAM_ADDR_HI : begin
          if (spi_tx_tvalid && spi_tx_tready) begin
            tx_state <= ST_TX_STREAM_ADDR_LO;
          end
        end

        ST_TX_STREAM_ADDR_LO : begin
          if (spi_tx_tvalid && spi_tx_tready) begin
            tx_state <= ST_TX_STREAM_DATA;
          end
        end

        ST_TX_STREAM_DATA : begin
          if (spi_tx_tvalid && spi_tx_tready) begin
            tx_bytes_rem <= tx_bytes_rem - 1;
            if (tx_bytes_rem == 1) begin
              if (ctrl_tx_tdata.dir == SPI_WRITE && tx_drain_cnt != 0) begin
                tx_state <= ST_TX_DRAIN;
              end else begin
                tx_state <= ST_TX_IDLE;
              end
            end
          end
        end

        ST_TX_DRAIN : begin
          if (tx_byte_tvalid) begin
            tx_drain_cnt <= tx_drain_cnt - 1;
            if (tx_drain_cnt == 1) begin
              tx_state <= ST_TX_IDLE;
            end
          end
        end

        default : begin
          tx_state <= ST_TX_IDLE;
        end
      endcase
    end
  end

  // ctrl_tx_tready is driven combinationally so that the control FIFO word is
  // consumed on the exact same cycle that the last SPI byte is accepted by
  // axis_spi. This ensures the FIFO advances before the FSM re-enters
  // ST_TX_IDLE, so it sees the correct next control word.
  always_comb begin
    spi_tx_tdata   = 8'h00;
    spi_tx_tvalid  = 1'b0;
    spi_tx_tlast   = 1'b0;
    tx_byte_tready = 1'b0;
    ctrl_tx_tready = 1'b0;

    case (tx_state)
      ST_TX_IDLE : begin
        // Waiting; nothing to send.
      end

      ST_TX_RAW : begin
        // RAW: pass tx_byte bytes straight through regardless of direction.
        spi_tx_tdata   = tx_byte_tdata;
        spi_tx_tvalid  = tx_byte_tvalid;
        tx_byte_tready = spi_tx_tready;
        spi_tx_tlast   = (tx_bytes_rem == 1);
        ctrl_tx_tready = spi_tx_tvalid && spi_tx_tready && (tx_bytes_rem == 1);
      end

      ST_TX_SI_ADDR_HI : begin
        // addr[15:8] = addr_hi; first byte on the wire.
        spi_tx_tdata  = tx_cur_addr[15:8];
        spi_tx_tvalid = 1'b1;
      end

      ST_TX_SI_ADDR_LO : begin
        // addr[7:0] = addr_lo; second byte on the wire.
        spi_tx_tdata  = tx_cur_addr[7:0];
        spi_tx_tvalid = 1'b1;
      end

      ST_TX_SI_DATA : begin
        if (ctrl_tx_tdata.dir == SPI_READ) begin
          // Drive MOSI=0 for reads; do not consume TX data FIFO.
          spi_tx_tdata  = 8'h00;
          spi_tx_tvalid = 1'b1;
        end else begin
          spi_tx_tdata   = tx_byte_tdata;
          spi_tx_tvalid  = tx_byte_tvalid;
          tx_byte_tready = spi_tx_tready;
        end
        spi_tx_tlast   = (tx_bytes_rem == 1);
        ctrl_tx_tready = spi_tx_tvalid && spi_tx_tready && (tx_bytes_rem == 1);
      end

      ST_TX_STREAM_ADDR_HI : begin
        spi_tx_tdata  = tx_cur_addr[15:8];
        spi_tx_tvalid = 1'b1;
      end

      ST_TX_STREAM_ADDR_LO : begin
        spi_tx_tdata  = tx_cur_addr[7:0];
        spi_tx_tvalid = 1'b1;
      end

      ST_TX_STREAM_DATA : begin
        if (ctrl_tx_tdata.dir == SPI_READ) begin
          spi_tx_tdata  = 8'h00;
          spi_tx_tvalid = 1'b1;
        end else begin
          spi_tx_tdata   = tx_byte_tdata;
          spi_tx_tvalid  = tx_byte_tvalid;
          tx_byte_tready = spi_tx_tready;
        end
        spi_tx_tlast   = (tx_bytes_rem == 1);
        ctrl_tx_tready = spi_tx_tvalid && spi_tx_tready && (tx_bytes_rem == 1);
      end

      ST_TX_DRAIN : begin
        // Consume width-converter padding bytes without sending them on MOSI.
        tx_byte_tready = 1'b1;
      end

      default : begin
        // Nothing
      end
    endcase
  end


  //---------------------------------------------------------------------------
  // SPI Interface
  //---------------------------------------------------------------------------

  axis_spi #(
    .HALF_PER_W (HALF_PER_W),
    .CS_HOLD    (CS_HOLD),
    .CS_GUARD   (CS_GUARD)
  ) axis_spi_i (
    .clk           (clk),
    .rst           (rst),
    .half_per      (half_per),
    .s_axis_tdata  (spi_tx_tdata),
    .s_axis_tvalid (spi_tx_tvalid),
    .s_axis_tready (spi_tx_tready),
    .s_axis_tlast  (spi_tx_tlast),
    .m_axis_tdata  (spi_rx_tdata),
    .m_axis_tvalid (spi_rx_tvalid),
    .m_axis_tready (spi_rx_tready),
    .m_axis_tlast  (spi_rx_tlast),
    .sclk          (sclk),
    .cs_n          (cs_n),
    .mosi          (mosi),
    .miso          (miso)
  );


  //---------------------------------------------------------------------------
  // RX State Machine
  //---------------------------------------------------------------------------
  //
  // Filters bytes from spi_rx_*, forwarding data-phase bytes to rx_byte_*
  // (axis_width_conv_rx) per the mode table below:
  //
  //   RAW write  : Discard all bytes until tlast.
  //   RAW read   : Forward all bytes until tlast.
  //   SI_SEQ/REP : For each group of 3, discard addr_hi, discard addr_lo,
  //                forward data byte; repeat until tlast.
  //   STREAMING  : Discard addr_hi, discard addr_lo, then forward all bytes
  //                until tlast.
  //
  //---------------------------------------------------------------------------

  typedef enum logic [3:0] {
    ST_RX_IDLE              = 0,
    ST_RX_RAW_DROP          = 1,
    ST_RX_SI_DISCARD_HI     = 2,
    ST_RX_SI_DISCARD_LO     = 3,
    ST_RX_SI_DATA           = 4,
    ST_RX_STREAM_DISCARD_HI = 5,
    ST_RX_STREAM_DISCARD_LO = 6,
    ST_RX_PASS_DATA         = 7
  } rx_state_t;

  rx_state_t rx_state;

  always_ff @(posedge clk) begin
    if (rst) begin
      rx_state <= ST_RX_IDLE;
    end else begin
      case (rx_state)
        ST_RX_IDLE : begin
          if (ctrl_rx_tvalid) begin
            case (ctrl_rx_tdata.mode)
              MODE_RAW : begin
                rx_state <= ctrl_rx_tdata.dir == SPI_READ ?
                            ST_RX_PASS_DATA : ST_RX_RAW_DROP;
              end
              MODE_SI_SEQ,
              MODE_SI_REP : begin
                rx_state <= ctrl_rx_tdata.dir == SPI_READ ?
                            ST_RX_SI_DISCARD_HI : ST_RX_RAW_DROP;
              end
              MODE_STREAMING : begin
                rx_state <= ctrl_rx_tdata.dir == SPI_READ ?
                            ST_RX_STREAM_DISCARD_HI : ST_RX_RAW_DROP;
              end
              default : rx_state <= ST_RX_RAW_DROP;
            endcase
          end
        end

        ST_RX_PASS_DATA : begin
          if (spi_rx_tvalid && spi_rx_tready && spi_rx_tlast) begin
            rx_state <= ST_RX_IDLE;
          end
        end

        ST_RX_RAW_DROP : begin
          if (spi_rx_tvalid && spi_rx_tready && spi_rx_tlast) begin
            rx_state <= ST_RX_IDLE;
          end
        end

        ST_RX_SI_DISCARD_HI : begin
          if (spi_rx_tvalid && spi_rx_tready) begin
            rx_state <= ST_RX_SI_DISCARD_LO;
          end
        end

        ST_RX_SI_DISCARD_LO : begin
          if (spi_rx_tvalid && spi_rx_tready) begin
            rx_state <= ST_RX_SI_DATA;
          end
        end

        ST_RX_SI_DATA : begin
          if (spi_rx_tvalid && spi_rx_tready) begin
            if (spi_rx_tlast) begin
              rx_state <= ST_RX_IDLE;
            end else begin
              rx_state <= ST_RX_SI_DISCARD_HI;
            end
          end
        end

        ST_RX_STREAM_DISCARD_HI : begin
          if (spi_rx_tvalid && spi_rx_tready) begin
            rx_state <= ST_RX_STREAM_DISCARD_LO;
          end
        end

        ST_RX_STREAM_DISCARD_LO : begin
          if (spi_rx_tvalid && spi_rx_tready) begin
            rx_state <= ST_RX_PASS_DATA;
          end
        end

        default : begin
          rx_state <= ST_RX_IDLE;
        end
      endcase
    end
  end

  always_comb begin
    // Default: consume from spi_rx but don't forward (discard).
    spi_rx_tready  = 1'b0;
    rx_byte_tdata  = spi_rx_tdata;
    rx_byte_tvalid = 1'b0;
    rx_byte_tlast  = spi_rx_tlast;
    ctrl_rx_tready = 1'b0;

    case (rx_state)
      ST_RX_IDLE : begin
        // Waiting; hold spi_rx backpressured.
      end

      ST_RX_PASS_DATA,
      ST_RX_SI_DATA : begin
        // Forward bytes to the width converter.
        spi_rx_tready  = rx_byte_tready;
        rx_byte_tvalid = spi_rx_tvalid;
        ctrl_rx_tready = spi_rx_tvalid && spi_rx_tready && spi_rx_tlast;
      end

      ST_RX_RAW_DROP : begin
        // Consume and discard; do not forward.
        spi_rx_tready  = 1'b1;
        ctrl_rx_tready = spi_rx_tvalid && spi_rx_tready && spi_rx_tlast;
      end

      ST_RX_SI_DISCARD_HI,
      ST_RX_SI_DISCARD_LO,
      ST_RX_STREAM_DISCARD_HI,
      ST_RX_STREAM_DISCARD_LO : begin
        // Consume and discard; do not forward.
        spi_rx_tready = 1'b1;
      end

      default : begin
        // Discard.
        spi_rx_tready = 1'b1;
      end
    endcase
  end


  //---------------------------------------------------------------------------
  // RX Width Converter (8-bit -> 32-bit)
  //---------------------------------------------------------------------------

  axis_width_conv #(
    .WORD_W    (8),
    .IN_WORDS  (1),
    .OUT_WORDS (4),
    .SYNC_CLKS (1),
    .PIPELINE  ("NONE")
  ) axis_width_conv_rx (
    .s_axis_aclk   (clk),
    .s_axis_rst    (rst),
    .s_axis_tdata  (rx_byte_tdata),
    .s_axis_tkeep  (1'b1),
    .s_axis_tlast  (rx_byte_tlast),
    .s_axis_tvalid (rx_byte_tvalid),
    .s_axis_tready (rx_byte_tready),
    .m_axis_aclk   (clk),
    .m_axis_rst    (rst),
    .m_axis_tdata  (rx_word_tdata),
    .m_axis_tkeep  (),
    .m_axis_tlast  (rx_word_tlast),
    .m_axis_tvalid (rx_word_tvalid),
    .m_axis_tready (rx_word_tready)
  );


  //---------------------------------------------------------------------------
  // RX FIFO
  //---------------------------------------------------------------------------

  axi_fifo #(
    .WIDTH (32),
    .SIZE  (RX_FIFO_SIZE)
  ) axi_fifo_i (
    .clk      (clk),
    .reset    (rst),
    .clear    (1'b0),
    .i_tdata  (rx_word_tdata),
    .i_tvalid (rx_word_tvalid),
    .i_tready (rx_word_tready),
    .o_tdata  (rx_fifo_tdata),
    .o_tvalid (rx_fifo_tvalid),
    .o_tready (rx_fifo_tready),
    .space    (),
    .occupied ()
  );

endmodule : ctrlport_spi_adrv


`default_nettype wire


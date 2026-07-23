//
// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: axis_spi
//
// Description:
//
//   SPI master (CPOL=0, CPHA=0) with AXI-Stream TX and RX interfaces. Each
//   byte presented on the input stream is shifted out MSB-first on MOSI while
//   MISO bits are simultaneously assembled into bytes and presented on the
//   output stream (full-duplex).
//
//   CS_N is asserted (driven low) and held low for the duration of the
//   AXI-Stream packet. Between words, CS_N stays asserted and SCLK simply
//   pauses if the next byte in the packet is not yet available (s_axis_tvalid
//   == 0) or if there's back-pressure on the output (m_axis_tready == 0).
//
//   After the last SCLK falling edge, CS_N deassertion timing is controlled by
//   CS_HOLD and CS_GUARD:
//
//     CS_HOLD=0 : CS_N deasserts on the same clock edge as the last SCLK
//                 falling edge.
//     CS_HOLD=1 : CS_N stays low for one additional half-period before being
//                 deasserted.
//
//   After CS_N deasserts, CS_GUARD determines the number of half-period
//   cycles before the next CS_N assertion for the next packet:
//
//     CS_GUARD=1 : One half SCLK period before next CS_N assertion.
//     CS_GUARD=2 : One full SCLK period before next CS_N assertion.
//
//   SPI Clock Rate:
//
//     The SPI clock rate is controlled by setting the desired number of clock
//     cycles per half SPI clock period, minus 1. In other words, the actual
//     SPI clock frequency for a given clock frequency will be:
//
//       f_sclk = f_clk / (2 * (half_per + 1))
//
//     Or, the required half period setting to approximate given SPI clock
//     frequency can be calculated as:
//
//       half_per = round((f_clk / (2 * f_sclk)) - 1)
//
// Parameters:
//
//   HALF_PER_W : Width of the half-period input port in bits. This determines
//                the slowest SPI clock rate that can be achieved.
//   CS_HOLD    : Number of half SCLK periods to hold CS_N low after last
//                falling edge of SCLK. Can be 0 or 1.
//   CS_GUARD   : Number of half SCLK periods to hold CS_N between
//                transactions. Can be 1 or 2.
//
// Signals:
//
//   clk / rst    : Clock and active-high synchronous reset.
//   half_per     : Half-period value; compared against an internal counter
//                  each cycle. SCLK toggles every (half_per + 1) clock cycles.
//                  Minimum value is 0, which yields the fastest SCLK rate of
//                  clk/2. This port must remain stable for the duration of any
//                  active transaction; it is expected to be driven by a static
//                  constant or an external register that is only updated while
//                  the core is idle.
//   s_axis_*     : AXI-Stream slave (TX data in). Each byte is serialized
//                  MSB-first.
//   m_axis_*     : AXI-Stream master (RX data out). Each byte is serialized
//                  MSB-first.
//   sclk         : SPI clock output
//   cs_n         : Active-low chip select
//   mosi         : Master-out / slave-in
//   miso         : Master-in / slave-out input
//

`default_nettype none


module axis_spi #(
  int HALF_PER_W = 8,
  bit CS_HOLD    = 1,
  int CS_GUARD   = 2,

  localparam int DATA_W = 8
) (
  input wire clk,
  input wire rst,

  // Half-period control
  input wire [HALF_PER_W-1:0] half_per,

  // AXI-Stream Slave (TX data in)
  input  wire [DATA_W-1:0] s_axis_tdata,
  input  wire              s_axis_tvalid,
  output logic             s_axis_tready,
  input  wire              s_axis_tlast,

  // AXI-Stream Master (RX data out)
  output logic [DATA_W-1:0] m_axis_tdata,
  output logic              m_axis_tvalid,
  input  wire               m_axis_tready,
  output logic              m_axis_tlast,

  // SPI Interface
  output logic sclk = 1'b0,
  output logic cs_n = 1'b1,
  output logic mosi = 1'b0,
  input  wire  miso
);

  //---------------------------------------------------------------------------
  // Parameter Checks
  //---------------------------------------------------------------------------

  if (CS_HOLD < 0 || CS_HOLD > 1) begin : gen_hold_check
    $error("axis_spi: CS_HOLD must be 0 or 1 (setting is %0d)", CS_HOLD);
  end

  if (CS_GUARD < 1 || CS_GUARD > 2) begin : gen_guard_check
    $error("axis_spi: CS_GUARD must be 1 or 2 (setting is %0d)", CS_GUARD);
  end


  //---------------------------------------------------------------------------
  // Local Parameters
  //---------------------------------------------------------------------------

  // Enough bits to count 0 .. DATA_W-1
  localparam int BIT_CNT_W = $clog2(DATA_W);


  //---------------------------------------------------------------------------
  // State Machine Type
  //---------------------------------------------------------------------------

  typedef enum logic [2:0] {
    ST_IDLE,
    ST_SHIFT_A,
    ST_SHIFT_B,
    ST_HOLD,
    ST_GUARD1,
    ST_GUARD2
  } state_t;

  state_t state;


  //---------------------------------------------------------------------------
  // Internal Signals
  //---------------------------------------------------------------------------

  // Half-period counter. Done asserts when it reaches half_per.
  logic [HALF_PER_W-1:0] half_per_cnt;
  logic                  half_per_done;

  // Bit position counter (0 to DATA_W-1)
  logic [BIT_CNT_W-1:0] bit_cnt;

  // TX / RX shift registers
  logic [DATA_W-1:0] mosi_sr;
  logic [DATA_W-1:0] miso_sr;

  // tlast latched to indicate the end of the SPI transaction
  logic tlast_reg;

  // True when CS_N is currently asserted (mid-packet)
  logic in_packet;

  // One-entry registered output buffer
  logic [DATA_W-1:0] out_data;
  logic              out_valid;
  logic              out_last;

  // True when the output buffer is free this cycle (empty or being drained)
  logic out_buf_free;


  //---------------------------------------------------------------------------
  // Combinational Assignments
  //---------------------------------------------------------------------------

  assign half_per_done = (half_per_cnt == half_per);
  assign out_buf_free  = !out_valid || m_axis_tready;

  // AXI-Stream output driven from the registered output buffer
  assign m_axis_tdata  = out_data;
  assign m_axis_tvalid = out_valid;
  assign m_axis_tlast  = out_last;

  // Accept input when idle and the output buffer has room. In ST_IDLE the SPI
  // bus is paused (SCLK stopped), so CS_N stays asserted while we wait for
  // the next word when in_packet=1.
  assign s_axis_tready = (state == ST_IDLE) && out_buf_free;


  //---------------------------------------------------------------------------
  // Half-Period Counter
  //---------------------------------------------------------------------------
  //
  // Counts up every cycle while a SPI transaction is occurring. Resets to zero
  // the same cycle half_per_done fires so each half-period is exactly
  // (half_per + 1) clock cycles.
  //
  //---------------------------------------------------------------------------

  always_ff @(posedge clk) begin
    if (rst) begin
      half_per_cnt <= '0;
    end else begin
      if (state == ST_IDLE) begin
        half_per_cnt <= '0;
      end else begin
        if (half_per_done) begin
          half_per_cnt <= '0;
        end else begin
          half_per_cnt <= half_per_cnt + 1'b1;
        end
      end
    end
  end


  //---------------------------------------------------------------------------
  // State Machine
  //---------------------------------------------------------------------------

  always_ff @(posedge clk) begin
    if (rst) begin
      state     <= ST_IDLE;
      sclk      <= 1'b0;
      cs_n      <= 1'b1;
      mosi      <= 1'b0;
      mosi_sr   <= 'X;
      miso_sr   <= 'X;
      bit_cnt   <= 'X;
      tlast_reg <= 1'b0;
      in_packet <= 1'b0;
      out_data  <= 'X;
      out_valid <= 1'b0;
      out_last  <= 'X;
    end else begin

      //-----------------------------------------------------------------------
      // Output buffer drain
      //-----------------------------------------------------------------------

      if (m_axis_tready && out_valid) begin
        out_valid <= 1'b0;
      end

      unique case (state)

        //---------------------------------------------------------------------
        // IDLE: SCLK stopped. CS_N follows in_packet (stays asserted between
        // bytes of a packet, deasserted before/after).
        //
        // On input handshake: latch word and tlast, pre-drive MOSI with the
        // MSB, assert CS_N, and enter the shift loop.
        //---------------------------------------------------------------------
        ST_IDLE: begin
          sclk    <= 1'b0;
          cs_n    <= !in_packet;
          bit_cnt <= '0;

          if (s_axis_tvalid && out_buf_free) begin
            mosi_sr   <= s_axis_tdata;
            tlast_reg <= s_axis_tlast;

            // Assert CS_N and pre-drive MSB onto MOSI before the first
            // SCLK rising edge (CPHA=0 requirement).
            cs_n <= 1'b0;
            mosi <= s_axis_tdata[DATA_W-1];

            in_packet <= 1'b1;
            state     <= ST_SHIFT_A;
          end
        end

        //---------------------------------------------------------------------
        // SHIFT_A: First (low) half of the current SCLK bit period.
        //
        // SCLK rises at the end of this half, MISO is sampled on the rising
        // edge.
        //---------------------------------------------------------------------
        ST_SHIFT_A: begin
          if (half_per_done) begin
            sclk    <= 1'b1;
            // Sample MISO into the LSB of the receive shift register
            miso_sr <= {miso_sr[DATA_W-2:0], miso};
            state   <= ST_SHIFT_B;
          end
        end

        //---------------------------------------------------------------------
        // SHIFT_B: Second (high) half of the current SCLK bit period.
        //
        // SCLK falls at the end of this half, MOSI advances to the next bit on
        // the falling edge.
        //
        // On the last bit, push the assembled received byte to the output
        // buffer and choose the next state.
        //---------------------------------------------------------------------
        ST_SHIFT_B: begin
          if (half_per_done) begin
            sclk    <= 1'b0;
            bit_cnt <= bit_cnt + 1;

            // Advance MOSI shift register and drive the next bit
            mosi_sr <= {mosi_sr[DATA_W-2:0], 1'b0};
            mosi    <= mosi_sr[DATA_W-2];

            if (bit_cnt == DATA_W-1) begin
              // Last bit don. Push received word to output buffer. out_valid
              // guaranteed 0 here (see block header).
              out_data  <= miso_sr;
              out_valid <= 1'b1;
              out_last  <= tlast_reg;

              bit_cnt <= '0;

              if (tlast_reg) begin
                // End of packet. Update CS_N based on CS_HOLD.
                in_packet <= 1'b0;
                if (CS_HOLD) begin
                  state <= ST_HOLD;
                end else begin
                  cs_n  <= 1'b1;
                  state <= ST_GUARD1;
                end
              end else begin
                // More words to come. Return to IDLE to accept the next word.
                // CS_N stays asserted while we wait.
                state <= ST_IDLE;
              end
            end else begin
              state <= ST_SHIFT_A;
            end
          end
        end

        //---------------------------------------------------------------------
        // HOLD: Hold CS_N asserted for one extra half-period after the last
        // SCLK falling edge (only entered if CS_HOLD=1), then deassert CS_N
        // and proceed to the guard entry state.
        //---------------------------------------------------------------------
        ST_HOLD: begin
          if (half_per_done) begin
            cs_n  <= 1'b1;
            state <= ST_GUARD1;
          end
        end

        //---------------------------------------------------------------------
        // GUARD1/GUARD2: Hold CS_N deasserted for CS_GUARD half-periods after
        // the end of the transaction.
        //---------------------------------------------------------------------
        ST_GUARD1: begin
          if (half_per_done) begin
            state <= (CS_GUARD > 1) ? ST_GUARD2 : ST_IDLE;
          end
        end

        ST_GUARD2: begin
          if (half_per_done) begin
            state <= ST_IDLE;
          end
        end

        default: begin
          state <= ST_IDLE;
        end

      endcase
    end
  end

endmodule : axis_spi


`default_nettype wire

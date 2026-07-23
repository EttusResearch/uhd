//
// Copyright 2025 Ettus Research, A National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: ctrlport_to_simple_spi
//
// Description:
//    Implements a simple SPI core controlled via CtrlPort. This design
//    is an adaptation of the simple_spi_core module.
//
//    Settings reg map:
//
//    BASE_ADDRESS+0 divider setting
//      bits [15:0] spi clock divider
//
//    BASE_ADDRESS+4 configuration input
//      bits [23:0] slave select, bit0 = slave0 enabled
//      bits [29:24] num bits (1 through 32)
//      bit [30] data input edge = in data bit latched on rising edge of clock
//      bit [31] data output edge = out data bit latched on rising edge of clock
//
//    BASE_ADDRESS+8 output data
//      Writing this register begins a spi transaction.
//      Bits are latched out starting from bit 31.
//      Therefore, data has to be MSB aligned for transmission
//
//    BASE_ADDRESS+12 Status
//      bit [0] ready signal, high when spi core can begin another transaction
//
//    BASE_ADDRESS+16 Readback
//      Readback data from the last SPI transaction
//

module ctrlport_to_simple_spi # (
  //settings register base address
  int BASE_ADDRESS = 0,

  //width of serial enables (up to 24 is possible)
  int SEN_WIDTH = 8,

  //idle state of the spi clock
  bit CLK_IDLE = 0,

  //idle state of the serial enables
  logic [23:0] SEN_IDLE = '1
)
(
  input wire ctrlport_clk,
  input wire ctrlport_rst,

  // Request
  input wire        s_ctrlport_req_wr,
  input wire        s_ctrlport_req_rd,
  input wire [19:0] s_ctrlport_req_addr,
  input wire [31:0] s_ctrlport_req_data,

  // Response
  output logic        s_ctrlport_resp_ack     = 1'b0,
  output logic [ 1:0] s_ctrlport_resp_status  = 2'b0,
  output logic [31:0] s_ctrlport_resp_data    = 32'b0,

  //spi interface, slave selects, clock, data in, data out
  output logic [SEN_WIDTH-1:0]  sen,
  output logic                  sclk,
  output logic                  mosi,
  input  wire                   miso,

  //optional debug output
  output logic [31:0] debug
);

  `include "../rfnoc/core/ctrlport.vh"
  `include "ctrlport_to_simple_spi_regmap_utils.vh"

  wire address_in_range =
    (s_ctrlport_req_addr >= BASE_ADDRESS) &&
    (s_ctrlport_req_addr < BASE_ADDRESS + 'h14); // 5 registers (20 bytes)

  logic [15:0]  sclk_divider;
  logic [23:0]  slave_select;
  logic [5:0]   num_bits;
  logic         datain_edge, dataout_edge;
  logic [31:0]  mosi_data;
  logic         trigger_spi;
  logic [31:0]  datain_reg;
  logic [23:0]  readback_reg;
  logic         readback_stb;
  logic         ready;
  logic         force_cs;

  always_ff @(posedge ctrlport_clk) begin
    if (ctrlport_rst) begin
      s_ctrlport_resp_ack     <= '0;
      s_ctrlport_resp_data    <= 'x;
      s_ctrlport_resp_status  <= '0;
      trigger_spi <= 1'b0;
    end else begin
      // Reset response
      s_ctrlport_resp_ack     <= '0;
      s_ctrlport_resp_data    <= 'x;
      s_ctrlport_resp_status  <= '0;
      trigger_spi             <= 1'b0;

      // Write request
      if (s_ctrlport_req_wr) begin
        s_ctrlport_resp_ack <= '1;

        case (s_ctrlport_req_addr)

          BASE_ADDRESS + SCLK_DIVIDER: begin
            sclk_divider <= s_ctrlport_req_data[SPI_CLK_DIV_MSB : SPI_CLK_DIV];
            force_cs     <= s_ctrlport_req_data[FORCE_SS];
          end

          BASE_ADDRESS + SPI_PERSONALITY: begin
            dataout_edge <= s_ctrlport_req_data[SPI_MOSI_EDGE];
            datain_edge  <= s_ctrlport_req_data[SPI_MISO_EDGE];
            num_bits     <= s_ctrlport_req_data[SPI_NUM_BITS_MSB : SPI_NUM_BITS];
            slave_select <= s_ctrlport_req_data[SPI_SLAVE_SELECT_MSB : SPI_SLAVE_SELECT];
          end

          BASE_ADDRESS + SPI_MOSI_DATA: begin
            mosi_data   <= s_ctrlport_req_data[SPI_DATA_OUT_MSB : SPI_DATA_OUT];
            trigger_spi <= 1'b1; // trigger the SPI transaction
          end

          // No register implementation for provided address
          default: begin
            // Acknowledge and provide error status if address is in range
            if (address_in_range) begin
              s_ctrlport_resp_status <= CTRL_STS_CMDERR;

            // No response if out of range
            end else begin
              s_ctrlport_resp_ack <= 1'b0;
            end
          end

        endcase

      end else if (s_ctrlport_req_rd) begin
        // read request handling
        case (s_ctrlport_req_addr)
          BASE_ADDRESS + SCLK_DIVIDER: begin
            // Divider setting readback
            s_ctrlport_resp_data[SPI_CLK_DIV_MSB : SPI_CLK_DIV] <= sclk_divider[SPI_CLK_DIV_SIZE-1 : 0];
            s_ctrlport_resp_data[FORCE_SS] <= force_cs;
            s_ctrlport_resp_ack <= 1'b1;
          end

          BASE_ADDRESS + SPI_PERSONALITY: begin
            // Control register readback
            s_ctrlport_resp_data[SPI_MOSI_EDGE] <= dataout_edge;
            s_ctrlport_resp_data[SPI_MISO_EDGE] <= datain_edge;
            s_ctrlport_resp_data[SPI_NUM_BITS_MSB : SPI_NUM_BITS] <= num_bits;
            s_ctrlport_resp_data[SPI_SLAVE_SELECT_MSB : SPI_SLAVE_SELECT] <= slave_select;
            s_ctrlport_resp_ack <= 1'b1;
          end

          BASE_ADDRESS + SPI_STATUS: begin
            // Data register readback
            s_ctrlport_resp_data[SPI_READY] <= ready;
            s_ctrlport_resp_ack <= 1'b1;
          end

          BASE_ADDRESS + SPI_READBACK: begin
            // Readback data from the last SPI transaction
            s_ctrlport_resp_data[SPI_READBACK_DATA_MSB : SPI_READBACK_DATA] <= readback_reg;
            s_ctrlport_resp_ack <= 1'b1;
          end

          // No register implementation for provided address
          default: begin
            // Acknowledge and provide error status if address is in range
            if (address_in_range) begin
              s_ctrlport_resp_status <= CTRL_STS_CMDERR;

            // No response if out of range
            end else begin
              s_ctrlport_resp_ack <= 1'b0;
            end
          end

        endcase
      end
    end
  end

  // Latch readback signal
  always_ff @(posedge ctrlport_clk) begin
    if (ctrlport_rst) begin
      readback_reg <= '0;
    end else if(readback_stb) begin
      readback_reg <= datain_reg[23:0]; // readback data is latched from the last SPI transaction
    end
  end

  // SPI Engine Implementation
  // The implementation below is leveraged from the simple_spi_core module.

  typedef enum logic [2:0] {
    WAIT_TRIG,
    PRE_IDLE,
    CLK_REG,
    CLK_INV,
    POST_IDLE,
    IDLE_SEN
  } case_t;

  case_t spi_state;

  logic ready_reg;

  // De-assert ready when a new transaction is triggered. ready_reg will be deasserted
  // on the clock cycle after the trigger, and keep the ready signal low until the
  // transaction is done.
  assign ready = ready_reg && ~trigger_spi;

  // Serial clock either idles or is in one of two clock states
  logic sclk_reg;
  assign sclk = sclk_reg;

  // Serial enables either idle or enabled (based on spi_state or forced
  // via FORCE_SS register field).
  // One pipeline stage to break critical path from register in I/O pads.
  wire              sen_is_idle = ((spi_state == WAIT_TRIG) ||
                                  (spi_state == IDLE_SEN))  &&
                                  ~force_cs;

  wire  [23:0]          sen24   = (sen_is_idle)? SEN_IDLE : (SEN_IDLE ^ slave_select);
  logic [SEN_WIDTH-1:0] sen_reg = SEN_IDLE;

  always_ff @(posedge ctrlport_clk) begin
    if (ctrlport_rst) begin
      sen_reg <= SEN_IDLE;
    end else begin
      sen_reg <= sen24[SEN_WIDTH-1:0];
    end
  end

  assign sen = sen_reg;

  // Data output shift register
  // One pipeline stage to break critical path from register in I/O pads.
  logic   [31:0]  dataout_reg;
  wire    [31:0]  dataout_next = {dataout_reg[30:0], 1'b0};

  always_ff @(posedge ctrlport_clk) begin
    mosi <= dataout_reg[31];
  end

  // Data input shift register
  // Two pipeline stages to break critical path from register in I/O pads.
  logic 		miso_pipe, miso_pipe2;
  always_ff @(posedge ctrlport_clk) begin
    miso_pipe2 <= miso;
    miso_pipe <= miso_pipe2;
  end

  wire  [31:0]  datain_next = {datain_reg[30:0], miso_pipe};

  // Counter for spi clock
  logic [15:0]  sclk_counter;
  wire          sclk_counter_done = (sclk_counter == sclk_divider);
  wire  [15:0]  sclk_counter_next = (sclk_counter_done)? 0 : sclk_counter + 1;

  // Counter for latching bits miso/mosi
  logic [6:0]   bit_counter;
  wire  [6:0]   bit_counter_next = bit_counter + 1;
  wire          bit_counter_done = (bit_counter_next == num_bits);

  always_ff @(posedge ctrlport_clk) begin
    if (ctrlport_rst) begin
      spi_state <= WAIT_TRIG;
      sclk_reg <= CLK_IDLE;
      ready_reg <= 0;
      readback_stb <= 1'b0;
    end
    else begin
      case (spi_state)

      WAIT_TRIG: begin
        if (trigger_spi) begin
          spi_state <= PRE_IDLE;
        end
        readback_stb <= 1'b0;
        ready_reg <= ~trigger_spi;
        dataout_reg <= mosi_data;
        sclk_counter <= 0;
        bit_counter <= 0;
        sclk_reg <= CLK_IDLE;
      end

      // Drive clock to idle state. This also drives the serial enables
      // to their active state.
      PRE_IDLE: begin
        if (sclk_counter_done) begin
          spi_state <= CLK_REG;
        end
        sclk_counter <= sclk_counter_next;
        sclk_reg <= CLK_IDLE;
      end

      // Clock generation states
      CLK_REG: begin
        if (sclk_counter_done) begin
          spi_state <= CLK_INV;
          if (datain_edge  != CLK_IDLE)  begin
            datain_reg  <= datain_next;
          end
          if (dataout_edge != CLK_IDLE && bit_counter != 0) begin
            dataout_reg <= dataout_next;
          end
          sclk_reg <= ~CLK_IDLE; //transition to rising when CLK_IDLE == 0
        end
        sclk_counter <= sclk_counter_next;
      end

      CLK_INV: begin
        if (sclk_counter_done) begin
          // Have all bits been shifted out?
          spi_state <= (bit_counter_done)? POST_IDLE : CLK_REG;
          bit_counter <= bit_counter_next;
          if (datain_edge  == CLK_IDLE) begin
            datain_reg  <= datain_next;
          end
          if (dataout_edge == CLK_IDLE && ~bit_counter_done) begin
            dataout_reg <= dataout_next;
          end
          sclk_reg <= CLK_IDLE; //transition to falling when CLK_IDLE == 0
        end
        sclk_counter <= sclk_counter_next;
      end

      // Drive clock to idle state
      POST_IDLE: begin
        if (sclk_counter_done) begin
          spi_state <= IDLE_SEN;
        end
        sclk_counter <= sclk_counter_next;
        sclk_reg <= CLK_IDLE;
      end

      // Drive serial enables to idle state
      IDLE_SEN: begin
        if (sclk_counter_done) begin
          ready_reg <= 1'b1;
          readback_stb <= 1'b1;
          spi_state <= WAIT_TRIG;
        end
        sclk_counter <= sclk_counter_next;
        sclk_reg <= CLK_IDLE;
      end

      default: spi_state <= WAIT_TRIG;

      endcase //spi_state
    end
  end

  assign debug = {
    8'h00, //8
    trigger_spi, spi_state, //4
    sclk, mosi, miso, ready, //4
    1'b0, bit_counter[6:0], //8
    sclk_counter_done, bit_counter_done, //2
    sclk_counter[5:0] //6
  };

endmodule : ctrlport_to_simple_spi

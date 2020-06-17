//
// Copyright 2020 Ettus Research, A National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: simple_spi_core_64bit
// Description:
// Simple SPI core based on simple_spi_core.v. Extended to 64 bit transmissions
// while preserving the same control interface for 32 bit transmissions.

// Settings register controlled.
// 4 settings regs, control and data
// 1 up to 64-bit readback and status signal

// Settings reg map:
//
// BASE+0 divider setting
// bits [15:0] spi clock divider
//
// BASE+1 configuration input
// bits [23:0] slave select, bit0 = slave0 enabled
// bits [29:24] num bits (0 through 63; value of 0 transmits 64 bits)
// bit [30] data input edge = in data bit latched on rising edge of clock
// bit [31] data output edge = out data bit latched on rising edge of clock
//
// BASE+2 input data (bits 63...32)
// Writing this register begins a spi transaction. If up to 32 bits (MAX_BITS <=
// 32) are required only this input data register needs to be written. Bits are
// latched out from bit 63. Therefore, load this register aligning with MSBs.
//
// BASE+3 input data (bits 31...0, present if MAX_BITS>32)
// This register needs to be written before accessing BASE+2 input data
// register when MAX_BITS > 32.
//
// Readback
// Bits are latched into bit 0.

module simple_spi_core_64bit
  #(
    //settings register base address
    parameter BASE = 0,

    //width of serial enables (up to 24 is possible)
    parameter WIDTH = 8,

    //idle state of the spi clock
    parameter CLK_IDLE = 0,

    //idle state of the serial enables
    parameter SEN_IDLE = 24'hffffff,

    //maximum number of bits for single transmission (<=64)
    parameter MAX_BITS = 32
  )
  (
    //clock and synchronous reset
    input clock, input reset,

    //32-bit settings bus inputs
    input set_stb, input [7:0] set_addr, input [31:0] set_data,

    //up to 64-bit data readback
    output [MAX_BITS-1:0] readback,
    output reg readback_stb,

    //read is high when spi core can begin another transaction
    output ready,

    //spi interface, slave selects, clock, data in, data out
    output [WIDTH-1:0] sen,
    output reg sclk,
    output reg mosi,
    input miso,

    //optional debug output
    output [23:0] debug
  );

  // assert for MAX_BITS
  generate
  if (MAX_BITS > 64 || MAX_BITS < 1) begin
    MAX_BITS_must_be_between_1_and_64();
  end
  endgenerate

  wire [15:0] sclk_divider;
  setting_reg #(.my_addr(BASE+0),.width(16)) divider_sr(
    .clk(clock),.rst(reset),.strobe(set_stb),.addr(set_addr),.in(set_data),
    .out(sclk_divider),.changed());

  wire [23:0] slave_select;
  wire [5:0] num_bits;
  wire datain_edge, dataout_edge;
  setting_reg #(.my_addr(BASE+1),.width(32)) ctrl_sr(
    .clk(clock),.rst(reset),.strobe(set_stb),.addr(set_addr),.in(set_data),
    .out({dataout_edge, datain_edge, num_bits, slave_select}),.changed());

  wire [63:0] mosi_data;
  wire trigger_spi;
  setting_reg #(.my_addr(BASE+2),.width(32)) data_upper_sr(
    .clk(clock),.rst(reset),.strobe(set_stb),.addr(set_addr),.in(set_data),
    .out(mosi_data[63:32]),.changed(trigger_spi));
  setting_reg #(.my_addr(BASE+3),.width(32)) data_lower_sr(
    .clk(clock),.rst(reset),.strobe(set_stb),.addr(set_addr),.in(set_data),
    .out(mosi_data[31:0]),.changed());

  localparam WAIT_TRIG = 0;
  localparam PRE_IDLE = 1;
  localparam CLK_REG = 2;
  localparam CLK_INV = 3;
  localparam POST_IDLE = 4;
  localparam IDLE_SEN = 5;

  reg [2:0] state;

  reg ready_reg;
  assign ready = ready_reg && ~trigger_spi;

  //serial clock either idles or is in one of two clock states
  //One pipeline stage to align output data with clock edge.
  reg sclk_reg;
  always @(posedge clock) begin
    sclk <= sclk_reg;
  end

  //serial enables either idle or enabled based on state
  // IJB. One pipeline stage to break critical path from register in I/O pads.
  wire sen_is_idle = (state == WAIT_TRIG) || (state == IDLE_SEN);
  wire [23:0] sen24 = (sen_is_idle) ? SEN_IDLE : (SEN_IDLE ^ slave_select);
  reg [WIDTH-1:0] sen_reg = SEN_IDLE[WIDTH-1:0];
  always @(posedge clock) begin
    if (reset) begin
      sen_reg <= SEN_IDLE[WIDTH-1:0];
    end else begin
      sen_reg <= sen24[WIDTH-1:0];
    end
  end
  assign sen = sen_reg;

  //data output shift register
  // IJB. One pipeline stage to break critical path from register in I/O pads.
  reg [MAX_BITS-1:0] dataout_reg = {MAX_BITS {1'b0}};
  wire [MAX_BITS-1:0] dataout_next = {dataout_reg[MAX_BITS-2:0], 1'b0};

  always @(posedge clock) begin
    mosi <= dataout_reg[MAX_BITS-1];
  end

  //data input shift register
  // IJB. Two pipeline stages to break critical path from register in I/O pads.
  reg  miso_pipe, miso_pipe2;
  always @(posedge clock) begin
    miso_pipe2 <= miso;
    miso_pipe <= miso_pipe2;
  end

  // Register to control input data capturing, compensating 2 miso_pipe
  // registers and the output register on mosi/sclk.
  //
  // When sclk_counter_done is asserted the FSM below updates sclk_reg and
  // dataout_reg (compensated by datain_capture_reg).
  // One clock cycle later those values get propagated to sclk and mosi. On
  // the active datain_edge miso would be capture into miso_pipe2. This clock
  // cycle is compensated by datain_capture_pipe[0].
  // Propagation of miso data to miso_pipe is compensated by
  // datain_capture_pipe[1].
  // On the next clock edge datain_capture_pipe[1] serves as enable signal for
  // datain_reg which then consumes the aligned data from miso_pipe.
  reg datain_capture_reg = 1'b0;
  reg [1:0] datain_capture_pipe;
  wire capturing_done = ~| {datain_capture_reg, datain_capture_pipe};

  reg [MAX_BITS-1:0] datain_reg;
  wire [MAX_BITS-1:0] datain_next = {datain_reg[MAX_BITS-2:0], miso_pipe};
  assign readback = datain_reg;

  always @(posedge clock) begin
    datain_capture_pipe <= {datain_capture_pipe[0], datain_capture_reg};
    if (datain_capture_pipe[1]) begin
      datain_reg <= datain_next;
    end
  end

  //counter for spi clock
  reg [15:0] sclk_counter = 16'b0;
  wire sclk_counter_done = (sclk_counter == sclk_divider);
  wire [15:0] sclk_counter_next = (sclk_counter_done)? 0 : sclk_counter + 1;

  //counter for latching bits miso/mosi
  reg [5:0] bit_counter = 6'b0;
  wire [5:0] bit_counter_next = bit_counter + 1;
  wire bit_counter_done = (bit_counter_next == num_bits);

  always @(posedge clock) begin
    if (reset) begin
      state <= WAIT_TRIG;
      sclk_reg <= CLK_IDLE;
      ready_reg <= 0;
      readback_stb <= 1'b0;
    end else begin
      datain_capture_reg <= 1'b0;

      case (state)
        WAIT_TRIG: begin
          if (trigger_spi) begin
            state <= PRE_IDLE;
          end
          readback_stb <= 1'b0;
          ready_reg <= ~trigger_spi;
          dataout_reg <= mosi_data[63:64-MAX_BITS];
          sclk_counter <= 0;
          bit_counter <= 0;
          sclk_reg <= CLK_IDLE;
        end

        PRE_IDLE: begin
          if (sclk_counter_done) begin
            state <= CLK_REG;
          end
          sclk_counter <= sclk_counter_next;
          sclk_reg <= CLK_IDLE;
        end

        CLK_REG: begin
          if (sclk_counter_done) begin
            state <= CLK_INV;
            if (datain_edge  != CLK_IDLE) begin
              datain_capture_reg <= 1'b1;
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
            state <= (bit_counter_done) ? POST_IDLE : CLK_REG;
            bit_counter <= bit_counter_next;
            if (datain_edge  == CLK_IDLE) begin
              datain_capture_reg <= 1'b1;
            end
            if (dataout_edge == CLK_IDLE && ~bit_counter_done) begin
              dataout_reg <= dataout_next;
            end
            sclk_reg <= CLK_IDLE; //transition to falling when CLK_IDLE == 0
          end
          sclk_counter <= sclk_counter_next;
        end

        POST_IDLE: begin
          if (sclk_counter_done) begin
            state <= IDLE_SEN;
          end
          sclk_counter <= sclk_counter_next;
          sclk_reg <= CLK_IDLE;
        end

        IDLE_SEN: begin
          if (sclk_counter_done && capturing_done) begin
            ready_reg <= 1'b1;
            readback_stb <= 1'b1;
            state <= WAIT_TRIG;
          end
          sclk_counter <= sclk_counter_next;
          sclk_reg <= CLK_IDLE;
        end

        default: begin
          state <= WAIT_TRIG;
        end
      endcase //state
    end
  end

  assign debug = {
    trigger_spi, state, //4
    sclk, mosi, miso, ready, //4
    2'b0, bit_counter[5:0], //8
    sclk_counter_done, bit_counter_done, //2
    sclk_counter[5:0] //6
  };

endmodule //simple_spi_core

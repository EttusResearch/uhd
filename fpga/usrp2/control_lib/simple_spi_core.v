//
// Copyright 2012 Ettus Research LLC
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

// Simple SPI core, the simplest, yet complete spi core I can think of

// Settings register controlled.
// 2 settings regs, control and data
// 1 32-bit readback and status signal

// Settings reg map:
//
// BASE+0 divider setting
// bits [15:0] spi clock divider
//
// BASE+1 configuration input
// bits [23:0] slave select, bit0 = slave0 enabled
// bits [29:24] num bits (1 through 32)
// bit [30] data input edge = in data bit latched on rising edge of clock
// bit [31] data output edge = out data bit latched on rising edge of clock
//
// BASE+2 input data
// Writing this register begins a spi transaction.
// Bits are latched out from bit 0.
// Therefore, load this register in reverse.
//
// Readback
// Bits are latched into bit 0.
// Therefore, data will be in-order.

module simple_spi_core
    #(
        //settings register base address
        parameter BASE = 0,

        //width of serial enables (up to 24 is possible)
        parameter WIDTH = 8,

        //idle state of the spi clock
        parameter CLK_IDLE = 0,

        //idle state of the serial enables
        parameter SEN_IDLE = 24'hffffff
    )
    (
        //clock and synchronous reset
        input clock, input reset,

        //32-bit settings bus inputs
        input set_stb, input [7:0] set_addr, input [31:0] set_data,

        //32-bit data readback
        output [31:0] readback,

        //read is high when spi core can begin another transaction
        output ready,

        //spi interface, slave selects, clock, data in, data out
        output [WIDTH-1:0] sen,
        output sclk,
        output mosi,
        input miso,

        //optional debug output
        output [31:0] debug
    );

    wire [15:0] sclk_divider;
    setting_reg #(.my_addr(BASE+0),.width(16)) divider_sr(
        .clk(clock),.rst(reset),.strobe(set_stb),.addr(set_addr),.in(set_data),
        .out(sclk_divider),.changed());

    wire [23:0] slave_select;
    wire [5:0] num_bits;
    wire datain_edge, dataout_edge;
    setting_reg #(.my_addr(BASE+1),.width(32)) config_sr(
        .clk(clock),.rst(reset),.strobe(set_stb),.addr(set_addr),.in(set_data),
        .out({dataout_edge, datain_edge, num_bits, slave_select}),.changed());

    wire [31:0] mosi_data;
    wire trigger_spi;
    setting_reg #(.my_addr(BASE+2),.width(32)) data_sr(
        .clk(clock),.rst(reset),.strobe(set_stb),.addr(set_addr),.in(set_data),
        .out(mosi_data),.changed(trigger_spi));

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
    reg sclk_reg;
    assign sclk = sclk_reg;

    //serial enables either idle or enabled based on state
    wire sen_is_idle = (state == WAIT_TRIG) || (state == IDLE_SEN);
    wire [23:0] sen24 = (sen_is_idle)? SEN_IDLE : (SEN_IDLE ^ slave_select);
    reg [WIDTH-1:0] sen_reg;
    always @(posedge clock) sen_reg <= sen24[WIDTH-1:0];
    assign sen = sen_reg;

    //data output shift register
    reg [31:0] dataout_reg;
    wire [31:0] dataout_next = {dataout_reg[30:0], 1'b0};
    assign mosi = dataout_reg[31];

    //data input shift register
    reg [31:0] datain_reg;
    wire [31:0] datain_next = {datain_reg[30:0], miso};
    assign readback = datain_reg;

    //counter for spi clock
    reg [15:0] sclk_counter;
    wire sclk_counter_done = (sclk_counter == sclk_divider);
    wire [15:0] sclk_counter_next = (sclk_counter_done)? 0 : sclk_counter + 1;

    //counter for latching bits miso/mosi
    reg [6:0] bit_counter;
    wire [6:0] bit_counter_next = bit_counter + 1;
    wire bit_counter_done = (bit_counter_next == num_bits);

    always @(posedge clock) begin
        if (reset) begin
            state <= WAIT_TRIG;
            sclk_reg <= CLK_IDLE;
            ready_reg <= 0;
        end
        else begin
            case (state)

            WAIT_TRIG: begin
                if (trigger_spi) state <= PRE_IDLE;
                ready_reg <= ~trigger_spi;
                dataout_reg <= mosi_data;
                sclk_counter <= 0;
                bit_counter <= 0;
                sclk_reg <= CLK_IDLE;
            end

            PRE_IDLE: begin
                if (sclk_counter_done) state <= CLK_REG;
                sclk_counter <= sclk_counter_next;
                sclk_reg <= CLK_IDLE;
            end

            CLK_REG: begin
                if (sclk_counter_done) begin
                    state <= CLK_INV;
                    if (datain_edge  != CLK_IDLE)                     datain_reg  <= datain_next;
                    if (dataout_edge != CLK_IDLE && bit_counter != 0) dataout_reg <= dataout_next;
                    sclk_reg <= ~CLK_IDLE; //transition to rising when CLK_IDLE == 0
                end
                sclk_counter <= sclk_counter_next;
            end

            CLK_INV: begin
                if (sclk_counter_done) begin
                    state <= (bit_counter_done)? POST_IDLE : CLK_REG;
                    bit_counter <= bit_counter_next;
                    if (datain_edge  == CLK_IDLE)                      datain_reg  <= datain_next;
                    if (dataout_edge == CLK_IDLE && ~bit_counter_done) dataout_reg <= dataout_next;
                    sclk_reg <= CLK_IDLE; //transition to falling when CLK_IDLE == 0
                end
                sclk_counter <= sclk_counter_next;
            end

            POST_IDLE: begin
                if (sclk_counter_done) state <= IDLE_SEN;
                sclk_counter <= sclk_counter_next;
                sclk_reg <= CLK_IDLE;
            end

            IDLE_SEN: begin
                if (sclk_counter_done) state <= WAIT_TRIG;
                sclk_counter <= sclk_counter_next;
                sclk_reg <= CLK_IDLE;
            end

            default: state <= WAIT_TRIG;

            endcase //state
        end
    end

    assign debug = {
        trigger_spi, state, //4
        sclk, mosi, miso, ready, //4
        sen[7:0], //8
        1'b0, bit_counter[6:0], //8
        sclk_counter_done, bit_counter_done, //2
        sclk_counter[5:0] //6
    };

endmodule //simple_spi_core

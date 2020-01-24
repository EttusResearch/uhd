
//
// Copyright 2013 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//


//create a compressed vita based uart data interface

module cvita_uart
#(
    parameter SIZE = 0
)
(
    //clocking interface
    input clk, input rst,

    //uart interface
    input rxd, output txd,

    //chdr fifo input
    input [63:0] i_tdata,
    input i_tlast,
    input i_tvalid,
    output i_tready,

    //chdr fifo output
    output [63:0] o_tdata,
    output o_tlast,
    output o_tvalid,
    input o_tready
);

    reg [31:0] sid;

    //baud clock divider
    reg [15:0] clkdiv;

    //hold rx in disable until a tx event
    reg rxd_enable;

    //==================================================================
    //== RXD capture and packet generation interface
    //==================================================================
    wire [7:0] rx_char;
    wire fifo_empty;
    wire fifo_read;
    reg [11:0] seqnum;
    wire pgen_trigger;
    wire pgen_done;

    //rx uart capture
    simple_uart_rx #(.SIZE(SIZE)) simple_uart_rx
    (
        .clk(clk), .rst(rst | ~rxd_enable),
        .fifo_out(rx_char), .fifo_read(fifo_read), .fifo_level(), .fifo_empty(fifo_empty),
        .clkdiv(clkdiv), .rx(rxd)
    );

    //packet generation - holds rx character
    context_packet_gen context_packet_gen
    (
        .clk(clk), .reset(rst), .clear(1'b0),
        .trigger(pgen_trigger),
        .seqnum(seqnum),
        .sid({sid[15:0], sid[31:16]}),
        .body({56'b0, rx_char}),
        .vita_time(64'b0),

        .done(pgen_done),
        .o_tdata(o_tdata), .o_tlast(o_tlast), .o_tvalid(o_tvalid), .o_tready(o_tready)
    );

    //state machine to manage pgen and rx uart
    reg [1:0] rxd_state;
    localparam RXD_STATE_RECV_CHAR = 0;
    localparam RXD_STATE_PGEN_TRIG = 1;
    localparam RXD_STATE_WAIT_DONE = 2;
    localparam RXD_STATE_READ_FIFO = 3;

    always @(posedge clk) begin
        if (rst) begin
            seqnum <= 12'b0;
            rxd_state <= RXD_STATE_RECV_CHAR;
        end
        else case (rxd_state)

        RXD_STATE_RECV_CHAR: begin
            if (!fifo_empty && rxd_enable) rxd_state <= RXD_STATE_PGEN_TRIG;
        end

        RXD_STATE_PGEN_TRIG: begin
            rxd_state <= RXD_STATE_WAIT_DONE;
        end

        RXD_STATE_WAIT_DONE: begin
            if (pgen_done) rxd_state <= RXD_STATE_READ_FIFO;
        end

        RXD_STATE_READ_FIFO: begin
            rxd_state <= RXD_STATE_RECV_CHAR;
            seqnum <= seqnum + 1'b1;
        end

        endcase //rxd_state
    end

    assign fifo_read = (rxd_state == RXD_STATE_READ_FIFO) || (!rxd_enable);
    assign pgen_trigger = (rxd_state == RXD_STATE_PGEN_TRIG);

    //==================================================================
    //== TXD generation and packet control interface
    //==================================================================
    wire [7:0] tx_char;
    wire fifo_write;
    wire fifo_full;

    simple_uart_tx #(.SIZE(SIZE)) simple_uart_tx
    (
        .clk(clk), .rst(rst),
        .fifo_in(tx_char), .fifo_write(fifo_write), .fifo_level(), .fifo_full(fifo_full),
        .clkdiv(clkdiv), .baudclk(), .tx(txd)
    );

    //state machine to manage control and tx uart
    reg [1:0] txd_state;
    localparam TXD_STATE_RECV_CHDR = 0;
    localparam TXD_STATE_RECV_TIME = 1;
    localparam TXD_STATE_RECV_BODY = 2;
    localparam TXD_STATE_DROP_FIFO = 3;

    always @(posedge clk) begin
        if (rst) begin;
            txd_state <= TXD_STATE_RECV_CHDR;
            rxd_enable <= 1'b0;
        end
        if (i_tvalid && i_tready) case (txd_state)

        TXD_STATE_RECV_CHDR: begin
            txd_state <= (i_tdata[61])? TXD_STATE_RECV_TIME : TXD_STATE_RECV_BODY;
            sid <= i_tdata[31:0];
        end

        TXD_STATE_RECV_TIME: begin
            txd_state <= TXD_STATE_RECV_BODY;
        end

        TXD_STATE_RECV_BODY: begin
            txd_state <= (i_tlast)? TXD_STATE_RECV_CHDR : TXD_STATE_DROP_FIFO;
            clkdiv <= i_tdata[47:32];
            rxd_enable <= 1'b1;
        end

        TXD_STATE_DROP_FIFO: begin
            if (i_tlast) txd_state <= TXD_STATE_RECV_CHDR;
        end

        endcase //txd_state
    end

    assign tx_char = i_tdata[7:0];
    assign fifo_write = (txd_state == TXD_STATE_RECV_BODY) && i_tvalid && i_tready;
    assign i_tready = !fifo_full;

endmodule // cvita_uart

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

// A settings and readback bus controlled via fifo36 interface

//TODO: take vita packets as input and use tsf to wait for time
//currently we skip vita packet on input, strait to payload

module settings_readback_bus_fifo_ctrl
    #(
        parameter SID = 0, //stream id for vita return packet
        parameter PROT_DEST = 0 //protocol framer destination
    )
    (
        //clock and synchronous reset for all interfaces
        input clock, input reset,

        //current system time
        input [63:0] vita_time,

        //input fifo36 interface control
        input [35:0] in_data, input in_valid, output in_ready,

        //output fifo36 interface status
        output [35:0] out_data, output out_valid, input out_ready,

        //32-bit settings bus outputs
        output strobe, output [7:0] addr, output [31:0] data,

        //16X 32-bit inputs for readback
        input [31:0] word00,
        input [31:0] word01,
        input [31:0] word02,
        input [31:0] word03,
        input [31:0] word04,
        input [31:0] word05,
        input [31:0] word06,
        input [31:0] word07,
        input [31:0] word08,
        input [31:0] word09,
        input [31:0] word10,
        input [31:0] word11,
        input [31:0] word12,
        input [31:0] word13,
        input [31:0] word14,
        input [31:0] word15,

        //debug output
        output [31:0] debug
    );

    wire [35:0] in_data0;
    wire in_valid0, in_ready0;

    fifo_cascade #(.WIDTH(36), .SIZE(9/*512 lines plenty for short pkts*/)) input_fifo (
        .clk(clock), .reset(reset), .clear(0),
        .datain(in_data),   .src_rdy_i(in_valid),  .dst_rdy_o(in_ready),
        .dataout(in_data0), .src_rdy_o(in_valid0), .dst_rdy_i(in_ready0)
    );

    wire reading = in_valid0 && in_ready0;
    wire writing = out_valid && out_ready;

    //state machine constants
    localparam READ_HDR = 0;
    localparam READ_DATA = 1;
    localparam WAIT_EOF = 2;
    localparam ACTION_EVENT = 3;
    localparam WRITE_PROT_HDR = 4;
    localparam WRITE_VRT_HDR = 5;
    localparam WRITE_VRT_SID = 6;
    localparam WRITE_VRT_TSF0 = 7;
    localparam WRITE_VRT_TSF1 = 8;
    localparam WRITE_RB_HDR = 9;
    localparam WRITE_RB_DATA = 10;

    reg [3:0] state;

    //holdover from current read inputs
    reg [31:0] in_data_reg, in_hdr_reg;
    wire [7:0] in_addr = in_hdr_reg[7:0];
    wire [7:0] do_poke = in_hdr_reg[8];

    always @(posedge clock) begin
        if (reset) begin
            state <= READ_HDR;
            in_hdr_reg <= 0;
            in_data_reg <= 0;
        end
        else begin
            case (state)

            READ_HDR: begin
                if (reading/* && in_data0[32]*/) begin
                    in_hdr_reg <= in_data0[31:0];
                    state <= READ_DATA;
                end
            end

            READ_DATA: begin
                if (reading) begin
                    in_data_reg <= in_data0[31:0];
                    state <= (in_data0[33])? ACTION_EVENT : WAIT_EOF;
                end
            end

            WAIT_EOF: begin
                if (reading) begin
                    if (in_data0[33]) begin
                        state <= ACTION_EVENT;
                    end
                end
            end

            ACTION_EVENT: begin // poking and peeking happens here!
                state <= WRITE_PROT_HDR;
            end

            WRITE_RB_DATA: begin
                if (writing) begin
                    state <= READ_HDR;
                end
            end

            default: begin
                if (writing) begin
                    state <= state + 1;
                end
            end

            endcase //state
        end
    end

    //readback mux
    reg [31:0] rb_data;
    reg [63:0] rb_time;
    always @(posedge clock) begin
        if (state == ACTION_EVENT) begin
            rb_time <= vita_time;
            case (in_addr[3:0])
                0 : rb_data <= word00;
                1 : rb_data <= word01;
                2 : rb_data <= word02;
                3 : rb_data <= word03;
                4 : rb_data <= word04;
                5 : rb_data <= word05;
                6 : rb_data <= word06;
                7 : rb_data <= word07;
                8 : rb_data <= word08;
                9 : rb_data <= word09;
                10: rb_data <= word10;
                11: rb_data <= word11;
                12: rb_data <= word12;
                13: rb_data <= word13;
                14: rb_data <= word14;
                15: rb_data <= word15;
            endcase // case(addr_reg[3:0])
        end
    end

    //assign protocol framer header
    wire [31:0] prot_hdr;
    assign prot_hdr[15:0] = 24; //bytes in proceeding vita packet
    assign prot_hdr[16] = 1; //yes frame
    assign prot_hdr[18:17] = PROT_DEST;
    assign prot_hdr[31:19] = 0; //nothing

    //register for output data
    reg [31:0] out_data_int;
    always @* begin
        case (state)
            WRITE_PROT_HDR: out_data_int <= prot_hdr;
            WRITE_VRT_HDR:  out_data_int <= {12'b010100000001, 4'b0/*seqno*/, 16'd6};
            WRITE_VRT_SID:  out_data_int <= SID;
            WRITE_VRT_TSF0: out_data_int <= rb_time[63:32];
            WRITE_VRT_TSF1: out_data_int <= rb_time[31:0];
            WRITE_RB_HDR:   out_data_int <= in_hdr_reg;
            WRITE_RB_DATA:  out_data_int <= rb_data;
            default:        out_data_int <= 0;
        endcase //state
    end

    //assign to input fifo interface
    assign in_ready0 = (state < ACTION_EVENT);

    //assign to output fifo interface
    assign out_valid = (state > ACTION_EVENT);
    assign out_data[35:34] = 2'b0;
    assign out_data[33] = (state == WRITE_RB_DATA);
    assign out_data[32] = (state == WRITE_PROT_HDR);
    assign out_data[31:0] = out_data_int;

    //assign to settings bus interface
    assign strobe = (state == ACTION_EVENT) && do_poke;
    assign data = in_data_reg;
    assign addr = in_addr;

    assign debug = {
        state,
        in_valid, in_ready, in_data[33:32],
        in_valid0, in_ready0, in_data0[33:32],
        out_valid, out_ready, out_data[33:32],
        16'b0
    };

endmodule //settings_readback_bus_fifo_ctrl

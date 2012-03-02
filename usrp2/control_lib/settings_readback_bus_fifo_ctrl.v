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

module settings_readback_bus_fifo_ctrl
    #(
        parameter SID = 0, //stream id for vita return packet
        parameter PROT_DEST = 0 //protocol framer destination
    )
    (
        //clock and synchronous reset for all interfaces
        input clock, input reset, input clear,

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

    wire reading = in_valid && in_ready;
    wire writing = out_valid && out_ready;

    //------------------------------------------------------------------
    //-- The command fifo:
    //-- Stores an individual register access command per line.
    //------------------------------------------------------------------
    wire [63:0] in_command_ticks, out_command_ticks;
    wire [31:0] in_command_hdr, out_command_hdr;
    wire [31:0] in_command_data, out_command_data;
    wire in_command_has_time, out_command_has_time;
    wire in_command_valid, in_command_ready;
    wire out_command_valid, out_command_ready;

    fifo_cascade #(.WIDTH(129), .SIZE(4)) command_fifo (
        .clk(clock), .reset(reset), .clear(clear),
        .datain({in_command_ticks, in_command_hdr, in_command_data, in_command_has_time}),
        .src_rdy_i(in_command_valid), .dst_rdy_o(in_command_ready),
        .dataout({out_command_ticks, out_command_hdr, out_command_data, out_command_has_time}),
        .src_rdy_o(out_command_valid), .dst_rdy_i(out_command_ready)
    );

    //------------------------------------------------------------------
    //-- Input state machine:
    //-- Read input packet and fill a command fifo entry.
    //------------------------------------------------------------------
    localparam READ_LINE0     = 0;
    localparam VITA_HDR       = 1;
    localparam VITA_SID       = 2;
    localparam VITA_CID0      = 3;
    localparam VITA_CID1      = 4;
    localparam VITA_TSI       = 5;
    localparam VITA_TSF0      = 6;
    localparam VITA_TSF1      = 7;
    localparam READ_HDR       = 8;
    localparam READ_DATA      = 9;
    localparam WAIT_EOF       = 10;
    localparam STORE_CMD      = 11;

    reg [4:0] in_state;

    //holdover from current read inputs
    reg [31:0] in_data_reg, in_hdr_reg;
    reg [63:0] in_ticks_reg;
    wire has_sid = in_data[28];
    wire has_cid = in_data[27];
    wire has_tsi = in_data[23:22] != 0;
    wire has_tsf = in_data[21:20] != 0;
    reg has_sid_reg, has_cid_reg, has_tsi_reg, has_tsf_reg;

    assign in_ready = (in_state < STORE_CMD);

    //wire-up command inputs
    assign in_command_valid    = (in_state == STORE_CMD);
    assign in_command_ticks    = in_ticks_reg;
    assign in_command_data     = in_data_reg;
    assign in_command_hdr      = in_hdr_reg;
    assign in_command_has_time = has_tsf_reg;

    always @(posedge clock) begin
        if (reset) begin
            in_state <= READ_LINE0;
        end
        else begin
            case (in_state)

            READ_LINE0: begin
                if (reading/* && in_data[32]*/) in_state <= VITA_HDR;
            end

            VITA_HDR: begin
                if (reading) begin
                    if      (has_sid) in_state <= VITA_SID;
                    else if (has_cid) in_state <= VITA_CID0;
                    else if (has_tsi) in_state <= VITA_TSI;
                    else if (has_tsf) in_state <= VITA_TSF0;
                    else              in_state <= READ_HDR;
                end
                has_sid_reg <= has_sid;
                has_cid_reg <= has_cid;
                has_tsi_reg <= has_tsi;
                has_tsf_reg <= has_tsf;
            end

            VITA_SID: begin
                if (reading) begin
                    if      (has_cid_reg) in_state <= VITA_CID0;
                    else if (has_tsi_reg) in_state <= VITA_TSI;
                    else if (has_tsf_reg) in_state <= VITA_TSF0;
                    else                  in_state <= READ_HDR;
                end
            end

            VITA_CID0: begin
                if (reading) in_state <= VITA_CID1;
            end

            VITA_CID1: begin
                if (reading) begin
                    if      (has_tsi_reg) in_state <= VITA_TSI;
                    else if (has_tsf_reg) in_state <= VITA_TSF0;
                    else                  in_state <= READ_HDR;
                end
            end

            VITA_TSI: begin
                if (reading) begin
                    if (has_tsf_reg) in_state <= VITA_TSF0;
                    else             in_state <= READ_HDR;
                end
            end

            VITA_TSF0: begin
                if (reading) in_state <= VITA_TSF1;
                in_ticks_reg[63:32] <= in_data;
            end

            VITA_TSF1: begin
                if (reading) in_state <= READ_HDR;
                in_ticks_reg[31:0] <= in_data;
            end

            READ_HDR: begin
                if (reading) in_state <= READ_DATA;
                in_hdr_reg <= in_data[31:0];
            end

            READ_DATA: begin
                if (reading) in_state <= (in_data[33])? STORE_CMD : WAIT_EOF;
                in_data_reg <= in_data[31:0];
            end

            WAIT_EOF: begin
                if (reading && in_data[33]) in_state <= STORE_CMD;
            end

            STORE_CMD: begin
                if (in_command_valid && in_command_ready) in_state <= READ_LINE0;
            end

            endcase //in_state
        end
    end

    //------------------------------------------------------------------
    //-- Output state machine:
    //-- Read a command fifo entry, act on it, produce ack packet.
    //------------------------------------------------------------------
    localparam LOAD_CMD       = 0;
    localparam WAIT_CMD       = 1;
    localparam ACTION_EVENT   = 2;
    localparam WRITE_PROT_HDR = 3;
    localparam WRITE_VRT_HDR  = 4;
    localparam WRITE_VRT_SID  = 5;
    localparam WRITE_VRT_TSF0 = 6;
    localparam WRITE_VRT_TSF1 = 7;
    localparam WRITE_RB_HDR   = 8;
    localparam WRITE_RB_DATA  = 9;

    reg [4:0] out_state;

    //holdover from current read inputs
    reg [31:0] out_data_reg, out_hdr_reg;
    reg [63:0] out_ticks_reg;

    assign out_valid = (out_state > ACTION_EVENT);

    assign out_command_ready = (out_state == LOAD_CMD);

    wire now, early, late, too_early;
    time_compare time_compare(
        .time_now(vita_time), .trigger_time(out_ticks_reg),
        .now(now), .early(early), .late(late), .too_early(too_early));

    always @(posedge clock) begin
        if (reset) begin
            out_state <= LOAD_CMD;
        end
        else begin
            case (out_state)

            LOAD_CMD: begin
                if (out_command_valid && out_command_ready) begin
                    out_state <= (out_command_has_time)? WAIT_CMD : ACTION_EVENT;
                    out_data_reg <= out_command_data;
                    out_hdr_reg <= out_command_hdr;
                    out_ticks_reg <= out_command_ticks;
                end
            end

            WAIT_CMD: begin
                if (now || late) out_state <= ACTION_EVENT;
            end

            ACTION_EVENT: begin // poking and peeking happens here!
                out_state <= WRITE_PROT_HDR;
            end

            WRITE_RB_DATA: begin
                if (writing) out_state <= LOAD_CMD;
            end

            default: begin
                if (writing) out_state <= out_state + 1;
            end

            endcase //out_state
        end
    end

    //------------------------------------------------------------------
    //-- assign to settings bus interface
    //------------------------------------------------------------------
    reg strobe_reg;
    assign strobe = strobe_reg;
    assign data = out_data_reg;
    assign addr = out_hdr_reg[7:0];
    wire poke = out_hdr_reg[8];

    always @(posedge clock) begin
        if (reset || out_state != ACTION_EVENT) strobe_reg <= 0;
        else                                    strobe_reg <= poke;
    end

    //------------------------------------------------------------------
    //-- readback mux
    //------------------------------------------------------------------
    reg [31:0] rb_data;
    reg [63:0] rb_time;
    always @(posedge clock) begin
        if (out_state == ACTION_EVENT) begin
            rb_time <= vita_time;
            case (addr[3:0])
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

    //------------------------------------------------------------------
    //-- assign to output fifo interface
    //------------------------------------------------------------------
    wire [31:0] prot_hdr;
    assign prot_hdr[15:0] = 24; //bytes in proceeding vita packet
    assign prot_hdr[16] = 1; //yes frame
    assign prot_hdr[18:17] = PROT_DEST;
    assign prot_hdr[31:19] = 0; //nothing

    reg [31:0] out_data_int;
    always @* begin
        case (out_state)
            WRITE_PROT_HDR: out_data_int <= prot_hdr;
            WRITE_VRT_HDR:  out_data_int <= {12'b010100000001, out_hdr_reg[19:16], 16'd6};
            WRITE_VRT_SID:  out_data_int <= SID;
            WRITE_VRT_TSF0: out_data_int <= rb_time[63:32];
            WRITE_VRT_TSF1: out_data_int <= rb_time[31:0];
            WRITE_RB_HDR:   out_data_int <= out_hdr_reg;
            WRITE_RB_DATA:  out_data_int <= rb_data;
            default:        out_data_int <= 0;
        endcase //state
    end

    assign out_data[35:34] = 2'b0;
    assign out_data[33] = (out_state == WRITE_RB_DATA);
    assign out_data[32] = (out_state == WRITE_PROT_HDR);
    assign out_data[31:0] = out_data_int;

    //------------------------------------------------------------------
    //-- debug outputs
    //------------------------------------------------------------------
    assign debug = {
        in_state, out_state, //8
        in_valid, in_ready, in_data[33:32], //4
        out_valid, out_ready, out_data[33:32], //4
        in_command_valid, in_command_ready, //2
        out_command_valid, out_command_ready, //2
        addr, //8
        strobe_reg, strobe, poke, out_command_has_time //4
    };

endmodule //settings_readback_bus_fifo_ctrl

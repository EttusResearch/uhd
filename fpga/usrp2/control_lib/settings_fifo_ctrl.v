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

module settings_fifo_ctrl
    #(
        parameter XPORT_HDR = 1, //extra transport hdr line
        parameter PROT_DEST = 0, //protocol framer destination
        parameter PROT_HDR = 1, //needs a protocol header?
        parameter ACK_SID = 0 //stream ID for packet ACK
    )
    (
        //clock and synchronous reset for all interfaces
        input clock, input reset, input clear,

        //current system time
        input [63:0] vita_time,

        //ready signal for multiple peripherals
        input perfs_ready,

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
    wire command_fifo_full, command_fifo_empty;
    wire command_fifo_read, command_fifo_write;

    wire [128:0] fifo2_datain, fifo3_datain, fifo4_datain;
    wire fifo1_empty, fifo2_empty, fifo3_empty;
    wire fifo2_full, fifo3_full, fifo4_full;
    wire fifo1_to_fifo2, fifo2_to_fifo3, fifo3_to_fifo4;

    shortfifo #(.WIDTH(129)) command_fifo (
        .clk(clock), .rst(reset), .clear(clear),
        .datain({in_command_ticks, in_command_hdr, in_command_data, in_command_has_time}),
        .dataout(fifo2_datain),
        .write(command_fifo_write), .full(command_fifo_full), //input interface
        .empty(fifo1_empty), .read(fifo1_to_fifo2)  //output interface
    );

    shortfifo #(.WIDTH(129)) command_fifo2 (
        .clk(clock), .rst(reset), .clear(clear),
        .datain(fifo2_datain),
        .dataout(fifo3_datain),
        .write(fifo1_to_fifo2), .full(fifo2_full), //input interface
        .empty(fifo2_empty), .read(fifo2_to_fifo3)  //output interface
    );

    shortfifo #(.WIDTH(129)) command_fifo3 (
        .clk(clock), .rst(reset), .clear(clear),
        .datain(fifo3_datain),
        .dataout(fifo4_datain),
        .write(fifo2_to_fifo3), .full(fifo3_full), //input interface
        .empty(fifo3_empty), .read(fifo3_to_fifo4)  //output interface
    );

    shortfifo #(.WIDTH(129)) command_fifo4 (
        .clk(clock), .rst(reset), .clear(clear),
        .datain(fifo4_datain),
        .dataout({out_command_ticks, out_command_hdr, out_command_data, out_command_has_time}),
        .write(fifo3_to_fifo4), .full(fifo4_full), //input interface
        .empty(command_fifo_empty), .read(command_fifo_read)  //output interface
    );

    assign fifo1_to_fifo2 = ~fifo2_full & ~fifo1_empty;
    assign fifo2_to_fifo3 = ~fifo3_full & ~fifo2_empty;
    assign fifo3_to_fifo4 = ~fifo4_full & ~fifo3_empty;

    //------------------------------------------------------------------
    //-- The result fifo:
    //-- Stores an individual result of a command per line.
    //------------------------------------------------------------------
    wire [31:0] in_result_hdr, out_result_hdr;
    wire [31:0] in_result_data, out_result_data;
    wire result_fifo_full, result_fifo_empty;
    wire result_fifo_read, result_fifo_write;

    shortfifo #(.WIDTH(64)) result_fifo (
        .clk(clock), .rst(reset), .clear(clear),
        .datain({in_result_hdr, in_result_data}),
        .dataout({out_result_hdr, out_result_data}),
        .write(result_fifo_write), .full(result_fifo_full), //input interface
        .empty(result_fifo_empty), .read(result_fifo_read)  //output interface
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

    localparam START_STATE = (XPORT_HDR)? READ_LINE0 : VITA_HDR;

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
    assign command_fifo_write  = (in_state == STORE_CMD);
    assign in_command_ticks    = in_ticks_reg;
    assign in_command_data     = in_data_reg;
    assign in_command_hdr      = in_hdr_reg;
    assign in_command_has_time = has_tsf_reg;

    always @(posedge clock) begin
        if (reset) begin
            in_state <= START_STATE;
        end
        else begin
            case (in_state)

            READ_LINE0: begin
                if (reading) in_state <= VITA_HDR;
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
                if (~command_fifo_full) in_state <= START_STATE;
            end

            endcase //in_state
        end
    end

    //------------------------------------------------------------------
    //-- Command state machine:
    //-- Read a command fifo entry, act on it, produce result.
    //------------------------------------------------------------------
    localparam LOAD_CMD    = 0;
    localparam EVENT_CMD   = 1;

    reg cmd_state;
    reg [31:0] rb_data;

    reg [63:0] command_ticks_reg;
    reg [31:0] command_hdr_reg;
    reg [31:0] command_data_reg;

    reg [63:0] vita_time_reg;
    always @(posedge clock)
        vita_time_reg <= vita_time;

    wire late;
    `ifndef FIFO_CTRL_NO_TIME
    time_compare time_compare(
        .time_now(vita_time_reg), .trigger_time(command_ticks_reg), .late(late));
    `else
    assign late = 1;
    `endif

    //action occurs in the event state and when there is fifo space (should always be true)
    //the third condition is that all peripherals in the perfs signal are ready/active high
    //the fourth condition is that is an event time has been set, action is delayed until that time
    wire time_ready = (out_command_has_time)? late : 1;
    wire action = (cmd_state == EVENT_CMD) && ~result_fifo_full && perfs_ready && time_ready;

    assign command_fifo_read = action;
    assign result_fifo_write = action;
    assign in_result_hdr = command_hdr_reg;
    assign in_result_data = rb_data;

    always @(posedge clock) begin
        if (reset) begin
            cmd_state <= LOAD_CMD;
        end
        else begin
            case (cmd_state)

            LOAD_CMD: begin
                if (~command_fifo_empty) cmd_state <= EVENT_CMD;
                command_ticks_reg <= out_command_ticks;
                command_hdr_reg <= out_command_hdr;
                command_data_reg <= out_command_data;
            end

            EVENT_CMD: begin // poking and peeking happens here!
                if (action || clear) cmd_state <= LOAD_CMD;
            end

            endcase //cmd_state
        end
    end

    //------------------------------------------------------------------
    //-- assign to settings bus interface
    //------------------------------------------------------------------
    reg strobe_reg;
    assign strobe = strobe_reg;
    assign data = command_data_reg;
    assign addr = command_hdr_reg[7:0];
    wire poke = command_hdr_reg[8];

    always @(posedge clock) begin
        if (reset || clear) strobe_reg <= 0;
        else                strobe_reg <= action && poke;
    end

    //------------------------------------------------------------------
    //-- readback mux
    //------------------------------------------------------------------
    always @(posedge clock) begin
        case (out_command_hdr[3:0])
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

    //------------------------------------------------------------------
    //-- Output state machine:
    //-- Read a command fifo entry, act on it, produce ack packet.
    //------------------------------------------------------------------
    localparam WRITE_PROT_HDR = 0;
    localparam WRITE_VRT_HDR  = 1;
    localparam WRITE_VRT_SID  = 2;
    localparam WRITE_RB_HDR   = 3;
    localparam WRITE_RB_DATA  = 4;

    //the state for the start of packet condition
    localparam WRITE_PKT_HDR = (PROT_HDR)? WRITE_PROT_HDR : WRITE_VRT_HDR;

    reg [2:0] out_state;

    assign out_valid = ~result_fifo_empty;
    assign result_fifo_read = out_data[33] && writing;

    always @(posedge clock) begin
        if (reset) begin
            out_state <= WRITE_PKT_HDR;
        end
        else if (writing && out_data[33]) begin
            out_state <= WRITE_PKT_HDR;
        end
        else if (writing) begin
            out_state <= out_state + 1;
        end
    end

    //------------------------------------------------------------------
    //-- assign to output fifo interface
    //------------------------------------------------------------------
    wire [31:0] prot_hdr;
    assign prot_hdr[15:0] = 16; //bytes in proceeding vita packet
    assign prot_hdr[16] = 1; //yes frame
    assign prot_hdr[18:17] = PROT_DEST;
    assign prot_hdr[31:19] = 0; //nothing

    reg [31:0] out_data_int;
    always @* begin
        case (out_state)
            WRITE_PROT_HDR: out_data_int <= prot_hdr;
            WRITE_VRT_HDR:  out_data_int <= {12'b010100000000, out_result_hdr[19:16], 2'b0, prot_hdr[15:2]};
            WRITE_VRT_SID:  out_data_int <= ACK_SID;
            WRITE_RB_HDR:   out_data_int <= out_result_hdr;
            WRITE_RB_DATA:  out_data_int <= out_result_data;
            default:        out_data_int <= 0;
        endcase //state
    end

    assign out_data[35:34] = 2'b0;
    assign out_data[33] = (out_state == WRITE_RB_DATA);
    assign out_data[32] = (out_state == WRITE_PKT_HDR);
    assign out_data[31:0] = out_data_int;

    //------------------------------------------------------------------
    //-- debug outputs
    //------------------------------------------------------------------
    assign debug = {
        in_state, out_state, //8
        in_valid, in_ready, in_data[33:32], //4
        out_valid, out_ready, out_data[33:32], //4
        command_fifo_empty, command_fifo_full, //2
        command_fifo_read, command_fifo_write, //2
        addr, //8
        strobe_reg, strobe, poke, out_command_has_time //4
    };

endmodule //settings_fifo_ctrl

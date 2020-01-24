//
// Copyright 2011-2012 Ettus Research LLC
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

// PAD to NUM LINES

module resp_packet_padder36
#(
    parameter NUM_LINES32 = 128
)
(
    input clk, input reset,

    //input interface
    input [35:0] data_i,
    input src_rdy_i,
    output dst_rdy_o,

    //output interface
    output [35:0] data_o,
    output src_rdy_o,
    input dst_rdy_i
);

    localparam STATE_FWD = 0;
    localparam STATE_PAD = 1;
    reg state;

    reg [15:0] counter;

    always @(posedge clk) begin
        if (reset) begin
            counter <= 0;
        end
        else if (src_rdy_o && dst_rdy_i) begin
            if (data_o[33]) counter <= 0;
            else counter <= counter + 1;
        end
    end

    always @(posedge clk) begin
        if (reset) begin
            state <= STATE_FWD;
        end
        else case(state)

        STATE_FWD: begin
            if (src_rdy_i && dst_rdy_o && data_i[33] && ~data_o[33]) begin
                state <= STATE_PAD;
            end
        end

        STATE_PAD: begin
            if (src_rdy_o && dst_rdy_i && data_o[33]) begin
                state <= STATE_FWD;
            end
        end

        endcase //state
    end

    //assign data out
    assign data_o[31:0] = (state == STATE_FWD)? data_i[31:0] : {32'b0};
    wire eof = (counter == (NUM_LINES32-1));
    assign data_o[35:32] = {data_i[35:34], eof, data_i[32]};

    //assign ready
    assign src_rdy_o = (state == STATE_FWD)? src_rdy_i : 1'b1;
    assign dst_rdy_o = (state == STATE_FWD)? dst_rdy_i : 1'b0;

endmodule // resp_packet_padder36





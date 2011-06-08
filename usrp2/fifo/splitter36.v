//
// Copyright 2011 Ettus Research LLC
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


// Split packets from a fifo based interface so it goes out identically on two interfaces

module splitter36
    (
        input clk, input rst, input clr,
        input [35:0] inp_data, input inp_valid, output inp_ready,
        output [35:0] out0_data, output out0_valid, input out0_ready,
        output [35:0] out1_data, output out1_valid, input out1_ready
    );

    localparam STATE_COPY_BOTH = 0;
    localparam STATE_COPY_ZERO = 1;
    localparam STATE_COPY_ONE = 2;

    reg [1:0] state;
    reg [35:0] data_reg;

    assign out0_data = (state == STATE_COPY_BOTH)? inp_data : data_reg;
    assign out1_data = (state == STATE_COPY_BOTH)? inp_data : data_reg;

    assign out0_valid =
        (state == STATE_COPY_BOTH)? inp_valid : (
        (state == STATE_COPY_ZERO)? 1'b1      : (
    1'b0));

    assign out1_valid =
        (state == STATE_COPY_BOTH)? inp_valid : (
        (state == STATE_COPY_ONE)?  1'b1      : (
    1'b0));

    assign inp_ready = (state == STATE_COPY_BOTH)? (out0_ready | out1_ready) : 1'b0;

    always @(posedge clk)
    if (rst | clr) begin
        state <= STATE_COPY_BOTH;
    end
    else begin
        case (state)

        STATE_COPY_BOTH: begin
            if ((out0_valid & out0_ready) & ~(out1_valid & out1_ready)) begin
                state <= STATE_COPY_ONE;
            end
            else if (~(out0_valid & out0_ready) & (out1_valid & out1_ready)) begin
                state <= STATE_COPY_ZERO;
            end
            data_reg <= inp_data;
        end

        STATE_COPY_ZERO: begin
            if (out0_valid & out0_ready) begin
                state <= STATE_COPY_BOTH;
            end
        end

        STATE_COPY_ONE: begin
            if (out1_valid & out1_ready) begin
                state <= STATE_COPY_BOTH;
            end
        end

        endcase //state
    end



endmodule //splitter36

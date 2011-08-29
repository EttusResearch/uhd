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

module cross_clock_reader
    #(
        parameter WIDTH = 1,
        parameter DEFAULT = 0
    )
    (
        input clk, input rst,
        input [WIDTH-1:0] in,
        output reg [WIDTH-1:0] out
    );

    reg [WIDTH-1:0] shadow;

    always @(posedge clk) begin
        if (rst) begin
            out <= DEFAULT;
            shadow <= DEFAULT;
        end
        else if (shadow == in) begin
            out <= shadow;
        end
        shadow <= in;
    end

endmodule //cross_clock_reader

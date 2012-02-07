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


// 64 bits worth of ticks

module time_compare
  (input [63:0] time_now,
   input [63:0] trigger_time,
   output now,
   output early,
   output late,
   output too_early);

    assign now = time_now == trigger_time;
    assign late = time_now > trigger_time;
    assign early = ~now & ~late;
    assign too_early = 0; //not implemented

endmodule // time_compare

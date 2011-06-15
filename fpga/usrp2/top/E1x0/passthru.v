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

`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////

module passthru
  (input overo_gpio145,
   output cgen_sclk,
   output cgen_sen_b,
   output cgen_mosi,
   input fpga_cfg_din,
   input fpga_cfg_cclk
   );
   
   assign cgen_sclk = fpga_cfg_cclk;
   assign cgen_sen_b = overo_gpio145;
   assign cgen_mosi = fpga_cfg_din;
   
   
endmodule // passthru

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

//COPY ME, CUSTOMIZE ME...

//The following module effects the IO of the DUC chain.
//By default, this entire module is a simple pass-through.

//To implement DSP logic before the DUC:
//Implement custom DSP between baseband and duc input.

//To implement DSP logic after the DUC:
//Implement custom DSP between duc output and frontend.

//To bypass the DUC with custom logic:
//Implement custom DSP between baseband and frontend.

module custom_dsp_tx
#(
    //frontend bus width
    parameter WIDTH = 24
)
(
    //control signals
    input clock, //dsp clock
    input reset, //active high synchronous reset
    input clear, //active high on packet control init
    input enable, //active high when streaming enabled

    //user settings bus, controlled through user setting regs API
    input set_stb, input [7:0] set_addr, input [31:0] set_data,

    //full rate outputs directly to the TX frontend
    output [WIDTH-1:0] frontend_i,
    output [WIDTH-1:0] frontend_q,

    //full rate outputs directly from the DUC chain
    input [WIDTH-1:0] duc_out_i,
    input [WIDTH-1:0] duc_out_q,

    //strobed samples {I16,Q16} to the TX DUC chain
    output [31:0] duc_in_sample,
    input duc_in_strobe, //this is a backpressure signal
    output duc_in_enable, //enables DUC module

    //strobbed baseband samples {I16,Q16} to this module
    input [31:0] bb_sample,
    output bb_strobe //this is a backpressure signal
);

    assign frontend_i = duc_out_i;
    assign frontend_q = duc_out_q;
    assign duc_in_sample = bb_sample;
    assign bb_strobe = duc_in_strobe;
    assign duc_in_enable = enable;

endmodule //custom_dsp_tx

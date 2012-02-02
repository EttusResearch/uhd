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

//CUSTOMIZE ME!

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
    //the dsp unit number: 0, 1, 2...
    parameter DSPNO = 0,

    //frontend bus width
    parameter WIDTH = 24
)
(
    //control signals
    input clock, input reset, input enable,

    //main settings bus for built-in modules
    input set_stb_main, input [7:0] set_addr_main, input [31:0] set_data_main,

    //user settings bus, controlled through user setting regs API
    input set_stb_user, input [7:0] set_addr_user, input [31:0] set_data_user,

    //full rate outputs directly to the TX frontend
    output [WIDTH-1:0] frontend_i,
    output [WIDTH-1:0] frontend_q,

    //full rate outputs directly from the DUC chain
    input [WIDTH-1:0] duc_out_i,
    input [WIDTH-1:0] duc_out_q,

    //strobed samples {I16,Q16} to the TX DUC chain
    output [31:0] duc_in_sample,
    input duc_in_strobe, //this is a backpressure signal

    //strobbed baseband samples {I16,Q16} to this module
    input [31:0] bb_sample,
    output bb_strobe, //this is a backpressure signal

    //debug output (optional)
    output [31:0] debug
);

    generate
        if (DSPNO==0) begin
            `ifndef TX_DSP0_MODULE
            assign frontend_i = duc_out_i;
            assign frontend_q = duc_out_q;
            assign duc_in_sample = bb_sample;
            assign bb_strobe = duc_in_strobe;
            `else
            TX_DSP0_CUSTOM_MODULE_NAME tx_dsp0_custom
            (
                .clock(clock), .reset(reset), .enable(enable),
                .set_stb(set_stb), .set_addr(set_addr), .set_data(set_data),
                .frontend_i(frontend_i), .frontend_q(frontend_q),
                .duc_out_i(duc_out_i), .duc_out_q(duc_out_q),
                .duc_in_sample(duc_in_sample), .duc_in_strobe(duc_in_strobe),
                .bb_sample(bb_sample), .bb_strobe(bb_strobe)
            );
            `endif
        end
        else begin
            `ifndef TX_DSP1_MODULE
            assign frontend_i = duc_out_i;
            assign frontend_q = duc_out_q;
            assign duc_in_sample = bb_sample;
            assign bb_strobe = duc_in_strobe;
            `else
            TX_DSP1_CUSTOM_MODULE_NAME tx_dsp1_custom
            (
                .clock(clock), .reset(reset), .enable(enable),
                .set_stb(set_stb), .set_addr(set_addr), .set_data(set_data),
                .frontend_i(frontend_i), .frontend_q(frontend_q),
                .duc_out_i(duc_out_i), .duc_out_q(duc_out_q),
                .duc_in_sample(duc_in_sample), .duc_in_strobe(duc_in_strobe),
                .bb_sample(bb_sample), .bb_strobe(bb_strobe)
            );
            `endif
        end
    endgenerate

endmodule //custom_dsp_tx

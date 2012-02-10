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

//The following module effects the IO of the DDC chain.
//By default, this entire module is a simple pass-through.

module dsp_rx_glue
#(
    //the dsp unit number: 0, 1, 2...
    parameter DSPNO = 0,

    //frontend bus width
    parameter WIDTH = 24
)
(
    //control signals
    input clock, input reset, input clear, input enable,

    //user settings bus, controlled through user setting regs API
    input set_stb, input [7:0] set_addr, input [31:0] set_data,

    //full rate inputs directly from the RX frontend
    input [WIDTH-1:0] frontend_i,
    input [WIDTH-1:0] frontend_q,

    //full rate outputs directly to the DDC chain
    output [WIDTH-1:0] ddc_in_i,
    output [WIDTH-1:0] ddc_in_q,

    //strobed samples {I16,Q16} from the RX DDC chain
    input [31:0] ddc_out_sample,
    input ddc_out_strobe, //high on valid sample
    output ddc_out_enable, //enables DDC module

    //strobbed baseband samples {I16,Q16} from this module
    output [31:0] bb_sample,
    output bb_strobe, //high on valid sample

    //debug output (optional)
    output [31:0] debug
);

    generate
        if (DSPNO==0) begin
            `ifndef RX_DSP0_MODULE
            assign ddc_in_i = frontend_i;
            assign ddc_in_q = frontend_q;
            assign bb_sample = ddc_out_sample;
            assign bb_strobe = ddc_out_strobe;
            assign ddc_out_enable = enable;
            `else
            `RX_DSP0_MODULE #(.WIDTH(WIDTH)) rx_dsp0_custom
            (
                .clock(clock), .reset(reset), .clear(clear), .enable(enable),
                .set_stb(set_stb), .set_addr(set_addr), .set_data(set_data),
                .frontend_i(frontend_i), .frontend_q(frontend_q),
                .ddc_in_i(ddc_in_i), .ddc_in_q(ddc_in_q),
                .ddc_out_sample(ddc_out_sample), .ddc_out_strobe(ddc_out_strobe), .ddc_out_enable(ddc_out_enable),
                .bb_sample(bb_sample), .bb_strobe(bb_strobe)
            );
            `endif
        end
        else begin
            `ifndef RX_DSP1_MODULE
            assign ddc_in_i = frontend_i;
            assign ddc_in_q = frontend_q;
            assign bb_sample = ddc_out_sample;
            assign bb_strobe = ddc_out_strobe;
            assign ddc_out_enable = enable;
            `else
            `RX_DSP1_MODULE #(.WIDTH(WIDTH)) rx_dsp1_custom
            (
                .clock(clock), .reset(reset), .clear(clear), .enable(enable),
                .set_stb(set_stb), .set_addr(set_addr), .set_data(set_data),
                .frontend_i(frontend_i), .frontend_q(frontend_q),
                .ddc_in_i(ddc_in_i), .ddc_in_q(ddc_in_q),
                .ddc_out_sample(ddc_out_sample), .ddc_out_strobe(ddc_out_strobe), .ddc_out_enable(ddc_out_enable),
                .bb_sample(bb_sample), .bb_strobe(bb_strobe)
            );
            `endif
        end
    endgenerate

endmodule //dsp_rx_glue

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


module vita_rx_chain
  #(parameter BASE=0,
    parameter UNIT=0,
    parameter FIFOSIZE=10,
    parameter PROT_ENG_FLAGS=1,
    parameter DSP_NUMBER=0)
   (input clk, input reset,
    input set_stb, input [7:0] set_addr, input [31:0] set_data,
    input set_stb_user, input [7:0] set_addr_user, input [31:0] set_data_user,
    input [63:0] vita_time,
    input [31:0] sample, input strobe,
    output [35:0] rx_data_o, output rx_src_rdy_o, input rx_dst_rdy_i,
    output overrun, output run, output clear_o,
    output [31:0] debug );

   wire [100:0] sample_data;
   wire 	sample_dst_rdy, sample_src_rdy;
   wire [31:0] 	vrc_debug, vrf_debug;

   wire [35:0] 	rx_data_int;
   wire 	rx_src_rdy_int, rx_dst_rdy_int;

   wire clear;
   assign clear_o = clear;
   wire clear_int;
   setting_reg #(.my_addr(BASE+8)) sr
     (.clk(clk),.rst(reset),.strobe(set_stb),.addr(set_addr),
      .in(set_data),.out(),.changed(clear_int));

   vita_rx_control #(.BASE(BASE), .WIDTH(32)) vita_rx_control
     (.clk(clk), .reset(reset), .clear(clear),
      .set_stb(set_stb),.set_addr(set_addr),.set_data(set_data),
      .vita_time(vita_time), .overrun(overrun),
      .sample(sample), .run(run), .strobe(strobe),
      .sample_fifo_o(sample_data), .sample_fifo_dst_rdy_i(sample_dst_rdy), .sample_fifo_src_rdy_o(sample_src_rdy),
      .debug_rx(vrc_debug));
   
   vita_rx_framer #(.BASE(BASE), .MAXCHAN(1)) vita_rx_framer
     (.clk(clk), .reset(reset), .clear(clear),
      .set_stb(set_stb),.set_addr(set_addr),.set_data(set_data),
      .sample_fifo_i(sample_data), .sample_fifo_dst_rdy_o(sample_dst_rdy), .sample_fifo_src_rdy_i(sample_src_rdy),
      .data_o(rx_data_int), .src_rdy_o(rx_src_rdy_int), .dst_rdy_i(rx_dst_rdy_int),
      .debug_rx(vrf_debug) );

   wire [FIFOSIZE-1:0] access_adr, access_len;
   wire 	       access_we, access_stb, access_ok, access_done, access_skip_read;
   wire [35:0] 	       dsp_to_buf, buf_to_dsp;
   wire [35:0] 	       rx_data_int2;
   wire 	       rx_src_rdy_int2, rx_dst_rdy_int2;
   
   double_buffer #(.BUF_SIZE(FIFOSIZE)) db
     (.clk(clk),.reset(reset),.clear(clear),
      .access_we(access_we), .access_stb(access_stb), .access_ok(access_ok), .access_done(access_done), 
      .access_skip_read(access_skip_read), .access_adr(access_adr), .access_len(access_len), 
      .access_dat_i(dsp_to_buf), .access_dat_o(buf_to_dsp),

      .data_i(rx_data_int), .src_rdy_i(rx_src_rdy_int), .dst_rdy_o(rx_dst_rdy_int),
      .data_o(rx_data_int2), .src_rdy_o(rx_src_rdy_int2), .dst_rdy_i(rx_dst_rdy_int2));

   vita_rx_engine_glue #(.DSPNO(DSP_NUMBER), .MAIN_SETTINGS_BASE(BASE+3), .BUF_SIZE(FIFOSIZE)) dspengine_rx
     (.clock(clk),.reset(reset),.clear(clear),
      .set_stb_main(set_stb), .set_addr_main(set_addr), .set_data_main(set_data),
      .set_stb_user(set_stb_user), .set_addr_user(set_addr_user), .set_data_user(set_data_user),
      .access_we(access_we), .access_stb(access_stb), .access_ok(access_ok), .access_done(access_done), 
      .access_skip_read(access_skip_read), .access_adr(access_adr), .access_len(access_len), 
      .access_dat_i(buf_to_dsp), .access_dat_o(dsp_to_buf));
   
   add_routing_header #(.PORT_SEL(UNIT), 
			.PROT_ENG_FLAGS(PROT_ENG_FLAGS)) dsp_routing_header
     (.clk(clk), .reset(reset), .clear(clear),
      .data_i(rx_data_int2), .src_rdy_i(rx_src_rdy_int2), .dst_rdy_o(rx_dst_rdy_int2),
      .data_o(rx_data_o), .src_rdy_o(rx_src_rdy_o), .dst_rdy_i(rx_dst_rdy_i) );

    //only clear once a full packet has passed through the output interface
    reg xfer_pkt, clear_oneshot;
    assign clear = (clear_oneshot)? ~xfer_pkt : 0;
    always @(posedge clk) begin

        if (reset || clear) begin
            clear_oneshot <= 0;
        end
        else if (clear_int) begin
            clear_oneshot <= 1;
        end

        if (reset || clear) begin
            xfer_pkt <= 0;
        end
        else if (rx_src_rdy_o && rx_dst_rdy_i) begin
            xfer_pkt <= ~rx_data_o[33];
        end
    end

   assign debug = vrc_debug; //  | vrf_debug;
   
endmodule // vita_rx_chain

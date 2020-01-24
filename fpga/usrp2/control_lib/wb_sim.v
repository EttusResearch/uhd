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



module wb_sim();
   
   wire wb_clk, wb_rst;
   wire start;

   reg 	POR, aux_clk, clk_fpga;
   
   initial POR = 1'b1;
   initial #103 POR = 1'b0;

   initial aux_clk = 1'b0;
   always #25 aux_clk = ~aux_clk;

   initial clk_fpga = 1'bx;
   initial #3007 clk_fpga = 1'b0;
   always #7 clk_fpga = ~clk_fpga;
      
   initial begin
      $dumpfile("wb_sim.vcd");
      $dumpvars(0,wb_sim);
   end

   initial #10000 $finish;

   wire [15:0] rom_addr;
   wire [47:0] rom_data;
   wire [31:0] wb_dat;
   wire [15:0] wb_adr;
   wire        wb_cyc,wb_stb,wb_we,wb_ack;
   wire [3:0]  wb_sel;
   
   wire [31:0] port_output;


   system_control system_control(.dsp_clk(dsp_clk),
				 .reset_out(reset_out),
				 .wb_clk_o(wb_clk),
				 .wb_rst_o(wb_rst),
				 .wb_rst_o_alt(wb_rst_o_alt),
				 .start	(start),
				 .aux_clk(aux_clk),
				 .clk_fpga(clk_fpga),
				 .POR	(POR),
				 .done	(done));
   
   clock_bootstrap_rom cbrom(.addr(rom_addr),.data(rom_data));

   wb_bus_writer bus_writer(.rom_addr	(rom_addr[15:0]),
			    .wb_dat_o	(wb_dat[31:0]),
			    .wb_adr_o	(wb_adr[15:0]),
			    .wb_cyc_o	(wb_cyc),
			    .wb_sel_o	(wb_sel[3:0]),
			    .wb_stb_o	(wb_stb),
			    .wb_we_o	(wb_we),
			    .start	(start),
			    .done       (done),
			    .rom_data	(rom_data[47:0]),
			    .wb_clk_i	(wb_clk),
			    .wb_rst_i	(wb_rst),
			    .wb_ack_i	(wb_ack));

   wb_output_pins32 output_pins(.wb_dat_o(),
				.wb_ack_o(wb_ack),
				.port_output(port_output[31:0]),
				.wb_rst_i(wb_rst),
				.wb_clk_i(wb_clk),
				.wb_dat_i(wb_dat[31:0]),
				.wb_we_i(wb_we),
				.wb_sel_i(wb_sel[3:0]),
				.wb_stb_i(wb_stb),
				.wb_cyc_i(wb_cyc));
   
   
   
   
endmodule // wb_sim


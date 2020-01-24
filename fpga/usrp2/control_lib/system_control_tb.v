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



module system_control_tb();
   
   reg 	aux_clk, clk_fpga;
   wire wb_clk, dsp_clk;
   wire wb_rst, dsp_rst, rl_rst, proc_rst;

   reg 	rl_done, clock_ready;
   
   initial aux_clk = 1'b0;
   always #25 aux_clk = ~aux_clk;

   initial clk_fpga = 1'b0;

   initial clock_ready = 1'b0;
   initial
     begin
	@(negedge proc_rst);
	#1003 clock_ready <= 1'b1;
     end

   always #7 clk_fpga = ~clk_fpga;
      
   initial begin
      $dumpfile("system_control_tb.vcd");
      $dumpvars(0,system_control_tb);
   end

   initial #10000 $finish;

   initial
     begin
	@(negedge rl_rst);
	rl_done <= 1'b0;
	#1325 rl_done <= 1'b1;
     end

   initial
     begin
	@(negedge proc_rst);
	clock_ready <= 1'b0;
	#327 clock_ready <= 1'b1;
     end
     
   system_control 
     system_control(.aux_clk_i(aux_clk),.clk_fpga_i(clk_fpga),
		    .dsp_clk_o(dsp_clk),.wb_clk_o(wb_clk),
		    .ram_loader_rst_o(rl_rst),
		    .processor_rst_o(proc_rst),
		    .wb_rst_o(wb_rst),
		    .dsp_rst_o(dsp_rst),
		    .ram_loader_done_i(rl_done),
		    .clock_ready_i(clock_ready),
		    .debug_o());
   
endmodule // system_control_tb

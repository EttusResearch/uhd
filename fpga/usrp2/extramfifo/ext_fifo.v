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

//
// FIFO backed by an off chip ZBT/NoBL SRAM.
//
// This module and its sub-hierarchy implment a FIFO capable of sustaining 
// a data throughput rate of at least int_clk/2 * 36bits and bursts of int_clk * 36bits.
//
// This has been designed and tested for an int_clk of 100MHz and an ext_clk of 125MHz,
// your milage may vary with other clock ratio's especially those where int_clk < ext_clk.
// Testing has also exclusively used a rst signal synchronized to int_clk.
//
// Interface operation mimics a Xilinx FIFO configured as "First Word Fall Through",
// though signal naming differs.
//
// For FPGA use registers interfacing directly with signals prefixed "RAM_*" should be 
// packed into the IO ring.
//

 //`define NO_EXT_FIFO

module ext_fifo
  #(parameter INT_WIDTH=36,EXT_WIDTH=18,RAM_DEPTH=19,FIFO_DEPTH=19)
    (
     input int_clk,
     input ext_clk,
     input rst,
     input [EXT_WIDTH-1:0] RAM_D_pi,
     output [EXT_WIDTH-1:0] RAM_D_po,
     output RAM_D_poe,
     output [RAM_DEPTH-1:0] RAM_A,
     output RAM_WEn,
     output RAM_CENn,
     output RAM_LDn,
     output RAM_OEn,
     output RAM_CE1n,
     input [INT_WIDTH-1:0] datain,
     input src_rdy_i,                // WRITE
     output dst_rdy_o,               // not FULL
     output [INT_WIDTH-1:0] dataout,
     output src_rdy_o,               // not EMPTY
     input dst_rdy_i,                 // READ
     output reg [31:0] debug,
     output reg [31:0] debug2
     );

   wire [EXT_WIDTH-1:0] write_data;
   wire [EXT_WIDTH-1:0] read_data;
   wire 		full1, empty1;
   wire 		almost_full2, almost_full2_spread, full2, empty2;
   wire [FIFO_DEPTH-1:0] capacity;
   wire 		 space_avail;
   wire 		 data_avail;
		 
   // These next 2 lines here purely because ICARUS is crap at handling generate statements.
   // Empirically this has been determined to make simulations work.
   wire 		 read_input_fifo = space_avail & ~empty1;
   wire 		 write_output_fifo = data_avail;
   
   assign 		 src_rdy_o = ~empty2;
   assign 		 dst_rdy_o = ~full1;

`ifdef NO_EXT_FIFO
   assign 		 space_avail = ~full2;
   assign 		 data_avail = ~empty1;
   assign 		 read_data = write_data;
`else
   
   // External FIFO running at ext clock rate  and 18 or 36 bit width.
   nobl_fifo  #(.WIDTH(EXT_WIDTH),.RAM_DEPTH(RAM_DEPTH),.FIFO_DEPTH(FIFO_DEPTH))
     nobl_fifo_i1
       (   
	   .clk(ext_clk),
	   .rst(rst),
	   .RAM_D_pi(RAM_D_pi),
	   .RAM_D_po(RAM_D_po),
	   .RAM_D_poe(RAM_D_poe),
	   .RAM_A(RAM_A),
	   .RAM_WEn(RAM_WEn),
	   .RAM_CENn(RAM_CENn),
	   .RAM_LDn(RAM_LDn),
	   .RAM_OEn(RAM_OEn),
	   .RAM_CE1n(RAM_CE1n),
	   .write_data(write_data),
	   .write_strobe(~empty1 ),
	   .space_avail(space_avail),
	   .read_data(read_data),
	   .read_strobe(~almost_full2_spread),
	   .data_avail(data_avail),
	   .capacity(capacity)
	   );
`endif // !`ifdef NO_EXT_FIFO

   
   generate
      if (EXT_WIDTH == 18 && INT_WIDTH == 36) begin: fifo_g1
	 // FIFO buffers data from UDP engine into external FIFO clock domain.
	 fifo_xlnx_512x36_2clk_36to18 fifo_xlnx_512x36_2clk_36to18_i1 (
								       .rst(rst),
								       .wr_clk(int_clk),
								       .rd_clk(ext_clk),
								       .din(datain), // Bus [35 : 0] 
								       .wr_en(src_rdy_i), 
								       .rd_en(read_input_fifo),    		
								       .dout(write_data), // Bus [17 : 0]
								       .full(full1),			
							               .empty(empty1));

	 
	 // FIFO buffers data read from external FIFO into DSP clk domain and to TX DSP.
	 fifo_xlnx_512x36_2clk_18to36 fifo_xlnx_512x36_2clk_18to36_i1 (
								       .rst(rst),
								       .wr_clk(ext_clk),
								       .rd_clk(int_clk),
								       .din(read_data), // Bus [17 : 0]
								       .wr_en(write_output_fifo),
								       .rd_en(dst_rdy_i),
								       .dout(dataout), // Bus [35 : 0]
								       .full(full2),
								       .prog_full(almost_full2),
								       .empty(empty2));
      end // block: fifo_g1
      else if (EXT_WIDTH == 36 && INT_WIDTH == 36) begin: fifo_g1
	 // FIFO buffers data from UDP engine into external FIFO clock domain.
	 fifo_xlnx_32x36_2clk fifo_xlnx_32x36_2clk_i1 (
						       .rst(rst),
						       .wr_clk(int_clk),
						       .rd_clk(ext_clk),
						       .din(datain), // Bus [35 : 0] 
						       .wr_en(src_rdy_i),
						       .rd_en(read_input_fifo),
						       .dout(write_data), // Bus [35 : 0] 
						       .full(full1),
						       .empty(empty1));
	 
	 // FIFO buffers data read from external FIFO into DSP clk domain and to TX DSP.
	 fifo_xlnx_512x36_2clk_prog_full fifo_xlnx_32x36_2clk_prog_full_i1 (
									    .rst(rst),
									    .wr_clk(ext_clk),
									    .rd_clk(int_clk),
									    .din(read_data), // Bus [35 : 0] 
									    .wr_en(write_output_fifo),
									    .rd_en(dst_rdy_i),
									    .dout(dataout), // Bus [35 : 0] 
									    .full(full2),
									    .empty(empty2),
									    .prog_full(almost_full2));

      end    
   endgenerate
   

   refill_randomizer #(.BITS(7))
     refill_randomizer_i1 (
			   .clk(ext_clk),
			   .rst(rst),
			   .full_in(almost_full2),
			   .full_out(almost_full2_spread)
			   );
   
//   always @ (posedge int_clk)
//     debug[31:28] <= {empty2,full1,dst_rdy_i,src_rdy_i };
   
   always @ (posedge ext_clk)
 //    debug[27:0] <= {RAM_WEn,RAM_CE1n,RAM_A[3:0],read_data[17:0],empty1,space_avail,data_avail,almost_full2 };
     debug[31:0] <= {7'h0,src_rdy_i,read_input_fifo,write_output_fifo,dst_rdy_i,full2,almost_full2,empty2,full1,empty1,write_data[7:0],read_data[7:0]};
   
     
   always@ (posedge ext_clk)
     //     debug2[31:0] <= {write_data[15:0],read_data[15:0]};
     debug2[31:0] <= 0;
   
endmodule // ext_fifo

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

module ext_fifo
   #(parameter INT_WIDTH=36,EXT_WIDTH=18,DEPTH=19)
    (
     input int_clk,
     input ext_clk,
     input rst,
     input [EXT_WIDTH-1:0] RAM_D_pi,
     output [EXT_WIDTH-1:0] RAM_D_po,
     output RAM_D_poe,
     output [DEPTH-1:0] RAM_A,
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
     input dst_rdy_i                 // READ
     );

   wire [EXT_WIDTH-1:0] write_data;
   wire [EXT_WIDTH-1:0] read_data;
   wire 	    full1, empty1;
   wire 	    almost_full2, full2, empty2;
   wire [INT_WIDTH-1:0] data_to_fifo;
   wire [INT_WIDTH-1:0] data_from_fifo;

   
   // FIFO buffers data from UDP engine into external FIFO clock domain.
   fifo_xlnx_512x36_2clk_36to18 fifo_xlnx_512x36_2clk_36to18_i1 (
								 .rst(rst),
								 .wr_clk(int_clk),
								 .rd_clk(ext_clk),
								 .din(datain), // Bus [35 : 0]						
								 .wr_en(src_rdy_i),						
								 .rd_en(space_avail&~empty1),						
								 .dout(write_data), // Bus [17 : 0] 
								 .full(full1),			
							         .empty(empty1));

    assign 	    dst_rdy_o = ~full1;
   
/* -----\/----- EXCLUDED -----\/-----
   assign 	    space_avail = ~full2;
   assign 	    data_avail = ~empty1;
   assign 	    read_data = write_data;
 -----/\----- EXCLUDED -----/\----- */
   

   // External FIFO running at ext clock rate  and 18 bit width.
   nobl_fifo  #(.WIDTH(EXT_WIDTH),.DEPTH(DEPTH),.FDEPTH(DEPTH))
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
	   .write_strobe(space_avail & ~empty1 ),
	   .space_avail(space_avail),
	   .read_data(read_data),
	   .read_strobe(data_avail & ~full2),
	   .data_avail(data_avail),
	   .upstream_full(almost_full2)
	   );
    
 
   // FIFO buffers data read from external FIFO into DSP clk domain and to TX DSP.
   fifo_xlnx_512x36_2clk_18to36 fifo_xlnx_512x36_2clk_18to36_i1 (
								 .rst(rst),
								 .wr_clk(ext_clk),
								 .rd_clk(int_clk),
								 .din(read_data), // Bus [17 : 0]
								 .wr_en(data_avail & ~full2  ),
								 .rd_en(dst_rdy_i),
								 .dout(dataout), // Bus [35 : 0]
								 .full(full2),
								 .almost_full(),
								 .prog_full(almost_full2),
								 .empty(empty2));
   assign  src_rdy_o = ~empty2;

   
endmodule // ext_fifo

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
   wire 	    full2, empty2;
  

   // FIFO buffers data from UDP engine into external FIFO clock domain.
   fifo_xlnx_512x36_2clk_36to18 fifo_xlnx_512x36_2clk_36to18_i1 (
								 .rst(rst),
								 .wr_clk(int_clk),
								 .rd_clk(ext_clk),
								 .din(datain), // Bus [35 : 0] 
								 .wr_en(src_rdy_i),
								 .rd_en(space_avail&~empty1),
							//	 .rd_en(~full2&~empty1),
								 .dout(write_data), // Bus [17 : 0] 
								 .full(full1),			
							         .empty(empty1));
   assign 	    dst_rdy_o = ~full1;

   

   // External FIFO running at ext clock rate  and 18 bit width.
   nobl_fifo  #(.WIDTH(EXT_WIDTH),.DEPTH(DEPTH))
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
	   .data_avail(data_avail)
	   );
 
   
 
   // FIFO buffers data read from external FIFO into DSP clk domain and to TX DSP.
   fifo_xlnx_512x36_2clk_18to36 fifo_xlnx_512x36_2clk_18to36_i1 (
								 .rst(rst),
								 .wr_clk(ext_clk),
								 .rd_clk(int_clk),
								 .din(read_data), // Bus [17 : 0]
							//	 .din(write_data), // Bus [17 : 0]
								 .wr_en(data_avail & ~full2  ),
							//	 .wr_en(~full2&~empty1),
								 .rd_en(dst_rdy_i),
								 .dout(dataout), // Bus [35 : 0] 
								 .full(full2),
								 .empty(empty2));
   assign  src_rdy_o = ~empty2;

   

   wire [35:0] CONTROL0;
   reg [7:0]   datain_reg,write_data_reg,read_data_reg ;
   reg 	    space_avail_reg,data_avail_reg,empty1_reg,full2_reg   ;

   always @(posedge ext_clk)
     begin
	//datain_reg <= datain[7:0];
	write_data_reg <= write_data[7:0];
	read_data_reg <= read_data[7:0];
	space_avail_reg <= space_avail;
	data_avail_reg <= data_avail;	
	empty1_reg <= empty1;	
	full2_reg <= full2;	
     end
   
   
   icon icon_i1
     (
      .CONTROL0(CONTROL0)
      );
   
   ila ila_i1
     (    
	  .CLK(ext_clk), 
	  .CONTROL(CONTROL0), 
	  //    .TRIG0(address_reg), 
	  .TRIG0(write_data_reg[7:0]), 
	  .TRIG1(read_data_reg[7:0]),
	  .TRIG2(0),
	  .TRIG3({space_avail_reg,data_avail_reg,empty1_reg,full2_reg})
	  );

endmodule // ext_fifo

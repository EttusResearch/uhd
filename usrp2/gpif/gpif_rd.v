
module gpif_rd
  (input gpif_clk, input gpif_rst,
   output [15:0] gpif_data, input gpif_rd, input gpif_ep,
   output reg gpif_empty_d, output reg gpif_empty_c,
   
   input sys_clk, input sys_rst,
   input [17:0] data_i, input src_rdy_i, output dst_rdy_o,
   input [17:0] resp_i, input resp_src_rdy_i, output resp_dst_rdy_o,
   output [31:0] debug
   );
   
   wire [17:0] 	data_o, resp_o;
   wire 	final_rdy_data, final_rdy_resp;
   
   // 33/257 Bug Fix
   reg [8:0] 	read_count;
   always @(negedge gpif_clk)
     if(gpif_rst)
       read_count <= 0;
     else if(gpif_rd)
       read_count <= read_count + 1;
     else
       read_count <= 0;

   // Data Path
   wire [17:0] 	data_int;
   wire 	src_rdy_int, dst_rdy_int;
   fifo_2clock_cascade #(.WIDTH(18), .SIZE(4)) rd_fifo_2clk
     (.wclk(sys_clk), .datain(data_i), .src_rdy_i(src_rdy_i), .dst_rdy_o(dst_rdy_o), .space(),
      .rclk(~gpif_clk), .dataout(data_int), .src_rdy_o(src_rdy_int), .dst_rdy_i(dst_rdy_int), .occupied(),
      .arst(sys_rst));

   // FIXME -- handle short packets
   wire 	send_data_line = gpif_rd & ~gpif_ep & ~read_count[8];
   
   fifo_cascade #(.WIDTH(18), .SIZE(9)) rd_fifo
     (.clk(~gpif_clk), .reset(gpif_rst), .clear(0),
      .datain(data_int), .src_rdy_i(src_rdy_int), .dst_rdy_o(dst_rdy_int), .space(),
      .dataout(data_o), .src_rdy_o(final_rdy_data), .dst_rdy_i(send_data_line), .occupied());

   reg [7:0] 	packet_count;
   always @(negedge gpif_clk)
     if(gpif_rst)
       packet_count <= 0;
     else
       if(src_rdy_int & dst_rdy_int & data_int[17]) // eop
	 if(~(send_data_line & data_o[16]))
	   packet_count <= packet_count + 1;
	 else
	   ;
       else
	 if(send_data_line & data_o[16])
	   packet_count <= packet_count - 1;

   always @(negedge gpif_clk)
     if(gpif_rst)
       gpif_empty_d <= 1;
     else
       gpif_empty_d <= ~|packet_count;
   	      
   // Response Path
   wire [15:0] 	resp_fifolevel;
   wire 	send_resp_line = gpif_rd & gpif_ep & ~read_count[4];

   fifo_2clock_cascade #(.WIDTH(18), .SIZE(4)) resp_fifo_2clk
     (.wclk(sys_clk), .datain(resp_i), .src_rdy_i(resp_src_rdy_i), .dst_rdy_o(resp_dst_rdy_o), .space(),
      .rclk(~gpif_clk), .dataout(resp_o), 
      .src_rdy_o(final_rdy_resp), .dst_rdy_i(send_resp_line), .occupied(resp_fifolevel),
      .arst(sys_rst));

   // FIXME -- handle short packets
   
   always @(negedge gpif_clk)
     if(gpif_rst)
       gpif_empty_c <= 1;
     else
       gpif_empty_c <= resp_fifolevel < 16;
   
   // Output Mux
   assign gpif_data = gpif_ep ? resp_o[15:0] : data_o[15:0];

   assign debug = { { 16'd0 },
		    { data_int[17:16], data_o[17:16], packet_count[3:0] },
		    { 2'b0,final_rdy_data, final_rdy_resp, send_data_line, send_resp_line, src_rdy_int, dst_rdy_int} };
   
endmodule // gpif_rd

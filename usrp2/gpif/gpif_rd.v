
module gpif_rd
  (input gpif_clk, input gpif_rst,
   output [15:0] gpif_data, input gpif_rd, output reg have_pkt_rdy,

   input sys_clk, input sys_rst,
   input [18:0] data_i, input src_rdy_i, output dst_rdy_o
   );
   
   wire [15:0] 	rxfifolevel;
   wire [15:0] 	data_o;
   wire 	rx_full;
   
   // USB Read Side of FIFO
   always @(negedge gpif_clk)
     have_pkt_rdy <= (rxfifolevel >= 256);  // FIXME make this more robust

   // 257 Bug Fix
   reg [8:0] 	read_count;
   always @(negedge gpif_clk)
     if(gpif_rst)
       read_count <= 0;
     else if(gpif_rd)
       read_count <= read_count + 1;
     else
       read_count <= 0;

   wire [17:0] 	data_int;
   wire 	src_rdy_int, dst_rdy_int;
   
   fifo_2clock_cascade #(.WIDTH(18), .SIZE(4)) rd_fifo_2clk
     (.wclk(sys_clk), .datain(data_i), .src_rdy_i(src_rdy_i), .dst_rdy_o(dst_rdy_o), .space(),
      .rclk(~gpif_clk), .dataout(data_int), .src_rdy_o(src_rdy_int), .dst_rdy_i(dst_rdy_int), .occupied(),
      .arst(sys_rst));

   fifo_cascade #(.WIDTH(19), .SIZE(9)) rd_fifo
     (.clk(~gpif_clk), .reset(gpif_rst), .clear(0),
      .datain(data_int), .src_rdy_i(src_rdy_int), .dst_rdy_o(dst_rdy_int), .space(),
      .dataout(data_o), .src_rdy_o(), .dst_rdy_i(gpif_rd & ~read_count[8]), .occupied(rxfifolevel));

   assign gpif_data = data_o[15:0];
   
endmodule // gpif_rd

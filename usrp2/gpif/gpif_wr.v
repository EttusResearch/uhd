
module gpif_wr
  (input gpif_clk, input gpif_rst,
   input [15:0] gpif_data, input WR,
   output reg have_space,

   input sys_clk, input sys_rst,
   output [19:0] data_o, output src_rdy_o, input dst_rdy_i,
   output [31:0] debug );

   reg 		 wr_reg;
   reg [15:0] 	 gpif_data_reg;
   
   always @(posedge gpif_clk)
     begin
	wr_reg <= WR;
	gpif_data_reg <= gpif_data;
     end

   reg [9:0] write_count;
   
   always @(posedge gpif_clk)
     if(gpif_rst)
       write_count <= 0;
     else if(wr_reg)
       write_count <= write_count + 1;
     else
       write_count <= 0;

   reg 	     sop;
   wire      occ = 0;
   wire      eop = (write_count == 255);
   
   always @(posedge gpif_clk)
     sop <= WR & ~wr_reg;

   wire [15:0] fifo_space;
   always @(posedge gpif_clk)
     have_space <= fifo_space > 256;
   		  
   fifo_2clock_cascade #(.WIDTH(19), .SIZE(9)) wr_fifo
     (.wclk(gpif_clk), .datain({occ,eop,sop,gpif_data_reg}),
      .src_rdy_i(wr_reg & ~write_count[8]), .dst_rdy_o(), .space(fifo_space),
      .rclk(sys_clk), .dataout(data_o), .src_rdy_o(src_rdy_o), .dst_rdy_i(dst_rdy_i), .occupied(),
      .arst(sys_rst));
   
endmodule // gpif_wr

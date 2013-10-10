//
// Copyright 2013 Ettus Research LLC
//


// Special case SIZE <= 5 uses a short fifo

module axi_fifo_2clk
  #(parameter WIDTH=69, SIZE=9)
   (input reset,
    input i_aclk,
    input [WIDTH-1:0] i_tdata,
    input i_tvalid,
    output i_tready,
    input o_aclk,
    output [WIDTH-1:0] o_tdata,
    output o_tvalid,
    input o_tready);
   
   wire   write, read, empty, full;
   assign i_tready = ~full;
   assign write = i_tvalid & i_tready;

   wire [71:0] tdata_int;
   wire tvalid_int, tready_int;
   assign tvalid_int = ~empty;
   assign read = tvalid_int & tready_int;

    wire [71:0] wr_data;
    assign wr_data[WIDTH-1:0] = i_tdata;
    wire [71:0] rd_data;
    assign tdata_int = rd_data[WIDTH-1:0];

    generate
      if(WIDTH<72) begin
        assign wr_data[71:WIDTH] = 0;
      end
    endgenerate

   generate
      if(SIZE<=5)
	fifo_short_2clk fifo_short_2clk
	      (.rst(reset),
	       .wr_clk(i_aclk),
	       .din(wr_data), // input [71 : 0] din
	       .wr_en(write), // input wr_en
	       .full(full), // output full
	       .wr_data_count(), // output [9 : 0] wr_data_count
	       
	       .rd_clk(o_aclk), // input rd_clk
	       .dout(rd_data), // output [71 : 0] dout
	       .rd_en(read), // input rd_en
	       .empty(empty), // output empty
	       .rd_data_count()  // output [9 : 0] rd_data_count
	       );
      else
	fifo_4k_2clk fifo_4k_2clk
	  (.rst(reset),
	   .wr_clk(i_aclk),
	   .din(wr_data), // input [71 : 0] din
	   .wr_en(write), // input wr_en
	   .full(full), // output full
	   .wr_data_count(), // output [9 : 0] wr_data_count
	   
	   .rd_clk(o_aclk), // input rd_clk
	   .dout(rd_data), // output [71 : 0] dout
	   .rd_en(read), // input rd_en
	   .empty(empty), // output empty
	   .rd_data_count()  // output [9 : 0] rd_data_count
	   );
   endgenerate
   
   generate
      if(SIZE>9)
	axi_fifo #(.WIDTH(WIDTH), .SIZE(SIZE)) fifo_1clk
	  (.clk(o_aclk), .reset(reset), .clear(1'b0),
	   .i_tdata(tdata_int), .i_tvalid(tvalid_int), .i_tready(tready_int),
	   .o_tdata(o_tdata), .o_tvalid(o_tvalid), .o_tready(o_tready),
	   .space(), .occupied());
      else
	begin
	   assign o_tdata = tdata_int;
	   assign o_tvalid = tvalid_int;
	   assign tready_int = o_tready;
	end
   endgenerate
   
endmodule // axi_fifo_2clk

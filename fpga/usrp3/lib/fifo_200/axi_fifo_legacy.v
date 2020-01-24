//
// Copyright 2012-2013 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//



// Block RAM AXI fifo

// Special case SIZE <= 5 uses a short fifo

module axi_fifo_legacy
  #(parameter WIDTH=32, SIZE=9)
   (input clk, input reset, input clear,
    input [WIDTH-1:0] i_tdata,
    input i_tvalid,
    output i_tready,
    output [WIDTH-1:0] o_tdata,
    output o_tvalid,
    input o_tready,
    
    output reg [15:0] space,
    output reg [15:0] occupied);

    generate
    if(SIZE<=5) begin
        wire [5:0] space_short, occupied_short;
        axi_fifo_short #(.WIDTH(WIDTH)) fifo_short
        (
            .clk(clk), .reset(reset), .clear(clear),
            .i_tdata(i_tdata), .i_tvalid(i_tvalid), .i_tready(i_tready),
            .o_tdata(o_tdata), .o_tvalid(o_tvalid), .o_tready(o_tready),
            .space(space_short), .occupied(occupied_short)
        );
        always @* space <= {10'b0, space_short};
        always @* occupied <= {10'b0, occupied_short};
    end
    else begin

   wire 	      write 	     = i_tvalid & i_tready;
   wire 	      read 	     = o_tvalid & o_tready;
   wire 	      full, empty;
   
   assign i_tready  = ~full;
   assign o_tvalid  = ~empty;
   
   // Read side states
   localparam 	  EMPTY = 0;
   localparam 	  PRE_READ = 1;
   localparam 	  READING = 2;

   reg [SIZE-1:0] wr_addr, rd_addr;
   reg [1:0] 	  read_state;

   reg 	  empty_reg = 1'b1, full_reg = 1'b0;
   always @(posedge clk)
     if(reset)
       wr_addr <= 0;
     else if(clear)
       wr_addr <= 0;
     else if(write)
       wr_addr <= wr_addr + 1;

   ram_2port #(.DWIDTH(WIDTH),.AWIDTH(SIZE))
     ram (.clka(clk),
	  .ena(1'b1),
	  .wea(write),
	  .addra(wr_addr),
	  .dia(i_tdata),
	  .doa(),

	  .clkb(clk),
	  .enb((read_state==PRE_READ)|read),
	  .web(1'b0),
	  .addrb(rd_addr),
	  .dib({WIDTH{1'b1}}),
	  .dob(o_tdata));

   always @(posedge clk)
     if(reset)
       begin
	  read_state <= EMPTY;
	  rd_addr <= 0;
	  empty_reg <= 1;
       end
     else
       if(clear)
	 begin
	    read_state <= EMPTY;
	    rd_addr <= 0;
	    empty_reg <= 1;
	 end
       else 
	 case(read_state)
	   EMPTY :
	     if(write)
	       begin
		  //rd_addr <= wr_addr;
		  read_state <= PRE_READ;
	       end
	   PRE_READ :
	     begin
		read_state <= READING;
		empty_reg <= 0;
		rd_addr <= rd_addr + 1;
	     end
	   
	   READING :
	     if(read)
	       if(rd_addr == wr_addr)
		 begin
		    empty_reg <= 1; 
		    if(write)
		      read_state <= PRE_READ;
		    else
		      read_state <= EMPTY;
		 end
	       else
		 rd_addr <= rd_addr + 1;
	 endcase // case(read_state)

   wire [SIZE-1:0] dont_write_past_me = rd_addr - 2;
   wire 	   becoming_full = wr_addr == dont_write_past_me;
     
   always @(posedge clk)
     if(reset)
       full_reg <= 0;
     else if(clear)
       full_reg <= 0;
     else if(read & ~write)
       full_reg <= 0;
     //else if(write & ~read & (wr_addr == (rd_addr-3)))
     else if(write & ~read & becoming_full)
       full_reg <= 1;

   //assign empty = (read_state != READING);
   assign empty = empty_reg;

   // assign full = ((rd_addr - 1) == wr_addr);
   assign full = full_reg;

   //////////////////////////////////////////////
   // space and occupied are for diagnostics only
   // not guaranteed exact

   localparam NUMLINES = (1<<SIZE);
   always @(posedge clk)
     if(reset)
       space <= NUMLINES;
     else if(clear)
       space <= NUMLINES;
     else if(read & ~write)
       space <= space + 16'b1;
     else if(write & ~read)
       space <= space - 16'b1;
   
   always @(posedge clk)
     if(reset)
       occupied <= 16'b0;
     else if(clear)
       occupied <= 16'b0;
     else if(read & ~write)
       occupied <= occupied - 16'b1;
     else if(write & ~read)
       occupied <= occupied + 16'b1;

    end
    endgenerate

endmodule // axi_fifo_legacy

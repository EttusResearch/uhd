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

module ram_loader
  #(parameter AWIDTH=16, RAM_SIZE=16384)
    (
     // Wishbone I/F and clock domain
     input wb_clk,
     input dsp_clk,
     input ram_loader_rst,
     output wire [31:0] wb_dat,
     output wire [AWIDTH-1:0] wb_adr,
     output wb_stb,
     output reg [3:0] wb_sel,
     output wb_we,
     output reg ram_loader_done,
     // CPLD signals and clock domain
     input cpld_clk,
     input cpld_din,
     output reg cpld_start,
     output reg cpld_mode,
     output reg cpld_done,
     input cpld_detached
     );

   localparam S0 = 0;
   localparam S1 = 1;
   localparam S2 = 2;
   localparam S3 = 3;
   localparam S4 = 4;
   localparam S5 = 5;
   localparam S6 = 6;
   localparam RESET = 7;
   
   localparam WB_IDLE = 0;
   localparam WB_WRITE = 1;
  

   reg [AWIDTH+2:0] count;       // 3 LSB's count bits in, the MSB's generate the Wishbone address
   reg [6:0] 	    shift_reg;
   reg [7:0] 	    data_reg;
   reg 		    sampled_clk;
   reg 		    sampled_clk_meta;
   reg 		    sampled_din;
   reg 		    inc_count;
   reg 		    load_data_reg;
   reg 		    shift;  
   reg 		    wb_state, wb_next_state;
   reg [2:0] 	    state, next_state;
    
   //
   // CPLD clock doesn't free run and is approximately 12.5MHz.
   // Use 50MHz Wishbone clock to sample it as a signal and avoid having
   // an extra clock domain for no reason.
   //

   always @(posedge dsp_clk or posedge ram_loader_rst)
      if (ram_loader_rst)
	begin
	   sampled_clk_meta <= 1'b0;
	   sampled_clk <= 1'b0;
	   sampled_din <= 1'b0;
	   count <= 'h7FFF8;  // Initialize so that address will be 0 when first byte fully received.
	   data_reg <= 0;
	   shift_reg <= 0;
	end
      else 
	begin
	   sampled_clk_meta <= cpld_clk;
	   sampled_clk <= sampled_clk_meta;
	   sampled_din <= cpld_din;
	   if (inc_count)
	     count <= count + 1'b1;
	   if (load_data_reg)
	     data_reg <= {shift_reg,sampled_din};
	   if (shift)
	     shift_reg <= {shift_reg[5:0],sampled_din};	   
	end // else: !if(ram_loader_rst)
   
	   
   always @(posedge dsp_clk or posedge ram_loader_rst)
     if (ram_loader_rst)
       state <= RESET;
     else
       state <= next_state;


   always @*
     begin
	// Defaults
	next_state = state;
	cpld_start = 1'b0;
	shift = 1'b0;
	inc_count = 0;
	load_data_reg = 1'b0;
	ram_loader_done = 1'b0;
	cpld_mode = 1'b0;
	cpld_done = 1'b1;
	
	
	
	case (state) //synthesis parallel_case full_case
	  // After reset wait until CPLD indicates its detached.
	  RESET: begin		     
	     if (cpld_detached)
	       next_state = S0;
	     else
	       next_state = RESET;
	  end

	  // Assert cpld_start to signal the CPLD its to start sending serial clock and data.
	  // Assume cpld_clk is low as we transition into search for first rising edge
	  S0: begin
	     cpld_start = 1'b1;	 
	     cpld_done = 1'b0;	 
	     if (~cpld_detached)
	       next_state = S2;
	     else
	       next_state = S0;   
	  end
	  
	  //
	  S1: begin
	     cpld_start = 1'b1;	   
	     cpld_done = 1'b0;	 
	     if (sampled_clk)
	       begin
		  // Found rising edge on cpld_clk.
		  if (count[2:0] == 3'b111)
		    // Its the last bit of a byte, send it out to the Wishbone bus.
		    begin
		       load_data_reg = 1'b1;
		       inc_count = 1'b1;
		    end
		  else 
	          // Shift databit into LSB of shift register and increment count
		     begin
		       shift = 1'b1;
		       inc_count = 1'b1;
		     end // else: !if(count[2:0] == 3'b111)
		  next_state = S2;
	       end // if (sampled_clk)
	     else
	       next_state = S1;
	  end // case: S1
	  
	  //
	  S2: begin
	     cpld_start = 1'b1;	    
	     cpld_done = 1'b0;
	     if (~sampled_clk)
	       // Found negative edge of clock
	       if (count[AWIDTH+2:3] == RAM_SIZE-1) // NOTE need to change this constant
		 // All firmware now downloaded
		 next_state = S3;
	       else
		 next_state = S1;
	     else
	       next_state = S2;
	  end // case: S2
	  
	  // Now that terminal count is reached and all firmware is downloaded signal CPLD that download is done 
	  // and that mode is now SPI mode.
	  S3: begin
	     if (sampled_clk)
	       begin
		  cpld_mode = 1'b1;
		  cpld_done = 1'b1;
		  next_state = S4;
	       end
	     else
	       next_state = S3;	     
	  end

	  // Search for negedge of cpld_clk whilst keeping done sequence asserted.
	  // Keep done assserted 
	  S4: begin
	     cpld_mode = 1'b1;
	     cpld_done = 1'b1;
	     if (~sampled_clk)
	       next_state = S5;
	     else
	       next_state = S4;
	  end

	  // Search for posedge of cpld_clk whilst keeping done sequence asserted.
	  S5: begin
	     cpld_mode = 1'b1;
	     cpld_done = 1'b1;
	     if (sampled_clk)
	       next_state = S6;
	     else	      
	       next_state = S5;	       
	  end

	  // Stay in this state until reset/power down
	  S6: begin
	     ram_loader_done = 1'b1;
	     cpld_done = 1'b1;
	     cpld_mode = 1'b1;
	     next_state = S6;
	  end

	endcase // case(state)
     end

   always @(posedge dsp_clk or posedge ram_loader_rst)
     if (ram_loader_rst)
       wb_state <= WB_IDLE;
     else
       wb_state <= wb_next_state;

   reg do_write;
   wire empty, full;
   
   always @*
     begin
	wb_next_state = wb_state;
	do_write = 1'b0;
	
	case (wb_state) //synthesis full_case parallel_case
	  //
	  WB_IDLE: begin
	     if (load_data_reg)
	       // Data reg will load ready to write wishbone @ next clock edge
	       wb_next_state  =  WB_WRITE;
	     else
	       wb_next_state = WB_IDLE;
	  end

	  // Drive address and data onto wishbone.
	  WB_WRITE: begin
      	     do_write = 1'b1;
	     if (~full)	       
	       wb_next_state =  WB_IDLE;		      
	     else
	       wb_next_state = WB_WRITE;	       
	  end

	endcase // case(wb_state)
     end // always @ *
   
   wire [1:0] count_out;
   wire [7:0] data_out;

   fifo_xlnx_16x40_2clk crossclk
     (.rst(ram_loader_rst),
      .wr_clk(dsp_clk), .din({count[4:3],count[AWIDTH+2:3],data_reg}), .wr_en(do_write), .full(full),
      .rd_clk(wb_clk), .dout({count_out,wb_adr,data_out}), .rd_en(~empty), .empty(empty));

   assign wb_dat = {4{data_out}};

   always @*
     case(count_out[1:0]) //synthesis parallel_case full_case
       2'b00 : wb_sel = 4'b1000;
       2'b01 : wb_sel = 4'b0100;
       2'b10 : wb_sel = 4'b0010;
       2'b11 : wb_sel = 4'b0001;
     endcase

   assign wb_we = ~empty;
   assign wb_stb = ~empty;
   
endmodule // ram_loader

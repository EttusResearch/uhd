//
// Copyright 2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//

module serial_to_settings
  (
   input clk,
   input reset,
   // Serial signals (async)
   input scl,
   input sda,
   // Settngs bus out
   output reg set_stb,
   output reg [7:0] set_addr,
   output reg [31:0] set_data,
   // Debug
   output [31:0] debug
   );

   reg [2:0] 	 state;
   
   localparam 	 SEARCH = 3'h0;
   localparam 	 ADDRESS = 3'h1;
   localparam 	 DATA = 3'h2;
   localparam 	 STOP1 = 3'h3;
   localparam 	 STOP2 = 3'h4;
 	 
   reg 		 scl_pre_reg, scl_reg, scl_reg2;
   reg 		 sda_pre_reg, sda_reg, sda_reg2;
   reg [4:0] 	 counter;

 
     always @(posedge clk) begin
      scl_reg2 <= scl_reg;
      scl_reg <= scl_pre_reg;
      scl_pre_reg <= scl;
      sda_reg2 <= sda_reg;
      sda_reg <= sda_pre_reg;
      sda_pre_reg <= sda;
   end

   
   always @(posedge clk)
     if (reset) begin
	state <= SEARCH;
	counter <= 0;
	set_addr <= 0;
	set_data <= 0;
	set_stb <= 0;
     end else begin
	case(state)
	  //
	  // Search for I2C like start indication: SDA goes low whilst clock is high.
	  //
	  SEARCH: begin
	     set_stb <= 0;
	     // Look for START.
	     if (scl_reg && scl_reg2 && !sda_reg && sda_reg2) begin
		state <= ADDRESS;
		counter <= 0;
	     end
	  end
	  //
	  // Count 8 Address bits.
	  // Master changes SDA on falling edge of SCL, we sample on the rising edge.
	  //
	  ADDRESS: begin
	     if (scl_reg && !scl_reg2) begin
		set_addr[7:0] <= {set_addr[6:0],sda_reg};
		if (counter == 7) begin
		   state <= DATA;
		   counter <= 0;
		end else
		  counter <= counter + 1;		  
	    end
	  end
	  //
	  // Count 32 data bits.
	  // Master changes SDA on falling edge of SCL, we sample on the rising edge.
	  //
	  DATA: begin
	     if (scl_reg && !scl_reg2) begin
		set_data[31:0] <= {set_data[30:0],sda_reg};
		if (counter == 31) begin
		   state <= STOP1;
		   counter <= 0;
		end else
		  counter <= counter + 1;		  
	    end
	  end
	  //
	  // Looks for rising SCL edge before STOP bit.
	  //
	  STOP1: begin
	     if (scl_reg && !scl_reg2) begin
		state <= STOP2;
	     end
	  end
	  //
	  // Looks for STOP bit
	  //
	  STOP2: begin
	     if (scl_reg && scl_reg2 && sda_reg && !sda_reg2) begin
		state <= SEARCH;
		counter <= 0;
		set_stb <= 1;
	     end
	  end
	  
	endcase // case(state)
     end // else: !if(reset)

   assign debug =
		 {
		  counter[4:0],
		  state[2:0],
		  scl_reg,
		  sda_reg
		  };
   


endmodule // serial_to_settings

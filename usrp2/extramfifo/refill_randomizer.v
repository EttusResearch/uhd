//
// EMI mitigation.
// Process FULL flag from FIFO so that de-assertion
// (FIFO now not FULL) is delayed by a pseudo random
// value, but assertion is passed straight through.
// 


module refill_randomizer
  #(parameter BITS=7)
    (
     input clk,
     input rst,
     input full_in,
     output full_out
     );
   
   wire 	    feedback;
   reg 	    full_last;
   wire     full_deasserts;
   reg [6:0] shift_reg;
   reg [6:0] count;
   reg 	     delayed_fall;
   

   always @(posedge clk)
     full_last <= full_in;
   
   assign    full_deasserts = full_last & ~full_in;

   // 7 bit LFSR
   always @(posedge clk)
     if (rst)
       shift_reg <= 7'b1;
     else
       if (full_deasserts)
	 shift_reg <= {shift_reg[5:0],feedback};

   assign    feedback = ^(shift_reg & 7'h41);

   always @(posedge clk)
     if (rst)
       begin
	  count <= 1;
	  delayed_fall  <= 1;
       end
     else if (full_deasserts)
       begin
	  count <= shift_reg;
	  delayed_fall <= 1;
       end
     else if (count == 1)
       begin
	  count <= 1;
	  delayed_fall <= 0;
       end
     else
       begin
	  count <= count - 1;
	  delayed_fall <= 1;
       end
   
   // Full_out goes instantly high if full_in does. However its fall is delayed.
   assign    full_out = (full_in == 1) || (full_last == 1) || delayed_fall;

endmodule
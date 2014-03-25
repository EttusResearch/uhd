//
//  USRP2 - Universal Software Radio Peripheral Mk II
//
//  Copyright (C) 2008 Matt Ettus
//

//

module cic_strober
  #(parameter WIDTH=8)
    ( input clock,
      input reset,
      input enable,
      input [WIDTH-1:0] rate, // Rate should EQUAL to your desired divide ratio, no more -1 BS
      input strobe_fast,
      output wire strobe_slow );
   
   reg [WIDTH-1:0] counter;
   wire      now = (counter==1);
   assign    strobe_slow = now && enable && strobe_fast;
   
   always @(posedge clock)
     if(reset)
       counter <= 0; 
     else if (~enable)
       counter <= rate;
     else if(strobe_fast)
       if(now)
	 counter <= rate;
       else 
	 counter <= counter - 1;
   
endmodule // cic_strober

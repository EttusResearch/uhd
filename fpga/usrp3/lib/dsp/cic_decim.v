// -*- verilog -*-
//
//  USRP - Universal Software Radio Peripheral
//
//  Copyright (C) 2003 Matt Ettus
//

//


module cic_decim
  #(parameter bw = 16, parameter N = 4, parameter log2_of_max_rate = 7)
    (input clock,
     input reset,
     input enable,
     input [7:0] rate,
     input strobe_in,
     input strobe_out,
     input [bw-1:0] signal_in,
     output reg [bw-1:0] signal_out);

   localparam 	     maxbitgain = N * log2_of_max_rate;
   
   wire [bw+maxbitgain-1:0] signal_in_ext;
   reg [bw+maxbitgain-1:0]  integrator [0:N-1];
   reg [bw+maxbitgain-1:0]  differentiator [0:N-1];
   reg [bw+maxbitgain-1:0]  pipeline [0:N-1];
   reg [bw+maxbitgain-1:0]  sampler;
   
   integer 		    i;
   
   sign_extend #(bw,bw+maxbitgain) 
     ext_input (.in(signal_in),.out(signal_in_ext));
   
   always @(posedge clock)
     if(~enable)
       for(i=0;i<N;i=i+1)
	 integrator[i] <= 0;
     else if (strobe_in)
       begin
	  integrator[0] <= integrator[0] + signal_in_ext;
	  for(i=1;i<N;i=i+1)
	    integrator[i] <= integrator[i] + integrator[i-1];
       end	
   
   always @(posedge clock)
     if(~enable)
       begin
	  sampler <= 0;
	  for(i=0;i<N;i=i+1)
	    begin
	       pipeline[i] <= 0;
	       differentiator[i] <= 0;
	    end
       end
     else if (strobe_out)
       begin
	  sampler <= integrator[N-1];
	  differentiator[0] <= sampler;
	  pipeline[0] <= sampler - differentiator[0];
	  for(i=1;i<N;i=i+1)
	    begin
	       differentiator[i] <= pipeline[i-1];
	       pipeline[i] <= pipeline[i-1] - differentiator[i];
	    end
       end // if (enable && strobe_out)
   
   wire [bw-1:0] signal_out_unreg;
   
   cic_dec_shifter #(bw)
     cic_dec_shifter(rate,pipeline[N-1],signal_out_unreg);

   always @(posedge clock)
     signal_out <= signal_out_unreg;
   
endmodule // cic_decim

// -*- verilog -*-
//
//  USRP - Universal Software Radio Peripheral
//
//  Copyright (C) 2003 Matt Ettus
//

//


// Sign extension "macro"
// bits_out should be greater than bits_in

module sign_extend (in,out);
	parameter bits_in=0;  // FIXME Quartus insists on a default
	parameter bits_out=0;
	
	input [bits_in-1:0] in;
	output [bits_out-1:0] out;
	
	assign out = {{(bits_out-bits_in){in[bits_in-1]}},in};
	
endmodule

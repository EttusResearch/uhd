//
// Copyright 2013 Ettus Research LLC
//





module chdr_16sc_to_32f
  #(parameter BASE=0)
   ( input clk, input reset, input set_stb, input [7:0] set_addr,
    input [31:0]      set_data,
    input [63:0]      i_tdata,
    input 	      i_tlast,
    input 	      i_tvalid,
    output 	      i_tready,

   
    output reg [63:0] o_tdata,
    output 	      o_tlast,
    output 	      o_tvalid,
    input 	      o_tready,

    output [31:0]     debug
    );


   wire [31:0] 	      s0_real;
   wire [31:0] 	      s0_imag;
   wire [31:0] 	      s1_real;
   wire [31:0] 	      s1_imag;


   wire 	      chdr_has_hdr = 1'b1;
   wire 	      chdr_has_time = i_tdata[61];
   wire 	      chdr_has_tlr = 1'b0;

 	      
   //chdr length calculations

   wire [15:0] 	      chdr_header_lines = chdr_has_time? 16:8;
   wire [15:0] 	      samples = ((i_tdata[47:32] - chdr_header_lines) << 1);
   wire [15:0] 	      i_samples = (i_tdata[47:32] - chdr_header_lines);
   

   wire [15:0] 	      chdr_payload_lines = samples + chdr_header_lines;

   
   
    wire 	      set_sid;
   wire [15:0] 	      my_newhome;

   setting_reg #(.my_addr(BASE), .width(17)) new_destination
     (.clk(clk), .rst(reset), .strobe(set_stb), .addr(set_addr), .in(set_data),.out({set_sid, my_newhome[15:0]}));

 

		    
   
   //state machines
   localparam HEADER        = 2'd0;//IDLE
   localparam TIME          = 2'd1;
   localparam ODD           = 2'd2;
   localparam EVEN          = 2'd3;
   reg [1:0] 	      state;
   reg      end_on_odd;
   
   
 	      
   

   always @(posedge clk) begin
      
      
      if (reset) begin
	 state <= HEADER;
	 end_on_odd <= 1'b0;
      end
      else if (o_tready && i_tvalid)  case(state)

	     HEADER: begin
		   state <= (i_tdata[61])? TIME : ODD;
		   end_on_odd <= (i_samples[2:1] == 2 || i_samples[2:1] == 1);	
	     end

	     TIME: begin
		   state <= (i_tlast)? HEADER: ODD;
	     end

	     ODD: begin
		   state <= (i_tlast & end_on_odd)? HEADER:EVEN;
	     end

	     EVEN: begin
		  state <= (i_tlast) ? HEADER: ODD;
	     end
					
		default: state <= HEADER;
	   endcase
   end
   

 

   iq_to_float #(.BITS_IN(16), .BITS_OUT(32))
   iq_to_float_imag0 (.in(i_tdata[63:48]), .out(s0_imag[31:0]));
   
   iq_to_float #(.BITS_IN(16), .BITS_OUT(32))
   iq_to_float_real0 (.in(i_tdata[47:32]), .out(s0_real[31:0]));

   iq_to_float #(.BITS_IN(16), .BITS_OUT(32))
   iq_to_float_imag1 (.in(i_tdata[31:16]), .out(s1_imag[31:0]));	

   iq_to_float #(.BITS_IN(16), .BITS_OUT(32))
   iq_to_float_real1 (.in(i_tdata[15:0]), .out(s1_real[31:0]));	
   




   always @(*) 
     case(state)
     
       HEADER: o_tdata <= {i_tdata[63:48], chdr_payload_lines,
			  set_sid ? {i_tdata[15:0], my_newhome[15:0]}:i_tdata[31:0]};
       TIME: o_tdata <= i_tdata;
       ODD: o_tdata <= {s0_imag,s0_real};
       EVEN: o_tdata <= {s1_imag,s1_real};
  
       
       default : o_tdata = i_tdata;
     endcase
   
   assign o_tvalid = i_tvalid;
   assign i_tready = o_tready && ((state != ODD) || (i_tlast && end_on_odd));
   assign o_tlast  = i_tlast && ((state == EVEN) || (state == ODD && end_on_odd));

endmodule

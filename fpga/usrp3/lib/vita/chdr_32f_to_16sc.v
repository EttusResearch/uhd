//
// Copyright 2013 Ettus Research LLC
//



module chdr_32f_to_16sc
  #(parameter BASE=0)
   (input set_stb, input [7:0] set_addr, input [31:0] set_data,
    
    input 	      clk, input reset,
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

 
   wire 	      chdr_has_hdr = 1'b1;
   wire 	      chdr_has_time = i_tdata[61];
   wire 	      chdr_has_tlr = 1'b0;

   wire [15:0] 	      s0_imag;
   wire [15:0] 	      s0_real;
   wire [15:0] 	      s1_imag;
   wire [15:0] 	      s1_real;
   

   reg [15:0] 	      imag0;
   reg [15:0] 	      real0;
   wire [15:0] 	      imag1;
   wire [15:0] 	      real1;
   



   


   //chdr length calculations
   wire [15:0] 	      chdr_header_lines = chdr_has_time? 16 : 8;

   wire [15:0] 	      samples = ((i_tdata[47:32] - chdr_header_lines) >> 1);

   wire [15:0] 	      chdr_payload_lines = samples + chdr_header_lines;


   wire 	      set_sid;
   wire [15:0] 	      my_newhome;

   setting_reg #(.my_addr(BASE), .width(17)) new_destination
     (.clk(clk), .rst(reset), .strobe(set_stb), .addr(set_addr), .in(set_data),
      .out({set_sid, my_newhome[15:0]}));

 

   localparam HEADER        = 2'd0;//IDLE
   localparam TIME    = 2'd1;
   localparam ODD           = 2'd2;
   localparam EVEN          = 2'd3;

   reg [1:0] 	      state;
   
   always @(posedge clk) begin
      if (reset) begin
         state <= HEADER;

      end
      else if (i_tvalid && i_tready) case(state)

	      HEADER: begin
		if (!i_tlast) state <= (i_tdata[61])? TIME : ODD;
	      end
	      
	      TIME: begin
		state <= (i_tlast)? HEADER: ODD;
	      end
	      
	      ODD: begin
		state <= (i_tlast)? HEADER: EVEN;
	      end

	      EVEN: begin
		state <= (i_tlast)? HEADER: ODD;
	      end
	      
	      default: state <= HEADER;
	    endcase
   end // always @ (posedge clk)

   //hold data after each input transfer
   reg [63:0] 	       hold_tdata;
   always @(posedge clk) begin
    	if (i_tvalid && i_tready) hold_tdata <= i_tdata;
   end


   
   float_to_iq #(.BITS_IN(32),.BITS_OUT(16))
   float_to_iq_imag0 (.in(i_tdata[63:32]),.out(s1_imag[15:0]));

   float_to_iq #(.BITS_IN(32),.BITS_OUT(16))
   float_to_iq_real0 (.in(i_tdata[31:0]),.out(s1_real[15:0]));

   
   float_to_iq #(.BITS_IN(32),.BITS_OUT(16))
   float_to_iq_imag1 (.in(hold_tdata[63:32]),.out(s0_imag[15:0]));

   
   float_to_iq #(.BITS_IN(32),.BITS_OUT(16))
   float_to_iq_real1 (.in(hold_tdata[31:0]),.out(s0_real[15:0]));


   

   always @(*) 
     case(state)
       
       HEADER: o_tdata <= {i_tdata[63:48], chdr_payload_lines,
			   set_sid ? {i_tdata[15:0], my_newhome[15:0]}:i_tdata[31:0]};
       TIME: o_tdata <= i_tdata;
       ODD: o_tdata <= {s1_imag, s1_real, 32'h0};
       EVEN: o_tdata <= {s0_imag, s0_real, s1_imag, s1_real};
       
      default : o_tdata = i_tdata;
     endcase
   
   assign o_tvalid =  i_tvalid && (state != ODD || i_tlast);  
   assign i_tready =  o_tready || (state == ODD && !i_tlast); 
   assign o_tlast  = i_tlast;

endmodule




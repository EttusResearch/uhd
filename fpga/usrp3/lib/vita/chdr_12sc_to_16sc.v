//
// Copyright 2013 Ettus Research LLC
//






module chdr_12sc_to_16sc
  #(parameter BASE = 0)
   ( input set_stb, input [7:0] set_addr, input [31:0] set_data,
     //input side of device
     input 	       clk, input reset,
     input [63:0]      i_tdata,
     input 	       i_tlast,
     input 	       i_tvalid,
     output 	       i_tready,
     //output side of device
     output reg [63:0] o_tdata,
     output 	       o_tlast,
     output 	       o_tvalid,
     input 	       o_tready,

     output [31:0]     debug
     );
   

   
   
   
   wire 	       chdr_has_hdr = 1'b1;
   wire 	       chdr_has_time = i_tdata[61];
   wire 	       chdr_has_tlr = 1'b0;

   wire [15:0] 	       chdr_header_lines = chdr_has_time? 16 : 8;  
   wire [15:0] 	       just_samples_in = i_tdata[47:32] - chdr_header_lines;

   //calculating output length based on input ( 4/3*input = output)

   wire [30:0] 	       calc_output_len = ({just_samples_in,14'h0} + {just_samples_in,12'h0} + {just_samples_in,10'h0} + {just_samples_in,8'h0} + {just_samples_in,6'h0} + {just_samples_in,4'h0} + {just_samples_in,2'h0}+{just_samples_in} +'b0001000000000000)<<2;

   wire [15:0] 	       samples = calc_output_len[30:16];
   wire [15:0] 	       chdr_payload_lines = samples + chdr_header_lines;
   

   reg 		       has_exline;
   reg 		       in_exline;
   
   
   
   wire 	       set_sid;
   wire [15:0] 	       my_newhome;
   

   setting_reg #(.my_addr(BASE), .width(17)) new_destination
     (.clk(clk), .rst(reset), .strobe(set_stb), .addr(set_addr), .in(set_data),
      .out({set_sid, my_newhome[15:0]}));

   localparam HEADER = 3'd0; // IDLE
   localparam TIME     = 3'd1;
   localparam ODD_LINE_ZERO = 3'd2;
   localparam EVEN_LINE_ONE = 3'd3;
   localparam ODD_LINE_TWO = 3'd4;
   localparam EVEN_LINE_THREE = 3'd5;
   
   reg [2:0] 	       state;

   

   always @(posedge clk) begin
      
      if (reset) begin
         state <= HEADER;	 
      end
      
      else if (o_tvalid && o_tready) case(state)


				       HEADER: begin
					  has_exline <= ( (samples[4:2] == 5) || (samples[4:2] == 7) || (samples[4:2] == 0));
					  state <= (chdr_has_time)? TIME : ODD_LINE_ZERO;
				       end
				       
				       TIME: begin
					  state <= (i_tlast)? HEADER: ODD_LINE_ZERO; 
				       end

				       ODD_LINE_ZERO: begin
					  if ((i_tlast & !has_exline) || in_exline) begin
					     state <= HEADER;
					     in_exline <= 0;
					  end
					  else if (i_tlast & has_exline) begin
					     in_exline <= 1;
					     state <= EVEN_LINE_ONE;
					  end
					  else
					    state <= EVEN_LINE_ONE;
				       end

				       EVEN_LINE_ONE: begin
					  if ((i_tlast & !has_exline) || in_exline) begin
					     state <= HEADER;
					     in_exline <= 0;
					  end
					  else if (i_tlast & has_exline) begin
					     in_exline <= 1;
					     state <= ODD_LINE_TWO;
					  end
					  else
					    state <= ODD_LINE_TWO;
				       end
				       
				       ODD_LINE_TWO: begin
					  if ((i_tlast & !has_exline) || in_exline) begin
					     state <= HEADER;
					     in_exline <= 0;
					  end
					  else if (i_tlast & has_exline) begin
					     in_exline <= 1;
					     state <= EVEN_LINE_THREE;
					  end
					  else
					    state <= EVEN_LINE_THREE;
				       end 
				       
				       EVEN_LINE_THREE: begin
					  if (in_exline) begin
					     state <= HEADER;
					     in_exline <= 0;
					  end
					  else
					    state <= ODD_LINE_ZERO;
				       end 
				       
				       default: state <= HEADER;
				       
				     endcase
   end


   //hold data after each input xfer
   reg [63:0] 	       hold_tdata;
   always @(posedge clk) begin
      if (i_tvalid && i_tready) hold_tdata <= i_tdata;
   end

   //main mux
   
   always @(*)
     case(state)

       HEADER: o_tdata <= {i_tdata[63:48],chdr_payload_lines,
			   set_sid ? {i_tdata[15:0], my_newhome[15:0]}:i_tdata[31:0]};
       
       TIME: o_tdata <= i_tdata;
       
       ODD_LINE_ZERO: o_tdata <= {i_tdata[63:52], 4'h0, i_tdata[51:40], 4'h0, i_tdata[39:28],4'h0, i_tdata[27:16], 4'h0};
       
       EVEN_LINE_ONE: o_tdata <= {hold_tdata[15:4],4'h0,hold_tdata[3:0],i_tdata[63:56],4'h0,i_tdata[55:44], 4'h0,i_tdata[43:32],4'h0};
       
       ODD_LINE_TWO: o_tdata <= {hold_tdata[31:20], 4'h0, hold_tdata[19:8],4'h0, hold_tdata[7:0],i_tdata[63:60],4'h0,i_tdata[59:48],4'h0};
       
       EVEN_LINE_THREE: o_tdata <= {hold_tdata[47:36],4'h0,hold_tdata[35:24],4'h0,hold_tdata[23:12],4'h0,hold_tdata[11:0],4'h0};
       

       default: o_tdata <= i_tdata;
       
     endcase
   
   assign o_tvalid = (in_exline)? 1'b1: i_tvalid;
   assign i_tready = (state != EVEN_LINE_THREE) & o_tready & !in_exline;
   assign o_tlast  = (has_exline)? in_exline: ((state != EVEN_LINE_THREE) && i_tlast);

endmodule

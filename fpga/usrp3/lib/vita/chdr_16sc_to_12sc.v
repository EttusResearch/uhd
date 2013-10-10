//
// Copyright 2013 Ettus Research LLC
//


module chdr_16sc_to_12sc
  #(parameter BASE=0)
   ( input set_stb, input [7:0] set_addr, input [31:0] set_data,
     //left side of device
     input 	       clk, input reset,
     input [63:0]      i_tdata,
     input 	       i_tlast,
     input 	       i_tvalid,
     output 	       i_tready,
     //right side of device
     output reg [63:0] o_tdata = 0,
     output 	       o_tlast,
     output 	       o_tvalid,
     input 	       o_tready,

     output [31:0]     debug
     );
   
   wire 	       chdr_has_hdr = 1'b1;
   wire 	       chdr_has_time = i_tdata[61];
   wire 	       chdr_has_tlr = 1'b0;

   wire [11:0] 	       imag0;
   wire [11:0] 	       real0;
   wire [11:0] 	       imag1;
   wire [11:0] 	       real1;
   wire [11:0] 	       imag2;
   wire [11:0] 	       real2;
   
   

   wire [16:0] 	       round_i0;
   wire [16:0] 	       round_r0;
   wire [16:0] 	       round_i1;
   wire [16:0] 	       round_r1;
   wire [16:0] 	       round_i2;
   wire [16:0] 	       round_r2;

//pipiline registers

   reg [11:0] 	       imag0_out;
   reg [11:0] 	       real0_out;
   reg [11:0] 	       imag1_out;
   reg [11:0] 	       real1_out;

   reg [15:0] 	       len_data;

   //chdr length calculations
   wire [15:0] 	       chdr_header_lines = chdr_has_time? 16 : 8;
   wire [15:0] 	       in_samples = i_tdata[47:32] - chdr_header_lines;
   wire [15:0] 	       samples = (in_samples*3) >> 2;
   wire [15:0] 	       chdr_payload_lines = samples + chdr_header_lines;  


   reg 		       needs_exline = 0;
   reg                 in_exline = 0;
 
   
   wire 	       set_sid;
   wire [15:0] 	       my_newhome;
 	       
   setting_reg #(.my_addr(BASE), .width(17)) new_destination
     (.clk(clk), .rst(reset), .strobe(set_stb), .addr(set_addr), .in(set_data),
      .out({set_sid, my_newhome[15:0]}));
   
   //state machine

   localparam HEADER = 3'd0; // IDLE
   localparam TIME     = 3'd1;
   localparam LINE_ODD_ZERO = 3'd2;
   localparam LINE_EVEN_ONE = 3'd3;
   localparam LINE_ODD_TWO = 3'd4;
   localparam REG_STATE = 3'd5;
   
   reg [2:0] 	       state;
   
   
   
   always @(posedge clk) begin
      if (reset) begin
	 state <= HEADER;
	 needs_exline <= 0;
	 in_exline <= 0;
      end
  

      else if ((o_tvalid && o_tready) || (i_tready && i_tvalid)) case(state)

								   
								   HEADER: begin
								      
								      needs_exline <= (in_samples[4:2] == 3 || in_samples[4:2] == 4 || in_samples[4:2] == 6);
								      state <= (i_tdata[61])? TIME: REG_STATE;
								      
								   end

								   TIME: begin
								      state <= (i_tlast) ? HEADER: REG_STATE;     
								   end

								   REG_STATE: begin
								      if (i_tlast & !needs_exline || in_exline) begin
									 state <= HEADER;
									 in_exline <= 0;
								      end
								      else if (i_tlast & needs_exline) begin
									 state <= LINE_EVEN_ONE;
									 in_exline <= 1;
									 
								      end
								      else
									state <= LINE_EVEN_ONE;
								   end


								   LINE_EVEN_ONE: begin
								      if (i_tlast & !needs_exline || in_exline) begin
									 state <= HEADER;
									 in_exline <= 0;
								      end
								      else if (i_tlast & needs_exline) begin
									 state <= LINE_ODD_TWO;
									 in_exline <= 1;
									 
								      end
								      else
									state <= LINE_ODD_TWO;
								   end

								   
								   

								   LINE_ODD_TWO: begin
								      if (i_tlast & !needs_exline || in_exline) begin
									 state <= HEADER;
									 in_exline <= 0;
								      end
								      else if (i_tlast & needs_exline) begin
									 state <= LINE_ODD_ZERO;
									 in_exline <= 1;
									 
								      end
								      else
									state <= LINE_ODD_ZERO;
								   end

								   LINE_ODD_ZERO: begin
								      if (i_tlast & !needs_exline || in_exline) begin
									 state <= HEADER;
									 in_exline <= 0;
								      end
								      else if (i_tlast & needs_exline) begin
									 state <= REG_STATE;
									 in_exline <= 1;
								      end
								      else
									state <= REG_STATE;
								   end

								   default: state <= HEADER;

								 endcase
   end 
   
   assign	round_i0 = ({i_tdata[63],i_tdata[63:48]} + 'h0008);
   assign	round_r0 = ({i_tdata[47],i_tdata[47:32]} + 'h0008);
   
   assign imag0 = (round_i0[16] == 0 && round_i0[15] == 1)?(12'h7FF):(round_i0[16] == 1 && round_i0[15] == 0)? (12'h800):(round_i0[15:4]);
   
   assign	real0 = (round_r0[16] == 0 && round_r0[15] == 1)?(12'h7FF):(round_r0[16] == 1 && round_r0[15] == 0)? (12'h800):(round_r0[15:4]);
   
   assign round_i1 = ({i_tdata[31],i_tdata[31:16]} + 'h0008);
   assign round_r1 = ({i_tdata[15],i_tdata[15:0]} + 'h0008);
   
   assign imag1 = (round_i1[16] == 0 && round_i1[15] == 1)?(12'h7FF):(round_i1[16] == 1 && round_i1[15] == 0)? (12'h800):(round_i1[15:4]);
   
   assign	real1 = (round_r1[16] == 0 && round_r1[15] == 1)?(12'h7FF):(round_r1[16] == 1 && round_r1[15] == 0)? (12'h800):(round_r1[15:4]);


   
   
   always @(posedge clk)
     if (i_tvalid && o_tready)
       begin
	  imag0_out <= imag0;
	  real0_out <= real0;
	  imag1_out <= imag1;
	  real1_out <= real1;
       end



   always @(*)
     case(state)
       
        HEADER: o_tdata <= {i_tdata[63:48], chdr_payload_lines,
			  set_sid ? {i_tdata[15:0], my_newhome[15:0]}:i_tdata[31:0]};
        TIME: o_tdata <= i_tdata;

       
        REG_STATE: o_tdata <= {imag0,real0,imag1, real1, 16'b0};
       
	LINE_EVEN_ONE: o_tdata <= {imag0_out, real0_out, imag1_out, real1_out, imag0, real0[11:8]};
	LINE_ODD_TWO: o_tdata <= {real0_out[7:0], imag1_out, real1_out, imag0, real0,imag1[11:4]};
	LINE_ODD_ZERO: o_tdata <= {imag1_out[3:0], real1_out, imag0, real0, imag1, real1};
	      
       default : o_tdata <= i_tdata;
     endcase 
   
   assign o_tvalid =((in_exline) || (state != REG_STATE & i_tvalid) || (i_tlast & i_tvalid & !needs_exline)); 
   assign i_tready = (o_tready & !in_exline)||(state == REG_STATE && !i_tlast);
   assign o_tlast =  (needs_exline)? in_exline: i_tlast;
   
   

endmodule

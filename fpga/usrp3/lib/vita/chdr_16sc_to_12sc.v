//
// Copyright 2013 Ettus Research LLC
//


module chdr_16sc_to_12sc
  #(parameter BASE=0)
    (
     // Clocks and resets
     input             clk,
     input             reset,
     // Settings bus
     input             set_stb,
     input [7:0]       set_addr,
     input [31:0]      set_data,
     // Input CHDR bus
     input [63:0]      i_tdata,
     input 	       i_tlast,
     input 	       i_tvalid,
     output 	       i_tready,
     // Output CHDR bus
     output reg [63:0] o_tdata,
     output 	       o_tlast,
     output 	       o_tvalid,
     input 	       o_tready,
     // Debug
     output [31:0]     debug
      );

   wire 		chdr_has_time = i_tdata[61];

   wire [11:0] 		q0;
   wire [11:0] 		i0;
   wire [11:0] 		q1;
   wire [11:0] 		i1;
   wire [11:0] 		q2;
   wire [11:0] 		i2;

   wire [16:0] 		round_q0;
   wire [16:0] 		round_i0;
   wire [16:0] 		round_q1;
   wire [16:0] 		round_i1;
   wire [16:0] 		round_q2;
   wire [16:0] 		round_i2;

   // Pipeline registers
   reg [11:0] 		q0_out;
   reg [11:0] 		i0_out;
   reg [11:0] 		q1_out;
   reg [11:0] 		i1_out;

   // CHDR has either 8 bytes of header or 16 if VITA time is included.
   wire [15:0] 		chdr_header_lines = chdr_has_time? 16 : 8;
   // Calculate size of samples input in bytes by taking CHDR size filed and subtracting header length.
   wire [15:0] 		sample_byte_count_in = i_tdata[47:32] - chdr_header_lines;
   // Calculate size of samples to be output by taking input size and scaling by 3/4
   wire [15:0] 		sample_byte_count_out = (sample_byte_count_in*3) >> 2;
   // Calculate size of output CHDR packet by adding back header size to new payload size.
   wire [15:0] 		output_chdr_pkt_size = sample_byte_count_out + chdr_header_lines;

   reg 			needs_extra_line;
   reg 			in_extra_line;

   wire 		set_sid;
   wire [15:0] 		new_sid_dst;

   setting_reg #(.my_addr(BASE), .width(17)) new_destination
     (.clk(clk), .rst(reset), .strobe(set_stb), .addr(set_addr), .in(set_data),
      .out({set_sid, new_sid_dst[15:0]}));
   // state machine

   localparam 		HEADER  = 3'd0;
   localparam 		TIME    = 3'd1;
   localparam 		SAMPLE1 = 3'd2;
   localparam 		SAMPLE2 = 3'd3;
   localparam 		SAMPLE3 = 3'd4;
   localparam 		SAMPLE4 = 3'd5;

   reg [2:0] 		state;

   always @(posedge clk)
     if (reset) begin
	state <= HEADER;
	needs_extra_line <= 0;
	in_extra_line <= 0;
     end else begin
	case(state)
	  //
	  // Process Header line of input packet or idle in this state waiting for new packet.
	  // If the the input packet has SAMPLE_COUNT MODULO 8 == 3 or 4 or 6 then when tlast is asserted we will need one more
	  // output cycle to finish outputing processed samples.
	  //
	  HEADER: begin
	     if (o_tready && i_tvalid)
	       begin
		  needs_extra_line <= (sample_byte_count_in[4:2] == 3 || sample_byte_count_in[4:2] == 4 || sample_byte_count_in[4:2] == 6);
		  // If the input packet had time, then add time to output packet
		  state <= (i_tdata[61])? TIME: SAMPLE1;
	       end
	  end
	  //
	  // Process time field of input packet
	  //
	  TIME: begin
	     if (o_tready && i_tvalid)
	       begin
		  // If we get a premature end of line go back to searching for start of new packet.
		  state <= (i_tlast) ? HEADER: SAMPLE1;
	       end
	  end
	  //
	  // Process line of sample data from input packet.
	  // Not yet enough data to prepare first of three repeating output lines
	  // unless this the last line of a packet when the lats output line
	  // is composed of data from one or both samples in this input packet.
	  //
	  SAMPLE1: begin
	     if ((i_tlast && !needs_extra_line && o_tready && i_tvalid) || (in_extra_line && o_tready)) begin
		// We can finish this packet immediately.
		state <= HEADER;
		in_extra_line <= 0;
	     end else if (i_tlast && needs_extra_line && o_tready && i_tvalid) begin
		// We still need one more output line to drain all samples into this packet.
		// (SHOULD NOT BE POSSIBLE TO GET HERE!)
		state <= SAMPLE2;
		in_extra_line <= 1;
	     end else if (o_tready && i_tvalid)
	       state <= SAMPLE2;
	  end
	  //
	  // First of three repeating output line patterns
	  //
	  SAMPLE2: begin
	     if ((i_tlast && !needs_extra_line && o_tready && i_tvalid) || (in_extra_line && o_tready)) begin
		// We can finish this packet immediately.
		state <= HEADER;
		in_extra_line <= 0;
	     end else if (i_tlast && needs_extra_line && o_tready && i_tvalid) begin
		// We still need one more output line to drain all samples into this packet.
		state <= SAMPLE3;
		in_extra_line <= 1;
	     end else if (o_tready && i_tvalid)
	       state <= SAMPLE3;
	  end
	  //
	  // Second of three repeating output line patterns
	  //
	  SAMPLE3: begin
	     if ((i_tlast && !needs_extra_line && o_tready && i_tvalid) || (in_extra_line && o_tready)) begin
		// We can finish this packet immediately.
		state <= HEADER;
		in_extra_line <= 0;
	     end else if (i_tlast && needs_extra_line && o_tready && i_tvalid) begin
		// We still need one more output line to drain all samples into this packet.
		state <= SAMPLE4;
		in_extra_line <= 1;
	     end else if (o_tready && i_tvalid)
	       state <= SAMPLE4;
	  end
	  //
	  // Third of three repeating output line patterns
	  //
	  SAMPLE4: begin
	     if ((i_tlast && !needs_extra_line && o_tready && i_tvalid) || (in_extra_line && o_tready)) begin
		// We can finish this packet immediately.
		state <= HEADER;
		in_extra_line <= 0;
	     end else if (i_tlast && needs_extra_line && o_tready && i_tvalid) begin
		// We still need one more output line to drain all samples into this packet.
		// (SHOULD NOT BE POSSIBLE TO GET HERE!!)
		state <= SAMPLE1;
		in_extra_line <= 1;
	     end else if (o_tready && i_tvalid)
	       state <= SAMPLE1;
	  end
	  //
	  // Should never get here.
	  //
	  default: state <= HEADER;

	endcase
     end


   // Add rounding value into 16bit samples before trunctaion
   assign	round_q0 = ({i_tdata[63],i_tdata[63:48]} + 'h0008);
   assign	round_i0 = ({i_tdata[47],i_tdata[47:32]} + 'h0008);
   // Truncate with saturation to 12bits precision.
   assign 	q0 = (round_q0[16:15] == 2'b01) ? 12'h7FF : ((round_q0[16:15] == 2'b10) ? 12'h800 : round_q0[15:4]);
   assign	i0 = (round_i0[16:15] == 2'b01) ? 12'h7FF : ((round_i0[16:15] == 2'b10) ? 12'h800 : round_i0[15:4]);
   // Add rounding value into 16bit samples before trunctaion
   assign 	round_q1 = ({i_tdata[31],i_tdata[31:16]} + 'h0008);
   assign 	round_i1 = ({i_tdata[15],i_tdata[15:0]} + 'h0008);
   // Truncate with saturation to 12bits precision.
   assign 	q1 = (round_q1[16:15] == 2'b01) ? 12'h7FF : ((round_q1[16:15] == 2'b10) ? 12'h800 : round_q1[15:4]);
   assign	i1 = (round_i1[16:15] == 2'b01) ? 12'h7FF : ((round_i1[16:15] == 2'b10) ? 12'h800 : round_i1[15:4]);

   //
   // Keep values from current input line to populate fields in next cycles output line
   //
   always @(posedge clk)
     if (i_tvalid && o_tready)
       begin
	  q0_out <= q0;
	  i0_out <= i0;
	  q1_out <= q1;
	  i1_out <= i1;
       end

   //
   // Mux Output data
   //
   always @(*)
     case(state)
       // Populate header with CHDR fields
       HEADER: o_tdata = {i_tdata[63:48], output_chdr_pkt_size,
			   set_sid ? {i_tdata[15:0], new_sid_dst[15:0]}:i_tdata[31:0]};
       // Add 64bit VITA time to packet
       TIME: o_tdata = i_tdata;
       // Special ending corner case for input packets with SAMPLE_COUNT MODULO 8 == (1 | 2)
       SAMPLE1: o_tdata = {q0,i0,q1, i1, 16'b0};
       // Line one of repeating 12bit packed data pattern
       SAMPLE2: o_tdata = {q0_out, i0_out, q1_out, i1_out, q0, i0[11:8]};
       // Line two of repeating 12bit packed data pattern
       SAMPLE3: o_tdata = {i0_out[7:0], q1_out, i1_out, q0, i0,q1[11:4]};
       // Line three of repeating 12bit packed data pattern
       SAMPLE4: o_tdata = {q1_out[3:0], i1_out, q0, i0, q1, i1};
       default : o_tdata = i_tdata;
     endcase // case(state)


   assign  o_tvalid =
		      // We are outputing the (extra) last line of a packet
		      in_extra_line ||
		      // When not in the SAMPLE1 state and there's new input data (Unless its the last line....)
		      (state != SAMPLE1 & i_tvalid) ||
		      // Last line of input packet and we can finish this cycle. (Includes when state is SAMPLE1)
		      (i_tlast & i_tvalid & !needs_extra_line);

   assign  i_tready =
		      // Downstream is ready and we are not currently outputing last (extra) line of packet.
		      (o_tready && !in_extra_line) ||
		      // We don't create output data in SAMPLE1 unless its last line so don't need downstream ready to proceed.
		      ((state == SAMPLE1) && !i_tlast);

   assign  o_tlast =  (needs_extra_line) ? in_extra_line : i_tlast;

endmodule

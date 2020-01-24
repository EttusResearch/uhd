//
// Copyright 2013 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//


module chdr_16sc_to_12sc
  #(parameter BASE=0)
  (
    // Clocks and resets
    input               clk,
    input               reset,
    // Settings bus
    input               set_stb,
    input [7:0]         set_addr,
    input [31:0]        set_data,
    // Input CHDR bus
    input [63:0]        i_tdata,
    input               i_tlast,
    input               i_tvalid,
    output              i_tready,
    // Output CHDR bus
    output reg [63:0]   o_tdata,
    output              o_tlast,
    output              o_tvalid,
    input               o_tready,
    // Debug
    output [31:0]       debug
  );

  wire 		chdr_has_time = i_tdata[61];

  wire [11:0]   q0;
  wire [11:0]   i0;
  wire [11:0]   q1;
  wire [11:0]   i1;
  wire [11:0]   q2;
  wire [11:0]   i2;

  wire [16:0]   round_q0;
  wire [16:0]   round_i0;
  wire [16:0]   round_q1;
  wire [16:0]   round_i1;
  wire [16:0]   round_q2;
  wire [16:0]   round_i2;

  // Pipeline register
  reg [63:0]    line_buff;

  // CHDR has either 8 bytes of header or 16 if VITA time is included.
  wire [15:0]   chdr_header_lines = chdr_has_time? 16 : 8;
  // Calculate size of samples input in bytes by taking CHDR size filed and subtracting header length.
  wire [15:0]   sample_byte_count_in = i_tdata[47:32] - chdr_header_lines;
  // Calculate size of samples to be output by taking input size and scaling by 3/4
  wire [15:0]   sample_byte_count_out = (sample_byte_count_in*3) >> 2;
  // Calculate size of output CHDR packet by adding back header size to new payload size.
  wire [15:0]   output_chdr_pkt_size = sample_byte_count_out + chdr_header_lines;

  reg           odd;

  wire          set_sid;
  wire [15:0]   new_sid_dst;

  setting_reg #(.my_addr(BASE), .width(17)) new_destination
    (.clk(clk), .rst(reset), .strobe(set_stb), .addr(set_addr), .in(set_data),
    .out({set_sid, new_sid_dst[15:0]}));

  // state machine

  localparam    HEADER    = 3'd0;
  localparam    TIME      = 3'd1;
  localparam    SAMPLE1   = 3'd2;
  localparam    SAMPLE2   = 3'd3;
  localparam    SAMPLE3   = 3'd4;
  localparam    SAMPLE4   = 3'd5;
  localparam    RESIDUAL  = 3'd6;

  reg [2:0]     state;

  always @(posedge clk)
    if (reset) begin
      state <= HEADER;
      line_buff <= 0;
    end else begin
      case(state)
        //
        // Process header
        // Check for timestamp.  Byte count conversion is done above.
        //
        HEADER: begin
          if (i_tvalid & i_tready) begin
            odd <= sample_byte_count_in [2];
            // If the input packet had time, then add time to output packet
            state <= (i_tdata[61])? TIME: SAMPLE1;
          end
        end
        //
        // Process time field
        //
        TIME: begin
          if (i_tvalid & i_tready) begin
            // If we get a premature end of line go back to searching for start of new packet.
            state <= (i_tlast) ? HEADER: SAMPLE1;
          end
        end
        //
        // There are 3 lines of output data for each 4 lines of input data.
        // The 4 sample states below represent the 4 lines of input.
        // They are repeatedly cycled until all data is consumed.
        //
        // Process first line
        // The 8 bytes are converted to 6 bytes, so there is not enough for an
        // 8-byte output line.  Store the data unless this is the last line in
        // the packet.
        //
        SAMPLE1: begin
          if (i_tvalid & i_tready) begin
            if (i_tlast) begin
              line_buff <= 0;
              state <= HEADER;
            end else begin
              // Save data to buffer - no output
              line_buff <= {q0,i0,q1,i1,16'd0};
              state <= SAMPLE2;
            end
          end
        end
        //
        // Process second line
        // Output a line comprised of the 6 bytes from the fist line and
        // 2 bytes from this line.  Store the remaining 4 bytes.
        //
        SAMPLE2: begin
          if (i_tvalid & i_tready) begin
            line_buff <= {i0[7:0],q1,i1,32'd0};
            state <= i_tlast ? RESIDUAL : SAMPLE3;
          end
        end
        //
        // Process third line
        // Output line comprised of the 4 remaining bytes from the second line
        // and 4 bytes from this line.  Store the remaining 2 bytes unless this
        // is the last line in the packet and the number of samples is odd.
        //
        SAMPLE3: begin
          if (i_tvalid & i_tready) begin
            line_buff <= (i_tlast & odd) ? 0 : {q1[3:0],i1,48'd0};
            if (i_tlast)
                state <= odd ? HEADER : RESIDUAL;
            else
              state <= SAMPLE4;
          end
        end
        //
        // Process fourth line
        // Output line comprised of the remaining 2 bytes from the third line
        // and the 6 bytes from this line.
        //
        SAMPLE4: begin
          if (i_tvalid & i_tready) begin
            line_buff <= 0;
            state <= i_tlast ? HEADER : SAMPLE1;
          end
        end
        //
        // Pause input to output residual data in buffer
        //
        RESIDUAL: begin
          if (o_tvalid & o_tready) begin
            line_buff <= 0;
            state <= HEADER;
          end
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
  assign 	q1 = (round_q1[16:15] == 2'b01) ? 12'h3FF : ((round_q1[16:15] == 2'b10) ? 12'h800 : round_q1[15:4]);
  assign	i1 = (round_i1[16:15] == 2'b01) ? 12'h3FF : ((round_i1[16:15] == 2'b10) ? 12'h800 : round_i1[15:4]);

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
      // Only output if i_tlast in SAMPLE1 state
      SAMPLE1: o_tdata = {q0,i0,q1, i1, 16'b0};
      SAMPLE2: o_tdata = {line_buff[63:16], q0, i0[11:8]};
      SAMPLE3: o_tdata = {line_buff[63:32], q0, i0,q1[11:4]};
      SAMPLE4: o_tdata = {line_buff[63:48], q0, i0, q1, i1};
      RESIDUAL: o_tdata = line_buff;
      default : o_tdata = i_tdata;
    endcase // case(state)


  assign  o_tvalid = state == RESIDUAL || (i_tvalid &&
                        (state != SAMPLE1 || state == SAMPLE1 && i_tlast));

  assign  i_tready = (o_tready && state != RESIDUAL);

  wire need_extra_line = state == SAMPLE1 || state == SAMPLE2 ||
                          (state == SAMPLE3 && ~odd);
  assign  o_tlast = state == RESIDUAL || (i_tlast & ~need_extra_line);

endmodule

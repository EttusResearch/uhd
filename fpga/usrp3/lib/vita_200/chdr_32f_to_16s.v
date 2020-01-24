//
// Copyright 2013 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//

module chdr_32f_to_16s #
(
  parameter BASE = 0
)
(
  input              clk,
  input              rst,

  // axi4 stream slave interface
  input [63:0]       i_tdata,
  input              i_tvalid,
  input              i_tlast,
  output             i_tready,

  // axi4 stream master interface
  output reg [63:0]  o_tdata,
  output             o_tvalid,
  output             o_tlast,
  input              o_tready,

  // settings bus slave interface
  input              set_stb,
  input [7:0]        set_addr,
  input [31:0]       set_data,

  output [63:0]      debug
);


  wire [31:0] float0 = i_tdata[63:32];
  wire [31:0] float1 = i_tdata[31:0];

  wire [15:0] fixed1_cur;
  wire [15:0] fixed0_cur;

  // Parametrize the converter as IEEE 754 single precision float to Q15
  xxf_to_xxs #
  (
    .FBITS(32),
    .MBITS(23),
    .EBITS(8),
    .RADIX(15),
    .QWIDTH(16)
  ) f2q0
  (
    .i_float(float0),
    .o_fixed(fixed0_cur)
  );

  // Parametrize the converter as IEEE 754 single precision float to Q15
  xxf_to_xxs #
  (
    .FBITS(32),
    .MBITS(23),
    .EBITS(8),
    .RADIX(15),
    .QWIDTH(16)
  ) f2q1
  (
    .i_float(float1),
    .o_fixed(fixed1_cur)
  );

  // As we need two cycles for one output cycle store the output in regs
  reg [15:0] fixed1_old;
  reg [15:0] fixed0_old;

  wire handshake_ok = o_tready & i_tvalid;

  always @ (posedge clk)
    if (rst)
      {fixed0_old, fixed1_old} <= {16'h0, 16'h0};
    else if (handshake_ok)
      {fixed0_old, fixed1_old} <= {fixed0_cur, fixed1_cur};

  // Make routing (SID) available via settings bus
  wire        set_sid;
  wire [15:0] new_sid_dst;

  setting_reg #
  (
    .my_addr(BASE),
    .width(17)
  ) new_destination
  ( .clk(clk),
    .rst(rst),
    .strobe(set_stb),
    .addr(set_addr),
    .in(set_data),
    .out({set_sid, new_sid_dst[15:0]}),
    .changed()
  );

  // Parse CHDR info
  wire        chdr_has_time = i_tdata[61];
  // CHDR has either 8 bytes of header or 16 if VITA time is included.
  wire [15:0] chdr_header_bytes = chdr_has_time ? 16 : 8;
  // Calculate size of samples input in bytes by taking CHDR size filed
  // and subtracting header length.
  wire [15:0] sample_byte_count_in = i_tdata[47:32] - chdr_header_bytes;
  // Calculate size of samples to be output by taking input size
  // and dividing by two as sizeof(Q15) = 2*sizeof(float)
  wire [15:0] sample_byte_count_out = sample_byte_count_in >> 1;
  // Calculate size of output CHDR packet by adding back header size to new
  // payload size.
  wire [15:0] output_chdr_pkt_size = sample_byte_count_out + chdr_header_bytes;

  localparam HEADER  = 2'd0;
  localparam TIME    = 2'd1;
  localparam PREPARE = 2'd2;
  localparam OUTPUT  = 2'd3;


  reg [1:0] state;

  always @(posedge clk)
    if (rst) begin
      state <= HEADER;
    end

    else case(state)
      HEADER:
        // In case we see a i_last we just wait for the
        // next header here, otherwise move on to the next states
        if (handshake_ok & !i_tlast) begin
          state <= chdr_has_time ? TIME : PREPARE;
        end

      TIME:
        if (handshake_ok) begin
          // If we get a premature end of burst go back
          // to searching for the start of a new packet.
          state <= i_tlast ? HEADER : PREPARE;
        end

      PREPARE:
        if (handshake_ok) begin
          state <= i_tlast ? HEADER : OUTPUT;
        end

      OUTPUT:
        if (handshake_ok) begin
          state <= i_tlast ? HEADER : PREPARE;
        end

      default:
        state <= HEADER;
    endcase

  always @(*)
    case(state)
      // Populate header with CHDR fields
      HEADER:
        o_tdata = {i_tdata[63:48], output_chdr_pkt_size,
                   set_sid ? {i_tdata[15:0], new_sid_dst[15:0]} : i_tdata[31:0]};
      TIME:
        o_tdata = i_tdata;
      PREPARE:
        // The bits [31:0] of o_tdata are useless. The header will take
        // care of this by setting the correct length.
        o_tdata = {fixed0_cur[15:0], fixed1_cur[15:0], 32'h0};
      OUTPUT:
        o_tdata = {fixed0_old[15:0], fixed1_old[15:0],
                   fixed0_cur[15:0], fixed1_cur[15:0]};
      default :
        o_tdata = i_tdata;
    endcase

  // Either the input is valid and is directly output (HEADER, TIME, EOB),
  // or we need to be in the 'OUTPUT' state ({fixed0_old, fixed1_old} contains correct old
  // line)
  assign o_tvalid = (i_tvalid && state != PREPARE) || i_tvalid && i_tlast;
  assign i_tready = o_tready || (state == PREPARE && !i_tlast);
  assign o_tlast  = i_tlast;

endmodule

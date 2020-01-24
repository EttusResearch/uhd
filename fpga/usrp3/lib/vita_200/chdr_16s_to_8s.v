//
// Copyright 2013, 2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
module chdr_16s_to_8s
#(
  parameter BASE=0
)
(
  input clk,
  input rst,

  // axi4 stream slave interface
  input [63:0]      i_tdata,
  input             i_tlast,
  input             i_tvalid,
  output            i_tready,

  // axi4 stream master interface
  output reg [63:0] o_tdata,
  output            o_tlast,
  output            o_tvalid,
  input             o_tready,

  // settings bus slave interface
  input             set_stb,
  input [7:0]       set_addr,
  input [31:0]      set_data,

  output [31:0] debug
);

  // split up for lazyness reasons
  wire [15:0] fixed0 = i_tdata[63:48];
  wire [15:0] fixed1 = i_tdata[47:32];
  wire [15:0] fixed2 = i_tdata[31:16];
  wire [15:0] fixed3 = i_tdata[15:0];

  wire [7:0] rounded_cur0;
  wire [7:0] rounded_cur1;
  wire [7:0] rounded_cur2;
  wire [7:0] rounded_cur3;

  // Parse CHDR info
  wire        chdr_has_time = i_tdata[61];
  // CHDR has either 8 bytes of header or 16 if VITA time is included.
  wire [15:0] chdr_header_bytes = chdr_has_time ? 16 : 8;
  // Calculate size of samples input in bytes by taking CHDR size filed
  // and subtracting header length.
  wire [15:0] sample_byte_count_in = i_tdata[47:32] - chdr_header_bytes;
  // Calculate size of samples to be output by taking input size
  // and dividing by two as sizeof(Q15) = 2*sizeof(Q7)
  wire [15:0] sample_byte_count_out = sample_byte_count_in >> 1;
  // Calculate size of output CHDR packet by adding back header size to new
  // payload size.
  wire [15:0] output_chdr_pkt_size = sample_byte_count_out + chdr_header_bytes;

  // Make routing (SID) available via settings bus
  wire        set_sid;
  wire [15:0] new_sid_dst;

  setting_reg #
  (
    .my_addr(BASE),
    .width(17)
  )
  new_destination
  (
    .clk(clk),
    .rst(rst),
    .strobe(set_stb),
    .addr(set_addr),
    .in(set_data),
    .out({set_sid, new_sid_dst[15:0]}),
    .changed()
  );

  wire handshake_ok = o_tready & i_tvalid;

  reg [7:0] rounded_old0;
  reg [7:0] rounded_old1;
  reg [7:0] rounded_old2;
  reg [7:0] rounded_old3;

  // respect your elders, yo
  always @ (posedge clk)
    if (rst)
      {rounded_old0, rounded_old1, rounded_old2, rounded_old3} <= 32'h0;
    else if (handshake_ok) begin
      {rounded_old0, rounded_old1} <= {rounded_cur0, rounded_cur1};
      {rounded_old2, rounded_old3} <= {rounded_cur2, rounded_cur3};
  end

  localparam HEADER  = 2'd0;
  localparam TIME    = 2'd1;
  localparam PREPARE = 2'd2;
  localparam OUTPUT  = 2'd3;

  reg [1:0] state;

  always @(posedge clk) begin
    if (rst)
      state <= HEADER;
    else case(state)
      HEADER:
        // In case we see a i_last we just wait for the
        // next header here, otherwise move on to the next states
        if (handshake_ok & !i_tlast)
          state <= chdr_has_time ? TIME : PREPARE;

      TIME:
        // If we get a premature end of burst go back
        // to searching for the start of a new packet
        if (handshake_ok)
          state <= i_tlast ? HEADER: PREPARE;

      PREPARE:
        if (handshake_ok)
          state <= i_tlast ? HEADER: OUTPUT;

      OUTPUT:
        if (handshake_ok)
          state <= i_tlast ? HEADER: PREPARE;

      default:
        state <= HEADER;

    endcase
  end

  round #
  (
    .bits_in(16),
    .bits_out(8)
  )
  round0
  (
    .in(fixed0),
    .out(rounded_cur0),
    .err()
  );

  round #
  (
    .bits_in(16),
    .bits_out(8)
  )
  round1
  (
    .in(fixed1),
    .out(rounded_cur1),
    .err()
  );

  round #
  (
    .bits_in(16),
    .bits_out(8)
  )
  round2
  (
    .in(fixed2),
    .out(rounded_cur2),
    .err()
  );

  round #
  (
    .bits_in(16),
    .bits_out(8))
  round3
  (
    .in(fixed3),
    .out(rounded_cur3),
    .err()
  );

  always @(*)
    case(state)
      HEADER:
        o_tdata = {i_tdata[63:48], output_chdr_pkt_size,
                   set_sid ? {i_tdata[15:0], new_sid_dst[15:0]} : i_tdata[31:0]};
      TIME:
        o_tdata = i_tdata;
      PREPARE:
        // Here the second half of this stuff is invalid, as the header will
        // take care of that by setting the correct length field,
        // we assign the same to simplify the mux
        o_tdata = {rounded_cur0, rounded_cur1, rounded_cur2, rounded_cur3,
                   rounded_cur0, rounded_cur1, rounded_cur2, rounded_cur3};
      OUTPUT:
        o_tdata = {rounded_old0, rounded_old1, rounded_old2, rounded_old3,
                   rounded_cur0, rounded_cur1, rounded_cur2, rounded_cur3};
      default:
        o_tdata = i_tdata;
    endcase

   assign o_tvalid = i_tvalid && (state != PREPARE || i_tlast);
   assign i_tready = o_tready || (state == PREPARE && !i_tlast);
   assign o_tlast  = i_tlast;

endmodule

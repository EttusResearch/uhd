//
// Copyright 2013 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//

module chdr_16s_to_32f #
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

  reg [1:0] state;
  localparam HEADER  = 2'd0;
  localparam TIME    = 2'd1;
  localparam ODD     = 2'd2;
  localparam EVEN    = 2'd3;

  // split up the input for lazyness reasons
  wire [15:0] fixed0 = i_tdata[63:48];
  wire [15:0] fixed1 = i_tdata[47:32];
  wire [15:0] fixed2 = i_tdata[31:16];
  wire [15:0] fixed3 = i_tdata[15:0];

  // mux the inputs
  wire [15:0] fixed_muxed0 = (state == ODD) ? fixed0 : fixed2;
  wire [15:0] fixed_muxed1 = (state == ODD) ? fixed1 : fixed3;

  wire [31:0] float0;
  wire [31:0] float1;

  // Parametrize the converter as Q15 to IEEE 754 single precision float
  xxs_to_xxf #
  (
    .FBITS(32),
    .MBITS(23),
    .EBITS(8),
    .RADIX(15),
    .QWIDTH(16)
  ) q2f0
  (
    .i_fixed(fixed_muxed0),
    .o_float(float0)
  );

  // Parametrize the converter as Q15 to IEEE 754 single precision float
  xxs_to_xxf #
  (
    .FBITS(32),
    .MBITS(23),
    .EBITS(8),
    .RADIX(15),
    .QWIDTH(16)
  ) q2f1
  (
    .i_fixed(fixed_muxed1),
    .o_float(float1)
  );

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
  // Calculate size of samples input in bytes by taking CHDR size field
  // and subtracting header length.
  wire [15:0] sample_byte_count_in = i_tdata[47:32] - chdr_header_bytes;
  // Calculate size of samples to be EVEN by taking input size
  // and multiplying by two as sizeof(float) = 2 * sizeof(Q15)
  wire [15:0] sample_byte_count_out = sample_byte_count_in << 1;
  // Calculate size of output CHDR packet by adding back header size to new
  // payload size.
  wire [15:0] output_chdr_pkt_size = sample_byte_count_out + chdr_header_bytes;

  reg end_on_odd;

  always @(posedge clk)
    if (rst) begin
      state <= HEADER;
      end_on_odd <= 1'b0;
    end

    else case(state)
      HEADER:
        if (o_tready && i_tvalid) begin
          state <= chdr_has_time ? TIME : ODD;
          end_on_odd <= |sample_byte_count_in[2:0];
        end

      TIME:
        if (o_tready && i_tvalid) begin
          // If we get a premature end of burst go back
          // to searching for the start of a new packet.
          state <= i_tlast ? HEADER : ODD;
        end

      ODD:
        if (o_tready && i_tvalid) begin
          state <= (i_tlast && end_on_odd) ? HEADER : EVEN;
        end

      EVEN:
        if (o_tready && i_tvalid) begin
          state <= i_tlast ? HEADER : ODD;
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
      ODD:
        o_tdata = {float0, float1};
      EVEN:
        o_tdata = {float0, float1};
      default :
        o_tdata = i_tdata;
    endcase

  assign o_tvalid  = i_tvalid;
  assign i_tready  = (o_tready && state != ODD) || (i_tlast && end_on_odd);
  assign o_tlast   = i_tlast && ((state == EVEN) || (state == ODD) && end_on_odd);

endmodule

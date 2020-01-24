//
// Copyright 2013, 2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//

module chdr_8s_to_16s #
(
  parameter BASE=0
)
(
  input             clk,
  input             rst,

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

  output [31:0]     debug
);

  // split up the input for lazyness reasons
  wire [7:0] fixed0 = i_tdata[63:56];
  wire [7:0] fixed1 = i_tdata[55:48];
  wire [7:0] fixed2 = i_tdata[47:40];
  wire [7:0] fixed3 = i_tdata[39:32];
  wire [7:0] fixed4 = i_tdata[31:24];
  wire [7:0] fixed5 = i_tdata[23:16];
  wire [7:0] fixed6 = i_tdata[15:8];
  wire [7:0] fixed7 = i_tdata[7:0];

  // Parse CHDR info
  wire chdr_has_time = i_tdata[61];
  // CHDR has either 8 bytes of header or 16 if VITA time is included.
  wire [15:0] chdr_header_bytes = chdr_has_time ? 16 : 8;
  // Calculate size of samples input in bytes by taking CHDR size field
  // and subtracting header length.
  wire [15:0] sample_byte_count_in = i_tdata[47:32] - chdr_header_bytes;
  // Calculate size of samples by taking input size
  // and multiplying by two
  wire [15:0] sample_byte_count_out = sample_byte_count_in << 1;
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

  wire handshake_ok = i_tvalid & o_tready;

  //state declarations
  localparam HEADER  = 2'd0;
  localparam TIME    = 2'd1;
  localparam ODD     = 2'd2;
  localparam EVEN    = 2'd3;

  reg [1:0] state;
  reg       end_on_odd;

  always @(posedge clk) begin
   if (rst) begin
     state <= HEADER;
     end_on_odd <= 1'b0;
   end
   else case(state)
     HEADER:
       // if we get a premature end of burst,
       // we just stick around for the next header
       if (handshake_ok & !i_tlast) begin
         state <= (i_tdata[61])? TIME : ODD;
         end_on_odd <= (i_tdata[34:32] > 0) && (i_tdata[34:32] < 5);
       end

     TIME:
       // if we get a premature i_tlast we bail out, else proceed
       if (handshake_ok)
         state <= (i_tlast)? HEADER: ODD;

     ODD:
       if (handshake_ok)
         state <= (i_tlast & end_on_odd) ? HEADER : EVEN;

     EVEN:
       if (handshake_ok)
         state <= (i_tlast) ? HEADER: ODD;

     default:
       state <= HEADER;

    endcase
end

  always @(*)
    case(state)
      HEADER:
        o_tdata = {i_tdata[63:48], output_chdr_pkt_size,
                    set_sid ? {i_tdata[15:0], new_sid_dst[15:0]} : i_tdata[31:0]};
      TIME:
        o_tdata = i_tdata;
      ODD:
        o_tdata = {fixed0, 8'h0, fixed1, 8'h0, fixed2, 8'h0, fixed3, 8'h0};
      EVEN:
        o_tdata = {fixed4, 8'h0, fixed5, 8'h0, fixed6, 8'h0, fixed7, 8'h0};
      default:
        o_tdata = i_tdata;
    endcase

   assign o_tvalid = i_tvalid;
   assign i_tready = o_tready && ((state != ODD) || (i_tlast & end_on_odd));
   assign o_tlast  = i_tlast && ((state == EVEN) || ((state == ODD) & end_on_odd));

endmodule

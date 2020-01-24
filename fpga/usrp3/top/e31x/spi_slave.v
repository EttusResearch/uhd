//
// Copyright 2015 Ettus Research LLC
//

`ifndef LOG2
`define LOG2(N) (\
  N < 2 ? 0 : \
  N < 4 ? 1 : \
  N < 8 ? 2 : \
  N < 16 ? 3 : \
  N < 32 ? 4 : \
  N < 64 ? 5 : \
  N < 128 ? 6 : \
  N < 256 ? 7 : \
  N < 512 ? 8 : \
  N < 1024 ? 9 : \
  10)
`endif

module spi_slave
#(
  parameter DEPTH = 64
)
(
  // sys connect
  input              clk,
  input              rst,

  // spi slave port
  input              ss,
  input              mosi,
  output             miso,
  input              sck,

  // parallel data io port
  output             parallel_stb,
  input [DEPTH-1:0]  parallel_din,
  output [DEPTH-1:0] parallel_dout
);
  reg mosi_d, mosi_q;
  reg ss_d, ss_q;
  reg sck_d, sck_q;
  reg sck_old_d, sck_old_q;
  reg miso_d, miso_q;

  reg [DEPTH-1:0] data_d, data_q;
  reg             parallel_stb_d, parallel_stb_q;
  reg [`LOG2(DEPTH)-1:0] bit_ct_d, bit_ct_q;
  reg [DEPTH-1:0] parallel_dout_d, parallel_dout_q;

  assign miso = miso_q;
  assign parallel_stb = parallel_stb_q;
  assign parallel_dout = parallel_dout_q;

  always @(*) begin
    ss_d            = ss;
    mosi_d          = mosi;
    miso_d          = miso_q;
    sck_d           = sck;
    sck_old_d       = sck_q;
    data_d          = data_q;
    parallel_stb_d  = 1'b0;
    bit_ct_d        = bit_ct_q;
    parallel_dout_d = parallel_dout_q;

    if (ss_q) begin
      bit_ct_d = 'h0;
      data_d   = parallel_din;
      miso_d   = data_q[DEPTH-1];
    end
    else begin
      if (!sck_old_q && sck_q) begin // rising edge
        data_d   = {data_q[DEPTH-1-1:0], mosi_q};
        bit_ct_d = bit_ct_q + 1'b1;
        if (bit_ct_q == (DEPTH - 1)) begin
          parallel_dout_d = {data_q[DEPTH-1-1:0], mosi_q};
          parallel_stb_d          = 1'b1;
          data_d          = parallel_din;
        end
      end
      else if (sck_old_q && !sck_q) begin // falling edge
        miso_d = data_q[DEPTH-1];
      end
    end
  end

  always @(posedge clk) begin
    if (rst) begin
      parallel_stb_q          <= 1'b0;
      bit_ct_q        <= 'h0;
      parallel_dout_q <= 'h0;
      miso_q          <= 1'b1;
    end else begin
      parallel_stb_q  <= parallel_stb_d;
      bit_ct_q        <= bit_ct_d;
      parallel_dout_q <= parallel_dout_d;
      miso_q          <= miso_d;
    end

    sck_q     <= sck_d;
    mosi_q    <= mosi_d;
    ss_q      <= ss_d;
    data_q    <= data_d;
    sck_old_q <= sck_old_d;
  end

endmodule

//
// Copyright 2013 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//

`define log2(N) ( N < 2    ? 0 : \
                  N < 4    ? 1 : \
                  N < 8    ? 2 : \
                  N < 16   ? 3 : \
                  N < 32   ? 4 : \
                  N < 64   ? 5 : \
                  N < 128  ? 6 : \
                  N < 256  ? 7 : \
                  N < 512  ? 8 : \
                  N < 1024 ? 9 : \
                             10 \
                )

module priority_encoder
#(
  parameter WIDTH = 16
)
(
  input  [WIDTH-1:0]        in,
  output [`log2(WIDTH)-1:0] out
);

  wire [WIDTH-1:0] one_hot;

  // the priority encoder spits out the position
  // of the leading bit as one hot coding
  priority_encoder_one_hot #
  (
    .WIDTH(WIDTH)
  )
  prio_one_hot0
  (
    .in(in),
    .out(one_hot)
  );

  // binary encoder turns the one hot coding
  // into binary encoding
  binary_encoder #
  (
    .WIDTH(WIDTH)
  )
  binary_enc0
  (
    .in(one_hot),
    .out(out)
  );
endmodule

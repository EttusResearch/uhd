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

module binary_encoder
#(
      parameter SIZE = 16
)
(
       input [SIZE-1:0] in,
       output [`log2(SIZE)-1:0] out
);

  genvar m,n;

  generate
  // Loop enough times to represent the total number of input bits as an encoded value
  for (m = 0; m <= `log2(SIZE-1); m = m + 1)  begin: expand_or_tree
    wire [SIZE-1:0] encoding;
     // Build enable mask by iterating through every input bit.
     for (n = 0; n < SIZE ; n = n + 1) begin: encode_this_bit
       assign encoding[n] = n[m];
     end
     // OR tree for this output bit with appropriate bits enabled.
     assign out[m] = |(encoding & in);
   end
 endgenerate

endmodule // binary_encoder

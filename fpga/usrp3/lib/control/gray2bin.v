//
// Copyright 2016 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
//
// Gray: {a,b,c,d}
// Bits: {a,a^b,a^b^c,a^b^c^d}
//
module gray2bin #(
    parameter WIDTH = 8)
(
    input [WIDTH-1:0] gray,
    output reg [WIDTH-1:0] bin
);

    integer i;
    always @(*) begin
        bin[WIDTH-1] = gray[WIDTH-1];
        for (i = WIDTH-2; i >= 0; i = i - 1) begin
            bin[i] = bin[i+1] ^ gray[i];
        end
    end

endmodule

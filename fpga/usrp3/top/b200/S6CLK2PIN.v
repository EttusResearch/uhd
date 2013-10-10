module S6CLK2PIN
(
    input I,
    output O
);

    ODDR2 #(
           .DDR_ALIGNMENT("NONE"), // to "NONE", "C0" or "C1"
           .INIT(1'b0),            // output to 1'b0 or 1'b1
           .SRTYPE("ASYNC"))        // set/reset "SYNC" or "ASYNC"

         ODDR2_S6CLK2PIN
           (
        .Q(O),  // 1-bit DDR output data
        .C0(I), // 1-bit clock input
        .C1(~I), // 1-bit clock input
        .CE(1'b1), // 1-bit clock enable input
        .D0(1'b1), // 1-bit data input (associated with C0)
        .D1(1'b0), // 1-bit data input (associated with C1)
        .R(1'b0),  // 1-bit reset input
        .S(1'b0) );// 1-bit set input

endmodule //S6CLK2PIN

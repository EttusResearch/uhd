//
// Copyright 2015 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//

module pps_generator #(
   parameter CLK_FREQ   = 32'd10_000_000, //Min:10kHz, Max:4GHz
   parameter DUTY_CYCLE = 25,
   parameter PIPELINE   = "NONE" // Optional register on output? {"NONE", "OUT"}
) (
   input  clk,
   input  reset,
   output pps
);
    reg [31:0] count = 32'h0;

    always @(posedge clk) begin
        if (reset) begin
            count <= 32'd0;
        end else if (count >= CLK_FREQ - 1) begin
            count <= 32'd0;
        end else begin
            count <= count + 32'd1;
        end
    end

    wire pps_int;
    assign pps_int = (count < ((CLK_FREQ / 100) * DUTY_CYCLE));

    generate
      if (PIPELINE == "OUT") begin : gen_pipeline_out
        // create output register and assign to output
        reg pps_reg = 1'b0;
        assign pps = pps_reg;

        always @(posedge clk) begin
          if (reset) begin
            pps_reg <= 1'b0;
          end else begin
            pps_reg <= pps_int;
          end
        end
      end else begin : gen_pipeline_none
        // no output register
        assign pps = pps_int;
      end
    endgenerate
endmodule //pps_generator

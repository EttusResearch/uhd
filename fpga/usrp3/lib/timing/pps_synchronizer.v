//
// Copyright 2015 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//

module pps_synchronizer
(
   input      ref_clk,
   input      timebase_clk,
   input      pps_in,
   output     pps_out,
   output reg pps_count
);
   wire pps_refclk;
   reg  pps_out_del;

   //The input pps is treated an as async signal and is first synchronized
   //to a common reference clock shared between multiple devices. It is then
   //synchronized to the timebase clock which counts up the VITA time.
   //The reference clock frequency must be equal to or smaller than the 
   //timebase clock frequency to remove any time ambiguity.

   //For robust synchronization across FPGA builds, the async delay for pps_in
   //must be constrained along with the clock delay (or meet static timing there).
   //The path length between the two synchronizers must also be constrained
   synchronizer #(.INITIAL_VAL(1'b0), .FALSE_PATH_TO_IN(0)) pps_sync_refclk_inst (
      .clk(ref_clk), .rst(1'b0 /* no reset */), .in(pps_in), .out(pps_refclk));

   synchronizer #(.INITIAL_VAL(1'b0), .FALSE_PATH_TO_IN(0)) pps_sync_tbclk_inst (
      .clk(timebase_clk), .rst(1'b0 /* no reset */), .in(pps_refclk), .out(pps_out));

   //Implement a 1-bit counter to detect PPS edges
   always @(posedge timebase_clk)
      pps_out_del <= pps_out;

   always @(posedge timebase_clk)
      if (~pps_out_del && pps_out)
         pps_count <= ~pps_count;

endmodule //pps_synchronizer

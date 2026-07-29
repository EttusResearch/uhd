//
// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: cic_integrator_all_tb
//
// Description:
//
//   Sweeps cic_integrator_tb over the design space:
//     ORDER  ∈ {1, 2, 4}
//     SPC    ∈ {1, 2, 4, 8}   (12 configurations total)
//

`default_nettype none

module cic_integrator_all_tb;

  // ORDER=1 ----------------------------------------------------------------
  cic_integrator_tb #(.SPC(1), .ACCUM_W(48), .SAMP_W(32), .ORDER(1)) tb_o1_s1 ();
  cic_integrator_tb #(.SPC(2), .ACCUM_W(48), .SAMP_W(32), .ORDER(1)) tb_o1_s2 ();
  cic_integrator_tb #(.SPC(4), .ACCUM_W(48), .SAMP_W(32), .ORDER(1)) tb_o1_s4 ();
  cic_integrator_tb #(.SPC(8), .ACCUM_W(48), .SAMP_W(32), .ORDER(1)) tb_o1_s8 ();

  // ORDER=2 ----------------------------------------------------------------
  cic_integrator_tb #(.SPC(1), .ACCUM_W(48), .SAMP_W(32), .ORDER(2)) tb_o2_s1 ();
  cic_integrator_tb #(.SPC(2), .ACCUM_W(48), .SAMP_W(32), .ORDER(2)) tb_o2_s2 ();
  cic_integrator_tb #(.SPC(4), .ACCUM_W(48), .SAMP_W(32), .ORDER(2)) tb_o2_s4 ();
  cic_integrator_tb #(.SPC(8), .ACCUM_W(48), .SAMP_W(32), .ORDER(2)) tb_o2_s8 ();

  // ORDER=4 ----------------------------------------------------------------
  cic_integrator_tb #(.SPC(1), .ACCUM_W(48), .SAMP_W(32), .ORDER(4)) tb_o4_s1 ();
  cic_integrator_tb #(.SPC(2), .ACCUM_W(48), .SAMP_W(32), .ORDER(4)) tb_o4_s2 ();
  cic_integrator_tb #(.SPC(4), .ACCUM_W(48), .SAMP_W(32), .ORDER(4)) tb_o4_s4 ();
  cic_integrator_tb #(.SPC(8), .ACCUM_W(48), .SAMP_W(32), .ORDER(4)) tb_o4_s8 ();

endmodule : cic_integrator_all_tb

`default_nettype wire

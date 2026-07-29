//
// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: dsp48_accum
//
// Description:
//
//   Thin wrapper around a single DSP48E1 primitive implementing a
//   registered accumulator with PCIN cascade and C inputs.
//
//   OPMODE is switched each cycle via OPMODEREG:
//     valid = 1  → OPMODE = 7'h1E  (Z=PCIN, Y=C, X=P)  → P <= P + PCIN + C
//     valid = 0  → OPMODE = 7'h02  (Z=0,    Y=0, X=P)  → P <= P  (hold)
//
//   OPMODEREG = 1 registers the mux select one cycle before the adder, which
//   aligns with CREG = 1 so both the data and the opmode arrive at the ALU
//   in the same clock cycle.
//
//   Pipeline registers:
//     PREG     = 1  (required for P-feedback accumulation; also the output register)
//     CREG     = 1  (C input registered inside DSP)
//     OPMODEREG= 1  (OPMODE registered; same 1-cycle latency as CREG, so both
//                    arrive at the adder together)
//     AREG/BREG = 0  (A:B inputs not used)
//     MREG = 0       (multiplier not used)
//
//   Reset / clock-enable:
//     rst   → RSTP   : synchronous reset of PREG (clears accumulator to 0)
//     en    → CEC, CEP, CECTRL : clock-enable for CREG, PREG and OPMODEREG
//     valid → OPMODE : selects accumulate (7'h1E) or hold (7'h02); registered
//                      by OPMODEREG before reaching the ALU
//

module dsp48_accum (
  input  logic        clk,
  input  logic        rst,
  input  logic        en,
  input  logic        valid,
  input  logic [47:0] c,
  input  logic [47:0] pcin,
  output logic [47:0] pcout,
  output logic [47:0] p
);

  // OPMODE[6:4]=Z=001(PCIN), [3:2]=Y=11(C), [1:0]=X=10(P feedback)  → accumulate
  // OPMODE[6:4]=Z=000,        [3:2]=Y=00,    [1:0]=X=10(P feedback)  → hold P
  logic [6:0] opmode;
  assign opmode = valid ? 7'h1E : 7'h02;

  DSP48E1 #(
    .A_INPUT("DIRECT"),
    .B_INPUT("DIRECT"),
    .USE_DPORT("FALSE"),
    .USE_MULT("NONE"),
    .USE_SIMD("ONE48"),
    .AUTORESET_PATDET("NO_RESET"),
    .USE_PATTERN_DETECT("NO_PATDET"),
    .SEL_MASK("MASK"),
    .SEL_PATTERN("PATTERN"),
    .MASK(48'h3fffffffffff),
    .PATTERN(48'h000000000000),
    .ACASCREG(0),
    .ADREG(0),
    .ALUMODEREG(0),
    .AREG(0),
    .BCASCREG(0),
    .BREG(0),
    .CARRYINREG(0),
    .CARRYINSELREG(0),
    .CREG(1),
    .DREG(0),
    .INMODEREG(0),
    .MREG(0),
    .OPMODEREG(1),
    .PREG(1)
  ) dsp_i (
    // Clock and control
    .CLK(clk),
    .CEP(en),
    .RSTP(rst),
    // Operation mode
    .OPMODE(opmode),
    .ALUMODE(4'h0),
    .INMODE(5'h0),
    .CARRYINSEL(3'h0),
    .CARRYIN(1'b0),
    // Data inputs (A:B not used)
    .A(30'b0),
    .B(18'b0),
    .C(c),
    .D(25'b0),
    // Cascade inputs
    .ACIN(30'b0),
    .BCIN(18'b0),
    .PCIN(pcin),
    .CARRYCASCIN(1'b0),
    .MULTSIGNIN(1'b0),
    // Resets
    .RSTA(1'b0),
    .RSTALLCARRYIN(1'b0),
    .RSTALUMODE(1'b0),
    .RSTB(1'b0),
    .RSTC(1'b0),
    .RSTCTRL(1'b0),
    .RSTD(1'b0),
    .RSTINMODE(1'b0),
    .RSTM(1'b0),
    // Clock enables
    .CEA1(1'b0),
    .CEA2(1'b0),
    .CEAD(1'b0),
    .CEALUMODE(1'b1),
    .CEB1(1'b0),
    .CEB2(1'b0),
    .CEC(en),
    .CECARRYIN(1'b0),
    .CECTRL(en),
    .CED(1'b0),
    .CEINMODE(1'b0),
    .CEM(1'b0),
    // Outputs
    .PCOUT(pcout),
    .P(p),
    .ACOUT(),
    .BCOUT(),
    .CARRYCASCOUT(),
    .MULTSIGNOUT(),
    .OVERFLOW(),
    .UNDERFLOW(),
    .PATTERNDETECT(),
    .PATTERNBDETECT(),
    .CARRYOUT()
  );

endmodule : dsp48_accum

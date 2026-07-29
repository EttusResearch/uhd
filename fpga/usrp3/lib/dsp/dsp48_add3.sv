//
// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: dsp48_add3
//
// Description:
//
//   Thin wrapper around a single DSP48E1 primitive implementing a
//   registered three-input adder with optional PCIN cascade input.
//
//     USE_PCIN=1 (default):  P = PCIN + C + A:B   (OPMODE = 7'h1F)
//     USE_PCIN=0:            P =        C + A:B   (OPMODE = 7'h33)
//
//   Pipeline registers enabled on A/B (AREG=1), C (CREG=1), and P (PREG=1).
//   Total latency from input to P output: 2 clock-enabled cycles.
//
//   For the first stage in a chain set USE_PCIN=0.
//

module dsp48_add3 #(
  bit AB_CASCADE = 0,  // 0 = DIRECT (use A/B ports), 1 = CASCADE (use ACIN/BCIN ports)
  bit USE_PCIN   = 1   // 1 = include PCIN in sum, 0 = two-input adder (C + A:B)
) (
  input  logic        clk,
  input  logic        rst,
  input  logic        en,
  input  logic [47:0] ab,
  input  logic [47:0] c,
  input  logic [47:0] pcin,
  output logic [47:0] pcout,
  output logic [47:0] abcout,
  output logic [47:0] p
);

  // USE_PCIN=1: OPMODE[6:4]=Z=001(PCIN), [3:2]=Y=11(C), [1:0]=X=11(A:B) → PCIN+C+A:B
  // USE_PCIN=0: OPMODE[6:4]=Z=011(C),    [3:2]=Y=00,    [1:0]=X=11(A:B) →      C+A:B
  localparam logic [6:0] OPMODE   = USE_PCIN ? 7'h1F : 7'h33;
  localparam             AB_INPUT = AB_CASCADE ? "CASCADE" : "DIRECT";

  DSP48E1 #(
    .A_INPUT(AB_INPUT),
    .B_INPUT(AB_INPUT),
    .USE_DPORT("FALSE"),
    .USE_MULT("NONE"),
    .USE_SIMD("ONE48"),
    .AUTORESET_PATDET("NO_RESET"),
    .USE_PATTERN_DETECT("NO_PATDET"),
    .SEL_MASK("MASK"),
    .SEL_PATTERN("PATTERN"),
    .MASK(48'h3fffffffffff),
    .PATTERN(48'h000000000000),
    .ACASCREG(1),
    .ADREG(0),
    .ALUMODEREG(0),
    .AREG(1),
    .BCASCREG(1),
    .BREG(1),
    .CARRYINREG(0),
    .CARRYINSELREG(0),
    .CREG(1),
    .DREG(0),
    .INMODEREG(0),
    .MREG(0),
    .OPMODEREG(0),
    .PREG(1)
  ) dsp_i (
    // Clock and control
    .CLK(clk),
    .CEP(en),
    .RSTP(rst),
    // Operation mode
    .OPMODE(OPMODE),
    .ALUMODE(4'h0),
    .INMODE(5'h0),
    .CARRYINSEL(3'h0),
    .CARRYIN(1'b0),
    // Data inputs
    .A(AB_CASCADE ? 30'b0 : ab[47:18]),
    .B(AB_CASCADE ? 18'b0 : ab[17:0]),
    .C(c),
    .D(25'b0),
    // Cascade inputs
    .ACIN(AB_CASCADE ? ab[47:18] : 30'b0),
    .BCIN(AB_CASCADE ? ab[17:0] : 18'b0),
    .PCIN(USE_PCIN ? pcin : 48'b0),
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
    .CEA1(en),
    .CEA2(en),
    .CEAD(1'b1),
    .CEALUMODE(1'b1),
    .CEB1(en),
    .CEB2(en),
    .CEC(en),
    .CECARRYIN(1'b1),
    .CECTRL(1'b1),
    .CED(1'b0),
    .CEINMODE(1'b1),
    .CEM(1'b0),
    // Outputs
    .PCOUT(pcout),
    .P(p),
    .ACOUT(abcout[47:18]),
    .BCOUT(abcout[17:0]),
    .CARRYCASCOUT(),
    .MULTSIGNOUT(),
    .OVERFLOW(),
    .UNDERFLOW(),
    .PATTERNDETECT(),
    .PATTERNBDETECT(),
    .CARRYOUT()
  );

endmodule : dsp48_add3

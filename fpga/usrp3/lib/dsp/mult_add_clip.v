// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
// Write xilinx DSP48E1 primitive for mult-add-clip (signed)

`default_nettype none

module mult_add_clip #(
  parameter WIDTH_A=25,  // Max 25
  parameter BIN_PT_A=24,
  parameter WIDTH_B=18,  // Max 18
  parameter BIN_PT_B=17,
  parameter WIDTH_C=43,  // Max 43

  // Min (47-WIDTH_C-1)+BIN_PT_A+BIN_PT_B,
  // Max WIDTH_C-1+BIN_PT_A+BIN_PT_B
  parameter BIN_PT_C=42,

  parameter WIDTH_O=43,  // Max 43-(BIN_PT_A+BIN_PT_B-BIN_PT_O)
  parameter BIN_PT_O=42,
  parameter LATENCY=2    // Maximum is 4
) (
  input wire clk,
  input wire reset,
  input wire CE, // Ordinarily set to 1'b1
  input wire [WIDTH_A-1:0] A,
  input wire [WIDTH_B-1:0] B,
  input wire [WIDTH_C-1:0] C,
  output reg  [WIDTH_O-1:0] O
);
  // DSP operations:
  // O = clip(A * B + C)
  // 
  // Mux settings:
  // X,Y (01,01) = M
  // Z (011) = C
 
  localparam MREG_IN = (LATENCY >= 1) ? 1 : 0;
  localparam CREG_IN = MREG_IN;
  localparam PREG_IN = (LATENCY >= 2) ? 1 : 0;
  localparam A2REG_IN = (LATENCY >= 3) ? 1 : 0;
  localparam A1REG_IN = (LATENCY == 4) ? 1 : 0;
  localparam AREG_IN = A1REG_IN + A2REG_IN;
  // See OPMODE Control Bits Settings, Table 2-7,2-8,2-9
  localparam ZMUX_C = 3'b011;
  localparam YMUX_M = 2'b01;
  localparam XMUX_M = 2'b01;
  localparam [6:0] OPMODE = {ZMUX_C, YMUX_M, XMUX_M};

  // A_IN is 25 bits; B_IN is 18 bits. Product M's binary point shifts:
  localparam BIN_PT_M = BIN_PT_A+(25-WIDTH_A) + BIN_PT_B+(18-WIDTH_B);

  // Calculate shift for C to align binary point to A*B product (M)
  // Determine top and bottom indices of C (in C_IN), normalized to M
  // Divide by 2**BIN_PT_C then multiply up by 2**BIN_PT_M
  localparam C_TOP = WIDTH_C-1 - BIN_PT_C + BIN_PT_M;
  localparam C_BOT = 0 - BIN_PT_C + BIN_PT_M;
  // Determine number of sign-extended bits above C_TOP
  localparam C_EXT = 47 - C_TOP;

  // P is a 43-bit fixed point number with bin pt BIN_PT_M
  // O is extracted from those bits
  // Sign extend if more bits to left of bin pt
  localparam O_EXT = ((WIDTH_O-BIN_PT_O) > (43-BIN_PT_M)) ?
                      (WIDTH_O-BIN_PT_O) - (43-BIN_PT_M) : 0;
  // If extending, use highest bit of P, else extract bits based on bin pt
  localparam P_TOP = (O_EXT > 0) ? 42 :
                     (42 + (WIDTH_O-BIN_PT_O) - (43-BIN_PT_M));
  // Pad bottom of O if remaining P not enough bits
  localparam O_PAD = (WIDTH_O > P_TOP+1) ? (WIDTH_O-P_TOP-1) : 0;
  // If padding O, grab lowest bit of P, else determine based on O's width
  localparam P_BOT = (O_PAD > 0) ? 0 : (P_TOP+1-WIDTH_O);

  //------------------------------------------------
  // Normalize C input to A*B product's binary point
  //------------------------------------------------
  function automatic [47:0] align_c;
    input [WIDTH_C-1:0] c;
    begin
      // Do sign extension
      if (C_EXT > 0) begin
        align_c[47 -: C_EXT] = {C_EXT{c[WIDTH_C-1]}};
      end
      if (C_BOT < 0) begin
        // Chop off lower bits of C
        align_c[C_TOP:0] = c[WIDTH_C-1:(-C_BOT)];
      end else begin
        // Place C and zero pad if necessary
        align_c[C_TOP:C_BOT] = c;
        if (C_BOT > 0) begin
          align_c[C_BOT-1:0] = {C_BOT{1'b0}};
        end
      end
    end
  endfunction

  wire [24:0] A_IN = (WIDTH_A < 25) ? { A, {(25-(WIDTH_A)){1'b0}}} : A;
  wire [17:0] B_IN = (WIDTH_B < 18) ? { B, {(18-(WIDTH_B)){1'b0}}} : B;
  wire [47:0] C_IN;
  wire [47:0] P_OUT;

  //--------------------------------------------------
  // C needs more pipeline registers at higher latency
  //--------------------------------------------------
  generate if (AREG_IN > 0) begin
    reg [AREG_IN*WIDTH_C-1:0] c_r;

    if (AREG_IN > 1) begin
      always @ (posedge clk)
      begin
        if (CE) begin
          c_r <= {c_r[0 +: (AREG_IN-1)*WIDTH_C], C};
        end
      end
    end else begin
      always @ (posedge clk)
      begin
        if (CE) begin
          c_r <= C;
        end
      end
    end

    wire [WIDTH_C-1:0] c_pre = c_r[AREG_IN*WIDTH_C-1 -: WIDTH_C];
    assign C_IN = align_c(c_pre);
  end else begin
    assign C_IN = align_c(C);
  end endgenerate

  //----------------------------------------------
  // Track signs for overflow/underflow processing
  //----------------------------------------------
  reg  [LATENCY-1:0]     mult_sign;
  reg  [LATENCY-1:0]     c_sign;
  wire                   bin_pt_overflow;
  wire                   adder_overflow;
  wire [WIDTH_O-1:0]     p_extract;

  generate if (LATENCY > 1) begin
    always @ (posedge clk)
    begin
      if (CE) begin
        mult_sign <= {mult_sign[0 +: LATENCY-1], A[WIDTH_A-1] ^ B[WIDTH_B-1]};
        c_sign <= {c_sign[0 +: LATENCY-1], C[WIDTH_C-1]};
      end
    end
  end else begin
    always @ (posedge clk)
    begin
      if (CE) begin
        mult_sign <= A[WIDTH_A-1] ^ B[WIDTH_B-1];
        c_sign <= C[WIDTH_C-1];
      end
    end
  end endgenerate

  assign adder_overflow = (mult_sign[LATENCY-1] == c_sign[LATENCY-1]) &&
                          (P_OUT[42] != c_sign[LATENCY-1]);

  //----------------------------------------------
  // Extract renormalized bits from P_OUT
  //----------------------------------------------
  generate 
    if (P_TOP < 42) begin
      assign bin_pt_overflow = (|P_OUT[42:P_TOP]) != (&P_OUT[42:P_TOP]);
    end else begin
      assign bin_pt_overflow = 1'b0;
    end

    if (O_EXT > 0) begin
      assign p_extract[WIDTH_O-1 -: O_EXT] = {O_EXT{P_OUT[42]}};
    end

    if (O_PAD > 0) begin
      assign p_extract[O_PAD-1:0] = {O_PAD{1'b0}};
    end
  endgenerate

  assign p_extract[WIDTH_O-1-O_EXT:O_PAD] = P_OUT[P_TOP:P_BOT];

  //----------------------------------
  // Clip if underflowed or overflowed
  //----------------------------------
  always @ (*)
  begin
    if (bin_pt_overflow || adder_overflow) begin
      O <= {c_sign[LATENCY-1], {WIDTH_O-1{!c_sign[LATENCY-1]}}};
    end else begin
      O <= p_extract;
    end
  end

  
  DSP48E1 #(
    .ACASCREG(AREG_IN),
    .AREG(AREG_IN),
    .ADREG(0),
    .DREG(0),
    .BCASCREG(AREG_IN),
    .BREG(AREG_IN),
    .MREG(MREG_IN),
    .CREG(CREG_IN),
    .PREG(PREG_IN)
  ) DSP48_inst (
    // Outputs
    .ACOUT(),
    .BCOUT(),
    .CARRYCASCOUT(),
    .CARRYOUT(),
    .MULTSIGNOUT(),
    .OVERFLOW(),
    .P(P_OUT),
    .PATTERNBDETECT(),
    .PATTERNDETECT(),
    .PCOUT(),
    .UNDERFLOW(),

    // Inputs
    .A({5'b0,A_IN}),
    .ACIN(30'b0),
    .ALUMODE(4'b0000),
    .B(B_IN),
    .BCIN(18'b0),
    .C(C_IN),
    .CARRYCASCIN(1'b0),
    .CARRYIN(1'b0),
    .CARRYINSEL(3'b0),
    .CEA1(CE),
    .CEA2(CE),
    .CEAD(1'b0),
    .CEALUMODE(1'b1),
    .CEB1(CE),
    .CEB2(CE),
    .CEC(CE),
    .CECARRYIN(CE),
    .CECTRL(CE),
    .CED(1'b0),
    .CEINMODE(CE),
    .CEM(CE),
    .CEP(CE),
    .CLK(clk),
    .D({25{1'b1}}),
    .INMODE(5'b0),
    .MULTSIGNIN(1'b0),
    .OPMODE(OPMODE),
    .PCIN(48'b0),
    .RSTA(reset),
    .RSTALLCARRYIN(reset),
    .RSTALUMODE(reset),
    .RSTB(reset),
    .RSTC(reset),
    .RSTD(reset),
    .RSTCTRL(reset),
    .RSTINMODE(reset),
    .RSTM(reset),
    .RSTP(reset)
  );

endmodule // mult_add_clip
`default_nettype wire

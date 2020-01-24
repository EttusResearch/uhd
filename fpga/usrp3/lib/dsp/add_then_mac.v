//
// Copyright 2015 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//

`timescale  1 ps / 1 ps

//
// Implements acc=((a+d)*b)+c or acc=((a+d)*b)+acc'
//

module add_then_mac
  #(parameter DEVICE = "VIRTEX6")
    (
     // Output ports
     output [47:0] acc,

     // Input ports
     input carryin,
     input ce,
     input clk,
     input [17:0] b,
     input load,
     input [47:0] c,
     input [17:0] a,
     input [17:0] d,
     input rst
     );


   wire [24:0] a_in;
   wire [24:0] d_in;

   localparam  AREG_IN = 1;
   localparam  BREG_IN = 1;
   localparam  MREG_IN = 1;
   localparam  PREG_IN = 1;
   localparam  A1REG_IN = 1;
   localparam  A0REG_IN = 0;
   localparam  B1REG_IN = 1;
   localparam  B0REG_IN = 1;

   // Sign extend inputs
   assign      a_in = (a[17] == 1'b1) ? {7'hff, a} : {7'h00, a};
   assign      d_in = (d[17] == 1'b1) ? {7'hff, d} : {7'h00, d};

   generate
      case(DEVICE)
	// begin generate virtex6
	"VIRTEX6", "7SERIES" :
	  begin
	     DSP48E1 #(
		       .ACASCREG(AREG_IN),
		       .AREG(AREG_IN),
		       .BCASCREG(BREG_IN),
		       .BREG(BREG_IN),
		       .MREG(MREG_IN),
		       .PREG(PREG_IN),
		       .USE_DPORT("TRUE")
		       )
	       DSP48E_BL (
			  .ACOUT(),
			  .BCOUT(),
			  .CARRYCASCOUT(),
			  .CARRYOUT(),
			  .MULTSIGNOUT(),
			  .OVERFLOW(),
			  .P(acc),
			  .PATTERNBDETECT(),
			  .PATTERNDETECT(),
			  .PCOUT(),
			  .UNDERFLOW(),
			  .A({5'b0, a_in[24:0]}),
			  .ACIN(30'b0),
			  .ALUMODE(4'b0000),
			  .B(b[17:0]),
			  .BCIN(18'b0),
			  .C(c),
			  .CARRYCASCIN(1'b0),
			  .CARRYIN(carryin),
			  .CARRYINSEL(3'b0),
			  .CEA1(1'b0),
			  .CEA2(ce),
			  .CEAD(ce),
			  .CEALUMODE(ce),
			  .CEB1(1'b0),
			  .CEB2(ce),
			  .CEC(ce),
			  .CECARRYIN(ce),
			  .CECTRL(ce),
			  .CED(ce),
			  .CEINMODE(ce),
			  .CEM(ce),
			  .CEP(ce),
			  .CLK(clk),
			  .D(d_in[24:0]),
			  .INMODE(5'b00100),
			  .MULTSIGNIN(1'b0),
			  .OPMODE({2'b01,load,4'b0101}),
			  .PCIN(48'b0),
			  .RSTA(rst),
			  .RSTALLCARRYIN(rst),
			  .RSTALUMODE(rst),
			  .RSTB(rst),
			  .RSTC(rst),
			  .RSTCTRL(rst),
			  .RSTD(rst),
			  .RSTINMODE(rst),
			  .RSTM(rst),
			  .RSTP(rst)
			  );
	  end // end generate virtex6
	// begin generate spartan6
	"SPARTAN6" :
	  begin
	     // DSP48A1 has 18b+18b=18b pre-adder, must discard LSB of A and D and compensate by shifting ACC.
	     wire discard;;
	     assign acc[0] = 1'b0;
	     
	     DSP48A1 #(
		       .A0REG(A0REG_IN),
		       .A1REG(A1REG_IN),
		       .B0REG(B0REG_IN),
		       .B1REG(B1REG_IN),
		       .MREG(MREG_IN),
		       .PREG(PREG_IN)
		       )
	       DSP48AST (
			 .BCOUT(),
			 .CARRYOUT(),
			 .CARRYOUTF(),
			 .M(),
			 .P({discard,acc[47:1]}),
			 .PCOUT(),
			 .A(b[17:0]),
			 .B({a_in[17],a_in[17:1]}),
			 .C(c),
			 .CARRYIN(carryin),
			 .CEA(ce),
			 .CEB(ce),
			 .CEC(ce),
			 .CECARRYIN(ce),
			 .CED(ce),
			 .CEM(ce),
			 .CEOPMODE(ce),
			 .CEP(ce),
			 .CLK(clk),
			 .D({d_in[17],d_in[17:1]}),
			 .OPMODE({5'b00011,load, 2'b01}),
			 .PCIN(48'b0),
			 .RSTA(rst),
			 .RSTB(rst),
			 .RSTC(rst),
			 .RSTCARRYIN(rst),
			 .RSTD(rst),
			 .RSTM(rst),
			 .RSTOPMODE(rst),
			 .RSTP(rst)
			 );
	  end // end generate spartan6
      endcase
   endgenerate
endmodule


// Copyright 2014 Ettus Research
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
// Write xilinx DSP48E1 primitive for mult-add with AXI interfaces

module mult_add
  #(parameter WIDTH_A=25,
    parameter WIDTH_B=18,
    parameter WIDTH_P=48,       // Must be 48 if you are cascading
    parameter DROP_TOP_P=0,     // Must be 0 if you are cascading
    parameter LATENCY=3,
    parameter CASCADE_IN=0,
    parameter CASCADE_OUT=0)
   (input clk, input reset,
    input [WIDTH_A-1:0] a_tdata, input a_tlast, input a_tvalid, output a_tready,
    input [WIDTH_B-1:0] b_tdata, input b_tlast, input b_tvalid, output b_tready,
    input [WIDTH_P-1:0] c_tdata, input c_tlast, input c_tvalid, output c_tready,
    output [WIDTH_P-1:0] p_tdata, output p_tlast, output p_tvalid, input p_tready);
   
   wire [24:0] 		 A_IN = { a_tdata, {(25-(WIDTH_A)){1'b0}}};
   wire [17:0] 		 B_IN = { b_tdata, {(18-(WIDTH_B)){1'b0}}};
   wire [47:0] 		 P1_OUT, P1_OUT_CASC;
   wire [47:0] 		 p_tdata_int = CASCADE_OUT ? P1_OUT_CASC : P1_OUT;
   assign p_tdata = p_tdata_int[47-DROP_TOP_P:48-WIDTH_P-DROP_TOP_P];

   wire [47:0] 		 c_tdata_int = { {DROP_TOP_P{c_tdata[WIDTH_P-1]}}, c_tdata, {(48-WIDTH_P-DROP_TOP_P){1'b0}} };
   
   wire [47:0] 		 CIN  = CASCADE_IN ? 48'h0000_0000_0000 : c_tdata_int;
   wire [47:0] 		 PCIN = CASCADE_IN ? c_tdata_int : 48'h0000_0000_0000;

   localparam MREG_IN = 1;    // Always have this reg
   localparam PREG_IN = (LATENCY >= 3) ? 1 : 0;
   localparam A2REG_IN = (LATENCY >= 2) ? 1 : 0;
   localparam A1REG_IN = (LATENCY == 4) ? 1 : 0;
   localparam AREG_IN = A1REG_IN + A2REG_IN;
   // See OPMODE Control Bits Settings, Table 2-7,2-8,2-9
   localparam ZMUX_PCIN = 3'b001;
   localparam ZMUX_C = 3'b011;
   localparam XMUX_M = 2'b01;
   localparam YMUX_M = 2'b01;
   
   wire [A1REG_IN:0] 	 enables_a, enables_b;
   wire 		 enable_c, enable_m;
   wire [PREG_IN:0] 	 en_post;
   wire 		 CE = 1'b1;   // FIXME
   wire 		 LOAD = 1'b1;
   wire 		 CEC, CEM, CEP;
   reg 			 CEA2, CEA1, CEB2, CEB1;
   
   always @*
     case(LATENCY)
       3 : {CEA2, CEA1, CEB2, CEB1} <= { enables_a[0], 1'b0  , enables_b[0], 1'b0   };
       4 : {CEA2, CEA1, CEB2, CEB1} <= { enables_a[1], enables_a[0], enables_b[1], enables_b[0] };
     endcase
   
   axi_pipe_mac #(.LATENCY(LATENCY), .CASCADE_IN(CASCADE_IN)) axi_pipe_mac
     (.clk(clk), .reset(reset), .clear(1'b0),
      .a_tlast(a_tlast), .a_tvalid(a_tvalid), .a_tready(a_tready),
      .b_tlast(b_tlast), .b_tvalid(b_tvalid), .b_tready(b_tready),
      .c_tlast(c_tlast), .c_tvalid(c_tvalid), .c_tready(c_tready),
      .p_tlast(p_tlast), .p_tvalid(p_tvalid), .p_tready(p_tready),
      .enables_a(enables_a), .enables_b(enables_b), .enable_c(CEC), .enable_m(CEM), .enable_p(CEP));
   
   DSP48E1 #(.ACASCREG(AREG_IN),       
             .AREG(AREG_IN),           
             .ADREG(0),
             .DREG(0),
             .BCASCREG(AREG_IN),       
             .BREG(AREG_IN),           
             .MREG(MREG_IN),           
             .PREG(PREG_IN)) 
   DSP48_inst (.ACOUT(),          // Outputs start here
               .BCOUT(),  
               .CARRYCASCOUT(), 
               .CARRYOUT(), 
               .MULTSIGNOUT(), 
               .OVERFLOW(), 
               .P(P1_OUT),          
               .PATTERNBDETECT(), 
               .PATTERNDETECT(), 
               .PCOUT(P1_OUT_CASC),  
               .UNDERFLOW(), 
               .A({5'b0,A_IN}),   // Inputs start here
               .ACIN(30'b0),    
               .ALUMODE(4'b0000),   //////////////////////
               .B(B_IN),       
               .BCIN(18'b0),    
               .C(CIN),          ///////////////////////
               .CARRYCASCIN(1'b0), 
               .CARRYIN(1'b0), 
               .CARRYINSEL(3'b0), 
               .CEA1(CEA1),      
               .CEA2(CEA2),      
               .CEAD(1'b0),
               .CEALUMODE(1'b1),   ////////////////////////
               .CEB1(CEB1),      
               .CEB2(CEB2),      
               .CEC(CEC),       ///////////////////////////
               .CECARRYIN(CE), 
               .CECTRL(CE), 
               .CED(CE),
               .CEINMODE(CE),
               .CEM(CEM),       
               .CEP(CEP),       
               .CLK(clk),       
               .D(25'b0),
               .INMODE(5'b0),    ///////////////////////
               .MULTSIGNIN(1'b0), 
               .OPMODE({(CASCADE_IN ? ZMUX_PCIN : ZMUX_C), YMUX_M, XMUX_M}), // ////////////////////
               .PCIN(PCIN),      //////////////////////
               .RSTA(reset),     
               .RSTALLCARRYIN(reset), 
               .RSTALUMODE(reset), 
               .RSTB(reset),     
               .RSTC(reset),   
               .RSTD(reset),  
               .RSTCTRL(reset),
               .RSTINMODE(reset), 
               .RSTM(reset), 
               .RSTP(reset));
   
endmodule // mult

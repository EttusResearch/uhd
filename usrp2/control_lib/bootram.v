
// Boot RAM for S3A, 8KB, dual port

// RAMB16BWE_S36_S36: 512 x 32 + 4 Parity bits byte-wide write Dual-Port RAM
//      Spartan-3A Xilinx HDL Libraries Guide, version 10.1.1

module bootram
  (input clk,
   input [12:0] if_adr,
   output [31:0] if_data,

   input [12:0] dwb_adr_i,
   input [31:0] dwb_dat_i,
   output [31:0] dwb_dat_o,
   input dwb_we_i,
   output reg dwb_ack_o,
   input dwb_stb_i,
   input [3:0] dwb_sel_i);

   wire [31:0] DOA0, DOA1, DOA2, DOA3;
   wire [31:0] DOB0, DOB1, DOB2, DOB3;
   wire        ENB0, ENB1, ENB2, ENB3;
   wire [3:0]  WEB;
   
   assign if_data = if_adr[12] ? (if_adr[11] ? DOA3 : DOA2) : (if_adr[11] ? DOA1 : DOA0);
   assign dwb_dat_o = dwb_adr_i[12] ? (dwb_adr_i[11] ? DOA3 : DOA2) : (dwb_adr_i[11] ? DOA1 : DOA0);

   always @(posedge clk)
     if(dwb_stb_i & ~dwb_ack_o)
       dwb_ack_o <= 1;
     else
       dwb_ack_o <= 0;

   assign ENB0 = dwb_stb_i & (dwb_adr_i[12:11] == 2'b00);
   assign ENB1 = dwb_stb_i & (dwb_adr_i[12:11] == 2'b01);
   assign ENB2 = dwb_stb_i & (dwb_adr_i[12:11] == 2'b10);
   assign ENB3 = dwb_stb_i & (dwb_adr_i[12:11] == 2'b11);

   assign WEB = {4{dwb_we_i}} & dwb_sel_i;
   
   RAMB16BWE_S36_S36 
     #(.INIT_A(36'h000000000), // Value of output RAM registers on Port A at startup
       .INIT_B(36'h000000000), // Value of output RAM registers on Port B at startup
       .SIM_COLLISION_CHECK("ALL"), // "NONE", "WARNING_ONLY", "GENERATE_X_ONLY", "ALL"
       .SRVAL_A(36'h000000000), // Port       A output value upon SSR    assertion
       .SRVAL_B(36'h000000000), // Port       B output value upon SSR    assertion
       .WRITE_MODE_A("WRITE_FIRST"), //       WRITE_FIRST, READ_FIRST    or NO_CHANGE
       .WRITE_MODE_B("WRITE_FIRST")) //       WRITE_FIRST, READ_FIRST    or NO_CHANGE
   RAM0
     (.DOA(DOA0),           // Port A 32-bit Data Output
      .DOPA(),              // Port A 4-bit Parity Output
      .ADDRA(if_adr[10:2]), // Port A 9-bit Address Input
      .CLKA(clk),           // Port A 1-bit Clock
      .DIA(32'd0),          // Port A 32-bit Data Input
      .DIPA(4'd0),          // Port A 4-bit parity Input
      .ENA(1'b1),           // Port A 1-bit RAM Enable Input
      .SSRA(1'b0),          // Port A 1-bit Synchronous Set/Reset Input
      .WEA(1'b0),           // Port A 4-bit Write Enable Input

      .DOB(DOB0),              // Port B 32-bit Data Output
      .DOPB(),                 // Port B 4-bit Parity Output
      .ADDRB(dwb_adr_i[10:2]), // Port B 9-bit Address Input
      .CLKB(clk),              // Port B 1-bit Clock
      .DIB(dwb_dat_i),         // Port B 32-bit Data Input
      .DIPB(4'd0),             // Port-B 4-bit parity Input
      .ENB(ENB0),              // Port B 1-bit RAM Enable Input
      .SSRB(1'b0),             // Port B 1-bit Synchronous Set/Reset Input
      .WEB(WEB)                // Port B 4-bit Write Enable Input
      );   // End of RAMB16BWE_S36_S36_inst instantiation

   RAMB16BWE_S36_S36 
     #(.INIT_A(36'h000000000), // Value of output RAM registers on Port A at startup
       .INIT_B(36'h000000000), // Value of output RAM registers on Port B at startup
       .SIM_COLLISION_CHECK("ALL"), // "NONE", "WARNING_ONLY", "GENERATE_X_ONLY", "ALL"
       .SRVAL_A(36'h000000000), // Port       A output value upon SSR    assertion
       .SRVAL_B(36'h000000000), // Port       B output value upon SSR    assertion
       .WRITE_MODE_A("WRITE_FIRST"), //       WRITE_FIRST, READ_FIRST    or NO_CHANGE
       .WRITE_MODE_B("WRITE_FIRST")) //       WRITE_FIRST, READ_FIRST    or NO_CHANGE
   RAM1
     (.DOA(DOA1),           // Port A 32-bit Data Output
      .DOPA(),              // Port A 4-bit Parity Output
      .ADDRA(if_adr[10:2]), // Port A 9-bit Address Input
      .CLKA(clk),           // Port A 1-bit Clock
      .DIA(32'd0),          // Port A 32-bit Data Input
      .DIPA(4'd0),          // Port A 4-bit parity Input
      .ENA(1'b1),           // Port A 1-bit RAM Enable Input
      .SSRA(1'b0),          // Port A 1-bit Synchronous Set/Reset Input
      .WEA(1'b0),           // Port A 4-bit Write Enable Input

      .DOB(DOB1),              // Port B 32-bit Data Output
      .DOPB(),                 // Port B 4-bit Parity Output
      .ADDRB(dwb_adr_i[10:2]), // Port B 9-bit Address Input
      .CLKB(clk),              // Port B 1-bit Clock
      .DIB(dwb_dat_i),         // Port B 32-bit Data Input
      .DIPB(4'd0),             // Port-B 4-bit parity Input
      .ENB(ENB1),              // Port B 1-bit RAM Enable Input
      .SSRB(1'b0),             // Port B 1-bit Synchronous Set/Reset Input
      .WEB(WEB)                // Port B 4-bit Write Enable Input
      );   // End of RAMB16BWE_S36_S36_inst instantiation

   RAMB16BWE_S36_S36 
     #(.INIT_A(36'h000000000), // Value of output RAM registers on Port A at startup
       .INIT_B(36'h000000000), // Value of output RAM registers on Port B at startup
       .SIM_COLLISION_CHECK("ALL"), // "NONE", "WARNING_ONLY", "GENERATE_X_ONLY", "ALL"
       .SRVAL_A(36'h000000000), // Port       A output value upon SSR    assertion
       .SRVAL_B(36'h000000000), // Port       B output value upon SSR    assertion
       .WRITE_MODE_A("WRITE_FIRST"), //       WRITE_FIRST, READ_FIRST    or NO_CHANGE
       .WRITE_MODE_B("WRITE_FIRST")) //       WRITE_FIRST, READ_FIRST    or NO_CHANGE
   RAM2
     (.DOA(DOA2),           // Port A 32-bit Data Output
      .DOPA(),              // Port A 4-bit Parity Output
      .ADDRA(if_adr[10:2]), // Port A 9-bit Address Input
      .CLKA(clk),           // Port A 1-bit Clock
      .DIA(32'd0),          // Port A 32-bit Data Input
      .DIPA(4'd0),          // Port A 4-bit parity Input
      .ENA(1'b1),           // Port A 1-bit RAM Enable Input
      .SSRA(1'b0),          // Port A 1-bit Synchronous Set/Reset Input
      .WEA(1'b0),           // Port A 4-bit Write Enable Input

      .DOB(DOB2),              // Port B 32-bit Data Output
      .DOPB(),                 // Port B 4-bit Parity Output
      .ADDRB(dwb_adr_i[10:2]), // Port B 9-bit Address Input
      .CLKB(clk),              // Port B 1-bit Clock
      .DIB(dwb_dat_i),         // Port B 32-bit Data Input
      .DIPB(4'd0),             // Port-B 4-bit parity Input
      .ENB(ENB2),              // Port B 1-bit RAM Enable Input
      .SSRB(1'b0),             // Port B 1-bit Synchronous Set/Reset Input
      .WEB(WEB)                // Port B 4-bit Write Enable Input
      );   // End of RAMB16BWE_S36_S36_inst instantiation

   RAMB16BWE_S36_S36 
     #(.INIT_A(36'h000000000), // Value of output RAM registers on Port A at startup
       .INIT_B(36'h000000000), // Value of output RAM registers on Port B at startup
       .SIM_COLLISION_CHECK("ALL"), // "NONE", "WARNING_ONLY", "GENERATE_X_ONLY", "ALL"
       .SRVAL_A(36'h000000000), // Port       A output value upon SSR    assertion
       .SRVAL_B(36'h000000000), // Port       B output value upon SSR    assertion
       .WRITE_MODE_A("WRITE_FIRST"), //       WRITE_FIRST, READ_FIRST    or NO_CHANGE
       .WRITE_MODE_B("WRITE_FIRST")) //       WRITE_FIRST, READ_FIRST    or NO_CHANGE
   RAM3
     (.DOA(DOA3),           // Port A 32-bit Data Output
      .DOPA(),              // Port A 4-bit Parity Output
      .ADDRA(if_adr[10:2]), // Port A 9-bit Address Input
      .CLKA(clk),           // Port A 1-bit Clock
      .DIA(32'd0),          // Port A 32-bit Data Input
      .DIPA(4'd0),          // Port A 4-bit parity Input
      .ENA(1'b1),           // Port A 1-bit RAM Enable Input
      .SSRA(1'b0),          // Port A 1-bit Synchronous Set/Reset Input
      .WEA(1'b0),           // Port A 4-bit Write Enable Input

      .DOB(DOB3),              // Port B 32-bit Data Output
      .DOPB(),                 // Port B 4-bit Parity Output
      .ADDRB(dwb_adr_i[10:2]), // Port B 9-bit Address Input
      .CLKB(clk),              // Port B 1-bit Clock
      .DIB(dwb_dat_i),         // Port B 32-bit Data Input
      .DIPB(4'd0),             // Port-B 4-bit parity Input
      .ENB(ENB3),              // Port B 1-bit RAM Enable Input
      .SSRB(1'b0),             // Port B 1-bit Synchronous Set/Reset Input
      .WEB(WEB)                // Port B 4-bit Write Enable Input
      );   // End of RAMB16BWE_S36_S36_inst instantiation

endmodule // bootram

/*
       // The following INIT_xx declarations specify the initial contents of the RAM
       // Address 0 to 127
       .INIT_00(256’h00000000_00000000_00000000_00000000_00000000_00000000_00000000_00000000),
       .INIT_01(256’h00000000_00000000_00000000_00000000_00000000_00000000_00000000_00000000),
       .INIT_02(256’h00000000_00000000_00000000_00000000_00000000_00000000_00000000_00000000),
       .INIT_03(256’h00000000_00000000_00000000_00000000_00000000_00000000_00000000_00000000),
       .INIT_04(256’h00000000_00000000_00000000_00000000_00000000_00000000_00000000_00000000),
       .INIT_05(256’h00000000_00000000_00000000_00000000_00000000_00000000_00000000_00000000),
       .INIT_06(256’h00000000_00000000_00000000_00000000_00000000_00000000_00000000_00000000),
       .INIT_07(256’h00000000_00000000_00000000_00000000_00000000_00000000_00000000_00000000),
       .INIT_08(256’h00000000_00000000_00000000_00000000_00000000_00000000_00000000_00000000),
       .INIT_09(256’h00000000_00000000_00000000_00000000_00000000_00000000_00000000_00000000),
       .INIT_0A(256’h00000000_00000000_00000000_00000000_00000000_00000000_00000000_00000000),
       .INIT_0B(256’h00000000_00000000_00000000_00000000_00000000_00000000_00000000_00000000),
       .INIT_0C(256’h00000000_00000000_00000000_00000000_00000000_00000000_00000000_00000000),
       .INIT_0D(256’h00000000_00000000_00000000_00000000_00000000_00000000_00000000_00000000),
       .INIT_0E(256’h00000000_00000000_00000000_00000000_00000000_00000000_00000000_00000000),
       .INIT_0F(256’h00000000_00000000_00000000_00000000_00000000_00000000_00000000_00000000),
       // Address 128 to 255
       .INIT_10(256’h00000000_00000000_00000000_00000000_00000000_00000000_00000000_00000000),
       .INIT_11(256’h00000000_00000000_00000000_00000000_00000000_00000000_00000000_00000000),
       .INIT_12(256’h00000000_00000000_00000000_00000000_00000000_00000000_00000000_00000000),
       .INIT_13(256’h00000000_00000000_00000000_00000000_00000000_00000000_00000000_00000000),
       .INIT_14(256’h00000000_00000000_00000000_00000000_00000000_00000000_00000000_00000000),
       .INIT_15(256’h00000000_00000000_00000000_00000000_00000000_00000000_00000000_00000000),
       .INIT_16(256’h00000000_00000000_00000000_00000000_00000000_00000000_00000000_00000000),
       .INIT_17(256’h00000000_00000000_00000000_00000000_00000000_00000000_00000000_00000000),
       .INIT_18(256’h00000000_00000000_00000000_00000000_00000000_00000000_00000000_00000000),
       .INIT_19(256’h00000000_00000000_00000000_00000000_00000000_00000000_00000000_00000000),
       .INIT_1A(256’h00000000_00000000_00000000_00000000_00000000_00000000_00000000_00000000),
       .INIT_1B(256’h00000000_00000000_00000000_00000000_00000000_00000000_00000000_00000000),
       .INIT_1C(256’h00000000_00000000_00000000_00000000_00000000_00000000_00000000_00000000),
       .INIT_1D(256’h00000000_00000000_00000000_00000000_00000000_00000000_00000000_00000000),
       .INIT_1E(256’h00000000_00000000_00000000_00000000_00000000_00000000_00000000_00000000),
       .INIT_1F(256’h00000000_00000000_00000000_00000000_00000000_00000000_00000000_00000000),
       // Address 256 to 383
       .INIT_20(256’h00000000_00000000_00000000_00000000_00000000_00000000_00000000_00000000),
       .INIT_21(256’h00000000_00000000_00000000_00000000_00000000_00000000_00000000_00000000),
       .INIT_22(256’h00000000_00000000_00000000_00000000_00000000_00000000_00000000_00000000),
       .INIT_23(256’h00000000_00000000_00000000_00000000_00000000_00000000_00000000_00000000),
       .INIT_24(256’h00000000_00000000_00000000_00000000_00000000_00000000_00000000_00000000),
       .INIT_25(256’h00000000_00000000_00000000_00000000_00000000_00000000_00000000_00000000),
       .INIT_26(256’h00000000_00000000_00000000_00000000_00000000_00000000_00000000_00000000),
       .INIT_27(256’h00000000_00000000_00000000_00000000_00000000_00000000_00000000_00000000),
       .INIT_28(256’h00000000_00000000_00000000_00000000_00000000_00000000_00000000_00000000),
       .INIT_29(256’h00000000_00000000_00000000_00000000_00000000_00000000_00000000_00000000),
       .INIT_2A(256’h00000000_00000000_00000000_00000000_00000000_00000000_00000000_00000000),
       .INIT_2B(256’h00000000_00000000_00000000_00000000_00000000_00000000_00000000_00000000),
       .INIT_2C(256’h00000000_00000000_00000000_00000000_00000000_00000000_00000000_00000000),
       .INIT_2D(256’h00000000_00000000_00000000_00000000_00000000_00000000_00000000_00000000),
       .INIT_2E(256’h00000000_00000000_00000000_00000000_00000000_00000000_00000000_00000000),
       .INIT_2F(256’h00000000_00000000_00000000_00000000_00000000_00000000_00000000_00000000),
       // Address 384 to 511
       .INIT_30(256’h00000000_00000000_00000000_00000000_00000000_00000000_00000000_00000000),
       .INIT_31(256’h00000000_00000000_00000000_00000000_00000000_00000000_00000000_00000000),
       .INIT_32(256’h00000000_00000000_00000000_00000000_00000000_00000000_00000000_00000000),
       .INIT_33(256’h00000000_00000000_00000000_00000000_00000000_00000000_00000000_00000000),
       .INIT_34(256’h00000000_00000000_00000000_00000000_00000000_00000000_00000000_00000000),
       .INIT_35(256’h00000000_00000000_00000000_00000000_00000000_00000000_00000000_00000000),
       .INIT_36(256’h00000000_00000000_00000000_00000000_00000000_00000000_00000000_00000000),
       .INIT_37(256’h00000000_00000000_00000000_00000000_00000000_00000000_00000000_00000000),
       .INIT_38(256’h00000000_00000000_00000000_00000000_00000000_00000000_00000000_00000000),
       .INIT_39(256’h00000000_00000000_00000000_00000000_00000000_00000000_00000000_00000000),
       .INIT_3A(256’h00000000_00000000_00000000_00000000_00000000_00000000_00000000_00000000),
       .INIT_3B(256’h00000000_00000000_00000000_00000000_00000000_00000000_00000000_00000000),
       .INIT_3C(256’h00000000_00000000_00000000_00000000_00000000_00000000_00000000_00000000),
       .INIT_3D(256’h00000000_00000000_00000000_00000000_00000000_00000000_00000000_00000000),
       .INIT_3E(256’h00000000_00000000_00000000_00000000_00000000_00000000_00000000_00000000),
       .INIT_3F(256’h00000000_00000000_00000000_00000000_00000000_00000000_00000000_00000000),
       // The next set of INITP_xx are for the parity bits
       // Address 0 to 127
       .INITP_00(256’h0000000000000000000000000000000000000000000000000000000000000000),
       .INITP_01(256’h0000000000000000000000000000000000000000000000000000000000000000),
       // Address 128 to 255
       .INITP_02(256’h0000000000000000000000000000000000000000000000000000000000000000),
       .INITP_03(256’h0000000000000000000000000000000000000000000000000000000000000000),
       // Address 256 to 383
       .INITP_04(256’h0000000000000000000000000000000000000000000000000000000000000000),
       .INITP_05(256’h0000000000000000000000000000000000000000000000000000000000000000),
       // Address 384 to 511
       .INITP_06(256’h0000000000000000000000000000000000000000000000000000000000000000),
       .INITP_07(256’h0000000000000000000000000000000000000000000000000000000000000000)
*/

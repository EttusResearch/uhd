//////////////////////////////////////////////////////////////////////
////                                                              ////
////  File name "wishbone.v"                                      ////
////                                                              ////
////  This file is part of the "10GE MAC" project                 ////
////  http://www.opencores.org/cores/xge_mac/                     ////
////                                                              ////
////  Author(s):                                                  ////
////      - A. Tanguay (antanguay@opencores.org)                  ////
////                                                              ////
//////////////////////////////////////////////////////////////////////
////                                                              ////
//// Copyright (C) 2008 AUTHORS. All rights reserved.             ////
////                                                              ////
//// This source file may be used and distributed without         ////
//// restriction provided that this copyright statement is not    ////
//// removed from the file and that any derivative work contains  ////
//// the original copyright notice and the associated disclaimer. ////
////                                                              ////
//// This source file is free software; you can redistribute it   ////
//// and/or modify it under the terms of the GNU Lesser General   ////
//// Public License as published by the Free Software Foundation; ////
//// either version 2.1 of the License, or (at your option) any   ////
//// later version.                                               ////
////                                                              ////
//// This source is distributed in the hope that it will be       ////
//// useful, but WITHOUT ANY WARRANTY; without even the implied   ////
//// warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR      ////
//// PURPOSE.  See the GNU Lesser General Public License for more ////
//// details.                                                     ////
////                                                              ////
//// You should have received a copy of the GNU Lesser General    ////
//// Public License along with this source; if not, download it   ////
//// from http://www.opencores.org/lgpl.shtml                     ////
////                                                              ////
//////////////////////////////////////////////////////////////////////

`include "defines.v"
 
module wishbone_if(/*AUTOARG*/
  // Outputs
  wb_dat_o, wb_ack_o, wb_int_o, ctrl_tx_enable, 
  `ifdef MDIO
   mdc, mdio_out, mdio_tri, xge_gpo,
   `endif
		   
  // Inputs
  wb_clk_i, wb_rst_i, wb_adr_i, wb_dat_i, wb_we_i, wb_stb_i, wb_cyc_i,
  status_crc_error, status_fragment_error, status_txdfifo_ovflow,
  status_txdfifo_udflow, status_rxdfifo_ovflow, status_rxdfifo_udflow,
  status_pause_frame_rx, status_local_fault, status_remote_fault
  `ifdef MDIO
  ,mdio_in, xge_gpi
  `endif		   
  );
 
 
input         wb_clk_i;
input         wb_rst_i;
 
input  [7:0]  wb_adr_i;
input  [31:0] wb_dat_i;
input         wb_we_i;
input         wb_stb_i;
input         wb_cyc_i;
 
output [31:0] wb_dat_o;
output        wb_ack_o;
output        wb_int_o;
 
input         status_crc_error;
input         status_fragment_error;
 
input         status_txdfifo_ovflow;
 
input         status_txdfifo_udflow;
 
input         status_rxdfifo_ovflow;
 
input         status_rxdfifo_udflow;
 
input         status_pause_frame_rx;
 
input         status_local_fault;
input         status_remote_fault;
 
output        ctrl_tx_enable;
 
  `ifdef MDIO
   output reg    mdc;
   output reg    mdio_out;
   output reg    mdio_tri;   // Assert to tristate driver.
   input      mdio_in;
   input [7:0] xge_gpi;
   output reg [7:0] xge_gpo;

   //
   // State Declarations
   //
   parameter  IDLE = 0,
		PREAMBLE1 = 1,
		PREAMBLE2 = 2,
		PREAMBLE3 = 3,
		PREAMBLE4 = 4,
		PREAMBLE5 = 5,
		PREAMBLE6 = 6,
		PREAMBLE7 = 7,
		PREAMBLE8 = 8,
		PREAMBLE9 = 9,
		PREAMBLE10 = 10,
		PREAMBLE11 = 11,
		PREAMBLE12 = 12,
		PREAMBLE13 = 13,
		PREAMBLE14 = 14,
		PREAMBLE15 = 15,
		PREAMBLE16 = 16,
		PREAMBLE17 = 17,
		PREAMBLE18 = 18,
		PREAMBLE19 = 19,
		PREAMBLE20 = 20,
		PREAMBLE21 = 21,
		PREAMBLE22 = 22,
		PREAMBLE23 = 23,
		PREAMBLE24 = 24,
		PREAMBLE25 = 25,
		PREAMBLE26 = 26,
		PREAMBLE27 = 27,
		PREAMBLE28 = 28,
		PREAMBLE29 = 29,
		PREAMBLE30 = 30,
		PREAMBLE31 = 31,
		PREAMBLE32 = 32,
		START1 = 33,
		C22_START2 = 34,
		C45_START2 = 35,
		OP1 = 36,
		OP2 = 37,
		PRTAD1 = 38,
		PRTAD2 = 39,
		PRTAD3 = 40,
		PRTAD4 = 41,
		PRTAD5 = 42,
		DEVAD1 = 43,
		DEVAD2 = 44,
		DEVAD3 = 45,
		DEVAD4 = 46,
		DEVAD5 = 47,
		TA1 = 48,
		TA2 = 49,
		TA3 = 50,
		READ1 = 51,
		READ2 = 52,
		READ3 = 53,
		READ4 = 54,
		READ5 = 55,
		READ6 = 56,
		READ7 = 57,
		READ8 = 58,
		READ9 = 59,
		READ10 = 60,
		READ11 = 61,
		READ12 = 62,
		READ13 = 63,
		READ14 = 64,
		READ15 = 65,
		READ16 = 66,
		WRITE1 = 67,
		WRITE2 = 68,
		WRITE3 = 69,
		WRITE4 = 70,
		WRITE5 = 71,
		WRITE6 = 72,
		WRITE7 = 73,
		WRITE8 = 74,
		WRITE9 = 75,
		WRITE10 = 76,
		WRITE11 = 77,
		WRITE12 = 78,
		WRITE13 = 79,
		WRITE14 = 80,
		WRITE15 = 81,
		WRITE16 = 82,
		C45_ADDR1 = 83,
		C45_ADDR2 = 84,
		C45_ADDR3 = 85,
		C45_ADDR4 = 86,
		C45_ADDR5 = 87,
		C45_ADDR6 = 88,
		C45_ADDR7 = 89,
		C45_ADDR8 = 90,
		C45_ADDR9 = 91,
		C45_ADDR10 = 92,
		C45_ADDR11 = 93,
		C45_ADDR12 = 94,
		C45_ADDR13 = 95,
		C45_ADDR14 = 96,
		C45_ADDR15 = 97,
		C45_ADDR16 = 98,
		PREIDLE = 99;
   
       
`endif

   
/*AUTOREG*/
// Beginning of automatic regs (for this module's undeclared outputs)
reg [31:0]              wb_dat_o;
reg                     wb_int_o;
// End of automatics
 
reg  [0:0]              cpureg_config0;
reg  [8:0]              cpureg_int_pending;
reg  [8:0]              cpureg_int_mask;
 
reg                     cpuack;
 
reg                     status_remote_fault_d1;
reg                     status_local_fault_d1;

`ifdef MDIO
   reg [15:0]			mdio_read_data;
   reg [15:0] 			mdio_write_data;
   reg [15:0] 			mdio_address;
   reg [12:0] 			mdio_operation;
   reg 				mdio_control;
   reg [7:0] 			mdc_clk_count;
   reg 				mdc_falling_edge;
   reg 				mdio_running;
   reg 				mdio_done;
   reg [7:0] 			state;
   reg [7:0] 			xge_gpi_reg;
   reg [7:0] 			xge_gpo_reg;
   
`endif
   
 
/*AUTOWIRE*/
 
wire [8:0] 			int_sources;
 
 
//---
// Source of interrupts, some are edge sensitive, others
// expect a pulse signal.
 
assign int_sources = {
                      status_fragment_error,
                      status_crc_error,
 
                      status_pause_frame_rx,
 
                      status_remote_fault ^ status_remote_fault_d1,
                      status_local_fault ^ status_local_fault_d1,
 
                      status_rxdfifo_udflow,
                      status_rxdfifo_ovflow,
                      status_txdfifo_udflow,
                      status_txdfifo_ovflow
                      };
 
//---
// Config Register 0
 
assign ctrl_tx_enable = cpureg_config0[0];
 
 
 
//---
// Wishbone signals
 
assign wb_ack_o = cpuack && wb_stb_i;
 
always @(posedge wb_clk_i or posedge wb_rst_i) begin
 
    if (wb_rst_i == 1'b1) begin
 
        cpureg_config0 <= 1'h1;
        cpureg_int_pending <= 9'b0;
        cpureg_int_mask <= 9'b0;
 
        wb_dat_o <= 32'b0;
        wb_int_o <= 1'b0;
 
        cpuack <= 1'b0;

`ifdef MDIO
       mdio_address <= 0;
       mdio_operation <= 0;
       mdio_write_data <= 0;
       
       mdio_running <= 0;
       xge_gpi_reg <= 0;
       xge_gpo <= 0;
       xge_gpo_reg <= 0;
       

`endif
	 
 
 //       status_remote_fault_d1 <= status_remote_fault;
 //       status_local_fault_d1 <= status_local_fault;
       // IJB. Original code was unsynthesizable and a little bizzare
       // implying reset would latch data.
       status_remote_fault_d1 <= 0;
       status_local_fault_d1 <= 0;
    end
    else begin
 
        wb_int_o <= |(cpureg_int_pending & cpureg_int_mask);
 
        cpureg_int_pending <= cpureg_int_pending | int_sources;
 
        cpuack <= wb_cyc_i && wb_stb_i;
 
        status_remote_fault_d1 <= status_remote_fault;
        status_local_fault_d1 <= status_local_fault;
       
`ifdef MDIO
       // Handshake to MDIO state machine to reset running flag in status.
       // Wait for falling MDC edge to prevent S/W race condition occuring
       // where done flag still asserted but running flag now cleared (repeatedly).
       if (mdio_done && mdc_falling_edge)
	 mdio_running <= 0;

       // Register GPIO to allow regs placed in the I/O cells and provide some metastability prot
       xge_gpi_reg <= xge_gpi;
       xge_gpo <= xge_gpo_reg;
       

`endif
        //---
        // Read access
 
        if (wb_cyc_i && wb_stb_i && !wb_we_i) begin
 
            case ({wb_adr_i[7:2], 2'b0})
 
              `CPUREG_CONFIG0: begin
                  wb_dat_o <= {31'b0, cpureg_config0};
              end
 
              `CPUREG_INT_PENDING: begin
                  wb_dat_o <= {23'b0, cpureg_int_pending};
                  cpureg_int_pending <= int_sources;
                  wb_int_o <= 1'b0;
              end
 
              `CPUREG_INT_STATUS: begin
                  wb_dat_o <= {23'b0, int_sources};
              end
 
              `CPUREG_INT_MASK: begin
                  wb_dat_o <= {23'b0, cpureg_int_mask};
              end

 `ifdef MDIO
	      `CPUREG_MDIO_DATA: begin
		 wb_dat_o <= {16'b0, mdio_read_data};
	      end
	      
	      `CPUREG_MDIO_STATUS: begin
		 wb_dat_o <= {31'b0, mdio_running};
	      end

	      `CPUREG_GPIO: begin
		 wb_dat_o <= {24'b0, xge_gpi_reg};
	      end
	      
 `endif  
	      
              default: begin
              end
 
            endcase
 
        end
 
        //---
        // Write access
 
        if (wb_cyc_i && wb_stb_i && wb_we_i) begin
	   $display("reg write @ addr %x",({wb_adr_i[7:2], 2'b0}));
	   
            case ({wb_adr_i[7:2], 2'b0})
 
              `CPUREG_CONFIG0: begin
                  cpureg_config0 <= wb_dat_i[0:0];
              end
 
              `CPUREG_INT_PENDING: begin
                  cpureg_int_pending <= wb_dat_i[8:0] | cpureg_int_pending | int_sources;
              end
 
              `CPUREG_INT_MASK: begin
                  cpureg_int_mask <= wb_dat_i[8:0];
              end

`ifdef MDIO
	      `CPUREG_MDIO_DATA: begin
		 mdio_write_data <= wb_dat_i[15:0];
	      end

	       `CPUREG_MDIO_ADDR: begin
		 mdio_address <= wb_dat_i[15:0];
	       end
	      
	      `CPUREG_MDIO_OP: begin
		 mdio_operation <= wb_dat_i[12:0];
	       end

	      `CPUREG_MDIO_CONTROL: begin
		 // Trigger mdio operation here. Cleared by state machine at end of bus transaction.
		 if (wb_dat_i[0])
		   mdio_running <= 1;		 
	      end

	      `CPUREG_GPIO: begin
		 xge_gpo_reg <= wb_dat_i[7:0];
	       end
	      
`endif //  `ifdef MDIO
	       
              default: begin
              end
 
            endcase
 
        end
 
    end
 
end // always @ (posedge wb_clk_i or posedge wb_rst_i)

`ifdef MDIO
   //
   // Produce mdc clock as a signal synchronously from Wishbone clock.
   //
   always @(posedge wb_clk_i or posedge wb_rst_i)
     if (wb_rst_i)
       begin
	  mdc_clk_count <= 1;
	  mdc <= 0;
	  mdc_falling_edge <= 0;	  
       end
     else if (mdc_clk_count == `MDC_HALF_PERIOD)
       begin
	  mdc_clk_count <= 1;
	  mdc <= ~mdc;
	  mdc_falling_edge <= mdc;	  
       end
     else
       begin
	  mdc_clk_count <= mdc_clk_count + 1;
	  mdc_falling_edge <= 0;		   
       end
  
   //
   // MDIO state machine
   //
   always @(posedge wb_clk_i or posedge wb_rst_i)
     if (wb_rst_i)
       begin
	  mdio_tri <= 1;
	  mdio_out <= 0;
	  mdio_done <= 0;
	  mdio_read_data <= 0;
	  state <= IDLE;
       end
     else if (mdc_falling_edge)
       //
       // This is the MDIO bus controller. Use falling edge of MDC.
       //      
       begin
	  // Defaults	  
	  mdio_tri <= 1;
	  mdio_out <= 0;
	  mdio_done <= 0;
	  
	  
	  case(state)
	    // IDLE.
	    // In Clause 22 & 45 the master of the MDIO bus is tristate during idle.
	    // 
	    IDLE: begin
	       mdio_tri <= 1;
	       mdio_out <= 0;
	       if (mdio_running)
		 state <= PREAMBLE1;
	    end
	    // Preamble. All MDIO transactions begin witrh 32bits of 1 bits as a preamble.
	    PREAMBLE1: begin
	       mdio_tri <= 0;
	       mdio_out <= 1;  
	       state <= PREAMBLE2;
	    end
	    PREAMBLE2: begin
	       mdio_tri <= 0;
	       mdio_out <= 1;  
	       state <= PREAMBLE3;
	    end 
	    PREAMBLE3: begin
	       mdio_tri <= 0;
	       mdio_out <= 1;  
	       state <= PREAMBLE4;
	    end 
	    PREAMBLE4: begin
	       mdio_tri <= 0;
	       mdio_out <= 1;  
	       state <= PREAMBLE5;
	    end 
	    PREAMBLE5: begin
	       mdio_tri <= 0;
	       mdio_out <= 1;  
	       state <= PREAMBLE6;
	    end
	    PREAMBLE6: begin
	       mdio_tri <= 0;
	       mdio_out <= 1;  
	       state <= PREAMBLE7;
	    end
	    PREAMBLE7: begin
	       mdio_tri <= 0;
	       mdio_out <= 1;  
	       state <= PREAMBLE8;
	    end
	    PREAMBLE8: begin
	       mdio_tri <= 0;
	       mdio_out <= 1;
	       state <= PREAMBLE9;
	    end
	    PREAMBLE9: begin
	       mdio_tri <= 0;
	       mdio_out <= 1;
	       state <= PREAMBLE10;
	    end
	    PREAMBLE10: begin
	       mdio_tri <= 0;
	       mdio_out <= 1;
	       state <= PREAMBLE11;
	    end
	    PREAMBLE11: begin
	       mdio_tri <= 0;
	       mdio_out <= 1;  
	       state <= PREAMBLE12;
	    end
	    PREAMBLE12: begin
	       mdio_tri <= 0;
	       mdio_out <= 1;  
	       state <= PREAMBLE13;
	    end 
	    PREAMBLE13: begin
	       mdio_tri <= 0;
	       mdio_out <= 1;  
	       state <= PREAMBLE14;
	    end 
	    PREAMBLE14: begin
	       mdio_tri <= 0;
	       mdio_out <= 1;  
	       state <= PREAMBLE15;
	    end 
	    PREAMBLE15: begin
	       mdio_tri <= 0;
	       mdio_out <= 1;  
	       state <= PREAMBLE16;
	    end
	    PREAMBLE16: begin
	       mdio_tri <= 0;
	       mdio_out <= 1;  
	       state <= PREAMBLE17;
	    end
	    PREAMBLE17: begin
	       mdio_tri <= 0;
	       mdio_out <= 1;  
	       state <= PREAMBLE18;
	    end
	    PREAMBLE18: begin
	       mdio_tri <= 0;
	       mdio_out <= 1;
	       state <= PREAMBLE19;
	    end
	    PREAMBLE19: begin
	       mdio_tri <= 0;
	       mdio_out <= 1;
	       state <= PREAMBLE20;
	    end
	    PREAMBLE20: begin
	       mdio_tri <= 0;
	       mdio_out <= 1;
	       state <= PREAMBLE21;
	    end
	    PREAMBLE21: begin
	       mdio_tri <= 0;
	       mdio_out <= 1;  
	       state <= PREAMBLE22;
	    end
	    PREAMBLE22: begin
	       mdio_tri <= 0;
	       mdio_out <= 1;  
	       state <= PREAMBLE23;
	    end 
	    PREAMBLE23: begin
	       mdio_tri <= 0;
	       mdio_out <= 1;  
	       state <= PREAMBLE24;
	    end 
	    PREAMBLE24: begin
	       mdio_tri <= 0;
	       mdio_out <= 1;  
	       state <= PREAMBLE25;
	    end 
	    PREAMBLE25: begin
	       mdio_tri <= 0;
	       mdio_out <= 1;  
	       state <= PREAMBLE26;
	    end
	    PREAMBLE26: begin
	       mdio_tri <= 0;
	       mdio_out <= 1;  
	       state <= PREAMBLE27;
	    end
	    PREAMBLE27: begin
	       mdio_tri <= 0;
	       mdio_out <= 1;  
	       state <= PREAMBLE28;
	    end
	    PREAMBLE28: begin
	       mdio_tri <= 0;
	       mdio_out <= 1;
	       state <= PREAMBLE29;
	    end
	    PREAMBLE29: begin
	       mdio_tri <= 0;
	       mdio_out <= 1;
	       state <= PREAMBLE30;
	    end
	    PREAMBLE30: begin
	       mdio_tri <= 0;
	       mdio_out <= 1;
	       state <= PREAMBLE31;
	    end
	    PREAMBLE31: begin
	       mdio_tri <= 0;
	       mdio_out <= 1;
	       state <= PREAMBLE32;
	    end
	    PREAMBLE32: begin
	       mdio_tri <= 0;
	       mdio_out <= 1;
	       state <= START1;
	    end
	    //
	    // Start code for Clause 22 is 01 and Clause 45 is 00
	    //
	    START1: begin
	       mdio_tri <= 0;
	       mdio_out <= 0;
	       if (mdio_operation[12])
		 // Clause 45 bit set.
		 state <= C45_START2;
	       else
		 state <= C22_START2;	       
	    end
	    //
	    // 2nd Clause 22 start bit is a 1
	    //
	    C22_START2: begin
	       mdio_tri <= 0;
	       mdio_out <= 1;
	       state <= OP1;
	    end
	    //
	    // 2nd Clause 45 start bit is a 0
	    //
	    C45_START2: begin
	       mdio_tri <= 0;
	       mdio_out <= 0;
	       state <= OP1;
	    end
	    //
	    // Both Clause 22 & 45 use 2 bits for operation and are compatable.
	    // Note we don't screen here for illegal Clause 22 ops.
	    //
	    OP1: begin
	       mdio_tri <= 0;
	       mdio_out <= mdio_operation[11];
	       state <= OP2;
	    end
	    OP2: begin
	       mdio_tri <= 0;
	       mdio_out <= mdio_operation[10];
	       state <= PRTAD1;
	    end
	    //
	    // Both Clause 22 & 45 use 2 sucsessive 5 bit fields to form a hierarchical address
	    // though it's used slightly different between the 2 standards.
	    //
	    PRTAD1: begin
	       mdio_tri <= 0;
	       mdio_out <= mdio_operation[9];
	       state <= PRTAD2;	       
	    end
	    PRTAD2: begin
	       mdio_tri <= 0;
	       mdio_out <= mdio_operation[8];
	       state <= PRTAD3;	       
	    end
	    PRTAD3: begin
	       mdio_tri <= 0;
	       mdio_out <= mdio_operation[7];
	       state <= PRTAD4;	       
	    end
	    PRTAD4: begin
	       mdio_tri <= 0;
	       mdio_out <= mdio_operation[6];
	       state <= PRTAD5;	       
	    end
	    PRTAD5: begin
	       mdio_tri <= 0;
	       mdio_out <= mdio_operation[5];
	       state <= DEVAD1;	       
	    end
	    DEVAD1: begin
	       mdio_tri <= 0;
	       mdio_out <= mdio_operation[4];
	       state <= DEVAD2;	       
	    end
	    DEVAD2: begin
	       mdio_tri <= 0;
	       mdio_out <= mdio_operation[3];
	       state <= DEVAD3;	       
	    end
	    DEVAD3: begin
	       mdio_tri <= 0;
	       mdio_out <= mdio_operation[2];
	       state <= DEVAD4;	       
	    end
	    DEVAD4: begin
	       mdio_tri <= 0;
	       mdio_out <= mdio_operation[1];
	       state <= DEVAD5;	       
	    end
	    DEVAD5: begin
	       mdio_tri <= 0;
	       mdio_out <= mdio_operation[0];
	       state <= TA1;	       
	    end
	    //
	    // Both Clause 22 & Clause 45 use the same turn around on the bus.
	    // Reads have Z as the first bit and 0 driven by the slave for the 2nd bit.
	    // Note that slaves drive the bus on the rising edge of MDC.
	    // Writes and Address cycles have 10 driven by the master.
	    //
	    TA1: begin
	       if (mdio_operation[11] == 0) // Write/Address
		 begin
		    mdio_tri <= 0;
		    mdio_out <= 1;
		    state <= TA2;
		 end
	       else // Read
		 begin
		    mdio_tri <= 1;
		    state <= TA3;
		 end
	    end
	    TA2: begin
	       mdio_tri <= 0;
	       mdio_out <= 0;
	       if (!mdio_operation[12]) // Clause 22 Write
		 state <= WRITE1;
	       else if (mdio_operation[10]) // Clause 45 Write
		 state <= WRITE1;
	       else // Clause 45 ADDRESS
		 state <= C45_ADDR1;
	    end
	    TA3: begin
	       mdio_tri <= 1;
	       state <= READ1;
	    end
	    //
	    // Clause 22 Reads and both forms of clause 45 Reads have the same bus transaction from here out.
	    //
	    READ1: begin
	       mdio_tri <= 1;	
	       mdio_read_data[15] <= mdio_in;
	       state <= READ2;	    
	    end
	    READ2: begin
	       mdio_tri <= 1;	
	       mdio_read_data[14] <= mdio_in;
	       state <= READ3;	    
	    end
	    READ3: begin
	       mdio_tri <= 1;	
	       mdio_read_data[13] <= mdio_in;
	       state <= READ4;	    
	    end
	    READ4: begin
	       mdio_tri <= 1;	
	       mdio_read_data[12] <= mdio_in;
	       state <= READ5;	    
	    end
	    READ5: begin
	       mdio_tri <= 1;	
	       mdio_read_data[11] <= mdio_in;
	       state <= READ6;	    
	    end
	    READ6: begin
	       mdio_tri <= 1;	
	       mdio_read_data[10] <= mdio_in;
	       state <= READ7;	    
	    end
	    READ7: begin
	       mdio_tri <= 1;	
	       mdio_read_data[9] <= mdio_in;
	       state <= READ8;	    
	    end
	    READ8: begin
	       mdio_tri <= 1;	
	       mdio_read_data[8] <= mdio_in;
	       state <= READ9;	    
	    end
	    READ9: begin
	       mdio_tri <= 1;	
	       mdio_read_data[7] <= mdio_in;
	       state <= READ10;	    
	    end
	    READ10: begin
	       mdio_tri <= 1;	
	       mdio_read_data[6] <= mdio_in;
	       state <= READ11;	    
	    end
	    READ11: begin
	       mdio_tri <= 1;	
	       mdio_read_data[5] <= mdio_in;
	       state <= READ12;	    
	    end
	    READ12: begin
	       mdio_tri <= 1;	
	       mdio_read_data[4] <= mdio_in;
	       state <= READ13;	    
	    end
	    READ13: begin
	       mdio_tri <= 1;	
	       mdio_read_data[3] <= mdio_in;
	       state <= READ14;	    
	    end
	    READ14: begin
	       mdio_tri <= 1;	
	       mdio_read_data[2] <= mdio_in;
	       state <= READ15;	    
	    end
	    READ15: begin
	       mdio_tri <= 1;	
	       mdio_read_data[1] <= mdio_in;
	       state <= READ16;	    
	    end
	    READ16: begin
	       mdio_tri <= 1;	
	       mdio_read_data[0] <= mdio_in;
	       state <= PREIDLE;	 
	       mdio_done <= 1;	          
	    end	    
	    //
	    // Write 16bits of data for all types of Write.
	    //
	    WRITE1:begin
	       mdio_tri <= 0;
	       mdio_out <= mdio_write_data[15];
	       state <= WRITE2;
	    end
	    WRITE2:begin
	       mdio_tri <= 0;
	       mdio_out <= mdio_write_data[14];
	       state <= WRITE3;
	    end
	    WRITE3:begin
	       mdio_tri <= 0;
	       mdio_out <= mdio_write_data[13];
	       state <= WRITE4;
	    end
	    WRITE4:begin
	       mdio_tri <= 0;
	       mdio_out <= mdio_write_data[12];
	       state <= WRITE5;
	    end
	    WRITE5:begin
	       mdio_tri <= 0;
	       mdio_out <= mdio_write_data[11];
	       state <= WRITE6;
	    end
	    WRITE6:begin
	       mdio_tri <= 0;
	       mdio_out <= mdio_write_data[10];
	       state <= WRITE7;
	    end
	    WRITE7:begin
	       mdio_tri <= 0;
	       mdio_out <= mdio_write_data[9];
	       state <= WRITE8;
	    end
	    WRITE8:begin
	       mdio_tri <= 0;
	       mdio_out <= mdio_write_data[8];
	       state <= WRITE9;
	    end
	    WRITE9:begin
	       mdio_tri <= 0;
	       mdio_out <= mdio_write_data[7];
	       state <= WRITE10;
	    end
	    WRITE10:begin
	       mdio_tri <= 0;
	       mdio_out <= mdio_write_data[6];
	       state <= WRITE11;
	    end
	    WRITE11:begin
	       mdio_tri <= 0;
	       mdio_out <= mdio_write_data[5];
	       state <= WRITE12;
	    end
	    WRITE12:begin
	       mdio_tri <= 0;
	       mdio_out <= mdio_write_data[4];
	       state <= WRITE13;
	    end
	    WRITE13:begin
	       mdio_tri <= 0;
	       mdio_out <= mdio_write_data[3];
	       state <= WRITE14;
	    end
	    WRITE14:begin
	       mdio_tri <= 0;
	       mdio_out <= mdio_write_data[2];
	       state <= WRITE15;
	    end
	    WRITE15:begin
	       mdio_tri <= 0;
	       mdio_out <= mdio_write_data[1];
	       state <= WRITE16;
	    end
	    WRITE16:begin
	       mdio_tri <= 0;
	       mdio_out <= mdio_write_data[0];
	       state <= PREIDLE;
	       mdio_done <= 1;	       
	    end
	    //
	    // Write 16bits of address for a Clause 45 Address transaction
	    //
	    C45_ADDR1:begin
	       mdio_tri <= 0;
	       mdio_out <= mdio_address[15];
	       state <= C45_ADDR2;
	    end
	    C45_ADDR2:begin
	       mdio_tri <= 0;
	       mdio_out <= mdio_address[14];
	       state <= C45_ADDR3;
	    end
	    C45_ADDR3:begin
	       mdio_tri <= 0;
	       mdio_out <= mdio_address[13];
	       state <= C45_ADDR4;
	    end
	    C45_ADDR4:begin
	       mdio_tri <= 0;
	       mdio_out <= mdio_address[12];
	       state <= C45_ADDR5;
	    end
	    C45_ADDR5:begin
	       mdio_tri <= 0;
	       mdio_out <= mdio_address[11];
	       state <= C45_ADDR6;
	    end
	    C45_ADDR6:begin
	       mdio_tri <= 0;
	       mdio_out <= mdio_address[10];
	       state <= C45_ADDR7;
	    end
	    C45_ADDR7:begin
	       mdio_tri <= 0;
	       mdio_out <= mdio_address[9];
	       state <= C45_ADDR8;
	    end
	    C45_ADDR8:begin
	       mdio_tri <= 0;
	       mdio_out <= mdio_address[8];
	       state <= C45_ADDR9;
	    end
	    C45_ADDR9:begin
	       mdio_tri <= 0;
	       mdio_out <= mdio_address[7];
	       state <= C45_ADDR10;
	    end
	    C45_ADDR10:begin
	       mdio_tri <= 0;
	       mdio_out <= mdio_address[6];
	       state <= C45_ADDR11;
	    end
	    C45_ADDR11:begin
	       mdio_tri <= 0;
	       mdio_out <= mdio_address[5];
	       state <= C45_ADDR12;
	    end
	    C45_ADDR12:begin
	       mdio_tri <= 0;
	       mdio_out <= mdio_address[4];
	       state <= C45_ADDR13;
	    end
	    C45_ADDR13:begin
	       mdio_tri <= 0;
	       mdio_out <= mdio_address[3];
	       state <= C45_ADDR14;
	    end
	    C45_ADDR14:begin
	       mdio_tri <= 0;
	       mdio_out <= mdio_address[2];
	       state <= C45_ADDR15;
	    end
	    C45_ADDR15:begin
	       mdio_tri <= 0;
	       mdio_out <= mdio_address[1];
	       state <= C45_ADDR16;
	    end
	    C45_ADDR16:begin
	       mdio_tri <= 0;
	       mdio_out <= mdio_address[0];
	       state <= PREIDLE;
	       mdio_done <= 1;	       
	    end
	    //
	    // PREIDLE allows the mdio_running bit to reset.
	    //
	    PREIDLE: begin
	       state <= IDLE;
	    end
	  endcase // case(state)
	  
       end // if (mdc_falling_edge)


   
`endif //  ifdef MDIO
		   
endmodule
 

// $Header: /devl/xcs/repo/env/Databases/CAEInterfaces/verunilibs/data/spartan4/ODDR2.v,v 1.1 2004/06/21 21:45:36 wloo Exp $
///////////////////////////////////////////////////////////////////////////////
// Copyright (c) 1995/2004 Xilinx, Inc.
// All Right Reserved.
///////////////////////////////////////////////////////////////////////////////
//   ____  ____
//  /   /\/   /
// /___/  \  /    Vendor : Xilinx
// \   \   \/     Version : 10.1
//  \   \         Description : Xilinx Functional Simulation Library Component
//  /   /                  Dual Data Rate Output D Flip-Flop
// /___/   /\     Filename : ODDR2.v
// \   \  /  \    Timestamp : Thu Mar 25 16:43:52 PST 2004
//  \___\/\___\
//
// Revision:
//    03/23/04 - Initial version.

`timescale 1 ps / 1 ps

module ODDR2 (Q, C0, C1, CE, D0, D1, R, S);
    
    output Q;

    input C0;
    input C1;
    input CE;
    input D0;
    input D1;
    tri0 GSR = glbl.GSR;
    input R;
    input S;

    parameter DDR_ALIGNMENT = "NONE";    
    parameter INIT = 1'b0;
    parameter SRTYPE = "SYNC";

    reg q_out, q_d0_c1_out_int, q_d1_c0_out_int;    
    
    buf buf_q (Q, q_out);


    initial begin

	if ((INIT != 1'b0) && (INIT != 1'b1)) begin
	    $display("Attribute Syntax Error : The attribute INIT on ODDR2 instance %m is set to %d.  Legal values for this attribute are 0 or 1.", INIT);
	    $finish;
	end
	
    	if ((DDR_ALIGNMENT != "NONE") && (DDR_ALIGNMENT != "C0") && (DDR_ALIGNMENT != "C1")) begin
	    $display("Attribute Syntax Error : The attribute DDR_ALIGNMENT on ODDR2 instance %m is set to %s.  Legal values for this attribute are NONE, C0 or C1.", DDR_ALIGNMENT);
	    $finish;
	end
	
	if ((SRTYPE != "ASYNC") && (SRTYPE != "SYNC")) begin
	    $display("Attribute Syntax Error : The attribute SRTYPE on ODDR2 instance %m is set to %s.  Legal values for this attribute are ASYNC or SYNC.", SRTYPE);
	    $finish;
	end

    end // initial begin
    

    always @(GSR or R or S) begin

	if (GSR == 1) begin

	    assign q_out = INIT;
	    assign q_d0_c1_out_int = INIT;
	    assign q_d1_c0_out_int = INIT;

	end
	else begin

	    deassign q_out;
	    deassign q_d0_c1_out_int;
	    deassign q_d1_c0_out_int;
	    
	    if (SRTYPE == "ASYNC") begin
		if (R == 1) begin
		    assign q_out = 0;
		    assign q_d0_c1_out_int = 0;
		    assign q_d1_c0_out_int = 0;
		end
		else if (R == 0 && S == 1) begin
		    assign q_out = 1;
		    assign q_d0_c1_out_int = 1;
		    assign q_d1_c0_out_int = 1;
		end
	    end // if (SRTYPE == "ASYNC")
	    
	end // if (GSR == 1'b0)
	
    end // always @ (GSR or R or S)

    
    always @(posedge C0) begin
	
 	if (R == 1 && SRTYPE == "SYNC") begin
	    q_out <= 0;
	    q_d1_c0_out_int <= 0;
	end
	else if (R == 0 && S == 1 && SRTYPE == "SYNC") begin
	    q_out <= 1;
	    q_d1_c0_out_int <= 1;
	end
	else if (CE == 1 && R == 0 && S == 0) begin
	    
	    if (DDR_ALIGNMENT == "C1")
		q_out <= q_d0_c1_out_int;
	    else begin 
		q_out <= D0;
		
		if (DDR_ALIGNMENT == "C0")
		    q_d1_c0_out_int <= D1;
	    end
	    
	end // if (CE == 1 && R == 0 && S == 0)
	
    end // always @ (posedge C0)
    
    
    always @(posedge C1) begin
	
	if (R == 1 && SRTYPE == "SYNC") begin
	    q_out <= 0;
	    q_d0_c1_out_int <= 0;
	end
	else if (R == 0 && S == 1 && SRTYPE == "SYNC") begin
	    q_out <= 1;
	    q_d0_c1_out_int <= 1;
	    end
	else if (CE == 1 && R == 0 && S == 0) begin
	    
	    if (DDR_ALIGNMENT == "C0")
		q_out <= q_d1_c0_out_int;
	    else begin 
		q_out <= D1;
		
		if (DDR_ALIGNMENT == "C1")
		    q_d0_c1_out_int <= D0;
	    end
	    
	end // if (CE == 1 && R == 0 && S == 0)
	
    end // always @ (negedge c_in)
    
    
    specify

	if (C0) (C0 => Q) = (100, 100);
	if (C1) (C1 => Q) = (100, 100);
	specparam PATHPULSE$ = 0;

    endspecify

endmodule // ODDR2


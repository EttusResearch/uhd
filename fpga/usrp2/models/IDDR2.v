// $Header: /devl/xcs/repo/env/Databases/CAEInterfaces/verunilibs/data/spartan4/IDDR2.v,v 1.1 2004/06/21 21:45:36 wloo Exp $
///////////////////////////////////////////////////////////////////////////////
// Copyright (c) 1995/2004 Xilinx, Inc.
// All Right Reserved.
///////////////////////////////////////////////////////////////////////////////
//   ____  ____
//  /   /\/   /
// /___/  \  /    Vendor : Xilinx
// \   \   \/     Version : 10.1
//  \   \         Description : Xilinx Functional Simulation Library Component
//  /   /                  Dual Data Rate Input D Flip-Flop
// /___/   /\     Filename : IDDR2.v
// \   \  /  \    Timestamp : Thu Mar 25 16:43:51 PST 2004
//  \___\/\___\
//
// Revision:
//    03/23/04 - Initial version.

`timescale  1 ps / 1 ps

module IDDR2 (Q0, Q1, C0, C1, CE, D, R, S);
    
    output Q0;
    output Q1;
    
    input C0;
    input C1;
    input CE;
    input D;
    tri0 GSR = glbl.GSR;
    input R;
    input S;

    parameter DDR_ALIGNMENT = "NONE";
    parameter INIT_Q0 = 1'b0;
    parameter INIT_Q1 = 1'b0;
    parameter SRTYPE = "SYNC";

    reg q0_out, q1_out;
    reg q0_out_int, q1_out_int;
    reg q0_c1_out_int, q1_c0_out_int;
    
    buf buf_q0 (Q0, q0_out);
    buf buf_q1 (Q1, q1_out);

    
    initial begin

	if ((INIT_Q0 != 1'b0) && (INIT_Q0 != 1'b1)) begin
	    $display("Attribute Syntax Error : The attribute INIT_Q0 on IDDR2 instance %m is set to %d.  Legal values for this attribute are 0 or 1.", INIT_Q0);
	    $finish;
	end
	
    	if ((INIT_Q1 != 1'b0) && (INIT_Q1 != 1'b1)) begin
	    $display("Attribute Syntax Error : The attribute INIT_Q0 on IDDR2 instance %m is set to %d.  Legal values for this attribute are 0 or 1.", INIT_Q1);
	    $finish;
	end

    	if ((DDR_ALIGNMENT != "C1") && (DDR_ALIGNMENT != "C0") && (DDR_ALIGNMENT != "NONE")) begin
	    $display("Attribute Syntax Error : The attribute DDR_ALIGNMENT on IDDR2 instance %m is set to %s.  Legal values for this attribute are C0, C1 or NONE.", DDR_ALIGNMENT);
	    $finish;
	end
	
	if ((SRTYPE != "ASYNC") && (SRTYPE != "SYNC")) begin
	    $display("Attribute Syntax Error : The attribute SRTYPE on IDDR2 instance %m is set to %s.  Legal values for this attribute are ASYNC or SYNC.", SRTYPE);
	    $finish;
	end

    end // initial begin


    always @(GSR or R or S) begin

	if (GSR == 1) begin

	    assign q0_out_int = INIT_Q0;
	    assign q1_out_int = INIT_Q1;
	    assign q0_c1_out_int = INIT_Q0;
	    assign q1_c0_out_int = INIT_Q1;

	end
	else begin
	    
	    deassign q0_out_int;
	    deassign q1_out_int;
	    deassign q0_c1_out_int;
	    deassign q1_c0_out_int;
	    
	    if (SRTYPE == "ASYNC") begin
		if (R == 1) begin
		    assign q0_out_int = 0;
		    assign q1_out_int = 0;
		    assign q0_c1_out_int = 0;
		    assign q1_c0_out_int = 0;
		end
		else if (R == 0 && S == 1) begin
		    assign q0_out_int = 1;
		    assign q1_out_int = 1;
		    assign q0_c1_out_int = 1;
		    assign q1_c0_out_int = 1;
		end
	    end // if (SRTYPE == "ASYNC")
	    
	end // if (GSR == 1'b0)
	
    end // always @ (GSR or R or S)

	    
    always @(posedge C0) begin
 	if (R == 1 && SRTYPE == "SYNC") begin
	    q0_out_int <= 0;
	    q1_c0_out_int <= 0;
	end
	else if (R == 0 && S == 1 && SRTYPE == "SYNC") begin
	    q0_out_int <= 1;
	    q1_c0_out_int <= 1;
	end
	else if (CE == 1 && R == 0 && S == 0) begin
            q0_out_int <= D;
	    q1_c0_out_int <= q1_out_int;
	end
    end // always @ (posedge C0)

    
    always @(posedge C1) begin
 	if (R == 1 && SRTYPE == "SYNC") begin
	    q1_out_int <= 0;
	    q0_c1_out_int <= 0;
	end
	else if (R == 0 && S == 1 && SRTYPE == "SYNC") begin
	    q1_out_int <= 1;
	    q0_c1_out_int <= 1;
	end
	else if (CE == 1 && R == 0 && S == 0) begin
            q1_out_int <= D;
	    q0_c1_out_int <= q0_out_int;
	end
    end // always @ (posedge C1)
    
    
    always @(q0_out_int or q1_out_int or q1_c0_out_int or q0_c1_out_int) begin

	case (DDR_ALIGNMENT)
	    "NONE" : begin
		       q0_out <= q0_out_int;
		       q1_out <= q1_out_int;
	             end
	    "C0" : begin
	               q0_out <= q0_out_int;
	               q1_out <= q1_c0_out_int;
	           end
	    "C1" : begin
		       q0_out <= q0_c1_out_int;
                       q1_out <= q1_out_int;
	           end
	endcase // case(DDR_ALIGNMENT)

    end // always @ (q0_out_int or q1_out_int or q1_c0_out_int or q0_c1_out_int)
    

    specify

	if (C0) (C0 => Q0) = (100, 100);
	if (C0) (C0 => Q1) = (100, 100);
	if (C1) (C1 => Q1) = (100, 100);
	if (C1) (C1 => Q0) = (100, 100);
	specparam PATHPULSE$ = 0;

    endspecify

endmodule // IDDR2


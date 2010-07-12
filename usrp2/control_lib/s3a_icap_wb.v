

module s3a_icap_wb
  (input clk, input reset,
   input cyc_i, input stb_i, input we_i, output ack_o,
   input [31:0] dat_i, output [31:0] dat_o);

   assign dat_o[31:8] = 24'd0;
   
   wire 	BUSY, CE, WRITE;
   
   reg [2:0] 	icap_state;
   localparam ICAP_IDLE  = 0;
   localparam ICAP_WR0 	 = 1;
   localparam ICAP_WR1 	 = 2;
   localparam ICAP_RD0 	 = 3;
   localparam ICAP_RD1 	 = 4;

   always @(posedge clk)
     if(reset)
       icap_state 	<= ICAP_IDLE;
     else
       case(icap_state)
	 ICAP_IDLE :
	   begin
	      if(stb_i & cyc_i)
		if(we_i)
		  icap_state <= ICAP_WR0;
		else
		  icap_state <= ICAP_RD0;
	   end
	 ICAP_WR0 :
	   icap_state <= ICAP_WR1;
	 ICAP_WR1 :
	   icap_state <= ICAP_IDLE;
	 ICAP_RD0 :
	   icap_state <= ICAP_RD1;
	 ICAP_RD1 :
	   icap_state <= ICAP_IDLE;
       endcase // case (icap_state)

   assign WRITE 	 = (icap_state == ICAP_WR0) | (icap_state == ICAP_WR1);
   assign CE 		 = (icap_state == ICAP_WR1) | (icap_state == ICAP_RD0);

   assign ack_o = (icap_state == ICAP_WR1) | (icap_state == ICAP_RD1);
   
   ICAP_SPARTAN3A ICAP_SPARTAN3A_inst 
     (.BUSY(BUSY),          // Busy output
      .O(dat_o[7:0]),            // 32-bit data output
      .CE(~CE),              // Clock enable input
      .CLK(clk),            // Clock input
      .I(dat_i[7:0]),            // 32-bit data input
      .WRITE(~WRITE)         // Write input
      );

endmodule // s3a_icap_wb

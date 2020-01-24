//
// Copyright 2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//

module chdr_8sc_to_16sc
  #(parameter BASE=0)
   (input clk, input reset,
    input set_stb, input [7:0] set_addr, input [31:0] set_data,

    input [63:0] i_tdata,
    input  i_tlast,
    input  i_tvalid,
    output i_tready,

    output reg [63:0] o_tdata,
    output        o_tlast,
    output        o_tvalid,
    input         o_tready,

    output [31:0] debug
    );

   //bit assignments
   wire 	  chdr_has_hdr = 1'b1;
   wire 	  chdr_has_time = i_tdata[61];
   wire 	  chdr_has_tlr = 1'b0;
   wire 	  set_sid;

   //chdr length calculations
   wire [15:0] 	  chdr_header_lines16 = chdr_has_time? 16 : 8;

   wire [15:0] 	  chdr_almost_payload_lines16 = ((i_tdata[47:32] - chdr_header_lines16) << 1);

   wire [15:0] 	  chdr_payload_lines16 = chdr_almost_payload_lines16 + chdr_header_lines16;

   //new destination reg set
   wire [15:0] 	  my_newhome;


   setting_reg #(.my_addr(BASE), .width(17)) new_destination
     (.clk(clk), .rst(reset), .strobe(set_stb), .addr(set_addr), .in(set_data),
      .out({set_sid, my_newhome[15:0]}));


   //state declarations


   localparam HEADER        = 2'd0;//IDLE
   localparam TIME = 2'd1;
   localparam ODD           = 2'd2;
   localparam EVEN          = 2'd3;

   reg [1:0] 	   state;
   reg 		   end_on_odd;

   always @(posedge clk) begin
      if (reset) begin
         state <= HEADER;
	 end_on_odd <= 1'b0;
      end
      else case(state)

             HEADER: begin
		if (i_tvalid && o_tready) begin
		   state <= (i_tdata[61])? TIME : ODD;
           	   end_on_odd <= (i_tdata[34:32] > 0) && (i_tdata[34:32] < 5);
		end

             end

             TIME: begin
		if (i_tvalid && o_tready) begin

		   state <= (i_tlast)? HEADER: ODD;
		end
             end

             ODD: begin
		if (i_tvalid && o_tready) begin
		   state <= (i_tlast & end_on_odd) ? HEADER : EVEN;
		end
             end

             EVEN: begin
		if (i_tvalid && o_tready)
		  state <= (i_tlast) ? HEADER: ODD;
             end
	     default: state <= HEADER;
	     
	   endcase
   end
   
   always @(*) 
     case(state)
	HEADER: o_tdata <= {i_tdata[63:48], chdr_payload_lines16,
			  set_sid ? {i_tdata[15:0], my_newhome[15:0]}:i_tdata[31:0]};
	TIME: o_tdata <= i_tdata;
       ODD: o_tdata <= {i_tdata[63:56], 8'h0, i_tdata[55:48] , 8'h0, i_tdata[47:40], 8'h0, i_tdata[39:32] , 8'h0};
       EVEN: o_tdata <= {i_tdata[31:24], 8'h0, i_tdata[23:16], 8'h0, i_tdata[15:8], 8'h0, i_tdata[7:0], 8'h0};
   
       
       default : o_tdata = i_tdata;
     endcase
      
   assign o_tvalid = i_tvalid;
   assign i_tready = o_tready && ((state != ODD) || (i_tlast & end_on_odd));
   assign o_tlast  = i_tlast && ((state == EVEN)||((state == ODD) & end_on_odd));

endmodule











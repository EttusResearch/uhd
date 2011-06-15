//
// Copyright 2011 Ettus Research LLC
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//



module time_sender
  (input clk, input rst,
   input [63:0] vita_time,
   input send_sync,
   output reg exp_time_out);

   reg [7:0] datain;
   reg 	     k;
   wire [9:0] dataout;
   reg [9:0]  dataout_reg;
   reg 	      disp_reg;
   wire       disp, new_word;
   reg [4:0]  state;
   reg [3:0]  bit_count;
   
   encode_8b10b encode_8b10b 
     (.datain({k,datain}),.dispin(disp_reg),
      .dataout(dataout),.dispout(disp));

   always @(posedge clk)
     if(rst)
       disp_reg <= 0;
     else if(new_word)
       disp_reg <= disp;
   
   always @(posedge clk)
     if(rst)
       dataout_reg <= 0;
     else if(new_word)
       dataout_reg <= dataout;
     else
       dataout_reg <= {1'b0,dataout_reg[9:1]};
   
   always @(posedge clk)
     exp_time_out <= dataout_reg[0];
   
   assign    new_word = (bit_count == 9);
   
   always @(posedge clk)
     if(rst)
       bit_count <= 0;
     else if(new_word | send_sync)
       bit_count <= 0;
     else
       bit_count <= bit_count + 1;

   localparam SEND_IDLE = 0;
   localparam SEND_HEAD = 1;
   localparam SEND_T0 = 2;
   localparam SEND_T1 = 3;
   localparam SEND_T2 = 4;
   localparam SEND_T3 = 5;
   localparam SEND_T4 = 6;
   localparam SEND_T5 = 7;
   localparam SEND_T6 = 8;
   localparam SEND_T7 = 9;
   localparam SEND_TAIL = 10;

   localparam COMMA = 8'hBC;
   localparam HEAD = 8'h3C;
   localparam TAIL = 8'hF7;
   
   reg [63:0] vita_time_reg;
   
   always @(posedge clk)
     if(rst)
       vita_time_reg <= 0;
     else if(send_sync)
       vita_time_reg <= vita_time;
   
   always @(posedge clk)
     if(rst)
       begin
	  {k,datain} <= 0;
	  state <= SEND_IDLE;
       end
     else
       if(send_sync)
	 state <= SEND_HEAD;
       else if(new_word)
	 case(state)
	   SEND_IDLE :
	     {k,datain} <= {1'b1,COMMA};
	   SEND_HEAD :
	     begin
		{k,datain} <= {1'b1, HEAD};
		state <= SEND_T0;
	     end
	   SEND_T0 :
	     begin
		{k,datain} <= {1'b0, vita_time_reg[63:56] };
		state <= SEND_T1;
	     end
	   SEND_T1 :
	     begin
		{k,datain} <= {1'b0, vita_time_reg[55:48]};
		state <= SEND_T2;
	     end
	   SEND_T2 :
	     begin
		{k,datain} <= {1'b0, vita_time_reg[47:40]};
		state <= SEND_T3;
	     end
	   SEND_T3 :
	     begin
		{k,datain} <= {1'b0, vita_time_reg[39:32]};
		state <= SEND_T4;
	     end
	   SEND_T4 :
	     begin
		{k,datain} <= {1'b0, vita_time_reg[31:24]};
		state <= SEND_T5;
	     end
	   SEND_T5 :
	     begin
		{k,datain} <= {1'b0, vita_time_reg[23:16]};
		state <= SEND_T6;
	     end
	   SEND_T6 :
	     begin
		{k,datain} <= {1'b0, vita_time_reg[15:8]};
		state <= SEND_T7;
	     end
	   SEND_T7 :
	     begin
		{k,datain} <= {1'b0, vita_time_reg[7:0]};
		state <= SEND_TAIL;
	     end
	   SEND_TAIL :
	     begin
		{k,datain} <= {1'b1, TAIL};
		state <= SEND_IDLE;
	     end
	   default :
	     state <= SEND_IDLE;
	 endcase // case(state)
   
endmodule // time_sender

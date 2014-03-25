//
// AXI stream neds N+1 bits to transmit packets of N bits so that the LAST bit can be represented.
// LAST occurs relatively infrequently and can be synthesized by using an in-band ESC code to generate
// a multi-word sequence to encode it (and the escape character when it appears as data input).
//
// 0x1234567887654321 with last becomes
// 0xDEADBEEFFEEDCAFE 0x0000000000000001 0x1234567887654321
//
// 0xDEADBEEFFEEDCAFE with last becomes
// 0xDEADBEEFFEEDCAFE 0x0000000000000001 0xDEADBEEFFEEDCAFE
//
// 0xDEADBEEFFEEDCAFE without last becomes
// 0xDEADBEEFFEEDCAFE 0x0000000000000000 0xDEADBEEFFEEDCAFE
//

module axi_embed_tlast
   #(parameter WIDTH=64)
  (
   input clk,
   input reset,
   input clear,
   //
   input [WIDTH-1:0] i_tdata,
   input i_tlast,
   input i_tvalid,
   output i_tready,
   //
   output reg [WIDTH-1:0] o_tdata,
   output o_tvalid,
   input o_tready
   
   );

   localparam PASS = 0;
   localparam ZERO = 1;
   localparam ONE = 2;
   localparam ESCAPE = 3;

   localparam IDLE = 0;
   localparam LAST = 1;
   localparam ESC = 2;
   localparam FINISH = 3;
   
   reg [1:0]  state, next_state;

   reg [1:0]  select;

   reg [31:0] checksum;
   
   always @(posedge clk) 
     if (reset | clear) begin
	checksum <= 0;
     end else if (i_tready && i_tvalid && i_tlast) begin
	checksum <= 0;
     end else if (i_tready && i_tvalid) begin
	checksum <= checksum + i_tdata[31:0] + i_tdata[63:32];
     end 
   
   always @(posedge clk) 
     if (reset | clear) begin
	state <= IDLE;
     end else begin if (o_tready)
	state <= next_state;
     end 
   
   always @(*) begin
      case(state)
	IDLE: begin
	   if (i_tlast && i_tvalid)
	     begin
		next_state = LAST;
		select = ESCAPE;
	     end
	   else if ((i_tdata == 64'hDEADBEEFFEEDCAFE) && i_tvalid)
	     begin
		next_state = ESC;
		select = ESCAPE;
	     end
	   else
	     begin
		next_state = IDLE;
		select = PASS;
	     end
	end // case: IDLE
	//
	//
	LAST: begin
	   select = ONE;
	   next_state = FINISH;
	end
	//
	//
	ESC: begin
	   select = ZERO;
	   next_state = FINISH;
	end
	//
	//
	FINISH: begin
	   select = PASS;
	   if (i_tvalid)
	     next_state = IDLE;
	   else
	     next_state = FINISH;
	end
      endcase // case(state)
   end // always @ (*)
   	      
   //
   // Muxes
   //
   always @*
     begin
	case(select)
	  PASS:   o_tdata = i_tdata;	  
	  ZERO:   o_tdata = 0;
	  ONE:    o_tdata = {checksum[31:0],32'h1};
	  ESCAPE: o_tdata = 64'hDEADBEEFFEEDCAFE;
	endcase // case(select)
     end

   assign o_tvalid = (select == PASS) ? i_tvalid : 1'b1;
   assign i_tready = (select == PASS) ? o_tready : 1'b0;

endmodule // axi_embed_tlast




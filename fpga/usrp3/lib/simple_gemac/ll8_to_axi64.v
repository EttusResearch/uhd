
// Takes 8-bit wide data on a Local-link fifo interface and converts to 64-bit wide axi
//  Parameter START_BYTE controls which byte of the 8 the first incoming byte goes into
//   Use START_BYTE=6 with ethernet to nicely align the words for processing of IP packets
//  Parameter LABEL specifies a value to put in the high word of the very first packet.
//   This is useful for labeling packets with the port it came from so downstream knows
//   how to process it.  LABEL gets overwritten if START_BYTE = 0

module ll8_to_axi64
  #(parameter START_BYTE=6,
    parameter LABEL=8'h00)
   (input clk, input reset, input clear,
    input [7:0] ll_data, input ll_eof, input ll_error, input ll_src_rdy, output ll_dst_rdy,
    output [63:0] axi64_tdata, output axi64_tlast, output [3:0] axi64_tuser, output axi64_tvalid, input axi64_tready);

   wire 	  error_int, eof_int;
   wire [7:0] 	  data_int;
   wire 	  valid_int, ready_int;
   
   axi_fifo_short #(.WIDTH(10)) ll8_fifo
     (.clk(clk), .reset(reset), .clear(0),
      .i_tdata({ll_error, ll_eof, ll_data}), .i_tvalid(ll_src_rdy), .i_tready(ll_dst_rdy),
      .o_tdata({error_int, eof_int, data_int}), .o_tvalid(valid_int), .o_tready(ready_int),
      .space(), .occupied());

   wire [7:0] 	  label_wire = LABEL; // Enforces parameter width
   
   reg [3:0] 	  state = START_BYTE;
   reg [63:0] 	  holding; // = {label_wire, 56'h0};
   reg 		  err, eof, done;
   reg [3:0] 	  occ;

   localparam WAIT = 4'd8;
   
   always @(posedge clk)
     if(reset | clear)
       begin
	  state <= START_BYTE;
	  holding <= {label_wire, 56'h0};
	  err <= 1'b0;
	  eof <= 1'b0;
	  occ <= 3'd0;
	  done <= 1'b0;
       end
     else
       if(state == WAIT)
	 begin
	    state <= START_BYTE;
	    done <= 1'b0;
	    holding <= {label_wire, 56'h0};
	 end
       else if(valid_int & ready_int)
	 begin
	    case(state)
	      4'd0: 
		begin
		   holding[63:56] <= data_int;
		   holding[55:0] <= 56'h0;
		end
	      4'd1: holding[55:48] <= data_int;
	      4'd2: holding[47:40] <= data_int;
	      4'd3: holding[39:32] <= data_int;
	      4'd4: holding[31:24] <= data_int;
	      4'd5: holding[23:16] <= data_int;
	      4'd6: holding[15:8] <= data_int;
	      4'd7: holding[7:0] <= data_int;
	    endcase // case (state)
	    
	    err <= error_int;
	    eof <= eof_int;
            if(error_int | eof_int)
	      begin
		 occ <= state+1;
		 done <= 1'b1;
		 state <= WAIT;
	      end
	    else if (state == 4'd7)
	      begin
		 occ <= 3'd0;
		 done <= 1'b1;
		 state <= 4'd0;
	      end
	    else
	      begin
		 occ <= 3'd0;
		 done <= 1'b0;
		 state <= state + 4'd1;
	      end // else: !if(state == 4'd7)
	    
	 end // if (valid_int & ready_int)
       else
	 done <= 1'b0;
   
   assign axi64_tdata = holding;
   assign axi64_tlast = eof;
   assign axi64_tuser[3] = err;
   assign axi64_tuser[2:0] = occ;

   assign ready_int = axi64_tready & (state != WAIT);
   assign axi64_tvalid = done;
   		  
endmodule // ll8_to_axi64

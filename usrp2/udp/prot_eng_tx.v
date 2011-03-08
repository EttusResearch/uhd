
//   The first line:
//          Bits 18:17   Select which source/dest pair
//          Bit  16      1 for fast path (accelerated protocol)
//          Bits 15:0    Length in bytes

module prot_eng_tx
  #(parameter BASE=0)
   (input clk, input reset, input clear,
    input set_stb, input [7:0] set_addr, input [31:0] set_data,
    input [35:0] datain, input src_rdy_i, output dst_rdy_o,
    output [35:0] dataout, output src_rdy_o, input dst_rdy_i);
   
   // Store header values in a small dual-port (distributed) ram
   reg [31:0] 	  header_ram[0:63];
   wire [31:0] 	  header_word;
   reg [3:0] 	  state;
   reg [1:0] 	  port_sel;
   
   always @(posedge clk)
     if(set_stb & ((set_addr & 8'hC0) == BASE))
       header_ram[set_addr[5:0]] <= set_data;

   assign header_word = header_ram[{port_sel[1:0],state[3:0]}];

   // Protocol State Machine
   reg [15:0] length;
   wire [15:0] ip_length = length + 28;  // IP HDR + UDP HDR
   wire [15:0] udp_length = length + 8;  //  UDP HDR
   reg 	       sof_o;
   reg [31:0]  dataout_int;
   
   always @(posedge clk)
     if(reset)
       begin
	  state   <= 0;
	  sof_o   <= 0;
       end
     else
       if(src_rdy_i & dst_rdy_i)
	 case(state)
	   0 :
	     begin
		port_sel <= datain[18:17];
		length 	<= datain[15:0];
		sof_o <= 1;
		if(datain[16])
		  state <= 1;
		else
		  state <= 12;
	     end
	   12 :
	     begin
		sof_o <= 0;
		if(datain[33]) // eof
		  state <= 0;
	     end
	   default :
	     begin
		sof_o 	<= 0;
		state <= state + 1;
	     end
	 endcase // case (state)

   wire [15:0] ip_checksum;
   add_onescomp #(.WIDTH(16)) add_onescomp 
     (.A(header_word[15:0]),.B(ip_length),.SUM(ip_checksum));

   always @*
     case(state[2:0])
       1 : dataout_int <= header_word;  // ETH, top half ignored
       2 : dataout_int <= header_word;  // ETH
       3 : dataout_int <= header_word;  // ETH
       4 : dataout_int <= header_word;  // ETH
       5 : dataout_int <= { header_word[31:16], ip_length }; // IP
       6 : dataout_int <= header_word; // IP
       7 : dataout_int <= { header_word[31:16], (16'hFFFF ^ ip_checksum) }; // IP
       8 : dataout_int <= header_word; // IP
       9 : dataout_int <= header_word; // IP
       10: dataout_int <= header_word;  // UDP 
       11: dataout_int <= { udp_length, header_word[15:0]}; // UDP
       default : dataout_int <= datain;
     endcase // case (state[2:0])

   assign dataout = { datain[35:33] & {3{state[3]}},  sof_o, dataout_int };
   assign dst_rdy_o = dst_rdy_i & ((state == 0) | (state == 12));
   assign src_rdy_o = src_rdy_i & (state != 0);
   
endmodule // prot_eng_tx

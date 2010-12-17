
// The input FIFO contents should be 16 bits wide
//   The first word is 1 for fast path (accelerated protocol)
//                     0 for software implemented protocol
//   The second word is the number of bytes in the packet, 
//         and must be valid even if we are in slow path mode
//            Odd means the last word is half full
//   Flags[1:0] is {eop, sop}
//   Protocol word format is:
//             20   UDP Port Here
//             19   Last Header Line
//             18   IP Header Checksum XOR
//             17   IP Length Here
//             16   UDP Length Here
//           15:0   data word to be sent

module prot_eng_tx
  #(parameter BASE=0)
   (input clk, input reset, input clear,
    input set_stb, input [7:0] set_addr, input [31:0] set_data,
    input [18:0] datain, input src_rdy_i, output dst_rdy_o,
    output [18:0] dataout, output src_rdy_o, input dst_rdy_i);

   wire [2:0] flags_i = datain[18:16];
   reg [15:0] dataout_int;
   reg fast_path, sof_o;
   
   wire [2:0] flags_o 	 = {flags_i[2], flags_i[1], sof_o};  // OCC, EOF, SOF

   assign dataout 	 = {flags_o[2:0], dataout_int[15:0]};

   reg [4:0] state;
   wire do_payload 	 = (state == 31);
   
   assign dst_rdy_o 	 = dst_rdy_i & (do_payload | (state==0) | (state==1) | (state==30));
   assign src_rdy_o 	 = src_rdy_i & ~((state==0) | (state==1) | (state==30));
   
   localparam HDR_WIDTH  = 16 + 6;  // 16 bits plus flags
   localparam HDR_LEN 	 = 32;      // Up to 64 bytes of protocol
   
   // Store header values in a small dual-port (distributed) ram
   reg [HDR_WIDTH-1:0] header_ram[0:HDR_LEN-1];
   wire [HDR_WIDTH-1:0] header_word;

   reg [1:0] 		port_sel;
   reg [32:0] 		per_port_data[0:3];
   reg [15:0] 		udp_src_port, udp_dst_port, chk_precompute;

   always @(posedge clk) udp_src_port <= per_port_data[port_sel][31:16];
   always @(posedge clk) udp_dst_port <= per_port_data[port_sel][15:0];

   always @(posedge clk)
     if(set_stb & ((set_addr & 8'hE0) == BASE))
       begin
	  header_ram[set_addr[4:0]] <= set_data;
	  if(set_data[18])
	    chk_precompute <= set_data[15:0];
       end

   always @(posedge clk)
     if(set_stb & ((set_addr & 8'hFC) == (BASE+24)))
       per_port_data[set_addr[1:0]] <= set_data;
   
   wire do_udp_src_port = header_word[21];
   wire do_udp_dst_port = header_word[20];
   wire last_hdr_line   = header_word[19];
   wire do_ip_chk       = header_word[18];
   wire do_ip_len       = header_word[17];
   wire do_udp_len      = header_word[16];

   assign header_word = header_ram[state];
   
   // Protocol State Machine
   reg [15:0] length;
   wire [15:0] ip_length = length + 28;  // IP HDR + UDP HDR
   wire [15:0] udp_length = length + 8;  //  UDP HDR
 
   always @(posedge clk)
     if(reset)
       begin
	  state     <= 0;
	  fast_path <= 0;
	  sof_o   <= 0;
       end
     else
       if(src_rdy_i & dst_rdy_i)
	 case(state)
	   0 :
	     begin
		fast_path <= datain[0];
		port_sel <= datain[2:1];
		state <= 1;
	     end
	   1 :
	     begin
		length 	<= datain[15:0];
		sof_o <= 1;
		if(fast_path)
		  state <= 2;
		else
		  state <= 30;  // Skip 1 word for alignment
	     end
	   30 :
	     state <= 31;
	   31 :
	     begin
		sof_o <= 0;
		if(flags_i[1]) // eop
		  state <= 0;
	     end
	   default :
	     begin
		sof_o 	<= 0;
		if(~last_hdr_line)
		  state <= state + 1;
		else
		  state <= 31;
	     end
	 endcase // case (state)

   wire [15:0] checksum;
   add_onescomp #(.WIDTH(16)) add_onescomp 
     (.A(chk_precompute),.B(ip_length),.SUM(checksum));

   reg [15:0]  checksum_reg;
   always @(posedge clk)
     checksum_reg <= checksum;
   
   always @*
     if(do_payload)
       dataout_int 	<= datain[15:0];
     else if(do_ip_chk)
       dataout_int 	<= 16'hFFFF ^ checksum_reg;
     else if(do_ip_len)
       dataout_int 	<= ip_length;
     else if(do_udp_len)
       dataout_int 	<= udp_length;
     else if(do_udp_src_port)
       dataout_int      <= udp_src_port;
     else if(do_udp_dst_port)
       dataout_int      <= udp_dst_port;
     else
       dataout_int 	<= header_word[15:0];
   
endmodule // prot_eng_tx

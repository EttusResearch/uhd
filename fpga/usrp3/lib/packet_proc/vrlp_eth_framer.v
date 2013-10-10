
// vrlp_eth_framer
//  Takes a vrlp stream in and adds udp, ip, and ethernet framing
//  Uses 8 setting reg addresses.  First 4 are simple registers:
//    BASE+0 : Upper 16 bits of ethernet src mac
//    BASE+1 : Lower 32 bits of ethernet src mac
//    BASE+2 : IP src address
//    BASE+3 : UDP src port
//
//  Next 4 control write ports on a RAM indexed by destination field of stream ID
//    BASE+4 : Dest SID for next 3 regs
//    BASE+5 : Dest IP
//    BASE+6 : Dest UDP port, upper 16 bits of dest mac
//    BASE+7 : Lower 32 bits of dest mac
//
// in_tuser holds streamid

module vrlp_eth_framer
  #(parameter BASE=0)
   (input clk, input reset, input clear,
    input set_stb, input [7:0] set_addr, input [31:0] set_data,
    input [63:0] in_tdata, input [15:0] in_tuser, input in_tlast, input in_tvalid, output in_tready,
    output [63:0] out_tdata, output [3:0] out_tuser, output out_tlast, output out_tvalid, input out_tready,
    output [31:0] debug );

   localparam SR_AWIDTH = 8;
   
   reg [15:0] 	  sid;
   reg [15:0] 	  vrlp_len;
   
   reg [2:0] 	  vef_state;
   localparam VEF_IDLE    = 3'd0;
   localparam VEF_PAYLOAD = 3'd7;
   
   reg [63:0] 	  tdata;
   
   always @(posedge clk)
     if(reset | clear)
       begin
	  vef_state <= VEF_IDLE;
	  sid <= 16'd0;
	  vrlp_len <= 16'd0;
       end
     else
       case(vef_state)
	 VEF_IDLE :
	   if(in_tvalid)
	     begin
		vef_state <= 1;
		sid <= in_tuser[15:0];
		vrlp_len <= in_tdata[15:0]; // modified for VRLP header
	     end
	 VEF_PAYLOAD :
	   if(in_tvalid & out_tready)
	     if(in_tlast)
	       vef_state <= VEF_IDLE;
	 default :
	   if(out_tready)
	     vef_state <= vef_state + 3'd1;
       endcase // case (vef_state)
   
   wire [15:0] vrlp_len_in_bytes = {vrlp_len[13:0],2'b00};  // Vrlp length is in 32-bit words

   assign in_tready = (vef_state == VEF_PAYLOAD) ? out_tready : 1'b0;
   assign out_tvalid = (vef_state == VEF_PAYLOAD) ? in_tvalid : (vef_state == VEF_IDLE) ? 1'b0 : 1'b1;
   assign out_tlast = (vef_state == VEF_PAYLOAD) ? in_tlast : 1'b0;
   assign out_tuser = ((vef_state == VEF_PAYLOAD) & in_tlast) ? {1'b0,vrlp_len_in_bytes[2:0]} : 4'b0000;
   assign out_tdata = tdata;

   wire [47:0] pad = 48'h0;
   wire [47:0] mac_src, mac_dst;
   wire [15:0] eth_type = 16'h0800;
   wire [15:0] misc_ip = { 4'd4 /* IPv4 */, 4'd5 /* IP HDR Len */, 8'h00 /* DSCP and ECN */};
   wire [15:0] ip_len = (16'd28 + vrlp_len_in_bytes);  // 20 for IP, 8 for UDP
   wire [15:0] ident = 16'h0;
   wire [15:0] flag_frag = { 3'b010 /* don't fragment */, 13'h0 };
   wire [15:0] ttl_prot = { 8'h10 /* TTL */, 8'h11 /* UDP */ };
   wire [15:0] iphdr_checksum;
   wire [31:0] ip_src, ip_dst;
   wire [15:0] udp_src, udp_dst;
   wire [15:0] udp_len = (16'd8 + vrlp_len_in_bytes);
   wire [15:0] udp_checksum = 16'h0;

   setting_reg #(.my_addr(BASE), .awidth(SR_AWIDTH), .width(16)) set_mac_upper
     (.clk(clk), .rst(reset),
      .strobe(set_stb), .addr(set_addr), .in(set_data),
      .out(mac_src[47:32]), .changed());
   
   setting_reg #(.my_addr(BASE+1), .awidth(SR_AWIDTH), .width(32)) set_mac_lower
     (.clk(clk), .rst(reset),
      .strobe(set_stb), .addr(set_addr), .in(set_data),
      .out(mac_src[31:0]), .changed());
   
   setting_reg #(.my_addr(BASE+2), .awidth(SR_AWIDTH), .width(32)) set_ip
     (.clk(clk), .rst(reset),
      .strobe(set_stb), .addr(set_addr), .in(set_data),
      .out(ip_src), .changed());

   setting_reg #(.my_addr(BASE+3), .awidth(SR_AWIDTH), .width(16)) set_udp
     (.clk(clk), .rst(reset),
      .strobe(set_stb), .addr(set_addr), .in(set_data),
      .out(udp_src), .changed());

   // Tables of MAC/IP/UDP addresses
   wire [8:0]  ram_addr;  // FIXME we could skip this part if we had wider SR addresses
   
   setting_reg #(.my_addr(BASE+4), .awidth(SR_AWIDTH), .width(9)) set_ram_addr
     (.clk(clk), .rst(reset),
      .strobe(set_stb), .addr(set_addr), .in(set_data),
      .out(ram_addr), .changed());

   ram_2port #(.DWIDTH(32), .AWIDTH(9)) ram_ip
     (.clka(clk), .ena(1'b1), .wea(set_stb & (set_addr == BASE+5)), .addra(ram_addr), .dia(set_data), .doa(),
      .clkb(clk), .enb(1'b1), .web(1'b0), .addrb(sid[8:0]), .dib(32'hFFFF_FFFF), .dob(ip_dst));
   
   ram_2port #(.DWIDTH(32), .AWIDTH(9)) ram_udpmac
     (.clka(clk), .ena(1'b1), .wea(set_stb & (set_addr == BASE+6)), .addra(ram_addr), .dia(set_data), .doa(),
      .clkb(clk), .enb(1'b1), .web(1'b0), .addrb(sid[8:0]), .dib(32'hFFFF_FFFF), .dob({udp_dst,mac_dst[47:32]}));
   
   ram_2port #(.DWIDTH(32), .AWIDTH(9)) ram_maclower
     (.clka(clk), .ena(1'b1), .wea(set_stb & (set_addr == BASE+7)), .addra(ram_addr), .dia(set_data), .doa(),
      .clkb(clk), .enb(1'b1), .web(1'b0), .addrb(sid[8:0]), .dib(32'hFFFF_FFFF), .dob(mac_dst[31:0]));
   
   ip_hdr_checksum ip_hdr_checksum
     (.clk(clk), .in({misc_ip,ip_len,ident,flag_frag,ttl_prot,16'd0,ip_src,ip_dst}),
      .out(iphdr_checksum));

   always @*
     case(vef_state)
       1 : tdata <= { pad[47:0], mac_dst[47:32]};
       2 : tdata <= { mac_dst[31:0], mac_src[47:16]};
       3 : tdata <= { mac_src[15:0], eth_type[15:0], misc_ip[15:0], ip_len[15:0] };
       4 : tdata <= { ident[15:0], flag_frag[15:0], ttl_prot[15:0], iphdr_checksum[15:0]};
       5 : tdata <= { ip_src, ip_dst};
       6 : tdata <= { udp_src, udp_dst, udp_len, udp_checksum};
       default : tdata <= in_tdata;
     endcase // case (vef_state)
      
endmodule // vrlp_eth_framer

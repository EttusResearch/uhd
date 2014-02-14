//
// Copyright 2012 Ettus Research LLC
//


//
// This module is a SIMULATION ONLY bus monitor that probes AXI4-STREAM bus
// that transfer ETTUS non-CHDR VITA49.0 data, providing logging and diagnostoic info.
//

module axi_probe_tb
  #(
    parameter FILENAME="probe.txt",
    parameter VITA_PORT0=0,  // UDP ports to identify VITA
    parameter VITA_PORT1=0,  // UDP ports to identify VITA
    parameter START_AT_VRL=0 // Flags already stripped back to VRL.
    )
   (
    input clk, 
    input reset, 
    input clear,
    input [63:0] tdata,
    input tvalid,
    input tready,
    input tlast
    );
   
   localparam ARP = 16'h0806;
   localparam IPv4 = 16'h0800;
   localparam UDP = 8'h11;
   localparam VRL = 1;
   
   
   

   localparam WAIT_SOF = 0;
   localparam LINE1 = 1;
   localparam LINE2 = 2;
   localparam LINE3 = 3;
   localparam LINE4 = 4;
   localparam LINE5 = 5;
   localparam LINE6 = 6;
   localparam LINE7 = 7;
   
   localparam WAIT_EOF = 15;
   
   reg [3:0]	    out_state;

   reg [63:0] 	    last_line;
   reg [15:0] 	    eth_proto;
   reg [7:0] 	    ip_proto;
   reg 		    vita_proto;
   
   

   integer  dump_file;

   initial
     begin	
	dump_file = $fopen(FILENAME,"w");
     end

    //
    // Monitor packets leaving FIFO
    //
    always @(posedge clk)
      if (reset | clear) begin
         out_state <= WAIT_SOF;
      end else
	case(out_state)
	  //
	  // After RESET or the EOF of previous packet, the first cycle with
	  // output valid asserted is the SOF and presents the Header word.
	  // The cycle following the concurrent presentation of asserted output 
	  // valid and output ready presents the word following the header.
	  //
          WAIT_SOF: 
            if (tvalid && tready) begin
	       last_line <= 0;
	       eth_proto <= 0;
	       ip_proto <= 0;
	       vita_proto <= 0;
	       
	       $fdisplay(dump_file,"------------------------------------------");
	       $fdisplay(dump_file,"Time = %1d",$time);
               if (START_AT_VRL) begin
		  if (tdata[63:32] == "VRLP") begin
		     $fdisplay(dump_file,"VRL VITA49 PAYLOAD");
		     vita_proto <= VRL;
		  end
		  out_state <= LINE7;
	       end
	       else out_state <= LINE1;
	       
	       last_line <= tdata;
            end else begin
               out_state <= WAIT_SOF;
            end
	  //
	  LINE1: if (tvalid && tready) begin
	     $fdisplay(dump_file,"Dst MAC: %x:%x:%x:%x:%x:%x",
		       last_line[15:8],last_line[7:0],tdata[63:56],tdata[55:48],tdata[47:40],tdata[39:32]);
	     last_line = tdata;
	     out_state <= LINE2;
	     if (tlast) begin
		$fdisplay(dump_file,"------------------------------------------");
		out_state <= WAIT_SOF;
	     end
	  end
	  //
	  LINE2: if (tvalid && tready) begin
	     $fdisplay(dump_file,"Src MAC: %x:%x:%x:%x:%x:%x",
		       last_line[31:24],last_line[23:16],last_line[15:8],last_line[7:0],
		       tdata[63:56],tdata[55:48]);
	     eth_proto = tdata[47:32];
	     // Protocol??
	     if (tdata[47:32] == ARP) begin
		//ARP
		$fdisplay(dump_file,"ARP packet");
		if (tdata[31:16] == 'h0001)
		  $fdisplay(dump_file,"HTYPE = 1 (Ethernet)");
		if (tdata[31:16] == 'h0800)
		  $fdisplay(dump_file,"PTYPE = 0x0800 (IPv4)");
		out_state <= LINE3;
	     end else if (tdata[47:32] == IPv4) begin
		// IPv4
		$fdisplay(dump_file,"IPv4 packet");
		$fdisplay(dump_file,"Packet Length: %1d",tdata[15:0]);
		out_state <= LINE3;
	     end else
	       out_state <= WAIT_EOF;
	     if (tlast) begin
	       	$fdisplay(dump_file,"------------------------------------------");
		out_state <= WAIT_SOF;
	     end
	  end // case: LINE2
	  //
	  LINE3: 
	    if (tvalid && tready) begin
	       // Protocol??
	       if (eth_proto == ARP) begin
		  $fdisplay(dump_file,"HLEN: %d  PLEN: %d",tdata[63:56],tdata[55:48]);
		  if (tdata[47:32] == 1) $fdisplay(dump_file,"Operation: ARP REQUEST");
		  else if (tdata[47:32] == 2) $fdisplay(dump_file,"Operation: ARP REPLY");
		  else $fdisplay(dump_file,"Operation: UNKNOWN");
		  last_line = tdata;
		  // out_state <= LINE4;
		  out_state <= WAIT_EOF; // Add further ARP decode later if desired.
		  
	       end else if (eth_proto == IPv4) begin
		  ip_proto = tdata[23:16];
		  if (tdata[23:16] == UDP)  $fdisplay(dump_file,"IPv4 Protocol: UDP");
		  else  $fdisplay(dump_file,"IPv4 Protocol: %x",tdata[23:16]);
		  out_state <= LINE4;
	       end else 
		 out_state <= WAIT_EOF;
	       if (tlast) begin
	       	  $fdisplay(dump_file,"------------------------------------------");
		  out_state <= WAIT_SOF;
	       end
	    end // if (tvalid && tready)
	  //
	  LINE4:
	    if (tvalid && tready) begin
	       // Protocol??
	       if (eth_proto == IPv4) begin
		  $fdisplay(dump_file,"IP Src Address: %1d.%1d.%1d.%1d  IP Dst Address: %1d.%1d.%1d.%1d",
			    tdata[63:56],tdata[55:48],tdata[47:40],tdata[39:32],
			    tdata[31:24],tdata[23:16],tdata[15:8],tdata[7:0]);
		  if (ip_proto == UDP) 
		    out_state <= LINE5;
		  else 
		    out_state <= WAIT_EOF;		  
	       end
	       if (tlast) begin
	       	  $fdisplay(dump_file,"------------------------------------------");
		  out_state <= WAIT_SOF;
	       end
	    end // if (tvalid && tready)
	  //
	  LINE5:
	    if (tvalid && tready) begin
	       // Protocol??
	       if (ip_proto == UDP) begin
		 $fdisplay(dump_file,"UDP Src Port: %d  UDP Dst Port: %d",
			   tdata[63:48],tdata[47:32]);
		  $fdisplay(dump_file,"UDP Length: %1d",tdata[31:16]);
		  
		  last_line = tdata;
		  out_state <= LINE6;
	       end
	       if (tlast) begin
	       	  $fdisplay(dump_file,"------------------------------------------");
		  out_state <= WAIT_SOF;
	       end
	    end // if (tvalid && tready)
	  //
	  LINE6:
	    if (tvalid && tready) begin
	       // Protocol??
	       if (tdata[63:32] == "VRLP") begin
		  $fdisplay(dump_file,"VRL VITA49 PAYLOAD");
		  // Expand VITA decode later
		  vita_proto <= VRL;
		  out_state <= LINE7;
	       end
	       if (tlast) begin
	       	  $fdisplay(dump_file,"------------------------------------------");
		  out_state <= WAIT_SOF;
	       end
	    end // if (tvalid && tready)
	  //
	  LINE7:
	    if (tvalid && tready) begin
	       // Protocol??
	       if (vita_proto == VRL) begin
		  $fdisplay(dump_file,"VRT: Packet Size: %1d  StreamID: %x",tdata[47:32],tdata[31:0]);
		  out_state <= WAIT_EOF;
	       end
	       if (tlast) begin
	       	  $fdisplay(dump_file,"------------------------------------------");
		  out_state <= WAIT_SOF;
	       end
	    end // if (tvalid && tready)
	  
	  
	  //
	  // EOF is signalled by o_tlast asserted whilst output valid and ready asserted.
	  //
          WAIT_EOF: 
            if (tlast && tvalid && tready) begin
               out_state <= WAIT_SOF;
            end else begin
               out_state <= WAIT_EOF;
            end
	endcase // case(in_state)

endmodule // axi_probe_tb

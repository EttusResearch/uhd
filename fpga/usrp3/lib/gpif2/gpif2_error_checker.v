
// Copyright 2013 Ettus Research LLC

// inspect the input for invalid conditions
// when bad - drain input, flag error, and insert error msg packet
//
// Packets alignment errors are searched for in two different ways:
// 1) Blatently illegal values in what is assumed to be the PACKET_LENGTH field
// in the CHDR header
// (We could probably improve this by looking at other fields of the header that 
// have a limited range of values)
// 2) Packet length indicating an EOF word that doesn't have TLAST set in the FIFO.
// (Upstream can howvever legally insert TLAST in the FIFO for words that are not EOF)
// 
// Packet allignment recovery strategy is to wait for TLAST asserted and then decode the 
// following data assuming it is the start of new CHDR headers.
//
//TODO - insert bad packet

module gpif2_error_checker
  #(parameter SIZE = 9)
  (input clk, input reset, input clear,
   input [31:0] i_tdata, input i_tlast, input i_tvalid, output i_tready,
   output [31:0] o_tdata, output o_tlast, output o_tvalid, input o_tready,
   output bus_error, output [63:0] debug);

    wire [31:0] gate_tdata;
    wire gate_tlast, gate_terror;
    wire gate_tvalid, gate_tready;

    localparam STATE_HDR = 0;
    localparam STATE_FWD = 1;
    localparam STATE_EOF = 2;
    localparam STATE_WAIT = 3;
    reg [1:0] state;

    reg [15:0] lines32;
    reg [11:0] seq_id_ref;
    reg        seq_id_bad;
    reg        seq_id_wayoff;
   

    wire [15:0] hdr_bytes = i_tdata[15:0] + 3; //round up to multiple of 4
    wire [15:0] hdr_lines32 = {2'b0, hdr_bytes[15:2]}; //convert to lines32 count
    wire [11:0] seq_id_actual = i_tdata[27:16];
   
   
    wire obviously_bad_hdr = (hdr_lines32 == 16'h0) || (hdr_lines32 > (1 << SIZE));

    always @(posedge clk) begin
        if (reset | clear) begin
            state <= STATE_HDR;
            lines32 <= 16'b0;
	    seq_id_ref <= 12'h0;
	    seq_id_bad <= 0;
	    seq_id_wayoff <= 0;
	   
	   
        end
        else case (state)

        STATE_HDR: begin //forward header and grab vita length
            if (i_tvalid && i_tready) begin
                if (obviously_bad_hdr)           state <= STATE_WAIT;
                else if (hdr_lines32 == 16'h1)  state <= STATE_HDR;
                else if (hdr_lines32 == 16'h2)  state <= STATE_EOF;
                else                            state <= STATE_FWD;
	        seq_id_bad <= (seq_id_actual != seq_id_ref);
	        seq_id_wayoff <= (seq_id_actual != seq_id_ref) |
				(seq_id_actual != seq_id_ref+1) |
				(seq_id_actual != seq_id_ref+2) |
				(seq_id_actual != seq_id_ref+3);
	        if (seq_id_actual != seq_id_ref)
		 seq_id_ref <= seq_id_actual + 1;
	        else
		 seq_id_ref <= seq_id_ref + 1;
            end
            lines32 <= hdr_lines32;
	    
        end

        STATE_FWD: begin //forward the rest of vita packet
            if (i_tvalid && i_tready) begin
                if (lines32 == 16'h3) state <= STATE_EOF;
                lines32 <= lines32 - 1'b1;
            end
        end

        STATE_EOF: begin //do last line of vita frame + eof
	   if (i_tvalid && i_tready) 
	     if (gate_tlast) state <= STATE_HDR;
	     else state <= STATE_WAIT; // Try somehow to get synchronized again.
        end

        STATE_WAIT: begin //drop until idle
            if (i_tvalid && i_tready && i_tlast) state <= STATE_HDR;
        end

        endcase //state
    end

    assign bus_error = (gate_terror && gate_tvalid && gate_tready) || ((state == STATE_HDR) && i_tvalid && i_tready && obviously_bad_hdr);
    assign gate_tlast = (state == STATE_HDR)? (hdr_lines32 == 16'h1) : (state == STATE_EOF);
    assign gate_tdata = i_tdata;
    assign gate_tvalid = i_tvalid && ((state == STATE_HDR)? !obviously_bad_hdr : (state != STATE_WAIT));
    assign i_tready = gate_tready;

    axi_packet_gate #(.WIDTH(32), .SIZE(SIZE)) gate_xfer
    (
        .clk(clk), .reset(reset), .clear(clear),
        .i_tdata(gate_tdata), .i_tlast(gate_tlast), .i_terror(1'b0), .i_tvalid(gate_tvalid), .i_tready(gate_tready),
        .o_tdata(o_tdata), .o_tlast(o_tlast), .o_tvalid(o_tvalid), .o_tready(o_tready)
    );
   assign  debug = {13'b0,
		    seq_id_wayoff, //[50]      [114]
		    gate_terror,  // [49]      [113]
		    obviously_bad_hdr, // [48] [112]
		    seq_id_bad,   // [47]      [111]
		    seq_id_ref,   // [46:35]   [110:99]
		    i_tlast,      // [34]      [98]
		    i_tready,     // [33]      [97]
		    i_tvalid,     // [32]      [96]
		    i_tdata};     // [31:0]    [95:64]
   

endmodule // cvita_insert_tlast

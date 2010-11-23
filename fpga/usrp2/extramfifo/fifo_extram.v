
// Everything on sram_clk

module fifo_extram
  (input reset, input clear,
   input [17:0] datain, input src_rdy_i, output dst_rdy_o, output [15:0] space, input [15:0] occ_in,
   output [17:0] dataout, output src_rdy_o, input dst_rdy_i, output [15:0] occupied, input [15:0] space_in,
   input sram_clk, output [18:0] sram_a, inout [17:0] sram_d, output sram_we,
   output [1:0] sram_bw, output sram_adv, output sram_ce, output sram_oe, 
   output sram_mode, output sram_zz);

   localparam AWIDTH   = 19; // 1 MB in x18
   localparam RAMSIZE  = ((1<<AWIDTH) - 1);

   wire   do_store, do_retrieve;
   reg [1:0]  do_store_del, do_retr_del;
   
   reg [AWIDTH-1:0] addr_retrieve, addr_store;
   always @(posedge sram_clk)
     if(reset | clear)
       addr_retrieve <= 0;
     else if (do_retrieve)
       addr_retrieve <= addr_retrieve + 1;
   
   always @(posedge sram_clk)
     if(reset | clear)
       addr_store    <= 0;
     else if(do_store)
       addr_store    <= addr_store + 1;

   //wire [AWIDTH-1:0] fullness = (addr_store - addr_retrieve);
   reg [AWIDTH-1:0] fullness;
   always @(posedge sram_clk)
     if(reset | clear)
       fullness      <= 0;
     else if(do_store)
       fullness      <= fullness + 1;
     else if(do_retrieve)
       fullness      <= fullness - 1;
   
   //   wire        empty  = (fullness == 0);
   //wire        full   = (fullness == RAMSIZE); // 19'h7FF);
   reg 		    empty, full;
   
   //  The math in the following functions is 'AWIDTH wide.  Use
   //  continuous assignments to prevent the numbers from being
   //  promoted to 32-bit (which would make it wrap wrong).
   //
   wire [AWIDTH-1:0] addr_retrieve_p1, addr_store_p2;
   assign addr_retrieve_p1 = addr_retrieve + 1;
   assign addr_store_p2 = addr_store + 2;

   always @(posedge sram_clk)
     if(reset | clear)
       empty 	     <= 1;
     else if(do_store)
       empty 	     <= 0;
     else if(do_retrieve & (/*(addr_retrieve + 1)*/ addr_retrieve_p1 == addr_store))
       empty 	     <= 1;
   
   always @(posedge sram_clk)
     if(reset | clear)
       full 	     <= 0;
     else if(do_retrieve)
       full 	     <= 0;
     else if(do_store & (/*(addr_store+2)*/ addr_store_p2 == addr_retrieve))
       full <= 1;

   reg 	       can_store;
   always @*
     if(full | ~src_rdy_i)
       can_store 	  <= 0;
     else if(do_store_del == 0)
       can_store 	  <= 1;
     else if((do_store_del == 1) || (do_store_del == 2))
       can_store 	  <= (occ_in > 1);
     else
       can_store 	  <= (occ_in > 2);
     
   reg 	       can_retrieve;
   always @*
     if(empty | ~dst_rdy_i)
       can_retrieve 	     <= 0;
     else if(do_retr_del == 0)
       can_retrieve 	     <= 1;
     else if((do_retr_del == 1) || (do_retr_del == 2))
       can_retrieve 	     <= (space_in > 1);
     else
       can_retrieve 	     <= (space_in > 2);
   
   reg [1:0]  state;
   localparam IDLE_STORE_NEXT  = 0;
   localparam STORE 	       = 1;
   localparam IDLE_RETR_NEXT   = 2;
   localparam RETRIEVE 	       = 3;
   
   reg [7:0]  countdown;
   wire       countdown_done 		   = (countdown == 0);

   localparam CYCLE_SIZE 		   = 6;
   
   assign        do_store 		   = can_store & (state == STORE);
   assign        do_retrieve 		   = can_retrieve & (state == RETRIEVE);
   always @(posedge sram_clk)
     if(reset)
       do_store_del 			  <= 0;
     else
       do_store_del 			  <= {do_store_del[0],do_store};
   
   always @(posedge sram_clk) 
     if(reset)
       do_retr_del <= 0;
     else
       do_retr_del <= {do_retr_del[0],do_retrieve};

   always @(posedge sram_clk)
     if(reset | clear)
       begin
	  state 			  <= IDLE_STORE_NEXT;
	  countdown 			  <= 0;
       end
     else
       case(state)
	 IDLE_STORE_NEXT :
	   if(can_store)
	     begin
		state 	  <= STORE;
		countdown <= CYCLE_SIZE;
	     end
	   else if(can_retrieve)
	     begin
		state 	  <= RETRIEVE;
		countdown <= CYCLE_SIZE;
	     end
	 STORE :
	   if(~can_store | (can_retrieve & countdown_done))
	     state <= IDLE_RETR_NEXT;
	   else if(~countdown_done)
	     countdown <= countdown - 1;
	 IDLE_RETR_NEXT :
	   if(can_retrieve)
	     begin
		state 	  <= RETRIEVE;
		countdown <= CYCLE_SIZE;
	     end
	   else if(can_store)
	     begin
		state 	  <= STORE;
		countdown <= CYCLE_SIZE;
	     end
	 RETRIEVE :
	   if(~can_retrieve | (can_store & countdown_done))
	     state <= IDLE_STORE_NEXT;
	   else if(~countdown_done)
	     countdown <= countdown - 1;
       endcase // case (state)

   // RAM wires
   assign sram_bw      = 0;
   assign sram_adv     = 0;
   assign sram_mode    = 0;
   assign sram_zz      = 0;
   assign sram_ce      = 0;

   assign sram_a       = (state==STORE) ? addr_store : addr_retrieve;
   assign sram_we      = ~do_store;
   assign sram_oe      = ~do_retr_del[1];
   assign my_oe        = do_store_del[1] & sram_oe;
   assign sram_d       = my_oe ? datain : 18'bz;
   
   // FIFO wires
   assign dataout      = sram_d;
   assign src_rdy_o    = do_retr_del[1];
   assign dst_rdy_o    = do_store_del[1];
   
endmodule // fifo_extram


   //wire        have_1 		   = (fullness == 1);
   //wire        have_2 		   = (fullness == 2);
   //wire        have_atleast_1 	   = ~empty;
   //wire        have_atleast_2 	   = ~(empty | have_1);
   //wire        have_atleast_3 	   = ~(empty | have_1 | have_2);   
   //wire        full_minus_1 	   = (fullness == (RAMSIZE-1)); // 19'h7FE);
   //wire        full_minus_2 	   = (fullness == (RAMSIZE-2)); // 19'h7FD);
   //wire        spacefor_atleast_1  = ~full;
   //wire        spacefor_atleast_2  = ~(full | full_minus_1);
   //wire        spacefor_atleast_3  = ~(full | full_minus_1 | full_minus_2);

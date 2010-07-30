module nobl_fifo
  #(parameter WIDTH=18,DEPTH=19)
    (   
	input clk,
	input rst,
	input [WIDTH-1:0] RAM_D_pi,
	output [WIDTH-1:0] RAM_D_po,
	output RAM_D_poe,
	output [DEPTH-1:0] RAM_A,
	output RAM_WEn,
	output RAM_CENn,
	output RAM_LDn,
	output RAM_OEn,
	output RAM_CE1n,
	input [WIDTH-1:0] write_data,
	input write_strobe,
	output reg space_avail,
	output reg [WIDTH-1:0] read_data,
	input read_strobe,
	output reg data_avail
	);

   reg [DEPTH-1:0] capacity;
   reg [DEPTH-1:0] wr_pointer;
   reg [DEPTH-1:0] rd_pointer;
   wire [DEPTH-1:0] address;
   
   reg 		   supress;
   reg 		   data_avail_int;   // Data available with high latency from ext FIFO flag
   wire [WIDTH-1:0] data_in;
   wire 	    data_in_valid;
   reg [WIDTH-1:0]  read_data_pending;
   reg 		    pending_avail;
   wire 	    read_strobe_int;
   
   

   assign 	   read = read_strobe_int && data_avail_int;
   assign 	   write = write_strobe && space_avail;

   // When a read and write collision occur, supress availability flags next cycle
   // and complete write followed by read over 2 cycles. This forces balanced arbitration
   // and makes for a simple logic design.

   always @(posedge clk)
     if (rst)
       begin
	  capacity <= 1 << (DEPTH-1);
	  wr_pointer <= 0;
	  rd_pointer <= 0;
	  space_avail <= 0;
	  data_avail_int <= 0;
	  supress <= 0;
       end
     else	  
       begin
	  space_avail <= ~((capacity == 0) || (read&&write) || (capacity == 1 && write) );
	  // Capacity has 1 cycle delay so look ahead here for corner case of read of last item in FIFO.
	  data_avail_int <= ~((capacity == (1 << (DEPTH-1))) || (read&&write)  || (capacity == ((1 << (DEPTH-1))-1) && read)  );
	  supress <= read && write;
	  wr_pointer <= wr_pointer + write;
	  rd_pointer <= rd_pointer + ((~write && read) || supress);
	  capacity <= capacity - write + ((~write && read) || supress); // REVISIT
       end // else: !if(rst)

   assign address = write ? wr_pointer : rd_pointer;
   assign enable = write || read || supress; 

   //
   // Need to have first item in external FIFO moved into local registers for single cycle latency and throughput on read.
   // 2 local registers are provided so that a read every other clock cycle can be sustained.
   // No fowarding logic is provided to bypass the external FIFO as latency is of no concern.
   //
   always @(posedge clk)
     if (rst)
       begin
	  read_data <= 0;
	  data_avail <= 0;
	  read_data_pending <= 0;
	  pending_avail <= 0;	  
       end
     else
       begin
	  case({read_strobe,data_in_valid})
	    // No read externally, no new data arriving from external FIFO
	    2'b00: begin
	       case({data_avail,pending_avail})
		 // Start Data empty, Pending empty.
		 //
		 // End Data full, Pending empty
		 2'b00: begin
		    read_data <= read_data;		  
		    data_avail <= data_avail;		    
		    read_data_pending <= read_data_pending ;		    
		    pending_avail <= pending_avail;
		 end
		 // Start Data empty, Pending full.
		 // Data <= Pending, 
		 // End Data full, Penidng empty.
		 2'b01: begin
		    read_data <= read_data_pending;		  
		    data_avail <= 1'b1;		    
		    read_data_pending <= read_data_pending ;		    
		    pending_avail <= 1'b0;
		 end
		 // Start Data full, Pending empty.
		 //
		 // End Data full, Pending empty
		 2'b10: begin
     		    read_data <= read_data;		  
		    data_avail <= data_avail;		    
		    read_data_pending <= read_data_pending ;		    
		    pending_avail <= pending_avail;
		 end
		 // Start Data full, Pending full.
		 //
		 // End Data full, Pending full.
		 2'b11: begin
		    read_data <= read_data;		  
		    data_avail <= data_avail;		    
		    read_data_pending <= read_data_pending ;		    
		    pending_avail <= pending_avail;
		 end
	       endcase
	    end
	    // No read externally, new data arriving from external FIFO
	    2'b01: begin
	       case({data_avail,pending_avail})
		 // Start Data empty, Pending empty.
		 // Data <= FIFO
		 // End Data full, Pending empty
		 2'b00: begin
		    read_data <= data_in;		  
		    data_avail <= 1'b1;		    
		    read_data_pending <= read_data_pending ;		    
		    pending_avail <= 1'b0;
		 end
		 // Start Data empty, Pending full.
		 // Data <= Pending, Pending <= FIFO
		 // End Data full, Penidng full.
		 2'b01: begin
		    read_data <= read_data_pending;		  
		    data_avail <= 1'b1;		    
		    read_data_pending <= data_in ;		    
		    pending_avail <= 1'b1;
		 end
		 // Start Data full, Pending empty.
		 // Pending <= FIFO
		 // End Data full, Pending full
		 2'b10: begin
     		    read_data <= read_data;		  
		    data_avail <= 1'b1;		    
		    read_data_pending <= data_in ;		    
		    pending_avail <= 1'b1;
		 end
		 // Data full, Pending full.
		 // *ILLEGAL STATE*
		 2'b11: begin
		    
		 end
	       endcase
	    end
	    // Read externally, no new data arriving from external FIFO
	    2'b10: begin
	       case({data_avail,pending_avail})
		 // Start Data empty, Pending empty.
		 // *ILLEGAL STATE*
		 2'b00: begin
		    
		 end
		 // Start Data empty, Pending full.
		 // *ILLEGAL STATE*
		 2'b01: begin
		    
		 end
		 // Start Data full, Pending empty.
		 // Out <= Data
		 // End Data empty, Pending empty.
		 2'b10: begin
     		    read_data <= read_data;		  
		    data_avail <= 1'b0;		    
		    read_data_pending <= read_data_pending ;		    
		    pending_avail <= 1'b0;
		 end
		 // Start Data full, Pending full.
		 // Out <= Data, 
		 // End Data full, Pending empty
		 2'b11: begin
		    read_data <= read_data_pending;		  
		    data_avail <= 1'b1;		    
		    read_data_pending <= read_data_pending ;		    
		    pending_avail <= 1'b0;
		 end
	       endcase
	    end
	    // Read externally, new data arriving from external FIFO
	    2'b11: begin
	       case({data_avail,pending_avail})
		 // Start Data empty, Pending empty.
		 // *ILLEGAL STATE*
		 2'b00: begin
		    
		 end
		 // Start Data empty, Pending full.
		 // *ILLEGAL STATE*
		 2'b01: begin
		    
		 end
		 // Start Data full, Pending empty.
		 // Out <= Data, Data <= FIFO
		 // End Data full, Pending empty.
		 2'b10: begin
     		    read_data <= data_in;		  
		    data_avail <= 1'b1;		    
		    read_data_pending <= read_data_pending ;		    
		    pending_avail <= 1'b0;
		 end
		 // Start Data full, Pending full.
		 // Out <= Data, Data <= Pending, Pending <= FIFO
		 // End Data full, Pending full
		 2'b11: begin
		    read_data <= read_data_pending;		  
		    data_avail <= 1'b1;		    
		    read_data_pending <= data_in ;		    
		    pending_avail <= 1'b1;
		 end
	       endcase
	    end
	  endcase
       end

   // Start an external FIFO read as soon as a read of the buffer reg is strobed to minimise refill latency.
   // If the buffer reg or the pending buffer reg is already empty also pre-emptively start a read. 
   // However there must be something in ext FIFO to read.
   // This means that there can be 2 outstanding reads to the ext FIFO active at any time helping to hide latency.
   assign read_strobe_int = (read_strobe & data_avail & ~pending_avail) || (~data_avail && ~pending_avail);
   

   //
   // Simple NoBL SRAM interface, 4 cycle read latency.
   // Read/Write arbitration via temprary application of empty/full flags.
   //
   nobl_if nobl_if_i1
     (
      .clk(clk),
      .rst(rst),
      .RAM_D_pi(RAM_D_pi),
      .RAM_D_po(RAM_D_po),
      .RAM_D_poe(RAM_D_poe),
      .RAM_A(RAM_A),
      .RAM_WEn(RAM_WEn),
      .RAM_CENn(RAM_CENn),
      .RAM_LDn(RAM_LDn),
      .RAM_OEn(RAM_OEn),
      .RAM_CE1n(RAM_CE1n),
      .address(address),
      .data_out(write_data),
      .data_in(data_in),
      .data_in_valid(data_in_valid),
      .write(write),
      .enable(enable)
      );

endmodule // nobl_fifo

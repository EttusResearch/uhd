
// FIFO Interface to the 2K buffer RAMs
// Read port is read-acknowledge
// FIXME do we want to be able to interleave reads and writes?

module buffer_int2
  #(parameter BASE = 0,
    parameter BUF_SIZE = 9)
    (// Control Interface
     input clk,
     input rst,

     input set_stb, input [7:0] set_addr, input [31:0] set_data,
     output [31:0] status,
     output sys_int_o,   // unused

     // Wishbone interface to RAM
     input wb_clk_i,
     input wb_rst_i,
     input wb_we_i,
     input wb_stb_i,
     input [15:0] wb_adr_i,
     input [31:0] wb_dat_i,   
     output [31:0] wb_dat_o,
     output reg wb_ack_o,

     // Write FIFO Interface
     input [35:0] wr_data_i,
     input wr_ready_i,
     output wr_ready_o,
     
     // Read FIFO Interface
     output [35:0] rd_data_o,
     output rd_ready_o,
     input rd_ready_i
     );

   reg [BUF_SIZE-1:0] addr;
   wire [31:0] 	      ctrl;
   wire 	      we, en, done, error, idle, go;
   
   wire [BUF_SIZE-1:0] firstline = 0;
   wire [BUF_SIZE-1:0] lastline = ctrl[15+BUF_SIZE:16];
   
   wire 	       read = ctrl[2];
   wire 	       write = ctrl[1];
   wire 	       clear = ctrl[0];
   
   reg [2:0] 	       state;
   reg 		       rd_sop, rd_eop;
   wire 	       wr_sop, wr_eop, wr_error;
   reg [1:0] 	       rd_occ;
   wire [1:0] 	       wr_occ;
   
   localparam IDLE = 3'd0;
   localparam PRE_READ = 3'd1;
   localparam READING = 3'd2;
   localparam WRITING = 3'd3;
   localparam ERROR = 3'd4;
   localparam DONE = 3'd5;
   
   always @(posedge clk)
     if(rst)
       begin
	  state <= IDLE;
	  rd_sop <= 0;
	  rd_eop <= 0;
	  rd_occ <= 0;
       end
     else
       if(clear)
	 begin
	    state <= IDLE;
	    rd_sop <= 0;
	    rd_eop <= 0;
	    rd_occ <= 0;
	 end
       else 
	 case(state)
	   IDLE :
	     if(go & read)
	       begin
		  addr <= firstline;
		  state <= PRE_READ;
	       end
	     else if(go & write)
	       begin
		  addr <= firstline;
		  state <= WRITING;
	       end
	   
	   PRE_READ :
	     begin
		state <= READING;
		addr <= addr + 1;
		rd_occ <= 2'b00;
		rd_sop <= 1;
		rd_eop <= 0;
	     end
	   
	   READING :
	     if(rd_ready_i)
	       begin
		  rd_sop <= 0;
		  addr <= addr + 1;
		  if(addr == lastline)
		    begin
		       rd_eop <= 1;
		       // FIXME assign occ here
		       rd_occ <= 0;
		    end
		  else
		    rd_eop <= 0;
		  if(rd_eop)
		    state <= DONE;
	       end
	   
	   WRITING :
	     begin
		if(wr_ready_i)
		  begin
		     addr <= addr + 1;
		     if(wr_error)
		       begin
			  state <= ERROR;
			  // Save OCC flags here
		       end
		     else if((addr == lastline)||wr_eop)
		       state <= DONE;
		  end // if (wr_ready_i)
	     end // case: WRITING
	   
	 endcase // case(state)
   
   assign     rd_data_o[35:32] = { rd_occ[1:0], rd_eop, rd_sop };
   assign     rd_ready_o = (state == READING);
   
   assign     wr_sop = wr_data_i[32];
   assign     wr_eop = wr_data_i[33];
   assign     wr_occ = wr_data_i[35:34];
   assign     wr_error = wr_sop & wr_eop;
   assign     wr_ready_o = (state == WRITING);

   assign     we = (state == WRITING); // always write to avoid timing issue
   assign     en = ~((state==READING)& ~rd_ready_i);   // FIXME potential critical path
   
   assign     done = (state == DONE);
   assign     error = (state == ERROR);
   assign     idle = (state == IDLE);

   ram_2port #(.DWIDTH(32),.AWIDTH(BUF_SIZE)) buffer
     (.clka(wb_clk_i),.ena(wb_stb_i),.wea(wb_we_i),
      .addra(wb_adr_i[BUF_SIZE+1:2]),.dia(wb_dat_i),.doa(wb_dat_o),
      .clkb(clk),.enb(en),.web(we),
      .addrb(addr),.dib(wr_data_i[31:0]),.dob(rd_data_o[31:0]));
   
   always @(posedge wb_clk_i)
     if(wb_rst_i)
       wb_ack_o <= 0;
     else
       wb_ack_o <= wb_stb_i & ~wb_ack_o;

   setting_reg #(.my_addr(BASE)) 
   sreg(.clk(clk),.rst(rst),.strobe(set_stb),.addr(set_addr),.in(set_data),
	.out(ctrl),.changed(go));
   
   assign status = { {(16-BUF_SIZE){1'b0}},addr,8'b0,5'b0,idle,error,done};

endmodule // buffer_int2

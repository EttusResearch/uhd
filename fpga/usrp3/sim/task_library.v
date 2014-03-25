///////////////////////////////////////////////////////////////////
//
// USRP3 Task Libaray
//
///////////////////////////////////////////////////////////////////

`define SID(a,b,c,d) ((a & 'hff) << 24) | ((b & 'hff) << 16)| ((c & 'hff) << 8) | (d & 'hff)

`ifndef TB_FILE_IN_NUMBER
 `define TB_FILE_IN_NUMBER 0
`endif
`ifndef TB_FILE_OUT_NUMBER
 `define TB_FILE_OUT_NUMBER 0
`endif
`ifndef TB_FILE_GOLDEN_NUMBER
 `define TB_FILE_GOLDEN_NUMBER 0
`endif
`ifndef TB_FILE_IN_NAME
 `define TB_FILE_IN_NAME "tb_file_in"
`endif
`ifndef TB_FILE_OUT_NAME
 `define TB_FILE_OUT_NAME "tb_file_out"
`endif
`ifndef TB_FILE_GOLDEN_NAME
 `define TB_FILE_GOLDEN_NAME "tb_file_golden"
`endif

integer  tb_file_in_desc[`TB_FILE_IN_NUMBER-1:0];

/* -----\/----- EXCLUDED -----\/-----
reg clk;
reg reset;
reg clear;
 -----/\----- EXCLUDED -----/\----- */


/* -----\/----- EXCLUDED -----\/-----
reg set_stb;
reg [7:0] set_addr;
reg [31:0] set_data;
 -----/\----- EXCLUDED -----/\----- */
/* -----\/----- EXCLUDED -----\/-----

`ifndef CHDR_IN_NUMBER
 `define CHDR_IN_NUMBER 1
`endif


reg [63:0] data_in[`CHDR_IN_NUMBER-1:0];
reg last_in[`CHDR_IN_NUMBER-1:0];
reg valid_in[`CHDR_IN_NUMBER-1:0];
wire ready_in[`CHDR_IN_NUMBER-1:0];
 -----/\----- EXCLUDED -----/\----- */


///////////////////////////////////////////////////////////////////
//
// open_files
//
// The "open_files" task opens all standard stimulus, response, and
// golden response files to drive other tasks in the library.
// Both the numbers of files and there names can be altered by pre-defining
// the TB_FILE* pre-processor definitions. All files are assumed to be
// readmemh style format and will use a ".hex" file extension.
//
///////////////////////////////////////////////////////////////////
task open_files;
   reg [7:0] x;
   reg [7:0] y;
   reg [8*32:0] filename;

   begin
      x = 0;
      // Caveman verilog string handling to dynamicly form file name
      while (x != `TB_FILE_IN_NUMBER) begin
	 y = x;
	 filename =  {`TB_FILE_IN_NAME,("0" + (y % 10))};
	 while (y/10 >0) begin
	    filename = {(filename << 8),("0" + (y % 10))};
	    y = y/10;
	 end
	 // Always use .hex as file extention for readmemh style data
	 filename = {filename,".hex"};
	 tb_file_in_desc[x] = $fopen(filename);
	 x = x + 1;
      end
   end
endtask

///////////////////////////////////////////////////////////////////
//
// close_files
//
// The "close_files" task closes all files opened by a previous
// call to "open_files".
//
///////////////////////////////////////////////////////////////////
task close_files;
   reg [7:0] x;

   begin
      x = 0;
      while (x != `TB_FILE_IN_NUMBER) begin
	 $fclose(tb_file_in_desc[x]);
	 x = x + 1;
      end
   end
endtask // close_files

///////////////////////////////////////////////////////////////////
//
// write_settings_bus
//
// The "write settings_bus" task performs a single settings bus
// transaction in 3 clock cycles, the first and 3rd cycles being
// idle cycles. "write_settings_bus" is not re-entrant and should
// only be used sequentially in a single test bench thread.
//
///////////////////////////////////////////////////////////////////
  task write_setting_bus;
      input [15:0] address;
      input [31:0] data;

     begin

	@(posedge clk);
	set_stb <= 1'b0;
	set_addr <= 16'h0;
	set_data <= 32'h0;
	@(posedge clk);
	set_stb <= 1'b1;
	set_addr <= address;
	set_data <= data;
	@(posedge clk);
	set_stb <= 1'b0;
	set_addr <= 16'h0;
	set_data <= 32'h0;

      end
   endtask // write_setting_bus

///////////////////////////////////////////////////////////////////
//
// Place 64bits of data on CHDR databus, set last if indicated.
// Wait unitl ready asserted before returning with valid de-assrted.
//
///////////////////////////////////////////////////////////////////

   task automatic enqueue_line;
      input [7:0] input_port;
      input last;
      input [63:0] data;
      begin
         data_in[input_port] <= data;
	 last_in[input_port] <= last;
         valid_in[input_port] <= 1;
	 @(posedge clk);
         while (~ready_in[input_port]) begin
            @(posedge clk);
         end
         data_in[input_port] <= 0;
	 last_in[input_port] <= 0;
         valid_in[input_port] <= 0;
      end
   endtask // enqueue_line

///////////////////////////////////////////////////////////////////
//
// Place 64bits of data on CHDR databus, set last if indicated.
// Wait unitl ready asserted before returning with valid de-assrted.
//
///////////////////////////////////////////////////////////////////

   task automatic dequeue_line;
      input [7:0] output_port;
      output last;
      output [63:0] data;
      begin
	 while (~valid_out[output_port]) begin
	    ready_out[output_port] <= 1;
            @(posedge clk);
         end

	 // NOTE: A subtle constraint of Verilog is that non-blocking assignments can not be used
	 // to an automatically allocated variable. Fall back to blocking assignent
         data = data_out[output_port];
	 last = last_out[output_port];
         ready_out[output_port] <= 1;
	 @(posedge clk);
	 ready_out[output_port] <= 0;

      end
   endtask // enqueue_line


///////////////////////////////////////////////////////////////////
//
// CHDR Header format:
// Word1: FLAGS [63:60], SEQ_ID [59:48], SIZE [47:32], SID [31:0]
//        [63]ExtensionContext, [62]HasTrailer, [61]HasTime, [60]EOB
// Word2: Time [63:0] (Optional, see header bit [61])
// Word3+: Payload
//
///////////////////////////////////////////////////////////////////
   task automatic enqueue_chdr_pkt_count;
      input [7:0] input_port;       // Which test bench CHDR ingress port
      input [11:0] seq_id;          // CHDR sequence ID.
      input [15:0] chdr_size;       // Total CHDR packet size in bytes.
      input 	   has_time;        // This CHDR packet has 64bit time field.
      input [63:0] chdr_time;       // 64bit CHDR time value
      input 	   is_extension;    // This packet is flaged extension context
      input 	   is_eob;          // This packet is the end of a CHDR burst.
      input [31:0] chdr_sid;        // CHDR SID address pair.

      integer i;
      reg [15:0] j;

      begin
	 @(posedge clk);
	 enqueue_line(input_port, 0, {is_extension,1'b0,has_time,is_eob,seq_id,chdr_size,chdr_sid});
	 if (has_time)  // If time flag set add 64bit time as second word.
	   enqueue_line(input_port, 0, chdr_time);
	 j = 0;

	 for (i = (has_time ? 24 : 16); i < chdr_size; i = i + 8) begin
	    enqueue_line(input_port, 0 , {j,j+16'h1,j+16'h2,j+16'h3});
	    j = j + 4;
         end
	 // Populate last line with data even if sizes shows it's not used.
	 enqueue_line(input_port, 1, {j,j+16'h1,j+16'h2,j+16'h3});

      end
   endtask // automatic


task automatic dequeue_chdr_pkt_count;
   input [7:0] output_port;       // Which test bench CHDR ingress port
   input [11:0] seq_id;          // CHDR sequence ID.
   input [15:0] chdr_size;       // Total CHDR packet size in bytes.
   input 	has_time;        // This CHDR packet has 64bit time field.
   input [63:0] chdr_time;       // 64bit CHDR time value
   input 	is_extension;    // This packet is flaged extension context
   input 	is_eob;          // This packet is the end of a CHDR burst.
   input [31:0] chdr_sid;        // CHDR SID address pair.

   integer 	i;
   reg [15:0] 	j;

   reg 		last;
   reg [63:0] 	data;

   begin
      @(posedge clk);
      dequeue_line(output_port, last, data);
      if ({is_extension,1'b0,has_time,is_eob,seq_id,chdr_size,chdr_sid} !== data)
	$display("FAILED: Output port: %3d  Bad CHDR Header. Got %8x, expected %8x @ time: %d ",
		 output_port, data, {is_extension,1'b0,has_time,is_eob,seq_id,chdr_size,chdr_sid},$time);
//      else
//	$display("PASSED: Output port: %3d  Bad CHDR Header. Got %8x, expected %8x @ time: %d ",
//		 output_port, data, {is_extension,1'b0,has_time,is_eob,seq_id,chdr_size,chdr_sid},$time);
      if (has_time)  // If time flag set add 64bit time as second word.
	begin
	   dequeue_line(output_port, last, data);
	   if (data !== chdr_time)
	     $display("FAILED: Output port: %3d  Bad CHDR Time. Got %8x, expected %8x @ time: %d ",
		      output_port, data, chdr_time, $time);
//	   else
//	     $display("PASSED: Output port: %3d  Bad CHDR Time. Got %8x, expected %8x @ time: %d ",
//		      output_port, data, chdr_time, $time);
	end
      j = 0;

      for (i = (has_time ? 24 : 16); i < chdr_size; i = i + 8) begin
	 dequeue_line(output_port, last , data);
	 if ({j,j+16'h1,j+16'h2,j+16'h3} !== data)
	   $display("FAILED: Output port: %3d  Bad CHDR Payload. Got %8x, expected %8x @ time: %d ",
		    output_port, data,{j,j+16'h1,j+16'h2,j+16'h3} ,$time);
//	 else
//	   $display("PASSED: Output port: %3d  Bad CHDR Payload. Got %8x, expected %8x @ time: %d ",
//		    output_port, data, {is_extension,1'b0,has_time,is_eob,seq_id,chdr_size,chdr_sid},$time);
	 j = j + 4;
      end
      // Check only bytes included in packet
      dequeue_line(output_port, last, data);
      if (({j,j+16'h1,j+16'h2,j+16'h3} >> (8-j)) !== ( data>> (8-j)))
	$display("FAILED: Output port: %3d  Bad CHDR Payload. Got %8x, expected %8x @ time: %d ",
		 output_port, data >> (8-j),
		 {j,j+16'h1,j+16'h2,j+16'h3} >> (8-j),$time);
//      else
//	$display("PASSED: Output port: %3d  Bad CHDR Payload. Got %8x, expected %8x @ time: %d ",
//		 output_port, data >> (8-j),
//		 {is_extension,1'b0,has_time,is_eob,seq_id,chdr_size,chdr_sid} >> (8-j),$time);

   end

endtask // dequeue_packet


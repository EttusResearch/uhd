

module gpmc_model
  (output EM_CLK, inout [15:0] EM_D, output reg [10:1] EM_A, output reg [1:0] EM_NBE,
   output reg EM_WAIT0, output reg EM_NCS4, output reg EM_NCS6, 
   output reg EM_NWE, output reg EM_NOE );

   assign EM_CLK = 0;
   reg [15:0] EM_D_int;
   assign EM_D = EM_D_int;
   
   initial
     begin
	EM_A <= 10'bz;
	EM_NBE <= 2'b11;
	EM_NWE <= 1;
	EM_NOE <= 1;
	EM_NCS4 <= 1;
	EM_NCS6 <= 1;
	EM_D_int <= 16'bz;
 	EM_WAIT0 <= 0;  // FIXME this is actually an input
     end
   
   task GPMC_Write;
      input [10:0] addr;
      input [15:0] data;
      begin
	 #2;
	 EM_A <= addr[10:1];
	 EM_D_int <= data;
	 #4;
	 EM_NCS6 <= 0;
	 #5;
	 EM_NWE <= 0;
	 #41;
	 EM_NWE <= 1;
	 EM_NCS6 <= 1;
	 EM_A <= 10'bz;
	 EM_D_int <= 16'bz;
      end
   endtask // GPMC_Write

   task GPMC_Read;
      input [10:0] addr;
      begin
	 #2;
	 EM_A <= addr[10:1];
	 #4;
	 EM_NCS6 <= 0;
	 #5;
	 EM_NOE <= 0;
	 #41;
	 EM_NOE <= 1;
	 EM_NCS6 <= 1;
	 EM_A <= 10'bz;
	 $display("Data Read from GPMC: %X",EM_D);
      end
   endtask // GPMC_Read
   
   initial
     begin
	#1000;
	GPMC_Write(36,16'hBEEF);
	#1000;
	GPMC_Read(36);
	#1000;
	$finish;
     end
   
endmodule // gpmc_model

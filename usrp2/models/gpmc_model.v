

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
      input ctrl;
      input [10:0] addr;
      input [15:0] data;
      begin
	 #2.3;
	 EM_A <= addr[10:1];
	 EM_D_int <= data;
	 #2.01;
	 if(ctrl)
	   EM_NCS6 <= 0;
	 else
	   EM_NCS4 <= 0;
	 #14;
	 EM_NWE <= 0;
	 #77.5;
	 EM_NCS4 <= 1;
	 EM_NCS6 <= 1;
	 //#1.5;
	 EM_NWE <= 1;
	 #6;
	 EM_A <= 10'bz;
	 EM_D_int <= 16'bz;
      end
   endtask // GPMC_Write

   task GPMC_Read;
      input ctrl;
      input [10:0] addr;
      begin
	 #1.3;
	 EM_A <= addr[10:1];
	 #3;
	 if(ctrl)
	   EM_NCS6 <= 0;
	 else
	   EM_NCS4 <= 0;
	 #14;
	 EM_NOE <= 0;
	 #77.5;
	 EM_NCS4 <= 1;
	 EM_NCS6 <= 1;
	 //#1.5;
	 $display("Data Read from GPMC: %X",EM_D);
	 EM_NOE <= 1;
	 #254;
	 EM_A <= 10'bz;
      end
   endtask // GPMC_Read
   
   initial
     begin
	#1000;
	GPMC_Write(1,36,16'hF00D);
	#1000;
	GPMC_Read(1,36);
	#1000;
	GPMC_Write(0,36,16'hF00D);
	GPMC_Write(0,38,16'hF00D);
	GPMC_Write(0,40,16'hF00D);
	GPMC_Write(0,11'h7FFE,16'hF00D);
	#1000;
	$finish;
     end
   
endmodule // gpmc_model

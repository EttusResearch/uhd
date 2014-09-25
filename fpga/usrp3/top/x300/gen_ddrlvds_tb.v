`timescale 1ps/1ps

module gen_ddrlvds_tb();

   wire GSR, GTS;
   glbl glbl( );

   reg clk    = 1;
   reg clk_2x = 1;
   reg reset  = 1;
   reg sync_dacs_req = 0;
   
   always #100 clk = ~clk;
   always #50 clk_2x = ~clk_2x;
   
   wire [7:0] dac_pins_p, dac_pins_n;
   wire       dac_frame_p, dac_frame_n;
   wire       dac_clk_p, dac_clk_n;
      
   reg [7:0]  count;
   
   wire [15:0] i = {4'hA,count};
   wire [15:0] q = {4'hB,count};

   initial
   begin
      #500 reset = 0;
		@(posedge clk);
		//#10
      sync_dacs_req <= 1;
		#200
      sync_dacs_req <= 0;
		
//      BURST (4);
//      BURST (5);
      #20000;
   end

   task BURST;
      input [7:0] len;
   begin
      sync_dacs_req <= 0;
      count <= 0;
      @ (posedge clk);
      @ (posedge clk);
      repeat (len)
      begin
         sync_dacs_req <= 1;
         @ (posedge clk);
         sync_dacs_req <= 0;
         @ (posedge clk);
         count <= count + 1;
      end
      sync_dacs_req <= 0;
      @ (posedge clk);
      @ (posedge clk);
      @ (posedge clk);
   end
   endtask //

   gen_ddrlvds dut (
      .reset(reset),
      .tx_clk_2x_p(dac_clk_p), .tx_clk_2x_n(dac_clk_n),
      .tx_frame_p(dac_frame_p), .tx_frame_n(dac_frame_n),
      .tx_d_p(dac_pins_p), .tx_d_n(dac_pins_n),
      .tx_clk_2x(clk_2x), .tx_clk_1x(clk),
      .i(i), .q(q),
      .sync_dacs(sync_dacs_req)
   );

endmodule // gen_ddrlvds_tb

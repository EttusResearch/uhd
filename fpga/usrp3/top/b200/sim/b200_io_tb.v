`timescale 1ns/1ps

  module b200_io_tb();

   //
   // Xilinx Mandatory Simulation Primitive for global signals.
   //
   wire        GSR, GTS;
   glbl glbl( );

   //
   // Test bench declarations
   //
   reg [7:0]   count;
   wire [11:0] i0 = {4'hA,count};
   wire [11:0] q0 = {4'hB,count};
   wire [11:0] i1 = {4'hC,count};
   wire [11:0] q1 = {4'hD,count};
   reg 	       tb_clk = 0;

   // RX sample bus.
   reg 	       rx_clk = 0;  // Simulated clock from AD9361 for RX sample interface, radio_clk derived from this.
   reg 	       rx_frame;
   reg [11:0]  rx_data;
   // TX sample bus.
   wire        tx_clk;
   wire        tx_frame;
   wire [11:0] tx_data;
   // Internal FPGA interface(s)
   reg 	       reset  = 1;
   wire        radio_clk;
   reg 	       mimo;
   wire [11:0] rx_i0, rx_q0, rx_i1, rx_q1;
   reg [11:0] tx_i0, tx_q0, tx_i1, tx_q1;


   // Set tb_clk to 100MHz.
   // rx_clk is half the frequency of tb_clk, and tb_clk posedges are miday between edges on the rx_clk
   always #10 tb_clk = ~tb_clk;
   always @(negedge tb_clk) rx_clk <= ~rx_clk;


   b200_io dut
     (
     .reset(reset),
     .mimo(mimo),

     // Baseband sample interface
     .radio_clk(radio_clk),
     .rx_i0(rx_i0),
     .rx_q0(rx_q0),
     .rx_i1(rx_i1),
     .rx_q1(rx_q1),
     .tx_i0(tx_i0),
     .tx_q0(tx_q0),
     .tx_i1(tx_i1),
     .tx_q1(tx_q1),

     // Catalina interface
     .rx_clk(rx_clk),
     .rx_frame(rx_frame),
     .rx_data(rx_data),
     .tx_clk(tx_clk),
     .tx_frame(tx_frame),
     .tx_data(tx_data)
     );

   // Internal Loopback Rx -> Tx.
   always @(posedge radio_clk)
     begin
	tx_i0 <= rx_i0;
	tx_q0 <= rx_q0;
	tx_i1 <= rx_i1;
	tx_q1 <= rx_q1;
     end

   //
   // Task's for stimulus
   //

   task siso_burst;
      input [7:0] len;
      begin
	 rx_frame <= 0;
	 mimo <= 0;
	 count <= 0;
	 // Now give configuration a chance to perculate
	 @(posedge rx_clk);
	 @(posedge rx_clk);
	 @(posedge rx_clk);
	 @(posedge rx_clk);
	 // Now entering main stimulus loop just after rising edge of rx_clk
	 repeat(len)
	   begin
	      // Drive I data so that it surrounds a falling edge on rx_clk
	      @(posedge tb_clk);
	      rx_data <= i0;
	      rx_frame <= 1;
	      // Drive Q data so that it surrounds a rising edge on rx_clk
	      @(posedge tb_clk);
	      rx_data <= q0;
	      rx_frame <= 0;
	      // Increment test data pattern
	      count <= count + 1;
	   end // repeat (len)
	 @(posedge rx_clk);
	 @(posedge rx_clk);
      end

   endtask // BURST


   task mimo_burst;
      input [7:0] len;
      begin
	 rx_frame <= 0;
	 mimo <= 1;
	 count <= 0;
	 // Now give configuration a chance to perculate
	 @(posedge rx_clk);
	 @(posedge rx_clk);
	 @(posedge rx_clk);
	 @(posedge rx_clk);
	 // Now entering main stimulus loop just after rising edge of rx_clk
	 repeat(len)
	   // REMEMBER! B210 PCB markings for radio channels are swapped w.r.t AD9361's channels.
	   // "Ch0" as indicated here is "Ch1" inside AD9361
	   begin
	      // Drive I data for Ch1 so that it surrounds a falling edge on rx_clk
	      @(posedge tb_clk);
	      rx_data <= i1;
	      rx_frame <= 1;
	      // Drive Q data for Ch1 so that it surrounds a rising edge on rx_clk
	      @(posedge tb_clk);
	      rx_data <= q1;
	      // Drive I data for Ch0 so that it surrounds a falling edge on rx_clk
	      @(posedge tb_clk);
	      rx_data <= i0;
	      rx_frame <= 0;
	      // Drive Q data for Ch0 so that it surrounds a rising edge on rx_clk
	      @(posedge tb_clk);
	      rx_data <= q0;
	      // Increment test data pattern
	      count <= count + 1;
	   end
	 @(posedge rx_clk);
	 @(posedge rx_clk);
      end
   endtask // MIMO_BURST

   // Pull in local simulation script here.
`include "simulation_script.v"


endmodule // b200_io_tb

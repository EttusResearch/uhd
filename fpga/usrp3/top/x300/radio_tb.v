module radio_tb();
   //
   // Pull in standard tasks and signal name declarations.
   //
`include "task_libraray.v"


   
   
   //
   // DUT
   //
   radio radio_i0
     #(
       .CHIPSCOPE (0),
       .DELETE_DSP(0))
      
       (
	.radio_clk(clk), 
	.radio_clk_2x(1'b0), // Unused
	.radio_rst(reset),
	.rx(rx), 
	.tx(tx),
	.db_gpio(db_gpio),
	.fp_gpio(fp_gpio),
	.sen(sen), 
	.sclk(sclk), 
	.mosi(mosi), 
	.miso(miso),
	.misc_outs(misc_outs), 
	.leds(leds),

	.bus_clk(clk), 
	.bus_rst(reset),
	.in_tdata(data_in), 
	.in_tlast(last_in), 
	.in_tvalid(valid_in), 
	.in_tready(ready_in),
	.out_tdata(data_out), 
	.out_tlast(data_out), 
	.out_tvalid(valid_out), 
	.out_tready(ready_out),
	.tx_tdata_bi(tx_tdata_bi), 
	.tx_tlast_bi(tx_tlast_bi), 
	.tx_tvalid_bi(tx_tvalid_bi), 
	.tx_tready_bi(tx_tready_bi),
	.tx_tdata_bo(tx_tdata_bo), 
	.tx_tlast_bo(tx_tlast_bo), 
	.tx_tvalid_bo(tx_tvalid_bo), 
	.tx_tready_bo(tx_tready_bo),

	.pps(pps),

	.debug()
	);

   //
   // Alternate smaller internal SRAM based FIFO's for Tx when DRAM not compiled into FPGA.
   //
   wire [63:0] tx_tdata_0; wire tx_tlast_0, tx_tvalid_0, tx_tready_0;
   wire [63:0] tx_tdata_1; wire tx_tlast_1, tx_tvalid_1, tx_tready_1;
   wire [63:0] tx_tdata_2; wire tx_tlast_2, tx_tvalid_2, tx_tready_2;

   axi_fifo #(.WIDTH(65), .SIZE(`SRAM_FIFO_SIZE-2)) tx_fifo0_0
     (.clk(bus_clk), .reset(bus_rst), .clear(1'b0),
      .i_tdata({tx_tlast_bo,tx_tdata_bo}), .i_tvalid(tx_tvalid_bo), .i_tready(tx_tready_bo),
      .o_tdata({tx_tlast_0,tx_tdata_0}), .o_tvalid(tx_tvalid_0), .o_tready(tx_tready_0),
      .space(), .occupied());
   
   axi_fifo #(.WIDTH(65), .SIZE(`SRAM_FIFO_SIZE-2)) tx_fifo0_1
     (.clk(bus_clk), .reset(bus_rst), .clear(1'b0),
      .i_tdata({tx_tlast_0,tx_tdata_0}), .i_tvalid(tx_tvalid_0), .i_tready(tx_tready_0),
      .o_tdata({tx_tlast_1,tx_tdata_1}), .o_tvalid(tx_tvalid_1), .o_tready(tx_tready_1),
      .space(), .occupied());
   
   axi_fifo #(.WIDTH(65), .SIZE(`SRAM_FIFO_SIZE-2)) tx_fifo0_2
     (.clk(bus_clk), .reset(bus_rst), .clear(1'b0),
      .i_tdata({tx_tlast_1,tx_tdata_1}), .i_tvalid(tx_tvalid_1), .i_tready(tx_tready_1),
      .o_tdata({tx_tlast_2,tx_tdata_2}), .o_tvalid(tx_tvalid_2), .o_tready(tx_tready_2),
      .space(), .occupied());
   
   axi_fifo #(.WIDTH(65), .SIZE(`SRAM_FIFO_SIZE-2)) tx_fifo0_3
     (.clk(bus_clk), .reset(bus_rst), .clear(1'b0),
      .i_tdata({tx_tlast_2,tx_tdata_2}), .i_tvalid(tx_tvalid_2), .i_tready(tx_tready_2),
      .o_tdata({tx_tlast_bi,tx_tdata_bi}), .o_tvalid(tx_tvalid_bi), .o_tready(tx_tready_bi),
      .space(), .occupied());

   //
   // DUT ends.
   //


   //
   // Include per simulation scripts
   //
   `include "simulation_script.v"

endmodule // radio_tb

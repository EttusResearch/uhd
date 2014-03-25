

module serial_to_settings_tb();



   reg clk;
   reg reset;

   wire scl;
   wire sda;
   wire set_stb;
   wire [7:0] set_addr;
   wire [31:0] set_data;

   //
   // These registers optionaly used
   // to drive nets through procedural assignments in test bench.
   // These drivers default to tri-stated.
   //
   reg 	       scl_r;
   reg 	       sda_r;

   assign      scl = scl_r;
   assign      sda = sda_r;

   initial
     begin
	scl_r <= 1'bz;
	sda_r <= 1'bz;
     end
   
   

     serial_to_settings   serial_to_settings_i
     (
      .clk(clk),
      .reset(reset),
      // Serial signals (async)
      .scl(scl),
      .sda(sda),
      // Settngs bus out
      .set_stb(set_stb),
      .set_addr(set_addr),
      .set_data(set_data)
      );

   // Nasty HAck to convert settings to wishbone crudely.
   reg 	       wb_stb;
   wire        wb_ack_o;
   
   
   always @(posedge clk)
     if (reset)
       wb_stb <= 0;
     else
       wb_stb <= set_stb ? 1 : ((wb_ack_o) ? 0 : wb_stb);
   
     simple_uart debug_uart
     (
      .clk_i(clk), 
      .rst_i(reset),
      .we_i(wb_stb), 
      .stb_i(wb_stb), 
      .cyc_i(wb_stb), 
      .ack_o(wb_ack_o),
      .adr_i(set_addr[2:0]), 
      .dat_i(set_data[31:0]), 
      .dat_o(),
      .rx_int_o(), 
      .tx_int_o(), 
      .tx_o(txd), 
      .rx_i(rxd), 
      .baud_o()
      );
   
   //
   // Bring in a simulation script here
   //
   `include "simulation_script.v"

endmodule


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

   
   //
   // Bring in a simulation script here
   //
   `include "simulation_script.v"

endmodule
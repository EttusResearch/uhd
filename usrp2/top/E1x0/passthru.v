`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////

module passthru
  (input overo_gpio145,
   output cgen_sclk,
   output cgen_sen_b,
   output cgen_mosi,
   input fpga_cfg_din,
   input fpga_cfg_cclk
   );
   
   assign cgen_sclk = fpga_cfg_cclk;
   assign cgen_sen_b = overo_gpio145;
   assign cgen_mosi = fpga_cfg_din;
   
   
endmodule // passthru

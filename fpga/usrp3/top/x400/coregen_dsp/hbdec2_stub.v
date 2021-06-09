module hbdec2(
  sclr, ce, rfd, rdy, data_valid, coef_we, nd, clk, 
  coef_ld, dout_1, dout_2, din_1, din_2, coef_din
)
/* synthesis syn_black_box black_box_pad_pin="sclr,ce,rfd,rdy,data_valid,coef_we,nd,clk,coef_ld,dout_1[47:0],dout_2[47:0],din_1[23:0],din_2[23:0],coef_din[17:0]" */;
  input sclr;
  input ce;
  output rfd;
  output rdy;
  output data_valid;
  input coef_we;
  input nd;
  input clk;
  input coef_ld;
  output [46:0]dout_1;
  output [46:0]dout_2;
  input [23:0]din_1;
  input [23:0]din_2;
  input [17:0]coef_din;
endmodule

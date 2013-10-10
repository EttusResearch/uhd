
//
// Copyright 2011 Ettus Research LLC
//



module gpio_atr
  #(parameter BASE = 0,
    parameter WIDTH = 32)
   (input clk, input reset,
    input set_stb, input [7:0] set_addr, input [31:0] set_data,
    input rx, input tx,
    inout [WIDTH-1:0] gpio,
    output reg [31:0] gpio_readback
    );
   
   wire [WIDTH-1:0]   ddr, in_idle, in_tx, in_rx, in_fdx;
   reg [WIDTH-1:0]    rgpio, igpio;
   reg [WIDTH-1:0]    gpio_pipe;
   
   
   setting_reg #(.my_addr(BASE+0), .width(WIDTH)) reg_idle
     (.clk(clk),.rst(reset),.strobe(set_stb),.addr(set_addr), .in(set_data),
      .out(in_idle),.changed());

   setting_reg #(.my_addr(BASE+1), .width(WIDTH)) reg_rx
     (.clk(clk),.rst(reset),.strobe(set_stb),.addr(set_addr), .in(set_data),
      .out(in_rx),.changed());

   setting_reg #(.my_addr(BASE+2), .width(WIDTH)) reg_tx
     (.clk(clk),.rst(reset),.strobe(set_stb),.addr(set_addr), .in(set_data),
      .out(in_tx),.changed());

   setting_reg #(.my_addr(BASE+3), .width(WIDTH)) reg_fdx
     (.clk(clk),.rst(reset),.strobe(set_stb),.addr(set_addr), .in(set_data),
      .out(in_fdx),.changed());

   setting_reg #(.my_addr(BASE+4), .width(WIDTH)) reg_ddr
     (.clk(clk),.rst(reset),.strobe(set_stb),.addr(set_addr), .in(set_data),
      .out(ddr),.changed());

   always @(posedge clk)
     case({tx,rx})
       2'b00: rgpio <= in_idle;
       2'b01: rgpio <= in_rx;
       2'b10: rgpio <= in_tx;
       2'b11: rgpio <= in_fdx;
     endcase // case ({tx,rx})
   
   integer 	      n;
   always @*
     for(n=0;n<WIDTH;n=n+1)
       igpio[n] <= ddr[n] ? rgpio[n] : 1'bz;

   assign     gpio = igpio;

   // Double pipeline stage for timing, first flop is in IOB, second in core logic.
   always @(posedge clk) begin
      gpio_pipe <= gpio;
      gpio_readback <= gpio_pipe;
   end
   
endmodule // gpio_atr

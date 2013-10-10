////////////////////////////////////////////////////////////////////////
// Copyright Ettus Research LLC
////////////////////////////////////////////////////////////////////////

module dram_2port
  #(parameter DWIDTH=32,
    parameter AWIDTH=9)
    (input clk,
     input write,
     input [AWIDTH-1:0] raddr,
     input [AWIDTH-1:0] waddr,
     input [DWIDTH-1:0] wdata,
     output [DWIDTH-1:0] rdata);

    reg [DWIDTH-1:0] ram [(1<<AWIDTH)-1:0];
   integer 	    i;
   initial
     for(i=0;i<(1<<AWIDTH);i=i+1)
       ram[i] <= {DWIDTH{1'b0}};

    assign rdata = ram[raddr];

    always @(posedge clk) begin
        if (write) ram[waddr] <= wdata;
    end

endmodule //dram_2port

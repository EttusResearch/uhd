

module edge_sync
  #(parameter POSEDGE = 1)
   (input clk,
    input rst,
    input sig,
    output trig);
   
   reg [1:0] delay;
   
   always @(posedge clk)
     if(rst)
       delay <= 2'b00;
     else
       delay <= {delay[0],sig};
   
   assign trig = POSEDGE ? (delay==2'b01) : (delay==2'b10);
   
endmodule // edge_sync



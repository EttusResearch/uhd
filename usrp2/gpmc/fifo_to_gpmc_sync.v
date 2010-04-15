
// Assumes a GPMC cycle with GPMC clock, as in the timing diagrams
//   If a packet bigger or smaller than we are told is sent, behavior is undefined.
//   If dst_rdy_i is low when we get data, behavior is undefined and we signal bus error.
//   If there is a bus error, we should be reset

module fifo_to_gpmc_sync
  (input arst,
   input [17:0] data_i, input src_rdy_i, output dst_rdy_o,
   input EM_CLK, output [15:0] EM_D, input EM_NCS, input EM_NOE,
   output fifo_ready, 
   output reg bus_error);

   assign EM_D = data_i[15:0];
   wire       read_access = ~EM_NCS & ~EM_NOE;

   assign dst_rdy_o = read_access;

   always @(posedge EM_CLK or posedge arst)
     if(arst)
       bus_error <= 0;
     else if(dst_rdy_o & ~src_rdy_i)
       bus_error <= 1;
   

endmodule // fifo_to_gpmc_sync

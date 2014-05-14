//
// Copyright 2013 Ettus Research LLC
//


module pcie_lossy_samp_gate
(
   input [63:0]   i_tdata,
   input          i_tvalid,
   output         i_tready,
   
   output [63:0]  o_tdata,
   output         o_tvalid,
   input          o_tready,
   
   input          drop,
   output         dropping
);

   assign o_tdata    = i_tdata;
   assign o_tvalid   = i_tvalid & ~drop;
   assign i_tready   = o_tready | drop;
   
   assign dropping   = drop & i_tvalid;
   
endmodule // pcie_lossy_samp_gate

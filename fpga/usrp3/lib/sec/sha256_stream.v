//======================================================================
//
// sha256_stream.v
// --------
// Top level wrapper for the SHA-256 hash function providing
// a simple stream interface with 512 bit data access.
//
//
// Author: Olof Kindgren
// Copyright (c) 2016, Olof Kindgren
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or 
// without modification, are permitted provided that the following 
// conditions are met: 
// 
// 1. Redistributions of source code must retain the above copyright 
//    notice, this list of conditions and the following disclaimer. 
// 
// 2. Redistributions in binary form must reproduce the above copyright 
//    notice, this list of conditions and the following disclaimer in 
//    the documentation and/or other materials provided with the 
//    distribution. 
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
// FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE 
// COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, 
// INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, 
// BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER 
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, 
// STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF 
// ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
//======================================================================

`default_nettype wire

module sha256_stream
  (input 	  clk,
   input 	  rst,
   input 	  mode,
   input [511:0]  s_tdata_i,
   input 	  s_tlast_i,
   input 	  s_tvalid_i,
   output 	  s_tready_o,
   output [255:0] digest_o,
   output 	  digest_valid_o);

   reg 		  first_block;

   always @(posedge clk) begin
      if (s_tvalid_i & s_tready_o)
	first_block <= s_tlast_i;

      if (rst) begin
	 first_block <= 1'b1;
      end
   end

   sha256_core core
     (.clk     (clk),
      .reset_n (~rst),

      .init(s_tvalid_i & first_block),
      .next(s_tvalid_i & !first_block),
      .mode (mode),

      .block(s_tdata_i),

      .ready(s_tready_o),

      .digest       (digest_o),
      .digest_valid (digest_valid_o));

endmodule

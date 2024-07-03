//======================================================================
//
// sha256_w_mem_regs.v
// -------------------
// The W memory. This version uses 16 32-bit registers as a sliding
// window to generate the 64 words.
//
//
// Author: Joachim Strombergson
// Copyright (c) 2013, Secworks Sweden AB
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

`default_nettype none

module sha256_w_mem(
                    input wire           clk,
                    input wire           reset_n,

                    input wire [511 : 0] block,
                    input wire [5 : 0]   round,

                    input wire           init,
                    input wire           next,
                    output wire [31 : 0] w
                   );


  //----------------------------------------------------------------
  // Registers including update variables and write enable.
  //----------------------------------------------------------------
  reg [31 : 0] w_mem [0 : 15];
  reg [31 : 0] w_mem00_new;
  reg [31 : 0] w_mem01_new;
  reg [31 : 0] w_mem02_new;
  reg [31 : 0] w_mem03_new;
  reg [31 : 0] w_mem04_new;
  reg [31 : 0] w_mem05_new;
  reg [31 : 0] w_mem06_new;
  reg [31 : 0] w_mem07_new;
  reg [31 : 0] w_mem08_new;
  reg [31 : 0] w_mem09_new;
  reg [31 : 0] w_mem10_new;
  reg [31 : 0] w_mem11_new;
  reg [31 : 0] w_mem12_new;
  reg [31 : 0] w_mem13_new;
  reg [31 : 0] w_mem14_new;
  reg [31 : 0] w_mem15_new;
  reg          w_mem_we;


  //----------------------------------------------------------------
  // Wires.
  //----------------------------------------------------------------
  reg [31 : 0] w_tmp;
  reg [31 : 0] w_new;


  //----------------------------------------------------------------
  // Concurrent connectivity for ports etc.
  //----------------------------------------------------------------
  assign w = w_tmp;


  //----------------------------------------------------------------
  // reg_update
  // Update functionality for all registers in the core.
  // All registers are positive edge triggered with synchronous
  // active low reset. All registers have write enable.
  //----------------------------------------------------------------
  always @ (posedge clk or negedge reset_n)
    begin : reg_update
      integer i;

      if (!reset_n)
        begin
          for (i = 0 ; i < 16 ; i = i + 1) begin
            w_mem[i] <= 32'h0;
	  end
        end
      else
        begin
          if (w_mem_we)
            begin
              w_mem[00] <= w_mem00_new;
              w_mem[01] <= w_mem01_new;
              w_mem[02] <= w_mem02_new;
              w_mem[03] <= w_mem03_new;
              w_mem[04] <= w_mem04_new;
              w_mem[05] <= w_mem05_new;
              w_mem[06] <= w_mem06_new;
              w_mem[07] <= w_mem07_new;
              w_mem[08] <= w_mem08_new;
              w_mem[09] <= w_mem09_new;
              w_mem[10] <= w_mem10_new;
              w_mem[11] <= w_mem11_new;
              w_mem[12] <= w_mem12_new;
              w_mem[13] <= w_mem13_new;
              w_mem[14] <= w_mem14_new;
              w_mem[15] <= w_mem15_new;
            end
        end
    end // reg_update


  //----------------------------------------------------------------
  // select_w
  //
  // Mux for the external read operation. This is where we exract
  // the W variable.
  //----------------------------------------------------------------
  always @*
    begin : select_w
      if (round < 16)
        w_tmp = w_mem[round[3 : 0]];
      else
        w_tmp = w_new;
    end // select_w


  //----------------------------------------------------------------
  // w_new_logic
  //
  // Logic that calculates the next value to be inserted into
  // the sliding window of the memory.
  //----------------------------------------------------------------
  always @*
    begin : w_mem_update_logic
      reg [31 : 0] w_0;
      reg [31 : 0] w_1;
      reg [31 : 0] w_9;
      reg [31 : 0] w_14;
      reg [31 : 0] d0;
      reg [31 : 0] d1;

      w_mem00_new = 32'h0;
      w_mem01_new = 32'h0;
      w_mem02_new = 32'h0;
      w_mem03_new = 32'h0;
      w_mem04_new = 32'h0;
      w_mem05_new = 32'h0;
      w_mem06_new = 32'h0;
      w_mem07_new = 32'h0;
      w_mem08_new = 32'h0;
      w_mem09_new = 32'h0;
      w_mem10_new = 32'h0;
      w_mem11_new = 32'h0;
      w_mem12_new = 32'h0;
      w_mem13_new = 32'h0;
      w_mem14_new = 32'h0;
      w_mem15_new = 32'h0;
      w_mem_we    = 0;

      w_0  = w_mem[0];
      w_1  = w_mem[1];
      w_9  = w_mem[9];
      w_14 = w_mem[14];

      d0 = {w_1[6  : 0], w_1[31 :  7]} ^
           {w_1[17 : 0], w_1[31 : 18]} ^
           {3'b000, w_1[31 : 3]};

      d1 = {w_14[16 : 0], w_14[31 : 17]} ^
           {w_14[18 : 0], w_14[31 : 19]} ^
           {10'b0000000000, w_14[31 : 10]};

      w_new = d1 + w_9 + d0 + w_0;

      if (init)
        begin
          w_mem00_new = block[511 : 480];
          w_mem01_new = block[479 : 448];
          w_mem02_new = block[447 : 416];
          w_mem03_new = block[415 : 384];
          w_mem04_new = block[383 : 352];
          w_mem05_new = block[351 : 320];
          w_mem06_new = block[319 : 288];
          w_mem07_new = block[287 : 256];
          w_mem08_new = block[255 : 224];
          w_mem09_new = block[223 : 192];
          w_mem10_new = block[191 : 160];
          w_mem11_new = block[159 : 128];
          w_mem12_new = block[127 :  96];
          w_mem13_new = block[95  :  64];
          w_mem14_new = block[63  :  32];
          w_mem15_new = block[31  :   0];
          w_mem_we    = 1;
        end

      if (next && (round > 15))
        begin
          w_mem00_new = w_mem[01];
          w_mem01_new = w_mem[02];
          w_mem02_new = w_mem[03];
          w_mem03_new = w_mem[04];
          w_mem04_new = w_mem[05];
          w_mem05_new = w_mem[06];
          w_mem06_new = w_mem[07];
          w_mem07_new = w_mem[08];
          w_mem08_new = w_mem[09];
          w_mem09_new = w_mem[10];
          w_mem10_new = w_mem[11];
          w_mem11_new = w_mem[12];
          w_mem12_new = w_mem[13];
          w_mem13_new = w_mem[14];
          w_mem14_new = w_mem[15];
          w_mem15_new = w_new;
          w_mem_we    = 1;
        end
    end // w_mem_update_logic
endmodule // sha256_w_mem

`default_nettype wire
//======================================================================
// sha256_w_mem.v
//======================================================================

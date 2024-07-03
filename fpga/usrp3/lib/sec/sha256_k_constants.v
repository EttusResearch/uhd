//======================================================================
//
// sha256_k_constants.v
// --------------------
// The table K with constants in the SHA-256 hash function.
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

module sha256_k_constants(
                          input wire  [5 : 0] round,
                          output wire [31 : 0] K
                         );

  //----------------------------------------------------------------
  // Wires.
  //----------------------------------------------------------------
  reg [31 : 0] tmp_K;


  //----------------------------------------------------------------
  // Concurrent connectivity for ports etc.
  //----------------------------------------------------------------
  assign K = tmp_K;


  //----------------------------------------------------------------
  // round_mux
  //----------------------------------------------------------------
  always @*
    begin : round_mux
      case(round)
        00: tmp_K = 32'h428a2f98;
        01: tmp_K = 32'h71374491;
        02: tmp_K = 32'hb5c0fbcf;
        03: tmp_K = 32'he9b5dba5;
        04: tmp_K = 32'h3956c25b;
        05: tmp_K = 32'h59f111f1;
        06: tmp_K = 32'h923f82a4;
        07: tmp_K = 32'hab1c5ed5;
        08: tmp_K = 32'hd807aa98;
        09: tmp_K = 32'h12835b01;
        10: tmp_K = 32'h243185be;
        11: tmp_K = 32'h550c7dc3;
        12: tmp_K = 32'h72be5d74;
        13: tmp_K = 32'h80deb1fe;
        14: tmp_K = 32'h9bdc06a7;
        15: tmp_K = 32'hc19bf174;
        16: tmp_K = 32'he49b69c1;
        17: tmp_K = 32'hefbe4786;
        18: tmp_K = 32'h0fc19dc6;
        19: tmp_K = 32'h240ca1cc;
        20: tmp_K = 32'h2de92c6f;
        21: tmp_K = 32'h4a7484aa;
        22: tmp_K = 32'h5cb0a9dc;
        23: tmp_K = 32'h76f988da;
        24: tmp_K = 32'h983e5152;
        25: tmp_K = 32'ha831c66d;
        26: tmp_K = 32'hb00327c8;
        27: tmp_K = 32'hbf597fc7;
        28: tmp_K = 32'hc6e00bf3;
        29: tmp_K = 32'hd5a79147;
        30: tmp_K = 32'h06ca6351;
        31: tmp_K = 32'h14292967;
        32: tmp_K = 32'h27b70a85;
        33: tmp_K = 32'h2e1b2138;
        34: tmp_K = 32'h4d2c6dfc;
        35: tmp_K = 32'h53380d13;
        36: tmp_K = 32'h650a7354;
        37: tmp_K = 32'h766a0abb;
        38: tmp_K = 32'h81c2c92e;
        39: tmp_K = 32'h92722c85;
        40: tmp_K = 32'ha2bfe8a1;
        41: tmp_K = 32'ha81a664b;
        42: tmp_K = 32'hc24b8b70;
        43: tmp_K = 32'hc76c51a3;
        44: tmp_K = 32'hd192e819;
        45: tmp_K = 32'hd6990624;
        46: tmp_K = 32'hf40e3585;
        47: tmp_K = 32'h106aa070;
        48: tmp_K = 32'h19a4c116;
        49: tmp_K = 32'h1e376c08;
        50: tmp_K = 32'h2748774c;
        51: tmp_K = 32'h34b0bcb5;
        52: tmp_K = 32'h391c0cb3;
        53: tmp_K = 32'h4ed8aa4a;
        54: tmp_K = 32'h5b9cca4f;
        55: tmp_K = 32'h682e6ff3;
        56: tmp_K = 32'h748f82ee;
        57: tmp_K = 32'h78a5636f;
        58: tmp_K = 32'h84c87814;
        59: tmp_K = 32'h8cc70208;
        60: tmp_K = 32'h90befffa;
        61: tmp_K = 32'ha4506ceb;
        62: tmp_K = 32'hbef9a3f7;
        63: tmp_K = 32'hc67178f2;
      endcase // case (round)
    end // block: round_mux
endmodule // sha256_k_constants

`default_nettype wire

//======================================================================
// sha256_k_constants.v
//======================================================================

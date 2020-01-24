// Modified from code originally by Richard Herveille, his copyright is below

/////////////////////////////////////////////////////////////////////
////                                                             ////
////  OpenCores Simple General Purpose IO core                   ////
////                                                             ////
////  Author: Richard Herveille                                  ////
////          richard@asics.ws                                   ////
////          www.asics.ws                                       ////
////                                                             ////
/////////////////////////////////////////////////////////////////////
////                                                             ////
//// Copyright (C) 2002 Richard Herveille                        ////
////                    richard@asics.ws                         ////
////                                                             ////
//// This source file may be used and distributed without        ////
//// restriction provided that this copyright statement is not   ////
//// removed from the file and that any derivative work contains ////
//// the original copyright notice and the associated disclaimer.////
////                                                             ////
////     THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY     ////
//// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED   ////
//// TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS   ////
//// FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL THE AUTHOR      ////
//// OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,         ////
//// INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES    ////
//// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE   ////
//// GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR        ////
//// BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF  ////
//// LIABILITY, WHETHER IN  CONTRACT, STRICT LIABILITY, OR TORT  ////
//// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT  ////
//// OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE         ////
//// POSSIBILITY OF SUCH DAMAGE.                                 ////
////                                                             ////
/////////////////////////////////////////////////////////////////////


module nsgpio16LE
  (input clk_i, input rst_i, 
   input cyc_i, input stb_i, input [3:0] adr_i, input we_i, input [15:0] dat_i, 
   output reg [15:0] dat_o, output reg ack_o,
   input [31:0] atr, input [31:0] debug_0, input [31:0] debug_1, 
   inout [31:0] gpio
   );

   reg [31:0] 	ctrl, line, ddr, dbg, lgpio;
   
   wire 	wb_acc = cyc_i & stb_i;            // WISHBONE access
   wire 	wb_wr  = wb_acc & we_i;            // WISHBONE write access

   always @(posedge clk_i or posedge rst_i)
     if (rst_i)
       begin
          ctrl <= 32'h0;
          line <= 32'h0;
	  ddr <= 32'h0;
	  dbg <= 32'h0;
       end
     else if (wb_wr)
       case( adr_i[3:1] )
	 3'b000 : 
           line[15:0] <= dat_i;
	 3'b001 : 
           line[31:16] <= dat_i;
	 3'b010 :
	   ddr[15:0] <= dat_i;
	 3'b011 :
	   ddr[31:16] <= dat_i;
	 3'b100 :
	   ctrl[15:0] <= dat_i;
	 3'b101 :
	   ctrl[31:16] <= dat_i;
	 3'b110 :
	   dbg[15:0] <= dat_i;
	 3'b111 :
	   dbg[31:16] <= dat_i;
       endcase // case ( adr_i[3:1] )
   
   always @(posedge clk_i)
     case (adr_i[3:1])
       3'b000 :
	 dat_o <= lgpio[15:0];
       3'b001 :
	 dat_o <= lgpio[31:16];
       3'b010 :
	 dat_o <= ddr[15:0];
       3'b011 :
	 dat_o <= ddr[31:16];
       3'b100 :
	 dat_o <= ctrl[15:0];
       3'b101 :
	 dat_o <= ctrl[31:16];
       3'b110 :
	 dat_o <= dbg[15:0];
       3'b111 :
	 dat_o <= dbg[31:16];
     endcase // case (adr_i[3:1])
   
   
   always @(posedge clk_i or posedge rst_i)
     if (rst_i)
       ack_o <= 1'b0;
     else
       ack_o <= wb_acc & !ack_o;
   
   // latch GPIO input pins
   always @(posedge clk_i)
     lgpio <= gpio;
   
   // assign GPIO outputs
   integer   n;
   reg [31:0] igpio; // temporary internal signal
   
   always @(ctrl or line or debug_1 or debug_0 or atr or ddr or dbg)
     for(n=0;n<32;n=n+1)
       igpio[n] <= ddr[n] ? (dbg[n] ? (ctrl[n] ? debug_1[n] : debug_0[n]) : 
			     (ctrl[n] ?  atr[n] : line[n]) )
	 : 1'bz;
   
   assign     gpio = igpio;
   
endmodule


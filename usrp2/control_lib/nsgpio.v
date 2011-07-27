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


module nsgpio
  (input clk_i, input rst_i, 
   input cyc_i, input stb_i, input [4:0] adr_i, input we_i, input [31:0] dat_i, 
   output reg [31:0] dat_o, output reg ack_o,
   input tx, input rx, inout [31:0] gpio
   );

   integer n;
   reg [31:0] ddr;
   reg [31:0] idle_out, rx_out, tx_out, fdx_out;
   reg [31:0] rgpio, igpio;
   
   wire 	wb_acc = cyc_i & stb_i;            // WISHBONE access
   wire 	wb_wr  = wb_acc & we_i;            // WISHBONE write access

   always @(posedge clk_i or posedge rst_i)
     if (rst_i)
       ddr <= 0;
     else if (wb_wr)
       case( adr_i[4:2] )
	 3'b000 : 
           idle_out <= dat_i;
	 3'b001 :
	   rx_out <= dat_i;
	 3'b010 :
	   tx_out <= dat_i;
	 3'b011 :
	   fdx_out <= dat_i;
	 3'b100 :
	   ddr <= dat_i;
       endcase // case ( adr_i[4:2] )
      
   always @(posedge clk_i)
     dat_o <= gpio;
   
   always @(posedge clk_i or posedge rst_i)
     if (rst_i)
       ack_o <= 1'b0;
     else
       ack_o <= wb_acc & !ack_o;

   always @(posedge clk_i)
     case({tx,rx})
       2'b00 : rgpio <= idle_out;
       2'b01 : rgpio <= rx_out;
       2'b10 : rgpio <= tx_out;
       2'b11 : rgpio <= fdx_out;
     endcase // case ({tx,rx})
   
   always @*
     for(n=0;n<32;n=n+1)
       igpio[n] <= ddr[n] ? rgpio[n] : 1'bz;

   assign     gpio = igpio;
   
endmodule

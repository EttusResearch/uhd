`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////

module gpmc
  (input EM_CLK, inout [15:0] EM_D, input [10:1] EM_A, input [1:0] EM_NBE,
   input EM_WAIT0, input EM_NCS4, input EM_NCS6, input EM_NWE, input EM_NOE,

   input wb_clk, input wb_rst,
   output reg [10:0] wb_adr_o, output reg [15:0] wb_dat_mosi, input [15:0] wb_dat_miso,
   output reg [1:0] wb_sel_o, output wb_cyc_o, output reg wb_stb_o, output reg wb_we_o, input wb_ack_i
   );

   wire 	EM_output_enable = (~EM_NOE & (~EM_NCS4 | ~EM_NCS6));
   wire [15:0] 	EM_D_ram;
   wire [15:0] 	EM_D_wb;

   assign EM_D = ~EM_output_enable ? 16'bz : ~EM_NCS4 ? EM_D_ram : EM_D_wb;

   // CS4 is RAM_2PORT for high-speed data
   ram_2port #(.DWIDTH(16), .AWIDTH(10)) ram_2port
     (.clka(wb_clk), .ena(~EM_NCS4), .wea(~EM_NWE), .addra(EM_A), .dia(EM_D), .doa(EM_D_ram),
      .clkb(wb_clk), .enb(0), .web(0), .addrb(0), .dib(0), .dob());

   // CS6 is Control, Wishbone bus bridge (wb master)
   // Sync version
   reg [1:0] 	cs_del, we_del, oe_del;

   // Synchronize the async control signals
   always @(posedge wb_clk)
     begin
	cs_del <= { cs_del[0], EM_NCS6 };
	we_del <= { we_del[0], EM_NWE };
	oe_del <= { oe_del[0], EM_NOE };
     end

   always @(posedge wb_clk)
     if(cs_del == 2'b10)  // Falling Edge
       wb_adr_o <= { EM_A, 1'b0 };

   always @(posedge wb_clk)
     if(we_del == 2'b10)  // Falling Edge
       begin
	  wb_dat_mosi <= EM_D;
	  wb_sel_o <= ~EM_NBE;
       end

   reg [15:0] EM_D_wb_reg;
   always @(posedge wb_clk)
     if(wb_ack_i)
       EM_D_wb_reg <= wb_dat_miso;

   assign EM_D_wb = wb_ack_i ? wb_dat_miso : EM_D_wb_reg;
   
   // stb, oe_del, we_del
   assign wb_cyc_o = wb_stb_o;

   always @(posedge wb_clk)
     if( ~cs_del[0] & (we_del == 2'b10) )
       wb_we_o <= 1;
     else if(wb_ack_i)  // Turn off we when done.  Could also use we_del[0], others...
       wb_we_o <= 0;

   always @(posedge wb_clk)
     if( ~cs_del[0] & ((we_del == 2'b10) | (oe_del == 2'b10)))
       wb_stb_o <= 1;
     else if(wb_ack_i)
       wb_stb_o <= 0;
   
endmodule // gpmc

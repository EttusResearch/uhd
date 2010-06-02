`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////

//`define DCM 1

module u1e
  (input CLK_FPGA_P, input CLK_FPGA_N,  // Diff
   output [2:0] debug_led, output [31:0] debug, output [1:0] debug_clk,
   input [2:0] debug_pb, input [7:0] dip_sw, output FPGA_TXD, input FPGA_RXD,

   // GPMC
   input EM_CLK, inout [15:0] EM_D, input [10:1] EM_A, input [1:0] EM_NBE,
   input EM_WAIT0, input EM_NCS4, input EM_NCS6, input EM_NWE, input EM_NOE,

   inout db_sda, inout db_scl, // I2C

   output db_sclk_tx, output db_sen_tx, output db_mosi_tx, input db_miso_tx,   // DB TX SPI
   output db_sclk_rx, output db_sen_rx, output db_mosi_rx, input db_miso_rx,   // DB TX SPI
   output sclk_codec, output sen_codec, output mosi_codec, input miso_codec,   // AD9862 main SPI
   output cgen_sclk, output cgen_sen_b, output cgen_mosi, input cgen_miso,     // Clock gen SPI

   input cgen_st_status, input cgen_st_ld, input cgen_st_refmon, output cgen_sync_b, output cgen_ref_sel,
   
   output overo_gpio144, output overo_gpio145, output overo_gpio146, output overo_gpio147,  // Fifo controls
   input overo_gpio0, input overo_gpio14, input overo_gpio21, input overo_gpio22,  // Misc GPIO
   input overo_gpio23, input overo_gpio64, input overo_gpio65, input overo_gpio127, // Misc GPIO
   input overo_gpio128, input overo_gpio163, input overo_gpio170, input overo_gpio176, // Misc GPIO
   
   inout [15:0] io_tx, inout [15:0] io_rx,

   output [13:0] TX, output TXSYNC, output TXBLANK,
   input [11:0] DA, input [11:0] DB, input RXSYNC,
  
   input PPS_IN
   );

   // /////////////////////////////////////////////////////////////////////////
   // Clocking
   wire  clk_fpga, clk_fpga_in;
   
   IBUFGDS #(.IOSTANDARD("LVDS_33"), .DIFF_TERM("TRUE")) 
   clk_fpga_pin (.O(clk_fpga_in),.I(CLK_FPGA_P),.IB(CLK_FPGA_N));

`ifdef DCM
   wire  clk_2x, dcm_rst, dcm_locked;
   DCM #(.CLK_FEEDBACK ( "1X" ),
	 .CLKDV_DIVIDE ( 2.0 ),
	 .CLKFX_DIVIDE ( 1 ),
	 .CLKFX_MULTIPLY ( 2 ),
	 .CLKIN_DIVIDE_BY_2 ( "FALSE" ),
	 .CLKIN_PERIOD ( 15.625 ),
	 .CLKOUT_PHASE_SHIFT ( "NONE" ),
	 .DESKEW_ADJUST ( "SYSTEM_SYNCHRONOUS" ),
	 .DFS_FREQUENCY_MODE ( "LOW" ),
	 .DLL_FREQUENCY_MODE ( "LOW" ),
	 .DUTY_CYCLE_CORRECTION ( "TRUE" ),
	 .FACTORY_JF ( 16'h8080 ),
	 .PHASE_SHIFT ( 0 ),
	 .STARTUP_WAIT ( "FALSE" ))
   clk_doubler (.CLKFB(clk_fpga), .CLKIN(clk_fpga_in), .RST(dcm_rst), 
                .DSSEN(0), .PSCLK(0), .PSEN(0), .PSINCDEC(0), .PSDONE(), 
		.CLKDV(), .CLKFX(), .CLKFX180(), 
                .CLK2X(clk_2x), .CLK2X180(), 
                .CLK0(clk_fpga), .CLK90(), .CLK180(), .CLK270(), 
                .LOCKED(dcm_locked), .STATUS());
`else // !`ifdef DCM
   BUFG clk_fpga_BUFG (.I(clk_fpga_in), .O(clk_fpga));
`endif // !`ifdef DCM
   
   // /////////////////////////////////////////////////////////////////////////
   // SPI
   wire  mosi, sclk, miso;
   assign { db_sclk_tx, db_mosi_tx } = ~db_sen_tx ? {sclk,mosi} : 2'b0;
   assign { db_sclk_rx, db_mosi_rx } = ~db_sen_rx ? {sclk,mosi} : 2'b0;
   assign { sclk_codec, mosi_codec } = ~sen_codec ? {sclk,mosi} : 2'b0;
   assign { cgen_sclk, cgen_mosi } = ~cgen_sen_b ? {sclk,mosi} : 2'b0;
   assign miso = (~db_sen_tx & db_miso_tx) | (~db_sen_rx & db_miso_rx) |
		 (~sen_codec & miso_codec) | (~cgen_sen_b & cgen_miso);

   // /////////////////////////////////////////////////////////////////////////
   // TX DAC -- handle the interleaved data bus to DAC, with clock doubling DLL

   assign TXBLANK = 0;
   wire [13:0] tx_i, tx_q;

`ifdef DCM
   reg [13:0]  TX;
   reg 	       TXSYNC;
   
   always @(posedge clk_2x)
     if(clk_fpga)
       begin
	  TX <= tx_i;
	  TXSYNC <= 0;  // Low indicates first data item
       end
     else
       begin
	  TX <= tx_q;
	  TXSYNC <= 1;
       end
`else // !`ifdef DCM
   genvar i;
   generate
      for(i=0;i<14;i=i+1)
	begin : gen_dacout
	   ODDR2 #(.DDR_ALIGNMENT("NONE"), // Sets output alignment to "NONE", "C0" or "C1" 
		   .INIT(1'b0),            // Sets initial state of the Q output to 1'b0 or 1'b1
		   .SRTYPE("SYNC"))        // Specifies "SYNC" or "ASYNC" set/reset
	   ODDR2_inst (.Q(TX[i]),      // 1-bit DDR output data
		       .C0(clk_fpga),  // 1-bit clock input
		       .C1(~clk_fpga), // 1-bit clock input
		       .CE(1'b1),      // 1-bit clock enable input
		       .D0(tx_i[i]),   // 1-bit data input (associated with C0)
		       .D1(tx_q[i]),   // 1-bit data input (associated with C1)
		       .R(1'b0),       // 1-bit reset input
		       .S(1'b0));      // 1-bit set input
	end // block: gen_dacout
      endgenerate
   ODDR2 #(.DDR_ALIGNMENT("NONE"), // Sets output alignment to "NONE", "C0" or "C1" 
	   .INIT(1'b0),            // Sets initial state of the Q output to 1'b0 or 1'b1
	   .SRTYPE("SYNC"))        // Specifies "SYNC" or "ASYNC" set/reset
   ODDR2_txsnc (.Q(TXSYNC),      // 1-bit DDR output data
		.C0(clk_fpga),  // 1-bit clock input
		.C1(~clk_fpga), // 1-bit clock input
		.CE(1'b1),      // 1-bit clock enable input
		.D0(1'b0),   // 1-bit data input (associated with C0)
		.D1(1'b1),   // 1-bit data input (associated with C1)
		.R(1'b0),       // 1-bit reset input
		.S(1'b0));      // 1-bit set input
   
`endif // !`ifdef DCM
   
   // /////////////////////////////////////////////////////////////////////////
   // Main U1E Core
   u1e_core u1e_core(.clk_fpga(clk_fpga), .rst_fpga(~debug_pb[2]),
		     .debug_led(debug_led), .debug(debug), .debug_clk(debug_clk),
		     .debug_pb(~debug_pb), .dip_sw(dip_sw), .debug_txd(FPGA_TXD), .debug_rxd(FPGA_RXD),
		     .EM_CLK(EM_CLK), .EM_D(EM_D), .EM_A(EM_A), .EM_NBE(EM_NBE),
		     .EM_WAIT0(EM_WAIT0), .EM_NCS4(EM_NCS4), .EM_NCS6(EM_NCS6), 
		     .EM_NWE(EM_NWE), .EM_NOE(EM_NOE),
		     .db_sda(db_sda), .db_scl(db_scl),
		     .sclk(sclk), .sen({cgen_sen_b,sen_codec,db_sen_tx,db_sen_rx}), .mosi(mosi), .miso(miso),
		     .cgen_st_status(cgen_st_status), .cgen_st_ld(cgen_st_ld),.cgen_st_refmon(cgen_st_refmon), 
		     .cgen_sync_b(cgen_sync_b), .cgen_ref_sel(cgen_ref_sel),
		     .tx_have_space(overo_gpio144), .tx_underrun(overo_gpio145),
		     .rx_have_data(overo_gpio146), .rx_overrun(overo_gpio147),
		     .io_tx(io_tx), .io_rx(io_rx),
		     .tx_i(tx_i), .tx_q(tx_q), 
		     .rx_i(DA), .rx_q(DB),
		     .misc_gpio( {{overo_gpio128,overo_gpio163,overo_gpio170,overo_gpio176},
				  {overo_gpio0,overo_gpio14,overo_gpio21,overo_gpio22},
				  {overo_gpio23,overo_gpio64,overo_gpio65,overo_gpio127}}),
		     .pps_in(PPS_IN) );

   // /////////////////////////////////////////////////////////////////////////
   // Local Debug
   // assign debug_clk = {clk_fpga, clk_2x };
   // assign debug = { TXSYNC, TXBLANK, TX };
   
endmodule // u1e

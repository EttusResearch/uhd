

module gen_ddrlvds
  (
   // 1X Radio Clock
   input tx_clk_1x,
   // 2X Radio clock
   input tx_clk_2x,
   // Clk to drive DCI ODDR. This is a phase shifted version of
   // tx_clk_2x. The phase shift is to center the DCI edge in the
   // valid window of the data in the DAC.
   input tx_dci_clk,
   // Reset signal synchronous to radio clock
   input reset,
   // Source synchronous differential clocks to DAC
   output tx_clk_2x_p, 
   output tx_clk_2x_n,
   // Differential frame sync to DAC
   output tx_frame_p, 
   output tx_frame_n,
   // Differential byte wide data to DAC.
   // Alternates I[15:8],I[7:0],Q[15:8],Q[7:0]
   output [7:0] tx_d_p, 
   output [7:0] tx_d_n,
   // Input data
   input [15:0] i, 
   input [15:0] q,
   // Rising edge sampled on sync_dacs triggers frame sync sequence
   input sync_dacs
   );

   reg [15:0] 	i_reg, q_reg;
   reg [15:0] 	i_2x, q_2x;
   reg 		rising_edge;
   wire [15:0] 	i_and_q_2x;
   reg 		sync_2x;
   
   genvar 	z;
   wire [7:0] 	tx_int;
   wire 	tx_clk_2x_int;
   wire 	tx_frame_int;

   // Keep constraint to ensure these signals are not resource shared which can cause timing failures
   (* keep = "true" *) reg phase, phase_2x, sync_dacs_reg;

   wire phase_eq_phase2x = (phase == phase_2x);

   always @(posedge tx_clk_1x)
     if (reset)
       phase <= 1'b0;
     else
       phase <= ~phase;


   //
   // Pipeline input data so that 1x to 2x clock domain jump includes no logic external to this module.
   //
   always @(posedge tx_clk_1x) 
     begin
	i_reg <= i;
	q_reg <= q;
	sync_dacs_reg <= sync_dacs;
     end  

   always @(posedge tx_clk_2x)
     begin
	// Move 1x data to 2x domain, mostly just to add pipeline regs
	// for timing closure.
	i_2x <= i_reg;
	q_2x <= q_reg;
	// Sample phase to determine when 1x clock edges occur.
	// To sync multiple AD9146 DAC's an extended assertion of FRAME is required,
	// when sync flag set, squash one rising_edge assertion which causes a 3 word assertion of FRAME,
	// also reset sync flag. "sync_dacs" comes from 1x clk and pulse lasts 2 2x clock cycles...this is accounted for.
	sync_2x <=  (phase_eq_phase2x && sync_2x) ? 1'b0 /*RESET */ : (sync_dacs_reg) ? 1'b1 /* SET */ : sync_2x /* HOLD */;
	rising_edge <= (phase_eq_phase2x && ~sync_2x);
	phase_2x <= phase;
     end

   // Interleave I and Q as SDR signals
   assign i_and_q_2x = rising_edge ? q_2x : i_2x;
   
   generate	
      for(z = 0; z < 8; z = z + 1)
	begin : gen_pins
	   OBUFDS obufds (.I(tx_int[z]), .O(tx_d_p[z]), .OB(tx_d_n[z]));
	   ODDR #(.DDR_CLK_EDGE("SAME_EDGE")) oddr
	     (.Q(tx_int[z]), .C(tx_clk_2x),
	      .CE(1'b1), .D1(i_and_q_2x[z+8]), .D2(i_and_q_2x[z]), .S(1'b0), .R(1'b0));
	end
   endgenerate

   // Generate framing signal to identify I and Q
   OBUFDS obufds_frame (.I(tx_frame_int), .O(tx_frame_p), .OB(tx_frame_n));
   ODDR #(.DDR_CLK_EDGE("SAME_EDGE")) oddr_frame
     (.Q(tx_frame_int), .C(tx_clk_2x),
      .CE(1'b1), .D1(~rising_edge), .D2(~rising_edge), .S(1'b0), .R(1'b0));

   // Source synchronous clk
   OBUFDS obufds_clk (.I(tx_clk_2x_int), .O(tx_clk_2x_p), .OB(tx_clk_2x_n));
   ODDR #(.DDR_CLK_EDGE("SAME_EDGE")) oddr_clk
     (.Q(tx_clk_2x_int), .C(tx_dci_clk),
      .CE(1'b1), .D1(1'b1), .D2(1'b0), .S(1'b0), .R(1'b0));

endmodule // gen_ddrlvds


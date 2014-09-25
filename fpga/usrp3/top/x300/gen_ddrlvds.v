

module gen_ddrlvds (
   // 2X Radio clock
   input tx_clk_2x,
   // 1X Radio Clock
   input tx_clk_1x,
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

   localparam SYNC_PULSE_WIDTH = 3'd2;

   //
   // Figure out the 1X clock level
   //
   localparam TX_CLK_1X_LOW   = 1'b0;
   localparam TX_CLK_1X_HIGH  = 1'b1;

   reg   tx_clk_1x_level;
   reg   phase, phase_2x;
   reg   reset_2x;

   always @(posedge tx_clk_1x)
      if (reset)
         phase <= 1'b0;
      else
         phase <= ~phase;

   always @(posedge tx_clk_2x)
   begin
      phase_2x <= phase;
      //Pipeline reset and tx_clk_1x_level
      reset_2x <= reset;
      tx_clk_1x_level <= (phase == phase_2x) ? TX_CLK_1X_HIGH : TX_CLK_1X_LOW;
   end

   //
   // Pipeline input data so that 1x to 2x clock domain jump includes no logic external to this module.
   //
   reg [15:0]  i_reg, q_reg;
   reg         sync_dacs_reg;

   always @(posedge tx_clk_1x) 
   begin
      i_reg <= i;
      q_reg <= q;
      sync_dacs_reg <= sync_dacs;
   end  

   //
   // Generate frame signal and interleave I and Q signals
   //
   reg [15:0]  i_2x, q_2x;
   reg [2:0]   sync_count;
   reg         frame;

   always @(posedge tx_clk_2x)
   begin
      // Move 1x data to 2x domain, mostly just to add pipeline regs
      // for timing closure.
      i_2x <= i_reg;
      q_2x <= q_reg;
      
      // Sample phase to determine when 1x clock edges occur.
      // To sync multiple AD9146 DAC's an extended assertion of FRAME is required,
      // when sync flag set, squash one frame assertion which causes a SYNC_PULSE_WIDTH+1 word assertion of FRAME,
      // also reset sync flag. "sync_dacs" comes from 1x clk and pulse lasts 2 2x clock cycles...this is accounted for.
      if (reset_2x) begin
         frame <= 0;
         sync_count <= 3'd0;
      end else begin
         frame <= (tx_clk_1x_level == TX_CLK_1X_LOW) | (sync_count != 3'd0);
         if ((tx_clk_1x_level == TX_CLK_1X_LOW) & sync_dacs_reg)
            sync_count <= SYNC_PULSE_WIDTH;
         else if (sync_count > 3'd0)
            sync_count <= sync_count - 3'd1;
      end
   end

   wire [15:0] i_and_q_2x = frame ? i_2x : q_2x;
   
   //
   // Instantiate IO primitives for the source synchronous interface
   //
   wire [7:0]  tx_int;
   wire        tx_clk_2x_int;
   wire        tx_frame_int;

   genvar z;
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
      .CE(1'b1), .D1(frame), .D2(frame), .S(1'b0), .R(1'b0));

   // Source synchronous clk
   OBUFDS obufds_clk (.I(tx_clk_2x_int), .O(tx_clk_2x_p), .OB(tx_clk_2x_n));
   ODDR #(.DDR_CLK_EDGE("SAME_EDGE")) oddr_clk
     (.Q(tx_clk_2x_int), .C(tx_clk_2x),
      .CE(1'b1), .D1(1'b1), .D2(1'b0), .S(1'b0), .R(1'b0));

endmodule // gen_ddrlvds


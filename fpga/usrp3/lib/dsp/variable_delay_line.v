//
// Copyright 2018 Ettus Research, A National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: variable_delay_line
// Description: 
//   This module implements a variable length delay line. It can be used
//   in filter implementation where the delay is either variable and/or
//   longer than a few flip-flops
//
// Parameters:
//   - WIDTH: Width of data_in and data_out
//   - DYNAMIC_DELAY: Is the delay variable (configurable at runtime)
//   - DEPTH: The depth of the delay line. Must be greater than 2.
//            The output delay can be between 0 and DEPTH-1.
//            If DYNAMIC_DELAY==0, then this is the static delay
//   - DEFAULT_DATA: Data to output if time post-delay is negative 
//   - OUT_REG: Add an output register. This adds a cycle of latency
//   - DEVICE: FPGA device family
// Signals:
//   - data_in  : Input sample value
//   - stb_in   : Is input sample valid?
//   - delay    : Delay value for output (Must be between 0 and DEPTH-1)
//   - data_out : Output sample value. data_out is updated 1 clock
//                cycle (2 if OUT_REG == 1) after assertion of delay
//

module variable_delay_line #(
  parameter             WIDTH         = 18,
  parameter             DEPTH         = 256,
  parameter             DYNAMIC_DELAY = 0,
  parameter [WIDTH-1:0] DEFAULT_DATA  = 0,
  parameter             OUT_REG       = 0,
  parameter             DEVICE        = "7SERIES"
) (
  input  wire                     clk,
  input  wire                     clk_en,
  input  wire                     reset,
  input  wire [WIDTH-1:0]         data_in,
  input  wire                     stb_in,
  input  wire [$clog2(DEPTH)-1:0] delay,
  output wire [WIDTH-1:0]         data_out
);
   //FIXME: Change to localparam when Vivado doesn't freak out
   //       about the use of clog2.
  parameter  ADDR_W = $clog2(DEPTH+1);
  localparam DATA_W = WIDTH;

  //-----------------------------------------------------------
  // RAM State Machine: FIFO write, random access read
  //-----------------------------------------------------------
  wire              w_en;
  wire [DATA_W-1:0] r_data, w_data;
  wire [ADDR_W-1:0] r_addr;
  reg  [ADDR_W-1:0] w_addr = {ADDR_W{1'b0}}, occupied = {ADDR_W{1'b0}};
  reg  [1:0]        use_default = 2'b11;

  // FIFO write, random access read
  always @(posedge clk) begin
    if (reset) begin
      w_addr <= {ADDR_W{1'b0}};
      occupied <= {ADDR_W{1'b0}};
    end else if (w_en) begin
      w_addr <= w_addr + 1'b1;
      if (occupied != DEPTH) begin
        occupied <= occupied + 1'b1;
      end
    end
  end

  // Logic to handle negative delays
  always @(posedge clk) begin
    if (reset) begin
      use_default <= 2'b11;
    end else if (clk_en && (occupied != 0)) begin
      use_default <= {use_default[0], (r_addr >= occupied ? 1'b1 : 1'b0)};
    end
  end

  assign w_en     = stb_in & clk_en;
  assign w_data   = data_in;
  assign r_addr   = (DYNAMIC_DELAY == 0) ? DEPTH : delay;
  assign data_out = use_default[OUT_REG] ? DEFAULT_DATA : r_data;

  //-----------------------------------------------------------
  // Delay Line RAM Implementation
  //-----------------------------------------------------------
  // Use a delay line implementation based on the depth. 
  // The DEVICE parameter is passed in but SPARTAN6,
  // 7Series, Ultrascale and Ultrascale+ have the same 
  // MACROs for SRLs so we don't use the param quite yet.

  genvar i;
  generate
    if (ADDR_W == 4 || ADDR_W == 5) begin
      // SRLs don't have an output register to instantiate
      // that plus the pipeline register manually
      wire [DATA_W-1:0] r_data_srl;
      reg  [DATA_W-1:0] r_data_shreg[0:1];
      always @(posedge clk) begin
        if (clk_en)
          {r_data_shreg[1], r_data_shreg[0]} <= {r_data_shreg[0], r_data_srl};
      end
      assign r_data = r_data_shreg[OUT_REG];

      for (i = 0; i < DATA_W; i = i + 1) begin: bits
        // Pick SRL based on address width
        if (ADDR_W == 4) begin
          SRL16E #(
            .INIT(16'h0000), .IS_CLK_INVERTED(1'b0)
          ) srl16e_i (
            .CLK(clk), .CE(w_en),
            .D(w_data[i]),
            .A0(r_addr[0]),.A1(r_addr[1]),.A2(r_addr[2]),.A3(r_addr[3]),
            .Q(r_data_srl[i])
          );
        end else begin
          SRLC32E #(
            .INIT(32'h00000000), .IS_CLK_INVERTED(1'b0)
          ) srlc32e_i (
            .CLK(clk), .CE(w_en),
            .D(w_data[i]),
            .A(r_addr),
            .Q(r_data_srl[i]), .Q31()
         );
        end
      end
    end else begin
      // For ADDR_W < 4, the RAM should ideally get 
      // synthesized down to flip-flops.
      ram_2port #(
        .DWIDTH (DATA_W), .AWIDTH(ADDR_W),
        .RW_MODE("NO-CHANGE"), .OUT_REG(OUT_REG)
      ) ram_i (
        .clka (clk), .ena(clk_en), .wea(w_en),
        .addra(w_addr), .dia(w_data), .doa(),
        .clkb (clk), .enb(clk_en), .web(1'b0),
        .addrb(w_addr - r_addr - 1), .dib(), .dob(r_data)
      );
    end
  endgenerate

endmodule // delay_line
//
// Copyright 2015 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
//
// Synthesizable test pattern checker
//

module cap_pattern_verifier #(
  parameter WIDTH       = 16,       //Width of data bus
  parameter PATTERN     = "RAMP",   //Pattern to detect. Choose from {RAMP, ONES, ZEROS, TOGGLE, LEFT_BARREL, RIGHT_BARREL}
  parameter RAMP_START  = 'h0000,   //Start value for ramp (PATTERN=RAMP only)
  parameter RAMP_STOP   = 'hFFFF,   //Stop value for ramp (PATTERN=RAMP only)
  parameter RAMP_INCR   = 'h0001,   //Increment for ramp (PATTERN=RAMP only)
  parameter BARREL_INIT = 'h0001,   //Initial value for the barrel shifter (PATTERN=*_BARREL only)
  parameter HOLD_CYCLES = 1         //Number of cycles to hold each value in the pattern
) (
  input             clk,
  input             rst,

  //Data input
  input             valid,
  input [WIDTH-1:0] data,

  //Status output (2 cycle latency)
  output reg [31:0] count,
  output reg [31:0] errors,
  output            locked,
  output            failed
);

  //Create a synchronous version of rst
  wire sync_rst;
  reset_sync reset_sync_i (
    .clk(clk), .reset_in(rst), .reset_out(sync_rst));

  // Register the data to minimize fanout at source
  reg [WIDTH-1:0] data_reg;
  reg             valid_reg;
  always @(posedge clk)
    {data_reg, valid_reg} <= {data, valid};

  // Define pattern start and next states
  wire [WIDTH-1:0]  patt_start, patt_next;
  reg [WIDTH-1:0]   patt_next_reg;
  generate if (PATTERN == "RAMP") begin
    assign patt_start = RAMP_START;
    assign patt_next  = (data_reg==RAMP_STOP) ? RAMP_START : data_reg+RAMP_INCR;
  end else if (PATTERN == "ZEROS") begin
    assign patt_start = {WIDTH{1'b0}};
    assign patt_next  = {WIDTH{1'b0}};
  end else if (PATTERN == "ONES") begin
    assign patt_start = {WIDTH{1'b1}};
    assign patt_next  = {WIDTH{1'b1}};
  end else if (PATTERN == "TOGGLE") begin
    assign patt_start = {(WIDTH/2){2'b10}};
    assign patt_next  = ~data_reg;
  end else if (PATTERN == "LEFT_BARREL") begin
    assign patt_start = BARREL_INIT;
    assign patt_next  = {data_reg[WIDTH-2:0],data_reg[WIDTH-1]};
  end else if (PATTERN == "RIGHT_BARREL") begin
    assign patt_start = BARREL_INIT;
    assign patt_next  = {data_reg[0],data_reg[WIDTH-1:1]};
  end endgenerate

  reg [1:0] state;
  localparam ST_IDLE    = 2'd0;
  localparam ST_LOCKED  = 2'd1;

  reg [7:0] cyc_count;

  //All registers in this state machine need to have an
  //asynchronous reset because the "data" and "valid" can
  //be metastable coming into this module, and can possibly
  //corrupt "state".
  always @(posedge clk or posedge rst) begin
    if (rst) begin            //Asynchronous reset
      count         <= 32'd0;
      errors        <= 32'd0;
      state         <= ST_IDLE;
      cyc_count     <= 8'd0;
      patt_next_reg <= {WIDTH{1'b0}};
    end else begin
      //Only do something if data is valid
      if (valid_reg & ~sync_rst) begin
        case (state)
          ST_IDLE: begin
            //Trigger on start of pattern
            //We use a case equality here to ensure that this module
            //does the right thing in simulation. In HW this should
            //infer a "=="
            if (data_reg === patt_start) begin
              state     <= ST_LOCKED;
              count     <= 32'd1;
              cyc_count <= HOLD_CYCLES - 1;
            end
          end
          ST_LOCKED: begin
            if (cyc_count == 0) begin           //Hold counter has expired. Check next word
              count <= count + 32'd1;
              //We use a case equality here to ensure that this module
              //does the right thing in simulation. In HW this should
              //infer a "!="
              if (data_reg !== patt_next_reg) begin
                errors <= errors + 32'd1;
              end
              cyc_count <= HOLD_CYCLES - 1;
            end else begin                      //Hold until the next update
              cyc_count <= cyc_count - 1;
            end
          end
        endcase
        patt_next_reg <= patt_next;             //Update next pattern
      end
    end
  end  

  assign locked = (state == ST_LOCKED);
  assign failed = (errors != 32'd0) && locked;

endmodule




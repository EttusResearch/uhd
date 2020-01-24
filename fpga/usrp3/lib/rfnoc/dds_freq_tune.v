//
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// DDS frequency shift with complex multiply

module dds_freq_tune  #(
  parameter WIDTH = 24,
  parameter PHASE_WIDTH = 24,
  parameter SIN_COS_WIDTH = 16,
  parameter OUTPUT_WIDTH = 24
)(
  input         clk,
  input         reset,
  input         eob,
  input         rate_changed,
  input [15:0]  dds_input_fifo_occupied,
  /* IQ input */
  input [WIDTH*2-1:0]  s_axis_din_tdata,
  input         s_axis_din_tlast,
  input         s_axis_din_tvalid,
  output        s_axis_din_tready,
  /* Phase input from NCO */
  input [PHASE_WIDTH-1:0]  s_axis_phase_tdata,
  input         s_axis_phase_tlast,
  input         s_axis_phase_tvalid,
  output        s_axis_phase_tready,
  /* IQ output */
  output [OUTPUT_WIDTH*2-1:0] m_axis_dout_tdata,
  output        m_axis_dout_tlast,
  output        m_axis_dout_tvalid,
  input         m_axis_dout_tready,

  //debug signals
  output [2:0] state_out,
  output phase_valid_hold_out,
  output [7:0] phase_invalid_wait_count_out,
  output reset_dds_out,
  output m_axis_dds_tlast_out,
  output m_axis_dds_tvalid_out,
  output m_axis_dds_tready_out,
  output [SIN_COS_WIDTH*2-1:0] m_axis_dds_tdata_out //[31:16] = sin|q [15:0] cos|i
);

  //wires for dds output
  wire m_axis_dds_tlast;
  wire m_axis_dds_tvalid;
  wire m_axis_dds_tready;
  wire [SIN_COS_WIDTH*2-1:0] m_axis_dds_tdata; //[31:16] = sin|q [15:0] cos|i
  reg reset_reg;
  reg phase_valid_hold;
  reg [7:0] phase_invalid_wait_count;
  reg [2:0] state;
  reg reset_dds     = 1'b1;  // Init DDS resets to 1, since simulation model 
  reg reset_dds_reg = 1'b1;  // requires reset at time 0 to avoid failure.
  reg phase_ready_wait;
  wire s_axis_phase_tready_dds;

  //when we're holding valid, make ready low so no new data comes in.
  assign s_axis_phase_tready = s_axis_phase_tready_dds & ~phase_valid_hold;

  localparam INIT = 3'b000;
  localparam VALID = 3'b001;
  localparam WAIT = 3'b010;
  localparam HOLD_VALID = 3'b011;

  //reset needs to be 2 clk cycles minimum for Xilinx DDS IP
  always @(posedge clk) begin
    reset_reg <= reset;
    reset_dds_reg <= reset_dds;
  end

  //some logic to reset the dds when data is goes from valid to not valid
  //also holds valid high until the pipeline has passed tlast through.
  always @(posedge clk) begin
    if(reset) begin
      state <= INIT;
      phase_valid_hold <= 1'b0;
      phase_invalid_wait_count <= 16'h00;
      reset_dds <= 1'b0;
    end
    else begin
      case(state)
        INIT: begin//init case
          phase_valid_hold <= 1'b0;
          phase_invalid_wait_count <= 16'h0000;
          reset_dds <= 1'b0;
          if(s_axis_phase_tvalid) begin
            state <= VALID;
          end
        end
        VALID: begin //valid data
          if(~s_axis_phase_tvalid) begin
            state <= WAIT;
          end
        end
        WAIT: begin //wait until we either get valid data or don't
          if(m_axis_dds_tready) begin //only increment when the downstream can accept data.
            phase_invalid_wait_count <= phase_invalid_wait_count + 4'b1;
          end
          if(s_axis_phase_tvalid) begin //if we get valid data shortly after, then don't push data through and reset
            state <= INIT;
          end else begin
            if(eob | (phase_invalid_wait_count >= 16'h40) | rate_changed ) begin //if a valid never comes, aka eob
              state <= HOLD_VALID;
            end
          end
        end
        HOLD_VALID: begin//hold valid to finish pipeline. Apparently the dds IP won't empty without additional valids.
          phase_valid_hold <= 1'b1;
          // Wait for input FIFO to be empty
          if (~s_axis_din_tvalid) begin
            state <= INIT;
            reset_dds <= 1'b1;
          end
        end
      endcase
    end
  end

  //dds to generate sin/cos data from phase
  dds_sin_cos_lut_only dds_inst (
    .aclk(clk),                                // input wire aclk
    .aresetn(~(reset | reset_reg | reset_dds | reset_dds_reg)),            // input wire aresetn active low rst
    .s_axis_phase_tvalid(s_axis_phase_tvalid | phase_valid_hold),  // input wire s_axis_phase_tvalid
    .s_axis_phase_tready(s_axis_phase_tready_dds),  // output wire s_axis_phase_tready
    .s_axis_phase_tlast(s_axis_phase_tlast),     //tlast
    .s_axis_phase_tdata(s_axis_phase_tdata),    // input wire [23 : 0] s_axis_phase_tdata
    .m_axis_data_tvalid(m_axis_dds_tvalid),    // output wire m_axis_data_tvalid
    .m_axis_data_tready(m_axis_dds_tready),    // input wire m_axis_data_tready
    .m_axis_data_tlast(m_axis_dds_tlast),      // input wire m_axis_data_tready
    .m_axis_data_tdata(m_axis_dds_tdata)      // output wire [31 : 0] m_axis_data_tdata
  );

  wire [WIDTH*2-1:0] mult_in_a_tdata;
  wire mult_in_a_tvalid;
  wire mult_in_a_tready;
  wire mult_in_a_tlast;
  wire [SIN_COS_WIDTH*2-1:0] mult_in_b_tdata;
  wire mult_in_b_tvalid;
  wire mult_in_b_tready;
  wire mult_in_b_tlast; //no connect
  wire [2*32-1:0] mult_out_tdata;
  wire mult_out_tvalid;
  wire mult_out_tready;
  wire mult_out_tlast;

  axi_sync #(
    .SIZE(2),
    .WIDTH_VEC({SIN_COS_WIDTH*2, WIDTH*2}),
    .FIFO_SIZE(0))
  axi_sync (
    .clk(clk), .reset(reset), .clear(),
    .i_tdata({m_axis_dds_tdata,s_axis_din_tdata}),
    .i_tlast({m_axis_dds_tlast,s_axis_din_tlast}),
    .i_tvalid({m_axis_dds_tvalid,s_axis_din_tvalid}),
    .i_tready({m_axis_dds_tready,s_axis_din_tready}),
    .o_tdata({mult_in_b_tdata,mult_in_a_tdata}),
    .o_tlast({mult_in_b_tlast,mult_in_a_tlast}),
    .o_tvalid({mult_in_b_tvalid,mult_in_a_tvalid}),
    .o_tready({mult_in_b_tready,mult_in_a_tready}));

  //a = input i/q data stream 48 bit i/q lower bits i, upper bits q
  //b = output of dds 32 bit cos/sin. lower cos, upper sin
  complex_multiplier_dds complex_mult_inst (
    .aclk(clk),                              // input wire aclk
    .aresetn(~(reset | reset_reg)),                        // input wire aresetn
    .s_axis_a_tvalid(mult_in_a_tvalid),        // input wire s_axis_a_tvalid
    .s_axis_a_tready(mult_in_a_tready),        // output wire s_axis_a_tready
    .s_axis_a_tlast(mult_in_a_tlast),          // input wire s_axis_a_tlast
    .s_axis_a_tdata({mult_in_a_tdata}),          // input wire [47 : 0] s_axis_a_tdata
    .s_axis_b_tvalid(mult_in_b_tvalid),        // input wire s_axis_b_tvalid
    .s_axis_b_tready(mult_in_b_tready),        // output wire s_axis_b_tready
    .s_axis_b_tlast(mult_in_b_tlast),        // output wire s_axis_b_tlast
    .s_axis_b_tdata(mult_in_b_tdata),          // input wire [31 : 0] s_axis_b_tdata
    .m_axis_dout_tvalid(mult_out_tvalid),  // output wire m_axis_dout_tvalid
    .m_axis_dout_tready(mult_out_tready),  // input wire m_axis_dout_tready
    .m_axis_dout_tlast(mult_out_tlast),    // output wire m_axis_dout_tlast
    .m_axis_dout_tdata(mult_out_tdata)    // output wire [63 : 0] m_axis_dout_tdata
  );

  axi_round_complex #(
    .WIDTH_IN(32),
    .WIDTH_OUT(OUTPUT_WIDTH))
  axi_round_complex_inst (
    .clk(clk),
    .reset(reset | reset_reg),
    .i_tdata(mult_out_tdata),
    .i_tlast(mult_out_tlast),
    .i_tvalid(mult_out_tvalid),
    .i_tready(mult_out_tready),
    .o_tdata(m_axis_dout_tdata),
    .o_tlast(m_axis_dout_tlast),
    .o_tvalid(m_axis_dout_tvalid),
    .o_tready(m_axis_dout_tready));

  //debug
  assign state_out = state;
  assign phase_valid_hold_out = phase_valid_hold;
  assign phase_invalid_wait_count_out = phase_invalid_wait_count;
  assign reset_dds_out = reset_dds;
  assign m_axis_dds_tlast_out = m_axis_dds_tlast;
  assign m_axis_dds_tvalid_out = m_axis_dds_tvalid;
  assign m_axis_dds_tready_out = m_axis_dds_tready;
  assign m_axis_dds_tdata_out = m_axis_dds_tdata;

endmodule

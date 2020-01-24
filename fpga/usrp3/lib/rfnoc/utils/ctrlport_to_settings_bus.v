//
// Copyright 2019 Ettus Research, A National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: ctrlport_to_settings_bus
//
// Description: 
//
//   Converts CTRL port interface requests to a user register settings bus 
//   access. This can be used to connect RFNoC block IP settings registers and 
//   user read-back registers to a control port.
//
//   There are a few key differences between control port and the settings bus 
//   that need to be accounted for. 
//
//    * Control port uses byte address whereas the settings bus uses a 
//      word address. 
//    * Control port is 32-bit whereas the settings bus supports 
//      32-bit writes and 64-bit reads.
//    * The settings bus always does both a write and a read for each
//      transaction. If the intent is to read a register, then it writes the
//      address for the read to SR_RB_ADDR. If the intent is to write a
//      register, then the read result is ignored.
//
//   This block handles these differences by allocating a 2048-byte address 
//   space to each settings bus. Each word of the settings bus is treated like 
//   a 64-bit word on an eight-byte boundary. To write to a settings register 
//   N, simply write a 32-bit value to address N*8. To read read-back register 
//   N, simply perform a 32-bit read from address N*8 followed by a 32-bit read 
//   from address N*8+4 to get the full 64-bits. If only the lower 32-bits are 
//   needed then it is not necessary to read the upper 32 bits. Note however, 
//   that software must always read the lower 32-bits before trying to read the 
//   upper 32-bits and that these reads should be atomic (no intervening reads 
//   should occur).
//
// Parameters:
//
//   NUM_PORTS  : The number of settings buses you wish to connect 
//
//   SR_RB_ADDR : Address to use for the settings register that holds the
//                read-back address. Set to 124 to model register access to
//                user logic registers. Set to 127 to model access to internal
//                NoC shell registers.
//
//   USE_TIME   : When 0, timestamps are simply passed from ctrlport to
//                settings bus and the timestamp input is not used. When 1,
//                this block will wait until the indicated time to arrive on
//                the timestamp input before issuing the transaction on the
//                settings bus. In this case, the time must be provided on the
//                timestamp input.
//

module ctrlport_to_settings_bus #(
  parameter NUM_PORTS  = 1,
  parameter SR_RB_ADDR = 124,
  parameter USE_TIME   = 0
) (
  input wire ctrlport_clk,
  input wire ctrlport_rst,

  //---------------------------------------------------------------------------
  // CTRL Port Interface
  //---------------------------------------------------------------------------

  input  wire        s_ctrlport_req_wr,
  input  wire        s_ctrlport_req_rd,
  input  wire [19:0] s_ctrlport_req_addr,
  input  wire [31:0] s_ctrlport_req_data,
  input  wire        s_ctrlport_req_has_time,
  input  wire [63:0] s_ctrlport_req_time,
  output reg         s_ctrlport_resp_ack = 0,
  output reg  [31:0] s_ctrlport_resp_data,

  //---------------------------------------------------------------------------
  // Settings Bus Interface
  //---------------------------------------------------------------------------

  output wire [NUM_PORTS*32-1:0] set_data,
  output wire [ NUM_PORTS*8-1:0] set_addr,
  output reg  [   NUM_PORTS-1:0] set_stb = 0,
  output wire [NUM_PORTS*64-1:0] set_time,
  output wire [   NUM_PORTS-1:0] set_has_time,

  input  [NUM_PORTS-1:0]    rb_stb,
  output [NUM_PORTS*8-1:0]  rb_addr,
  input  [NUM_PORTS*64-1:0] rb_data,

  //---------------------------------------------------------------------------
  // Timestamp
  //---------------------------------------------------------------------------

  // Current timestamp, synchronous to ctrlport_clk
  input wire [63:0] timestamp
);

  localparam PORT_W = (NUM_PORTS > 1) ? $clog2(NUM_PORTS) : 1;

  wire [PORT_W-1:0] port_num;
  reg  [PORT_W-1:0] port_num_reg;

  wire msw_access;

  reg [31:0] set_data_reg;
  reg [ 7:0] set_addr_reg;
  reg [63:0] set_time_reg;
  reg        set_has_time_reg;
  reg [ 7:0] rb_addr_reg;

  reg [31:0] upper_word;

  // Extract the port index from the address (the bits above the lower 11 bits)
  assign port_num = (NUM_PORTS > 1) ? s_ctrlport_req_addr[(PORT_W+11)-1:11] : 0;

  // Determine if the upper word is being accessed
  assign msw_access = s_ctrlport_req_addr[2];

  localparam ST_IDLE        = 0;
  localparam ST_TIME_CHECK  = 1;
  localparam ST_STROBE_WAIT = 2;
  localparam ST_WAIT_RESP   = 3;

  reg [2:0] state = ST_IDLE;


  always @(posedge ctrlport_clk) begin
    if (ctrlport_rst) begin
      s_ctrlport_resp_ack  <= 0;
      set_stb              <= 0;
      state                <= ST_IDLE;
      s_ctrlport_resp_data <= 32'hX;
      set_addr_reg         <= 8'hX;
      rb_addr_reg          <= 8'hX;
      port_num_reg         <= 8'hX;
      upper_word           <= 32'hX;
    end else begin
      // Default assignments
      s_ctrlport_resp_ack <= 0;
      set_stb             <= 0;

      case (state)
        ST_IDLE : begin
          if (s_ctrlport_req_rd && port_num < NUM_PORTS) begin
            // Handle register reads (read-back registers)
            if (msw_access) begin
              // Reading the upper word always returns the cached upper-word value 
              // from the previous lower-word read.
              s_ctrlport_resp_ack  <= 1;
              s_ctrlport_resp_data <= upper_word;
            end else begin
              // Handle register reads (read-back registers)
              rb_addr_reg <= s_ctrlport_req_addr[10:3];

              // Read-back of a user register on settings bus is always
              // combined with a write to the SR_RB_ADDR address.
              set_addr_reg     <= SR_RB_ADDR;
              set_data_reg     <= 32'bX;    // CtrlPort has no data in this case
              set_time_reg     <= s_ctrlport_req_time;
              set_has_time_reg <= s_ctrlport_req_has_time;

              // Save which port the read is for so that we only watch for 
              // acknowledgments from that port.
              port_num_reg <= port_num;

              if (USE_TIME) begin
                state <= ST_TIME_CHECK;
              end else begin
                set_stb[port_num] <= 1;
                state             <= ST_STROBE_WAIT;
              end
            end

          end else if (s_ctrlport_req_wr && port_num < NUM_PORTS) begin
            // Handle register writes (settings registers)
            set_addr_reg     <= s_ctrlport_req_addr[10:3];
            set_data_reg     <= s_ctrlport_req_data;
            set_time_reg     <= s_ctrlport_req_time;
            set_has_time_reg <= s_ctrlport_req_has_time;

            // Save which port the write is for so that we only watch for 
            // acknowledgments from that port.
            port_num_reg <= port_num;

            if (USE_TIME) begin
              state <= ST_TIME_CHECK;
            end else begin
              set_stb[port_num] <= 1;
              state             <= ST_STROBE_WAIT;
            end
          end
        end

        ST_TIME_CHECK : begin
          // If the transaction is timed, wait until the time arrives before
          // starting. This state is only reachable if USE_TIME is true.
          if (set_has_time_reg) begin
            if (timestamp >= set_time_reg) begin
              set_stb[port_num_reg] <= 1;
              state                 <= ST_STROBE_WAIT;
            end
          end else begin
            set_stb[port_num_reg] <= 1;
            state                 <= ST_STROBE_WAIT;
          end
        end

        ST_STROBE_WAIT : begin
          // Wait a cycle before checking for a response
          state <= ST_WAIT_RESP;
        end

        ST_WAIT_RESP : begin
          // Wait for read completion on settings bus, acknowledged by rb_stb.
          // The read-back data will be ignored by ctrlport if this is a write.
          upper_word           <= rb_data[(64*port_num_reg + 32) +: 32];
          s_ctrlport_resp_data <= rb_data[(64*port_num_reg +  0) +: 32];
          if (rb_stb[port_num_reg] == 1) begin
            s_ctrlport_resp_ack <= 1;
            state               <= ST_IDLE;
          end
        end
      
      endcase
    end
  end


  genvar i;
  generate
    for (i = 0; i < NUM_PORTS; i = i+1) begin : gen_settings_bus
      // Drive all settings buses with the same values, except the strobe
      assign rb_addr      [ 8*i +:  8] = rb_addr_reg;
      assign set_data     [32*i +: 32] = set_data_reg;
      assign set_addr     [ 8*i +:  8] = set_addr_reg;
      assign set_time     [64*i +: 64] = set_time_reg;
      assign set_has_time [         i] = set_has_time_reg;
    end
  endgenerate


endmodule

//
// Copyright 2024 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: axis_cp_list
//
// Description:
//
//   This module maintains a list that can be used to implement storing the
//   cyclic prefix list. Items can be loaded into the list by writing them to
//   the input port. As soon as items are in the list, they can be read out on
//   the output port. Items in the list are read out in the order that they
//   were added and the list automatically repeats in a circular manner. Once
//   the list is full, it stops accepting inputs.
//
//   The list can be cleared by asserting clear for one clock cycle, after
//   which you can load and read out a new list.
//
//   There's no arbitration between reading and writing. It's assumed the list
//   is loaded in one step, then read out and used in another. You can do both
//   at the same time, but there's no guarantee about the order in which things
//   complete.
//
//   The output is fairly slow at one output every three clock cycles.
//
// Parameters:
//
//   ADDR_W  : Sets the maximum length of the list, which will be 2**ADDR_W.
//   DATA_W  : Width of the cyclic-prefix length.
//   REPEAT  : 1: Cyclic prefix list repeats. 0: Cyclic prefix list
//             does not repeat, and the default prefix length will
//             be used once the list is completed.
//   DEFAULT : The value that is output when there is no valid data to output.
//

`default_nettype none


module axis_cp_list #(
  int                ADDR_W  = 5,
  int                DATA_W  = 32,
  bit                REPEAT  = 1,
  logic [DATA_W-1:0] DEFAULT = '0
) (
  input  wire               clk,
  input  wire               rst,

  input  wire               clear,

  input  wire  [DATA_W-1:0] i_tdata,
  input  wire               i_tvalid,
  output wire               i_tready,

  output logic [DATA_W-1:0] o_tdata = DEFAULT,
  output logic              o_tvalid,
  input  wire               o_tready,

  output wire  [  ADDR_W:0] occupied
);

  // Make addresses one extra bit wide to double as fullness and to detect the
  // full condition.
  logic [ADDR_W:0] wr_addr;
  logic [ADDR_W:0] rd_addr;
  logic            full;


  //---------------------------------------------------------------------------
  // RAM
  //---------------------------------------------------------------------------

  logic [DATA_W-1:0] rd_data;
  logic              wr_en;

  assign wr_en = i_tvalid && i_tready;

  ram_2port #(
    .DWIDTH (DATA_W),
    .AWIDTH (ADDR_W),
    .OUT_REG(0     )
  ) ram_2port_i (
    .clka (clk               ),
    .ena  ('1                ),
    .wea  (wr_en             ),
    .addra(wr_addr[0+:ADDR_W]),
    .dia  (i_tdata           ),
    .doa  (                  ),
    .clkb (clk               ),
    .enb  ('1                ),
    .web  ('0                ),
    .addrb(rd_addr[0+:ADDR_W]),
    .dib  ('0                ),
    .dob  (rd_data           )
  );


  //---------------------------------------------------------------------------
  // Write Logic
  //---------------------------------------------------------------------------

  assign occupied = wr_addr;
  assign full     = wr_addr[ADDR_W];
  assign i_tready = !full;

  always_ff @(posedge clk) begin
    if (i_tvalid && i_tready) begin
      wr_addr <= wr_addr + 1;
    end

    if (rst || clear) begin
      wr_addr  <= '0;
    end
  end


  //---------------------------------------------------------------------------
  // Read Logic
  //---------------------------------------------------------------------------

  enum logic [1:0] { ST_IDLE, ST_LOAD, ST_OUTPUT } rd_state;

  always_ff @(posedge clk) begin
    case (rd_state)
      ST_IDLE : begin
        // The read is started during this cycle, since rd_addr is valid
        if (rd_addr < occupied) begin
          rd_state <= ST_LOAD;
        end
      end
      ST_LOAD : begin
        // The read is available during this cycle
        o_tvalid <= 1'b1;
        o_tdata  <= rd_data;
        rd_state <= ST_OUTPUT;
      end
      ST_OUTPUT : begin
        // Wait for the output to be captured during this cycle
        if (o_tready) begin
          // Always output default value when there's no data to output. This
          // is important for cyclic-prefix insertion/removal where we want the
          // default to be 0.
          o_tdata  <= DEFAULT;
          o_tvalid <= '0;
          rd_state <= ST_IDLE;

          if (REPEAT && rd_addr == occupied-1) begin
            rd_addr <= '0;
          end else begin
            rd_addr <= rd_addr + 1;
          end
        end
      end
    endcase

    if (rst | clear) begin
      rd_state <= ST_IDLE;
      rd_addr  <= '0;
      o_tdata  <= DEFAULT;
      o_tvalid <= '0;
    end
  end

endmodule : axis_cp_list


`default_nettype wire

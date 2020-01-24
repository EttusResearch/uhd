//
// Copyright 2018 Ettus Research, A National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: kv_map

module kv_map #(
  parameter KEY_WIDTH = 16,
  parameter VAL_WIDTH = 32,
  parameter SIZE      = 6
) (
  // Clock and reset
  input  wire                 clk,
  input  wire                 reset,
  // Insert port
  input  wire                 insert_stb,
  input  wire [KEY_WIDTH-1:0] insert_key,
  input  wire [VAL_WIDTH-1:0] insert_val,
  output wire                 insert_busy,
  // Find port
  input  wire                 find_key_stb,
  input  wire [KEY_WIDTH-1:0] find_key,
  output wire                 find_res_stb,
  output wire                 find_res_match,
  output wire [VAL_WIDTH-1:0] find_res_val,
  // Count
  output reg  [SIZE-1:0]      count = {SIZE{1'b0}}
);

  //-------------------------------------------------
  // Instantiate a CAM and a RAM
  //-------------------------------------------------
  // The CAM serves as a "set" and the RAM serves as a
  // random addressable "array". Using thse two data structures
  // we can build a map. The role of the CAM is to compress
  // the key to an address that can be used to lookup data
  // stored in the RAM

  wire                 cam_wr_en, cam_wr_busy, cam_rd_match;
  wire [SIZE-1:0]      cam_wr_addr, cam_rd_addr;
  wire [KEY_WIDTH-1:0] cam_wr_data, cam_rd_key;

  wire                 ram_wr_en;
  wire [SIZE-1:0]      ram_wr_addr;
  reg  [SIZE-1:0]      ram_rd_addr;
  wire [VAL_WIDTH-1:0] ram_wr_data, ram_rd_data;

  cam #(
    .DATA_WIDTH  (KEY_WIDTH),
    .ADDR_WIDTH  (SIZE),
    .CAM_STYLE   (SIZE > 8 ? "BRAM" : "SRL"),
    .SLICE_WIDTH (SIZE > 8 ? 9 : 5)
  ) cam_i (
    .clk         (clk),
    .rst         (reset),
    .write_addr  (cam_wr_addr),
    .write_data  (cam_wr_data),
    .write_delete(1'b0),
    .write_enable(cam_wr_en),
    .write_busy  (cam_wr_busy),
    .compare_data(cam_rd_key),
    .match_addr  (cam_rd_addr),
    .match       (cam_rd_match),
    .match_many  (),
    .match_single()
  );

  ram_2port #(
    .DWIDTH(VAL_WIDTH),
    .AWIDTH(SIZE)
  ) mem_i (
    .clka  (clk),
    .ena   (ram_wr_en),
    .wea   (1'b1),
    .addra (ram_wr_addr),
    .dia   (ram_wr_data),
    .doa   (/* Write port only */),
    .clkb  (clk),
    .enb   (1'b1),
    .web   (1'b0),
    .addrb (ram_rd_addr),
    .dib   (/* Read port only */),
    .dob   (ram_rd_data)
  );

  // Pipeline read address into RAM
  always @(posedge clk)
    ram_rd_addr <= cam_rd_addr;

  //-------------------------------------------------
  // Find state machine
  //-------------------------------------------------
  // The lookup process has three cycles of latency
  // - CAM lookup has a 1 cycle latency
  // - The lookup address into the RAM is delayed by 1 cycle for timing
  // - The RAM takes 1 cycle to produce data

  localparam FIND_CYC = 3;

  reg [FIND_CYC-1:0] find_key_stb_shreg = {FIND_CYC{1'b0}};
  reg [FIND_CYC-2:0] find_match_shreg   = {(FIND_CYC-1){1'b0}};
  reg                find_pending       = 1'b0;

  wire find_busy = find_pending | find_key_stb;

  // Delay the find valid signal to account for the latency 
  // of the CAM and RAM
  always @(posedge clk) begin
    find_key_stb_shreg <= reset ? {FIND_CYC{1'b0}} :
      {find_key_stb_shreg[FIND_CYC-2:0], find_key_stb};
  end
  assign find_res_stb = find_key_stb_shreg[FIND_CYC-1];

  // Latch the find signal to compute pending
  always @(posedge clk) begin
    if (find_key_stb)
      find_pending <= 1'b1;
    else if (find_pending)
      find_pending <= ~find_res_stb;
  end

  // Delay the match signal to account for the latency of the RAM
  always @(posedge clk) begin
    find_match_shreg <= reset ? {(FIND_CYC-1){1'b0}} :
      {find_match_shreg[FIND_CYC-3:0], cam_rd_match};
  end
  assign find_res_match = find_match_shreg[FIND_CYC-2];


  //-------------------------------------------------
  // Insert state machine
  //-------------------------------------------------

  localparam [2:0] ST_IDLE            = 3'd0;
  localparam [2:0] ST_WAIT_FIND       = 3'd1;
  localparam [2:0] ST_CAM_READ        = 3'd2;
  localparam [2:0] ST_CAM_CHECK_MATCH = 3'd3;
  localparam [2:0] ST_CAM_RAM_WRITE   = 3'd4;
  localparam [2:0] ST_CAM_WRITE_WAIT  = 3'd5;
  localparam [2:0] ST_RAM_WRITE       = 3'd6;

  reg [2:0] ins_state = ST_IDLE;

  reg [KEY_WIDTH-1:0] ins_key_cached;
  reg [VAL_WIDTH-1:0] ins_val_cached;
  reg [SIZE-1:0]      write_addr = {SIZE{1'b0}};
  reg [SIZE-1:0]      next_addr  = {SIZE{1'b0}};


  always @(posedge clk) begin
    if (reset) begin
      ins_state <= ST_IDLE;
      next_addr <= {SIZE{1'b0}};
    end else begin
      case (ins_state)

        // Idle and waiting for an insert transaction 
        //
        ST_IDLE: begin
          // Cache insertion parameters
          if (insert_stb) begin
            ins_key_cached <= insert_key;
            ins_val_cached <= insert_val;
            // Wait for find to finish
            ins_state <= find_busy ? ST_WAIT_FIND : ST_CAM_READ;
          end
        end

        // Wait for a find transaction to finish
        //
        ST_WAIT_FIND: begin
          // Wait for find to finish
          if (~find_busy)
            ins_state <= ST_CAM_READ;
        end

        // Read the CAM to check if the key to insert already exists
        //
        ST_CAM_READ: begin
          // Ensure that find always has priority
          if (~find_key_stb)
            ins_state <= ST_CAM_CHECK_MATCH;
        end

        // Look at the CAM match signal to evaluate if we skip writing the CAM
        //
        ST_CAM_CHECK_MATCH: begin
          // If the CAM already has this key, then overwrite it
          if (cam_rd_match) begin
            ins_state <= ST_RAM_WRITE;
            write_addr <= cam_rd_addr;
          end else if (~cam_wr_busy) begin
            ins_state <= ST_CAM_RAM_WRITE;
            write_addr <= next_addr;
            next_addr <= next_addr + 1'b1;
          end
        end

        // Write the specified key to the CAM and value to the RAM
        //
        ST_CAM_RAM_WRITE: begin
          ins_state <= ST_CAM_WRITE_WAIT;
        end

        // Wait for CAM write to finish
        //
        ST_CAM_WRITE_WAIT: begin
          if (~cam_wr_busy) begin
            ins_state <= ST_IDLE;
            count <= next_addr;
          end
        end

        // Write the specified value to the RAM
        //
        ST_RAM_WRITE: begin
          ins_state <= ST_IDLE;
          count <= next_addr;
        end

        default: begin
          // We should not get here
          ins_state <= ST_IDLE;
        end
      endcase
    end
  end

  // CAM Read Port:
  // - Find has priority so it can interrupt an insert
  assign cam_rd_key = 
    (ins_state != ST_CAM_READ || find_key_stb) ? find_key : ins_key_cached;

  // RAM Write Port:
  // - The RAM write enable is held high for 1 cycle
  // - The address may come from a CAM lookup or could generated
  assign ram_wr_en    = (ins_state == ST_RAM_WRITE || ins_state == ST_CAM_RAM_WRITE);
  assign ram_wr_addr  = write_addr;
  assign ram_wr_data  = ins_val_cached;

  // CAM Write Port:
  // - The CAM write enable is held high for 1 cycle
  // - The address may come from a CAM lookup or could generated (same as RAM)
  assign cam_wr_en    = (ins_state == ST_CAM_RAM_WRITE);
  assign cam_wr_addr  = write_addr;
  assign cam_wr_data  = ins_key_cached;

  // Outputs
  assign insert_busy  = (ins_state != ST_IDLE);
  assign find_res_val = ram_rd_data;

endmodule

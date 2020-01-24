//
// Copyright 2018 Ettus Research, A National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: chdr_xb_routing_table
// Description:
//   A routing table for the CHDR crossbar. This table is designed
//   to be shared between all ports. It has an AXI-Stream lookup
//   interface and a ctrlport (reduced) configuration interface.

module chdr_xb_routing_table #(
  parameter SIZE            = 6,
  parameter NPORTS          = 4,
  parameter EXT_INS_PORT_EN = 1
) (
  // Clocks and resets
  input  wire                               clk,
  input  wire                               reset,
  // Insertion Interface (for XB ports)
  input  wire [NPORTS-1:0]                  port_req_wr,
  input  wire [(16*NPORTS)-1:0]             port_req_addr,
  input  wire [(32*NPORTS)-1:0]             port_req_data,
  output wire [NPORTS-1:0]                  port_resp_ack,
  // Insertion Interface (External)         
  input  wire                               ext_req_wr,
  input  wire [15:0]                        ext_req_addr,
  input  wire [31:0]                        ext_req_data,
  output wire                               ext_resp_ack,
  // Find Interface                         
  input  wire [(16*NPORTS)-1:0]             axis_find_tdata,
  input  wire [NPORTS-1:0]                  axis_find_tvalid,
  output wire [NPORTS-1:0]                  axis_find_tready,
  // Result Interface (for Find)
  output wire [($clog2(NPORTS)*NPORTS)-1:0] axis_result_tdata,
  output wire [NPORTS-1:0]                  axis_result_tkeep,
  output wire [NPORTS-1:0]                  axis_result_tvalid,
  input  wire [NPORTS-1:0]                  axis_result_tready
);
  localparam NPORTS_W   = $clog2(NPORTS);
  localparam CFG_W      = NPORTS_W + 16;
  localparam CFG_PORTS  = NPORTS + EXT_INS_PORT_EN;

  // CAM-based lookup table

  wire [15:0]         insert_tdest;
  wire [NPORTS_W-1:0] insert_tdata;
  wire                insert_tvalid;
  wire                insert_tready;

  axis_muxed_kv_map #(
    .KEY_WIDTH(16), .VAL_WIDTH(NPORTS_W),
    .SIZE(SIZE), .NUM_PORTS(NPORTS)
  ) kv_map_i (
    .clk               (clk               ),
    .reset             (reset               ),
    .axis_insert_tdata (insert_tdata      ),
    .axis_insert_tdest (insert_tdest      ),
    .axis_insert_tvalid(insert_tvalid     ),
    .axis_insert_tready(insert_tready     ),
    .axis_find_tdata   (axis_find_tdata   ),
    .axis_find_tvalid  (axis_find_tvalid  ),
    .axis_find_tready  (axis_find_tready  ),
    .axis_result_tdata (axis_result_tdata ),
    .axis_result_tkeep (axis_result_tkeep ),
    .axis_result_tvalid(axis_result_tvalid),
    .axis_result_tready(axis_result_tready)
  );

  // Logic to convert from ctrlport to AXI-Stream

  wire                ins_req_wr  [0:CFG_PORTS-1];
  wire [15:0]         ins_req_addr[0:CFG_PORTS-1];
  wire [NPORTS_W-1:0] ins_req_data[0:CFG_PORTS-1];
  wire                ins_resp_ack[0:CFG_PORTS-1];

  reg  [(CFG_PORTS*CFG_W)-1:0] cfg_tdata;
  reg  [CFG_PORTS-1:0] cfg_tvalid = {CFG_PORTS{1'b0}};
  wire [CFG_PORTS-1:0] cfg_tready;

  genvar i;
  generate for (i = 0; i < CFG_PORTS; i=i+1) begin
    assign ins_req_wr  [i] = (i < NPORTS) ? port_req_wr[i]                  : ext_req_wr;
    assign ins_req_addr[i] = (i < NPORTS) ? port_req_addr[i*16 +: 16]       : ext_req_addr;
    assign ins_req_data[i] = (i < NPORTS) ? port_req_data[i*32 +: NPORTS_W] : ext_req_data[NPORTS_W-1:0];
    if (i < NPORTS)
      assign port_resp_ack[i] = ins_resp_ack[i];
    else
      assign ext_resp_ack     = ins_resp_ack[i];

    always @(posedge clk) begin
      if (reset) begin
        cfg_tvalid[i] <= 1'b0;
      end else begin
        if (~cfg_tvalid[i]) begin
          if (ins_req_wr[i]) begin
            cfg_tvalid[i] <= 1'b1;
            cfg_tdata[(CFG_W*i) +: CFG_W] <= {ins_req_data[i], ins_req_addr[i]};
          end
        end else begin
          cfg_tvalid[i] <= ~cfg_tready[i];
        end
      end
    end
    assign ins_resp_ack[i] = cfg_tvalid[i] & cfg_tready[i];
  end endgenerate

  // Multiplexer between XB ports and external cfg

  axi_mux #(
    .WIDTH(CFG_W), .SIZE(CFG_PORTS),
    .PRE_FIFO_SIZE(0), .POST_FIFO_SIZE(1)
  ) rtcfg_mux_i (
    .clk(clk), .reset(reset), .clear(1'b0),
    .i_tdata(cfg_tdata), .i_tlast({(NPORTS_W + 16){1'b1}}),
    .i_tvalid(cfg_tvalid), .i_tready(cfg_tready),
    .o_tdata({insert_tdata, insert_tdest}), .o_tlast(),
    .o_tvalid(insert_tvalid), .o_tready(insert_tready)
  );

endmodule


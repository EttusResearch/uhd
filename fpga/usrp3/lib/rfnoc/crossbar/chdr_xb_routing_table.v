//
// Copyright 2021 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: chdr_xb_routing_table
//
// Description:
//
//   A routing table for the CHDR crossbar. This table is designed to be shared
//   between all ports. It has an AXI-Stream lookup interface and two
//   simplified ctrlport configuration interfaces, one for crossbar (XB) ports
//   and the other for external configuration.
//
//   To insert an entry into the routing table, write the desired output port
//   number to the EPID address. That is, put the EPID for a route into the
//   req_addr field and the corresponding output port number on the req_data
//   field.
//
// Parameters:
//
//   SIZE            : The number entries to support in the routing table.
//   NPORTS          : The number of output crossbar ports.
//   EXT_INS_PORT_EN : Set to 1 to enable the external configuration interface.
//

`default_nettype none


module chdr_xb_routing_table #(
  parameter SIZE            = 6,
  parameter NPORTS          = 4,
  parameter EXT_INS_PORT_EN = 1
) (
  // Clocks and resets
  input  wire                               clk,
  input  wire                               reset,
  // Insertion Interface (for XB ports)
  input  wire [                 NPORTS-1:0] port_req_wr,
  input  wire [            (16*NPORTS)-1:0] port_req_addr,
  input  wire [            (32*NPORTS)-1:0] port_req_data,
  output wire [                 NPORTS-1:0] port_resp_ack,
  // Insertion Interface (External)
  input  wire                               ext_req_wr,
  input  wire [                       15:0] ext_req_addr,
  input  wire [                       31:0] ext_req_data,
  output wire                               ext_resp_ack,
  // Find Interface
  input  wire [            (16*NPORTS)-1:0] axis_find_tdata,
  input  wire [                 NPORTS-1:0] axis_find_tvalid,
  output wire [                 NPORTS-1:0] axis_find_tready,
  // Result Interface (for Find)
  output wire [($clog2(NPORTS)*NPORTS)-1:0] axis_result_tdata,
  output wire [                 NPORTS-1:0] axis_result_tkeep,
  output wire [                 NPORTS-1:0] axis_result_tvalid,
  input  wire [                 NPORTS-1:0] axis_result_tready
);
  localparam NPORTS_W  = $clog2(NPORTS);
  localparam CFG_W     = NPORTS_W + 16;
  localparam CFG_PORTS = NPORTS + EXT_INS_PORT_EN;

  //---------------------------------------------------------------------------
  // CAM-based lookup table
  //---------------------------------------------------------------------------

  wire [        15:0] insert_tdest;
  wire [NPORTS_W-1:0] insert_tdata;
  wire                insert_tvalid;
  wire                insert_tready;

  axis_muxed_kv_map #(
    .KEY_WIDTH(16      ),
    .VAL_WIDTH(NPORTS_W),
    .SIZE     (SIZE    ),
    .NUM_PORTS(NPORTS  )
  ) kv_map_i (
    .clk               (clk               ),
    .reset             (reset             ),
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

  //---------------------------------------------------------------------------
  // Logic to convert from CtrlPort to AXI-Stream
  //---------------------------------------------------------------------------

  wire                ins_req_wr  [0:CFG_PORTS-1];
  wire [        15:0] ins_req_addr[0:CFG_PORTS-1];
  wire [NPORTS_W-1:0] ins_req_data[0:CFG_PORTS-1];
  wire                ins_resp_ack[0:CFG_PORTS-1];

  reg  [(CFG_PORTS*CFG_W)-1:0] cfg_tdata;
  reg  [        CFG_PORTS-1:0] cfg_tvalid = {CFG_PORTS{1'b0}};
  wire [        CFG_PORTS-1:0] cfg_tready;

  genvar i;
  generate for (i = 0; i < CFG_PORTS; i=i+1) begin : gen_cfg_ports
    assign ins_req_wr[i]   = (i < NPORTS) ? port_req_wr[i]                  : ext_req_wr;
    assign ins_req_addr[i] = (i < NPORTS) ? port_req_addr[i*16 +: 16]       : ext_req_addr;
    assign ins_req_data[i] = (i < NPORTS) ? port_req_data[i*32 +: NPORTS_W] : ext_req_data[NPORTS_W-1:0];

    if (i < NPORTS) begin : gen_port_resp
      assign port_resp_ack[i] = ins_resp_ack[i];
    end else begin : gen_ext_resp
      assign ext_resp_ack = ins_resp_ack[i];
    end

    always @(posedge clk) begin : cfg_regs
      if (reset) begin
        cfg_tvalid[i] <= 1'b0;
      end else begin
        if (~cfg_tvalid[i]) begin
          if (ins_req_wr[i]) begin
            cfg_tvalid[i]               <= 1'b1;
            cfg_tdata[(CFG_W*i)+:CFG_W] <= {ins_req_data[i], ins_req_addr[i]};
          end
        end else begin
          cfg_tvalid[i] <= ~cfg_tready[i];
        end
      end
    end
    assign ins_resp_ack[i] = cfg_tvalid[i] & cfg_tready[i];
  end endgenerate

  //---------------------------------------------------------------------------
  // Multiplexer between XB ports and external configuration
  //---------------------------------------------------------------------------

  axi_mux #(
    .WIDTH         (CFG_W    ),
    .SIZE          (CFG_PORTS),
    .PRE_FIFO_SIZE (0        ),
    .POST_FIFO_SIZE(1        )
  ) axi_mux_i (
    .clk     (clk                         ),
    .reset   (reset                       ),
    .clear   (1'b0                        ),
    .i_tdata (cfg_tdata                   ),
    .i_tlast ({CFG_PORTS{1'b1}}           ),
    .i_tvalid(cfg_tvalid                  ),
    .i_tready(cfg_tready                  ),
    .o_tdata ({insert_tdata, insert_tdest}),
    .o_tlast (                            ),
    .o_tvalid(insert_tvalid               ),
    .o_tready(insert_tready               )
  );

endmodule


`default_nettype wire

//
// Copyright 2019 Ettus Research, A National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: chdr_data_swapper
// Description:
//  A module to adapt a CHDR stream to correctly sequence in
//  a software buffer of a user-specified type. Here are the 
//  the swapping assumptions:
//  - The CHDR header, timestamp and metadata for all packet
//    types must be interpreted as a uint64_t.
//  - All Control, Stream Status/Cmd, Management packet payloads
//    must reside in a uint64_t* buffer.
//  - The buffer type for the data packet payload and metadata
//    is user configurable
//
// Parameters:
//   - CHDR_W: Width of the tdata bus in bits
//
// Signals:
//   - payload_sw_buff: SW buffer mode for payload (0=u64, 1=u32, 2=u16, 3=u8)
//   - mdata_sw_buff  : SW buffer mode for metadata (0=u64, 1=u32, 2=u16, 3=u8)
//   - s_axis_*       : The input AXI stream
//   - m_axis_*       : The output AXI stream
//

module chdr_data_swapper #(
  parameter CHDR_W = 256
)(
  // Clock and Reset
  input  wire              clk,
  input  wire              rst,
  // Software Buffer Mode
  input  wire [1:0]        payload_sw_buff,
  input  wire [1:0]        mdata_sw_buff,
  input  wire              swap_endianness,
  // Input AXIS
  input  wire [CHDR_W-1:0] s_axis_tdata,
  input  wire              s_axis_tlast,
  input  wire              s_axis_tvalid,
  output wire              s_axis_tready,
  // Output AXIS           
  output wire [CHDR_W-1:0] m_axis_tdata,
  output wire              m_axis_tlast,
  output wire              m_axis_tvalid,
  input  wire              m_axis_tready
);

  `include "../core/rfnoc_chdr_utils.vh"

  // *_sw_buff values
  localparam [1:0] SW_BUFF_UINT64 = 2'd0;
  localparam [1:0] SW_BUFF_UINT32 = 2'd1;
  localparam [1:0] SW_BUFF_UINT16 = 2'd2;
  localparam [1:0] SW_BUFF_UINT8  = 2'd3;

  localparam SWAP_W = $clog2(CHDR_W);

  // Packet states
  localparam [2:0] ST_HDR       = 3'd0;
  localparam [2:0] ST_TS        = 3'd1;
  localparam [2:0] ST_MDATA     = 3'd2;
  localparam [2:0] ST_DATA_BODY = 3'd3;
  localparam [2:0] ST_OTHER     = 3'd4;

  reg [2:0]         state = ST_HDR;
  reg [4:0]         mdata_pending = CHDR_NO_MDATA;
  reg [SWAP_W-2:0]  pyld_tswap = 'h0, mdata_tswap = 'h0;

  // Shortcuts: CHDR header
  wire [2:0] pkt_type = chdr_get_pkt_type(s_axis_tdata[63:0]);
  wire [4:0] num_mdata = chdr_get_num_mdata(s_axis_tdata[63:0]);

  // State machine to determine packet state
  always @(posedge clk) begin
    if (rst) begin
      state <= ST_HDR;
    end else if (s_axis_tvalid & s_axis_tready) begin
      case (state)
        ST_HDR: begin
          mdata_pending <= num_mdata;
          if (!s_axis_tlast) begin
            if (CHDR_W > 64) begin
              if (pkt_type == CHDR_PKT_TYPE_DATA || pkt_type == CHDR_PKT_TYPE_DATA_TS) begin
                if (num_mdata != CHDR_NO_MDATA) begin
                  state <= ST_MDATA;
                end else begin
                  state <= ST_DATA_BODY;
                end
              end else begin
                state <= ST_OTHER;
              end
            end else begin
              if (pkt_type == CHDR_PKT_TYPE_DATA_TS) begin
                state <= ST_TS;
              end else if (pkt_type == CHDR_PKT_TYPE_DATA) begin
                if (num_mdata != CHDR_NO_MDATA) begin
                  state <= ST_MDATA;
                end else begin
                  state <= ST_DATA_BODY;
                end
              end else begin
                state <= ST_OTHER;
              end
            end
          end else begin
            state <= ST_HDR;
          end
        end
        ST_TS: begin
          if (!s_axis_tlast) begin
            if (mdata_pending != CHDR_NO_MDATA) begin
              state <= ST_MDATA;
            end else begin
              state <= ST_DATA_BODY;
            end
          end else begin
            state <= ST_HDR;
          end
        end
        ST_MDATA: begin
          if (!s_axis_tlast) begin
            if (mdata_pending == 5'd1) begin
              state <= ST_DATA_BODY;
            end else begin
              mdata_pending <= mdata_pending - 5'd1;
            end
          end else begin
            state <= ST_HDR;
          end
        end
        ST_DATA_BODY: begin
          if (s_axis_tlast) begin
            state <= ST_HDR;
          end
        end
        ST_OTHER: begin
          if (s_axis_tlast) begin
            state <= ST_HDR;
          end
        end
        default: begin
          state <= ST_HDR;
        end
      endcase
    end
  end

  // Convert SW buff size to swap-lane map
  always @(posedge clk) begin
    pyld_tswap <= 'h0;
    mdata_tswap <= 'h0;
    case (payload_sw_buff)
      SW_BUFF_UINT8:
        pyld_tswap[4:2] <= 3'b111;
      SW_BUFF_UINT16:
        pyld_tswap[4:2] <= 3'b110;
      SW_BUFF_UINT32:
        pyld_tswap[4:2] <= 3'b100;
      default:
        pyld_tswap[4:2] <= 3'b000;
    endcase
    case (mdata_sw_buff)
      SW_BUFF_UINT8:
        mdata_tswap[4:2] <= 3'b111;
      SW_BUFF_UINT16:
        mdata_tswap[4:2] <= 3'b110;
      SW_BUFF_UINT32:
        mdata_tswap[4:2] <= 3'b100;
      default:
        mdata_tswap[4:2] <= 3'b000;
    endcase
  end

  wire [SWAP_W-2:0] s_axis_tswap_dyn = 
    (state == ST_DATA_BODY) ? pyld_tswap : (
    (state == ST_MDATA) ? mdata_tswap : {(SWAP_W-1){1'b0}}
  );
  wire s_axis_tswap_end = swap_endianness && 
    (state == ST_DATA_BODY || state == ST_MDATA);

  // Swapper that re-aligns items in a buffer for software
 wire [CHDR_W-1:0] out_swap_tdata, out_swap_tdata_pre;
 wire              out_swap_tswap_end, out_swap_tlast, out_swap_tvalid, out_swap_tready;

  axis_data_swap #(
    .DATA_W(CHDR_W), .USER_W(1'b1),
    .STAGES_EN({{(SWAP_W-6){1'b0}}, 6'b111100}), .DYNAMIC(1)
  ) chdr_dyn_swap_i (
    .clk          (clk               ),
    .rst          (rst               ),
    .s_axis_tdata (s_axis_tdata      ),
    .s_axis_tswap (s_axis_tswap_dyn  ),
    .s_axis_tuser (s_axis_tswap_end  ),
    .s_axis_tlast (s_axis_tlast      ),
    .s_axis_tvalid(s_axis_tvalid     ),
    .s_axis_tready(s_axis_tready     ),
    .m_axis_tdata (out_swap_tdata_pre),
    .m_axis_tuser (out_swap_tswap_end),
    .m_axis_tlast (out_swap_tlast    ),
    .m_axis_tvalid(out_swap_tvalid   ),
    .m_axis_tready(out_swap_tready   )
  );

  // Swapper that pre-corrects for transport endianness
  genvar i;
  generate for (i = 0; i < CHDR_W/8; i=i+1) begin
    assign out_swap_tdata[i*8 +: 8] = out_swap_tswap_end ? 
      out_swap_tdata_pre[((CHDR_W/8)-i-1)*8 +: 8] : out_swap_tdata_pre[i*8 +: 8];
  end endgenerate

  axi_fifo_flop2 #(.WIDTH(CHDR_W+1)) out_reg_i (
    .clk     (clk                             ),
    .reset   (rst                             ),
    .clear   (1'b0                            ),
    .i_tdata ({out_swap_tlast, out_swap_tdata}),
    .i_tvalid(out_swap_tvalid                 ),
    .i_tready(out_swap_tready                 ),
    .o_tdata ({m_axis_tlast, m_axis_tdata}    ),
    .o_tvalid(m_axis_tvalid                   ),
    .o_tready(m_axis_tready                   ),
    .occupied(                                ),
    .space   (                                )
  );

endmodule // chdr_data_swapper

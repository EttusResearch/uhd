//
// Copyright 2011-2012 Ettus Research LLC
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

//////////////////////////////////////////////////////////////////////////////////

//this is a FIFO master interface for the FX2 in "slave fifo" mode.

module slave_fifo
  #(
        //how many cycles max in a transfer state
        parameter DATA_XFER_COUNT = 256,
        parameter CTRL_XFER_COUNT = 32,

        //sizes for fifo36 2 clock cascade fifos
        parameter DATA_RX_FIFO_SIZE = 9,
        parameter DATA_TX_FIFO_SIZE = 9,
        parameter CTRL_RX_FIFO_SIZE = 9,
        parameter CTRL_TX_FIFO_SIZE = 9
   )
   (// GPIF signals
    input gpif_clk, input gpif_rst,
    inout [15:0] gpif_d,
    input [3:0] gpif_ctl,
    output reg sloe, output reg slrd, output reg slwr, output reg pktend, output reg [1:0] fifoadr,

    // FIFO interface
    input fifo_clk, input fifo_rst,
    output [35:0] tx_data, output tx_src_rdy, input tx_dst_rdy,
    input [35:0] rx_data, input rx_src_rdy, output rx_dst_rdy,
    output [35:0] ctrl_data, output ctrl_src_rdy, input ctrl_dst_rdy,
    input [35:0] resp_data, input resp_src_rdy, output resp_dst_rdy,

    output [31:0] debug
    );

    wire FX2_DE_pre = ~gpif_ctl[0]; //EP2 FX2 FIFO empty (FLAGA)
    wire FX2_CE_pre = ~gpif_ctl[1]; //EP4 FX2 FIFO empty (FLAGB)
    wire FX2_DF_pre = ~gpif_ctl[2]; //EP6 FX2 FIFO full  (FLAGC)
    wire FX2_CF_pre = ~gpif_ctl[3]; //EP8 FX2 FIFO full  (FLAGD)

    reg FX2_DE, FX2_CE, FX2_DF, FX2_CF;
    always @(posedge gpif_clk) begin
        FX2_DE <= FX2_DE_pre; //EP2 FX2 FIFO empty (FLAGA)
        FX2_CE <= FX2_CE_pre; //EP4 FX2 FIFO empty (FLAGB)
        FX2_DF <= FX2_DF_pre; //EP6 FX2 FIFO full  (FLAGC)
        FX2_CF <= FX2_CF_pre; //EP8 FX2 FIFO full  (FLAGD)
    end

   wire [15:0] gpif_d_out_ctrl, gpif_d_out_data;
   reg [15:0] gpif_d_out, gpif_d_in;

   // ////////////////////////////////////////////////////////////////////
   // GPIF bus master state machine

    wire rx_valid, resp_valid;
    reg tx_valid, ctrl_valid;
    wire tx_ready, ctrl_ready;
    reg rx_enable, resp_enable;
    wire rx_data_enough_occ;

   reg [9:0] transfer_count; //number of lines (a line is 16 bits) in active transfer

   reg [3:0] state; //state machine current state
   localparam STATE_IDLE    = 0;
   localparam STATE_THINK   = 1;
   localparam STATE_DATA_RX = 2;
   localparam STATE_DATA_TX = 3;
   localparam STATE_CTRL_RX = 4;
   localparam STATE_CTRL_TX = 5;
   localparam STATE_DATA_TX_SLOE = 6;
   localparam STATE_CTRL_TX_SLOE = 7;
   localparam STATE_DATA_RX_ADR = 8;
   localparam STATE_CTRL_RX_ADR = 9;

   //logs the last bus user for xfer fairness
   //we only care about data rx vs. tx since ctrl pkts are so short
   reg last_data_bus_hog;
   localparam BUS_HOG_RX = 0;
   localparam BUS_HOG_TX = 1;

    wire resp_eof;
    reg [1:0] idle_count;

   // //////////////////////////////////////////////////////////////
   // FX2 slave FIFO bus master state machine
   //
    always @(posedge gpif_clk)
    if(gpif_rst) begin
        state <= STATE_IDLE;
        sloe <= 1;
        slrd <= 1;
        slwr <= 1;
        pktend <= 1;
        rx_enable <= 0;
        tx_valid <= 0;
        ctrl_valid <= 0;
        resp_enable <= 0;
        idle_count <= 0;
    end
    else case (state)
    STATE_IDLE: begin
        transfer_count <= 0;
        sloe <= 1;
        slrd <= 1;
        slwr <= 1;
        pktend <= 1;
        rx_enable <= 0;
        tx_valid <= 0;
        ctrl_valid <= 0;
        resp_enable <= 0;
        if (idle_count == 2'b11) state <= STATE_THINK;
        idle_count <= idle_count + 1;
    end

    STATE_THINK: begin

        idle_count <= 0;

        //handle transitions to other states
        if(ctrl_ready & ~FX2_CE) begin //if there's room in the ctrl fifo and the FX2 has ctrl data
            state <= STATE_CTRL_TX_SLOE;
            fifoadr <= 2'b01;
            sloe <= 0;
        end
        else if(resp_valid & ~FX2_CF) begin //if the ctrl fifo has data and the FX2 isn't full
            state <= STATE_CTRL_RX_ADR;
            fifoadr <= 2'b11;
        end
        else if(tx_ready & ~FX2_DE & last_data_bus_hog == BUS_HOG_RX) begin //if there's room in the data fifo and the FX2 has data
            state <= STATE_DATA_TX_SLOE;
            last_data_bus_hog <= BUS_HOG_TX;
            fifoadr <= 2'b00;
            sloe <= 0;
        end
        else if(rx_data_enough_occ & ~FX2_DF & last_data_bus_hog == BUS_HOG_TX) begin //if the data fifo has data and the FX2 isn't full
            state <= STATE_DATA_RX_ADR;
            last_data_bus_hog <= BUS_HOG_RX;
            fifoadr <= 2'b10;
        end
        else if(tx_ready & ~FX2_DE) begin
            state <= STATE_DATA_TX_SLOE;
            last_data_bus_hog <= BUS_HOG_TX;
            fifoadr <= 2'b00;
            sloe <= 0;
        end
        else if(rx_data_enough_occ & ~FX2_DF) begin
            state <= STATE_DATA_RX_ADR;
            last_data_bus_hog <= BUS_HOG_RX;
            fifoadr <= 2'b10;
        end
    end

    STATE_DATA_TX_SLOE: begin //just to assert SLOE one cycle before SLRD
        state <= STATE_DATA_TX;
        slrd <= 0;
    end

    STATE_CTRL_TX_SLOE: begin
        state <= STATE_CTRL_TX;
        slrd <= 0;
    end

    STATE_DATA_RX_ADR: begin //just to assert FIFOADR one cycle before SLWR
        state <= STATE_DATA_RX;
        rx_enable <= 1;
    end

    STATE_CTRL_RX_ADR: begin
        state <= STATE_CTRL_RX;
        resp_enable <= 1;
    end

    STATE_DATA_RX: begin
        if (FX2_DF_pre || ~rx_valid || transfer_count == DATA_XFER_COUNT-1) begin
            state <= STATE_IDLE;
            rx_enable <= 0;
        end
        gpif_d_out <= gpif_d_out_data;
        slwr <= ~rx_valid;
        transfer_count <= transfer_count + 1;
    end

    STATE_DATA_TX: begin
        if (FX2_DE_pre || transfer_count == DATA_XFER_COUNT-1) begin
            state <= STATE_IDLE;
            slrd <= 1;
        end
        gpif_d_in <= gpif_d;
        tx_valid <= 1;
        transfer_count <= transfer_count + 1;
    end

    STATE_CTRL_RX: begin
        if (FX2_CF_pre || ~resp_valid || resp_eof || transfer_count == CTRL_XFER_COUNT-1) begin
            state <= STATE_IDLE;
            resp_enable <= 0;
        end
        pktend <= ~resp_eof;
        gpif_d_out <= gpif_d_out_ctrl;
        slwr <= ~resp_valid;
        transfer_count <= transfer_count + 1;
    end

    STATE_CTRL_TX: begin
        if (FX2_CE_pre || transfer_count == CTRL_XFER_COUNT-1) begin
            state <= STATE_IDLE;
            slrd <= 1;
        end
        gpif_d_in <= gpif_d;
        ctrl_valid <= 1;
        transfer_count <= transfer_count + 1;
    end
    endcase

   // GPIF output data lines, tristate
   assign gpif_d = (sloe)? gpif_d_out[15:0] : 16'bz;
   
   // ////////////////////////////////////////////////////////////////////
   // TX Data Path

    gpmc16_to_fifo36 #(.FIFO_SIZE(DATA_TX_FIFO_SIZE), .MIN_SPACE16(DATA_XFER_COUNT)) fifo36_to_gpmc16_tx(
        .gpif_clk(gpif_clk), .gpif_rst(gpif_rst),
        .in_data(gpif_d_in), .ready(tx_ready), .valid(tx_valid),
        .fifo_clk(fifo_clk), .fifo_rst(fifo_rst),
        .out_data(tx_data), .out_src_rdy(tx_src_rdy), .out_dst_rdy(tx_dst_rdy)
    );

   // ////////////////////////////////////////////
   // RX Data Path

    fifo36_to_gpmc16 #(.FIFO_SIZE(DATA_RX_FIFO_SIZE), .MIN_OCC16(DATA_XFER_COUNT)) fifo36_to_gpmc16_rx(
        .fifo_clk(fifo_clk), .fifo_rst(fifo_rst),
        .in_data(rx_data), .in_src_rdy(rx_src_rdy), .in_dst_rdy(rx_dst_rdy),
        .gpif_clk(gpif_clk), .gpif_rst(gpif_rst),
        .has_data(rx_data_enough_occ),
        .out_data(gpif_d_out_data), .valid(rx_valid), .enable(rx_enable)
    );

   // ////////////////////////////////////////////////////////////////////
   // CTRL TX Data Path

    gpmc16_to_fifo36 #(.FIFO_SIZE(CTRL_TX_FIFO_SIZE), .MIN_SPACE16(CTRL_XFER_COUNT)) fifo36_to_gpmc16_ctrl(
        .gpif_clk(gpif_clk), .gpif_rst(gpif_rst),
        .in_data(gpif_d_in), .ready(ctrl_ready), .valid(ctrl_valid),
        .fifo_clk(fifo_clk), .fifo_rst(fifo_rst),
        .out_data(ctrl_data), .out_src_rdy(ctrl_src_rdy), .out_dst_rdy(ctrl_dst_rdy)
    );

   // ////////////////////////////////////////////
   // CTRL RX Data Path

    fifo36_to_gpmc16 #(.FIFO_SIZE(CTRL_RX_FIFO_SIZE)) fifo36_to_gpmc16_resp(
        .fifo_clk(fifo_clk), .fifo_rst(fifo_rst),
        .in_data(resp_data), .in_src_rdy(resp_src_rdy), .in_dst_rdy(resp_dst_rdy),
        .gpif_clk(gpif_clk), .gpif_rst(gpif_rst),
        .out_data(gpif_d_out_ctrl), .valid(resp_valid), .enable(resp_enable),
        .eof(resp_eof)
    );

    assign debug = 0;

endmodule // slave_fifo

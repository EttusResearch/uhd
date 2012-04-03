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
  #(parameter TXFIFOSIZE = 12, parameter RXFIFOSIZE = 12)
   (// GPIF signals
    input gpif_clk, input gpif_rst,
    inout [15:0] gpif_d,
    input [3:0] gpif_ctl,
    output sloe, output slrd, output slwr, output pktend, output [1:0] fifoadr,
    
    // Wishbone signals
    input wb_clk, input wb_rst,
    output [15:0] wb_adr_o, output [15:0] wb_dat_mosi, input [15:0] wb_dat_miso,
    output [1:0] wb_sel_o, output wb_cyc_o, output wb_stb_o, output wb_we_o, input wb_ack_i,
    input [7:0] triggers,
    
    input dsp_rx_run,
    
    // FIFO interface
    input fifo_clk, input fifo_rst, input clear_tx, input clear_rx,
    output [35:0] tx_data_o, output tx_src_rdy_o, input tx_dst_rdy_i,
    input [35:0] rx_data_i, input rx_src_rdy_i, output rx_dst_rdy_o,
    input [35:0] tx_err_data_i, input tx_err_src_rdy_i, output tx_err_dst_rdy_o,
    output tx_underrun, output rx_overrun,
    
    input [15:0] test_len, input [7:0] test_rate, input [3:0] test_ctrl,
    output [31:0] debug0, output [31:0] debug1
    );

    reg FX2_DE, FX2_CE, FX2_DF, FX2_CF;

   // inputs to FPGA (all active low)
    always @(posedge gpif_clk) begin
        FX2_DE <= ~gpif_ctl[0]; //EP2 FX2 FIFO empty (FLAGA)
        FX2_CE <= ~gpif_ctl[1]; //EP4 FX2 FIFO empty (FLAGB)
        FX2_DF <= ~gpif_ctl[2]; //EP6 FX2 FIFO full  (FLAGC)
        FX2_CF <= ~gpif_ctl[3]; //EP8 FX2 FIFO full  (FLAGD)
    end

   wire [17:0] 	  gpif_d_out_ctrl, gpif_d_out_data, gpif_d_out;

   // ////////////////////////////////////////////////////////////////////
   // GPIF bus master state machine

   //transfer size for GPIF data. this can be anything really, it's specified only for
   //fairness in bus sharing. 256 lines is 512 bytes over the wire, half the size of
   //the double buffers in B100/B150. this should probably be a toplevel parameter or even
   //a settings register value.
   localparam data_transfer_size = 256;
   localparam ctrl_transfer_size = 16; //probably unnecessary since ctrl xfers won't back up

   // state machine i/o to four fifos
   //tx
   wire ctrl_tx_dst_rdy; //sm input, ctrl tx path has space
   wire ctrl_tx_src_rdy; //sm output, ctrl tx path enable
   wire data_tx_dst_rdy; //sm input, data tx path has space
   wire data_tx_src_rdy; //sm output, data tx path enable

   //rx
   wire ctrl_rx_dst_rdy; //sm output, ctrl rx path enable
   wire ctrl_rx_src_rdy; //sm input, ctrl rx path has space
   wire data_rx_dst_rdy; //sm output, data rx path enable
   wire data_rx_src_rdy; //sm input, data rx path has space

   reg tx_data_enough_space;

   reg [9:0] transfer_count; //number of lines (a line is 16 bits) in active transfer
   
   reg pktend_latch;

   reg [3:0] state; //state machine current state
   localparam STATE_IDLE    = 0;
   localparam STATE_DATA_RX = 5;
   localparam STATE_DATA_TX = 3;
   localparam STATE_CTRL_RX = 6;
   localparam STATE_CTRL_TX = 9;
   localparam STATE_DATA_TX_SLOE = 2;
   localparam STATE_CTRL_TX_SLOE = 8;
   localparam STATE_DATA_RX_ADR = 1;
   localparam STATE_CTRL_RX_ADR = 4;
   localparam STATE_PKTEND_ADR = 10;
   localparam STATE_PKTEND = 7;

   //logs the last bus user for xfer fairness
   //we only care about data rx vs. tx since ctrl pkts are so short
   reg last_data_bus_hog;
   localparam BUS_HOG_RX = 0;
   localparam BUS_HOG_TX = 1;

   // //////////////////////////////////////////////////////////////
   // FX2 slave FIFO bus master state machine
   //
   always @(posedge gpif_clk)
     if(gpif_rst) 
       state <= STATE_IDLE;
     else
        begin
       case (state)
         STATE_IDLE:
            begin
           transfer_count <= 0;
           //handle transitions to other states
           if(ctrl_tx_dst_rdy & ~FX2_CE) //if there's room in the ctrl fifo and the FX2 has ctrl data
             state <= STATE_CTRL_TX_SLOE;
           else if(ctrl_rx_src_rdy & ~FX2_CF) //if the ctrl fifo has data and the FX2 isn't full
             state <= STATE_CTRL_RX_ADR;
           else if(data_tx_dst_rdy & ~FX2_DE & last_data_bus_hog == BUS_HOG_RX & tx_data_enough_space) //if there's room in the data fifo and the FX2 has data
             state <= STATE_DATA_TX_SLOE;
           else if(data_rx_src_rdy & ~FX2_DF & last_data_bus_hog == BUS_HOG_TX) //if the data fifo has data and the FX2 isn't full
             state <= STATE_DATA_RX_ADR;
           else if(data_tx_dst_rdy & ~FX2_DE & tx_data_enough_space)
             state <= STATE_DATA_TX_SLOE;
           else if(data_rx_src_rdy & ~FX2_DF)
             state <= STATE_DATA_RX_ADR;
           else if(~data_rx_src_rdy & ~dsp_rx_run & pktend_latch & ~FX2_DF)
             state <= STATE_PKTEND_ADR;
             
           if(data_rx_src_rdy)
             pktend_latch <= 1;
            end

         STATE_DATA_TX_SLOE: //just to assert SLOE one cycle before SLRD
           state <= STATE_DATA_TX;
         STATE_CTRL_TX_SLOE:
           state <= STATE_CTRL_TX;

         STATE_DATA_RX_ADR: //just to assert FIFOADR one cycle before SLWR
           state <= STATE_DATA_RX;
         STATE_CTRL_RX_ADR:
           state <= STATE_CTRL_RX;

         STATE_DATA_RX:
            begin
                if(data_rx_src_rdy && data_rx_dst_rdy)
                    transfer_count <= transfer_count + 1;
                else
                    state <= STATE_IDLE;
                last_data_bus_hog <= BUS_HOG_RX;
            end
            
         STATE_PKTEND_ADR:
            begin
           state <= STATE_PKTEND;
            end

         STATE_PKTEND:
            begin
           state <= STATE_IDLE;
           pktend_latch <= 0;
            end
            
         STATE_DATA_TX:
            begin
                if(data_tx_dst_rdy && data_tx_src_rdy)
                    transfer_count <= transfer_count + 1;
                else
                    state <= STATE_IDLE;
                last_data_bus_hog <= BUS_HOG_TX;
            end
         STATE_CTRL_RX:
            begin
                if(ctrl_rx_src_rdy && ctrl_rx_dst_rdy)
                    transfer_count <= transfer_count + 1;
                else
                    state <= STATE_IDLE;
            end
         STATE_CTRL_TX:
            begin
                if(ctrl_tx_dst_rdy && ctrl_tx_src_rdy)
                    transfer_count <= transfer_count + 1;
                else
                    state <= STATE_IDLE;
            end
       endcase
        end

   // ///////////////////////////////////////////////////////////////////
   // fifo signal assignments and enables

   //enable fifos
   assign data_rx_dst_rdy = (state == STATE_DATA_RX) && ~FX2_DF && (transfer_count != data_transfer_size);
   assign data_tx_src_rdy = (state == STATE_DATA_TX) && ~FX2_DE && (transfer_count != data_transfer_size);
   assign ctrl_rx_dst_rdy = (state == STATE_CTRL_RX) && ~FX2_CF;
   assign ctrl_tx_src_rdy = (state == STATE_CTRL_TX) && ~FX2_CE;

   //framing for TX ctrl packets
   wire sop_ctrl, eop_ctrl;
   assign sop_ctrl = (transfer_count == 0);
   assign eop_ctrl = (transfer_count == (ctrl_transfer_size-1));

   // ////////////////////////////////////////////////////////////////////
   // set GPIF pins

   //set fifoadr to the appropriate endpoint
   // {0,0}: EP2, data TX from host
   // {0,1}: EP4, ctrl TX from host
   // {1,0}: EP6, data RX to host
   // {1,1}: EP8, ctrl RX to host
   assign fifoadr = {(state == STATE_DATA_RX) | (state == STATE_CTRL_RX) | (state == STATE_DATA_RX_ADR) | (state == STATE_CTRL_RX_ADR) | (state == STATE_PKTEND) | (state == STATE_PKTEND_ADR),
                     (state == STATE_CTRL_RX) | (state == STATE_CTRL_RX_ADR) | (state == STATE_CTRL_TX) | (state == STATE_CTRL_TX_SLOE)};
   //set sloe, slwr, slrd (all active low)
   //SLOE gets asserted when we want data from the FX2; i.e., TX mode
   assign sloe = ~{(state == STATE_DATA_TX) | (state == STATE_CTRL_TX) | (state == STATE_DATA_TX_SLOE) | (state == STATE_CTRL_TX_SLOE)};
   //"read" and "write" here are from the master's point of view;
   //so "read" means "transmit" and "write" means "receive"
   assign slwr = ~{(data_rx_src_rdy && data_rx_dst_rdy) || (ctrl_rx_src_rdy && ctrl_rx_dst_rdy)};
   assign slrd = ~{(data_tx_src_rdy && data_tx_dst_rdy) || (ctrl_tx_src_rdy && ctrl_tx_dst_rdy)};

   wire pktend_ctrl, pktend_data;
   assign pktend_ctrl = ((~ctrl_rx_src_rdy | gpif_d_out_ctrl[17]) & (state == STATE_CTRL_RX));
   assign pktend_data = (state == STATE_PKTEND);
   assign pktend = ~(pktend_ctrl | pktend_data);

   //mux between ctrl/data RX data out based on endpoint selection
   assign gpif_d_out = fifoadr[0] ? gpif_d_out_ctrl : gpif_d_out_data;
   // GPIF output data lines, tristate
   assign gpif_d = sloe ? gpif_d_out : 16'bz;
   
   // ////////////////////////////////////////////////////////////////////
   // TX Data Path

   wire [15:0] 	  txfifo_data;
   wire 	  txfifo_src_rdy, txfifo_dst_rdy;
   wire [35:0] 	  tx36_data;
   wire 	  tx36_src_rdy, tx36_dst_rdy;
   wire [15:0]    data_tx_2clk;
   wire           tx_src_rdy_2clk, tx_dst_rdy_2clk;
   
   wire [15:0] wr_fifo_space;

   always @(posedge gpif_clk)
     tx_data_enough_space <= (wr_fifo_space >= data_transfer_size);

   fifo_cascade #(.WIDTH(16), .SIZE(12)) wr_fifo
     (.clk(gpif_clk), .reset(gpif_rst), .clear(clear_tx),
      .datain(gpif_d), .src_rdy_i(data_tx_src_rdy), .dst_rdy_o(data_tx_dst_rdy), .space(wr_fifo_space),
      .dataout(txfifo_data), .src_rdy_o(txfifo_src_rdy), .dst_rdy_i(txfifo_dst_rdy), .occupied());
   
   fifo_2clock_cascade #(.WIDTH(16), .SIZE(4)) wr_fifo_2clk
     (.wclk(gpif_clk), .datain(txfifo_data), .src_rdy_i(txfifo_src_rdy), .dst_rdy_o(txfifo_dst_rdy), .space(),
      .rclk(fifo_clk), .dataout(data_tx_2clk), .src_rdy_o(tx_src_rdy_2clk), .dst_rdy_i(tx_dst_rdy_2clk), .occupied(),
      .arst(fifo_rst));

   // join vita packets which are longer than one frame, add SOP/EOP/OCC
   wire [18:0] 	  refr_data;
   wire 	  refr_src_rdy, refr_dst_rdy;
   //below 3 signals for debug only
   wire refr_state;
   wire refr_eof;
   wire [15:0] refr_len;
   
   packet_reframer tx_packet_reframer 
     (.clk(fifo_clk), .reset(fifo_rst), .clear(clear_tx),
      .data_i(data_tx_2clk), .src_rdy_i(tx_src_rdy_2clk), .dst_rdy_o(tx_dst_rdy_2clk),
      .data_o(refr_data), .src_rdy_o(refr_src_rdy), .dst_rdy_i(refr_dst_rdy),
      .state(refr_state), .eof_out(refr_eof), .length(refr_len));

   fifo19_to_fifo36 #(.LE(1)) f19_to_f36
     (.clk(fifo_clk), .reset(fifo_rst), .clear(clear_tx),
      .f19_datain(refr_data), .f19_src_rdy_i(refr_src_rdy), .f19_dst_rdy_o(refr_dst_rdy),
      .f36_dataout(tx36_data), .f36_src_rdy_o(tx36_src_rdy), .f36_dst_rdy_i(tx36_dst_rdy));
   
   fifo_cascade #(.WIDTH(36), .SIZE(TXFIFOSIZE)) tx_fifo36
     (.clk(fifo_clk), .reset(fifo_rst), .clear(clear_tx),
      .datain(tx36_data), .src_rdy_i(tx36_src_rdy), .dst_rdy_o(tx36_dst_rdy),
      .dataout(tx_data_o), .src_rdy_o(tx_src_rdy_o), .dst_rdy_i(tx_dst_rdy_i));

   // ////////////////////////////////////////////
   // RX Data Path

   wire [35:0] 	  rx36_data;
   wire 	  rx36_src_rdy, rx36_dst_rdy;
   wire [18:0] 	  rx19_data;
   wire 	  rx19_src_rdy, rx19_dst_rdy;
   wire [15:0] rxfifospace;

   //deep 36 bit wide input fifo buffers from DSP
   fifo_cascade #(.WIDTH(36), .SIZE(8)) rx_fifo36
     (.clk(fifo_clk), .reset(fifo_rst), .clear(clear_rx),
      .datain(rx_data_i), .src_rdy_i(rx_src_rdy_i), .dst_rdy_o(rx_dst_rdy_o),
      .dataout(rx36_data), .src_rdy_o(rx36_src_rdy), .dst_rdy_i(rx36_dst_rdy));

   //convert to fifo19
   fifo36_to_fifo19 #(.LE(1)) f36_to_f19
     (.clk(fifo_clk), .reset(fifo_rst), .clear(clear_rx),
      .f36_datain(rx36_data), .f36_src_rdy_i(rx36_src_rdy), .f36_dst_rdy_o(rx36_dst_rdy),
      .f19_dataout(rx19_data), .f19_src_rdy_o(rx19_src_rdy), .f19_dst_rdy_i(rx19_dst_rdy) );

   wire [18:0] 	data_rx_int;
   wire 	rx_src_rdy_int, rx_dst_rdy_int;
   //clock domain crossing fifo for RX data
   fifo_2clock_cascade #(.WIDTH(19), .SIZE(4)) rd_fifo_2clk
     (.wclk(fifo_clk), .datain(rx19_data), .src_rdy_i(rx19_src_rdy), .dst_rdy_o(rx19_dst_rdy), .space(),
      .rclk(~gpif_clk), .dataout(data_rx_int), .src_rdy_o(rx_src_rdy_int), .dst_rdy_i(rx_dst_rdy_int), .occupied(),
      .arst(fifo_rst));

   //rd_fifo buffers writes to the 2clock fifo above
   fifo_cascade #(.WIDTH(16), .SIZE(RXFIFOSIZE)) rd_fifo
     (.clk(~gpif_clk), .reset(gpif_rst), .clear(clear_rx),
      .datain(data_rx_int), .src_rdy_i(rx_src_rdy_int), .dst_rdy_o(rx_dst_rdy_int), .space(rxfifospace),
      .dataout(gpif_d_out_data), .src_rdy_o(data_rx_src_rdy), .dst_rdy_i(data_rx_dst_rdy), .occupied());

   // ////////////////////////////////////////////////////////////////////
   // FIFO to Wishbone interface

   wire [18:0] 	  resp_data, resp_int;
   wire 	  resp_src_rdy, resp_dst_rdy;
   wire 	  resp_src_rdy_int, resp_dst_rdy_int;
   
   wire [18:0] 	  tx_err19_data;
   wire 	  tx_err19_src_rdy, tx_err19_dst_rdy;

   wire [18:0] 	  ctrl_data;
   wire 	  ctrl_src_rdy, ctrl_dst_rdy;

   fifo_to_wb fifo_to_wb
     (.clk(fifo_clk), .reset(fifo_rst), .clear(0),
      .data_i(ctrl_data), .src_rdy_i(ctrl_src_rdy), .dst_rdy_o(ctrl_dst_rdy),
      .data_o(resp_int), .src_rdy_o(resp_src_rdy_int), .dst_rdy_i(resp_dst_rdy_int),
      .wb_adr_o(wb_adr_o), .wb_dat_mosi(wb_dat_mosi), .wb_dat_miso(wb_dat_miso), .wb_sel_o(wb_sel_o), 
      .wb_cyc_o(wb_cyc_o), .wb_stb_o(wb_stb_o), .wb_we_o(wb_we_o), .wb_ack_i(wb_ack_i),
      .triggers(triggers),
      .debug0(), .debug1());
      
   // ////////////////////////////////////////////////////////////////////
   // TX CTRL PATH (ctrl commands into Wishbone)

   //how does this use fifo_clk instead of wb_clk
   //answer: on b100 fifo clk IS wb clk
   fifo_2clock_cascade #(.WIDTH(19), .SIZE(4)) ctrl_fifo_2clk
     (.wclk(gpif_clk), .datain({1'b0,eop_ctrl,sop_ctrl,gpif_d}), 
      .src_rdy_i(ctrl_tx_src_rdy), .dst_rdy_o(ctrl_tx_dst_rdy), .space(),
      .rclk(fifo_clk), .dataout(ctrl_data), 
      .src_rdy_o(ctrl_src_rdy), .dst_rdy_i(ctrl_dst_rdy), .occupied(),
      .arst(fifo_rst));

   // ////////////////////////////////////////////////////////////////////
   // RX CTRL PATH (async packets, ctrl response data)
   
   //tx_err_data_i is the 36wide tx async err data clocked on fifo_clk
   fifo36_to_fifo19 #(.LE(1)) f36_to_f19_txerr
     (.clk(fifo_clk), .reset(fifo_rst), .clear(clear_rx),
      .f36_datain(tx_err_data_i), .f36_src_rdy_i(tx_err_src_rdy_i), .f36_dst_rdy_o(tx_err_dst_rdy_o),
      .f19_dataout(tx_err19_data), .f19_src_rdy_o(tx_err19_src_rdy), .f19_dst_rdy_i(tx_err19_dst_rdy) );

   //mux FIFO-to-WB along with async tx err pkts into one ctrl resp fifo
   //how is this clocked on wb_clk?
   fifo19_mux #(.prio(0)) mux_err_stream
     (.clk(wb_clk), .reset(wb_rst), .clear(clear_rx),
      .data0_i(resp_int), .src0_rdy_i(resp_src_rdy_int), .dst0_rdy_o(resp_dst_rdy_int),
      .data1_i(tx_err19_data), .src1_rdy_i(tx_err19_src_rdy), .dst1_rdy_o(tx_err19_dst_rdy),
      .data_o(resp_data), .src_rdy_o(resp_src_rdy), .dst_rdy_i(resp_dst_rdy));

   //clock domain crossing cascade fifo for mux_err_stream to get from wb_clk to gpif_clk
   //the output of this fifo is CTRL DATA PENDING FOR GPIF
   fifo_2clock_cascade #(.WIDTH(18), .SIZE(4)) resp_fifo_2clk
     (.wclk(wb_clk), .datain(resp_data[17:0]), .src_rdy_i(resp_src_rdy), .dst_rdy_o(resp_dst_rdy), .space(),
      .rclk(~gpif_clk), .dataout(gpif_d_out_ctrl), 
      .src_rdy_o(ctrl_rx_src_rdy), .dst_rdy_i(ctrl_rx_dst_rdy), .occupied(),
      .arst(wb_rst));

        
   // ////////////////////////////////////////////////////////////////////
   // Debug support, timed and loopback
   // RX side muxes test data into the same stream

   ///////////////////////////////////////////////////////////////////////
   // debug lines
   wire [31:0] 	  debug_rd, debug_wr, debug_split0, debug_split1;
   
   wire [35:0] 	timedrx_data, loopbackrx_data, testrx_data;
   wire [35:0] 	timedtx_data, loopbacktx_data, testtx_data;
   wire 	timedrx_src_rdy, timedrx_dst_rdy, loopbackrx_src_rdy, loopbackrx_dst_rdy,
		testrx_src_rdy, testrx_dst_rdy;
   wire 	timedtx_src_rdy, timedtx_dst_rdy, loopbacktx_src_rdy, loopbacktx_dst_rdy,
		testtx_src_rdy, testtx_dst_rdy;
   wire 	timedrx_src_rdy_int, timedrx_dst_rdy_int, timedtx_src_rdy_int, timedtx_dst_rdy_int;

   wire [31:0] 	total, crc_err, seq_err, len_err;
   wire 	sel_testtx = test_ctrl[0];
   wire 	sel_loopbacktx = test_ctrl[1];
   wire 	pkt_src_enable = test_ctrl[2];
   wire 	pkt_sink_enable = test_ctrl[3];
/*   
   fifo36_mux rx_test_mux_lvl_1
     (.clk(fifo_clk), .reset(fifo_rst), .clear(clear_rx),
      .data0_i(timedrx_data), .src0_rdy_i(timedrx_src_rdy), .dst0_rdy_o(timedrx_dst_rdy),
      .data1_i(loopbackrx_data), .src1_rdy_i(loopbackrx_src_rdy), .dst1_rdy_o(loopbackrx_dst_rdy),
      .data_o(testrx_data), .src_rdy_o(testrx_src_rdy), .dst_rdy_i(testrx_dst_rdy));
   
   fifo36_mux rx_test_mux_lvl_2
     (.clk(fifo_clk), .reset(fifo_rst), .clear(clear_rx),
      .data0_i(testrx_data), .src0_rdy_i(testrx_src_rdy), .dst0_rdy_o(testrx_dst_rdy),
      .data1_i(rx_data_i), .src1_rdy_i(rx_src_rdy_i), .dst1_rdy_o(rx_dst_rdy_o),
      .data_o(rx_data), .src_rdy_o(rx_src_rdy), .dst_rdy_i(rx_dst_rdy));
   
   fifo_short #(.WIDTH(36)) loopback_fifo
     (.clk(fifo_clk), .reset(fifo_rst), .clear(clear_tx | clear_rx),
      .datain(loopbacktx_data), .src_rdy_i(loopbacktx_src_rdy), .dst_rdy_o(loopbacktx_dst_rdy),
      .dataout(loopbackrx_data), .src_rdy_o(loopbackrx_src_rdy), .dst_rdy_i(loopbackrx_dst_rdy));
   
   // Crossbar used as a demux for switching TX stream to main DSP or to test logic
   crossbar36 tx_crossbar_lvl_1
     (.clk(fifo_clk), .reset(fifo_rst), .clear(clear_tx),
      .cross(sel_testtx),
      .data0_i(tx_data), .src0_rdy_i(tx_src_rdy), .dst0_rdy_o(tx_dst_rdy),
      .data1_i(tx_data), .src1_rdy_i(1'b0), .dst1_rdy_o(),  // No 2nd input
      .data0_o(tx_data_o), .src0_rdy_o(tx_src_rdy_o), .dst0_rdy_i(tx_dst_rdy_i),
      .data1_o(testtx_data), .src1_rdy_o(testtx_src_rdy), .dst1_rdy_i(testtx_dst_rdy) );
   
   crossbar36 tx_crossbar_lvl_2
     (.clk(fifo_clk), .reset(fifo_rst), .clear(clear_tx),
      .cross(sel_loopbacktx),
      .data0_i(testtx_data), .src0_rdy_i(testtx_src_rdy), .dst0_rdy_o(testtx_dst_rdy),
      .data1_i(testtx_data), .src1_rdy_i(1'b0), .dst1_rdy_o(),  // No 2nd input
      .data0_o(timedtx_data), .src0_rdy_o(timedtx_src_rdy), .dst0_rdy_i(timedtx_dst_rdy),
      .data1_o(loopbacktx_data), .src1_rdy_o(loopbacktx_src_rdy), .dst1_rdy_i(loopbacktx_dst_rdy) );
   
   // Fixed rate TX traffic consumer
   fifo_pacer tx_pacer
     (.clk(fifo_clk), .reset(fifo_rst), .rate(test_rate), .enable(pkt_sink_enable),
      .src1_rdy_i(timedtx_src_rdy), .dst1_rdy_o(timedtx_dst_rdy),
      .src2_rdy_o(timedtx_src_rdy_int), .dst2_rdy_i(timedtx_dst_rdy_int),
      .underrun(tx_underrun), .overrun());

   packet_verifier32 pktver32
     (.clk(fifo_clk), .reset(fifo_rst), .clear(clear_tx),
      .data_i(timedtx_data), .src_rdy_i(timedtx_src_rdy_int), .dst_rdy_o(timedtx_dst_rdy_int),
      .total(total), .crc_err(crc_err), .seq_err(seq_err), .len_err(len_err));

   // Fixed rate RX traffic generator
   vita_pkt_gen pktgen
     (.clk(fifo_clk), .reset(fifo_rst), .clear(clear_rx),
      .len(test_len),
      .data_o(timedrx_data), .src_rdy_o(timedrx_src_rdy_int), .dst_rdy_i(timedrx_dst_rdy_int));

   fifo_pacer rx_pacer
     (.clk(fifo_clk), .reset(fifo_rst), .rate(test_rate), .enable(pkt_src_enable),
      .src1_rdy_i(timedrx_src_rdy_int), .dst1_rdy_o(timedrx_dst_rdy_int),
      .src2_rdy_o(timedrx_src_rdy), .dst2_rdy_i(timedrx_dst_rdy),
      .underrun(), .overrun(rx_overrun));
*/
   // ////////////////////////////////////////////
   //    DEBUG
   
   assign debug0 = { pktend_latch, data_rx_src_rdy, gpif_ctl[3:0], sloe, slrd, slwr, pktend, fifoadr[1:0], state[3:0], gpif_d[15:0]};
   //assign debug0 = { data_tx_src_rdy, data_tx_dst_rdy, tx_src_rdy_int, tx_dst_rdy_int, 
   //                  tx19_src_rdy, tx19_dst_rdy, refr_src_rdy, refr_dst_rdy, 
   //                  tx36_src_rdy, tx36_dst_rdy,
   //                  gpif_ctl[3:0], fifoadr[1:0], 
   //                  wr_fifo_space[15:0]};
   assign debug1 = { 16'b0, transfer_count[7:0], ctrl_rx_src_rdy, ctrl_tx_dst_rdy, data_rx_src_rdy,
                     data_tx_dst_rdy, ctrl_tx_src_rdy, ctrl_rx_dst_rdy, data_tx_src_rdy, data_rx_dst_rdy};
endmodule // slave_fifo

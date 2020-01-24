//
// Copyright 2011-2013 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//


//////////////////////////////////////////////////////////////////////////////////

//this is a FIFO master interface for the FX3 in "slave fifo" mode.

module gpif2_slave_fifo32
#(
    //sizes for fifo64 2 clock cascade fifos
    parameter DATA_RX_FIFO_SIZE = 12, //max vita pkt size
    parameter DATA_TX_FIFO_SIZE = 12, //max vita pkt size
    parameter CTRL_RX_FIFO_SIZE = 5,  //small resp packets
    parameter CTRL_TX_FIFO_SIZE = 5, //small ctrl packets

    //address constants for the endpoints
    parameter ADDR_DATA_TX = 2'b00,
    parameter ADDR_DATA_RX = 2'b01,
    parameter ADDR_CTRL_TX = 2'b10,
    parameter ADDR_CTRL_RX = 2'b11
)
   (
    // GPIF signals
    input gpif_clk,
    input gpif_rst,
    input gpif_enb,
    inout [31:0] gpif_d,
    input [3:0] gpif_ctl,
    output reg sloe,
    output reg slrd,
    output reg slwr,
    output slcs,
    output reg pktend,
    output reg [1:0] fifoadr,
    // FIFO interfaces
    input fifo_clk,
    input fifo_rst,
    // TX Data interface to DSP
    output [63:0] tx_tdata, output tx_tlast, output tx_tvalid, input tx_tready,
    // RX Data interface to DSP
    input [63:0] rx_tdata, input rx_tlast, input rx_tvalid, output rx_tready,
    // Incomming control interface
    output [63:0] ctrl_tdata, output ctrl_tlast, output ctrl_tvalid, input ctrl_tready,
    // Outgoing control interface
    input [63:0] resp_tdata, input resp_tlast, input resp_tvalid, output resp_tready,
    // Debug Signals
    output [31:0] debug
    );

   reg 		  fifo_nearly_full;
   wire 	  ctrl_tx_fifo_nearly_full, data_tx_fifo_nearly_full;
   wire 	  ctrl_tx_fifo_has_space, data_tx_fifo_has_space;


    assign slcs = 1'b0;

    //
    // DMA FIFO ready and watermark flags
    // These are double registered not for meta stability protection, but to make timing closure easier
    // since the first register is locked in the I/O pad.
    //
    reg fx3_ready, fx3_ready1, fx3_wmark, fx3_wmark1;
    always @(posedge gpif_clk) fx3_ready <= gpif_ctl[0];
    always @(posedge gpif_clk) fx3_wmark <= gpif_ctl[1];
    always @(posedge gpif_clk) fx3_ready1 <= fx3_ready;
    always @(posedge gpif_clk) fx3_wmark1 <= fx3_wmark;

    //
    // GPIF input and output data lines, tristate
    //
    reg [31:0] gpif_data_in, gpif_data_out;

    always @(posedge gpif_clk)
      if (~slrd2)
	// Update data register only when something useful is read.
	// Hold values until we know if they are end of packets for single beat reads.
    gpif_data_in <= gpif_d;

    assign gpif_d = sloe ? gpif_data_out[31:0] : 32'bz;

   // ////////////////////////////////////////////////////////////////////
   // GPIF bus master state machine

    wire wr_fifo_xfer, wr_fifo_eop;
    wire [31:0] wr_fifo_data;
    reg read_ready_go, write_ready_go;

    reg [3:0] state; //state machine current state
    localparam STATE_IDLE  = 0;
    localparam STATE_THINK = 1;
    localparam STATE_READ  = 2;
    localparam STATE_WRITE = 3;
    localparam STATE_WAIT = 4;
    localparam STATE_READ_FLUSH = 5;
    localparam STATE_WRITE_FLUSH = 6;
    localparam STATE_READ_SINGLE = 7;


    // General purpose pseudo-state counter.
    reg [2:0] idle_cycles;

    // Select next address (endpoint) to be processed
    reg [1:0] last_addr, next_addr;

    wire      local_fifo_ready;

    // Track size of a wriet burst to look for FX3 corner cases related to 2^n sized bursts.
    reg [15:0] transfer_size;

    // Read strobe pipeline.
    reg slrd1, slrd2, slrd3, slrd4, slrd5;

    always @(posedge gpif_clk)
     if (gpif_rst) begin
        slrd1 <= 1'b1;
        slrd2 <= 1'b1;
        slrd3 <= 1'b1;
        slrd4 <= 1'b1;
        slrd5 <= 1'b1;
     end else begin
        slrd1 <= slrd;
        slrd2 <= slrd1;
        slrd3 <= slrd2;
        slrd4 <= slrd3;
        slrd5 <= slrd4;
     end

   // End of packet pipeline for reads.
   reg 	rx_eop, rx_eop1, rx_eop2;

   // This pipeline tracks the end of a CHDR TX packet seperately from the local FIFO becoming full.
   // This is because a true packet end causes a tlast assertion to the FIFO, where as a full local FIFO only requires
   // the GPIF transaction to be ended before local FIFO overflow occurs.
    always @(posedge gpif_clk)
      if (gpif_rst) begin
        rx_eop1 <= 1;
        rx_eop2 <= 1;
      end else begin
        rx_eop2 <= rx_eop1;
        rx_eop1 <= rx_eop;
    end

   reg first_read;
   reg pad = 0;

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
        gpif_data_out <= 32'b0;
        idle_cycles <= 3'h0;
        fifoadr <= 0;
        first_read <= 1'b0;
        last_addr <= 2'b0;
        rx_eop <= 1'b0;
        transfer_size <= 1;
        pad <= 0;

    end
    else if (gpif_enb) begin
      case (state)

      //
      // Increment fifoadr to point at next thread, set all strobes to idle,
      //
        STATE_IDLE: begin
          sloe <= 1;
          slrd <= 1;
          slwr <= 1;
          pktend <= 1;
          gpif_data_out <= 32'b0;
          fifoadr <= next_addr;
          state <= STATE_WAIT;
          idle_cycles <= 3'h0;
       	  rx_eop <= 1'b0;
          first_read <= 1'b0;
        end

      //
      // If the current thread we are pointing at (fifoadr) can not immediately proceed
      // then quickly move to the next thread. Once we are pointing at a thread that can proceed locally
      // wait for 8 clock cycles to allow fifoadr to propogate to FX3, and corresponding flag state to
      // propogate back to FPGA and through resampling flops. At this point transition to STATE_THINK
      // to evaluate remote flag.
      //
        STATE_WAIT: begin
       // Current thread can proceed locally
          if (local_fifo_ready) begin
            idle_cycles <= idle_cycles + 1'b1;
          if (idle_cycles == 3'b111) state <= STATE_THINK; // Could shorten this delay, flags now stable for several clocks.
          end
       // ....move onto next thread.
          else begin
            idle_cycles <= 3'b0;
            //fifoadr <= fifoadr + 2'b1;
            fifoadr <= next_addr;

          end
        end

      //
      // Flags from FX3 now stable. Make a decision about what type of transaction to start.
      //
        STATE_THINK: begin
        // This is written like a priority encoder but in reality read_ready_go and
        // write_ready_go are mutually exclusive by design.
          if (fx3_ready1 && fx3_wmark1 && read_ready_go) begin
            state <= STATE_READ;
            slrd <= 0;
            rx_eop <= 1'b0;
            first_read <= 1'b1; // Set unconditional read flag to kick off transaction
            sloe <= 0; // FX3 drives the data bus.
          end else if (fx3_ready1 && ~fx3_wmark1 && read_ready_go) begin
            state <= STATE_READ_SINGLE;
            slrd <= 0;
            sloe <= 0; // FX3 drives the data bus.
          end else if (fx3_ready1 && write_ready_go && wr_fifo_eop && (transfer_size[7:0] == 0)) begin // remember that write_ready_go shows 1 cycle old status.
            // If an exact multiple of the native USB packet size (1K USB3, 512B USB2) has been transfered
            // and TLAST is asserted (but the transfer is less than a full FX3 DMA buffer - this is
            // indicated when the watermark will terminate the transfer in this case) then we will pad the packet
            // for one more cycle to ensure it does not get stuck in the FX3.
            pktend <= 1'b1; // Active low - De-asserted
            slwr <= 1'b0; //Active low - Asserted, write to FX3.
            transfer_size <= transfer_size + 1; // Increment transfer_size.
            gpif_data_out <= wr_fifo_data; // Always latch data from FIFO's into output register
            pad <= 1;
          end else if ((fx3_ready1 && write_ready_go && wr_fifo_eop) | pad) begin  // remember that write_ready_go shows 1 cycle old status.
            // Its the end of a CHDR packet and we are not on a FX3 corner case size.
            // Go IDLE with pktend and slwr asserted to write the last data.
            pktend <= 1'b0; // Active low - Asserted,
            state <= STATE_WRITE_FLUSH;
            idle_cycles <= 3'd5; // Stay in flush 3 cycles
            slwr <= 1'b0; // Active low - Asserted, write to FX3
            transfer_size <= 1; // End of packet will release FX3 DMA buffer, reset transfer size count.
            gpif_data_out <= wr_fifo_data; // Always latch data from FIFO's into output register
            pad <= 0; // Reset pad
          end else if (fx3_ready1 && write_ready_go) begin // remember that write_ready_go shows 1 cycle old status.
           // There is (an unknown amount of) data ready to send to FX from local FIFO.
            state <= STATE_WRITE;
            slwr <= 1'b0;  // Active low - Write strobe active
            gpif_data_out <= wr_fifo_data; // Always latch data from FIFO's into output register
            transfer_size <= transfer_size + 1; // Account for current cycles transfer
          end
          else begin
            state <= STATE_IDLE;
          end

            idle_cycles <= 3'h0;
            last_addr <= fifoadr;
        end // case: STATE_THINK

      // Got here because READY flag asserted but watermark deaaserted...QED there's less than the watermarks
      // worth of data to read from FX remaining in this DMA page. Need to do that with single beat reads
      // followed by rechecking the READY flag to see if it deassserted indicating that the page emptied.
      // Since we have the read data from FX3 earlier than we have a flag to inspect we keep the data in
      // gpif_data_in until we know if we are commiting it to the FIFO with or without an asserted TLAST.
      //
      STATE_READ_SINGLE: begin
        if (idle_cycles == 0) begin
    	// Deassert read strobe after reading single 32bit word
          slrd <= 1'b1;
          idle_cycles <= idle_cycles + 1;
	      end else if (idle_cycles == 5) begin
		// READY1 flag now reflect effects of last read.
        if (!fx3_ready1) begin
           state <= STATE_IDLE;
           sloe <= 1'b1;
        end else begin
        // Initiate another READ beat.
           state <= STATE_READ_SINGLE;
           slrd <= 1'b0;
        end
        idle_cycles <= 0;
        end else begin
          // All other idle_cycles counts.
          idle_cycles <= idle_cycles + 1;
        end
      end // case: STATE_READ_SINGLE


      // If flag first_read and ~slrd3 have gone deasserted
      // (meaning that the watermark deasserted 5 clock cycles ago or local FIFO full) transition to STATE_IDLE.
      // If watermark deasserted 2 cycles ago de-assert slrd ...read data is still traveling in the pipeline.
      // Whilst ~slrd3 stays asserted keep the first_read flag armed.
      // Trigger TLAST only for transfer ended by watermark (Which indicates a true packet end), not local full FIFO.
      STATE_READ: begin

        if (~fx3_wmark1 | fifo_nearly_full) begin
          // Either end of packet or local FIFO full is imminent, start shutting down this read burst.
          slrd <= 1'b1;  // Active low - Take read strobe inactive
          state <= STATE_READ_FLUSH;
        end else begin
           slrd <= 1'b0; // Active low - Keep read strobe active.
        end

        if (~fx3_wmark1)
          // Put TLAST into pipeline to mark end of packet
          rx_eop <= 1'b1;

        if (~slrd3)
          // Reset first_read flag as slrd assertion progresses down pipeline
         first_read <= 1'b0;
      end // case: STATE_READ

      // SLRD has been deasserted but data continues to flow from FX3 into FPGA until pipeline empties.
      STATE_READ_FLUSH: begin
        slrd <= 1'b1; // Active low - Keep read strobe inactive.
        rx_eop <= 1'b0; // EOP indication can be reset now - Already traveling in the pipeline if it was active.
        if (~slrd3)
          // Reset first_read flag as slrd assertion progresses down pipeline
          first_read <= 1'b0;
        if (!first_read && slrd3) begin // Active low signal
          // Last data of burst will be written to FIFO next clock edge so transition to IDLE also.
          state <= STATE_IDLE;
          sloe <= 1'b1; // Active low - Resume parking bus with FPGA driving.
        end
      end


      // Now in potential write burst. Exit this sate immediately if we are only doing a single beat write.
      // Can exit this state in several ways:
      // At EOP and on a USB packet boundery (1K for USB3, 512B for USB2) must pad packet for 1 clock cycle in
      // addition to simply asserting pktend.
      // Otherwise at EOP just send a short packet.
      // If local FIFO goes empty then we terminatethe burst without asserting pktend.
      STATE_WRITE: begin
        if (wr_fifo_eop && wr_fifo_xfer && (transfer_size[7:0] == 0)) begin

          // If an exact multiple of the native USB packet size (1K USB3, 512B USB2) has been transfered
          // and TLAST is asserted (but the transfer is less than a full FX3 DMA buffer - this is
          // indicated when the watermark will terminate the transfer in this case) then we will pad the packet
          // for one more cycle to ensure it does not get stuck in the FX3.
          pktend <= 1'b1; // Active low - De-asserted,
          slwr <= 1'b0; // Active low - Asserted, write to FX3
          transfer_size <= transfer_size + 1; // Increment transfer_size.
          pad <= 1;
       end else if ((wr_fifo_eop && wr_fifo_xfer) | pad) begin
          // Its the end of a CHDR packet and we are not on a FX3 corner case size.
          // Go IDLE with pktend and slwr asserted to write the last data.
          pktend <= 1'b0; // Active low - Asserted,
          state <= STATE_WRITE_FLUSH;
          idle_cycles <= 3'd5; // Stay in flush 3 cycles
          slwr <= 1'b0; // Active low - Asserted, write to FX3
          transfer_size <= 1; // End of packet will release FX3 DMA buffer, reset transfer size count.
          pad <= 0; //Reset pad
       end else if (wr_fifo_xfer) begin
          // Regular write beat as part of a burst.
          pktend <= 1'b1; // Active low - De-asserted,
          slwr <= 1'b0; // Active low - Asserted, write to FX3
          transfer_size <= transfer_size + 1; // Account for current cycles transfer
       end else begin // Implicit if (~wr_fifo_xfer)
          // This was either a single beat write (watermark was never asserted)
          // or the water mark just deasserted or we ran out of local data to send.
          // slwr will be deasserted and we transition to the flush state.
          state <= STATE_WRITE_FLUSH;
          idle_cycles <= 3'd6; // Stay in flush 2 cycles.
          pktend <= 1'b1; // Active low - De-asserted,
          slwr <= 1'b1; // Active low - Deasserted, don't write to FX3
       end

        gpif_data_out <= wr_fifo_data; // Always latch data from FIFO's into output register

       end // case: STATE_WRITE

      // Some FX3 timing diagrams seem to imply address should be held stable after transaction
      STATE_WRITE_FLUSH: begin
        slrd <= 1;
        slwr <= 1;
        pktend <= 1;
        gpif_data_out <= 32'b0;
        idle_cycles <= idle_cycles + 1'b1;
        if (idle_cycles == 3'b111) begin
          state <= STATE_IDLE;
        end
      end


        default: state <= STATE_IDLE;
        endcase
    end

   // ///////////////////////////////////////////////////////////////////
   // fifo signal assignments and enables

    //output from fifos - ready to xfer
    wire data_tx_tready, ctrl_tx_tready;
    wire ctrl_rx_tvalid, data_rx_tvalid;

    //Priority encoding for the the next address to service:
    //The next address to service is based on the readiness
    //of the internal fifos and last serviced fairness metric.
/* -----\/----- EXCLUDED -----\/-----
    always @(posedge gpif_clk) next_addr <=
        ((ctrl_rx_tvalid && (last_addr != ADDR_CTRL_RX))? ADDR_CTRL_RX :
        ((ctrl_tx_fifo_has_space && (last_addr != ADDR_CTRL_TX))? ADDR_CTRL_TX :
        ((data_rx_tvalid && (last_addr != ADDR_DATA_RX))? ADDR_DATA_RX :
        ((data_tx_fifo_has_space && (last_addr != ADDR_DATA_TX))? ADDR_DATA_TX :
        (fifoadr + 2'b1)
    ))));
 -----/\----- EXCLUDED -----/\----- */
   //always @(posedge gpif_clk) next_addr <= (fifoadr + 2'b1);

   // Sequence addresses 0->2->1->3->0......
   always @(posedge gpif_clk) {next_addr[0],next_addr[1]} <= ({fifoadr[0],fifoadr[1]} + 2'b1);

    //Help the FPGA search to only look for addrs that the FPGA is ready for
    assign local_fifo_ready =
        (ctrl_rx_tvalid && (fifoadr == ADDR_CTRL_RX)) ||
        (ctrl_tx_fifo_has_space && (fifoadr == ADDR_CTRL_TX)) ||
        (data_rx_tvalid && (fifoadr == ADDR_DATA_RX)) ||
        (data_tx_fifo_has_space && (fifoadr == ADDR_DATA_TX));

    // Local TX FIFO imminantly about to fill.
    always @(posedge gpif_clk) fifo_nearly_full <=
        (ctrl_tx_fifo_nearly_full && (fifoadr == ADDR_CTRL_TX)) ||
        (data_tx_fifo_nearly_full && (fifoadr == ADDR_DATA_TX));

    // There is enough space in local FIFO to RX an entire CHDR packet (sized for channel type)
    always @(posedge gpif_clk) read_ready_go <=
        (ctrl_tx_fifo_has_space && (fifoadr == ADDR_CTRL_TX)) ||
        (data_tx_fifo_has_space && (fifoadr == ADDR_DATA_TX));

    // The is data waiting to be sent to FX3 in local FIFO's
    always @(posedge gpif_clk) write_ready_go <=
        (ctrl_rx_tvalid && (fifoadr == ADDR_CTRL_RX)) ||
        (data_rx_tvalid && (fifoadr == ADDR_DATA_RX));

    //fifo xfer enable
    wire data_rx_tready = (
               ((state == STATE_WRITE) && fx3_wmark1 && ~pad) || // Sustain burst
               ((state == STATE_THINK) && fx3_ready1)    // First beat
               ) && (fifoadr == ADDR_DATA_RX) ;

    wire ctrl_rx_tready = (
               ((state == STATE_WRITE) && fx3_wmark1) || // Sustain burst
               ((state == STATE_THINK) && fx3_ready1)    // First beat
               ) && (fifoadr == ADDR_CTRL_RX) ;

    // Burst reads tap the read strobe pipeline at stage3, single beat reads at stage5.
    wire data_tx_tvalid = (
               (((state == STATE_READ) || (state == STATE_READ_FLUSH)) && ~slrd3) |
               ((state == STATE_READ_SINGLE) && ~slrd5)
               ) && (fifoadr == ADDR_DATA_TX);
    wire ctrl_tx_tvalid = (
               (((state == STATE_READ) || (state == STATE_READ_FLUSH)) && ~slrd3) |
               ((state == STATE_READ_SINGLE) && ~slrd5)
               ) && (fifoadr == ADDR_CTRL_TX);

    // The position of RX TLAST is known well in advance for bursts by monitoring the watermark. However for
    // single beat reads it can only be deduced after a read that causes the ready flag to go inactive.
    wire  data_ctrl_tx_tlast = ((state == STATE_READ_FLUSH) && rx_eop2) || ((state == STATE_READ_SINGLE) && ~fx3_ready1);


    //outputs from rx fifo paths
    wire ctrl_rx_tlast, data_rx_tlast;
    wire [31:0] ctrl_rx_tdata, data_rx_tdata;

    // There will be a RX FIFO transaction this cycle
    assign wr_fifo_xfer = (fifoadr == ADDR_CTRL_RX)? (ctrl_rx_tvalid && ctrl_rx_tready) : (data_rx_tvalid && data_rx_tready);
    // The RX FIFO transaction this cycle has TLAST set
    assign wr_fifo_eop =  (fifoadr == ADDR_CTRL_RX)? ctrl_rx_tlast : data_rx_tlast;
    // Route data from addressed RX FIFO towards FX3
    assign wr_fifo_data = (fifoadr == ADDR_CTRL_RX)? ctrl_rx_tdata : data_rx_tdata;

    wire ctrl_bus_error, tx_bus_error;

   // ////////////////////////////////////////////////////////////////////
   // TX Data Path
   wire [31:0] debug_data_fifo;

    gpif2_to_fifo64 #(.FIFO_SIZE(DATA_TX_FIFO_SIZE)) gpif2_to_fifo64_tx(
        .gpif_clk(gpif_clk), .gpif_rst(gpif_rst),
        .i_tdata(gpif_data_in), .i_tlast(data_ctrl_tx_tlast), .i_tvalid(data_tx_tvalid), .i_tready(data_tx_tready), // IJB. NOTE data_tx_tready currently unused.
        .fifo_clk(fifo_clk), .fifo_rst(fifo_rst),
        .fifo_nearly_full(data_tx_fifo_nearly_full), .fifo_has_space(data_tx_fifo_has_space),
        .o_tdata(tx_tdata), .o_tlast(tx_tlast), .o_tvalid(tx_tvalid), .o_tready(tx_tready),
        .bus_error(tx_bus_error), .debug(debug_data_fifo)
    );

   // ////////////////////////////////////////////
   // RX Data Path

    fifo64_to_gpif2 #(.FIFO_SIZE(DATA_RX_FIFO_SIZE)) fifo64_to_gpif2_rx(
        .fifo_clk(fifo_clk), .fifo_rst(fifo_rst),
        .i_tdata(rx_tdata), .i_tlast(rx_tlast), .i_tvalid(rx_tvalid), .i_tready(rx_tready),
        .gpif_clk(gpif_clk), .gpif_rst(gpif_rst),
        .o_tdata(data_rx_tdata), .o_tlast(data_rx_tlast), .o_tvalid(data_rx_tvalid), .o_tready(data_rx_tready)
    );

    // ////////////////////////////////////////////////////////////////////
    // CTRL path
    wire [31:0] debug_ctrl_fifo;

    gpif2_to_fifo64 #(.FIFO_SIZE(CTRL_TX_FIFO_SIZE)) gpif2_to_fifo64_ctrl(
        .gpif_clk(gpif_clk), .gpif_rst(gpif_rst),
        .i_tdata(gpif_data_in), .i_tlast(data_ctrl_tx_tlast), .i_tvalid(ctrl_tx_tvalid), .i_tready(ctrl_tx_tready), // IJB. NOTE data_tx_tready currently unused.
        .fifo_clk(fifo_clk), .fifo_rst(fifo_rst),
        .fifo_nearly_full(ctrl_tx_fifo_nearly_full), .fifo_has_space(ctrl_tx_fifo_has_space),
        .o_tdata(ctrl_tdata), .o_tlast(ctrl_tlast), .o_tvalid(ctrl_tvalid), .o_tready(ctrl_tready),
        .bus_error(ctrl_bus_error), .debug(debug_ctrl_fifo)
    );

   // ////////////////////////////////////////////////////////////////////
   // RESP path

    fifo64_to_gpif2 #(.FIFO_SIZE(CTRL_RX_FIFO_SIZE)) fifo64_to_gpif2_resp(
        .fifo_clk(fifo_clk), .fifo_rst(fifo_rst),
        .i_tdata(resp_tdata), .i_tlast(resp_tlast), .i_tvalid(resp_tvalid), .i_tready(resp_tready),
        .gpif_clk(gpif_clk), .gpif_rst(gpif_rst),
        .o_tdata(ctrl_rx_tdata), .o_tlast(ctrl_rx_tlast), .o_tvalid(ctrl_rx_tvalid), .o_tready(ctrl_rx_tready)
    );

   // ////////////////////////////////////////////
   //    DEBUG
/* -----\/----- EXCLUDED -----\/-----

   wire [35:0] CONTROL0;
   reg 	       wr_fifo_eop_debug;
   reg 	       read_ready_go_debug;
   reg 	       fifo_nearly_full_debug;
   reg 	       local_fifo_ready_debug;
   reg 	       slwr_debug;
   reg 	       slrd_debug;
   reg 	       sloe_debug;
   reg 	       pktend_debug;
   reg [1:0]   fifoadr_debug;
   reg 	       ep_wmark1_debug;
   reg 	       ep_ready1_debug;
   reg [3:0]   state_debug;
   reg 	       wr_fifo_xfer_debug;



   always @(posedge gpif_clk) begin
      wr_fifo_eop_debug <= wr_fifo_eop;
      read_ready_go_debug <= read_ready_go;
      fifo_nearly_full_debug <= fifo_nearly_full;
      local_fifo_ready_debug <= local_fifo_ready;
      wr_fifo_xfer_debug <= wr_fifo_xfer;
      slwr_debug <= slwr;
      slrd_debug <= slrd;
      sloe_debug <= sloe;
      pktend_debug <= pktend;
      fifoadr_debug[1:0] <= fifoadr;
      ep_wmark1_debug <= fx3_wmark1;
      ep_ready1_debug <= fx3_ready1;
      state_debug[3:0] <= state;
   end

   chipscope_ila_32 chipscope_ila_32_0 (
				      .CONTROL(CONTROL0), // INOUT BUS [35:0]
				      .CLK(gpif_clk), // IN
				      .TRIG0({
					      debug_data_fifo[5:0],
					      debug_ctrl_fifo[5:0],
 					      wr_fifo_eop_debug,
   					      read_ready_go_debug,
   					      fifo_nearly_full_debug,
   					      local_fifo_ready_debug,
					      wr_fifo_xfer_debug,
   					      slwr_debug,
   					      slrd_debug,
   					      sloe_debug,
   					      pktend_debug,
					      fifoadr_debug[1:0],
   					      ep_wmark1_debug,
   					      ep_ready1_debug,
					      state_debug[3:0]
   					      }) // IN BUS [31:0]
				      );

   chipscope_icon chipscope_icon_i0
     (
      .CONTROL0(CONTROL0) // INOUT BUS [35:0]
      );
 -----/\----- EXCLUDED -----/\----- */


endmodule // gpif2_slave_fifo32

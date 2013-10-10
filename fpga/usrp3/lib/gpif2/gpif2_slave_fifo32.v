//
// Copyright 2011-2013 Ettus Research LLC
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
    parameter ADDR_CTRL_RX = 2'b11,

    parameter END_WITH_COMMA = 0
)
   (// GPIF signals
    input gpif_clk, input gpif_rst, input gpif_enb,
    inout [31:0] gpif_d,
    input [3:0] gpif_ctl,
    output reg sloe,
    output reg slrd,
    output reg slwr,
    output slcs,
    output reg pktend,
    output reg [1:0] fifoadr,

    // FIFO interfaces
    input fifo_clk, input fifo_rst,

    output [63:0] tx_tdata, output tx_tlast, output tx_tvalid, input tx_tready,
    input [63:0] rx_tdata, input rx_tlast, input rx_tvalid, output rx_tready,
    output [63:0] ctrl_tdata, output ctrl_tlast, output ctrl_tvalid, input ctrl_tready,
    input [63:0] resp_tdata, input resp_tlast, input resp_tvalid, output resp_tready,

    output [31:0] debug
    );

   reg 		  fifo_nearly_full;
   wire 	  ctrl_tx_fifo_nearly_full, data_tx_fifo_nearly_full;
   wire 	  ctrl_tx_fifo_has_space, data_tx_fifo_has_space;
   

   wire [159:0]   debug_tx_data, debug_tx_ctrl;
   
    assign slcs = 1'b0;

    //DMA FIFO ready and watermark flags
    reg EP_READY, EP_READY1, EP_WMARK, EP_WMARK1;
    always @(posedge gpif_clk) EP_READY <= gpif_ctl[0];
    always @(posedge gpif_clk) EP_WMARK <= gpif_ctl[1];
    always @(posedge gpif_clk) EP_READY1 <= EP_READY;
    always @(posedge gpif_clk) EP_WMARK1 <= EP_WMARK;

    // GPIF output data lines, tristate
    reg [31:0] gpif_data_in, gpif_data_out;
    always @(posedge gpif_clk) gpif_data_in <= gpif_d;
    assign gpif_d = sloe ? gpif_data_out[31:0] : 32'bz;

   // ////////////////////////////////////////////////////////////////////
   // GPIF bus master state machine

    wire wr_fifo_xfer, wr_fifo_eof;
    wire [31:0] wr_fifo_data;
    reg read_ready_go, write_ready_go;
    reg wr_one, rd_one;

    reg [3:0] state; //state machine current state
    localparam STATE_IDLE  = 0;
    localparam STATE_THINK = 1;
    localparam STATE_READ  = 2;
    localparam STATE_WRITE = 3;
    localparam STATE_WAIT = 4;

    reg [2:0] idle_cycles;
    reg [1:0] last_addr, next_addr;
    wire local_fifo_ready;

    reg slrd1, slrd2, slrd3;
   
   always @(posedge gpif_clk)
     if (gpif_rst) begin
        slrd1 <= 1;
        slrd2 <= 1;
        slrd3 <= 1;
     end else begin
        slrd1 <= slrd;
        slrd2 <= slrd1;
        slrd3 <= slrd2;
     end
   
   wire RD_VALID = ~slrd3;
   wire RD_LAST = slrd2;
   wire WR_VALID = (EP_WMARK1 || !wr_one);

   // //////////////////////////////////////////////////////////////
   // FX2 slave FIFO bus master state machine
   //
    always @(posedge gpif_clk)
    if(gpif_rst) begin
        state <= STATE_IDLE;
        sloe <= 0;
        slrd <= 1;
        slwr <= 1;
        pktend <= 1;
        gpif_data_out <= 32'b0;
        idle_cycles <= 0;
        fifoadr <= 0;
        wr_one <= 1'b0;
        rd_one <= 1'b0;
        last_addr <= 2'b0;
    end
    else if (gpif_enb) begin
        case (state)

	  //
	  // Increment fifoadr to point at next thread, set all strobes to idle,
	  //
        STATE_IDLE: begin
            sloe <= 0;
            slrd <= 1;
            slwr <= 1;
            pktend <= 1;
            gpif_data_out <= 32'b0;
            fifoadr <= next_addr;
            state <= STATE_WAIT;
            idle_cycles <= 0;
        end

	  //
	  // now wait here for 8 clock cycles before transitioning to STATE_THINK.
	  // We stay in this state if no local FIFO's can proceed at this point.
	  //
        STATE_WAIT: begin
            if (local_fifo_ready) begin
                idle_cycles <= idle_cycles + 1'b1;
                if (idle_cycles == 3'b111) state <= STATE_THINK;
            end
            else begin
                idle_cycles <= 3'b0;
                fifoadr <= fifoadr + 2'b1;
            end
        end

	  //
	  // If there is a read to start, assert SLRD and SLOE and transition to STATE_READ.
	  // If there is a write to perform, set flags that says there is the possibility to do at least
	  // one write (wr_one) and transition to STATE_WRITE
	  //
        STATE_THINK: begin

            if (EP_READY1 && read_ready_go) begin
                state <= STATE_READ;
                slrd <= 0;
                rd_one <= 0;
            end
            else if (EP_READY1 && write_ready_go) begin
                state <= STATE_WRITE;
                sloe <= 1;
                wr_one <= 1'b0;
            end
            else begin
                state <= STATE_IDLE;
            end

            idle_cycles <= 0;
            last_addr <= fifoadr;
        end

	  // If flag rd_one is set (armed 5 cycles after slrd goes initialy assrted) and RD_VALID has gone deasserted
	  // (meaning that the watermark deasserted 5 clock cycles ago) transition to STATE_IDLE.
	  // If watermark deasserted 2 cycles ago de-assert slrd ...read data is still traveling in the pipeline.
	  // Whilst RD_VALID stays asserted keep the rd_one flag armed.
        STATE_READ: begin
            if (rd_one && ~RD_VALID) state <= STATE_IDLE;
            if (~EP_WMARK1 | fifo_nearly_full) slrd <= 1;
            if (RD_VALID) rd_one <= 1'b1;
        end

	  // If local FIFO goes empty or tlast is set then transition to STATE_IDLE
	  // Push local FIFO data out onto GPIF data bus.
	  // if local FIFO has valid data then assert slwr
	  // if local FIFO assertes tlast then assert pktend
	  // If WR_VALID asserted (because wr_one already asserted in the first cycle in this state)
	  // now clear wr_one (watermark will keep WR_VALID asserted from now on if this is a burst).
	  //
        STATE_WRITE: begin
            if (~wr_fifo_xfer || wr_fifo_eof) state <= STATE_IDLE;
            gpif_data_out <= wr_fifo_data;
            slwr <= ~wr_fifo_xfer;
            pktend <= ~wr_fifo_eof;
            if (WR_VALID) wr_one <= 1'b1;
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
    always @(posedge gpif_clk) next_addr <=
        ((ctrl_rx_tvalid && last_addr != ADDR_CTRL_RX)? ADDR_CTRL_RX :
        ((ctrl_tx_fifo_has_space && last_addr != ADDR_CTRL_TX)? ADDR_CTRL_TX :
        ((data_rx_tvalid && last_addr != ADDR_DATA_RX)? ADDR_DATA_RX :
        ((data_tx_fifo_has_space && last_addr != ADDR_DATA_TX)? ADDR_DATA_TX :
        (fifoadr + 2'b1)
    ))));

    //Help the FPGA search to only look for addrs that the FPGA is ready for
    assign local_fifo_ready =
        (ctrl_rx_tvalid && (fifoadr == ADDR_CTRL_RX)) ||
        (ctrl_tx_fifo_has_space && (fifoadr == ADDR_CTRL_TX)) ||
        (data_rx_tvalid && (fifoadr == ADDR_DATA_RX)) ||
        (data_tx_fifo_has_space && (fifoadr == ADDR_DATA_TX));

    always @(posedge gpif_clk) fifo_nearly_full <=
        (ctrl_tx_fifo_nearly_full && (fifoadr == ADDR_CTRL_TX)) ||
        (data_tx_fifo_nearly_full && (fifoadr == ADDR_DATA_TX));

    always @(posedge gpif_clk) read_ready_go <=
        (ctrl_tx_fifo_has_space && (fifoadr == ADDR_CTRL_TX)) ||
        (data_tx_fifo_has_space && (fifoadr == ADDR_DATA_TX));

    always @(posedge gpif_clk) write_ready_go <=
        (ctrl_rx_tvalid && (fifoadr == ADDR_CTRL_RX)) ||
        (data_rx_tvalid && (fifoadr == ADDR_DATA_RX));

    //fifo xfer enable
    wire data_rx_tready = (state == STATE_WRITE) && (fifoadr == ADDR_DATA_RX) && WR_VALID;
    wire ctrl_rx_tready = (state == STATE_WRITE) && (fifoadr == ADDR_CTRL_RX) && WR_VALID;
    wire data_tx_tvalid = (state == STATE_READ) && (fifoadr == ADDR_DATA_TX) && RD_VALID;
    wire ctrl_tx_tvalid = (state == STATE_READ) && (fifoadr == ADDR_CTRL_TX) && RD_VALID;

    //outputs from rx fifo paths
    wire ctrl_rx_tlast, data_rx_tlast;
    wire [31:0] ctrl_rx_tdata, data_rx_tdata;

    //mux rx outputs for gpif state machine
    assign wr_fifo_xfer = (fifoadr == ADDR_CTRL_RX)? (ctrl_rx_tvalid && ctrl_rx_tready) : (data_rx_tvalid && data_rx_tready);
    assign wr_fifo_eof = wr_fifo_xfer && ((fifoadr == ADDR_CTRL_RX)? ctrl_rx_tlast : data_rx_tlast);
    assign wr_fifo_data = (fifoadr == ADDR_CTRL_RX)? ctrl_rx_tdata : data_rx_tdata;

    wire ctrl_bus_error, tx_bus_error;

   // ////////////////////////////////////////////////////////////////////
   // TX Data Path

    gpif2_to_fifo64 #(.FIFO_SIZE(DATA_TX_FIFO_SIZE)) gpif2_to_fifo64_tx(
        .gpif_clk(gpif_clk), .gpif_rst(gpif_rst),
        .i_tdata(gpif_data_in), .i_tlast(RD_LAST), .i_tvalid(data_tx_tvalid), .i_tready(data_tx_tready),
        .fifo_clk(fifo_clk), .fifo_rst(fifo_rst), 
        .fifo_nearly_full(data_tx_fifo_nearly_full), .fifo_has_space(data_tx_fifo_has_space),
        .o_tdata(tx_tdata), .o_tlast(tx_tlast), .o_tvalid(tx_tvalid), .o_tready(tx_tready),
        .bus_error(tx_bus_error), .debug(debug_tx_data)
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

    gpif2_to_fifo64 #(.FIFO_SIZE(CTRL_TX_FIFO_SIZE)) gpif2_to_fifo64_ctrl(
        .gpif_clk(gpif_clk), .gpif_rst(gpif_rst),
        .i_tdata(gpif_data_in), .i_tlast(RD_LAST), .i_tvalid(ctrl_tx_tvalid), .i_tready(ctrl_tx_tready),
        .fifo_clk(fifo_clk), .fifo_rst(fifo_rst), 
        .fifo_nearly_full(ctrl_tx_fifo_nearly_full), .fifo_has_space(ctrl_tx_fifo_has_space),
        .o_tdata(ctrl_tdata), .o_tlast(ctrl_tlast), .o_tvalid(ctrl_tvalid), .o_tready(ctrl_tready),
        .bus_error(ctrl_bus_error), .debug(debug_tx_ctrl)
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

    wire [31:0] debug0 = {
        sloe, slrd, slwr, pktend, fifoadr, EP_READY, EP_WMARK, //8
        state, //4
        data_tx_tvalid, data_tx_tready, data_rx_tvalid, data_rx_tready, //4
        gpif_d[15:0] //16
    };

    reg [31:0] debug_reg0;
    reg [31:0] debug_reg1;
    reg [31:0] debug_reg2;
    always @(posedge gpif_clk) debug_reg0 <= debug0;
    always @(posedge gpif_clk) debug_reg1 <= debug_reg0;
    always @(posedge gpif_clk) debug_reg2 <= debug_reg1;
    assign debug = debug_reg2;

   wire [37:0] debug_resp = {
			     resp_tlast,        // 37
			     resp_tready,       // 36
			     resp_tvalid,       // 35
			     ctrl_rx_tlast,     // 34
			     ctrl_rx_tready,    // 33
			     ctrl_rx_tvalid,    // 32
			     ctrl_rx_tdata      // 31:0
			     };
   
   
    reg [255:0] debug1,debug2;
   
    always @(posedge gpif_clk) debug1 <= {debug_resp,debug_tx_ctrl,debug0};
   always @(posedge gpif_clk) debug2 <= debug1;
   
   
 
   wire [35:0] CONTROL0,CONTROL1;
 /*
   chipscope_ila_32 chipscope_ila_32(
				   .CONTROL(CONTROL0), // INOUT BUS [35:0]
				   .CLK(gpif_clk), // IN
				   .TRIG0(debug2) // IN BUS [31:0]
				   );
   
   chipscope_ila_128  chipscope_ila_128(
				   .CONTROL(CONTROL1), // INOUT BUS [35:0]
				   .CLK(fifo_clk), // IN
				   .TRIG0({debug4,debug6}) // IN BUS [31:0]
				   );
   
   
   chipscope_ila_256 chipscope_ila_256(
				     .CONTROL(CONTROL0), // INOUT BUS [35:0]
				     .CLK(gpif_clk), // IN
				     .TRIG0(debug2) // IN BUS [31:0]
				     );
  
  chipscope_ila_32  chipscope_ila_32_2(
				   .CONTROL(CONTROL1), // INOUT BUS [35:0]
				   .CLK(gpif_clk), // IN
				   .TRIG0(32'd0) // IN BUS [31:0]
				   );
   
   chipscope_icon  chipscope_icon(
				 .CONTROL0(CONTROL0), // INOUT BUS [35:0]
				   .CONTROL1(CONTROL1) // INOUT BUS [35:0]
				    );

   */
endmodule // gpif2_slave_fifo32

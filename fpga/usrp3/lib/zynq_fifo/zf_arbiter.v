//////////////////////////////////////////////////////////////////////////////////
// Copyright Ettus Research LLC
// Copyright 2014 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// The ZYNQ FIFO configuration arbiter:
//  - holds configuration memory addresses
//  - setting and readback for slave state machines
//  - stream of memory addresses for master state machines
//////////////////////////////////////////////////////////////////////////////////


module zf_arbiter
#(
    parameter STREAMS_WIDTH = 2,
    parameter CMDFIFO_DEPTH = 4,
    parameter PAGE_WIDTH = 16,
    parameter USE_INT_STREAM_SEL = 0    // Use internal round robin stream selection
)
(
    input clk,
    input rst,

    //------------------------------------------------------------------
    //-- settings interface
    //------------------------------------------------------------------
    input [31:0] set_addr,
    input [31:0] set_data,
    input set_stb,

    //------------------------------------------------------------------
    //-- readback interface
    //------------------------------------------------------------------
    input [31:0] rb_addr,
    output [31:0] rb_data,
    input rb_stb,

    //------------------------------------------------------------------
    //-- fifo interactive interface
    //------------------------------------------------------------------
    output [71:0] cmd_tdata,
    output cmd_tvalid,
    input cmd_tready,
    input [7:0] sts_tdata,
    input sts_tvalid,
    output sts_tready,

    //------------------------------------------------------------------
    //-- which stream to process? externally provided if USE_INT_STREAM_SEL = 0
    //------------------------------------------------------------------
    input [STREAMS_WIDTH-1:0] ext_stream_sel,
    input ext_stream_valid,

    output [31:0] debug
);

////////////////////////////////////////////////////////////////////////
///////////////////////////// Begin R T L //////////////////////////////
////////////////////////////////////////////////////////////////////////

    localparam NUM_STREAMS = (1 << STREAMS_WIDTH);

    reg [STREAMS_WIDTH-1:0] which_stream;

    //readback mux assignment
    reg [31:0] rb_data_i [NUM_STREAMS-1:0];
    assign rb_data = rb_data_i[rb_addr[STREAMS_WIDTH+4:5]];

    //cmd + sts fifo mux signals
    wire [72:0] cmd_data_i [NUM_STREAMS-1:0];
    assign cmd_tdata = cmd_data_i[which_stream];
    wire cmd_tvalid_i [NUM_STREAMS-1:0];
    wire sts_tready_i [NUM_STREAMS-1:0];

    ////////////////////////////////////////////////////////////////////
    // state machine for driving fifo arbitration
    ////////////////////////////////////////////////////////////////////
    localparam STATE_SET_WHICH_STREAM = 0;
    localparam STATE_ASSERT_DO_CMD = 1;
    localparam STATE_ASSERT_DO_STS = 2;
    localparam STATE_SOME_IDLE = 3;

    reg [1:0] state;

    wire [STREAMS_WIDTH-1:0] stream_sel;
    wire stream_valid;

    generate
    if (USE_INT_STREAM_SEL) begin
        reg [STREAMS_WIDTH-1:0] int_stream_sel;
        always @(posedge clk) begin
            if (rst) begin
                int_stream_sel <= 0;
            end else begin
                if (state == STATE_SET_WHICH_STREAM) begin
                    if (int_stream_sel < NUM_STREAMS-1) begin
                        int_stream_sel <= int_stream_sel + 1;
                    end else begin
                        int_stream_sel <= 0;
                    end
                end
            end
        end
        assign stream_sel = int_stream_sel;
        assign stream_valid = 1'b1;
    end else begin
        assign stream_sel = ext_stream_sel;
        assign stream_valid = ext_stream_valid;
    end
    endgenerate

    always @(posedge clk) begin
        if (rst) begin
            state <= STATE_SET_WHICH_STREAM;
            which_stream <= 0;
        end
        else case (state)

        STATE_SET_WHICH_STREAM: begin
            if (cmd_tvalid_i[stream_sel]) state <= STATE_ASSERT_DO_CMD;
            which_stream <= stream_sel;
        end

        STATE_ASSERT_DO_CMD: begin
            if (cmd_tvalid && cmd_tready) state <= STATE_ASSERT_DO_STS;
        end

        STATE_ASSERT_DO_STS: begin
            if (sts_tvalid && sts_tready) state <= STATE_SOME_IDLE;
        end

        STATE_SOME_IDLE: begin
            state <= STATE_SET_WHICH_STREAM;
        end

        default: state <= STATE_SET_WHICH_STREAM;

        endcase //state
    end

    ////////////////////////////////////////////////////////////////////
    // memory map + fifos for the host control/status
    ////////////////////////////////////////////////////////////////////

    wire do_cmd = (state == STATE_ASSERT_DO_CMD);
    wire do_sts = (state == STATE_ASSERT_DO_STS);

    assign cmd_tvalid = do_cmd && cmd_tvalid_i[which_stream];
    assign sts_tready = do_sts && sts_tready_i[which_stream];

    genvar i;
    generate
    for (i=0; i < NUM_STREAMS; i=i+1) begin : stream_circuit

        wire [PAGE_WIDTH-3:0] set_addr32 = set_addr[PAGE_WIDTH-1:2];
        wire write_clear = set_stb && (set_addr32 == (0 + i*8));
        wire write_addr  = set_stb && (set_addr32 == (1 + i*8));
        wire write_size  = set_stb && (set_addr32 == (2 + i*8));
        wire write_sts_rdy  = set_stb && (set_addr32 == (3 + i*8));
        wire write_sts      = set_stb && (set_addr32 == (4 + i*8));

        wire [PAGE_WIDTH-3:0] rb_addr32 = rb_addr[PAGE_WIDTH-1:2];
        wire read_sig            = (rb_addr32 == (0 + i*8));
        wire read_status         = (rb_addr32 == (4 + i*8));
        wire read_sts_occupied   = (rb_addr32 == (5 + i*8));
        wire read_cmd_addr_space = (rb_addr32 == (6 + i*8));
        wire read_cmd_size_space = (rb_addr32 == (7 + i*8));

        wire [15:0] sts_occupied, cmd_addr_space, cmd_size_space;
        wire [7:0] sts_readback;

        wire [15:0] this_stream = i;
        always @* begin
            if (read_sig) rb_data_i[i] <= {16'hACE0, this_stream};
            else if (read_status) rb_data_i[i] <= {24'b0, sts_readback};
            else if (read_sts_occupied) rb_data_i[i] <= {16'b0, sts_occupied};
            else if (read_cmd_addr_space) rb_data_i[i] <= {16'b0, cmd_addr_space};
            else if (read_cmd_size_space) rb_data_i[i] <= {16'b0, cmd_size_space};
            else rb_data_i[i] <= 32'h12345678;
        end

        wire [31:0] cmd_addr, cmd_size;
        wire cmd_addr_tvalid, cmd_size_tvalid;
        assign cmd_data_i[i][32+39:32+36] = 4'b0; //reserved - 0?
        assign cmd_data_i[i][32+35:64] = i[3:0]; //tag
        assign cmd_data_i[i][63:32] = cmd_addr;
        assign cmd_data_i[i][31] = 1'b0; //DRE ReAlignment Request
        assign cmd_data_i[i][30] = 1'b1; //always EOF for tlast stream
        assign cmd_data_i[i][29:24] = 6'b0; //DRE Stream Alignment
        assign cmd_data_i[i][23] = 1'b1; // Transfer type, 0 = No addr incr / FIFO mode, 1 = incr addr
        assign cmd_data_i[i][22:0] = cmd_size[22:0];

        axi_fifo #(.WIDTH(32), .SIZE(CMDFIFO_DEPTH)) crl_addr_fifo
        (
            .clk(clk), .reset(rst), .clear(write_clear),
            .i_tdata(set_data), .i_tvalid(write_addr), .i_tready(), .space(cmd_addr_space),
            .o_tdata(cmd_addr), .o_tvalid(cmd_addr_tvalid), .o_tready(cmd_tready && do_cmd && (which_stream == i)), .occupied()
        );

        axi_fifo #(.WIDTH(32), .SIZE(CMDFIFO_DEPTH)) crl_size_fifo
        (
            .clk(clk), .reset(rst), .clear(write_clear),
            .i_tdata(set_data), .i_tvalid(write_size), .i_tready(), .space(cmd_size_space),
            .o_tdata(cmd_size), .o_tvalid(cmd_size_tvalid), .o_tready(cmd_tready && do_cmd && (which_stream == i)), .occupied()
        );

        assign cmd_tvalid_i[i] = cmd_addr_tvalid && cmd_size_tvalid && stream_valid;

        wire dm_sts_tvalid = sts_tvalid && do_sts && (which_stream == i);

        axi_fifo #(.WIDTH(8), .SIZE(CMDFIFO_DEPTH)) sts_fifo
        (
            .clk(clk), .reset(rst), .clear(),
            .i_tdata((write_sts)?set_data[7:0]:sts_tdata), .i_tvalid(dm_sts_tvalid || write_sts), .i_tready(sts_tready_i[i]), .space(),
            .o_tdata(sts_readback), .o_tvalid(), .o_tready(write_sts_rdy), .occupied(sts_occupied)
        );

    end
    endgenerate

    assign debug[1:0] = state;
    assign debug[4] = cmd_tvalid;
    assign debug[5] = cmd_tready;
    assign debug[6] = sts_tvalid;
    assign debug[7] = sts_tready;
    assign debug[8] = stream_sel;
    assign debug[9] = which_stream;
    assign debug[15] = rb_addr[STREAMS_WIDTH+4:5];

endmodule //zf_arbiter

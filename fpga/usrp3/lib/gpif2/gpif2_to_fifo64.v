//
// Copyright 2012-2013 Ettus Research LLC
//


module gpif2_to_fifo64
#(
    parameter FIFO_SIZE = 9
)
(
    //input interface
    input gpif_clk, input gpif_rst,
    input [31:0] i_tdata,
    input i_tlast,
    input i_tvalid,
    output i_tready,
    output fifo_has_space,
    output fifo_nearly_full,
 
    //output fifo interface
    input fifo_clk, input fifo_rst,
    output [63:0] o_tdata,
    output o_tlast,
    output o_tvalid,
    input o_tready,

    output bus_error,
    output [159:0] debug
);

    wire [31:0] int_tdata;
    wire int_tlast;
    wire int_tvalid, int_tready;

    wire [31:0] int0_tdata; wire int0_tlast, int0_tvalid, int0_tready;

    //this fifo provides a space signal so we know a burst is possible
    localparam BURST_SIZE = (FIFO_SIZE < 8)? FIFO_SIZE : 8;
    wire [15:0] space;
  
   
    assign fifo_has_space = space >= (1 << BURST_SIZE);
    assign  fifo_nearly_full = (space < 6); // 5 spaces left.
   
    axi_fifo #(.WIDTH(33), .SIZE(0)) ingress_timing_fifo
    (
        .clk(gpif_clk), .reset(gpif_rst), .clear(1'b0),
        .i_tdata({i_tlast, i_tdata}), .i_tvalid(i_tvalid), .i_tready(i_tready), .space(),
        .o_tdata({int0_tlast, int0_tdata}), .o_tvalid(int0_tvalid), .o_tready(int0_tready), .occupied()
    );
    axi_fifo #(.WIDTH(33), .SIZE(BURST_SIZE)) min_read_buff
    (
        .clk(gpif_clk), .reset(gpif_rst), .clear(1'b0),
        .i_tdata({int0_tlast, int0_tdata}), .i_tvalid(int0_tvalid), .i_tready(int0_tready), .space(space),
        .o_tdata({int_tlast, int_tdata}), .o_tvalid(int_tvalid), .o_tready(int_tready), .occupied()
    );
  
 reg 	   input_write_error;
   
   always @(posedge gpif_clk)  input_write_error <= i_tvalid & ~i_tready;
   

    wire [31:0] chk_tdata;
    wire chk_tlast;
    wire chk_tvalid, chk_tready;

    axi_fifo_2clk #(.WIDTH(33), .SIZE(0/*SRL*/)) cross_clock_fifo
    (
        .reset(fifo_rst | gpif_rst),
        .i_aclk(gpif_clk), .i_tdata({int_tlast, int_tdata}), .i_tvalid(int_tvalid), .i_tready(int_tready),
        .o_aclk(fifo_clk), .o_tdata({chk_tlast, chk_tdata}), .o_tvalid(chk_tvalid), .o_tready(chk_tready)
    );

    wire [31:0] o32_tdata;
    wire o32_tlast;
    wire o32_tvalid, o32_tready;

    //reframes a tlast from the vita header - and drops bad packets
    //*
    gpif2_error_checker #(.SIZE(FIFO_SIZE)) checker
    (
        .clk(fifo_clk), .reset(fifo_rst), .clear(1'b0),
        .i_tdata(chk_tdata), .i_tlast(chk_tlast), .i_tvalid(chk_tvalid), .i_tready(chk_tready),
        .o_tdata(o32_tdata), .o_tlast(o32_tlast), .o_tvalid(o32_tvalid), .o_tready(o32_tready),
        .bus_error(bus_error), .debug(debug[63:0])
    );
    //*/
    //assign o32_tdata = chk_tdata;
    //assign o32_tlast = chk_tlast;
    //assign o32_tvalid = chk_tvalid;
    //assign chk_tready = o32_tready;

    axi_fifo32_to_fifo64 fifo32_to_fifo64
    (
        .clk(fifo_clk), .reset(fifo_rst), .clear(1'b0),
        .i_tdata(o32_tdata), .i_tuser(2'b0/*always 32 bits*/), .i_tlast(o32_tlast), .i_tvalid(o32_tvalid), .i_tready(o32_tready),
        .o_tdata(o_tdata), .o_tuser(/*ignored cuz vita has len*/), .o_tlast(o_tlast), .o_tvalid(o_tvalid), .o_tready(o_tready)
    );

  assign debug[159:64] = {
			  fifo_nearly_full, // 146
			  space[9:0],      // 145:136
			  input_write_error, // 135
			  int_tlast,       // 134
			  int_tready,      // 133
			  int_tvalid,      // 132
			  i_tlast,         // 131
			  i_tready,    // 130
			  fifo_has_space,        // 129
			  i_tvalid,        // 128
			  int_tdata[31:0], // 127:96
			  i_tdata[31:0]    // 95:64
			  };

   
   

endmodule //fifo_to_gpif2

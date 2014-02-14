//
// Copyright 2013 Ettus Research LLC
//


`timescale 500ps/1ps

module pcie_dma_ctrl_tb();
    reg clk    = 0;
    reg reset  = 1;

    always #10 clk = ~clk;

    initial begin
        #100 reset = 0;
        #200000;
        $finish;
    end
   
    function [63:0] iop2_msg_write;
        input [19:0]    address;
        input [31:0]    data;
        input           half_wd;
    begin
        //            {rd_response, wr_request, rd_request, half_word, 8'h00, address, data};
        iop2_msg_write = {1'b0,        1'b1,       1'b0,       half_wd,   8'h00, address, data};
    end
    endfunction // iop2_msg_write

    function [63:0] iop2_msg_read;
        input [19:0]    address;
        input           half_wd;
    begin
        //            {rd_response, wr_request, rd_request, half_word, 8'h00, address, data};
        iop2_msg_read = {1'b0,        1'b0,       1'b1,       half_wd,   8'h00, address, 32'h0};
    end
    endfunction // iop2_msg_read

    wire [3:0]      clear;
    wire [63:0]     frame_size;
    reg [3:0]       pkt_stb = 0;
    reg [3:0]       samp_stb = 0;
    reg [3:0]       error = 0;
    reg [7:0]       rtr_sid = 4;
    wire [3:0]      rtr_dst;

    reg [63:0]  regi_tdata;
    reg         regi_tvalid;
    wire        regi_tready;
    wire [63:0] rego_tdata;
    wire        rego_tvalid;
    reg         rego_tready;
    reg [31:0]  rego_payload;
    
    always @(posedge clk)
        if (rego_tdata[63] & rego_tvalid & rego_tready)
            rego_payload <= rego_tdata[31:0];

    initial begin
        regi_tvalid <= 0;
        rego_tready <= 0;
        while (reset) @(posedge clk);
    
        rego_tready <= 1;
        @(posedge clk);
        
        regi_tdata <= iop2_msg_write(20'h304, 32'hA, 0);
        regi_tvalid <= 1;
        @(posedge clk);
        while (~regi_tready) @(posedge clk);
        regi_tvalid <= 0;
        @(posedge clk);
    
    end // initial begin
    
    pcie_dma_ctrl #(
        .NUM_STREAMS(4), .FRAME_SIZE_W(16),
        .REG_BASE_ADDR(20'h00200), .ENABLE_ROUTER(1),
        .ROUTER_SID_W(8), .ROUTER_DST_W(4)
    ) dut (
        .clk(clk), .reset(reset),
        .regi_tdata(regi_tdata), .regi_tvalid(regi_tvalid), .regi_tready(regi_tready),
        .rego_tdata(rego_tdata), .rego_tvalid(rego_tvalid), .rego_tready(rego_tready),
        .set_clear(clear), .set_frame_size(frame_size), .sample_stb(samp_stb), .packet_stb(pkt_stb), 
        .stream_err(error), .rtr_sid(rtr_sid), .rtr_dst(rtr_dst)
    );
    

endmodule

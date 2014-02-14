//
// Copyright 2013 Ettus Research LLC
//


`timescale 500ps/1ps

module pcie_iop2_msg_arbiter_tb();
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

    reg [63:0]  msgi_tdata;
    wire [63:0] msgo_tdata;
    wire        msgo_tvalid, msgi_tready;
    reg         msgo_tready, msgi_tvalid;

    wire [63:0] basic_regi_tdata, zpu_regi_tdata;
    wire        basic_regi_tvalid, zpu_regi_tvalid;
    reg         basic_regi_tready, zpu_regi_tready;
    reg [63:0]  basic_rego_tdata, zpu_rego_tdata;
    reg         basic_rego_tvalid, zpu_rego_tvalid;
    wire        basic_rego_tready, zpu_rego_tready;

    initial begin
        //@TODO: Make this a self-checking TB
        while (reset) @(posedge clk);
    
        msgo_tready <= 1;
        basic_regi_tready <= 1;
        @(posedge clk);
        
        
        msgi_tdata <= iop2_msg_write(20'h0, 32'hDEAD, 0);
        msgi_tvalid <= 1;
        while (~msgi_tready) @(posedge clk);
        msgi_tvalid <= 0;
        @(posedge clk);
        
        
        msgi_tdata <= iop2_msg_read(20'h00000, 0);
        msgi_tvalid <= 1;
        while (~msgi_tready) @(posedge clk);
        msgi_tvalid <= 0;
        @(posedge clk);

        zpu_rego_tdata <= {1, 31'h0, 32'h12345678};
        zpu_rego_tvalid <= 1;
        while (~zpu_rego_tready) @(posedge clk);
        zpu_rego_tvalid <= 0;
        


    end // initial begin
    
    pcie_iop2_msg_arbiter #(
        .E0_ADDR(20'h00000), .E0_MASK(20'hFFF00),     //0x00000 - 0x000FF: Basic PCIe registers
        .E1_ADDR(20'h00100), .E1_MASK(20'hFFF00),     //0x00100 - 0x001FF: PCIe router registers
        .E2_ADDR(20'h00200), .E2_MASK(20'hFFE00),     //0x00200 - 0x003FF: DMA stream registers
        .E3_ADDR(20'h40000), .E3_MASK(20'hC0000)      //0x40000 - 0x7FFFF: Client address space 
    ) iop2_msg_arbiter (
        .clk(clk), .reset(reset),
        //Master
        .regi_tdata(msgi_tdata), .regi_tvalid(msgi_tvalid), .regi_tready(msgi_tready),
        .rego_tdata(msgo_tdata), .rego_tvalid(msgo_tvalid), .rego_tready(msgo_tready),
        //Endpoint 0
        .e0_regi_tdata(basic_regi_tdata), .e0_regi_tvalid(basic_regi_tvalid), .e0_regi_tready(basic_regi_tready),
        .e0_rego_tdata(basic_rego_tdata), .e0_rego_tvalid(basic_rego_tvalid), .e0_rego_tready(basic_rego_tready),
        //Endpoint 1
        .e1_regi_tdata(), .e1_regi_tvalid(), .e1_regi_tready(1'b1),
        .e1_rego_tdata(64'h0), .e1_rego_tvalid(1'b0), .e1_rego_tready(),
        //Endpoint 2
        .e2_regi_tdata(), .e2_regi_tvalid(), .e2_regi_tready(1'b1),
        .e2_rego_tdata(64'h0), .e2_rego_tvalid(1'b0), .e2_rego_tready(),
        //Endpoint 3
        .e3_regi_tdata(zpu_regi_tdata), .e3_regi_tvalid(zpu_regi_tvalid), .e3_regi_tready(zpu_regi_tready),
        .e3_rego_tdata(zpu_rego_tdata), .e3_rego_tvalid(zpu_rego_tvalid), .e3_rego_tready(zpu_rego_tready)
    );
    

endmodule

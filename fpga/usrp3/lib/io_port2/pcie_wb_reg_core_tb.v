//
// Copyright 2013 Ettus Research LLC
//


`timescale 500ps/1ps
`define CHECK_VALUE(val, expected, report) \
        if (val == expected) \
            $display("%s...Passed",report); \
        else \
            $display("%s...FAILED!!! (Val=0x%x, Exp=0x%x)",report,val,expected); \

module pcie_wb_reg_core_tb();
    reg         clk = 0, reset = 1; 
    reg         wb_stb_i = 0;
    reg         wb_we_i = 0;
    reg  [15:0] wb_adr_i = 0;
    reg  [31:0] wb_dat_i = 0;
    wire        wb_ack_o;
    wire [31:0] wb_dat_o;

    wire [63:0] msgo_data;
    wire        msgo_valid;
    reg         msgo_ready = 0;
    reg  [63:0] msgi_data = 0;
    reg         msgi_valid = 0;
    wire        msgi_ready;
    
    reg  [31:0] msgo_payload = 32'h0;
    reg  [31:0] msgo_ctrl = 32'h0;
    
    reg  [31:0] it;

    always #10 clk = ~clk;

    initial begin
        #100 reset = 0;
        #200000;
        $finish;
    end
   
    localparam READ     = 3'b001;
    localparam WRITE    = 3'b010;
    localparam RESPONSE = 3'b100;
   
    task pcie_send;
        input [2:0]     op;
        input [19:0]    address;
        input [31:0]    data;
    begin
        //{rd_resp, wr_request, rd_request, half_word, 8'h00, address, data};
        msgi_data  <= {op, 1'b0, 8'h00, address, data};
        msgi_valid <= 1'b1;

        @(posedge clk);
        while (~msgi_ready) @(posedge clk);
        
        msgi_valid <= 1'b0;
        @(posedge clk);
    end
    endtask // pcie_send

    task pcie_recv;
        input [2:0]     op;
        input [19:0]    address;
        input [31:0]    data;
    begin
        while (~msgo_valid) @(posedge clk);

        msgo_ready <= 1'b1;
        @(posedge clk);
        
        if (msgo_data[63] == op[2] || (msgo_data[62:61] == op[1:0] && msgo_data[51:32] == address))
            msgo_payload <= msgo_data[31:0];
            msgo_ctrl <= msgo_data[63:32];

        msgo_ready <= 1'b0;
        @(posedge clk);
    end
    endtask // pcie_recv

    task wb_send;
        input [2:0]     op;
        input [15:0]    address;
        input [31:0]    data;
    begin
        wb_adr_i <= address;
        wb_dat_i <= data;
        wb_we_i <= op[1];
        wb_stb_i <= 1'b1;
        
        @(posedge clk);
        while (~wb_ack_o) @(posedge clk);
        
        wb_stb_i <= 1'b0;
    end
    endtask // pcie_send


    initial begin
        msgo_ready <= 1'b0;
        msgi_valid <= 1'b0;
        while (reset) @(posedge clk);    
        @(posedge clk);
        
        $display("\n[TEST] ZPU Read from PCIe");
        pcie_send(WRITE, 20'h6a000, 32'h0);
        `CHECK_VALUE(msgo_payload, 32'h0, "Verify PCIe readback before initiating read request");
        pcie_send(READ, 20'h6a000, 32'h0);
        pcie_recv(RESPONSE, 20'h0, 20'h0);
        `CHECK_VALUE(msgo_payload, 32'h1, "Verify PCIe status after initiating read");
        pcie_send(READ, 20'h6a000, 32'h0);
        pcie_recv(RESPONSE, 20'h0, 20'h0);
        `CHECK_VALUE(msgo_payload, 32'h1, "Verify PCIe status after initiating second read");
        wb_send(READ, 16'hC, 32'h0);
        `CHECK_VALUE(wb_dat_o, 32'h6, "Verify WB status after PCIe read request");
        wb_send(READ, 16'h4, 32'h0);
        `CHECK_VALUE(wb_dat_o, 32'h2000a000, "Verify WB control value after PCIe read request");
        wb_send(READ, 16'hC, 32'h0);
        `CHECK_VALUE(wb_dat_o, 32'h4, "Verify WB status value after consuming PCIe read request");
        pcie_send(READ, 20'h6a000, 32'h0);
        pcie_recv(RESPONSE, 20'h0, 20'h0);
        `CHECK_VALUE(msgo_payload, 32'h1, "Verify PCIe status after WB consumes request only");
        wb_send(WRITE, 16'h0, 32'hDEADBEEF);
        wb_send(WRITE, 16'h4, 32'h80000000);
        wb_send(READ, 16'hC, 32'h0);
        `CHECK_VALUE(wb_dat_o, 32'h0, "Verify WB status value after responding to PCIe read request");
        pcie_send(READ, 20'h6a000, 32'h0);
        pcie_recv(RESPONSE, 20'h0, 20'h0);
        `CHECK_VALUE(msgo_payload, 32'h0, "Verify PCIe status after WB responds to read request");
        pcie_send(READ, 20'h7a000, 32'h0);
        pcie_recv(RESPONSE, 20'h0, 20'h0);
        `CHECK_VALUE(msgo_payload, 32'hdeadbeef, "Verify PCIe read data");
    
        $display("\n[TEST] ZPU Write from PCIe");
        pcie_send(WRITE, 20'h7b000, 32'h12345678);
        pcie_send(READ, 20'h7a000, 32'h0);
        pcie_recv(RESPONSE, 20'h0, 20'h0);
        `CHECK_VALUE(msgo_payload, 32'hdeadbeef, "Verify that PCIe read data is still intact after write");
        wb_send(READ, 16'hC, 32'h0);
        `CHECK_VALUE(wb_dat_o, 32'h2, "Verify WB status value after PCIe write request");
        wb_send(READ, 16'h0, 32'h0);
        `CHECK_VALUE(wb_dat_o, 32'h12345678, "Verify WB data value after PCIe read request");
        wb_send(READ, 16'h4, 32'h0);
        `CHECK_VALUE(wb_dat_o, 32'h4000b000, "Verify WB control value after PCIe read request");
        wb_send(READ, 16'hC, 32'h0);
        `CHECK_VALUE(wb_dat_o, 32'h0, "Verify WB status value after consuming PCIe write request");
        pcie_send(READ, 20'h6a000, 32'h0);
        pcie_recv(RESPONSE, 20'h0, 20'h0);
        `CHECK_VALUE(msgo_payload, 32'h0, "Verify PCIe status after WB consumes request");

        $display("\n[TEST] Chinch Write from ZPU");
        wb_send(READ, 16'hC, 32'h0);
        `CHECK_VALUE(wb_dat_o, 32'h0, "Verify WB status value before initiating write request");
        wb_send(WRITE, 16'h0, 32'h00beef00);
        wb_send(READ, 16'hC, 32'h0);
        `CHECK_VALUE(wb_dat_o, 32'h0, "Verify WB status value after writing just the data reg");
        wb_send(WRITE, 16'h4, 32'h40000200);
        wb_send(READ, 16'hC, 32'h0);
        `CHECK_VALUE(wb_dat_o, 32'h0, "Verify WB status value after initiating write");
        pcie_recv(WRITE, 20'h200, 20'h0);
        `CHECK_VALUE(msgo_payload, 32'h00beef00, "Verify received PCIe data");
        `CHECK_VALUE(msgo_ctrl, 32'h40000200, "Verify received PCIe control");
        wb_send(WRITE, 16'h0, 32'h00feeb00);
        wb_send(WRITE, 16'h4, 32'h400002fc);
        pcie_recv(WRITE, 20'h200, 20'h0);
        `CHECK_VALUE(msgo_payload, 32'h00feeb00, "Verify second received PCIe data");
        `CHECK_VALUE(msgo_ctrl, 32'h400002fc, "Verify second received PCIe control");

        $display("\n[TEST] Chinch Read from ZPU");
        wb_send(READ, 16'hC, 32'h0);
        `CHECK_VALUE(wb_dat_o, 32'h0, "Verify WB status value before initiating read request");
        wb_send(WRITE, 16'h0, 32'hffffffff);
        wb_send(READ, 16'hC, 32'h0);
        `CHECK_VALUE(wb_dat_o, 32'h0, "Verify WB status value after writing just the data reg");
        wb_send(WRITE, 16'h4, 32'h20000400);
        wb_send(READ, 16'hC, 32'h0);
        `CHECK_VALUE(wb_dat_o, 32'h1, "Verify WB status value after initiating read request");
        pcie_recv(READ, 20'h400, 20'h0);
        `CHECK_VALUE(msgo_payload, 32'hffffffff, "Verify received PCIe data");
        `CHECK_VALUE(msgo_ctrl, 32'h20000400, "Verify received PCIe control");
        wb_send(READ, 16'hC, 32'h0);
        wb_send(READ, 16'hC, 32'h0);
        `CHECK_VALUE(wb_dat_o, 32'h1, "Verify WB status value before PCIe responds");
        pcie_send(RESPONSE, 20'h000, 32'hace06666);
        wb_send(READ, 16'hC, 32'h0);
        `CHECK_VALUE(wb_dat_o, 32'h0, "Verify WB status value after PCIe responds");
        wb_send(READ, 16'h8, 32'h0);
        `CHECK_VALUE(wb_dat_o, 32'hace06666, "Verify WB read value after PCIe responds");

        $display("\n[TEST] WB Outbound flood");
        wb_send(READ, 16'hC, 32'h0);
        `CHECK_VALUE(wb_dat_o, 32'h0, "Verify WB status before request flood");
        for (it = 0; it < 64; it = it + 1) begin
            wb_send(WRITE, 16'h4, 32'h20000400);
        end
        wb_send(READ, 16'hC, 32'h0);
        `CHECK_VALUE(wb_dat_o, 32'h11, "Verify WB status after request flood");
        for (it = 0; it < 64; it = it + 1) begin
            pcie_recv(READ, 20'h400, 20'h0);
        end
        wb_send(READ, 16'hC, 32'h0);
        `CHECK_VALUE(wb_dat_o, 32'h1, "Verify WB status after consuming requests");

        $display("\n[TEST] PCIe Transaction Status");
        pcie_send(READ, 20'h6a000, 32'h0);
        pcie_recv(RESPONSE, 20'h0, 20'h0);
        `CHECK_VALUE(msgo_payload, 32'h0, "Verify PCIe status before multiple reads");
        pcie_send(WRITE, 20'h6a000, 32'h0);
        pcie_send(WRITE, 20'h6a000, 32'h0);
        pcie_send(WRITE, 20'h6a000, 32'h0);
        pcie_send(READ, 20'h6a000, 32'h0);
        pcie_recv(RESPONSE, 20'h0, 20'h0);
        pcie_send(READ, 20'h6a000, 32'h0);
        pcie_recv(RESPONSE, 20'h0, 20'h0);
        pcie_send(READ, 20'h6a000, 32'h0);
        pcie_recv(RESPONSE, 20'h0, 20'h0);
        `CHECK_VALUE(msgo_payload, 32'h1, "Verify PCIe status before multiple read requests and status queries");
        wb_send(READ, 16'h4, 32'h0);
        wb_send(WRITE, 16'h0, 32'hDEADBEEF);
        wb_send(WRITE, 16'h4, 32'h80000000);
        pcie_send(READ, 20'h6a000, 32'h0);
        pcie_recv(RESPONSE, 20'h0, 20'h0);
        `CHECK_VALUE(msgo_payload, 32'h0, "Verify PCIe status after response");
        
        $display("\n[DONE]");


    end // initial begin
    
    pcie_wb_reg_core #(.WB_ADDRW(16), .WB_DATAW(32)) dut (
        .clk(clk), .rst(reset),
        .wb_stb_i(wb_stb_i), .wb_we_i(wb_we_i), .wb_adr_i(wb_adr_i),
        .wb_dat_i(wb_dat_i), .wb_ack_o(wb_ack_o), .wb_dat_o(wb_dat_o),
        .msgi_tdata(msgi_data), .msgi_tvalid(msgi_valid), .msgi_tready(msgi_ready),
        .msgo_tdata(msgo_data), .msgo_tvalid(msgo_valid), .msgo_tready(msgo_ready),
        .debug());
    

endmodule

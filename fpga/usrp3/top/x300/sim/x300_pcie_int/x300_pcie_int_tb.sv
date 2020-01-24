//
// Copyright 2013 Ettus Research LLC
//


`timescale 1ns/1ps
`define NS_PER_TICK 1
`define NUM_TEST_CASES 23

`define SIM_TIMEOUT_US 1000 // 1ms

`include "sim_clks_rsts.vh"
`include "sim_exec_report.vh"

module x300_pcie_int_tb();
    `DEFINE_CLK(clk, 8.000, 50)
    `DEFINE_RESET(reset, 0, 10)

    `TEST_BENCH_INIT("x300_pcie_int_tb",`NUM_TEST_CASES,`NS_PER_TICK)

    reg temp_pass       = 0;
    reg pkt_swap        = 0;
    reg [15:0] it       = 0;
    reg [15:0] tx_ch, rx_ch = 0;
    
    reg [31:0] dma_sample_cnt = 0;
    reg [31:0] dma_packet_cnt = 0;
    reg [31:0] dma_out_sample_cnt[0:5];
    reg [63:0] dma_out_last_sample[0:5];

    reg             pcie_usr_reg_wr, pcie_usr_reg_rd;
    reg [1:0]       pcie_usr_reg_len;
    reg [19:0]      pcie_usr_reg_addr;
    reg [31:0]      pcie_usr_reg_data_in, pcie_usr_data;
    wire            pcie_usr_reg_rc, pcie_usr_reg_rdy;
    wire [31:0]     pcie_usr_reg_data_out;

    wire            chinch_reg_wr, chinch_reg_rd;
    wire [1:0]      chinch_reg_len;
    wire [19:0]     chinch_reg_addr;
    wire [31:0]     chinch_reg_data_out;
    reg             chinch_reg_rc, chinch_reg_rdy;
    reg [31:0]      chinch_reg_data_in;

    reg  [2:0]      i_chan, o_chan;
    reg  [383:0]    i_tdata_par;
    reg  [5:0]      i_tvalid_par = 6'b000000, o_tready_par = 6'b111111;
    wire [383:0]    o_tdata_par;
    wire [5:0]      o_tvalid_par, i_tready_par;

    reg             i_tvalid, i_tready, o_tvalid, o_tready;
    reg [63:0]      i_tdata, o_tdata;

    localparam READ     = 2'b01;
    localparam WRITE    = 2'b10;
   
    task usr_regport_request;
        input [1:0]     operation;
        input [19:0]    address;
        input [31:0]    data;
    begin
        pcie_usr_reg_data_in    <= data;
        pcie_usr_reg_addr       <= address;
        pcie_usr_reg_wr         <= operation[1];
        pcie_usr_reg_rd         <= operation[0];
        pcie_usr_reg_len        <= 2'b10;

        @(posedge clk);
        while (~pcie_usr_reg_rdy) @(posedge clk);
        
        pcie_usr_reg_wr         <= 1'b0;
        pcie_usr_reg_rd         <= 1'b0;
        @(posedge clk);
    end
    endtask // usr_regport_request

    task usr_regport_response;
    begin
        @(posedge clk);
        while (~pcie_usr_reg_rc) @(posedge clk);
        pcie_usr_data <= pcie_usr_reg_data_out;
        @(posedge clk);
    end
    endtask // usr_regport_response

    task chinch_regport_request;
        input [1:0]     operation;
        input [19:0]    address;
        input [31:0]    data;
    begin
        @(posedge clk);
        while (~(chinch_reg_rdy &&
            chinch_reg_addr == address &&
            {chinch_reg_wr,chinch_reg_rd} == operation &&
            chinch_reg_len == 2'b10 &&
            (operation == WRITE || chinch_reg_data_out == data)
        )) @(posedge clk);
        
        @(posedge clk);
    end
    endtask // chinch_regport_request

    task chinch_regport_response;
        input [31:0]    data;
    begin
        @(posedge clk);
        chinch_reg_data_in <= data;
        chinch_reg_rc <= 1'b1;
        @(posedge clk);
        chinch_reg_rc <= 1'b0;
    end
    endtask // chinch_regport_response

    task send_packet;
        input [63:0] sid;
        input [31:0] len;
        input [31:0] quant;
    begin
        if(quant < 2) begin
            i_tdata <= { sid[63:32],len[15:0], sid[15:0] };
            i_tvalid <= 1;
            @(posedge clk);
            i_tvalid <= 0;
            @(posedge clk);
        end else begin
            i_tdata <= { sid[63:32],len[15:0], sid[15:0] };
            i_tvalid <= 1;
            @(posedge clk);
            i_tdata <= 64'h0000_0001_0000_0000;
            repeat(quant - 2) begin
                i_tdata <= i_tdata + 64'h0000_0002_0000_0002;
                @(posedge clk);
            end
            i_tdata <= i_tdata + 64'h0000_0002_0000_0002;
            @(posedge clk);
            i_tvalid <= 1'b0;
            @(posedge clk);
        end // else: !if(len < 3)
    end
    endtask // send_packet

    task reset_dma_counts;
    begin
        dma_sample_cnt <= 32'd0;
        dma_packet_cnt <= 32'd0;
        dma_out_sample_cnt[0] <= 32'd0;
        dma_out_sample_cnt[1] <= 32'd0;
        dma_out_sample_cnt[2] <= 32'd0;
        dma_out_sample_cnt[3] <= 32'd0;
        dma_out_sample_cnt[4] <= 32'd0;
        dma_out_sample_cnt[5] <= 32'd0;
        dma_out_last_sample[0] <= 64'd0;
        dma_out_last_sample[1] <= 64'd0;
        dma_out_last_sample[2] <= 64'd0;
        dma_out_last_sample[3] <= 64'd0;
        dma_out_last_sample[4] <= 64'd0;
        dma_out_last_sample[5] <= 64'd0;
        @(posedge clk);
    end
    endtask // reset_dma_counts

    task select_channels;
        input [2:0] tx_ch;
        input [2:0] rx_ch;
    begin
        i_chan <= tx_ch;
        o_chan <= rx_ch;
        @(posedge clk);
    end
    endtask // select_channels

    task wait_for_pkt_loopback;
    begin
        while (i_tvalid & i_tready) @(posedge clk);  //Wait for outbound pkt to pad and send
        while (~o_tvalid) @(posedge clk);           //Wait for inbound pkt to arrive
        while (o_tvalid & o_tready) @(posedge clk); //Wait for inbound pkt to finish
    end
    endtask // wait_for_pkt_loopback

    wire [63:0] dma_loop_tdata ;
    wire [ 2:0] dma_loop_tuser ;
    wire        dma_loop_tvalid, dma_loop_tlast, dma_loop_tready;

    wire [63:0] iop2_msg_tdata ;
    wire        iop2_msg_tvalid, iop2_msg_tlast, iop2_msg_tready;

   
    initial begin : tb_main
        while (reset) @(posedge clk);
    
        chinch_reg_rdy <= 1'b1;
        chinch_reg_rc <= 1'b0;

        `TEST_CASE_START("Verify signature register");
        usr_regport_request(READ, 20'h40000, 32'h0);
        usr_regport_response();
        `TEST_CASE_DONE((pcie_usr_data == "X300"))

        `TEST_CASE_START("Verify counter frequency register");
        usr_regport_request(READ, 20'h4000C, 32'h0);
        usr_regport_response();
        `TEST_CASE_DONE((pcie_usr_data == 166666667));

        `TEST_CASE_START("Verify scratch registers");
        usr_regport_request(WRITE, 20'h40010, 32'hDEAD);
        usr_regport_request(WRITE, 20'h40014, 32'hBEEF);
        usr_regport_request(READ, 20'h40014, 32'h0);
        usr_regport_response();
        temp_pass <= (pcie_usr_data == 32'hBEEF);
        usr_regport_request(READ, 20'h40010, 32'h0);
        usr_regport_response();
        `TEST_CASE_DONE((pcie_usr_data == 32'hDEAD) & temp_pass);

        `TEST_CASE_START("Client register port write 1");
        usr_regport_request(WRITE, 20'h60000, 32'h12345678);
        chinch_regport_request(WRITE, 20'h60000, 32'h12345678);

        usr_regport_request(WRITE, 20'h7FFFC, 32'h1357);
        chinch_regport_request(WRITE, 20'h7FFFC, 32'h1357);

        usr_regport_request(WRITE, 20'h70000, 32'h2468);
        chinch_regport_request(WRITE, 20'h70000, 32'h2468);
        `TEST_CASE_DONE(1);

        `TEST_CASE_START("Client register port read 1");
        usr_regport_request(READ, 20'h60000, 32'h0);
        chinch_regport_request(READ, 20'h60000, 32'h0);
        chinch_regport_response(32'hACE0BA51);
        usr_regport_response();
        `ASSERT_ERROR((pcie_usr_data == 32'hACE0BA51),"");

        usr_regport_request(READ, 20'h7FFFC, 32'h0);
        chinch_regport_request(READ, 20'h7FFFC, 32'h0);
        chinch_regport_response(32'hACE0BA52);
        usr_regport_response();
        `ASSERT_ERROR((pcie_usr_data == 32'hACE0BA52),"");

        usr_regport_request(READ, 20'h70000, 32'h0);
        chinch_regport_request(READ, 20'h70000, 32'h0);
        chinch_regport_response(32'hACE0BA53);
        usr_regport_response();
        `TEST_CASE_DONE((pcie_usr_data == 32'hACE0BA53));

        `TEST_CASE_START("Configure RX DMA routing table");
        usr_regport_request(WRITE, 20'h40500, 32'h0000_0000);
        usr_regport_request(WRITE, 20'h40500, 32'h0001_0001);
        usr_regport_request(WRITE, 20'h40500, 32'h0002_0002);
        usr_regport_request(WRITE, 20'h40500, 32'h0003_0003);
        usr_regport_request(WRITE, 20'h40500, 32'h00D3_0000);
        usr_regport_request(WRITE, 20'h40500, 32'h00D2_0001);
        usr_regport_request(WRITE, 20'h40500, 32'h00D1_0002);
        usr_regport_request(WRITE, 20'h40500, 32'h00D0_0003);
        `TEST_CASE_DONE(1);

        `TEST_CASE_START("Frame size register read");
        usr_regport_request(READ, 20'h40204, 32'h0);
        usr_regport_response();
        `ASSERT_ERROR((pcie_usr_data == 32), "Frame size register read (Default) [TX0].");
        usr_regport_request(READ, 20'h40214, 32'h0);
        usr_regport_response();
        `ASSERT_ERROR((pcie_usr_data == 32), "Frame size register read (Default) [TX1].");
        usr_regport_request(READ, 20'h40404, 32'h0);
        usr_regport_response();
        `ASSERT_ERROR((pcie_usr_data == 32), "Frame size register read (Default) [RX0].");
        usr_regport_request(READ, 20'h40414, 32'h0);
        usr_regport_response();
        `ASSERT_ERROR((pcie_usr_data == 32), "Frame size register read (Default) [RX1].");
        `TEST_CASE_DONE(1);

        o_tready <= 1'b1;

        `TEST_CASE_START("Loopback packet");
        reset_dma_counts();
        usr_regport_request(WRITE, 20'h40200, 32'h0000_0012);
        usr_regport_request(WRITE, 20'h40400, 32'h0000_0012);
        select_channels(0,0);
        send_packet(16'h00D2, 80, 32);
        wait_for_pkt_loopback();
        `TEST_CASE_DONE((dma_sample_cnt==10 && dma_packet_cnt==1 && dma_out_sample_cnt[0]==32));

        reset_dma_counts();
        select_channels(1,0);
        send_packet(16'h00D3, 80, 32);
        wait_for_pkt_loopback();
        `TEST_CASE_DONE(dma_sample_cnt==10 && dma_packet_cnt==1 && dma_out_sample_cnt[0]==32);

        `TEST_CASE_START("Frame size register write.");
        usr_regport_request(WRITE, 20'h40224, 32'd16);
        usr_regport_request(WRITE, 20'h40234, 32'd16);
        usr_regport_request(WRITE, 20'h40424, 32'd16);
        usr_regport_request(WRITE, 20'h40434, 32'd16);
        `TEST_CASE_DONE(1);

        `TEST_CASE_START("Frame size register read");
        usr_regport_request(READ, 20'h40224, 32'h0);
        usr_regport_response();
        `ASSERT_ERROR((pcie_usr_data == 16), "Frame size register read [TX2].");
        usr_regport_request(READ, 20'h40234, 32'h0);
        usr_regport_response();
        `ASSERT_ERROR((pcie_usr_data == 16), "Frame size register read [TX3].");
        usr_regport_request(READ, 20'h40424, 32'h0);
        usr_regport_response();
        `ASSERT_ERROR((pcie_usr_data == 16), "Frame size register read [RX2].");
        usr_regport_request(READ, 20'h40434, 32'h0);
        usr_regport_response();
        `ASSERT_ERROR((pcie_usr_data == 16), "Frame size register read [RX3].");
        `TEST_CASE_DONE(1);

        `TEST_CASE_START("Loopback packet");
        reset_dma_counts();
        select_channels(2,2);
        send_packet(16'h0002, 32, 16);
        wait_for_pkt_loopback();
        `TEST_CASE_DONE(dma_sample_cnt==4 && dma_packet_cnt==1 && dma_out_sample_cnt[2]==16);

        reset_dma_counts();
        select_channels(3,3);
        send_packet(16'h0003, 32, 16);
        wait_for_pkt_loopback();
        `TEST_CASE_DONE((dma_sample_cnt==4 && dma_packet_cnt==1 && dma_out_sample_cnt[3]==16));

        `TEST_CASE_START("Loopback multiple packets");
        reset_dma_counts();
        select_channels(3,2);
        send_packet(16'h0002, 128, 16);
        send_packet(16'h0002, 128, 16);
        send_packet(16'h0002, 128, 16);
        send_packet(16'h0002, 128, 16);
        wait_for_pkt_loopback();
        repeat(64) @(posedge clk);
        `TEST_CASE_DONE(dma_sample_cnt==64 && dma_packet_cnt==4 && dma_out_sample_cnt[2]==64 && o_tdata==64'h0000009e00000020);

        `TEST_CASE_START("Loopback multiple packets (RX Swapped)");
        reset_dma_counts();
        select_channels(3,2);

        usr_regport_request(WRITE, 20'h40230, 32'h10);
        usr_regport_request(WRITE, 20'h40420, 32'h00);
        repeat(16) @(posedge clk);

        send_packet(16'h0002, 128, 16);
        send_packet(16'h0002, 128, 16);
        wait_for_pkt_loopback();
        repeat(64) @(posedge clk);
        `TEST_CASE_DONE(dma_sample_cnt==32 && dma_packet_cnt==2 && dma_out_sample_cnt[2]==32 && o_tdata==64'h000000200000009e);

        /* @TODO: Need to implement data swapping in TB
        `TEST_CASE_START();
        reset_dma_counts();
        select_channels(3,2);

        usr_regport_request(WRITE, 20'h40230, 32'h00);
        usr_regport_request(WRITE, 20'h40420, 32'h10);
        repeat(16) @(posedge clk);

        send_packet(16'h0002, 128, 16);
        send_packet(16'h0002, 128, 16);
        wait_for_pkt_loopback();
        repeat(64) @(posedge clk);
        `TEST_CASE_DONE(dma_sample_cnt==32 && dma_packet_cnt==2 && dma_out_sample_cnt[2]==32 && o_tdata[7:0]==32, 
            "Loopback multiple packets (TX Swapped).");
        */

        `TEST_CASE_START("Good DMA status.");
        reset_dma_counts();
        select_channels(3,2);
        usr_regport_request(READ, 20'h40230, 32'h0);
        usr_regport_response();
        temp_pass <= (pcie_usr_data == 32'h0);
        usr_regport_request(READ, 20'h40420, 32'h0);
        usr_regport_response();
        `TEST_CASE_DONE(temp_pass && (pcie_usr_data == 32'h0));

        `TEST_CASE_START("Bad DMA status.");
        send_packet(16'h0002, 160, 20);
        repeat(64) @(posedge clk);
        usr_regport_request(READ, 20'h40230, 32'h0);
        usr_regport_response();
        temp_pass <= (pcie_usr_data == 32'h1);
        usr_regport_request(READ, 20'h40420, 32'h0);
        usr_regport_response();
        `TEST_CASE_DONE(temp_pass || (pcie_usr_data == 32'h1));

        `TEST_CASE_START("DMA Status reset.");
        usr_regport_request(WRITE, 20'h40230, 32'h1);
        usr_regport_request(READ, 20'h40230, 32'h0);
        usr_regport_response();
        temp_pass <= (pcie_usr_data == 32'h0);
        usr_regport_request(WRITE, 20'h40420, 32'h1);
        usr_regport_request(READ, 20'h40420, 32'h0);
        usr_regport_response();
        `TEST_CASE_DONE(temp_pass && (pcie_usr_data == 32'h0));

        `TEST_CASE_START("Packet count register reset");
        select_channels(2,1);
        reset_dma_counts();
        usr_regport_request(WRITE, 20'h4022C, 32'h0);
        usr_regport_request(READ, 20'h4022C, 32'h0);
        usr_regport_response();
        `ASSERT_ERROR((pcie_usr_data == 0), "TX Packet count register reset.");
        usr_regport_request(WRITE, 20'h4041C, 32'h0);
        usr_regport_request(READ, 20'h4041C, 32'h0);
        usr_regport_response();
        `ASSERT_ERROR((pcie_usr_data == 0), "RX Packet count register reset.");
        `TEST_CASE_DONE(1);

        `TEST_CASE_START("TX Packet count register read");
        send_packet(16'h0001, 80, 16);
        send_packet(16'h0001, 24, 16);
        send_packet(16'h0001, 48, 16);
        repeat(64) @(posedge clk);
        usr_regport_request(READ, 20'h4022C, 32'h0);
        usr_regport_response();
        `ASSERT_ERROR((pcie_usr_data == 3), "TX Packet count register read.");
        usr_regport_request(READ, 20'h4041C, 32'h0);
        usr_regport_response();
        `ASSERT_ERROR((pcie_usr_data == 3), "RX Packet count register read.");
        `TEST_CASE_DONE(1);

        `TEST_CASE_START("TX Sample count register reset");
        select_channels(1,2);
        reset_dma_counts();
        usr_regport_request(WRITE, 20'h40218, 32'h0);
        usr_regport_request(READ, 20'h40218, 32'h0);
        usr_regport_response();
        `ASSERT_ERROR((pcie_usr_data == 0), "TX Sample count register reset.");
        usr_regport_request(WRITE, 20'h40428, 32'h0);
        usr_regport_request(READ, 20'h40428, 32'h0);
        usr_regport_response();
        `ASSERT_ERROR((pcie_usr_data == 0), "RX Sample count register reset.");
        `TEST_CASE_DONE(1'b1);

        `TEST_CASE_START("TX Sample count register read");
        send_packet(16'h0002, 28, 16);
        wait_for_pkt_loopback();
        usr_regport_request(READ, 20'h40218, 32'h0);
        usr_regport_response();
        `ASSERT_ERROR((pcie_usr_data == 4), "TX Sample count register read.");
        usr_regport_request(READ, 20'h40428, 32'h0);
        usr_regport_response();
        `ASSERT_ERROR((pcie_usr_data == 4), "RX Sample count register read.");
        `TEST_CASE_DONE(1'b1);
        
        `TEST_CASE_START("Setup for NxN DMA test");
        for (it = 0; it < 16'd6; it = it + 16'd1) begin
            usr_regport_request(WRITE, 20'h40204 + (it * 16), 32'h4);
            usr_regport_request(WRITE, 20'h40404 + (it * 16), 32'h4);
            usr_regport_request(WRITE, 20'h40500, {it, it});
        end
        `TEST_CASE_DONE(1'b1);

        `TEST_CASE_START("Setup for NxN DMA test");
        for (tx_ch = 0; tx_ch < 6; tx_ch = tx_ch + 16'd2) begin
            for (rx_ch = 1; rx_ch < 6; rx_ch = rx_ch + 16'd1) begin
                select_channels(tx_ch,rx_ch);
                reset_dma_counts();
                @(posedge clk);
                send_packet(rx_ch, 16, 4);
                wait_for_pkt_loopback();
                if (dma_sample_cnt==2 && dma_packet_cnt==1 && dma_out_sample_cnt[rx_ch]==4) begin
                    $display("[TEST%d]: NxN DMA Test [TX=%d, RX=%d]...Passed",tc_run_count,tx_ch[3:0],rx_ch[3:0]);
                end else begin
                    $display("[TEST%d]: NxN DMA Test [TX=%d, RX=%d]...FAILED!!!",tc_run_count,tx_ch[3:0],rx_ch[3:0]);
                end
                @(posedge clk);
            end
        end
        `TEST_CASE_DONE(1'b1);
    end // initial begin
    

    x300_pcie_int #(
        .DMA_STREAM_WIDTH  (64),
        .NUM_TX_STREAMS    (6 ),
        .NUM_RX_STREAMS    (6 ),
        .REGPORT_ADDR_WIDTH(20),
        .REGPORT_DATA_WIDTH(32),
        .IOP2_MSG_WIDTH    (64)
    ) x300_pcie_int (
        .ioport2_clk          (clk                  ),
        .bus_clk              (clk                  ),
        .bus_rst              (reset                ),
        
        //DMA TX FIFOs (IoPort2 Clock Domain)
        .dmatx_tdata_iop2     (i_tdata_par          ),
        .dmatx_tvalid_iop2    (i_tvalid_par         ),
        .dmatx_tready_iop2    (i_tready_par         ),
        
        //DMA TX FIFOs (IoPort2 Clock Domain)
        .dmarx_tdata_iop2     (o_tdata_par          ),
        .dmarx_tvalid_iop2    (o_tvalid_par         ),
        .dmarx_tready_iop2    (o_tready_par         ),
        
        //PCIe User Regport
        .pcie_usr_reg_wr      (pcie_usr_reg_wr      ),
        .pcie_usr_reg_rd      (pcie_usr_reg_rd      ),
        .pcie_usr_reg_addr    (pcie_usr_reg_addr    ),
        .pcie_usr_reg_data_in (pcie_usr_reg_data_in ),
        .pcie_usr_reg_len     (pcie_usr_reg_len     ),
        .pcie_usr_reg_data_out(pcie_usr_reg_data_out),
        .pcie_usr_reg_rc      (pcie_usr_reg_rc      ),
        .pcie_usr_reg_rdy     (pcie_usr_reg_rdy     ),
        
        //Chinch Regport
        .chinch_reg_wr        (chinch_reg_wr        ),
        .chinch_reg_rd        (chinch_reg_rd        ),
        .chinch_reg_addr      (chinch_reg_addr      ),
        .chinch_reg_data_out  (chinch_reg_data_out  ),
        .chinch_reg_len       (chinch_reg_len       ),
        .chinch_reg_data_in   (chinch_reg_data_in   ),
        .chinch_reg_rc        (chinch_reg_rc        ),
        .chinch_reg_rdy       (chinch_reg_rdy       ),
        
        //DMA TX FIFO (Bus Clock Domain)
        .dmatx_tdata          (dma_loop_tdata       ),
        .dmatx_tuser          (dma_loop_tuser       ),
        .dmatx_tlast          (dma_loop_tlast       ),
        .dmatx_tvalid         (dma_loop_tvalid      ),
        .dmatx_tready         (dma_loop_tready      ),
        
        //DMA RX FIFO (Bus Clock Domain)
        .dmarx_tdata          (dma_loop_tdata       ),
        .dmarx_tuser          (dma_loop_tuser       ),
        .dmarx_tlast          (dma_loop_tlast       ),
        .dmarx_tvalid         (dma_loop_tvalid      ),
        .dmarx_tready         (dma_loop_tready      ),
        
        //Message FIFO Out (Bus Clock Domain)
        .rego_tdata           (iop2_msg_tdata       ),
        .rego_tvalid          (iop2_msg_tvalid      ),
        .rego_tlast           (iop2_msg_tlast       ),
        .rego_tready          (iop2_msg_tready      ),
        
        //Message FIFO In (Bus Clock Domain)
        .regi_tdata           (iop2_msg_tdata       ),
        .regi_tvalid          (iop2_msg_tvalid      ),
        .regi_tlast           (iop2_msg_tlast       ),
        .regi_tready          (iop2_msg_tready      ),
        
        .debug                (                     )
    );

    always @(posedge clk) begin
        if (dma_loop_tvalid & dma_loop_tready) begin
            dma_sample_cnt <= dma_sample_cnt + 32'd1;
            if (dma_loop_tlast) dma_packet_cnt <= dma_packet_cnt + 32'd1;
        end
    end
    
    always @(posedge clk) begin
        case (i_chan)
            3'd5:
                {i_tdata_par[383:320], i_tvalid_par[5], i_tready} <= {i_tdata, i_tvalid, i_tready_par[5]};
            3'd4:
                {i_tdata_par[319:256], i_tvalid_par[4], i_tready} <= {i_tdata, i_tvalid, i_tready_par[4]};
            3'd3:
                {i_tdata_par[255:192], i_tvalid_par[3], i_tready} <= {i_tdata, i_tvalid, i_tready_par[3]};
            3'd2:
                {i_tdata_par[191:128], i_tvalid_par[2], i_tready} <= {i_tdata, i_tvalid, i_tready_par[2]};
            3'd1:
                {i_tdata_par[127:64],  i_tvalid_par[1], i_tready} <= {i_tdata, i_tvalid, i_tready_par[1]};
            default:
                {i_tdata_par[63:0],    i_tvalid_par[0], i_tready} <= {i_tdata, i_tvalid, i_tready_par[0]};
        endcase
    end

    always @(posedge clk) begin
        case (o_chan)
            3'd5:
                {o_tdata, o_tvalid, o_tready_par[5]} <= {o_tdata_par[383:320], o_tvalid_par[5], o_tready};
            3'd4:
                {o_tdata, o_tvalid, o_tready_par[4]} <= {o_tdata_par[319:256], o_tvalid_par[4], o_tready};
            3'd3:
                {o_tdata, o_tvalid, o_tready_par[3]} <= {o_tdata_par[255:192], o_tvalid_par[3], o_tready};
            3'd2:
                {o_tdata, o_tvalid, o_tready_par[2]} <= {o_tdata_par[191:128], o_tvalid_par[2], o_tready};
            3'd1:
                {o_tdata, o_tvalid, o_tready_par[1]} <= {o_tdata_par[127:64],  o_tvalid_par[1], o_tready};
            default:
                {o_tdata, o_tvalid, o_tready_par[0]} <= {o_tdata_par[63:0],    o_tvalid_par[0], o_tready};
        endcase
    end

    genvar i;
    generate
        for (i=0; i<6; i=i+1) begin: dma_counter_generator
            always @(posedge clk) begin
                if (o_tvalid_par[i] & o_tready_par[i]) begin
                    dma_out_sample_cnt[i] <= dma_out_sample_cnt[i] + 32'd1;
                    dma_out_last_sample[i] <= o_tdata_par[(64*i)+63:64*i];
                end
            end
        end
    endgenerate

endmodule

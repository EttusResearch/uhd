//
// Copyright 2016 Ettus Research
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//

module axi_fifo_2clk #(
    parameter SYNC_STAGES = 2,
    parameter SIZE        = 10,
    parameter WIDTH       = 32,
    parameter PIPELINE    = "<UNUSED>")
(
    input reset,
    input i_aclk,
    input [WIDTH-1:0] i_tdata,
    input i_tvalid,
    output reg i_tready = 1'b0,
    input o_aclk,
    output [WIDTH-1:0] o_tdata,
    output reg o_tvalid = 1'b0,
    input o_tready
);

    localparam FIFOSIZE = (SIZE < 5) ? 5 : SIZE;

    // Synchronizers
    wire o_rst_sync, i_rst_sync;
    synchronizer #(
        .INITIAL_VAL(1'b1),
        .WIDTH(1),
        .STAGES(SYNC_STAGES))
    synchronizer_i_rst (
        .clk(i_aclk), .rst(1'b0),
        .in(reset), .out(i_rst_sync));
    synchronizer #(
        .INITIAL_VAL(1'b1),
        .WIDTH(1),
        .STAGES(SYNC_STAGES))
    synchronizer_o_rst (
        .clk(o_aclk), .rst(1'b0),
        .in(reset), .out(o_rst_sync));

    // Gray counter encode / decode + synchronizers
    reg [FIFOSIZE-1:0] wr_addr, rd_addr;
    wire [FIFOSIZE-1:0] wr_addr_sync, rd_addr_sync;
    wire [FIFOSIZE-1:0] wr_addr_gray_sync, wr_addr_gray, rd_addr_gray_sync, rd_addr_gray;
    synchronizer #(
        .INITIAL_VAL(0),
        .WIDTH(FIFOSIZE),
        .STAGES(SYNC_STAGES))
    synchronizer_rd_addr_gray (
        .clk(i_aclk), .rst(o_rst_sync),
        .in(rd_addr_gray), .out(rd_addr_gray_sync));
    synchronizer #(
        .INITIAL_VAL(0),
        .WIDTH(FIFOSIZE),
        .STAGES(SYNC_STAGES))
    synchronizer_wr_addr_gray (
        .clk(o_aclk), .rst(i_rst_sync),
        .in(wr_addr_gray), .out(wr_addr_gray_sync));
    bin2gray #(.WIDTH(FIFOSIZE))
    bin2gray_wr_addr (.bin(wr_addr), .gray(wr_addr_gray));
    bin2gray #(.WIDTH(FIFOSIZE))
    bin2gray_rd_addr (.bin(rd_addr), .gray(rd_addr_gray));
    gray2bin #(.WIDTH(FIFOSIZE))
    gray2bin_wr_addr (.gray(wr_addr_gray_sync), .bin(wr_addr_sync));
    gray2bin #(.WIDTH(FIFOSIZE))
    gray2bin_rd_addr (.gray(rd_addr_gray_sync), .bin(rd_addr_sync));

    reg [FIFOSIZE:0] i_occupied;
    reg [FIFOSIZE:0] i_space;
    reg i_full;
    reg i_empty;
    reg [FIFOSIZE:0] o_occupied;
    reg [FIFOSIZE:0] o_space;
    reg o_full;
    reg o_empty;

    reg [WIDTH:0] mem[0:2**(FIFOSIZE)-1];
    integer i;
    initial begin
        for (i = 0; i < 1 << FIFOSIZE; i = i + 1) begin
            mem[i] = 'd0;
        end
    end

    // Write
    always @(posedge i_aclk) begin
        if (i_rst_sync) begin
            wr_addr <= 'd0;
        end else begin
            if (i_tvalid & i_tready) begin
                mem[wr_addr] <= i_tdata;
                wr_addr      <= wr_addr + 1'b1;
            end
        end
    end

    // Write ready, full, empty, occupied signals
    always @(posedge i_aclk) begin
        if (i_rst_sync) begin
            i_tready   <= 1'b0;
            i_full     <= 1'b0;
            i_empty    <= 1'b1;
            i_occupied <= 'd0;
            i_space    <= (1'b1 << FIFOSIZE);
        end else begin
            if ((rd_addr_sync-1'b1 == wr_addr) & i_tvalid & i_tready) begin
                i_tready <= 1'b0;
                i_full   <= 1'b1;
            end else if ((rd_addr_sync != wr_addr) & i_full) begin
                i_tready <= 1'b1;
                i_full   <= 1'b0;
            end
            if ((rd_addr_sync == wr_addr) & ~i_full) begin
                i_tready <= 1'b1;
                if (~i_tvalid) begin
                    i_empty  <= 1'b1;
                end
            end else begin
                i_empty  <= 1'b0;
            end
            if (i_tvalid) begin
                if (wr_addr == rd_addr_sync) begin
                    if (i_full) begin
                        i_occupied <= 1'b1 << FIFOSIZE;
                        i_space    <= 'd0;
                    end else begin
                        i_occupied <= 'd1;
                        i_space    <= (1'b1 << FIFOSIZE)-1'b1;
                    end
                end else if (wr_addr > rd_addr_sync) begin
                    i_occupied <= wr_addr - rd_addr_sync + 1'b1;
                    i_space    <= (1'b1 << FIFOSIZE) - (wr_addr - rd_addr_sync + 1'b1);
                end else begin
                    i_occupied <= wr_addr+1'b1 + (1'b1 << FIFOSIZE)-1'b1 - rd_addr_sync + 1'b1;
                    i_space    <= rd_addr_sync - wr_addr - 1'b1;
                end
            end else begin
                if (wr_addr == rd_addr_sync) begin
                    if (i_full) begin
                        i_occupied <= 1'b1 << FIFOSIZE;
                        i_space    <= 'd0;
                    end else begin
                        i_occupied <= 'd0;
                        i_space    <= 1'b1 << FIFOSIZE;
                    end
                end else if (wr_addr > rd_addr_sync) begin
                    i_occupied <= wr_addr - rd_addr_sync;
                    i_space    <= (1'b1 << FIFOSIZE) - (wr_addr - rd_addr_sync);
                end else begin
                    i_occupied <= wr_addr+1'b1 + (1'b1 << FIFOSIZE)-1'b1 - rd_addr_sync;
                    i_space    <= rd_addr_sync - wr_addr;
                end
            end
        end
    end

    // Read
    always @(posedge o_aclk) begin
        if (o_rst_sync) begin
            rd_addr <= 'd0;
        end else begin
            if (o_tvalid & o_tready) begin
                rd_addr <= rd_addr + 1'b1;
            end
        end
    end

    assign o_tdata = mem[rd_addr];

    // Read valid, full, empty, occupied signals
    always @(posedge o_aclk) begin
        if (o_rst_sync) begin
            o_tvalid   <= 1'b0;
            o_full     <= 1'b0;
            o_empty    <= 1'b1;
            o_occupied <= 'd0;
            o_space    <= 'd0;
        end else begin
            if ((rd_addr+1'b1 == wr_addr_sync) & o_tready & o_tvalid) begin
                o_tvalid <= 1'b0;
                o_empty  <= 1'b1;
            end else if ((rd_addr != wr_addr_sync) & o_empty) begin
                o_tvalid <= 1'b1;
                o_empty  <= 1'b0;
            end
            if ((rd_addr == wr_addr_sync) & ~o_empty & ~o_tready) begin
                o_full   <= 1'b1;
            end else begin
                o_full   <= 1'b0;
            end
            if (o_tready) begin
                if (wr_addr_sync == rd_addr) begin
                    if (~o_empty) begin
                        o_occupied <= (1'b1 << FIFOSIZE) - 1'b1;
                        o_space    <= 'd1;
                    end else begin
                        o_occupied <= 'd0;
                        o_space    <= (1'b1 << FIFOSIZE);
                    end
                end else if (wr_addr_sync > rd_addr) begin
                    o_occupied <= wr_addr_sync - rd_addr - 1'b1;
                    o_space    <= (1'b1 << FIFOSIZE) - (wr_addr_sync - rd_addr - 1'b1);
                end else begin
                    o_occupied <= wr_addr_sync+1'b1 + (1'b1 << FIFOSIZE)-1'b1 - rd_addr - 1'b1;
                    o_space    <= rd_addr - wr_addr_sync + 1'b1;
                end
            end else begin
                if (wr_addr_sync == rd_addr) begin
                    if (~o_empty) begin
                        o_occupied <= 1'b1 << FIFOSIZE;
                        o_space    <= 'd0;
                    end else begin
                        o_occupied <= 'd0;
                        o_space    <= 1'b1 << FIFOSIZE;
                    end
                end else if (wr_addr_sync > rd_addr) begin
                    o_occupied <= wr_addr_sync - rd_addr;
                    o_space    <= (1'b1 << FIFOSIZE) - (wr_addr_sync - rd_addr);
                end else begin
                    o_occupied <= wr_addr_sync+1'b1 + (1'b1 << FIFOSIZE)-1'b1 - rd_addr;
                    o_space    <= rd_addr - wr_addr_sync;
                end
            end
        end
    end

endmodule

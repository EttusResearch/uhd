//
// Copyright 2013 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//


module zpu_bootram #(
    parameter ADDR_WIDTH    = 16,
    parameter DATA_WIDTH    = 32,
    parameter MAX_ADDR      = 16'h7FFC
) (
    input                   clk,
    input                   rst,

    input                   mem_stb,
    input                   mem_wea,
    input  [ADDR_WIDTH-1:0] mem_addra,
    input  [DATA_WIDTH-1:0] mem_dina,
    output [DATA_WIDTH-1:0] mem_douta,
    output reg              mem_acka,

    input                   ldr_stb,
    input                   ldr_wea,
    input  [ADDR_WIDTH-1:0] ldr_addra,
    input  [DATA_WIDTH-1:0] ldr_dina,
    output                  ldr_acka,
    
    output reg              zpu_rst
);
    localparam SR_LDR_ADDR_REG  = 4'h0;
    localparam SR_LDR_DATA_REG  = 4'h1;
    
    //---------------------------------------------------------
    // Mem ack logic
    always @(posedge clk) begin
        if (rst) mem_acka <= 0;
        else     mem_acka <= mem_stb & ~mem_acka;
    end

    //---------------------------------------------------------
    // Instantiate 2 bootram modules
    // They will both initialize with the pre-built ZPU firmware
    //
    wire        ram0_ena, ram1_ena;
    wire [3:0]  ram0_wea, ram1_wea;
    wire [12:0] ram0_addra, ram1_addra;
    wire [31:0] ram0_dina, ram0_douta, ram1_dina, ram1_douta;
    
    bootram sys_ram0(
        .clka(clk),
        .ena(ram0_ena),
        .wea(ram0_wea),
        .addra(ram0_addra),
        .dina(ram0_dina),
        .douta(ram0_douta));

    bootram sys_ram1(
        .clka(clk),
        .ena(ram1_ena),
        .wea(ram1_wea),
        .addra(ram1_addra),
        .dina(ram1_dina),
        .douta(ram1_douta));
      
    //---------------------------------------------------------
    // Settings bus interface for bootloader
    wire                    ldr_set_stb;
    wire [3:0]              ldr_set_addr;
    wire [DATA_WIDTH-1:0]   ldr_set_data;

    //@TODO: This address truncation seems unclean. Maybe settings_bus can take a addr_width as a param.
    wire [7:0]              ldr_set_addr_w;
    settings_bus #(.AWIDTH(ADDR_WIDTH), .DWIDTH(DATA_WIDTH)) ldr_settings_bus
    (
        .wb_clk(clk), .wb_rst(rst),
        .wb_adr_i(ldr_addra), .wb_dat_i(ldr_dina), 
        .wb_stb_i(ldr_stb), .wb_we_i(ldr_wea), .wb_ack_o(ldr_acka),
        .strobe(ldr_set_stb), .addr(ldr_set_addr_w), .data(ldr_set_data)
    );
    assign ldr_set_addr = ldr_set_addr_w[3:0];

    //---------------------------------------------------------
    // Selection logic
    //
    reg             bootram_ptr;
    reg [12:0]      ldr_curr_wr_addr;
    wire            ldr_we_stb;

    assign {ram0_ena, ram0_wea, ram0_addra, ram0_dina} = bootram_ptr ? {mem_stb, {4{(mem_wea & ~zpu_rst)}}, mem_addra[14:2], mem_dina} : 
                                                                       {ldr_we_stb, {4{ldr_we_stb}}, ldr_curr_wr_addr, ldr_set_data};
    assign {ram1_ena, ram1_wea, ram1_addra, ram1_dina} = bootram_ptr ? {ldr_we_stb, {4{ldr_we_stb}}, ldr_curr_wr_addr, ldr_set_data} : 
                                                                       {mem_stb, {4{(mem_wea & ~zpu_rst)}}, mem_addra[14:2], mem_dina};
    assign mem_douta                                   = bootram_ptr ? ram0_douta : ram1_douta;

    //---------------------------------------------------------
    // Boot loader
    //
    assign ldr_we_stb = ~zpu_rst && ldr_set_stb && (ldr_set_addr == SR_LDR_DATA_REG);

    always @(posedge clk) begin
        if (rst) begin
            zpu_rst             <= 1;
            bootram_ptr         <= 1;
            ldr_curr_wr_addr    <= 13'h0;
        end else if (ldr_set_stb & ~zpu_rst) begin
            case (ldr_set_addr)
                SR_LDR_ADDR_REG: begin
                    ldr_curr_wr_addr <= ldr_set_data[14:2];
                end
                SR_LDR_DATA_REG: begin
                    ldr_curr_wr_addr <= ldr_curr_wr_addr + 1;
                    if ({1'b0, ldr_curr_wr_addr, 2'b00} == MAX_ADDR) begin
                        zpu_rst     <= 1;
                        bootram_ptr <= ~bootram_ptr;
                    end
                end
            endcase
        end else begin
            zpu_rst <= 0;
        end
    end

endmodule

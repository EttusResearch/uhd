//
// Copyright 2012 Ettus Research LLC
//


// Simple I2C core

// Settings reg map:
//
// BASE+0 control register
//   byte0 - control bits, data byte, or command bits, prescaler
//   byte1 - what to do? (documented in cpp file)
//      write prescaler lo
//      write prescaler hi
//      write control
//      write data
//      write command
//      read data
//      read status
//

// Readback:
//
// byte0 has readback value based on the last read command
//

module simple_i2c_core
    #(
        //settings register base address
        parameter BASE = 0,

        //i2c line level at reset
        parameter ARST_LVL = 1
    )
    (
        //clock and synchronous reset
        input clock, input reset,

        //32-bit settings bus inputs
        input set_stb, input [7:0] set_addr, input [31:0] set_data,

        //32-bit data readback
        output reg [31:0] readback,

        //read is high when i2c core can begin another transaction
        output reg ready,

        // I2C signals
        // i2c clock line
        input  scl_pad_i,       // SCL-line input
        output scl_pad_o,       // SCL-line output (always 1'b0)
        output scl_padoen_o,    // SCL-line output enable (active low)

        // i2c data line
        input  sda_pad_i,       // SDA-line input
        output sda_pad_o,       // SDA-line output (always 1'b0)
        output sda_padoen_o,    // SDA-line output enable (active low)

        //optional debug output
        output [31:0] debug
    );

    //declare command settings register
    wire [7:0] sr_what, sr_data;
    wire sr_changed;
    setting_reg #(.my_addr(BASE+0),.width(16)) i2c_cmd_sr(
        .clk(clock),.rst(reset),.strobe(set_stb),.addr(set_addr),.in(set_data),
        .out({sr_what, sr_data}),.changed(sr_changed));

    //declare wb interface signals
    wire [2:0] wb_addr;
    wire [7:0] wb_data_mosi;
    wire [7:0] wb_data_miso;
    wire wb_we, wb_stb, wb_cyc;
    wire wb_ack;

    //create wishbone-based i2c core
    i2c_master_top #(.ARST_LVL(ARST_LVL)) i2c 
     (.wb_clk_i(clock),.wb_rst_i(reset),.arst_i(1'b0), 
      .wb_adr_i(wb_addr),.wb_dat_i(wb_data_mosi),.wb_dat_o(wb_data_miso),
      .wb_we_i(wb_we),.wb_stb_i(wb_stb),.wb_cyc_i(wb_cyc),
      .wb_ack_o(wb_ack),.wb_inta_o(),
      .scl_pad_i(scl_pad_i),.scl_pad_o(scl_pad_o),.scl_padoen_o(scl_padoen_o),
      .sda_pad_i(sda_pad_i),.sda_pad_o(sda_pad_o),.sda_padoen_o(sda_padoen_o) );

    //not ready between setting register and wishbone ack
    always @(posedge clock) begin
        if (reset || wb_ack) ready <= 1;
        else if (sr_changed) ready <= 0;
    end

    //register wishbone data on every ack
    always @(posedge clock) begin
        if (wb_ack) readback <= {24'b0, wb_data_miso};
    end

    //assign wishbone signals
    assign wb_addr = sr_what[2:0];
    assign wb_stb = sr_changed;
    assign wb_we = wb_stb && sr_what[3];
    assign wb_cyc = wb_stb;
    assign wb_data_mosi = sr_data;

endmodule //simple_i2c_core

//
// Copyright 2011 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Description
//  This code implements a parameterizable true dual port memory 
//  (both ports can read and write). If an enable is not necessary
//  it may be tied off.
//
// Note
//  This module requires the ram_2port_impl.vh header file. The
//  header is included multiple times with different values of
//  the RAM_DIRECTIVE macro to create different implementations of the
//  RAM. An implementation is chosen in ram_2port based on the 
//  user parameter for RAM_TYPE.

// Mode: AUTOMATIC
`define RAM_DIRECTIVE
`define RAM_MOD_NAME ram_2port_impl_auto
`include "ram_2port_impl.vh"
`undef RAM_MOD_NAME
`undef RAM_DIRECTIVE

// Mode: REG
`define RAM_DIRECTIVE (* ram_style = "registers" *)
`define RAM_MOD_NAME ram_2port_impl_reg
`include "ram_2port_impl.vh"
`undef RAM_MOD_NAME
`undef RAM_DIRECTIVE

// Mode: LUTRAM
`define RAM_DIRECTIVE (* ram_style = "distributed" *)
`define RAM_MOD_NAME ram_2port_impl_lutram
`include "ram_2port_impl.vh"
`undef RAM_MOD_NAME
`undef RAM_DIRECTIVE

// Mode: BRAM
`define RAM_DIRECTIVE (* ram_style = "block" *)
`define RAM_MOD_NAME ram_2port_impl_bram
`include "ram_2port_impl.vh"
`undef RAM_MOD_NAME
`undef RAM_DIRECTIVE

// Mode: URAM
`define RAM_DIRECTIVE (* ram_style = "ultra" *)
`define RAM_MOD_NAME ram_2port_impl_uram
`include "ram_2port_impl.vh"
`undef RAM_MOD_NAME
`undef RAM_DIRECTIVE

module ram_2port #(
  parameter DWIDTH    = 32,           // Width of the memory block
  parameter AWIDTH    = 9,            // log2 of the depth of the memory block
  parameter RW_MODE   = "READ-FIRST", // Read-write mode {READ-FIRST, WRITE-FIRST, NO-CHANGE}
  parameter RAM_TYPE  = "AUTOMATIC",  // Type of RAM to infer {AUTOMATIC, REG, LUTRAM, BRAM, URAM}
  parameter OUT_REG   = 0,            // Instantiate an output register? (+1 cycle of read latency)
  parameter INIT_FILE = ""            // Optionally initialize memory with this file
) (
  input  wire              clka,
  input  wire              ena,
  input  wire              wea,
  input  wire [AWIDTH-1:0] addra,
  input  wire [DWIDTH-1:0] dia,
  output wire [DWIDTH-1:0] doa,

  input  wire              clkb,
  input  wire              enb,
  input  wire              web,
  input  wire [AWIDTH-1:0] addrb,
  input  wire [DWIDTH-1:0] dib,
  output wire [DWIDTH-1:0] dob
);

  generate
    if (RAM_TYPE == "URAM")
      ram_2port_impl_uram #(
        .DWIDTH(DWIDTH), .AWIDTH(AWIDTH), .RW_MODE(RW_MODE),
        .OUT_REG(OUT_REG), .INIT_FILE(INIT_FILE)
      ) impl (
        .clka(clka), .ena(ena), .wea(wea), .addra(addra), .dia(dia), .doa(doa),
        .clkb(clkb), .enb(enb), .web(web), .addrb(addrb), .dib(dib), .dob(dob)
      );
    else if (RAM_TYPE == "BRAM")
      ram_2port_impl_bram #(
        .DWIDTH(DWIDTH), .AWIDTH(AWIDTH), .RW_MODE(RW_MODE),
        .OUT_REG(OUT_REG), .INIT_FILE(INIT_FILE)
      ) impl (
        .clka(clka), .ena(ena), .wea(wea), .addra(addra), .dia(dia), .doa(doa),
        .clkb(clkb), .enb(enb), .web(web), .addrb(addrb), .dib(dib), .dob(dob)
      );
    else if (RAM_TYPE == "LUTRAM")
      ram_2port_impl_lutram #(
        .DWIDTH(DWIDTH), .AWIDTH(AWIDTH), .RW_MODE(RW_MODE),
        .OUT_REG(OUT_REG), .INIT_FILE(INIT_FILE)
      ) impl (
        .clka(clka), .ena(ena), .wea(wea), .addra(addra), .dia(dia), .doa(doa),
        .clkb(clkb), .enb(enb), .web(web), .addrb(addrb), .dib(dib), .dob(dob)
      );
    else if (RAM_TYPE == "REG")
      ram_2port_impl_reg #(
        .DWIDTH(DWIDTH), .AWIDTH(AWIDTH), .RW_MODE(RW_MODE),
        .OUT_REG(OUT_REG), .INIT_FILE(INIT_FILE)
      ) impl (
        .clka(clka), .ena(ena), .wea(wea), .addra(addra), .dia(dia), .doa(doa),
        .clkb(clkb), .enb(enb), .web(web), .addrb(addrb), .dib(dib), .dob(dob)
      );
    else
      ram_2port_impl_auto #(
        .DWIDTH(DWIDTH), .AWIDTH(AWIDTH), .RW_MODE(RW_MODE),
        .OUT_REG(OUT_REG), .INIT_FILE(INIT_FILE)
      ) impl (
        .clka(clka), .ena(ena), .wea(wea), .addra(addra), .dia(dia), .doa(doa),
        .clkb(clkb), .enb(enb), .web(web), .addrb(addrb), .dib(dib), .dob(dob)
      );
  endgenerate

endmodule

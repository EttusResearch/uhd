//
// Copyright 2015 Ettus Research LLC
//
`ifndef INCLUDED_SIM_SET_RB_LIB
`define INCLUDED_SIM_SET_RB_LIB

interface settings_bus_t #(
  parameter SR_AWIDTH = 8,
  parameter SR_DWIDTH = 32,
  parameter RB_AWIDTH = 8,
  parameter RB_DWIDTH = 64,
  parameter NUM_BUSES = 1
)(
  input clk
);
  logic [NUM_BUSES-1:0]           set_stb;
  logic [NUM_BUSES*SR_AWIDTH-1:0] set_addr;
  logic [NUM_BUSES*SR_DWIDTH-1:0] set_data;
  logic [NUM_BUSES-1:0]           rb_stb;
  logic [NUM_BUSES*RB_AWIDTH-1:0] rb_addr;
  logic [NUM_BUSES*RB_DWIDTH-1:0] rb_data;

  modport master (output set_stb, output set_addr, output set_data,
                  input rb_stb, output rb_addr, input rb_data);
  modport slave (input set_stb, input set_addr, input set_data,
                  output rb_stb, input rb_addr, output rb_data);
endinterface

interface settings_bus_master #(
  parameter SR_AWIDTH = 8,
  parameter SR_DWIDTH = 32,
  parameter RB_AWIDTH = 8,
  parameter RB_DWIDTH = 64,
  parameter NUM_BUSES = 1,
  parameter TIMEOUT   = 65535 // readback() timeout
)(
  input clk
);
  settings_bus_t #(
    .SR_AWIDTH(SR_AWIDTH), .SR_DWIDTH(SR_DWIDTH),
    .RB_AWIDTH(RB_AWIDTH), .RB_DWIDTH(RB_DWIDTH),
    .NUM_BUSES(NUM_BUSES))
  settings_bus (.clk(clk));

  // Reset signals / properties used by this interface
  task automatic reset;
    settings_bus.set_stb = 0;
    settings_bus.set_addr = 0;
    settings_bus.set_data = 0;
    settings_bus.rb_addr = 0;
  endtask

  // Push a transaction onto the settings bus
  // Args:
  // - set_addr: Settings bus address
  // - set_data: Settings bus data
  // - rb_addr:  Readback bus address
  task automatic write (
    input logic [SR_AWIDTH-1:0] set_addr,
    input logic [SR_DWIDTH-1:0] set_data,
    input logic [RB_AWIDTH-1:0] rb_addr = 'd0,
    input int bus = 0); // Optional
    begin
      if (clk) @(negedge clk);
      settings_bus.set_stb[bus]  = 1'b1;
      settings_bus.set_addr[SR_AWIDTH*bus +: SR_AWIDTH] = set_addr;
      settings_bus.set_data[SR_DWIDTH*bus +: SR_DWIDTH] = set_data;
      settings_bus.rb_addr[RB_AWIDTH*bus +: RB_AWIDTH]  = rb_addr;
      @(negedge clk);
      settings_bus.set_stb[bus]  = 1'b0;
      settings_bus.set_addr[SR_AWIDTH*bus +: SR_AWIDTH] = 'd0;
      settings_bus.set_data[SR_DWIDTH*bus +: SR_DWIDTH] = 'd0;
      settings_bus.rb_addr[RB_AWIDTH*bus +: RB_AWIDTH]  = 'd0;
    end
  endtask

  // Pull a transaction from the readback bus. Typically called immediately after write().
  // Args:
  // - rb_data: Readback data
  task automatic readback (
    output logic [RB_DWIDTH-1:0] rb_data,
    input int bus = 0);
    begin
      integer timeout_counter = 0;
      if (clk & ~settings_bus.rb_stb[bus]) @(negedge clk);
      while (~settings_bus.rb_stb[bus]) begin
        if (timeout_counter < TIMEOUT) begin
          timeout_counter++;
        end else begin
          $error("settings_bus_t::readback(): Timeout waiting for readback strobe!");
          break;
        end
        @(negedge clk);
      end
      rb_data = settings_bus.rb_data[RB_DWIDTH*bus +: RB_DWIDTH];
    end
  endtask

endinterface

interface settings_bus_slave #(
  parameter SR_AWIDTH = 8,
  parameter SR_DWIDTH = 32,
  parameter RB_AWIDTH = 8,
  parameter RB_DWIDTH = 64,
  parameter NUM_BUSES = 1,
  parameter TIMEOUT   = 65535 // read() timeout
)(
  input clk
);
  settings_bus_t #(
    .SR_AWIDTH(SR_AWIDTH), .SR_DWIDTH(SR_DWIDTH),
    .RB_AWIDTH(RB_AWIDTH), .RB_DWIDTH(RB_DWIDTH),
    .NUM_BUSES(NUM_BUSES))
  settings_bus (.clk(clk));

  // Reset signals / properties used by this interface
  task automatic reset;
    settings_bus.rb_stb = 0;
    settings_bus.rb_data = 0;
  endtask

  // Pull a transaction from the settings bus
  // Args:
  // - set_addr: Settings bus address
  // - set_data: Settings bus data
  // - rb_addr:  Readback bus address
  task automatic read (
    output logic [SR_AWIDTH-1:0] set_addr,
    output logic [SR_DWIDTH-1:0] set_data,
    output logic [RB_AWIDTH-1:0] rb_addr,
    input int bus = 0);
    begin
      integer timeout_counter = 0;
      while (~settings_bus.set_stb[bus]) begin
        @(negedge clk);
        if (timeout_counter < TIMEOUT) begin
          timeout_counter++;
        end else begin
          $error("settings_bus_t::read(): Timeout waitling for settings bus strobe!");
          break;
        end
      end
      set_addr = settings_bus.set_addr[SR_AWIDTH*bus +: SR_AWIDTH];
      set_data = settings_bus.set_data[SR_DWIDTH*bus +: SR_DWIDTH];
      rb_addr  = settings_bus.rb_addr[RB_AWIDTH*bus +: RB_AWIDTH];
      @(negedge clk);
    end
  endtask

  // Push a transaction onto the readback bus, typically called immediately after read()
  // Args:
  // - rb_data: Readback data
  task writeback (
    input logic [RB_AWIDTH-1:0] rb_data,
    input int bus = 0);
    begin
      if (clk & ~settings_bus.set_stb[bus]) @(negedge clk);
      settings_bus.rb_stb[bus] = 1'b1;
      settings_bus.rb_data[RB_DWIDTH*bus +: RB_DWIDTH] = rb_data;
      @(negedge clk);
      settings_bus.rb_stb[bus] = 1'b0;
      settings_bus.rb_data[RB_DWIDTH*bus +: RB_DWIDTH] = 'd0;
    end
  endtask

endinterface

`endif
# UHD FPGA Coding Standards

## Preamble

To quote R. W. Emerson: "A foolish consistency is the hobgoblin of little minds,
adored by little statesmen and philosophers and divines". Ignoring the little
statesmen for a minute, these coding standards are here to make our life
*easier*, not simply add additional rules. They are meant as additional guidance
for developers, and are not meant to be interpreted as law.

So, ultimately, it is up to the developer to decide how much these guidelines
should be heeded when writing code, and up to reviewers how much they are
relevant to new submissions.
That said, a consistent codebase is easier to maintain, read, understand, and
extend. Choosing personal preferences over these coding guidelines is not a
helpful move for the team and future maintainability of the UHD FPGA codebase.

## General Coding Guidelines

* Code layout: We use 2 spaces for indentation levels, and never tabs.
* Never submit code with trailing whitespace.
* Code is read more often than it's written. Code readability is thus something
  worth optimizing for.
* Try and keep line lengths to 79 characters, unless readability suffers.
* Comment your code. Especially if your code is tricky or makes unique assumptions.
* Use the following header at the top of each file:
```verilog
//
// Copyright <YEAR> Ettus Research, A National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: <MODULE_NAME>
// Description:
// <DESCRIPTION>
```

## Verilog Style Guidelines

### General Syntax

* Always use `begin`and `end` statements for more complex code blocks even if the enclosing code is only
one line.
* Indent begin/end as follows:
```verilog
if (foo) begin
  // Do something
end else if (bar) begin
  case(xyz)
    1'b0: begin
      // Handle 0
    end
    default: begin
      // Handle 1
    end
  endcase
end else begin
  // Do nothing
end
```
* Instantiate and declare modules as follows:
```verilog
dummy_module #(
  .PARAM1(0), .PARAM2(1)
) inst (
  .clk(clk), .rst(rst)
);
```

### Assignments

* Sequential blocks **must** only have non-blocking assignments (`<=`)
* Combinational blocks should only have blocking assignments (`=`)
* Don't mix blocking and non-blocking assignments

### Modules

* Each module should be defined in a separate file
* Use Verilog 2001 ANSI-C style port declarations
```verilog
(
  ...
  output reg  foo,
  input  wire bar
);
```
* Declare inputs and outputs one per line. This makes searching and commenting easier.
* Add inline comments to input/output ports to describe their behavior
* Be explicit about whether an input or output is a wire or reg.
* Group signals logically instead of by direction. If a single AXI-Stream bus has multiple inputs and
outputs, keep them together.
* Instantiate all ports for a module even if they are tied off or unconnected. Don't let the compiler
insert values for any signals automatically.
```verilog
dummy_module inst (
  .clk(clk), .rst(1'b0), .status(/* unused */)
);
```
* Don't instantiate modules using positional arguments. Use the dot form illustrated above.
* Every module requires a header giving a synopsis of its function below the
  copyright header.

### Clocking and Resets

* Name clocks as `clk`. If there are multiple clocks then use a prefix like `bus_clk` and `radio_clk`.
* If a module has signals or input/outputs whose clock domain is not obvious, use a clock suffix
to be explicit about the domain, for example `axi_tdata_bclk`, `axi_tdata_rclk`.
* Try not to encode the frequency of the clock in the name unless the particular clock can
*never* take on any other frequency.
* Name resets as `rst`. If there are multiple clocks then use a prefix like `bus_rst` and `radio_rst`.
* If a reset is asynchronous, call it `arst`.
* Try to avoid asynchronous resets as much as possible.
* Don't active low resets unless it is used to drive IO.

### Parameters, defines and constants

* Parametrize modules wherever possible, especially if they are designed for reuse. Bus widths, addresses,
buffer sizes, etc are good candidates for parametrization.
* For modules with parameters, add inline comments to describe the behavior of each parameter
* Propagate parameters as far up the hierarchy as possible as long as it makes sense.
* Place `` `define`` statements in Verilog header file (.vh) and include them in modules.
* Avoid placing `` `define`` statements in modules
* For local parameters, use `localparam` instead on hard-coding things like widths, etc.

### AXI Specific

We heavily use AXI buses in the design so here are some best practices for those:
* Keep the components of an AXI-Stream or AXI-MM bus together in port/wire instantiations
* For module ports, use the master/slave naming convention as shown below. It makes connecting modules
easier because a master always connects to a slave
```verilog
  input  wire [63:0] s_axis_tdata,
  input  wire        s_axis_tlast,
  input  wire        s_axis_tvalid,
  output wire        s_axis_tready,
  output reg  [63:0] m_axis_tdata,
  output reg         m_axis_tlast,
  output reg         m_axis_tvalid,
  input  wire        m_axis_tready,
```
* For connections between a master and slave, *do not* use the master/slave convention. Name the bus based
on its function or underlying data.
```verilog
  wire [63:0] axis_eth2xbar_tdata,
  wire        axis_eth2xbar_tlast,
  wire        axis_eth2xbar_tvalid,
  wire        axis_eth2xbar_tready,

  // If "axis" is obvious, drop the prefix
  wire [63:0] samp_tdata,
  wire        samp_tlast,
  wire        samp_tvalid,
  wire        samp_tready,
```


## Design Best Practices

TBD

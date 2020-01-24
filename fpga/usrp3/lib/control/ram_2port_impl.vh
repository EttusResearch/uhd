//
// Copyright 2011 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Used by ram_2port.v
// Requires `RAM_MOD_NAME and `RAM_DIRECTIVE to be defined

module `RAM_MOD_NAME #(
  parameter DWIDTH    = 32,           // Width of the memory block
  parameter AWIDTH    = 9,            // log2 of the depth of the memory block
  parameter RW_MODE   = "READ-FIRST", // Read-write mode {READ-FIRST, WRITE-FIRST, NO-CHANGE}
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

  `RAM_DIRECTIVE reg [DWIDTH-1:0] ram [(1<<AWIDTH)-1:0];

  // Initialize ram to a specified file or to all zeros to match hardware
  generate if (INIT_FILE != "") begin
    initial
      $readmemh(INIT_FILE, ram, 0, (1<<AWIDTH)-1);
  end else begin
    integer i;
    initial
      for (i = 0; i < (1<<AWIDTH); i = i + 1)
        ram[i] = {DWIDTH{1'b0}};
  end endgenerate

  reg [DWIDTH-1:0] doa_r = 'h0, dob_r = 'h0;
  generate if (OUT_REG == 1) begin
    // A 2 clock cycle read latency with improve clock-to-out timing
    reg [DWIDTH-1:0] doa_rr = 'h0, dob_rr = 'h0;

    always @(posedge clka)
      if (ena)
        doa_rr <= doa_r;

    always @(posedge clkb)
      if (enb)
        dob_rr <= dob_r;

    assign doa = doa_rr;
    assign dob = dob_rr;
  end else begin
    // A 1 clock cycle read latency at the cost of a longer clock-to-out timing
    assign doa = doa_r;
    assign dob = dob_r;
  end endgenerate

  generate if (RW_MODE == "READ-FIRST") begin
    // When data is written, the prior memory contents at the write
    // address are presented on the output port.
    always @(posedge clka) begin
      if (ena) begin
        if (wea)
          ram[addra] <= dia;
        doa_r <= ram[addra];
      end
    end
    always @(posedge clkb) begin
      if (enb) begin
        if (web)
          ram[addrb] <= dib;
        dob_r <= ram[addrb];
      end
    end

  end else if (RW_MODE == "WRITE-FIRST") begin
    // The data being written to the RAM also resides on the output port.
    always @(posedge clka) begin
      if (ena) begin
        if (wea) begin
          ram[addra] <= dia;
          doa_r <= dia;
        end else begin
          doa_r <= ram[addra];
        end
      end
    end
    always @(posedge clkb) begin
      if (enb) begin
        if (web) begin
          ram[addrb] <= dib;
          dob_r <= dib;
        end else begin
          dob_r <= ram[addrb];
        end
      end
    end

  end else begin
    // This is a no change RAM which retains the last read value on the output during writes
    // which is the most power efficient mode.
    always @(posedge clka) begin
      if (ena) begin
        if (wea)
          ram[addra] <= dia;
        else
          doa_r <= ram[addra];
      end
    end
    always @(posedge clkb) begin
      if (enb) begin
        if (web)
          ram[addrb] <= dib;
        else
          dob_r <= ram[addrb];
      end
    end

  end endgenerate

endmodule

//
// Copyright 2011-2012 Ettus Research LLC
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

////////////////////////////////////////////////////////////////////////
// FIFO to GPMC
//
// Reads frames from FIFO interface and writes them into BRAM pages.
// The GPMC is asynchronously alerted when a BRAM page has been filled.
//
// EM_CLK:
// A GPMC read transaction consists of two EM_CLK cycles (idle low).
//
// EM_OE:
// Output enable is actually the combination of ~NOE & ~NCS.
// The output enable is only active for the second rising edge,
// to ensure one edge per transaction to transition on.
//
// EM_D:
// The BRAM performs a read on the first rising edge into EM_D.
// Then, data will then be read on the next rising edge by GPMC.
//
// EM_A:
// On the first rising edge of EM_CLK, the address is held.
// On the second rising edge, the address is set for the next transaction.
////////////////////////////////////////////////////////////////////////

module fifo_to_gpmc
  #(parameter PTR_WIDTH = 2, parameter ADDR_WIDTH = 10, parameter LAST_ADDR = 10'h3ff)
  (input clk, input reset, input clear, input arst,
   input [17:0] data_i, input src_rdy_i, output dst_rdy_o,
   output [15:0] EM_D, input [ADDR_WIDTH:1] EM_A, input EM_CLK, input EM_OE,
   output reg data_available);

    //states for the GPMC side of things
    wire [17:0] data_o;
    reg gpmc_state;
    reg [ADDR_WIDTH:1] addr;
    reg [PTR_WIDTH:0] gpmc_ptr, next_gpmc_ptr;
    localparam GPMC_STATE_START = 0;
    localparam GPMC_STATE_EMPTY = 1;

    //states for the FIFO side of things
    reg fifo_state;
    reg [ADDR_WIDTH-1:0] counter;
    reg [PTR_WIDTH:0] fifo_ptr;
    localparam FIFO_STATE_CLAIM = 0;
    localparam FIFO_STATE_FILL = 1;

    //------------------------------------------------------------------
    // State machine to control the data from GPMC to BRAM
    //------------------------------------------------------------------
    always @(posedge EM_CLK or posedge arst) begin
        if (arst) begin
            gpmc_state <= GPMC_STATE_START;
            gpmc_ptr <= 0;
            next_gpmc_ptr <= 0;
            addr <= 0;
        end
        else if (EM_OE) begin
            addr <= EM_A + 1;
            case(gpmc_state)

            GPMC_STATE_START: begin
                if (EM_A == 0) begin
                    gpmc_state <= GPMC_STATE_EMPTY;
                    next_gpmc_ptr <= gpmc_ptr + 1;
                end
            end

            GPMC_STATE_EMPTY: begin
                if (addr == LAST_ADDR) begin
                    gpmc_state <= GPMC_STATE_START;
                    gpmc_ptr <= next_gpmc_ptr;
                    addr <= 0;
                end
            end

            endcase //gpmc_state
        end //EM_OE
    end //always

    //------------------------------------------------------------------
    // High when the gpmc pointer has not caught up to the fifo pointer.
    //------------------------------------------------------------------
    wire [PTR_WIDTH:0] safe_gpmc_ptr;
    cross_clock_reader #(.WIDTH(PTR_WIDTH+1)) read_gpmc_ptr
        (.clk(clk), .rst(reset | clear), .in(gpmc_ptr), .out(safe_gpmc_ptr));

    wire bram_available_to_fill = (fifo_ptr ^ (1 << PTR_WIDTH)) != safe_gpmc_ptr;

    //------------------------------------------------------------------
    // Glich free generation of data available signal:
    // Data is available when the pointers dont match.
    //------------------------------------------------------------------
    wire [PTR_WIDTH:0] safe_next_gpmc_ptr;
    cross_clock_reader #(.WIDTH(PTR_WIDTH+1)) read_next_gpmc_ptr
        (.clk(clk), .rst(reset | clear), .in(next_gpmc_ptr), .out(safe_next_gpmc_ptr));

    always @(posedge clk)
        if (reset | clear) data_available <= 0;
        else               data_available <= safe_next_gpmc_ptr != fifo_ptr;

    //------------------------------------------------------------------
    // State machine to control the data from BRAM to FIFO
    //------------------------------------------------------------------
    always @(posedge clk) begin
        if (reset | clear) begin
            fifo_state <= FIFO_STATE_CLAIM;
            fifo_ptr <= 0;
            counter <= 0;
        end
        else begin
            case(fifo_state)

            FIFO_STATE_CLAIM: begin
                if (bram_available_to_fill) fifo_state <= FIFO_STATE_FILL;
                counter <= 0;
            end

            FIFO_STATE_FILL: begin
                if (src_rdy_i && dst_rdy_o && data_i[17]) begin
                    fifo_state <= FIFO_STATE_CLAIM;
                    fifo_ptr <= fifo_ptr + 1;
                end
                if (src_rdy_i && dst_rdy_o) begin
                    counter <= counter + 1;
                end
            end

            endcase //fifo_state
        end
    end //always

    assign dst_rdy_o = fifo_state == FIFO_STATE_FILL;

    //assign data from bram output
    assign EM_D = data_o[15:0];

    //instantiate dual ported bram for async read + write
    ram_2port #(.DWIDTH(18),.AWIDTH(PTR_WIDTH + ADDR_WIDTH)) async_fifo_bram
     (.clka(clk),.ena(1'b1),.wea(src_rdy_i && dst_rdy_o),
      .addra({fifo_ptr[PTR_WIDTH-1:0], counter}),.dia(data_i),.doa(),
      .clkb(EM_CLK),.enb(1'b1),.web(1'b0),
      .addrb({gpmc_ptr[PTR_WIDTH-1:0], addr}),.dib(18'h3ffff),.dob(data_o));

endmodule // fifo_to_gpmc

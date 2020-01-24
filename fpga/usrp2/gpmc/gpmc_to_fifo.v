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
// GPMC to FIFO
//
// Reads frames from BRAM pages and writes them into FIFO interface.
// The GPMC is asynchronously alerted when a BRAM page is available.
//
// EM_CLK:
// A GPMC write transaction consists of one EM_CLK cycle (idle low).
//
// EM_WE:
// Write enable is actually the combination of ~NWE & ~NCS.
// The write enable is active for the entire transaction.
//
// EM_D:
// Data is set on the rising edge and written into BRAM on the falling edge.
//
// EM_A:
// Address is set on the rising edge and read by BRAM on the falling edge.
////////////////////////////////////////////////////////////////////////

module gpmc_to_fifo
  #(parameter PTR_WIDTH = 2, parameter ADDR_WIDTH = 10, parameter LAST_ADDR = 10'h3ff)
  (input [15:0] EM_D, input [ADDR_WIDTH:1] EM_A, input EM_CLK, input EM_WE,
   input clk, input reset, input clear, input arst,
   output [17:0] data_o, output src_rdy_o, input dst_rdy_i,
   output reg have_space);

    //states for the GPMC side of things
    reg gpmc_state;
    reg [ADDR_WIDTH:1] addr;
    reg [PTR_WIDTH:0] gpmc_ptr, next_gpmc_ptr;
    localparam GPMC_STATE_START = 0;
    localparam GPMC_STATE_FILL = 1;

    //states for the FIFO side of things
    reg [1:0] fifo_state;
    reg [ADDR_WIDTH-1:0] counter;
    reg [ADDR_WIDTH-1:0] last_counter;
    reg [ADDR_WIDTH-1:0] last_xfer;
    reg [PTR_WIDTH:0] fifo_ptr;
    localparam FIFO_STATE_CLAIM = 0;
    localparam FIFO_STATE_EMPTY = 1;
    localparam FIFO_STATE_PRE = 2;

    //------------------------------------------------------------------
    // State machine to control the data from GPMC to BRAM
    //------------------------------------------------------------------
    always @(negedge EM_CLK or posedge arst) begin
        if (arst) begin
            gpmc_state <= GPMC_STATE_START;
            gpmc_ptr <= 0;
            next_gpmc_ptr <= 0;
            addr <= 0;
        end
        else if (EM_WE) begin
            addr <= EM_A + 1;
            case(gpmc_state)

            GPMC_STATE_START: begin
                if (EM_A == 0) begin
                    gpmc_state <= GPMC_STATE_FILL;
                    next_gpmc_ptr <= gpmc_ptr + 1;
                end
            end

            GPMC_STATE_FILL: begin
                if (addr == LAST_ADDR) begin
                    gpmc_state <= GPMC_STATE_START;
                    gpmc_ptr <= next_gpmc_ptr;
                    addr <= 0;
                end
            end

            endcase //gpmc_state
        end //EM_WE
    end //always

    //------------------------------------------------------------------
    // A block ram is available to empty when the pointers dont match.
    //------------------------------------------------------------------
    wire [PTR_WIDTH:0] safe_gpmc_ptr;
    cross_clock_reader #(.WIDTH(PTR_WIDTH+1)) read_gpmc_ptr
        (.clk(clk), .rst(reset | clear), .in(gpmc_ptr), .out(safe_gpmc_ptr));

    wire bram_available_to_empty = safe_gpmc_ptr != fifo_ptr;

    //------------------------------------------------------------------
    // Glich free generation of have space signal:
    // High when the fifo pointer has not caught up to the gpmc pointer.
    //------------------------------------------------------------------
    wire [PTR_WIDTH:0] safe_next_gpmc_ptr;
    cross_clock_reader #(.WIDTH(PTR_WIDTH+1)) read_next_gpmc_ptr
        (.clk(clk), .rst(reset | clear), .in(next_gpmc_ptr), .out(safe_next_gpmc_ptr));

    wire [PTR_WIDTH:0] fifo_ptr_next = fifo_ptr + 1;
    always @(posedge clk)
        if (reset | clear) have_space <= 0;
        else               have_space <= (fifo_ptr ^ (1 << PTR_WIDTH)) != safe_next_gpmc_ptr;

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
                if (bram_available_to_empty && data_o[16]) fifo_state <= FIFO_STATE_PRE;
                counter <= 0;
            end

            FIFO_STATE_PRE: begin
                fifo_state <= FIFO_STATE_EMPTY;
                counter <= counter + 1;
            end

            FIFO_STATE_EMPTY: begin
                if (src_rdy_o && dst_rdy_i && data_o[17]) begin
                    fifo_state <= FIFO_STATE_CLAIM;
                    fifo_ptr <= fifo_ptr + 1;
                    counter <= 0;
                end
                else if (src_rdy_o && dst_rdy_i) begin
                    counter <= counter + 1;
                end
            end

            endcase //fifo_state
        end
    end //always

    wire enable = (fifo_state != FIFO_STATE_EMPTY) || dst_rdy_i;

    assign src_rdy_o = fifo_state == FIFO_STATE_EMPTY;

    //instantiate dual ported bram for async read + write
    ram_2port #(.DWIDTH(16),.AWIDTH(PTR_WIDTH + ADDR_WIDTH)) async_fifo_bram
     (.clka(~EM_CLK),.ena(1'b1),.wea(EM_WE),
      .addra({gpmc_ptr[PTR_WIDTH-1:0], addr}),.dia(EM_D),.doa(),
      .clkb(clk),.enb(enable),.web(1'b0),
      .addrb({fifo_ptr[PTR_WIDTH-1:0], counter}),.dib(18'h3ffff),.dob(data_o[15:0]));

    //store the vita length -> last xfer count
    always @(posedge clk) begin
        if (src_rdy_o && dst_rdy_i && data_o[16]) begin
            last_xfer <= {data_o[ADDR_WIDTH-2:0], 1'b0};
        end
    end

    //logic for start and end of frame
    always @(posedge clk) if (enable) last_counter <= counter;
    assign data_o[17] = !data_o[16] && ((last_counter + 1'b1) == last_xfer);
    assign data_o[16] = last_counter == 0;

endmodule // gpmc_to_fifo

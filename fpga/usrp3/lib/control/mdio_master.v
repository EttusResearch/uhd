//
// Copyright 2016 Ettus Research
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//

module mdio_master #(
   parameter        REG_AWIDTH  = 32,
   parameter        REG_BASE    = 'h0,
   parameter [7:0]  MDC_DIVIDER = 8'd200
) (
   // Clock and reset
   input                  clk,
   input                  rst,
   // MDIO ports
   output reg             mdc,
   output reg             mdio_out,
   output reg             mdio_tri,   // Assert to tristate driver.
   input                  mdio_in,
   // Register ports
   input                  reg_wr_req,
   input [REG_AWIDTH-1:0] reg_wr_addr,
   input [31:0]           reg_wr_data,
   input                  reg_rd_req,
   input [REG_AWIDTH-1:0] reg_rd_addr,
   output reg             reg_rd_resp,
   output reg [31:0]      reg_rd_data
);

   localparam [7:0]
      IDLE = 0,
      PREAMBLE1 = 1,
      PREAMBLE2 = 2,
      PREAMBLE3 = 3,
      PREAMBLE4 = 4,
      PREAMBLE5 = 5,
      PREAMBLE6 = 6,
      PREAMBLE7 = 7,
      PREAMBLE8 = 8,
      PREAMBLE9 = 9,
      PREAMBLE10 = 10,
      PREAMBLE11 = 11,
      PREAMBLE12 = 12,
      PREAMBLE13 = 13,
      PREAMBLE14 = 14,
      PREAMBLE15 = 15,
      PREAMBLE16 = 16,
      PREAMBLE17 = 17,
      PREAMBLE18 = 18,
      PREAMBLE19 = 19,
      PREAMBLE20 = 20,
      PREAMBLE21 = 21,
      PREAMBLE22 = 22,
      PREAMBLE23 = 23,
      PREAMBLE24 = 24,
      PREAMBLE25 = 25,
      PREAMBLE26 = 26,
      PREAMBLE27 = 27,
      PREAMBLE28 = 28,
      PREAMBLE29 = 29,
      PREAMBLE30 = 30,
      PREAMBLE31 = 31,
      PREAMBLE32 = 32,
      START1 = 33,
      C22_START2 = 34,
      C45_START2 = 35,
      OP1 = 36,
      OP2 = 37,
      PRTAD1 = 38,
      PRTAD2 = 39,
      PRTAD3 = 40,
      PRTAD4 = 41,
      PRTAD5 = 42,
      DEVAD1 = 43,
      DEVAD2 = 44,
      DEVAD3 = 45,
      DEVAD4 = 46,
      DEVAD5 = 47,
      TA1 = 48,
      TA2 = 49,
      TA3 = 50,
      READ1 = 51,
      READ2 = 52,
      READ3 = 53,
      READ4 = 54,
      READ5 = 55,
      READ6 = 56,
      READ7 = 57,
      READ8 = 58,
      READ9 = 59,
      READ10 = 60,
      READ11 = 61,
      READ12 = 62,
      READ13 = 63,
      READ14 = 64,
      READ15 = 65,
      READ16 = 66,
      WRITE1 = 67,
      WRITE2 = 68,
      WRITE3 = 69,
      WRITE4 = 70,
      WRITE5 = 71,
      WRITE6 = 72,
      WRITE7 = 73,
      WRITE8 = 74,
      WRITE9 = 75,
      WRITE10 = 76,
      WRITE11 = 77,
      WRITE12 = 78,
      WRITE13 = 79,
      WRITE14 = 80,
      WRITE15 = 81,
      WRITE16 = 82,
      C45_ADDR1 = 83,
      C45_ADDR2 = 84,
      C45_ADDR3 = 85,
      C45_ADDR4 = 86,
      C45_ADDR5 = 87,
      C45_ADDR6 = 88,
      C45_ADDR7 = 89,
      C45_ADDR8 = 90,
      C45_ADDR9 = 91,
      C45_ADDR10 = 92,
      C45_ADDR11 = 93,
      C45_ADDR12 = 94,
      C45_ADDR13 = 95,
      C45_ADDR14 = 96,
      C45_ADDR15 = 97,
      C45_ADDR16 = 98,
      PREIDLE = 99
   ;

   localparam REG_MDIO_DATA         = REG_BASE + 'h0;
   localparam REG_MDIO_ADDR         = REG_BASE + 'h4;
   localparam REG_MDIO_OP           = REG_BASE + 'h8;
   localparam REG_MDIO_CTRL_STATUS  = REG_BASE + 'hC;

   reg [15:0]  mdio_read_data, mdio_write_data;
   reg [15:0]  mdio_address;
   reg [12:0]  mdio_operation;
   reg [7:0]   mdc_clk_count;
   reg         mdc_falling_edge;
   reg         mdio_running;
   reg         mdio_done;
   reg [7:0]   state;

   always @(posedge clk) begin
      if (rst) begin
         mdio_write_data <= 16'h0;
         mdio_address    <= 16'h0;
         mdio_operation  <= 13'h0;
         mdio_running    <= 1'b0;
      end else begin
         // Handshake to MDIO state machine to reset running flag in status.
         // Wait for falling MDC edge to prevent S/W race condition occuring
         // where done flag still asserted but running flag now cleared (repeatedly).
         if (mdio_done && mdc_falling_edge)
            mdio_running <= 1'b0;

         // Readable registers
         if (reg_rd_req) begin
            reg_rd_resp <= 1'b1;
            case (reg_rd_addr)
               REG_MDIO_DATA:
                  reg_rd_data <= {16'h0, mdio_read_data};
               REG_MDIO_ADDR:
                  reg_rd_data <= {16'h0, mdio_address};
               REG_MDIO_OP:
                  reg_rd_data <= {16'h0, mdio_operation};
               REG_MDIO_CTRL_STATUS:
                  reg_rd_data <= {31'b0, mdio_running};
               default:
                  reg_rd_resp <= 1'b0;
            endcase
         end else if (reg_rd_resp) begin
            reg_rd_resp <= 1'b0;
         end

         // Writable registers
         if (reg_wr_req) begin
            case(reg_wr_addr)
               REG_MDIO_DATA:
                  mdio_write_data <= reg_wr_data[15:0];
               REG_MDIO_ADDR:
                  mdio_address    <= reg_wr_data[15:0];
               REG_MDIO_OP:
                  mdio_operation  <= reg_wr_data[12:0];
               REG_MDIO_CTRL_STATUS:
                  if (reg_wr_data[0])
                     mdio_running <= 1'b1;   // Trigger mdio operation here. Cleared by state machine at end of bus transaction.
            endcase
         end
      end
   end

   //
   // Produce mdc clock as a signal synchronously from Wishbone clock.
   //
   always @(posedge clk) begin
      if (rst) begin
         mdc_clk_count     <= 8'd1;
         mdc               <= 1'b0;
         mdc_falling_edge  <= 1'b0;
      end else if (mdc_clk_count == (MDC_DIVIDER/2)) begin
         mdc_clk_count     <= 8'd1;
         mdc               <= ~mdc;
         mdc_falling_edge  <= mdc;
      end else begin
         mdc_clk_count     <= mdc_clk_count + 8'd1;
         mdc_falling_edge  <= 1'b0;
      end
   end

   //
   // MDIO state machine
   //
   always @(posedge clk) begin
      if (rst) begin
         mdio_tri <= 1'b1;
         mdio_out <= 1'b0;
         mdio_done <= 1'b0;
         mdio_read_data <= 16'b0;
         state <= IDLE;
      end else if (mdc_falling_edge) begin
         // This is the MDIO bus controller. Use falling edge of MDC.
         mdio_tri <= 1'b1;
         mdio_out <= 1'b0;
         mdio_done <= 1'b0;
         case(state)
            // IDLE.
            // In Clause 22 & 45 the master of the MDIO bus is tristate during idle.
            IDLE: begin
               mdio_tri <= 1'b1;
               mdio_out <= 1'b0;
               if (mdio_running)
                  state <= PREAMBLE1;
            end
            // Preamble. All MDIO transactions begin witrh 32bits of 1 bits as a preamble.
            PREAMBLE1: begin
               mdio_tri <= 1'b0;
               mdio_out <= 1'b1;
               state <= PREAMBLE2;
            end
            PREAMBLE2: begin
               mdio_tri <= 1'b0;
               mdio_out <= 1'b1;
               state <= PREAMBLE3;
            end
            PREAMBLE3: begin
               mdio_tri <= 1'b0;
               mdio_out <= 1'b1;
               state <= PREAMBLE4;
            end
            PREAMBLE4: begin
               mdio_tri <= 1'b0;
               mdio_out <= 1'b1;
               state <= PREAMBLE5;
            end
            PREAMBLE5: begin
               mdio_tri <= 1'b0;
               mdio_out <= 1'b1;
               state <= PREAMBLE6;
            end
            PREAMBLE6: begin
               mdio_tri <= 1'b0;
               mdio_out <= 1'b1;
               state <= PREAMBLE7;
            end
            PREAMBLE7: begin
               mdio_tri <= 1'b0;
               mdio_out <= 1'b1;
               state <= PREAMBLE8;
            end
            PREAMBLE8: begin
               mdio_tri <= 1'b0;
               mdio_out <= 1'b1;
               state <= PREAMBLE9;
            end
            PREAMBLE9: begin
               mdio_tri <= 1'b0;
               mdio_out <= 1'b1;
               state <= PREAMBLE10;
            end
            PREAMBLE10: begin
               mdio_tri <= 1'b0;
               mdio_out <= 1'b1;
               state <= PREAMBLE11;
            end
            PREAMBLE11: begin
               mdio_tri <= 1'b0;
               mdio_out <= 1'b1;
               state <= PREAMBLE12;
            end
            PREAMBLE12: begin
               mdio_tri <= 1'b0;
               mdio_out <= 1'b1;
               state <= PREAMBLE13;
            end
            PREAMBLE13: begin
               mdio_tri <= 1'b0;
               mdio_out <= 1'b1;
               state <= PREAMBLE14;
            end
            PREAMBLE14: begin
               mdio_tri <= 1'b0;
               mdio_out <= 1'b1;
               state <= PREAMBLE15;
            end
            PREAMBLE15: begin
               mdio_tri <= 1'b0;
               mdio_out <= 1'b1;
               state <= PREAMBLE16;
            end
            PREAMBLE16: begin
               mdio_tri <= 1'b0;
               mdio_out <= 1'b1;
               state <= PREAMBLE17;
            end
            PREAMBLE17: begin
               mdio_tri <= 1'b0;
               mdio_out <= 1'b1;
               state <= PREAMBLE18;
            end
            PREAMBLE18: begin
               mdio_tri <= 1'b0;
               mdio_out <= 1'b1;
               state <= PREAMBLE19;
            end
            PREAMBLE19: begin
               mdio_tri <= 1'b0;
               mdio_out <= 1'b1;
               state <= PREAMBLE20;
            end
            PREAMBLE20: begin
               mdio_tri <= 1'b0;
               mdio_out <= 1'b1;
               state <= PREAMBLE21;
            end
            PREAMBLE21: begin
               mdio_tri <= 1'b0;
               mdio_out <= 1'b1;
               state <= PREAMBLE22;
            end
            PREAMBLE22: begin
               mdio_tri <= 1'b0;
               mdio_out <= 1'b1;
               state <= PREAMBLE23;
            end
            PREAMBLE23: begin
               mdio_tri <= 1'b0;
               mdio_out <= 1'b1;
               state <= PREAMBLE24;
            end
            PREAMBLE24: begin
               mdio_tri <= 1'b0;
               mdio_out <= 1'b1;
               state <= PREAMBLE25;
            end
            PREAMBLE25: begin
               mdio_tri <= 1'b0;
               mdio_out <= 1'b1;
               state <= PREAMBLE26;
            end
            PREAMBLE26: begin
               mdio_tri <= 1'b0;
               mdio_out <= 1'b1;
               state <= PREAMBLE27;
            end
            PREAMBLE27: begin
               mdio_tri <= 1'b0;
               mdio_out <= 1'b1;
               state <= PREAMBLE28;
            end
            PREAMBLE28: begin
               mdio_tri <= 1'b0;
               mdio_out <= 1'b1;
               state <= PREAMBLE29;
            end
            PREAMBLE29: begin
               mdio_tri <= 1'b0;
               mdio_out <= 1'b1;
               state <= PREAMBLE30;
            end
            PREAMBLE30: begin
               mdio_tri <= 1'b0;
               mdio_out <= 1'b1;
               state <= PREAMBLE31;
            end
            PREAMBLE31: begin
               mdio_tri <= 1'b0;
               mdio_out <= 1'b1;
               state <= PREAMBLE32;
            end
            PREAMBLE32: begin
               mdio_tri <= 1'b0;
               mdio_out <= 1'b1;
               state <= START1;
            end
            // Start code for Clause 22 is 01 and Clause 45 is 00
            START1: begin
               mdio_tri <= 1'b0;
               mdio_out <= 1'b0;
               if (mdio_operation[12])
                  // Clause 45 bit set.
                  state <= C45_START2;
               else
                  state <= C22_START2;
            end
            // 2nd Clause 22 start bit is a 1
            C22_START2: begin
               mdio_tri <= 1'b0;
               mdio_out <= 1'b1;
               state <= OP1;
            end
            // 2nd Clause 45 start bit is a 0
            C45_START2: begin
               mdio_tri <= 1'b0;
               mdio_out <= 1'b0;
               state <= OP1;
            end
            // Both Clause 22 & 45 use 2 bits for operation and are compatable.
            // Note we don't screen here for illegal Clause 22 ops.
            OP1: begin
               mdio_tri <= 1'b0;
               mdio_out <= mdio_operation[11];
               state <= OP2;
            end
            OP2: begin
               mdio_tri <= 1'b0;
               mdio_out <= mdio_operation[10];
               state <= PRTAD1;
            end
            // Both Clause 22 & 45 use 2 sucsessive 5 bit fields to form a hierarchical address
            // though it's used slightly different between the 2 standards.
            PRTAD1: begin
               mdio_tri <= 1'b0;
               mdio_out <= mdio_operation[9];
               state <= PRTAD2;
            end
            PRTAD2: begin
               mdio_tri <= 1'b0;
               mdio_out <= mdio_operation[8];
               state <= PRTAD3;
            end
            PRTAD3: begin
               mdio_tri <= 1'b0;
               mdio_out <= mdio_operation[7];
               state <= PRTAD4;
            end
            PRTAD4: begin
               mdio_tri <= 1'b0;
               mdio_out <= mdio_operation[6];
               state <= PRTAD5;
            end
            PRTAD5: begin
               mdio_tri <= 1'b0;
               mdio_out <= mdio_operation[5];
               state <= DEVAD1;
            end
            DEVAD1: begin
               mdio_tri <= 1'b0;
               mdio_out <= mdio_operation[4];
               state <= DEVAD2;
            end
            DEVAD2: begin
               mdio_tri <= 1'b0;
               mdio_out <= mdio_operation[3];
               state <= DEVAD3;
            end
            DEVAD3: begin
               mdio_tri <= 1'b0;
               mdio_out <= mdio_operation[2];
               state <= DEVAD4;
            end
            DEVAD4: begin
               mdio_tri <= 1'b0;
               mdio_out <= mdio_operation[1];
               state <= DEVAD5;
            end
            DEVAD5: begin
               mdio_tri <= 1'b0;
               mdio_out <= mdio_operation[0];
               state <= TA1;
            end
            // Both Clause 22 & Clause 45 use the same turn around on the bus.
            // Reads have Z as the first bit and 0 driven by the slave for the 2nd bit.
            // Note that slaves drive the bus on the rising edge of MDC.
            // Writes and Address cycles have 10 driven by the master.
            TA1: begin
               if (mdio_operation[11] == 1'b0) // Write/Address
               begin
                  mdio_tri <= 1'b0;
                  mdio_out <= 1'b1;
                  state <= TA2;
               end
               else // Read
               begin
                  mdio_tri <= 1'b1;
                  state <= TA3;
               end
            end
            TA2: begin
               mdio_tri <= 1'b0;
               mdio_out <= 1'b0;
               if ( !mdio_operation[12]) // Clause 22 Write
                  state <= WRITE1;
               else if (mdio_operation[10]) // Clause 45 Write
                  state <= WRITE1;
               else // Clause 45 ADDRESS
                  state <= C45_ADDR1;
            end
            TA3: begin
               mdio_tri <= 1'b1;
               state <= READ1;
            end
            // Clause 22 Reads and both forms of clause 45 Reads have the same bus transaction from here out.
            READ1: begin
               mdio_tri <= 1'b1;
               mdio_read_data[15] <= mdio_in;
               state <= READ2;
            end
            READ2: begin
               mdio_tri <= 1'b1;
               mdio_read_data[14] <= mdio_in;
               state <= READ3;
            end
            READ3: begin
               mdio_tri <= 1'b1;
               mdio_read_data[13] <= mdio_in;
               state <= READ4;
            end
            READ4: begin
               mdio_tri <= 1'b1;
               mdio_read_data[12] <= mdio_in;
               state <= READ5;
            end
            READ5: begin
               mdio_tri <= 1'b1;
               mdio_read_data[11] <= mdio_in;
               state <= READ6;
            end
            READ6: begin
               mdio_tri <= 1'b1;
               mdio_read_data[10] <= mdio_in;
               state <= READ7;
            end
            READ7: begin
               mdio_tri <= 1'b1;
               mdio_read_data[9] <= mdio_in;
               state <= READ8;
            end
            READ8: begin
               mdio_tri <= 1'b1;
               mdio_read_data[8] <= mdio_in;
               state <= READ9;
            end
            READ9: begin
               mdio_tri <= 1'b1;
               mdio_read_data[7] <= mdio_in;
               state <= READ10;
            end
            READ10: begin
               mdio_tri <= 1'b1;
               mdio_read_data[6] <= mdio_in;
               state <= READ11;
            end
            READ11: begin
               mdio_tri <= 1'b1;
               mdio_read_data[5] <= mdio_in;
               state <= READ12;
            end
            READ12: begin
               mdio_tri <= 1'b1;
               mdio_read_data[4] <= mdio_in;
               state <= READ13;
            end
            READ13: begin
               mdio_tri <= 1'b1;
               mdio_read_data[3] <= mdio_in;
               state <= READ14;
            end
            READ14: begin
               mdio_tri <= 1'b1;
               mdio_read_data[2] <= mdio_in;
               state <= READ15;
            end
            READ15: begin
               mdio_tri <= 1'b1;
               mdio_read_data[1] <= mdio_in;
               state <= READ16;
            end
            READ16: begin
               mdio_tri <= 1'b1;
               mdio_read_data[0] <= mdio_in;
               state <= PREIDLE;
               mdio_done <= 1'b1;
            end
            // Write 16bits of data for all types of Write.
            WRITE1: begin
               mdio_tri <= 1'b0;
               mdio_out <= mdio_write_data[15];
               state <= WRITE2;
            end
            WRITE2: begin
               mdio_tri <= 1'b0;
               mdio_out <= mdio_write_data[14];
               state <= WRITE3;
            end
            WRITE3: begin
               mdio_tri <= 1'b0;
               mdio_out <= mdio_write_data[13];
               state <= WRITE4;
            end
            WRITE4: begin
               mdio_tri <= 1'b0;
               mdio_out <= mdio_write_data[12];
               state <= WRITE5;
            end
            WRITE5: begin
               mdio_tri <= 1'b0;
               mdio_out <= mdio_write_data[11];
               state <= WRITE6;
            end
            WRITE6: begin
               mdio_tri <= 1'b0;
               mdio_out <= mdio_write_data[10];
               state <= WRITE7;
            end
            WRITE7: begin
               mdio_tri <= 1'b0;
               mdio_out <= mdio_write_data[9];
               state <= WRITE8;
            end
            WRITE8: begin
               mdio_tri <= 1'b0;
               mdio_out <= mdio_write_data[8];
               state <= WRITE9;
            end
            WRITE9: begin
               mdio_tri <= 1'b0;
               mdio_out <= mdio_write_data[7];
               state <= WRITE10;
            end
            WRITE10: begin
               mdio_tri <= 1'b0;
               mdio_out <= mdio_write_data[6];
               state <= WRITE11;
            end
            WRITE11: begin
               mdio_tri <= 1'b0;
               mdio_out <= mdio_write_data[5];
               state <= WRITE12;
            end
            WRITE12: begin
               mdio_tri <= 1'b0;
               mdio_out <= mdio_write_data[4];
               state <= WRITE13;
            end
            WRITE13: begin
               mdio_tri <= 1'b0;
               mdio_out <= mdio_write_data[3];
               state <= WRITE14;
            end
            WRITE14: begin
               mdio_tri <= 1'b0;
               mdio_out <= mdio_write_data[2];
               state <= WRITE15;
            end
            WRITE15: begin
               mdio_tri <= 1'b0;
               mdio_out <= mdio_write_data[1];
               state <= WRITE16;
            end
            WRITE16: begin
               mdio_tri <= 1'b0;
               mdio_out <= mdio_write_data[0];
               state <= PREIDLE;
               mdio_done <= 1'b1;
            end
            // Write 16bits of address for a Clause 45 Address transaction
            C45_ADDR1: begin
               mdio_tri <= 1'b0;
               mdio_out <= mdio_address[15];
               state <= C45_ADDR2;
            end
            C45_ADDR2: begin
               mdio_tri <= 1'b0;
               mdio_out <= mdio_address[14];
               state <= C45_ADDR3;
            end
            C45_ADDR3: begin
               mdio_tri <= 1'b0;
               mdio_out <= mdio_address[13];
               state <= C45_ADDR4;
            end
            C45_ADDR4: begin
               mdio_tri <= 1'b0;
               mdio_out <= mdio_address[12];
               state <= C45_ADDR5;
            end
            C45_ADDR5: begin
               mdio_tri <= 1'b0;
               mdio_out <= mdio_address[11];
               state <= C45_ADDR6;
            end
            C45_ADDR6: begin
               mdio_tri <= 1'b0;
               mdio_out <= mdio_address[10];
               state <= C45_ADDR7;
            end
            C45_ADDR7: begin
               mdio_tri <= 1'b0;
               mdio_out <= mdio_address[9];
               state <= C45_ADDR8;
            end
            C45_ADDR8: begin
               mdio_tri <= 1'b0;
               mdio_out <= mdio_address[8];
               state <= C45_ADDR9;
            end
            C45_ADDR9: begin
               mdio_tri <= 1'b0;
               mdio_out <= mdio_address[7];
               state <= C45_ADDR10;
            end
            C45_ADDR10: begin
               mdio_tri <= 1'b0;
               mdio_out <= mdio_address[6];
               state <= C45_ADDR11;
            end
            C45_ADDR11: begin
               mdio_tri <= 1'b0;
               mdio_out <= mdio_address[5];
               state <= C45_ADDR12;
            end
            C45_ADDR12: begin
               mdio_tri <= 1'b0;
               mdio_out <= mdio_address[4];
               state <= C45_ADDR13;
            end
            C45_ADDR13: begin
               mdio_tri <= 1'b0;
               mdio_out <= mdio_address[3];
               state <= C45_ADDR14;
            end
            C45_ADDR14: begin
               mdio_tri <= 1'b0;
               mdio_out <= mdio_address[2];
               state <= C45_ADDR15;
            end
            C45_ADDR15: begin
               mdio_tri <= 1'b0;
               mdio_out <= mdio_address[1];
               state <= C45_ADDR16;
            end
            C45_ADDR16: begin
               mdio_tri <= 1'b0;
               mdio_out <= mdio_address[0];
               state <= PREIDLE;
               mdio_done <= 1'b1;
            end
            // PREIDLE allows the mdio_running bit to reset.
            PREIDLE: begin
               state <= IDLE;
            end
         endcase // case(state)
      end // if (mdc_falling_edge)
   end

endmodule


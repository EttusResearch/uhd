//------------------------------------------------------------------------------
// File       : gige_sfp_mdio_tx_elastic_buffer.v
// Author     : Xilinx Inc.
//------------------------------------------------------------------------------
// (c) Copyright 2002-2008 Xilinx, Inc. All rights reserved.
//
// This file contains confidential and proprietary information
// of Xilinx, Inc. and is protected under U.S. and
// international copyright and other intellectual property
// laws.
//
// DISCLAIMER
// This disclaimer is not a license and does not grant any
// rights to the materials distributed herewith. Except as
// otherwise provided in a valid license issued to you by
// Xilinx, and to the maximum extent permitted by applicable
// law: (1) THESE MATERIALS ARE MADE AVAILABLE "AS IS" AND
// WITH ALL FAULTS, AND XILINX HEREBY DISCLAIMS ALL WARRANTIES
// AND CONDITIONS, EXPRESS, IMPLIED, OR STATUTORY, INCLUDING
// BUT NOT LIMITED TO WARRANTIES OF MERCHANTABILITY, NON-
// INFRINGEMENT, OR FITNESS FOR ANY PARTICULAR PURPOSE; and
// (2) Xilinx shall not be liable (whether in contract or tort,
// including negligence, or under any other theory of
// liability) for any loss or damage of any kind or nature
// related to, arising under or in connection with these
// materials, including for any direct, or any indirect,
// special, incidental, or consequential loss or damage
// (including loss of data, profits, goodwill, or any type of
// loss or damage suffered as a result of any action brought
// by a third party) even if such damage or loss was
// reasonably foreseeable or Xilinx had been advised of the
// possibility of the same.
//
// CRITICAL APPLICATIONS
// Xilinx products are not designed or intended to be fail-
// safe, or for use in any application requiring fail-safe
// performance, such as life-support or safety devices or
// systems, Class III medical devices, nuclear facilities,
// applications related to the deployment of airbags, or any
// other applications that could lead to death, personal
// injury, or severe property or environmental damage
// (individually and collectively, "Critical
// Applications"). Customer assumes the sole risk and
// liability of any use of Xilinx products in Critical
// Applications, subject only to applicable laws and
// regulations governing limitations on product liability.
//
// THIS COPYRIGHT NOTICE AND DISCLAIMER MUST BE RETAINED AS
// PART OF THIS FILE AT ALL TIMES. 
// 
// 
//------------------------------------------------------------------------------
// Description: This is the Transmitter Elastic Buffer for the design
//              example of the Ethernet 1000BASE-X PCS/PMA or SGMII
//              core.
//
//              The FIFO is created from Distributed Memory and is of
//              depth 16 words.
//
//              When the write clock is a few parts per million faster
//              than the read clock, the occupancy of the FIFO will
//              increase and Idles should be removed. A MAC transmitter
//              should always insert a minimum of 12 Idles in a single
//              Inter-Packet Gap.  The IEEE802.3 specification allows
//              for up to 4 Idles to be lost within the system (eg. due
//              to clock correction) so that a minimum of 8 Idles should
//              always be presented to a MAC receiver.  Consequently the
//              logic in this example design will only remove a single
//              Idle per minimum Inter-Packet Gap. This leaves clock
//              correction potential for other components in the overall
//              system.
//
//              When the read clock is a few parts per million faster
//              than the write clock, the occupancy of the FIFO will
//              decrease and Idles should be inserted.  The logic in
//              this example design will always insert as many idles as
//              necessary in every Inter-frame Gap period to restore the
//              FIFO occupancy.
//
//              Because the Idle insertion logic is stronger than the
//              Idle removal logic, the bias in this example design is
//              to keep the occupancy of the FIFO low.  This allows more
//              overhead for the FIFO to fill up during heavy bursts of
//              traffic.


`timescale 1 ps/1 ps


//------------------------------------------------------------------------------
// Module declaration.
//------------------------------------------------------------------------------

module gige_sfp_mdio_tx_elastic_buffer
   (

      input            reset,            // Asynchronous Reset

      // Signals received from the input gmii_tx_clk_wr domain.
      //------------------------------------------------------

      input            gmii_tx_clk_wr,   // Write clock domain.
      input      [7:0] gmii_txd_wr,      // gmii_txd synchronous to gmii_tx_clk_wr.
      input            gmii_tx_en_wr,    // gmii_tx_en synchronous to gmii_tx_clk_wr.
      input            gmii_tx_er_wr,    // gmii_tx_er synchronous to gmii_tx_clk_wr.

      // Signals transfered onto the new gmii_tx_clk_rd domain.
      //-------------------------------------------------------

      input            gmii_tx_clk_rd,   // Read clock domain.
      output     [7:0] gmii_txd_rd,      // gmii_txd synchronous to gmii_tx_clk_rd.
      output           gmii_tx_en_rd,    // gmii_tx_en synchronous to gmii_tx_clk_rd.
      output           gmii_tx_er_rd     // gmii_tx_er synchronous to gmii_tx_clk_rd.
   );



  //----------------------------------------------------------------------------
  // Signal declarations
  //----------------------------------------------------------------------------

  wire [3:0]   lower_threshold;      // FIFO occupancy should be kept at 6 or above.
  wire [3:0]   upper_threshold;      // FIFO occupancy should be kept at 9 or below.


  // create a synchronous reset in the write clock domain
  wire         reset_wr;

  // create a synchronous reset in the read clock domain
  wire         reset_rd;

  reg [7:0]    gmii_txd_wr_reg;      // Registered version of gmii_txd_wr.
  reg          gmii_tx_en_wr_reg;    // Registered version of gmii_tx_en_wr.
  reg          gmii_tx_er_wr_reg;    // Registered version of gmii_tx_er_wr.
  reg          wr_enable;            // write enable for FIFO.
  reg          rd_enable;            // read enable for FIFO.
  wire         nearly_full;          // FIFO is getting full.
  wire         nearly_empty;         // FIFO is running empty.
  reg  [3:0]   wr_addr_plus2;        // Always ahead of the FIFO write address by 2.
  reg  [3:0]   wr_addr_plus1;        // Always ahead of the FIFO write address by 1.
  reg  [3:0]   wr_addr;              // FIFO write address.
  reg  [3:0]   wr_addrgray;          // FIFO write address converted to Gray Code.
  wire [3:0]   wag_readsync;         // wr_addrgray Registered on read clock for the 2nd time.
  wire [3:0]   wr_addrbin;           // wag_readsync converted back to binary - on READ clock.
  reg  [3:0]   rd_addr_plus2;        // Always ahead of the FIFO read address by 2.
  reg  [3:0]   rd_addr_plus1;        // Always ahead of the FIFO read address by 1.
  reg  [3:0]   rd_addr;              // FIFO read address.
  reg  [3:0]   rd_addrgray;          // FIFO read address converted to Gray Code.
  wire [3:0]   rag_writesync;        // rd_addrgray Registered on write clock for the 2nd time.
  wire [3:0]   rd_addrbin;           // rag_writesync converted back to binary - on WRITE clock.
  wire         tx_en_fifo;           // gmii_tx_en_wr read out of FIFO.
  wire         tx_er_fifo;           // gmii_tx_er_wr read out of FIFO.
  wire [7:0]   txd_fifo;             // gmii_txd_wr read out of FIFO.
  reg          tx_en_fifo_reg1;      // Registered version of tx_en_fifo.
  reg          tx_er_fifo_reg1;      // Registered version of tx_er_fifo.
  reg  [7:0]   txd_fifo_reg1;        // Registered version of txd_fifo.
  reg          tx_en_fifo_reg2;      // Registered version of tx_en_fifo_reg1.
  reg          tx_er_fifo_reg2;      // Registered version of tx_er_fifo_reg1.
  reg  [7:0]   txd_fifo_reg2;        // Registered version of txd_fifo_reg1. 
  reg  [3:0]   wr_occupancy;         // The occupancy of the FIFO in write clock domain.
  reg  [3:0]   rd_occupancy;         // The occupancy of the FIFO in read clock domain.
  wire         wr_idle;              // Detect an Idle written into the FIFO in the write clock domain.
  wire         rd_idle;              // Detect an Idle read out of the FIFO in the read clock domain.
  reg  [3:0]   ipg_count;            // Count the Inter-Packet Gap period.
  reg          allow_idle_removal;   // Allow the removal of a single Idle.


  // Assign the Upper and Lower thresholds for the FIFO.  These are used
  // to determine the nearly_full and nearly_empty signals.

  // FIFO occupancy should be kept at 6 or above.
  assign lower_threshold = 4'b0110;

  // FIFO occupancy should be kept at 9 or below.
  assign upper_threshold = 4'b1001;



//------------------------------------------------------------------------------
// FIFO write logic (Idles are removed as necessary).
//------------------------------------------------------------------------------



   // Create a synchronous reset in the write clock domain.
   gige_sfp_mdio_reset_sync gen_wr_reset (
      .clk            (gmii_tx_clk_wr),
      .reset_in       (reset),
      .reset_out      (reset_wr)
   );



   // Reclock the GMII Tx inputs.
   always @(posedge gmii_tx_clk_wr)
   begin : reclock_gmii
     if (reset_wr == 1'b1) begin
        gmii_txd_wr_reg   <= 8'b0;
        gmii_tx_en_wr_reg <= 1'b0;
        gmii_tx_er_wr_reg <= 1'b0;
     end

     else begin
        gmii_txd_wr_reg   <= gmii_txd_wr;
        gmii_tx_en_wr_reg <= gmii_tx_en_wr;
        gmii_tx_er_wr_reg <= gmii_tx_er_wr;
     end
   end // reclock_gmii



   // Detect Idles (Normal inter-frame encodings as desribed in
   // IEEE802.3 table 35-2)
   assign wr_idle = (
                    // 1st type of Idle.
                    (gmii_tx_en_wr == 1'b0 && gmii_tx_er_wr == 1'b0)
                    ||
                    // 2nd type of Idle.
                    (gmii_tx_en_wr == 1'b0 && gmii_tx_er_wr == 1'b1
                     && gmii_txd_wr == 8'b0)

                    ) ? 1'b1 : 1'b0;



   // Create a counter to count from 0 to 8.  When the counter reaches 8
   // it is reset to 0 and a pulse is generated (allow_idle_removal).

   // allow_idle_removal is therefore high for a single clock cycle once
   // every 9 clock periods.  This is used to ensure that the Idle
   // removal logic will only ever remove a single idle from a minimum
   // transmitter interframe gap (12 idles).  This leaves clock
   // correction potential for other components in the overall system
   // (the IEEE802.3 spec allows for a total of 4 idles to be lost
   // between a MAC transmitter and a MAC receiver).
   always @(posedge gmii_tx_clk_wr)
   begin : idle_removal_control
     if (reset_wr == 1'b1) begin
        ipg_count          <= 4'b0;
        allow_idle_removal <= 1'b0;
     end
     else begin
        if (ipg_count[3] == 1'b1) begin
           ipg_count          <= 4'b0;
           allow_idle_removal <= 1'b1;
        end
        else begin
           ipg_count          <= ipg_count + 4'b1;
           allow_idle_removal <= 1'b0;
        end
     end
   end // idle_removal_control



   // Create the FIFO write enable.  This is not asserted when Idles are
   // to be removed.
   always @(posedge gmii_tx_clk_wr)
   begin : gen_wr_enable
     if (reset_wr == 1'b1)
        wr_enable <= 1'b0;
     else begin
        if (wr_idle == 1'b1 && allow_idle_removal == 1'b1
           && nearly_full == 1'b1)                     // remove 1 Idle.
           wr_enable <= 1'b0;
        else
           wr_enable <= 1'b1;
     end
   end // gen_wr_enable



   // Create the FIFO write address pointer.  Note that wr_addr_plus2
   // will be converted to gray code and passed across the async clock
   // boundary.
   always @(posedge gmii_tx_clk_wr)
   begin : gen_wr_addr
     if (reset_wr == 1'b1) begin
        wr_addr_plus2 <= 4'b0010;
        wr_addr_plus1 <= 4'b0001;
        wr_addr       <= 4'b0000;
     end

     else if (wr_enable == 1'b1) begin
        wr_addr_plus2 <= wr_addr_plus2 + 4'b0001;
        wr_addr_plus1 <= wr_addr_plus2;
        wr_addr       <= wr_addr_plus1;
     end
   end // gen_wr_addr



//------------------------------------------------------------------------------
// Build FIFO out of distributed RAM.
//------------------------------------------------------------------------------



   genvar i;
   generate for (i=0; i<8; i=i+1)
     begin : gen_txd_fifo_bus

       RAM16X1D gen_txd_fifo_bit
       (
          .D(gmii_txd_wr_reg[i]),
          .WE(wr_enable),
          .WCLK(gmii_tx_clk_wr),
          .A0(wr_addr[0]),
          .A1(wr_addr[1]),
          .A2(wr_addr[2]),
          .A3(wr_addr[3]),
          .DPRA0(rd_addr[0]),
          .DPRA1(rd_addr[1]),
          .DPRA2(rd_addr[2]),
          .DPRA3(rd_addr[3]),
     
          .SPO(),
          .DPO(txd_fifo[i])
       );

     end
   endgenerate



   RAM16X1D gen_tx_en_fifo
   (
      .D(gmii_tx_en_wr_reg),
      .WE(wr_enable),
      .WCLK(gmii_tx_clk_wr),
      .A0(wr_addr[0]),
      .A1(wr_addr[1]),
      .A2(wr_addr[2]),
      .A3(wr_addr[3]),
      .DPRA0(rd_addr[0]),
      .DPRA1(rd_addr[1]),
      .DPRA2(rd_addr[2]),
      .DPRA3(rd_addr[3]),

      .SPO(),
      .DPO(tx_en_fifo)
   );



   RAM16X1D gen_tx_er_fifo
   (
      .D(gmii_tx_er_wr_reg),
      .WE(wr_enable),
      .WCLK(gmii_tx_clk_wr),
      .A0(wr_addr[0]),
      .A1(wr_addr[1]),
      .A2(wr_addr[2]),
      .A3(wr_addr[3]),
      .DPRA0(rd_addr[0]),
      .DPRA1(rd_addr[1]),
      .DPRA2(rd_addr[2]),
      .DPRA3(rd_addr[3]),

      .SPO(),
      .DPO(tx_er_fifo)
   );



//------------------------------------------------------------------------------
// FIFO read logic (Idles are repeated as necessary).
//------------------------------------------------------------------------------



   // Create a synchronous reset in the read clock domain.
   gige_sfp_mdio_reset_sync gen_rd_reset (
      .clk            (gmii_tx_clk_rd),
      .reset_in       (reset),
      .reset_out      (reset_rd)
   );



   // Register the FIFO outputs.
   always @(posedge gmii_tx_clk_rd)
   begin : drive_new_gmii
     if (reset_rd == 1'b1) begin
        txd_fifo_reg1   <= 8'b0;
        tx_en_fifo_reg1 <= 1'b0;
        tx_er_fifo_reg1 <= 1'b0;
        txd_fifo_reg2   <= 8'b0;
        tx_en_fifo_reg2 <= 1'b0;
        tx_er_fifo_reg2 <= 1'b0;
     end

     else begin
        txd_fifo_reg1   <= txd_fifo;
        tx_en_fifo_reg1 <= tx_en_fifo;
        tx_er_fifo_reg1 <= tx_er_fifo;
        txd_fifo_reg2   <= txd_fifo_reg1;
        tx_en_fifo_reg2 <= tx_en_fifo_reg1;
        tx_er_fifo_reg2 <= tx_er_fifo_reg1;
     end                             
   end // drive_new_gmii



   // Route GMII outputs, now synchronous to gmii_tx_clk_rd.
   assign gmii_txd_rd   = txd_fifo_reg2;
   assign gmii_tx_en_rd = tx_en_fifo_reg2;
   assign gmii_tx_er_rd = tx_er_fifo_reg2;



   // Detect Idles (Normal inter-frame encodings as desribed in
   // IEEE802.3 table 35-2)
   assign rd_idle = (
                    // 1st type of Idle.
                    (tx_en_fifo_reg1 == 1'b0 && tx_er_fifo_reg1 == 1'b0)
                    ||
                    // 2nd type of Idle.
                    (tx_en_fifo_reg1 == 1'b0 && tx_er_fifo_reg1 == 1'b1
                     && txd_fifo_reg1 == 8'b0)

                    ) ? 1'b1 : 1'b0;



   // Create the FIFO read enable.  This is not asserted when Idles are
   // to be repeated.
   always @(posedge gmii_tx_clk_rd)
   begin : gen_rd_enable
     if (reset_rd == 1'b1)
        rd_enable <= 1'b0;

     else begin
        if (rd_idle == 1'b1            // Detect an Idle
           && nearly_empty == 1'b1)    // when FIFO is running empty.

           // Repeat the Idle by freezing read pointer of FIFO (as
           // many times as necessary).
           rd_enable <= 1'b0;

        else
           rd_enable <= 1'b1;
     end
   end // gen_rd_enable



   // Create the FIFO read address pointer.  Note that rd_addr_plus2
   // will be converted to gray code and passed across the async clock
   // boundary.
   always @(posedge gmii_tx_clk_rd)
   begin : gen_rd_addr
     if (reset_rd == 1'b1) begin
        rd_addr_plus2 <= 4'b0010;
        rd_addr_plus1 <= 4'b0001;
        rd_addr       <= 4'b0000;
     end

     else if (rd_enable == 1'b1) begin
        rd_addr_plus2 <= rd_addr_plus2 + 4'b0001;
        rd_addr_plus1 <= rd_addr_plus2;
        rd_addr       <= rd_addr_plus1;
     end
   end // gen_rd_addr



//------------------------------------------------------------------------------
// Create nearly_full threshold in write clock domain.
//------------------------------------------------------------------------------

// Please refer to Xilinx Application Note 131 for a complete
// description of this logic.



   // Convert Binary Read Pointer to Gray Code.
   always @(posedge gmii_tx_clk_rd)
   begin : rd_addrgray_bits
     if (reset_rd == 1'b1)
        rd_addrgray <= 4'b0;

     else begin
        rd_addrgray[3] <= rd_addr_plus2[3];
        rd_addrgray[2] <= rd_addr_plus2[3] ^ rd_addr_plus2[2];
        rd_addrgray[1] <= rd_addr_plus2[2] ^ rd_addr_plus2[1];
        rd_addrgray[0] <= rd_addr_plus2[1] ^ rd_addr_plus2[0];
     end
   end // rd_addrgray_bits



   // Register rd_addrgray on gmii_tx_clk_wr.  By reclocking the gray
   // code, the worst case senario is that the reclocked value is only
   // in error by -1, since only 1 bit at a time changes between gray
   // code increment.
   genvar j;
   generate for (j=0; j<4; j=j+1)
     begin : reclock_rd_addrgray

       gige_sfp_mdio_sync_block sync_rd_addrgray
       (
         .clk       (gmii_tx_clk_wr),
         .data_in   (rd_addrgray[j]),
         .data_out  (rag_writesync[j])
       );

     end
   endgenerate



   // Convert rag_writesync Gray Code read address back to binary.
   // This has crossed clock domains from gmii_tx_clk_rd to
   // gmii_tx_clk_wr.
   assign rd_addrbin[3] = rag_writesync[3];
   assign rd_addrbin[2] = rag_writesync[3] ^ rag_writesync[2];

   assign rd_addrbin[1] = rag_writesync[3] ^ rag_writesync[2]
                          ^ rag_writesync[1];

   assign rd_addrbin[0] = rag_writesync[3] ^ rag_writesync[2]
                          ^ rag_writesync[1] ^ rag_writesync[0];



   // Determine the occupancy of the FIFO.  One clock of latency is
   // created here by registering wr_occupancy.
   always @(posedge gmii_tx_clk_wr)
   begin : gen_wr_occupancy
      wr_occupancy <= wr_addr - rd_addrbin;
   end // gen_wr_occupancy



   // Set nearly_full flag if FIFO occupancy is greater than
   // upper_threshold.
   assign nearly_full = (wr_occupancy > upper_threshold) ? 1'b1 : 1'b0;



//------------------------------------------------------------------------------
// Create nearly_empty threshold logic in read clock domain.
//------------------------------------------------------------------------------

// Please refer to Xilinx Application Note 131 for a complete
// description of this logic.



   // Convert Binary Write Pointer to Gray Code.
   always @(posedge gmii_tx_clk_wr)
   begin : wr_addrgray_bits
     if (reset_wr == 1'b1)
        wr_addrgray <= 4'b0;

     else begin
        wr_addrgray[3] <= wr_addr_plus2[3];
        wr_addrgray[2] <= wr_addr_plus2[3] ^ wr_addr_plus2[2];
        wr_addrgray[1] <= wr_addr_plus2[2] ^ wr_addr_plus2[1];
        wr_addrgray[0] <= wr_addr_plus2[1] ^ wr_addr_plus2[0];
     end
   end // wr_addrgray_bits



   // Register wr_addrgray on gmii_tx_clk_rd.  By reclocking the gray
   // code, the worst case senario is that the reclocked value is only
   // in error by -1, since only 1 bit at a time changes between gray
   // code increment.
   genvar k;
   generate for (k=0; k<4; k=k+1)
     begin : reclock_wr_addrgray

       gige_sfp_mdio_sync_block sync_wr_addrgray
       (
         .clk       (gmii_tx_clk_rd),
         .data_in   (wr_addrgray[k]),
         .data_out  (wag_readsync[k])
       );

     end
   endgenerate



   // Convert wag_readsync Gray Code write address back to binary.
   // This has crossed clock domains from gmii_tx_clk_wr to
   // gmii_tx_clk_rd.
   assign wr_addrbin[3] = wag_readsync[3];
   assign wr_addrbin[2] = wag_readsync[3] ^ wag_readsync[2];

   assign wr_addrbin[1] = wag_readsync[3] ^ wag_readsync[2]
                          ^ wag_readsync[1];

   assign wr_addrbin[0] = wag_readsync[3] ^ wag_readsync[2]
                          ^ wag_readsync[1] ^ wag_readsync[0];



   // Determine the occupancy of the FIFO.  One clock of latency is
   // created here by registering rd_occupancy.
   always @(posedge gmii_tx_clk_rd)
   begin : gen_rd_occupancy
      rd_occupancy <= wr_addrbin - rd_addr;
   end // gen_rd_occupancy



   // Set nearly_empty flag if FIFO occupancy is less than
   // lower_threshold.
   assign nearly_empty = (rd_occupancy < lower_threshold) ? 1'b1 : 1'b0;



endmodule


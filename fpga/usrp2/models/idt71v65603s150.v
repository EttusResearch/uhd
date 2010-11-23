/*******************************************************************************
 *
 *   File Name                 : idt71v65603s150.v
 *   Product                   : IDT71V65603
 *   Function                  : 256K x 36 pipeline ZBT Static RAM
 *   Simulation Tool/Version   : Verilog-XL 2.5
 *   Date                      : 07/19/00
 *
 *   Copyright 1999 Integrated Device Technology, Inc.
 *
 *   Revision Notes:   07/19/00  Rev00
 *      
 ******************************************************************************/
/*******************************************************************************
 * Module Name: idt71v65603s150 
 *
 * Notes                     : This model is believed to be functionally
 *                             accurate.  Please direct any inquiries to
 *                             IDT SRAM Applications at: sramhelp@idt.com
 *                               
 *******************************************************************************/

 /***************************************************************
 *
 *  Integrated Device Technology, Inc. ("IDT") hereby grants the
 *  user of this Verilog/VCS model a non-exclusive, nontransferable
 *  license to use this Verilog/VCS model under the following terms.
 *  The user is granted this license only to use the Verilog/VCS
 *  model and is not granted rights to sell, copy (except as needed
 *  to run the IBIS model), rent, lease or sub-license the Verilog/VCS
 *  model in whole or in part, or in modified form to anyone. The User
 *  may modify the Verilog/VCS model to suit its specific applications,
 *  but rights to derivative works and such modifications shall belong
 *  to IDT.
 *
 *  This Verilog/VCS model is provided on an "AS IS" basis and
 *  IDT makes absolutely no warranty with respect to the information
 *  contained herein. IDT DISCLAIMS AND CUSTOMER WAIVES ALL
 *  WARRANTIES, EXPRESS AND IMPLIED, INCLUDING WARRANTIES OF
 *  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. THE
 *  ENTIRE RISK AS TO QUALITY AND PERFORMANCE IS WITH THE
 *  USER ACCORDINGLY, IN NO EVENT SHALL IDT BE LIABLE
 *   FOR ANY DIRECT OR INDIRECT DAMAGES, WHETHER IN CONTRACT OR
 *  TORT, INCLUDING ANY LOST PROFITS OR OTHER INCIDENTAL,
 *  CONSEQUENTIAL, EXEMPLARY, OR PUNITIVE DAMAGES ARISING OUT OF
 *  THE USE OR APPLICATION OF THE VERILOG/VCS model.  Further,
 *  IDT reserves the right to make changes without notice to any
 *  product herein to improve reliability, function, or design.
 *  IDT does not convey any license under patent rights or
 *  any other intellectual property rights, including those of
 *  third parties. IDT is not obligated to provide maintenance
 *  or support for the licensed Verilog/VCS model.
 *
 ***************************************************************/

 `timescale 1ns/100ps

module idt71v65603s150 (A,
                 adv_ld_,                  // advance (high) / load (low)
                 bw1_, bw2_, bw3_, bw4_,   // byte write enables (low)
                 ce1_, ce2, ce2_,          // chip enables
                 cen_,                     // clock enable (low)
                 clk,                      // clock
                 IO, IOP,                  // data bus
                 lbo_,                     // linear burst order (low)
                 oe_,                      // output enable (low)
                 r_w_);                    // read (high) / write (low)

initial
begin
   $write("\n********************************************************\n");
   $write("   idt71v65603s150, 256K x 36 Pipelined burst ZBT SRAM    \n");
   $write("   Rev: 00   July 2000                                    \n"); 
   $write("   copyright 1997,1998,1999,2000 by IDT, Inc.             \n");
   $write("********************************************************\n\n");
end

input [17:0] A;
inout [31:0] IO;
inout [4:1]  IOP;
input adv_ld_, bw1_, bw2_, bw3_, bw4_, ce1_, ce2, ce2_, 
      cen_, clk, lbo_, oe_, r_w_;


//internal registers for data, address, etc
reg [8:0] mem1[0:262143];    //memory array
reg [8:0] mem2[0:262143];    //memory array
reg [8:0] mem3[0:262143];    //memory array
reg [8:0] mem4[0:262143];    //memory array

reg [35:0] dout;
reg [17:0] addr_a,
           addr_b;
reg  wren_a, wren_b;
reg  cs_a,   cs_b;
reg  bw_a1, bw_b1;
reg  bw_a2, bw_b2;
reg  bw_a3, bw_b3;
reg  bw_a4, bw_b4;
reg  [1:0] brst_cnt;

wire[35:0] data_out;
wire doe;
wire cs = (~ce1_ & ce2 & ~ce2_);
wire baddr0, baddr1;


parameter regdelay = 0.2;
parameter outdly   = 0.2;

specify
specparam
//Clock Parameters
   tCYC  = 6.7,       //clock cycle time
   tCH   = 2.0,       //clock high time
   tCL   = 2.0,       //clock low time

//Output Parameters
   tCD   = 3.8,       //clk to data valid
   tCLZ  = 1.5,       //clk to output Low-Z
   tCHZ  = 3.0,       //clk to data Hi-Z
   tOE   = 3.8,       //OE to output valid
   tOLZ  = 0.0,       //OE to output Hi-Z
   tOHZ  = 3.8,       //OE to output Hi-Z

//Set up times
   tSE   = 1.5,       //clock enable set-up
   tSA   = 1.5,       //address set-up
   tSD   = 1.5,       //data set-up
   tSW   = 1.5,       //Read/Write set-up
   tSADV = 1.5,       //Advance/Load set-up
   tSC   = 1.5,       //Chip enable set-up
   tSB   = 1.5,       //Byte write enable set-up

//Hold times
   tHE   = 0.5,       //clock enable hold
   tHA   = 0.5,       //address hold
   tHD   = 0.5,       //data hold
   tHW   = 0.5,       //Read/Write hold
   tHADV = 0.5,       //Advance/Load hold
   tHC   = 0.5,       //Chip enable hold
   tHB   = 0.5;       //Byte write enable hold


   (oe_ *> IO) = (tOE,tOE,tOHZ,tOLZ,tOHZ,tOLZ); //(01,10,0z,z1,1z,z0)
   (clk *> IO) = (tCD,tCD,tCHZ,tCLZ,tCHZ,tCLZ); //(01,10,0z,z1,1z,z0)

   (oe_ *> IOP) = (tOE,tOE,tOHZ,tOLZ,tOHZ,tOLZ);  //(01,10,0z,z1,1z,z0)
   (clk *> IOP) = (tCD,tCD,tCHZ,tCLZ,tCHZ,tCLZ);  //(01,10,0z,z1,1z,z0)

//timing checks

   $period(posedge clk, tCYC );
   $width (posedge clk, tCH );
   $width (negedge clk, tCL );


   $setuphold(posedge clk, A, tSA, tHA);
   $setuphold(posedge clk, IO, tSD, tHD);
   $setuphold(posedge clk, IOP, tSD, tHD);
   $setuphold(posedge clk, adv_ld_, tSADV, tHADV);
   $setuphold(posedge clk, bw1_, tSB, tHB);
   $setuphold(posedge clk, bw2_, tSB, tHB);
   $setuphold(posedge clk, bw3_, tSB, tHB);
   $setuphold(posedge clk, bw4_, tSB, tHB);
   $setuphold(posedge clk, ce1_, tSC, tHC);
   $setuphold(posedge clk, ce2, tSC, tHC);
   $setuphold(posedge clk, ce2_, tSC, tHC);
   $setuphold(posedge clk, cen_, tSE, tHE);
   $setuphold(posedge clk, r_w_, tSW, tHW);

endspecify

initial begin
  cs_a = 0;
  cs_b = 0;
end


/////////////////////////////////////////////////////////////////////////
//input registers
//--------------------
always @(posedge clk)
begin
   if ( ~cen_ & ~adv_ld_ ) cs_a <= #regdelay cs;
   if ( ~cen_ )            cs_b <= #regdelay cs_a;

   if ( ~cen_ & ~adv_ld_ ) wren_a <= #regdelay (cs & ~r_w_);
   if ( ~cen_ )            wren_b <= #regdelay wren_a;

   if ( ~cen_ ) bw_a1 <= #regdelay ~bw1_;
   if ( ~cen_ ) bw_a2 <= #regdelay ~bw2_;
   if ( ~cen_ ) bw_a3 <= #regdelay ~bw3_;
   if ( ~cen_ ) bw_a4 <= #regdelay ~bw4_;

   if ( ~cen_ ) bw_b1 <= #regdelay bw_a1;
   if ( ~cen_ ) bw_b2 <= #regdelay bw_a2;
   if ( ~cen_ ) bw_b3 <= #regdelay bw_a3;
   if ( ~cen_ ) bw_b4 <= #regdelay bw_a4;

   if ( ~cen_ & ~adv_ld_ ) addr_a[17:0] <= #regdelay A[17:0];
   if ( ~cen_ )   addr_b[17:0] <= #regdelay {addr_a[17:2], baddr1, baddr0};
end


/////////////////////////////////////////////////////////////////////////
//burst counter
//--------------------
always @(posedge clk)
begin
   if      ( lbo_ & ~cen_ & ~adv_ld_) brst_cnt <= #regdelay 0;
   else if (~lbo_ & ~cen_ & ~adv_ld_) brst_cnt <= #regdelay A[1:0];
   else if (        ~cen_ &  adv_ld_) brst_cnt <= #regdelay brst_cnt + 1;
end


/////////////////////////////////////////////////////////////////////////
//address logic
//--------------------
assign baddr1 = lbo_ ? (brst_cnt[1] ^ addr_a[1]) : brst_cnt[1];
assign baddr0 = lbo_ ? (brst_cnt[0] ^ addr_a[0]) : brst_cnt[0];


/////////////////////////////////////////////////////////////////////////
//data output register
//--------------------
always @(posedge clk)
begin
     #regdelay;
     #regdelay;
     dout[8:0]     = mem1[addr_b];
     dout[17:9]    = mem2[addr_b];
     dout[26:18]   = mem3[addr_b];
     dout[35:27]   = mem4[addr_b];
end

assign data_out = dout;


/////////////////////////////////////////////////////////////////////////
//Output buffers: using a bufif1 has the same effect as...
//
//	assign D = doe ? data_out : 36'hz;
//	
//It was coded this way to support SPECIFY delays in the specparam section.
//--------------------
bufif1 #outdly (IO[0],data_out[0],doe);
bufif1 #outdly (IO[1],data_out[1],doe);
bufif1 #outdly (IO[2],data_out[2],doe);
bufif1 #outdly (IO[3],data_out[3],doe);
bufif1 #outdly (IO[4],data_out[4],doe);
bufif1 #outdly (IO[5],data_out[5],doe);
bufif1 #outdly (IO[6],data_out[6],doe);
bufif1 #outdly (IO[7],data_out[7],doe);
bufif1 #outdly (IOP[1],data_out[8],doe);

bufif1 #outdly (IO[8],data_out[9],doe);
bufif1 #outdly (IO[9],data_out[10],doe);
bufif1 #outdly (IO[10],data_out[11],doe);
bufif1 #outdly (IO[11],data_out[12],doe);
bufif1 #outdly (IO[12],data_out[13],doe);
bufif1 #outdly (IO[13],data_out[14],doe);
bufif1 #outdly (IO[14],data_out[15],doe);
bufif1 #outdly (IO[15],data_out[16],doe);
bufif1 #outdly (IOP[2],data_out[17],doe);

bufif1 #outdly (IO[16],data_out[18],doe);
bufif1 #outdly (IO[17],data_out[19],doe);
bufif1 #outdly (IO[18],data_out[20],doe);
bufif1 #outdly (IO[19],data_out[21],doe);
bufif1 #outdly (IO[20],data_out[22],doe);
bufif1 #outdly (IO[21],data_out[23],doe);
bufif1 #outdly (IO[22],data_out[24],doe);
bufif1 #outdly (IO[23],data_out[25],doe);
bufif1 #outdly (IOP[3],data_out[26],doe);

bufif1 #outdly (IO[24],data_out[27],doe);
bufif1 #outdly (IO[25],data_out[28],doe);
bufif1 #outdly (IO[26],data_out[29],doe);
bufif1 #outdly (IO[27],data_out[30],doe);
bufif1 #outdly (IO[28],data_out[31],doe);
bufif1 #outdly (IO[29],data_out[32],doe);
bufif1 #outdly (IO[30],data_out[33],doe);
bufif1 #outdly (IO[31],data_out[34],doe);
bufif1 #outdly (IOP[4],data_out[35],doe);

assign  doe = cs_b & ~wren_b & ~oe_ ; 


/////////////////////////////////////////////////////////////////////////
// write to ram
//-------------
always @(posedge clk)
begin
   if (wren_b & bw_b1 & ~cen_) mem1[addr_b] = {IOP[1], IO[7:0]};
   if (wren_b & bw_b2 & ~cen_) mem2[addr_b] = {IOP[2], IO[15:8]};
   if (wren_b & bw_b3 & ~cen_) mem3[addr_b] = {IOP[3], IO[23:16]};
   if (wren_b & bw_b4 & ~cen_) mem4[addr_b] = {IOP[4], IO[31:24]};
end

endmodule

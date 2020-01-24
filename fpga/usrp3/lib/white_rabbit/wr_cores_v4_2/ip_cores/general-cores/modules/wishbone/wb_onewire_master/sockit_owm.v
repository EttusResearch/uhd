//////////////////////////////////////////////////////////////////////////////
//                                                                          //
//  Minimalistic 1-wire (onewire) master with Avalon MM bus interface       //
//                                                                          //
//  Copyright (C) 2010  Iztok Jeras                                         //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////
//                                                                          //
//  This RTL is free hardware: you can redistribute it and/or modify        //
//  it under the terms of the GNU Lesser General Public License             //
//  as published by the Free Software Foundation, either                    //
//  version 3 of the License, or (at your option) any later version.        //
//                                                                          //
//  This RTL is distributed in the hope that it will be useful,             //
//  but WITHOUT ANY WARRANTY; without even the implied warranty of          //
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           //
//  GNU General Public License for more details.                            //
//                                                                          //
//  You should have received a copy of the GNU General Public License       //
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.   //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
//  Modifications:
//      2016-08-24: by Jan Pospisil (j.pospisil@cern.ch)
//          * added default values; typos
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
//                                                                          //
// The clock divider parameter is computed with the next formula:           //
//                                                                          //
// CDR_N = f_CLK * BTP_N - 1  (example: CDR_N = 1MHz * 5.0us - 1 = 5-1)     //
// CDR_O = f_CLK * BTP_O - 1  (example: CDR_O = 1MHz * 1.0us - 1 = 1-1)     //
//                                                                          //
// If the dividing factor is not a round integer, than the timing of the    //
// controller will be slightly off, and would support only a subset of      //
// 1-wire devices with timing closer to the typical 30us slot.              //
//                                                                          //
// Base time periods BTP_N = "5.0" and BTP_O = "1.0" are optimized for      //
// onewire timing. The default timing restricts the range of available      //
// frequences to multiples of 1MHz.                                         //
//                                                                          //
// If even this restrictions are too strict use timing BTP_N = "6.0" and    //
// BTP_O = "0.5", where the actual periods can be in the range:             //
// 6.0us <= BTP_N <= 7.5us                                                  //
// 0.5us <= BTP_O <= 0.66us                                                 //
//                                                                          //
// A third timing option is available for normal mode BTP_N = "7.5", this   //
// option is optimized for logic size.                                      //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////

module sockit_owm #(
  // enable implementation of optional functionality
  parameter OVD_E =    1,  // overdrive functionality is implemented by default
  parameter CDR_E =    1,  // clock divider register is implemented by default
  // interface parameters
  parameter BDW   =   32,  // bus data width
  parameter OWN   =    1,  // number of 1-wire ports
  // computed bus address port width
`ifdef __ICARUS__
  parameter BAW   = (BDW==32) ? 1 : 2,
`else
  parameter BAW   = 1,  // TODO, the above is correct, but does not work well with Altera SOPC Builder
`endif
  // base time period
  parameter BTP_N = "5.0", // normal    mode (5.0us, options are "7.5", "5.0" and "6.0")
  parameter BTP_O = "0.5", // overdrive mode (1.0us, options are "1.0",       and "0.5")
  // normal mode timing
  parameter T_RSTH_N = (BTP_N == "7.5") ?  64 : (BTP_N == "5.0") ?  96 :  80,  // reset high
  parameter T_RSTL_N = (BTP_N == "7.5") ?  64 : (BTP_N == "5.0") ?  96 :  80,  // reset low
  parameter T_RSTP_N = (BTP_N == "7.5") ?  10 : (BTP_N == "5.0") ?  15 :  10,  // reset presence pulse
  parameter T_DAT0_N = (BTP_N == "7.5") ?   8 : (BTP_N == "5.0") ?  12 :  10,  // bit 0 low
  parameter T_DAT1_N = (BTP_N == "7.5") ?   1 : (BTP_N == "5.0") ?   1 :   1,  // bit 1 low
  parameter T_BITS_N = (BTP_N == "7.5") ?   2 : (BTP_N == "5.0") ?   3 :   2,  // bit sample
  parameter T_RCVR_N = (BTP_N == "7.5") ?   1 : (BTP_N == "5.0") ?   1 :   1,  // recovery
  parameter T_IDLE_N = (BTP_N == "7.5") ? 128 : (BTP_N == "5.0") ? 200 : 160,  // idle timer
  // overdrive mode timing
  parameter T_RSTH_O = (BTP_O == "1.0") ?  48 :  96,  // reset high
  parameter T_RSTL_O = (BTP_O == "1.0") ?  48 :  96,  // reset low
  parameter T_RSTP_O = (BTP_O == "1.0") ?  10 :  15,  // reset presence pulse
  parameter T_DAT0_O = (BTP_O == "1.0") ?   6 :  12,  // bit 0 low
  parameter T_DAT1_O = (BTP_O == "1.0") ?   1 :   2,  // bit 1 low
  parameter T_BITS_O = (BTP_O == "1.0") ?   2 :   3,  // bit sample
  parameter T_RCVR_O = (BTP_O == "1.0") ?   2 :   4,  // recovery
  parameter T_IDLE_O = (BTP_O == "1.0") ?  96 : 192,  // idle timer
  // clock divider ratios (defaults are for a 1MHz clock)
  parameter CDR_N = 5-1,  // normal    mode
  parameter CDR_O = 1-1   // overdrive mode
)(
  // system signals
  input            clk,
  input            rst,
  // CPU bus interface
  input            bus_ren,  // read  enable
  input            bus_wen,  // write enable
  input  [BAW-1:0] bus_adr,  // address
  input  [BDW-1:0] bus_wdt,  // write data
  output [BDW-1:0] bus_rdt,  // read  data
  output           bus_irq,  // interrupt request
  // 1-wire interface
  output [OWN-1:0] owr_p,    // output power enable
  output [OWN-1:0] owr_e,    // output pull down enable
  input  [OWN-1:0] owr_i     // input from bidirectional wire
);

//////////////////////////////////////////////////////////////////////////////
// local parameters
//////////////////////////////////////////////////////////////////////////////

function integer clogb2;               
input [31:0] value;
begin
   for (clogb2 = 0; value > 0; clogb2 = clogb2 + 1)
     value = value >> 1;
end
endfunction

// size of combined power and select registers
localparam PDW = (BDW==32) ? 24 : 8;

// size of boudrate generator counter (divider for normal mode is largest)
localparam CDW = CDR_E ? ((BDW==32) ? 16 : 8) : clogb2(CDR_N);

// size of port select signal
localparam SDW = clogb2(OWN);

// size of cycle timing counter
localparam TDW =       (T_RSTH_O+T_RSTL_O) >       (T_RSTH_N+T_RSTL_N)
               ? clogb2(T_RSTH_O+T_RSTL_O) : clogb2(T_RSTH_N+T_RSTL_N);

//////////////////////////////////////////////////////////////////////////////
// local signals
//////////////////////////////////////////////////////////////////////////////

// address dependent write enable
wire bus_ren_ctl_sts;
wire bus_wen_ctl_sts;
wire bus_wen_pwr_sel;
wire bus_wen_cdr_n;
wire bus_wen_cdr_o;

// read data bus segments
wire     [7:0] bus_rdt_ctl_sts;
wire [PDW-1:0] bus_rdt_pwr_sel;

// clock divider
reg  [CDW-1:0] div;
reg  [CDW-1:0] cdr_n = CDR_N[CDW-1:0];
reg  [CDW-1:0] cdr_o = CDR_O[CDW-1:0];
wire           pls;

// cycle control and status
reg            owr_cyc;  // cycle status
reg  [TDW-1:0] cnt;      // cycle counter

// port select
//generate if (OWN>1) begin : sel_declaration
reg  [SDW-1:0] owr_sel;
//end endgenerate

// modified input data for overdrive
wire           req_ovd;

// onewire signals
reg  [OWN-1:0] owr_pwr;  // power
reg            owr_ovd;  // overdrive
reg            owr_rst;  // reset
reg            owr_dat;  // data bit
reg            owr_smp;  // sample bit

reg            owr_oen;  // output enable
wire           owr_iln;  // input line

// interrupt signals
reg            irq_ena;  // interrupt enable
reg            irq_sts;  // interrupt status

// timing signals
wire [TDW-1:0] t_idl ;   // idle                 cycle    time
wire [TDW-1:0] t_rst ;   // reset                cycle    time
wire [TDW-1:0] t_bit ;   // data bit             cycle    time
wire [TDW-1:0] t_rstp;   // reset presence pulse sampling time
wire [TDW-1:0] t_rsth;   // reset                release  time
wire [TDW-1:0] t_dat0;   // data bit 0           release  time
wire [TDW-1:0] t_dat1;   // data bit 1           release  time
wire [TDW-1:0] t_bits;   // data bit             sampling time
wire [TDW-1:0] t_zero;   // end of               cycle    time

//////////////////////////////////////////////////////////////////////////////
// cycle timing
//////////////////////////////////////////////////////////////////////////////

// idle time
assign t_idl  = req_ovd ? T_IDLE_O[TDW-1:0]                         : T_IDLE_N[TDW-1:0];
// reset cycle time (reset low + reset hight)
assign t_rst  = req_ovd ? T_RSTL_O[TDW-1:0] + T_RSTH_O[TDW-1:0]     : T_RSTL_N[TDW-1:0] + T_RSTH_N[TDW-1:0];
// data bit cycle time (write 0 + recovery)
assign t_bit  = req_ovd ? T_DAT0_O[TDW-1:0] + T_RCVR_O[TDW-1:0]     : T_DAT0_N[TDW-1:0] + T_RCVR_N[TDW-1:0];

// reset presence pulse sampling time (reset high - reset presence)
assign t_rstp = owr_ovd ? T_RSTH_O[TDW-1:0] - T_RSTP_O[TDW-1:0]     : T_RSTH_N[TDW-1:0] - T_RSTP_N[TDW-1:0];
// reset      release time (reset high)
assign t_rsth = owr_ovd ? T_RSTH_O[TDW-1:0]                         : T_RSTH_N[TDW-1:0];

// data bit 0 release time (write bit 0 - write bit 0 + recovery)
assign t_dat0 = owr_ovd ? T_DAT0_O[TDW-1:0] - T_DAT0_O[TDW-1:0] + T_RCVR_O[TDW-1:0] : T_DAT0_N[TDW-1:0] - T_DAT0_N[TDW-1:0] + T_RCVR_N[TDW-1:0];
// data bit 1 release time (write bit 0 - write bit 1 + recovery)
assign t_dat1 = owr_ovd ? T_DAT0_O[TDW-1:0] - T_DAT1_O[TDW-1:0] + T_RCVR_O[TDW-1:0] : T_DAT0_N[TDW-1:0] - T_DAT1_N[TDW-1:0] + T_RCVR_N[TDW-1:0];
// data bit sampling time (write bit 0 - write bit 1 + recovery)
assign t_bits = owr_ovd ? T_DAT0_O[TDW-1:0] - T_BITS_O[TDW-1:0] + T_RCVR_O[TDW-1:0] : T_DAT0_N[TDW-1:0] - T_BITS_N[TDW-1:0] + T_RCVR_N[TDW-1:0];

// end of cycle time
assign t_zero = 'd0;

//////////////////////////////////////////////////////////////////////////////
// bus read
//////////////////////////////////////////////////////////////////////////////

// bus segnemt - controll/status register
assign bus_rdt_ctl_sts = {irq_ena, irq_sts, 1'b0, owr_pwr[0], owr_cyc, owr_ovd, owr_rst, owr_dat};

// bus segnemt - power and select register
generate
  if (BDW==32) begin
    if (OWN>1) begin
      assign bus_rdt_pwr_sel = {{16-OWN{1'b0}}, owr_pwr, 4'h0, {4-SDW{1'b0}}, owr_sel};
    end else begin
      assign bus_rdt_pwr_sel = 24'h0000_00;
    end
  end else if (BDW==8) begin
    if (OWN>1) begin
      assign bus_rdt_pwr_sel = {{ 4-OWN{1'b0}}, owr_pwr,       {4-SDW{1'b0}}, owr_sel};
    end else begin
      assign bus_rdt_pwr_sel = 8'hxx;
    end
  end
endgenerate

// bus read data
generate if (BDW==32) begin
  assign bus_rdt = (bus_adr[0]==1'b0) ? {bus_rdt_pwr_sel, bus_rdt_ctl_sts} : (cdr_o << 16 | cdr_n);
end else if (BDW==8) begin
  assign bus_rdt = (bus_adr[1]==1'b0) ? ((bus_adr[0]==1'b0) ? bus_rdt_ctl_sts
                                                            : bus_rdt_pwr_sel)
                                      : ((bus_adr[0]==1'b0) ? cdr_n
                                                            : cdr_o          );
end endgenerate

//////////////////////////////////////////////////////////////////////////////
// bus write
//////////////////////////////////////////////////////////////////////////////

// combined write/read enable and address decoder
generate if (BDW==32) begin
  assign bus_ren_ctl_sts = bus_ren & bus_adr[0] == 1'b0;
  assign bus_wen_ctl_sts = bus_wen & bus_adr[0] == 1'b0;
  assign bus_wen_pwr_sel = bus_wen & bus_adr[0] == 1'b0;
  assign bus_wen_cdr_n   = bus_wen & bus_adr[0] == 1'b1;
  assign bus_wen_cdr_o   = bus_wen & bus_adr[0] == 1'b1;
end else if (BDW==8) begin
  assign bus_ren_ctl_sts = bus_ren & bus_adr[1:0] == 2'b00;
  assign bus_wen_ctl_sts = bus_wen & bus_adr[1:0] == 2'b00;
  assign bus_wen_pwr_sel = bus_wen & bus_adr[1:0] == 2'b01;
  assign bus_wen_cdr_n   = bus_wen & bus_adr[1:0] == 2'b10;
  assign bus_wen_cdr_o   = bus_wen & bus_adr[1:0] == 2'b11;
end endgenerate

//////////////////////////////////////////////////////////////////////////////
// clock divider
//////////////////////////////////////////////////////////////////////////////

// clock divider ratio registers
generate
  if (CDR_E) begin
    if (BDW==32) begin
      always @ (posedge clk, posedge rst)
      if (rst) begin
        cdr_n <= CDR_N[CDW-1:0];
        cdr_o <= CDR_O[CDW-1:0];
      end else begin
        if (bus_wen_cdr_n)  cdr_n <= bus_wdt[15: 0];
        if (bus_wen_cdr_o)  cdr_o <= bus_wdt[31:16];
      end
    end else if (BDW==8) begin
      always @ (posedge clk, posedge rst)
      if (rst) begin
        cdr_n <= CDR_N[CDW-1:0];
        cdr_o <= CDR_O[CDW-1:0];
      end else begin
        if (bus_wen_cdr_n)  cdr_n <= bus_wdt;
        if (bus_wen_cdr_o)  cdr_o <= bus_wdt;
      end
    end
  end else begin
    initial begin
      cdr_n = CDR_N[CDW-1:0];
      cdr_o = CDR_O[CDW-1:0];
    end
  end
endgenerate

// clock divider
always @ (posedge clk, posedge rst)
if (rst)        div <= {CDW{1'd0}};
else begin
  if (bus_wen)  div <= {CDW{1'd0}};
  else          div <= pls ? {CDW{1'd0}} : div + owr_cyc;
end

// divided clock pulse
assign pls = (div == (owr_ovd ? cdr_o : cdr_n));

//////////////////////////////////////////////////////////////////////////////
// power and select register
//////////////////////////////////////////////////////////////////////////////

// select and power register implementation
generate if (OWN>1) begin : sel_implementation
  // port select
  always @ (posedge clk, posedge rst)
  if (rst)                   owr_sel <= {SDW{1'b0}};
  else if (bus_wen_pwr_sel)  owr_sel <= bus_wdt[(BDW==32 ?  8 : 0)+:SDW];
  
  // power delivery
  always @ (posedge clk, posedge rst)
  if (rst)                   owr_pwr <= {OWN{1'b0}};
  else if (bus_wen_pwr_sel)  owr_pwr <= bus_wdt[(BDW==32 ? 16 : 4)+:OWN];
end else begin
  // port select
  initial                    owr_sel <= 'd0; 
  // power delivery
  always @ (posedge clk, posedge rst)
  if (rst)                   owr_pwr <= 1'b0;
  else if (bus_wen_ctl_sts)  owr_pwr <= bus_wdt[4];
end endgenerate

//////////////////////////////////////////////////////////////////////////////
// interrupt logic
//////////////////////////////////////////////////////////////////////////////

// bus interrupt
assign bus_irq = irq_ena & irq_sts;

// interrupt enable
always @ (posedge clk, posedge rst)
if (rst)                   irq_ena <= 1'b0;     
else if (bus_wen_ctl_sts)  irq_ena <= bus_wdt[7]; 

// transmit status (active after onewire cycle ends)
always @ (posedge clk, posedge rst)
if (rst)                           irq_sts <= 1'b0;
else begin
  if (bus_wen_ctl_sts)             irq_sts <= 1'b0;
  else if (pls & (cnt == t_zero))  irq_sts <= 1'b1;
  else if (bus_ren_ctl_sts)        irq_sts <= 1'b0;
end

//////////////////////////////////////////////////////////////////////////////
// onewire state machine
//////////////////////////////////////////////////////////////////////////////

assign req_ovd = OVD_E ? bus_wen_ctl_sts & bus_wdt[2] : 1'b0; 

// overdrive
always @ (posedge clk, posedge rst)
if (rst)                   owr_ovd <= 1'b0;
else if (bus_wen_ctl_sts)  owr_ovd <= req_ovd;

// reset
always @ (posedge clk, posedge rst)
if (rst)                   owr_rst <= 1'b0;
else if (bus_wen_ctl_sts)  owr_rst <= bus_wdt[1];

// transmit data, reset, overdrive
always @ (posedge clk, posedge rst)
if (rst)                           owr_dat <= 1'b0;
else begin
  if (bus_wen_ctl_sts)             owr_dat <= bus_wdt[0];
  else if (pls & (cnt == t_zero))  owr_dat <= owr_smp;
end

// onewire cycle status
always @ (posedge clk, posedge rst)
if (rst)                           owr_cyc <= 1'b0;
else begin
  if (bus_wen_ctl_sts)             owr_cyc <= bus_wdt[3] & ~&bus_wdt[2:0];
  else if (pls & (cnt == t_zero))  owr_cyc <= 1'b0;
end

// state counter (initial value depends whether the cycle is reset or data)
always @ (posedge clk, posedge rst)
if (rst)                 cnt <= 0;
else begin
  if (bus_wen_ctl_sts)   cnt <= (&bus_wdt[1:0] ? t_idl : bus_wdt[1] ? t_rst : t_bit) - 1'd1;
  else if (pls)          cnt <= cnt - 1'd1;
end

// receive data (sampling point depends whether the cycle is reset or data)
always @ (posedge clk)
if (pls) begin
  if      ( owr_rst & (cnt == t_rstp))  owr_smp <= owr_iln;  // presence detect
  else if (~owr_rst & (cnt == t_bits))  owr_smp <= owr_iln;  // read data bit
end

// output register (switch point depends whether the cycle is reset or data)
always @ (posedge clk, posedge rst)
if (rst)                                owr_oen <= 1'b0;
else begin
  if (bus_wen_ctl_sts)                  owr_oen <= ~&bus_wdt[1:0];
  else if (pls) begin
    if      (owr_rst & (cnt == t_rsth)) owr_oen <= 1'b0;  // reset
    else if (owr_dat & (cnt == t_dat1)) owr_oen <= 1'b0;  // write 1, read
    else if (          (cnt == t_dat0)) owr_oen <= 1'b0;  // write 0
  end
end

//////////////////////////////////////////////////////////////////////////////
// IO
//////////////////////////////////////////////////////////////////////////////

// only one 1-wire line can be accessed at the same time
assign owr_e   = owr_oen << owr_sel;
// all 1-wire lines can be powered independently
assign owr_p   = owr_pwr;

// 1-wire line status read multiplexer
assign owr_iln = owr_i [owr_sel];

endmodule
